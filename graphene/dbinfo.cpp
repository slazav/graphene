#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring> /* memset */
#include <ctime>
#include <sys/time.h>
#include <cmath>

#include "data.h"
#include "dbgr.h"
#include "err/err.h"

using namespace std;

void check_name(const std::string & name_){
  static const char *reject = ".:+| \n\t/";
  if (strcspn(name_.c_str(), reject)!=name_.length())
    throw Err() << "symbols '.:+| \\n\\t/' are not allowed in the database name: " << name_;
}

/********************************************************************/
// Time handling


// Print timestamp
std::string
DBinfo::print_time(const string & s) const{
  if      (version==1) return print_time_v1(s);
  else if (version==2) return print_time_v2(s);
  else throw Err() << "Unknown database version: " << (int)version;
}

// Add two packed time values, return packed string
string
DBinfo::add_time(const std::string & s1, const std::string & s2) const{
  if      (version==1) return add_time_v1(s1,s2);
  else if (version==2) return add_time_v2(s1,s2);
  else throw Err() << "Unknown database version: " << (int)version;
}

// Print data
string
DBinfo::print_data(const string & s, const int col) const{
  if (dtype == DATA_TEXT) return s;

  size_t dsize = graphene_dtype_size(dtype);

  if (s.size() % dsize != 0)
    throw Err() << "Broken database: wrong data length";
  // number of columns
  size_t cn = s.size()/dsize;
  // column range we want to show:
  size_t c1=0, c2=cn;
  if (col!=-1) { c1=col; c2=col+1; }

  ostringstream ostr;
  for (size_t i=c1; i<c2; i++){
    if (i>c1) ostr << ' ';
    if (i>=cn) { return "NaN"; }
    switch (dtype){
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

