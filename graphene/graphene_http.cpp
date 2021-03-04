/*  HTTP interface for the Graphene time series database.

  The server works with simple-json-datasource Grafana plugin.
  It ansers "GET /" and "OPTIONS" requests by itself, and transfers all
  POST requests into graphene_json module.

  Port and database location can be adjusted from the command line.

  microhttpd documentation:
  https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html

  simple-json-datasource documentation:
  https://github.com/grafana/simple-json-datasource
*/

#include <iostream>
#include <fstream>
#include <string>
#include <sys/wait.h> // wait
#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>
#include <microhttpd.h>
#include "json.h"
#include "err/err.h"
#include "log/log.h"
#include "getopt/getopt.h"

#include "dbpool.h"
#include "filter.h"

#if MHD_VERSION < 0x00097002
#define MHD_Result int
#endif

using namespace std;

/*************************************************/
// print help message
void usage(const GetOptSet & options, bool pod=false){
  HelpPrinter pr(pod, options, "graphene_http");
  pr.name("read-only HTTP interface to graphene databese");
  pr.usage("<options>");

  pr.head(1, "Options:");
  pr.opts({"GR"});
  throw Err();
}

/*************************************************/
// signal handler
static void
StopFunc(int signum){ throw 0; }

/**********************************************************/
/* libmicrohttpd callback for processing a requent. */
static MHD_Result
request_answer(void * cls, struct MHD_Connection * connection, const char * url,
               const char * method, const char * version,
               const char * upload_data, size_t * upload_data_size, void ** con_cls) {
  struct MHD_Response * response;
  static string in_data; // data recieved in POST requests
  int code = MHD_HTTP_OK;
  DBpool *pool = (DBpool *) cls; /* server parameters */

  Log(2) << "> " << method << " " << url << "\n";

  try {
    // simple-json interface: GET method with empty URL
    if (strcmp(method, "GET")==0 && strcmp(url, "/")==0){
      response = MHD_create_response_from_buffer(0,0,MHD_RESPMEM_MUST_COPY);
    }
    // simple-json interface: OPTIONS method
    else if (strcmp(method, "OPTIONS")==0){
      response = MHD_create_response_from_buffer(0,0,MHD_RESPMEM_MUST_COPY);
      MHD_add_response_header (response, "Access-Control-Allow-Headers", "accept, content-type");
      MHD_add_response_header (response, "Access-Control-Allow-Methods", "POST");
    }
    // simple-json interface: POST method
    else if (strcmp(method, "POST")==0){
      if (*con_cls == NULL){ // first connection - reset input data
        in_data = string();
        *con_cls = &in_data;
        return MHD_YES;
      }
      if (*upload_data_size){ // data came -- append to input data
        in_data += string(upload_data, upload_data + *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
      }
      else{ // Process the query by graphene_json() and answer
        string out_data;
        out_data = graphene_json(pool, url, in_data);

        Log(3) << ">>> " << in_data << "\n";
        Log(4) << "<<< " << out_data << "\n";

        response = MHD_create_response_from_buffer(
          out_data.size(), (void *)out_data.data(), MHD_RESPMEM_MUST_COPY);
        MHD_add_response_header (response, "Content-Type", "application/json");
        if (response==NULL) return MHD_NO;
      }
    }
    // GET with database name as an URL
    else if (strcmp(method, "GET")==0){
      int col=-1, flt=-1;

      const char * n   = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "name");
      const char * t1  = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "t1");
      const char * t2  = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "t2");
      const char * dt  = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "dt");
      const char *tfmt = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "tfmt");

      std::string cmd  = string(url).substr(1);
      std::string name = parse_ext_name(n? n:"", col, flt);

      DBoutS dbo;
      dbo.col    = col;
      dbo.flt    = flt;

      if (strcasecmp(cmd.c_str(),"get")==0){
         DBgr & db = pool->get(name, DB_RDONLY);
         db.timefmt = graphene_tfmt_parse(tfmt? tfmt : "def");
         db.time0   = t1 ? t1 : "0";
         db.get(t1? t1:"inf", dbo); }
      else if (strcasecmp(cmd.c_str(),"get_next")==0) {
         DBgr & db = pool->get(name, DB_RDONLY);
         db.timefmt = graphene_tfmt_parse(tfmt? tfmt : "def");
         db.time0   = t1 ? t1 : "0";
         db.get_next(t1? t1:"0", dbo); }
      else if (strcasecmp(cmd.c_str(),"get_prev")==0) {
         DBgr & db = pool->get(name, DB_RDONLY);
         db.timefmt = graphene_tfmt_parse(tfmt? tfmt : "def");
         db.time0   = t1 ? t1 : "0";
         db.get_prev(t1? t1:"inf", dbo); }
      else if (strcasecmp(cmd.c_str(),"get_range")==0){
         DBgr & db = pool->get(name, DB_RDONLY);
         db.timefmt = graphene_tfmt_parse(tfmt? tfmt : "def");
         db.time0   = t1 ? t1 : "0";
         db.get_range(t1? t1:"0", t2? t2:"inf", dt? dt:"0", dbo); }
      else if (strcasecmp(cmd.c_str(), "list")==0){
         for (auto const & n: pool->dblist()) dbo.print_point(n + "\n"); }
      else throw Err() << "bad command: " << cmd.c_str();

      string out_data = dbo.get_str();
      response = MHD_create_response_from_buffer(
          out_data.size(), (void *)out_data.data(), MHD_RESPMEM_MUST_COPY);
      MHD_add_response_header (response, "Content-Type", "text/plain");
    }
    else {
      throw Err() << "unknown HTTP request";
    }
  }
  catch (Err e) {
    response = MHD_create_response_from_buffer(
        e.str().length(), (void*)e.str().data(), MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, "Error", e.str().c_str());
    code = 400;
    // close all databases. In case of an error which needs recovery/reopening.
    pool->close();
  }

  // this allowes external grafana server make requests
  MHD_add_response_header (response, "Access-Control-Allow-Origin",  "*");

  auto ret = MHD_queue_response(connection, code, response);
  MHD_destroy_response(response);
  return ret;
}

