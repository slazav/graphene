/*  Command-line interface for the Graphene time series database.
*/

#define VERSION "2.11"
#define GRAPHENE_DEF_ENV "lock"
#define GRAPHENE_DEF_DPOLICY "replace"
#define GRAPHENE_DEF_DBPATH  "."
#define GRAPHENE_DEF_TCLLIB  "/usr/share/graphene/tcllib/"

#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <cerrno>
#include <csignal>
#include <setjmp.h>

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include "gr_env.h"

#include "err/err.h"
#include "read_words/read_words.h"

#include <ext/stdio_filebuf.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

using namespace std;

// signal handler
jmp_buf sig_jmp_buf;
static void StopFunc(int signum){ longjmp(sig_jmp_buf, 0); }

// formatter callback for spp mode (see gr_env.h)
void
out_cb_spp(const std::string &t,  const std::vector<std::string> &d, void * cb_data){
  auto out = (std::ostream *)cb_data;

  // print values into a string (always \n in the end!)
  std::string s =  t;
  for (auto const & v:d) s += " " + v;
  s += "\n";

  *out << graphene_spp_text(s);
}

/**********************************************************/
/* global parameters */
class Pars{
  public:
  string dbpath;       /* path to the databases */
  string tcllib;       /* tcl library path */
  string dpolicy;      /* what to do with duplicated timestamps*/
  string env_type;     /* environment type (see gr_env.h)*/
  string sockname;     /* socket name*/
  bool interactive;    /* use interactive mode */
  vector<string> pars; /* non-option parameters */
  TimeFMT timefmt;     /* output time format */
  bool readonly;       /* open databases in read-only mode */

  // get options and parameters from argc/argv
  Pars(const int argc, char **argv){
    dbpath  = GRAPHENE_DEF_DBPATH;
    tcllib  = GRAPHENE_DEF_TCLLIB;
    dpolicy = GRAPHENE_DEF_DPOLICY;
    env_type = GRAPHENE_DEF_ENV;
    interactive = false;
    timefmt = TFMT_DEF;
    readonly  = false;
    if (argc<1) return; // needed for print_help()
    /* parse  options */
    int c;
    while((c = getopt(argc, argv, "+d:T:D:E:his:rR"))!=-1){
      switch (c){
        case '?':
        case ':': throw Err(); /* error msg is printed by getopt*/
        case 'd': dbpath = optarg; break;
        case 'T': tcllib = optarg; break;
        case 'D': dpolicy = optarg; break;
        case 'E': env_type = optarg; break;
        case 'h': print_help();
        case 'i': interactive = true; break;
        case 's': sockname = optarg; break;
        case 'r': timefmt  = TFMT_REL; break;
        case 'R': readonly = true; break;
      }
    }
    pars = vector<string>(argv+optind, argv+argc);

    // Set signal handler.
    // - It's important to close databases when
    //   program terminates. Without this databases
    //   (in a non-transactional env) can be easily broken.
    // - Do not throw c++ exceptions from the handler through libdb!
    //   This again can leave a database in a broken state.
    // - Currently I'm using jongjmp in the signal handler,
    //   resetting the jump point every time GrapheneEnv is
    //   created. This works much better then nothing, but
    //   I'm not sure that it is a good approach and catching
    //   a signal in every part of the program will be proccessed
    //   correctly.

    struct sigaction sa;
    sa.sa_handler = StopFunc;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart functions if interrupted by handler
    if (sigaction(SIGTERM, &sa, NULL) == -1 ||
        sigaction(SIGQUIT, &sa, NULL) == -1 ||
        sigaction(SIGABRT, &sa, NULL) == -1 ||
        sigaction(SIGHUP,  &sa, NULL) == -1 ||
        sigaction(SIGINT,  &sa, NULL) == -1)
      throw Err() << "can't set signal handler";
  }

