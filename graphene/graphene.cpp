/*  Command-line interface for the Graphene time series database.
*/

#define VERSION "2.4"

#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <dirent.h>

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include "db.h"
#include "dbpool.h"
#include "dbout.h"

#include <ext/stdio_filebuf.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

using namespace std;

/**********************************************************/
/* global parameters */
class Pars{
  public:
  string dbpath;       /* path to the databases */
  string dpolicy;      /* what to do with duplicated timestamps*/
  string sockname;     /* socket name*/
  bool interactive;    /* use interactive mode */
  vector<string> pars; /* non-option parameters */
  bool relative;       /* output relative times */

  // get options and parameters from argc/argv
  Pars(const int argc, char **argv){
    dbpath  = GRAPHENE_DEF_DBPATH;
    dpolicy = GRAPHENE_DEF_DPOLICY;
    interactive = false;
    relative  = false;
    if (argc<1) return; // needed for print_help()
    /* parse  options */
    int c;
    while((c = getopt(argc, argv, "+d:D:his:r"))!=-1){
      switch (c){
        case '?':
        case ':': throw Err(); /* error msg is printed by getopt*/
        case 'd': dbpath = optarg; break;
        case 'D': dpolicy = optarg; break;
        case 'h': print_help();
        case 'i': interactive = true; break;
        case 's': sockname = optarg; break;
        case 'r': relative = true; break;
      }
    }
    pars = vector<string>(argv+optind, argv+argc);
  }

  // print command list (used in both -h message and interactive mode help)
  void print_cmdlist(ostream & out){
    out <<  "  create <name> <data_fmt> <description>\n"
            "      -- create a database\n"
            "  delete <name>\n"
            "      -- delete a database\n"
            "  rename <old_name> <new_name>\n"
            "      -- rename a database\n"
            "  set_descr <name> <description>\n"
            "      -- change database description\n"
            "  info <name>\n"
            "      -- print database information, tab-separated time format,\n"
            "         data format and description (if it is not empty)\n"
            "  list\n"
            "      -- list all databases in the data folder\n"
            "  put <name> <time> <value1> ... <valueN>\n"
            "      -- write a data point\n"
            "  get <name>[:N] <time>\n"
            "      -- get previous or interpolated point\n"
            "  get_next <name>[:N] [<time1>]\n"
            "      -- get next point after time1\n"
            "  get_prev <name>[:N] [<time2>]\n"
            "      -- get previous point before time2\n"
            "  get_range <name>[:N] [<time1>] [<time2>] [<dt>]\n"
            "      -- get points in the time range\n"
            "  del <name> <time>\n"
            "      -- delete one data point\n"
            "  del_range <name> <time1> <time2>\n"
            "      -- delete all points in the time range\n"
            "  close        -- close all opened databases in interactive mode\n"
            "  close <name> -- close one database\n"
            "  sync         -- sync all opened databases\n"
            "  sync <name> -- sync one database\n"
            "  load <name> <file> -- create db and load file in a db_dump format\n"
            "  cmdlist -- print this list of commands\n"
            "  *idn?   -- print intentifier: Graphene database " << VERSION << "\n"
    ;
  }

  // print help message and exit
  void print_help(){
    Pars p(0, NULL); // default parameters
    cout << "graphene -- command line interface to Graphene time series database\n"
            "Usage: graphene [options] <command> <parameters>\n"
            "Options:\n"
            "  -d <path> -- database directory (default " << p.dbpath << "\n"
            "  -D <word> -- what to do with duplicated timestamps:\n"
            "               replace, skip, error, sshift, nsshift (default: " << p.dpolicy << ")\n"
            "  -h        -- write this help message and exit\n"
            "  -i        -- interactive mode, read commands from stdin\n"
            "  -s <name> -- socket mode: use unix socket <name> for communications\n"
            "  -r        -- output relative times (seconds from requested time) instead of absolute timestamps\n"
            "Commands:\n"
    ;
    print_cmdlist(cout);
    throw Err();
  }


  // get parameters from a string (for interactive mode)
  void parse_command_string(const string & str){
    istringstream in(str);
    pars.clear();
    while (1) {
      string a;
      in >> a;
      if (!in) break;
      pars.push_back(a);
    }
  }


  // Interactive mode.
  void run_interactive(std::istream & in, std::ostream & out){
    if (pars.size() !=0) throw Err() << "too many argumens for the interactive mode";
    string line;
    out << "#SPP001\n"; // command-line protocol, version 001.
    out << "Graphene database. Type cmdlist to see list of commands\n";
    out.flush();
    DBpool pool(dbpath);
    out << "#OK\n";
    out.flush();

    while (getline(in, line)){
      try {
        if (line=="") continue;
        parse_command_string(line);
        run_command(&pool, out);
        out << "#OK\n";
        out.flush();
      }
      catch(Err e){
        if (e.str()!="") out << "#Error: " << e.str() << "\n";
        out.flush();
      }
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
    DBpool pool(dbpath);
    run_command(&pool, cout);
  }

  // Run command, using parameters
  // For read/write commands time is transferred as a string
  // to db.put, db.get_* functions without change.
  // "now", "now_s" and "inf" strings can be used.
  void run_command(DBpool* pool, ostream & out){
    string cmd = pars[0];

    // create new database
    // args: create <name> [<data_fmt>] [<description>]
    if (strcasecmp(cmd.c_str(), "create")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      DBinfo info(
        pars.size()<3 ? DEFAULT_DATAFMT : DBinfo::str2datafmt(pars[2]),
        pars.size()<4 ? "": pars[3]);
      for (int i=4; i<pars.size(); i++) info.descr+=" "+pars[i];
      // todo: create folders if needed
      DBgr db = pool->get(pars[1], DB_CREATE | DB_EXCL);
      db.write_info(info);
      return;
    }

    // delete a database
    // args: delete <name>
    if (strcasecmp(cmd.c_str(), "delete")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>2) throw Err() << "too many parameters";
      pool->dbremove(pars[1]);
      return;
    }

    // rename a database
    // args: rename <old_name> <new_name>
    if (strcasecmp(cmd.c_str(), "rename")==0){
      if (pars.size()<3) throw Err() << "database old and new names expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      pool->dbrename(pars[1], pars[2]);
      return;
    }

    // change database description
    // args: set_descr <name> <description>
    if (strcasecmp(cmd.c_str(), "set_descr")==0){
      if (pars.size()<3) throw Err() << "database name and new description text expected";
      DBgr db = pool->get(pars[1]);
      DBinfo info = db.read_info();
      info.descr = pars[2];
      for (int i=3; i<pars.size(); i++) info.descr+=" "+pars[i];
      db.write_info(info);
      return;
    }

    // print database info
    // args: info <name>
    if (strcasecmp(cmd.c_str(), "info")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>2) throw Err() << "too many parameters";
      DBgr db = pool->get(pars[1], DB_RDONLY);
      DBinfo info = db.read_info();
      cout << DBinfo::datafmt2str(info.val);
      if (info.descr!="") out << '\t' << info.descr;
      out << "\n";
      return;
    }

