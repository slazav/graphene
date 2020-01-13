#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>
#include <sys/time.h>

#include "err/err.h"
#include "data.h"

/*
### Graphene data types

Each graphene record consists of two components: timestamp and data.
Timestamp type is fixed and depends on the database version.
Data type can be chosen when the database is created.

Following data types are supported:

* TEXT -- Non-empty text data of arbitrary length (1 or more bytes). Note
that graphene reads input command as a sequence of space-separated
arguments. Text data is assembled by joining arguments with a single
space between them. To submit text data as a single piece quote your
arguments.

* INT8, UINT8, INT16, UINT16, INT32, UINT32, INT64, UINT64 -- Integer types.
Data consists of 1 or more integer numbers of a certain type. Storage
size is 1,2,4, or 8 bytes per number depending on the type.

* FLOAT, DOUBLE -- Floating-point types. Data consists of 1 or more numbers
of a certain type. Storage size is 4 or 8 bytes per number depending on
the type. Nan, -inf, and +inf values are supported.

Changes/fixed errors since graphene 2.8:
- parsing INT8 and UINT8 data
- empty text is allowed
*/

DataType
graphene_parse_dtype(const std::string & s){
  if (strcasecmp(s.c_str(), "TEXT")==0)   return DATA_TEXT;
  if (strcasecmp(s.c_str(), "INT8")==0)   return DATA_INT8;
  if (strcasecmp(s.c_str(), "UINT8")==0)  return DATA_UINT8;
  if (strcasecmp(s.c_str(), "INT16")==0)  return DATA_INT16;
  if (strcasecmp(s.c_str(), "UINT16")==0) return DATA_UINT16;
  if (strcasecmp(s.c_str(), "INT32")==0)  return DATA_INT32;
  if (strcasecmp(s.c_str(), "UINT32")==0) return DATA_UINT32;
  if (strcasecmp(s.c_str(), "INT64")==0)  return DATA_INT64;
  if (strcasecmp(s.c_str(), "UINT64")==0) return DATA_UINT64;
  if (strcasecmp(s.c_str(), "FLOAT")==0)  return DATA_FLOAT;
  if (strcasecmp(s.c_str(), "DOUBLE")==0) return DATA_DOUBLE;
  throw Err() << "Unknown data type: " << s;
}

std::string
graphene_dtype_name(const DataType dtype){
  switch (dtype) {
    case DATA_TEXT:    return "TEXT";
    case DATA_INT8:    return "INT8";
    case DATA_UINT8:   return "UINT8";
    case DATA_INT16:   return "INT16";
    case DATA_UINT16:  return "UINT16";
    case DATA_INT32:   return "INT32";
    case DATA_UINT32:  return "UINT32";
    case DATA_INT64:   return "INT64";
    case DATA_UINT64:  return "UINT64";
    case DATA_FLOAT:   return "FLOAT";
    case DATA_DOUBLE:  return "DOUBLE";
    default: throw Err() << "Unknown data type: " << dtype;
  }
}

size_t
graphene_dtype_size(const DataType dtype){
  switch (dtype) {
    case DATA_TEXT:    return 1;
    case DATA_INT8:    return 1;
    case DATA_UINT8:   return 1;
    case DATA_INT16:   return 2;
    case DATA_UINT16:  return 2;
    case DATA_INT32:   return 4;
    case DATA_UINT32:  return 4;
    case DATA_INT64:   return 8;
    case DATA_UINT64:  return 8;
    case DATA_FLOAT:   return 4;
    case DATA_DOUBLE:  return 8;
    default: throw Err() << "Unknown data type: " << dtype;
  }
}

/********************************************************************/