  // print command list (used in both -h message and interactive mode help)
  void print_cmdlist(ostream & out){
    out <<  "  create <name> <data_fmt> <description> -- create a database\n"
            "  delete <name> -- delete a database\n"
            "  rename <old_name> <new_name> -- rename a database\n"
            "  set_descr <name> <description> -- set/change database description\n"
            "  set_filter <name> <N> <tcl code> -- set/change filter N\n"
            "  print_filter <name> <N> -- print code of the filter N\n"
            "  print_f0data <name> -- print data of the input filter\n"
            "  clear_f0data <name> -- clear data of the input filter\n"
            "  info <name> -- print database information, tab-separated time format,\n"
            "         data format and description (if it is not empty)\n"
            "  list -- list all databases in the data folder\n"
            "  put <name> <time> <value1> ... <valueN> -- write a data point\n"
            "  put_flt <name> <time> <value1> ... <valueN> -- write a data point using input filter (number 0)\n"
            "  get <name>[:N] <time> -- get previous or interpolated point\n"
            "  get_next <name>[:N] [<time1>] -- get next point after time1\n"
            "  get_prev <name>[:N] [<time2>] -- get previous point before time2\n"
            "  get_range <name>[:N] [<time1>] [<time2>] [<dt>] -- get points in the time range\n"
            "  get_wrange <name>[:N] [<time1>] [<time2>] [<dt>] -- do get_prev, get_range, get_next\n"
            "  get_count <name>[:N] [<time1>] [<cnt>] -- get up to cnt points starting from t1\n"
            "  del <name> <time> -- delete one data point\n"
            "  del_range <name> <time1> <time2> -- delete all points in the time range\n"
            "  close        -- close all opened databases in interactive mode\n"
            "  close <name> -- close one database\n"
            "  sync         -- sync all opened databases\n"
            "  sync <name> -- sync one database\n"
            "  load <name> <file> -- create db and load file in a db_dump format\n"
            "  dump <name> <file> -- dump the database into a file (same as db_dump utility)\n"
            "  list_dbs -- print environment database files for archiving (same as db_archive -s)"
            "  list_logs -- print environment log files (same as db_archive -l)"
            "  cmdlist -- print this list of commands\n"
            "  help -- same as cmdlist\n"
            "  *idn?   -- print intentifier: Graphene database " << VERSION << "\n"
            "  get_time -- print current time (unix seconds with microsecond precision)\n"
            "  libdb_version -- print libdb version\n"
            "  backup_start <name> -- notify that we are going to start backup, return backup timestamp.\n"
            "  backup_end <name> [<timestamp>] -- notify that backup is successfully finished\n"
            "  backup_reset <name> -- reset backup timer\n"
            "  backup_print <name> -- print backup timer\n"
            "  backup_list -- print all databases with finite backup timer\n"
            "\n"
            "For more information see https://github.com/slazav/graphene/\n"
    ;
  }

  // print help message and exit
  void print_help(){
    Pars p(0, NULL); // default parameters
    cout << "graphene -- command line interface to Graphene time series database\n"
            "Usage: graphene [options] <command> <parameters>\n"
            "Options:\n"
            "  -d <path> -- database directory (default " << p.dbpath << "\n"
            "  -T <path> -- TCL library path (default " << p.tcllib << "\n"
            "  -D <word> -- what to do with duplicated timestamps:\n"
            "               replace, skip, error, sshift, nsshift (default: " << p.dpolicy << ")\n"
            "  -E <word> -- environment type:\n"
            "               none, lock, txn (default: " << p.env_type << ")\n"
            "  -h        -- write this help message and exit\n"
            "  -i        -- interactive mode, read commands from stdin\n"
            "  -s <name> -- socket mode: use unix socket <name> for communications\n"
            "  -r        -- output relative times (seconds from requested time) instead of absolute timestamps\n"
            "  -R        -- read-only mode\n"
            "Commands:\n"
    ;
    print_cmdlist(cout);
    throw Err();
  }


  // Interactive mode.
  void run_interactive(std::istream & in, std::ostream & out){
    if (pars.size() !=0) throw Err() << "too many arguments for the interactive mode";
    string line;
    out << "#SPP001\n"; // command-line protocol, version 001.
    out << "Graphene database. Type cmdlist to see list of commands\n";
    out.flush();

    // Outer try -- exit on errors with #Error message
    // For SPP2 it should be #Fatal
    try {
      GrapheneEnv env(dbpath, readonly, env_type, tcllib);
      if (setjmp(sig_jmp_buf)) throw 0;
      out << "#OK\n";
      out.flush();

      while (1){
        // inner try -- continue to a new command with #Error message
        try {
          pars = read_words(in);
          if (pars.size()==0) break;
          run_command(&env, out);
          out << "#OK\n";
          out.flush();
        }
        catch(Err & e){
          if (e.str()!="") out << "#Error: " << e.str() << "\n";
          out.flush();
          // close all databases. In case of an error which needs recovery/reopening.
          env.close();
        }
      }
    }
    catch(Err & e){
      if (e.str()!="") out << "#Error: " << e.str() << "\n";
      return;
    }
    return;
  }

