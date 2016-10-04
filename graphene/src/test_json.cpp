/*  Test stsdb_json functions */

#include <cstdio>
#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>
#include "json.h"

/* these functions are not in the h-file */
std::string convert_time(const std::string & tstr);
std::string convert_interval(const std::string & tstr);

int
main(){

  /* convert_time() */
  assert( convert_time("")   == "");
  assert( convert_time("2016-05-02T11:20:36.356A")  == "");
  assert( convert_time("2016-05-02T11:20:36.356")   == "");
  assert( convert_time("2016-05-02T11:20:36.356ZZ") == "");
  assert( convert_time("2016-05-02T11:20:36Z")      == "");
  assert( convert_time("2016-05-02T11:20:36.356Z")  == "1462188036356");


  /* convert_interval() */
  assert( convert_interval("")   == "");
  assert( convert_interval("1")  == "");
  assert( convert_interval("1.2ms") == "");
  assert( convert_interval("1ms") == "1");
  assert( convert_interval("1s")  == "1000");
  assert( convert_interval("1m")  == "60000");
  assert( convert_interval("1h")  == "3600000");
  assert( convert_interval("1d")  == "86400000");
  assert( convert_interval("11d") == "950400000");

}

