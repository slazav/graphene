#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring> /* memset */
#include "db.h"
#include <ctime>
#include <cmath>
#include <sys/time.h>


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

// Parse timestemp from a string (time in seconds, words now, inf, +/- suffixes)
string
DBinfo::parse_time_v2(const string & ts) const{
  uint64_t t=0;
  if (strcasecmp(ts.c_str(), "now")==0){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    t = ((uint64_t)tv.tv_sec << 32) + (uint64_t)tv.tv_usec*1000;
  }
  else if (strcasecmp(ts.c_str(), "now_s")==0){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    t = (uint64_t)tv.tv_sec << 32;
  }
  else if (strcasecmp(ts.c_str(), "inf")==0){
    t = (uint64_t)((uint32_t)-1)<<32;
    t += (uint64_t)999999999;
  }
  else {
    int add=0;
    istringstream s;
    if (ts.size()>0 && (ts[ts.size()-1] == '+' || ts[ts.size()-1] == '-')){
      add = ts[ts.size()-1]=='+'? +1:-1;
      s.str(string(ts.begin(), ts.end()-1));
    }
    else s.str(ts);

    uint32_t t1=0, t2=0;
    s >> t1; // read seconds
    if (s.bad() || s.fail())
      throw Err() << "Bad timestamp: can't read seconds: " << ts;
    if (!s.eof()){
      char c;
      s >> c; // read decimal dot
      if (s.bad() || s.fail() || c!='.')
        throw Err() << "Bad timestamp: can't read decimal dot: " << ts;
      s >> c;
      int i=8;
      while (!s.eof()){
        if (c<'0'||c>'9')
          throw Err() << "Bad timestamp: can't read nanoseconds: " << ts;
        if (i>=0) t2 += (c-'0') * pow(10,i);
        s >> c;
        i--;
      }
    }
    t = ((uint64_t)t1<<32) + t2;
    if (t==0 && add<0){ // same as inf
      t = (uint64_t)((uint32_t)-1)<<32;
      t += (uint64_t)999999999;
    }
    else t+=add;
  }
  return pack_time_v2(t);
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

// Compare two packed time values, return +1,0,-1 if s1>s2,s1=s2,s1<s2
int
DBinfo::cmp_time_v2(const std::string & s1, const std::string & s2) const{
  uint64_t t1 = unpack_time_v2(s1);
  uint64_t t2 = unpack_time_v2(s2);
  if (t1==t2) return 0;
  return t1>t2 ? 1:-1;
}

// Is time equals zero?
bool
DBinfo::is_zero_time_v2(const std::string & s1) const {
  return unpack_time_v2(s1)==0;
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

// Subtract two packed time values, return number of seconds as a double value
double
DBinfo::time_diff_v2(const std::string & s1, const std::string & s2) const{
  uint64_t t1 = unpack_time_v2(s1);
  uint64_t t2 = unpack_time_v2(s2);

  // difference in seconds and in milliseconds
  int64_t d1 = (int64_t)(t1 >> 32) - (int64_t)(t2 >> 32);
  int64_t d2 = (int64_t)(t1 & 0xFFFFFFFF) - (int64_t)(t2 & 0xFFFFFFFF);

  return (double)d1 + (double)d2*1e-9;
}

/********************************************************************/
// interpolation

// interpolate data (for FLOAT and DOUBLE values)
// s1 and s2 are _packed_ strings!
// k is a weight of first point, 0 <= k <= 1
//
string
DBinfo::interpolate_v2(
        const string & k0,
        const string & k1, const string & k2,
        const string & v1, const string & v2){

  // check for correct key size (do not parse DB info)
  if (k1.size()!=sizeof(uint64_t) && k1.size()!=sizeof(uint32_t)) return "";
  if (k2.size()!=sizeof(uint64_t) && k2.size()!=sizeof(uint32_t)) return "";

  // unpack time
  uint64_t t0 = unpack_time_v2(k0);
  uint64_t t1 = unpack_time_v2(k1);
  uint64_t t2 = unpack_time_v2(k2);

  // point weights. No need to keep perfect accuracy
  double w1 = fabs( (double)(t1>>32) + (double)(t1&0xFFFFFFFF)/1e9
                  - (double)(t0>>32) - (double)(t0&0xFFFFFFFF)/1e9);
  double w2 = fabs( (double)(t2>>32) + (double)(t2&0xFFFFFFFF)/1e9
                  - (double)(t0>>32) - (double)(t0&0xFFFFFFFF)/1e9);
  double k = (double)w2/(w1+w2);

  // check for correct value size
  if (v1.size() % dsize() != 0 || v2.size() % dsize() != 0)
    throw Err() << "Broken database: wrong data length";

  // number of columns
  size_t cn1 = v1.size()/dsize();
  size_t cn2 = v2.size()/dsize();
  size_t cn0 = min(cn1,cn2);

  string v0(dsize()*cn0, '\0');
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

