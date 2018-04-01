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

void check_name(const std::string & name_){
  static const char *reject = ".:+| \n\t/";
  if (strcspn(name_.c_str(), reject)!=name_.length())
    throw Err() << "symbols '.:+| \\n\\t/' are not allowed in the database name: " << name_;
}

/********************************************************************/
// Time handling

// Parse timestemp from a string
string
DBinfo::parse_time(const string & ts) const{
  if      (version==1) return parse_time_v1(ts);
  else if (version==2) return parse_time_v2(ts);
  else throw Err() << "Unknown database version: " << version;
}

// Print timestamp
std::string
DBinfo::print_time(const string & s) const{
  if      (version==1) return print_time_v1(s);
  else if (version==2) return print_time_v2(s);
  else throw Err() << "Unknown database version: " << version;
}

// Compare two packed time values, return +1,0,-1 if s1>s2,s1=s2,s1<s2
int
DBinfo::cmp_time(const std::string & s1, const std::string & s2) const{
  if      (version==1) return cmp_time_v1(s1,s2);
  else if (version==2) return cmp_time_v2(s1,s2);
  else throw Err() << "Unknown database version: " << version;
}

// Is time equals zero?
bool
DBinfo::is_zero_time(const std::string & s1) const {
  if      (version==1) return is_zero_time_v1(s1);
  else if (version==2) return is_zero_time_v2(s1);
  else throw Err() << "Unknown database version: " << version;
}

// Add two packed time values, return packed string
string
DBinfo::add_time(const std::string & s1, const std::string & s2) const{
  if      (version==1) return add_time_v1(s1,s2);
  else if (version==2) return add_time_v2(s1,s2);
  else throw Err() << "Unknown database version: " << version;
}

// Add two packed time values, return packed string
double
DBinfo::time_diff(const std::string & s1, const std::string & s2) const{
  if      (version==1) return time_diff_v1(s1,s2);
  else if (version==2) return time_diff_v2(s1,s2);
  else throw Err() << "Unknown database version: " << version;
}

/********************************************************************/
// Data handling

// Parse data string according with data format.
// Output string is not a c-string!
// It is used as a convenient data storage, which
// can be easily converted into Berkleydb data.
// For text data all arguments are joined with ' ' separator.
// Quote argument if you want to keep original spaces!
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
  if (val == DATA_TEXT) return s;

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
  if      (version==1) return interpolate_v1(k0,k1,k2,v1,v2);
  else if (version==2) return interpolate_v2(k0,k1,k2,v1,v2);
  else throw Err() << "Unknown database version: " << version;
}

