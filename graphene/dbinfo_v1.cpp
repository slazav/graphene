#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring> /* memset */
#include <ctime>
#include <cmath>
#include <sys/time.h>

#include "dbgr.h"
#include "err/err.h"

using namespace std;

/********************************************************************/
// Time handling

// Unpack timestamp
uint64_t
DBinfo::unpack_time_v1(const string & s) const{
  if (s.size()!=sizeof(uint64_t))
    throw Err() << "Broken database: wrong timestamp size: " << s.size();
  return *(uint64_t *)s.data();
}

// Print timestamp
std::string
DBinfo::print_time_v1(const string & s) const{
  uint64_t t = unpack_time_v1(s);
  std::ostringstream ss;
  ss << t/1000;
  //if (t%1000)
  ss << "." << setw(9) << setfill('0') << (t%1000)*1000000;
  return ss.str();
}

