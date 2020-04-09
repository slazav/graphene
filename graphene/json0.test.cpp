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
  assert( convert_time("2016-05-02T11:20:36.356Z")  == "1462188036.356");
  assert( convert_time("2016-05-02T11:20:36.001Z")  == "1462188036.001");
  assert( convert_time("2016-05-02T11:20:36.100Z")  == "1462188036.100");
//  assert( convert_time("2016-05-02T11:20:36.10Z")  == "1462188036010");


  /* convert_interval() */
  assert( convert_interval("")   == "");
  assert( convert_interval("1")  == "");
  assert( convert_interval("1.2ms") == "");
  assert( convert_interval("1ms") == "0.001");
  assert( convert_interval("1s")  == "1.000");
  assert( convert_interval("1m")  == "60.000");
  assert( convert_interval("1h")  == "3600.000");
  assert( convert_interval("1d")  == "86400.000");
  assert( convert_interval("11d") == "950400.000");

}

