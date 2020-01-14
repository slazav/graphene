#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring> /* memset */
#include "dbgr.h"
#include <ctime>
#include <cmath>
#include <sys/time.h>
#include "err/err.h"


using namespace std;

/********************************************************************/
// Time handling

// Unpack timestamp (string as in DB -> int64_t)
uint64_t
DBinfo::unpack_time_v2(const string & s) const{
  if (s.size()==sizeof(uint64_t))
    return *(uint64_t *)s.data();
  if (s.size()==sizeof(uint32_t))
    return (uint64_t)(*(uint32_t *)s.data())<<32;
  throw Err() << "Broken database: wrong timestamp size: " << s.size();
}

// Print timestamp (DB packed string -> printed string)
std::string
DBinfo::print_time_v2(const string & s) const{
  uint64_t t = unpack_time_v2(s);
  std::ostringstream ss;
  ss << (t>>32);
  //if (t&0xFFFFFFFF)
  ss << "." << setw(9) << setfill('0') << (t&0xFFFFFFFF);
  return ss.str();
}

