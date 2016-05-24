/*  HTTP interface for the Simple time series database.

  The server works with simple-json-datasource Grafana plugin.
  It ansers "GET /" and "OPTIONS" requests by itself, and transfers all
  POST requests into stsdb_json module.

  Port and database location can be adjusted from the command line.

  microhttpd documentation:
  https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html

  simple-json-datasource documentation:
  https://github.com/grafana/simple-json-datasource
*/

#include <iostream>
#include <string>

#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <microhttpd.h>
#include "json.h"

using namespace std;

/**********************************************************/
/* server parameters */
struct spars_t{
  int     port;   /* tcp port for connections */
  string  dbpath; /* path to the databases (default /var/lib/stsdb/) */
  int16_t verb;   /* print commands to stdout */

  /* set default values */
  spars_t(){
    port   = 8081;
    dbpath = "/var/lib/stsdb/";
    verb   = 0;
  }

  /* parse cmdline options */
  int parse_cmdline(int *argc, char ***argv){
    while(1){
      switch (getopt(*argc, *argv, "p:d:hv")){
        case -1: return 0; /* end*/
        case '?':
        case ':': continue; /* error msg is printed by getopt*/
        case 'p': port = atoi(optarg); break;
        case 'd': dbpath = optarg; break;
        case 'v': verb = 1; break;
        case 'h':
          cout << "stsdb_http -- http interface for stsdb\n"
                  "Usage: stsdb_http [options]\n"
                  "Options:p\n"
                  " -p <port> -- tcp port for connections (default 8081)\n"
                  " -d <path> -- database path (default /var/lib/stsdb/)\n"
                  " -v        -- be verbose\n"
                  " -h        -- write this help message and exit\n";
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

  if (spars->verb){
    cout << "> " << method << " " << url << "\n";
    if (upload_data && upload_data_size) fwrite(upload_data, *upload_data_size, 1, stdout);
    if (*upload_data_size) cout << "\n";
  }

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
  else{ // Process the query by stsdb_json() and answer
    string out_data = stsdb_json(spars->dbpath, url, in_data);
    response = MHD_create_response_from_buffer(
      out_data.size(), (void *)out_data.data(), MHD_RESPMEM_MUST_COPY);
    if (response==NULL) return MHD_NO;
    ret = MHD_add_response_header (response, "Access-Control-Allow-Origin",  "*");
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
  }
}

/**********************************************************/
int main(int argc, char ** argv) {
  struct MHD_Daemon * d;
  spars_t spars; /* server parameters*/

  /* parse server parameters, exit if server is not needed */
  if (spars.parse_cmdline(&argc, &argv)) return 0;

  d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
                       spars.port, NULL, NULL,
                       &request_answer, &spars,
                       MHD_OPTION_END);
  if (d != NULL){
    cout << "Server is running; press enter to stop it.\n";
    if (spars.verb){
      cout << "Port: " <<  spars.port << "\n";
      cout << "Path to databases: " << spars.dbpath << "\n";
    }
    (void) getc(stdin);
    MHD_stop_daemon(d);
  }
  else {
    cout << "Error: can't start the http server\n";
  }
  return 0;
}
