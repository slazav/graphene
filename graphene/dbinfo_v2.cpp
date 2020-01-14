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

// Pack timestamp (int64_t -> string as in DB)
string
DBinfo::pack_time_v2(const uint64_t t) const{
  if (t&0xFFFFFFFF){
    string ret(sizeof(uint64_t), '\0');
    *(uint64_t *)ret.data() = t;
    return ret;
  }
  else {
    string ret(sizeof(uint32_t), '\0');
    *(uint32_t *)ret.data() = t>>32;
    return ret;
  }
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

// Add two packed time values, return packed string
string
DBinfo::add_time_v2(const std::string & s1, const std::string & s2) const{
  uint64_t t1 = unpack_time_v2(s1);
  uint64_t t2 = unpack_time_v2(s2);
  uint64_t sum1 = (t1 >> 32) + (t2 >> 32);
  uint64_t sum2 = (t1 & 0xFFFFFFFF) + (t2 & 0xFFFFFFFF);
  if (sum1 >= ((uint64_t)1<<32) ) throw Err() << "add_time overfull";
  while (sum2 > 999999999) {sum2-=1000000000; sum1++;}
  return pack_time_v2((sum1<<32)+sum2);
}

