#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring> /* memset */
#include "db.h"
#include <ctime>
#include <sys/time.h>

using namespace std;

std::string check_name(const std::string & name){
  static const char *reject = ".:+| \n\t";
  if (strcspn(name.c_str(), reject)!=name.length())
    throw Err() << "symbols '.:+| \\n\\t' are not allowed in the database name";
  return name;
}

/********************************************************************/
// Time handling

// Pack timestamp according with time format.
string
DBinfo::pack_time_v1(const uint64_t t) const{
  string ret(sizeof(uint64_t), '\0');
  *(uint64_t *)ret.data() = t;
  return ret;
}
// Unpack timestamp
uint64_t
DBinfo::unpack_time_v1(const string & s) const{
  if (s.size()!=sizeof(uint64_t))
    throw Err() << "Broken database: wrong timestamp size: " << s.size();
  return *(uint64_t *)s.data();
}

// Parse timestemp from a string
string
DBinfo::parse_time(const string & ts) const{
  if (strcasecmp(ts.c_str(), "now")==0){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return pack_time_v1(
           (uint64_t)tv.tv_usec/1000
         + (uint64_t)tv.tv_sec*1000 );
  }
  if (strcasecmp(ts.c_str(), "now_s")==0){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return pack_time_v1((uint64_t)tv.tv_sec*1000);
  }
  if (strcasecmp(ts.c_str(), "inf")==0){
    return pack_time_v1((uint64_t)-1);
  }
  istringstream s(ts);
  uint64_t t;  s >> t;
  if (s.bad() || s.fail() || !s.eof())
    throw Err() << "Not a timestamp: " << ts;
  return pack_time_v1(t);
}

// Print timestamp
std::string
DBinfo::print_time(const string & s) const{
  uint64_t t = unpack_time_v1(s);
  std::ostringstream ss;
  ss << t;
  return ss.str();
}
// Compare two packed time values, return +1,0,-1 if s1>s2,s1=s2,s1<s2
int
DBinfo::cmp_time(const std::string & s1, const std::string & s2) const{
  if (s1.size()!=sizeof(uint64_t))
    throw Err() << "Broken database: wrong timestamp size: " << s1.size();
  if (s2.size()!=sizeof(uint64_t))
    throw Err() << "Broken database: wrong timestamp size: " << s2.size();
  uint64_t t1 = *(uint64_t *)s1.data();
  uint64_t t2 = *(uint64_t *)s2.data();
  if (t1==t2) return 0;
  return t1>t2 ? 1:-1;
}
// Is time equals zero?
bool
DBinfo::is_zero_time(const std::string & s1) const {
  if (s1.size()!=sizeof(uint64_t))
    throw Err() << "Broken database: wrong timestamp size: " << s1.size();
  return *(uint64_t *)s1.data()==0;
}
// Add two packed time values, return packed string
string
DBinfo::add_time(const std::string & s1, const std::string & s2) const{
  if (s1.size()!=sizeof(uint64_t))
    throw Err() << "Broken database: wrong timestamp size: " << s1.size();
  if (s2.size()!=sizeof(uint64_t))
    throw Err() << "Broken database: wrong timestamp size: " << s2.size();
  uint64_t t1 = *(uint64_t *)s1.data();
  uint64_t t2 = *(uint64_t *)s2.data();
  return pack_time_v1(t1+t2);
}

/********************************************************************/
// Data handling

// Parse data string according with data format.
// Output string is not a c-string!
// It is used as a convenient data storage, which
// can be easily converted into Berkleydb data.
string
DBinfo::parse_data(const vector<string> & strs) const{
  if (strs.size() < 1) throw Err() << "Some data expected";
  string ret;
  if (val == DATA_TEXT){ // text: join all data
    ret = strs[0];
    for (int i=1; i<strs.size(); i++)
      ret+= " " + strs[i];
  }
  else {    // numbers
    ret = string(dsize()*strs.size(), '\0');
    for (int i=0; i<strs.size(); i++){
      istringstream s(strs[i]);
      switch (val){
        case DATA_INT8:   s >> ((int8_t   *)ret.data())[i]; break;
        case DATA_UINT8:  s >> ((uint8_t  *)ret.data())[i]; break;
        case DATA_INT16:  s >> ((int16_t  *)ret.data())[i]; break;
        case DATA_UINT16: s >> ((uint16_t *)ret.data())[i]; break;
        case DATA_INT32:  s >> ((int32_t  *)ret.data())[i]; break;
        case DATA_UINT32: s >> ((uint32_t *)ret.data())[i]; break;
        case DATA_INT64:  s >> ((int64_t  *)ret.data())[i]; break;
        case DATA_UINT64: s >> ((uint64_t *)ret.data())[i]; break;
        case DATA_FLOAT:  s >> ((float    *)ret.data())[i]; break;
        case DATA_DOUBLE: s >> ((double   *)ret.data())[i]; break;
        default: throw Err() << "Unexpected data format";
      }
      if (s.bad() || s.fail() || !s.eof())
        throw Err() << "Can't put value into "
                    << datafmt2str(val) << " database: " << strs[i];
    }
  }
  return ret;
}

// Print data
string
DBinfo::print_data(const string & s, const int col) const{
  if (val == DATA_TEXT){
    // remove linebreaks
    int i;
    string ret=s;
    while ((i=ret.find('\n'))!=string::npos) ret[i] = ' ';
    return ret;
  }
  if (s.size() % dsize() != 0)
    throw Err() << "Broken database: wrong data length";
  // number of columns
  size_t cn = s.size()/dsize();
  // column range we want to show:
  size_t c1=0, c2=cn;
  if (col!=-1) { c1=col; c2=col+1; }

  ostringstream ostr;
  for (size_t i=c1; i<c2; i++){
    if (i>c1) ostr << ' ';
    if (i>=cn) { return "NaN"; }
    switch (val){
      case DATA_INT8:   ostr << ((int8_t   *)s.data())[i]; break;
      case DATA_UINT8:  ostr << ((uint8_t  *)s.data())[i]; break;
      case DATA_INT16:  ostr << ((int16_t  *)s.data())[i]; break;
      case DATA_UINT16: ostr << ((uint16_t *)s.data())[i]; break;
      case DATA_INT32:  ostr << ((int32_t  *)s.data())[i]; break;
      case DATA_UINT32: ostr << ((uint32_t *)s.data())[i]; break;
      case DATA_INT64:  ostr << ((int64_t  *)s.data())[i]; break;
      case DATA_UINT64: ostr << ((uint64_t *)s.data())[i]; break;
      // No loss of information happens if we convert float and double
      // numbers into strings with 9 and 17 significant digits.
      // We use one less digit to have round values (3.1415 instead of 3.1415000000000002)
      case DATA_FLOAT:  ostr << setprecision(8)  << ((float  *)s.data())[i]; break;
      case DATA_DOUBLE: ostr << setprecision(16) << ((double *)s.data())[i]; break;
      default: throw Err() << "Unexpected data format";
    }
  }
  return ostr.str();
}

/********************************************************************/
// interpolation

// interpolate data (for FLOAT and DOUBLE values)
// s1 and s2 are _packed_ strings!
// k is a weight of first point, 0 <= k <= 1
//
string
DBinfo::interpolate(
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

