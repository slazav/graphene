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

#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>
#include <microhttpd.h>
#include "json.h"

using namespace std;

/**********************************************************/
/* server parameters */
struct spars_t{
  int     port;    /* tcp port for connections */
  string  dbpath;  /* path to the databases (default /var/lib/graphene/) */
  string  logfile; /* logfile */
  string  pidfile; /* pidfile */
  int16_t verb;    /* print commands to stdout */
  int16_t dofork;  /* print commands to stdout */
  ostream *log;    /* log stream */
  ofstream flog;

  /* set default values */
  spars_t(){
    port   = 8081;
    dbpath = "/var/lib/graphene/";
    logfile = ""; // to be set later
    pidfile = "/var/run/graphene_http.pid";
    verb   = 1;
    dofork = 0;
    log    = &cout;
  }

  /* parse cmdline options */
  int parse_cmdline(int *argc, char ***argv){
    while(1){
      switch (getopt(*argc, *argv, "p:d:hv:f")){
        case -1: return 0; /* end*/
        case '?':
        case ':': continue; /* error msg is printed by getopt*/
        case 'p': port =  atoi(optarg); break;
        case 'd': dbpath  = optarg; break;
        case 'v': verb    = atoi(optarg); break;
        case 'l': logfile = optarg; break;
        case 'f': dofork  = 1; break;
        case 'h':
          cout << "graphene_http -- http interface for graphene\n"
                  "Usage: graphene_http [options]\n"
                  "Options:p\n"
                  " -p <port>  -- tcp port for connections (default 8081)\n"
                  " -d <path>  -- database path (default /var/lib/graphene/)\n"
                  " -v <level> -- be verbose\n"
                  "                0 - write nothing\n" 
                  "                1 - write some information on start\n" 
                  "                2 - write info about connections\n" 
                  "                3 - write input data\n" 
                  "                4 - write output data\n" 
                  " -l <file>  -- log file, use '-' for stdout\n"
                  "               (default /var/log/graphene.log in daemon mode, '-' in)\n"
                  " -f         -- do fork and run as a daemon\n"
                  " -h         -- write this help message and exit\n"
        ;
        return 1;
      }
    }
    return 0;
  }
};

/**********************************************************/
/* libmicrohttpd callback for processing a requent. */
static int request_answer(void * cls, struct MHD_Connection * connection, const char * url,
                          const char * method, const char * version,
                          const char * upload_data, size_t * upload_data_size, void ** con_cls) {
  struct MHD_Response * response;
  static string in_data; // data recieved in POST requests
  int ret;
  spars_t *spars = (spars_t *) cls; /* server parameters */

  if (spars->verb>1) *(spars->log) << "> " << method << " " << url << "\n";
  spars->log->flush();

  if (strcmp(method, "GET")==0 && strcmp(url, "/")==0){
    response = MHD_create_response_from_buffer(0,0,MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header (response, "Access-Control-Allow-Origin",  "*");
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
  }

  if (strcmp(method, "OPTIONS")==0){
    response = MHD_create_response_from_buffer(0,0,MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header (response, "Access-Control-Allow-Headers", "accept, content-type");
    MHD_add_response_header (response, "Access-Control-Allow-Methods", "POST");
    MHD_add_response_header (response, "Access-Control-Allow-Origin",  "*");
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
  }

  /* response only POST method */
  if (0 != strcmp(method, "POST")) return MHD_NO;
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
    try{
      out_data = graphene_json(spars->dbpath, url, in_data);
    }
    catch(Json::Err e){
      out_data = e.str();
      if (spars->verb>0) *(spars->log) << "Error: " << e.str() << "\n";
    }
    catch(Err e){
      out_data = std::string("{\"error_type\": \"graphene\", ") +
                             "\"error_message\":\"" + e.str() + "\"}";
      if (spars->verb>0) *(spars->log) << "Error: " << e.str() << "\n";
    }

    if (spars->verb>2) *(spars->log) << ">>> " << in_data << "\n";
    if (spars->verb>3) *(spars->log) << "<<< " << out_data << "\n";
    spars->log->flush();

    response = MHD_create_response_from_buffer(
      out_data.size(), (void *)out_data.data(), MHD_RESPMEM_MUST_COPY);
    if (response==NULL) return MHD_NO;
    ret = MHD_add_response_header (response, "Access-Control-Allow-Origin",  "*");
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
  }
}

spars_t spars; /* server parameters*/
struct MHD_Daemon *d = NULL;

static void srv_stop(int signum){
  MHD_stop_daemon(d);
  if (spars.verb >0)
    *(spars.log) << "Stopping the server\n";
  remove(spars.pidfile.c_str()); // try to remove pid-file
  throw 0;
}

/**********************************************************/
int main(int argc, char ** argv) {

  /* parse server parameters, exit if server is not needed */
  if (spars.parse_cmdline(&argc, &argv)) return 0;

  /*******************/
  /* open log file */
  if (spars.logfile==""){
    if (spars.dofork) spars.logfile="/var/log/graphene.log";
    else spars.logfile="-";
  }
  if (spars.logfile!="-"){
    spars.flog.open(spars.logfile.c_str(), ios::app);
    if (spars.flog.fail()){
      cerr << "Can't open log file: " << spars.logfile << "\n";
      return 1;
    }
    spars.log = &spars.flog;
  }

  /*******************/
  /* check pid file */
  {
    ifstream pf(spars.pidfile.c_str());
    if (!pf.fail()){
      int pid;
      pf >> pid;
      std::cerr << "Already runing (pid-file exists): " << pid << "\n";
      return 1;
    }
  }

  /*******************/
  // daemon things
  if (spars.dofork){
    pid_t pid, sid;
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
      cerr << "Can't do fork\n";
      return 1;
    }
    if (pid > 0) {
      // write pid file
      ofstream pf(spars.pidfile.c_str());
      if (pf.fail()){
        cerr << "Can't open pid-file: " << spars.pidfile << "\n";
        return 1;
      }
      pf << pid;
      return 0;
    }
    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
      cerr << "Can't get SID\n";
      return 1;
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
      std::cerr << "Can't do chdir\n";
      return 1;
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }

  /*******************/
  d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
                       spars.port, NULL, NULL,
                       &request_answer, &spars,
                       MHD_OPTION_END);
  if (d == NULL){
    *(spars.log) << "Error: can't start the http server\n";
    return 1;
  }

  /*******************/
  // set up signals
  struct sigaction sa;
  sa.sa_handler = srv_stop;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; /* Restart functions if
                               interrupted by handler */
  if (sigaction(SIGTERM, &sa, NULL) == -1 ||
      sigaction(SIGQUIT, &sa, NULL) == -1 ||
      sigaction(SIGINT,  &sa, NULL) == -1 ||
      sigaction(SIGHUP,  &sa, NULL) == -1){
    *(spars.log) << "Error: can't set signal handler\n";
    srv_stop(0); return 1;}

  if (spars.verb >0){
    *(spars.log) << "Starting the server:\n"
                 << "  Port: " <<  spars.port << "\n"
                 << "  Path to databases: " << spars.dbpath << "\n";
     spars.log->flush();
  }

  // main loop
  try{
    while(1) sleep(10);
  }
  catch(int ret){
    return ret;
  }
  return 0;
}