std::string
graphene_parse_data(const std::vector<std::string> & strs, const DataType dtype){

  // TEXT: join all data
  if (dtype == DATA_TEXT){
    std::string ret;
    for (size_t i=0; i<strs.size(); i++){
      if (i>0) ret+=" ";
      ret+= strs[i];
    }
    return ret;
  }

  if (strs.size() < 1) throw Err() << "Some data expected";

  // numbers
  {
    std::string ret = std::string(graphene_dtype_size(dtype)*strs.size(), '\0');
    for (int i=0; i<strs.size(); i++){

      std::istringstream s(strs[i]);
      std::string tmp;
      int ii;
      switch (dtype){

        case DATA_INT8:
          // We can not read directly to int8_t because a byte
          // will be read, not number. Let's read to larger
          // integer, and check limits manually.
          s >> ii;
          if (ii<-128 || ii>127) throw Err()
            << "Bad INT8 value: "  << strs[i];
          ((int8_t *)ret.data())[i] = ii;
          break;

        case DATA_UINT8:
          // Same as with DATA_INT8
          s >> ii;
          if (ii<0 || ii>255) throw Err()
            << "Bad UINT8 value: " << strs[i];
          ((uint8_t *)ret.data())[i] = ii;
          break;

        case DATA_INT16:
          // For int16 this is enough. Limits are detected properly.
          s >> ((int16_t *)ret.data())[i];
          break;

        case DATA_UINT16:
          // We have to check manually that input is non-negative.
          if (s.peek()=='-') throw Err()
            << "Bad UINT16 value: " << strs[i];
          s >> ((uint16_t *)ret.data())[i];
          break;

        case DATA_INT32:
          s >> ((int32_t *)ret.data())[i];
          break;

        case DATA_UINT32:
          if (s.peek()=='-') throw Err()
            << "Bad UINT32 value: " << strs[i];
          s >> ((uint32_t *)ret.data())[i];
          break;

        case DATA_INT64:
          // There is a question about -2^63. It can be put
          // to the database and printed back, but it can
          // not be written as -9223372036854775808l
          s >> ((int64_t *)ret.data())[i];
          break;

        case DATA_UINT64:
          if (s.peek()=='-') throw Err()
            << "Bad UINT64 value: " << strs[i];
          s >> ((uint64_t *)ret.data())[i]; break;

        case DATA_FLOAT:
          if (strcasecmp(strs[i].c_str(),"inf")==0 ||
              strcasecmp(strs[i].c_str(),"+inf")==0) {
            ((float*)ret.data())[i] = +INFINITY;
            getline(s, tmp); break;
          }
          if (strcasecmp(strs[i].c_str(),"-inf")==0) {
            ((float*)ret.data())[i] = -INFINITY;
            getline(s, tmp); break;
          }
          if (strcasecmp(strs[i].c_str(),"nan")==0) {
            ((float*)ret.data())[i] = NAN;
            getline(s, tmp); break;
          }
          s >> ((float *)ret.data())[i]; break;

        case DATA_DOUBLE:
          if (strcasecmp(strs[i].c_str(),"inf")==0 ||
              strcasecmp(strs[i].c_str(),"+inf")==0) {
            ((double*)ret.data())[i] = +INFINITY;
            getline(s, tmp); break;
          }
          if (strcasecmp(strs[i].c_str(),"-inf")==0) {
            ((double*)ret.data())[i] = -INFINITY;
            getline(s, tmp); break;
          }
          if (strcasecmp(strs[i].c_str(),"nan")==0) {
            ((double*)ret.data())[i] = NAN;
            getline(s, tmp); break;
          }
          s >> ((double*)ret.data())[i]; break;
        default: throw Err() << "Unexpected data format";
      }

      if (s.bad() || s.fail() || !s.eof())
        throw Err() << "Bad " << graphene_dtype_name(dtype) << " value: " << strs[i];
    }
    return ret;
  }
}

/********************************************************************/
/*
### Graphene time types

Each graphene record consists of two components: timestamp and data.
Timestamp type is fixed and depends on the database version.
Data type can be chosen when the database is created.

Following time types are supported:

* TIME_V1 -- Used in old graphene databases for timestamps. A 64-bit
unsigned integer which is unix time in milliseconds. If used as data
one or more timestamps can be packed together.

When parsing user input following special values are supported:
 - inf -- largest possible timestamp,
 - now -- current time with millisecond precision
 - now_s -- current time with 1s precision

* TIME_V2 -- currently used in graphene databases for timestamps. One or
two 32-bit unsigned integers. First one is unix time in seconds,
opional second one is amount on nanoseconds (0..999999999). If used as data
more then one timestamps can be packed together. In this case each timstamp
occupies 8 bytes, except the last one which may have 4 bytes if
its nanosecond field is empty. Maximum possible value of the timestamp
corresponds to February 2106.

When parsing user input following values are supported:
 - `<seconds>[.<fractional part>][+|-]` - numerical timestamp,
    optional +/- suffix adds/subtracts 1ns.
 - `inf` -- largest possible timestamp, 4294967295.999999999
 - `now` -- current time with nanosecond precision
 - `now_s` -- current time with second precision

Changes/fixed errors since graphene 2.8:
- +/- modifiers with crossing 1-second boundary
*/

TimeType
graphene_parse_ttype(const std::string & s){
  if (strcasecmp(s.c_str(), "TIME_V1")==0) return TIME_V1;
  if (strcasecmp(s.c_str(), "TIME_V2")==0) return TIME_V2;
  throw Err() << "Unknown time type: " << s;
}

std::string
graphene_ttype_name(const TimeType ttype){
  switch (ttype) {
    case TIME_V1: return "TIME_V1";
    case TIME_V2: return "TIME_V2";
    default: throw Err() << "Unknown time type: " << ttype;
  }
}

/********************************************************************/

