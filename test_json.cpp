/*  Test stsdb_json functions */

#include <cstdio>
#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>
#include "json.h"

/* these functions are not in the h-file */
uint64_t convert_time(const std::string & tstr);
uint64_t convert_interval(const std::string & tstr);

int
main(){

  /* convert_time() */
  assert( convert_time("")   == 0);
  assert( convert_time("2016-05-02T11:20:36.356A")  == 0);
  assert( convert_time("2016-05-02T11:20:36.356")   == 0);
  assert( convert_time("2016-05-02T11:20:36.356ZZ") == 0);
  assert( convert_time("2016-05-02T11:20:36Z")      == 0);
  assert( convert_time("2016-05-02T11:20:36.356Z")  == 1462188036356);


  /* convert_interval() */
  assert( convert_interval("")   == 0);
  assert( convert_interval("1")  == 0);
  assert( convert_interval("1.2ms") == 0);
  assert( convert_interval("1ms") == 1);
  assert( convert_interval("1s")  == 1000);
  assert( convert_interval("1m")  == 60*1000);
  assert( convert_interval("1h")  == 3600*1000);
  assert( convert_interval("1d")  == 24*3600*1000);
  assert( convert_interval("11d") == 11*24*3600*1000);

}

