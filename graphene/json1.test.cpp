/*  Json interface for the Graphene time series database.

  Usually json interface is used via graphene_http
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
#include "err/err.h"

using namespace std;

int
main(int argc, char *argv[]){
  if (argc != 3){
    cerr << "Usage: graphene_json <dbpath> <url> < <json_input> > <json_output>\n";
    return 1;
  }
  string dbpath(argv[1]);
  string url(argv[2]);

  string in_data;
  while (!cin.eof()){ string s; getline(cin, s); in_data +=s+'\n'; }

  try {
    DBpool pool(dbpath, true, "none");
    cout << graphene_json(&pool, url, in_data);
  }
  catch (Err e){
    cout << "Error: " << e.str();
    return 1;
  }
  return 0;
}
