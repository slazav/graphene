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

// Compare two packed time values, return +1,0,-1 if s1>s2,s1=s2,s1<s2
int
DBinfo::cmp_time_v1(const std::string & s1, const std::string & s2) const{
  uint64_t t1 = unpack_time_v1(s1);
  uint64_t t2 = unpack_time_v1(s2);
  if (t1==t2) return 0;
  return t1>t2 ? 1:-1;
}

// Is time equals zero?
bool
DBinfo::is_zero_time_v1(const std::string & s1) const {
  return unpack_time_v1(s1)==0;
}

// Add two packed time values, return packed string
string
DBinfo::add_time_v1(const std::string & s1, const std::string & s2) const{
  uint64_t t1 = unpack_time_v1(s1);
  uint64_t t2 = unpack_time_v1(s2);
  string ret(sizeof(uint64_t), '\0');
  *(uint64_t *)ret.data() = t1+t2;
  return ret;
}

// Subtract two packed time values, return number of seconds as a double value
double
DBinfo::time_diff_v1(const std::string & s1, const std::string & s2) const{
  int64_t t1 = unpack_time_v1(s1);
  int64_t t2 = unpack_time_v1(s2);
  string ret(sizeof(uint64_t), '\0');
  return (double)(t1-t2)/1000.0;
}

/********************************************************************/
// interpolation

// interpolate data (for FLOAT and DOUBLE values)
// s1 and s2 are _packed_ strings!
// k is a weight of first point, 0 <= k <= 1
//
string
DBinfo::interpolate_v1(
        const string & k0,
        const string & k1, const string & k2,
        const string & v1, const string & v2){

  // check for correct key size (do not parse DB info)
  if (k1.size()!=sizeof(uint64_t)) return "";
  if (k2.size()!=sizeof(uint64_t)) return "";

  // unpack time
  uint64_t t0 = unpack_time_v1(k0);
  uint64_t t1 = unpack_time_v1(k1);
  uint64_t t2 = unpack_time_v1(k2);

  // calculate first point weight
  uint64_t dt1 = max(t1,t0)-min(t1,t0);
  uint64_t dt2 = max(t2,t0)-min(t2,t0);
  double k = (double)dt2/(dt1+dt2);

  // check for correct value size
  size_t dsize = graphene_dtype_size(val);
  if (v1.size() % dsize != 0 || v2.size() % dsize != 0)
    throw Err() << "Broken database: wrong data length";

  // number of columns
  size_t cn1 = v1.size()/dsize;
  size_t cn2 = v2.size()/dsize;
  size_t cn0 = min(cn1,cn2);

  string v0(dsize*cn0, '\0');
  for (size_t i=0; i<cn0; i++){
    switch (val){
      case DATA_FLOAT:
        ((float*)v0.data())[i] = ((float*)v1.data())[i]*k
                               + ((float*)v2.data())[i]*(1-k);
        break;
      case DATA_DOUBLE:
        ((double*)v0.data())[i] = ((double*)v1.data())[i]*k
                                + ((double*)v2.data())[i]*(1-k);
        break;
      default: throw Err() << "Unexpected data format";
    }
  }
  return v0;
}