struct MHD_Daemon *d = NULL;

/**********************************************************/
int main(int argc, char ** argv) {

  std::string logfile; // log file name
  std::string pidfile; // pidfile

  int ret; // return code
  bool mypid = false; // was the pidfile created by this process?

  try {

    // fill option structure
    GetOptSet options;
    options.add("dbpath",   1,'d', "GR", "database path (default: /var/lib/graphene/)");
    options.add("tcllib",   1,'T', "GR", "TCL library path (default: /usr/share/graphene/tcllib/)");
    options.add("env_type", 1,'E', "GR", "environment type: none, lock, txn "
       "(default: lock)");
    options.add("port",    1,'p', "GR", "TCP port for connections (default: 8081).");
    options.add("dofork",  0,'f', "GR", "Do fork and run as a daemon.");
    options.add("stop",    0,'S', "GR", "Stop running daemon (found by pid-file).");
    options.add("verbose", 1,'v', "GR", "Verbosity level: 0 - write nothing; "
      "1 - write some information on start; 2 - write information about connections; "
      "3 - write input data; 4 - write output data (default: 0).");
    options.add("logfile", 1,'l', "GR", "Log file, '-' for stdout. "
      "(default: /var/log/graphene_http.log in daemon mode, '-' in console mode.");
    options.add("pidfile", 1,'P', "GR", "Pid file "
      "(default: /var/run/graphene_http.pid)");
    options.add("help",    0,'h', "GR", "Print help message.");
    options.add("pod",     0,0,   "GR", "Print help message in POD format.");

    // parse options
    std::vector<std::string> nonopt;
    Opt opts = parse_options_all(&argc, &argv, options, {}, nonopt);
    if (nonopt.size()>0) throw Err()
      << "unexpected argument: " << nonopt[0];

    // print help message
    if (opts.exists("help")) usage(options);
    if (opts.exists("pod"))  usage(options,true);

    string dbpath   = opts.get("dbpath",   "/var/lib/graphene/");
    string tcllib   = opts.get("tcllib",   "/usr/share/graphene/tcllib/");
    string env_type = opts.get("env_type", "lock");
    DBpool pool(dbpath, true, env_type);

    int port    = opts.get("port",  8081);
    int verb    = opts.get("verbose", 0);
    logfile     = opts.get("logfile",  "");
    pidfile     = opts.get("pidfile", "/var/run/graphene_http.pid");
    bool stop   = opts.exists("stop");
    bool dofork = opts.exists("dofork");

    // load TCL library
    Filter::load_library(tcllib);

    // default log file
    if (logfile==""){
      if (dofork) logfile="/var/log/dev_server.log";
      else logfile="-";
    }
    Log::set_log_file(logfile);
    Log::set_log_level(verb);

    // stop running daemon
    if (stop) {
      std::ifstream pf(pidfile);
      if (pf.fail())
        throw Err() << "can't open pid-file: " << pidfile;
      int pid;
      pf >> pid;

      if (kill(pid, SIGTERM) == 0){
        // wait for process termination
        while (kill(pid, 0) == 0) usleep(1000);
      }
      else {
        if (errno == ESRCH){ // no such process, we should remove the pid-file
          remove(pidfile.c_str());
        }
        else {
          throw Err() << "can't stop dev_server process " << pid << ": "
                      << strerror(errno);
        }
      }
      return 0;
    }

    // check pid file
    {
      std::ifstream pf(pidfile);
      if (!pf.fail()){
        int pid;
        pf >> pid;
        throw Err() << "dev_server already runing (pid-file exists): " << pid;
      }
    }

    // set up daemon mode
    if (dofork){

      // Fork off the parent process
      pid_t pid, sid;
      pid = fork();
      if (pid < 0)
        throw Err() << "can't do fork";
      if (pid > 0){
        // write pidfile and exit
        std::ofstream pf(pidfile);
        if (pf.fail())
          throw Err() << "can't open pid-file: " << pidfile;
        pf << pid;
        Log(1) << "Starting dev_server in daemon mode, pid=" << pid;
        return 0;
      }
      mypid = true;

      // Change the file mode mask
      umask(022);

      // Create a new SID for the child process
      sid = setsid();
      if (sid < 0)
        throw Err() << "can't get SID";

      // // Change the current working directory
      // if ((chdir("/")) < 0)
      //   throw Err() << "can't do chdir";

      // Close out the standard file descriptors
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
    }

    // write pidfile
    else {
      pid_t pid = getpid();
      std::ofstream pf(pidfile);
      if (pf.fail())
        throw Err() << "can't open pid-file: " << pidfile;
      pf << pid;
      Log(1) << "Starting dev_server in console mode";
      mypid = true;
    }


    // start server
    d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
                         port, NULL, NULL,
                         &request_answer, &pool,
                         MHD_OPTION_END);
    if (d == NULL)
      throw Err() << "can't start the http server";

    // set up signals
    {
      struct sigaction sa;
      sa.sa_handler = StopFunc;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = SA_RESTART; // Restart functions if interrupted by handler
      if (sigaction(SIGTERM, &sa, NULL) == -1 ||
          sigaction(SIGQUIT, &sa, NULL) == -1 ||
          sigaction(SIGINT,  &sa, NULL) == -1 ||
          sigaction(SIGHUP,  &sa, NULL) == -1)
        throw Err() << "can't set signal handler";
    }

    Log(1) << "Starting the server:\n"
           << "  Port: " <<  port << "\n"
           << "  Pid file: " <<  pidfile << "\n"
           << "  Log file: " <<  logfile << "\n"
           << "  DB environment type: " <<  env_type << "\n"
           << "  Path to databases: " << dbpath << "\n";

    // main loop (to be interrupted by StopFunc)
    try{ while(1) sleep(10); }
    catch(int ret){}

    Log(1) << "Stopping HTTP server";
    ret=0;
  }

  catch (Err e){
    if (e.str()!="") Log(0) << "Error: " << e.str();
    ret = e.code();
    if (ret==-1) ret=1; // default code?!
  }

  if (d) MHD_stop_daemon(d);
  if (mypid && pidfile!= "") remove(pidfile.c_str()); // try to remove pid-file
  return ret;
}