    // print database list
    // args: list
    if (strcasecmp(cmd.c_str(), "list")==0){
      if (pars.size()>1) throw Err() << "too many parameters";
      DIR *dir = opendir(dbpath.c_str());
      if (!dir) throw Err() << "can't open database directory: " << strerror(errno);
      struct dirent *ent;
      while ((ent = readdir (dir)) != NULL) {
        string name(ent->d_name);
        size_t p = name.find(".db");
        if (name.size()>3 && p == name.size()-3)
          out << name.substr(0,p) << "\n";
      }
      closedir(dir);
      return;
    }

    // write data
    // args: put <name> <time> <value1> ...
    if (strcasecmp(cmd.c_str(), "put")==0){
      if (pars.size()<4) throw Err() << "database name, timestamp and some values expected";
      vector<string> dat;
      for (int i=3; i<pars.size(); i++) dat.push_back(string(pars[i]));
      // open database and write data
      DBgr db = pool->get(pars[1]);
      db.put(pars[2], dat, dpolicy);
      return;
    }

    // get next point after time1
    // args: get_next <name>[:N] [<time1>]
    if (strcasecmp(cmd.c_str(), "get_next")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t1 = pars.size()>2? pars[2]: "0";
      DBout dbo(dbpath, pars[1], out, relative ? t1:"");
      DBgr db = pool->get(dbo.name, DB_RDONLY);
      db.get_next(t1, dbo);
      return;
    }

    // get previous point before time2
    // args: get_prev <name>[:N] [<time2>]
    if (strcasecmp(cmd.c_str(), "get_prev")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t2 = pars.size()>2? pars[2]: "inf";
      DBout dbo(dbpath, pars[1], out, relative ? t2:"");
      DBgr db = pool->get(dbo.name, DB_RDONLY);
      db.get_prev(t2, dbo);
      return;
    }

    // get previous or interpolated point for the time
    // args: get <name>[:N] <time>
    if (strcasecmp(cmd.c_str(), "get")==0){
      if (pars.size()<2) throw Err() << "database name expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      string t2 = pars.size()>2? pars[2]: "inf";
      DBout dbo(dbpath, pars[1], out, relative ? t2:"");
      DBgr db = pool->get(dbo.name, DB_RDONLY);
      db.get(t2, dbo);
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
      DBout dbo(dbpath, pars[1], out, relative ? t1:"");
      DBgr db = pool->get(dbo.name, DB_RDONLY);
      db.get_range(t1,t2,dt, dbo);
      return;
    }

    // delete one data point
    // args: del <name> <time>
    if (strcasecmp(cmd.c_str(), "del")==0){
      if (pars.size()<3) throw Err() << "database name and time expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      DBgr db = pool->get(pars[1]);
      db.del(pars[2]);
      return;
    }

    // delete all points in the data range
    // args: del_range <name> <time1> <time2>
    if (strcasecmp(cmd.c_str(), "del_range")==0){
      if (pars.size()<4) throw Err() << "database name and two times expected";
      if (pars.size()>4) throw Err() << "too many parameters";
      DBgr db = pool->get(pars[1]);
      db.del_range(pars[2],pars[3]);
      return;
    }

    // close all opened databases in interactive mode
    // args: close
    if (strcasecmp(cmd.c_str(), "close")==0){
      if (pars.size()>2) throw Err() << "too many parameters";
      if (pars.size()==2) pool->close(pars[1]);
      else pool->close();
      return;
    }

    // close all opened databases in interactive mode
    // args: sync
    if (strcasecmp(cmd.c_str(), "sync")==0){
      if (pars.size()>2) throw Err() << "too many parameters";
      if (pars.size()==2) pool->sync(pars[1]);
      else pool->sync();
      return;
    }

    // create db and load file in db_dump format
    // (we can not use db_load because of user-defined comparison function)
    // args: load <name> <file>
    if (strcasecmp(cmd.c_str(), "load")==0){
      if (pars.size()<3) throw Err() << "database name and dump file expected";
      if (pars.size()>3) throw Err() << "too many parameters";
      DBgr db = pool->get(pars[1], DB_CREATE | DB_EXCL);
      db.load(pars[2]);
      return;
    }


    // print list of commands
    // args: cmdlist
    if (strcasecmp(cmd.c_str(), "cmdlist")==0){
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

  } catch(Err e){
    if (e.str()!="") cout << "#Error: " << e.str() << "\n";
    return 1;
  }
}