  // Socket mode
  void run_socket(const string & name){
    if (pars.size() !=0) throw Err() << "too many argumens for the socket mode";
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) throw Err() << "Can't create a socket";

    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, name.c_str());
    if (bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)))
      throw Err() << "can't bind socket to a file: " << name;

    listen(sock, 10);
    while (1) {

      // wait connection on the socket or a enter on stdin
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(sock, &rfds);
      FD_SET(0, &rfds);
      int ret = select(sock+1, &rfds, NULL, NULL, NULL);
      if (ret && FD_ISSET(0, &rfds)) break;
      if (ret == -1) throw Err() << "select error";

      // accept a connection
      int msgsock = accept(sock, 0, 0);
      if (msgsock == -1) throw Err() << "Socket accept error";

      // create in/out streams and run interactive mode
      __gnu_cxx::stdio_filebuf<char> filebuf_in(msgsock, std::ios::in);
      __gnu_cxx::stdio_filebuf<char> filebuf_out(msgsock, std::ios::out);
      ostream out(&filebuf_out);
      istream in(&filebuf_in);
      pars.clear();
      run_interactive(in, out);
    }
    close(sock);
    unlink(name.c_str());
  }


  // Cmdline mode.
  void run_cmdline(){
    if (pars.size() < 1) throw Err() << "command is expected";
    GrapheneEnv env(dbpath, readonly, env_type, tcllib);
    if (setjmp(sig_jmp_buf)) throw 0;
    run_command(&env, cout);
  }

  // Run command, using parameters
  // For read/write commands time is transferred as a string
  // to db.put, db.get_* functions without change.
  // "now", "now_s" and "inf" strings can be used.
  void run_command(GrapheneEnv* env, ostream & out){
    string cmd = pars[0];

    // print current time (unix seconds with ms precision)
    if (strcasecmp(cmd.c_str(), "get_time")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      struct timeval tv;
      gettimeofday(&tv, NULL);
      cout << tv.tv_sec << "." << setfill('0') << setw(6) << tv.tv_usec << "\n";
      return;
    }

    // print libdb version (<major>.<minor>.<patch>)
    if (strcasecmp(cmd.c_str(), "libdb_version")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      cout << db_version(NULL,NULL,NULL) << "\n";
      return;
    }

    // create new database
    // args: create <name> [<data_fmt>] [<description>]
    if (strcasecmp(cmd.c_str(), "create")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      DataType dtype = pars.size()<3 ? DATA_DOUBLE : graphene_dtype_parse(pars[2]);
      std::string descr = pars.size()<4 ? "": pars[3];
      for (int i=4; i<pars.size(); i++) descr+=" "+pars[i];
      env->dbcreate(pars[1], descr, dtype);
      return;
    }

    // delete a database
    // args: delete <name>
    if (strcasecmp(cmd.c_str(), "delete")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>2) throw Err() << "too many parameters";
      env->dbremove(pars[1]);
      return;
    }

    // rename a database
    // args: rename <old_name> <new_name>
    if (strcasecmp(cmd.c_str(), "rename")==0){
      if (pars.size()<3) throw Err() << "database old and new names expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      env->dbrename(pars[1], pars[2]);
      return;
    }

    // change database description
    // args: set_descr <name> <description>
    if (strcasecmp(cmd.c_str(), "set_descr")==0){
      if (pars.size()<3) throw Err() << "database name and new description text expected";
      std::string descr = pars[2];
      for (int i=3; i<pars.size(); i++) descr+=" "+pars[i];
      env->set_descr(pars[1], descr);
      return;
    }

    // print database info
    // args: info <name>
    if (strcasecmp(cmd.c_str(), "info")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>2) throw Err() << "too many parameters";
      auto dtype = env->get_dtype(pars[1]);
      auto descr = env->get_descr(pars[1]);
      cout << graphene_dtype_name(dtype);
      if (descr!="") out << '\t' << descr;
      out << "\n";
      return;
    }

    // print database list
    // args: list
    if (strcasecmp(cmd.c_str(), "list")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      auto names = env->dblist();
      for (auto const &n: names) out << n << "\n";
      return;
    }

    // backup start: notify that we are going to start backup.
    // - reset temporary backup timer
    // - return value of the main backup timer
    // args: backup_start <name>
    if (strcasecmp(cmd.c_str(), "backup_start")==0){
      if (pars.size()!=2) throw Err() << "database name expected";
      out << env->backup_start(pars[1]) << "\n";
      return;
    }

    // backup end: notify that backup is successfully done
    // - commit temporary backup timer into main one
    // args: backup_end <name> [<timestamp>]
    if (strcasecmp(cmd.c_str(), "backup_end")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t2 = pars.size()>2? pars[2]: "inf";
      env->backup_end(pars[1], t2);
      return;
    }

    // reset backup timer
    // args: backup_reset <name>
    if (strcasecmp(cmd.c_str(), "backup_reset")==0){
      if (pars.size()!=2) throw Err() << "database name expected";
      env->backup_reset(pars[1]);
      return;
    }

    // print backup timer
    // args: backup_print <name>
    if (strcasecmp(cmd.c_str(), "backup_print")==0){
      if (pars.size()!=2) throw Err() << "database name expected";
      out << env->backup_get(pars[1]) << "\n";
      return;
    }

    // list all databases with finite backup timer
    // args: backup_list
    if (strcasecmp(cmd.c_str(), "backup_list")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      for (auto const &n: env->dblist()){
        if (env->backup_needed(n)) out << n << "\n";
      }
      return;
    }

    // write data
    // args: put <name> <time> <value1> ...
    if (strcasecmp(cmd.c_str(), "put")==0){
      if (pars.size()<4) throw Err() << "database name, timestamp and some values expected";
      vector<string> dat(pars.begin()+3, pars.end());
      env->put(pars[1], pars[2], dat, dpolicy);
      return;
    }

    // write data using input filter
    // args: put_flt <name> <time> <value1> ...
    if (strcasecmp(cmd.c_str(), "put_flt")==0){
      if (pars.size()<4) throw Err() << "database name, timestamp and some values expected";
      vector<string> dat(pars.begin()+3, pars.end());
      env->put_flt(pars[1], pars[2], dat, dpolicy);
      return;
    }

    // get next point after time1
    // args: get_next <name>[:N] [<time1>]
    if (strcasecmp(cmd.c_str(), "get_next")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t1 = pars.size()>2? pars[2]: "0";
      env->get_next(pars[1], t1, timefmt,
                    interactive? out_cb_spp: out_cb_simple, &out);
      return;
    }

    // get previous point before time2
    // args: get_prev <name>[:N] [<time2>]
    if (strcasecmp(cmd.c_str(), "get_prev")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t2 = pars.size()>2? pars[2]: "inf";
      env->get_prev(pars[1], t2, timefmt,
                    interactive? out_cb_spp: out_cb_simple, &out);
      return;
    }

    // get previous or interpolated point for the time
    // args: get <name>[:N] <time>
    if (strcasecmp(cmd.c_str(), "get")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t2 = pars.size()>2? pars[2]: "inf";
      env->get(pars[1], t2, timefmt,
               interactive? out_cb_spp: out_cb_simple, &out);
      return;
    }

    // get data range
    // args: get_range <name>[:N] [<time1>] [<time2>] [<dt>]
    if (strcasecmp(cmd.c_str(), "get_range")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>5) throw Err() << "too many parameters";
      string t1 = pars.size()>2? pars[2]: "0";
      string t2 = pars.size()>3? pars[3]: "inf";
      string dt = pars.size()>4? pars[4]: "0";
      env->get_range(pars[1], t1,t2,dt, timefmt,
                     interactive? out_cb_spp: out_cb_simple, &out);
      return;
    }

    // get data wide range (get_prev, get_range, get_next)
    // args: get_wrange <name>[:N] [<time1>] [<time2>] [<dt>]
    if (strcasecmp(cmd.c_str(), "get_wrange")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>5) throw Err() << "too many parameters";
      string t1 = pars.size()>2? pars[2]: "0";
      string t2 = pars.size()>3? pars[3]: "inf";
      string dt = pars.size()>4? pars[4]: "0";
      env->get_wrange(pars[1], t1,t2,dt, timefmt,
                     interactive? out_cb_spp: out_cb_simple, &out);
      return;
    }

    // get limited number of points
    // args: get_count <name>[:N] [<time1>] [<cnt>]
    if (strcasecmp(cmd.c_str(), "get_count")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>5) throw Err() << "too many parameters";
      string t1  = pars.size()>2? pars[2]: "0";
      string cnt = pars.size()>3? pars[3]: "1000";
      env->get_count(pars[1], t1,cnt, timefmt,
                     interactive? out_cb_spp: out_cb_simple, &out);
      return;
    }

    // delete one data point
    // args: del <name> <time>
    if (strcasecmp(cmd.c_str(), "del")==0){
      if (pars.size()<3) throw Err() << "database name and time expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      env->del(pars[1],pars[2]);
      return;
    }

    // delete all points in the data range
    // args: del_range <name> <time1> <time2>
    if (strcasecmp(cmd.c_str(), "del_range")==0){
      if (pars.size()<4) throw Err() << "database name and two times expected";
      if (pars.size()>4) throw Err() << "too many parameters";
      env->del_range(pars[1], pars[2], pars[3]);
      return;
    }

    // close all opened databases in interactive mode
    // args: close
    if (strcasecmp(cmd.c_str(), "close")==0){
      if (pars.size()>2) throw Err() << "too many parameters";
      if (pars.size()==2) env->close(pars[1]);
      else env->close();
      return;
    }

    // close all opened databases in interactive mode
    // args: sync
    if (strcasecmp(cmd.c_str(), "sync")==0){
      if (pars.size()>2) throw Err() << "too many parameters";
      if (pars.size()==2) env->sync(pars[1]);
      else env->sync();
      return;
    }

    // create db and load file in db_dump format
    // (we can not use db_load because of user-defined comparison function)
    // args: load <name> <file>
    if (strcasecmp(cmd.c_str(), "load")==0){
      if (pars.size()<3) throw Err() << "database name and dump file expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      GrapheneEnv simple_env(dbpath, false, "none", "");
      if (setjmp(sig_jmp_buf)) throw 0;
      simple_env.load(pars[1], pars[2]);
      return;
    }

    // dump the database into a file (same as db_dump utility)
    // args: dump <name> <file>
    if (strcasecmp(cmd.c_str(), "dump")==0){
      if (pars.size()<3) throw Err() << "database name and dump file expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      GrapheneEnv simple_env(dbpath, false, "none", "");
      if (setjmp(sig_jmp_buf)) throw 0;
      simple_env.dump(pars[1], pars[2]);
      return;
    }

    // set filter
    // args: set_filter <name> <N> <tcl script>
    if (strcasecmp(cmd.c_str(), "set_filter")==0){
      if (pars.size()!=4) throw Err() << "database name, filter number, filter code expected";
      int N = str_to_type<int>(pars[2]);
      if (N<0 || N>=MAX_FILTERS) throw Err() << "filter number out of range: " << N;
      env->set_filter(pars[1], N, pars[3]);
      return;
    }

    // print filter
    // args: print_filter <name> <N>
    if (strcasecmp(cmd.c_str(), "print_filter")==0){
      if (pars.size()!=3) throw Err() << "database name and filter number expected";
      int N = str_to_type<int>(pars[2]);
      if (N<0 || N>=MAX_FILTERS) throw Err() << "filter number out of range: " << N;
      out << env->get_filter(pars[1], N) << "\n";
      return;
    }

    // print input filter data
    // args: print_f0data <name>
    if (strcasecmp(cmd.c_str(), "print_f0data")==0){
      if (pars.size()!=2) throw Err() << "database name expected";
      out << env->get_f0data(pars[1]) << "\n";
      return;
    }

    // clearinput filter data
    // args: clear_f0data <name>
    if (strcasecmp(cmd.c_str(), "clear_f0data")==0){
      if (pars.size()!=2) throw Err() << "database name expected";
      env->clear_f0data(pars[1]);
      return;
    }

    // print environment database files for archiving (same as db_archive -s)
    // args: list_dbs
    if (strcasecmp(cmd.c_str(), "list_dbs")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      if (env_type != "txn") throw Err() << "list_dbs can not by run in this environment type: " << env_type;
      env->list_dbs();
      return;
    }

    // print environment log files (same as db_archive -l)
    // args: list_logs
    if (strcasecmp(cmd.c_str(), "list_logs")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      if (env_type != "txn") throw Err() << "list_logs can not by run in this environment type: " << env_type;
      env->list_logs();
      return;
    }

    // print list of commands
    // args: cmdlist
    if (strcasecmp(cmd.c_str(), "cmdlist")==0 || strcasecmp(cmd.c_str(), "help")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      print_cmdlist(out);
      return;
    }

    // print id string
    // args: *idn?
    if (strcasecmp(cmd.c_str(), "*idn?")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      out << "Graphene database " << VERSION << "\n";
      return;
    }

    // unknown command
    throw Err() << "Unknown command: " << cmd;
  }

};


/**********************************************************/
int
main(int argc, char **argv) {

  try {
    Pars p(argc, argv);
    if (p.interactive) p.run_interactive(cin, cout);
    else if (p.sockname!="") p.run_socket(p.sockname);
    else p.run_cmdline();

  } catch(Err & e){
    if (e.str()!="") cout << "Error: " << e.str() << "\n";
    return 1;
  } catch(int r){
    return 1;
  }
  return 0;
}
