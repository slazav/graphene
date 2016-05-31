/*  Json interface for the Simple time series database.

  Usually json interface is used via stsdb_http
  program. This small program is written for tests only

*/

#include <iostream>
#include <string>

#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>
#include "json.h"

using namespace std;

int
main(int argc, char *argv[]){
  if (argc != 3){
    cerr << "Usage: stsdb_json <dbpath> <url> < <json_input> > <json_output>\n";
    return 1;
  }
  string dbpath(argv[1]);
  string url(argv[2]);

  string in_data;
  while (!cin.eof()){ string s; getline(cin, s); in_data +=s+'\n'; }

  try {
    cout << stsdb_json(dbpath, url, in_data);
  }
  catch (Err e){
    cout << "Error:" << e.str();
    return 1;
  }
  return 0;
}