std::string
graphene_parse_time(const std::string & str, const TimeType ttype){

  if (str=="") throw Err() << "Empty timestamp";

  /// TIME_V2: two uint32_t numbers: seconds and nanoseconds.
  /// For the last record number of nanoseconds is skipped if value s zero.
  /// Supported values: "inf", "now", "now_s", numerical values
  ///  with optional + or - suffix.
  if (ttype == TIME_V2){

    uint64_t t=0;
    if (strcasecmp(str.c_str(), "now")==0){
      struct timeval tv;
      gettimeofday(&tv,NULL);
      t = ((uint64_t)tv.tv_sec << 32) + (uint64_t)tv.tv_usec*1000;
    }
    else if (strcasecmp(str.c_str(), "now_s")==0){
      struct timeval tv;
      gettimeofday(&tv,NULL);
      t = (uint64_t)tv.tv_sec << 32;
    }
    else if (strcasecmp(str.c_str(), "inf")==0){
      t = (uint64_t)((uint32_t)-1)<<32;
      t += (uint64_t)999999999;
    }
    else {
      // +/- suffixes
      int add=0;
      std::istringstream s;
      if (str.size()>0 && (str[str.size()-1] == '+' ||
                               str[str.size()-1] == '-')){
        add = str[str.size()-1]=='+'? +1:-1;
        s.str(std::string(str.begin(), str.end()-1));
      }
      else s.str(str);

      // read numerical value
      uint32_t t1=0, t2=0;
      if (s.peek()=='-') throw Err()
        << "Bad timestamp: positive value expected: " << str;
      s >> t1; // read seconds
      if (s.bad() || s.fail())
        throw Err() << "Bad timestamp: can't read seconds: " << str;
      if (!s.eof()){
        char c;
        s >> c; // read decimal dot
        if (s.bad() || s.fail() || c!='.')
          throw Err() << "Bad timestamp: can't read decimal dot: " << str;
        s >> c;
        int n=8;
        while (!s.eof()){
          if (c<'0'||c>'9')
            throw Err() << "Bad timestamp: can't read nanoseconds: " << str;
          if (n>=0) t2 += (c-'0') * pow(10,n);
          s >> c;
          n--;
        }
      }
      // add +/-
      if (add==+1){
        if (t2<999999999) t2++;
        else {
          t2=0;
          t1++;
        }
      }
      if (add==-1){
        if (t2>0) t2--;
        else {
          t2=999999999;
          t1--;
        }
      }
      // assemble time
      t = ((uint64_t)t1<<32) + t2;
    }

    if ((t&0xFFFFFFFF) == 0){
      std::string ret = std::string(sizeof(uint32_t), '\0');
      *(uint32_t*)ret.data() = (t>>32);
      return ret;
    }
    else {
      std::string ret = std::string(sizeof(uint64_t), '\0');
      *(uint64_t*)ret.data() = t;
      return ret;
    }
  }

  /// TIME_V1: uint64_t with unix time in milliseconds
  /// Supported values: "inf", "now", "now_s", numerical values
  if (ttype == TIME_V1){

    uint64_t t=0;
    if (strcasecmp(str.c_str(), "now")==0){
      struct timeval tv;
      gettimeofday(&tv,NULL);
      t = (uint64_t)tv.tv_usec/1000 + (uint64_t)tv.tv_sec*1000;
    }
    else if (strcasecmp(str.c_str(), "now_s")==0){
      struct timeval tv;
      gettimeofday(&tv,NULL);
      t = (uint64_t)tv.tv_sec*1000;
    }
    else if (strcasecmp(str.c_str(), "inf")==0){
      t = (uint64_t)-1;
    }
    else {
      std::istringstream s(str);
      uint64_t t1=0, t2=0;
      if (s.peek()=='-') throw Err()
        << "Bad V1 timestamp: positive value expected: " << str;
      s >> t1; // read seconds
      if (s.bad() || s.fail())
        throw Err() << "Bad V1 timestamp: can't read seconds: " << str;
      if (!s.eof()){
        char c;
        s >> c; // read decimal dot
        if (s.bad() || s.fail() || c!='.')
          throw Err() << "Bad V1 timestamp: can't read decimal dot: " << str;
        s >> c;
        int n=2;
        while (!s.eof()){
          if (c<'0'||c>'9')
            throw Err() << "Bad V1 timestamp: can't read milliseconds: " << str;
          if (n>=0) t2 += (c-'0') * pow(10,n);
          s >> c;
          n--;
        }
      }
      uint64_t m = (uint64_t)-1;
      if (t1 > m/1000 || 1000*t1 > m-t2)
        throw Err() << "Bad V1 timestamp: too large value: " << str;
      t = t1*1000+t2;
    }
    std::string ret = std::string(sizeof(uint64_t), '\0');
    *(uint64_t *)ret.data() = t;
    return ret;
  }

  throw Err() << "Unknown time type: " << ttype;
}
/********************************************************************/


std::string
graphene_spp_text(const std::string & data){
  size_t nb=0, ne=0;
  std::string out;
  while ((ne = data.find('\n', nb)) != std::string::npos) {
    if (data.length()>nb && data[nb]=='#') out += '#';
    out += data.substr(nb, ne-nb+1);
    nb=ne+1;
  }
  if (data.length()>nb && data[nb]=='#') out += '#';
  out += data.substr(nb, std::string::npos);
  return out;
}
