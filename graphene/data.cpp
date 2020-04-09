#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>
#include <sys/time.h>

#include "opt/opt.h"
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
graphene_dtype_parse(const std::string & s){
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
graphene_data_parse(const std::vector<std::string> & strs, const DataType dtype){

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

std::vector<std::string>
graphene_data_print(const std::string & s, const int col, const DataType dtype){
  std::vector<std::string> ret;

  if (dtype == DATA_TEXT) {
    ret.push_back(s);
    return ret;
  }

  size_t dsize = graphene_dtype_size(dtype);

  if (s.size() % dsize != 0)
    throw Err() << "Broken database: wrong data length";
  // number of columns
  size_t cn = s.size()/dsize;
  // column range we want to show:
  size_t c1=0, c2=cn;
  if (col!=-1) { c1=col; c2=col+1; }

  for (size_t i=c1; i<c2; i++){
    if (i>=cn) { ret.push_back("NaN"); continue;}
    std::ostringstream ostr;
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
      // We use one less digit to have round values (3.1415 instead of 3.1415000
      case DATA_FLOAT:  ostr << std::setprecision(8)  << ((float  *)s.data())[i]; break;
      case DATA_DOUBLE: ostr << std::setprecision(16) << ((double *)s.data())[i]; break;
      default: throw Err() << "Unexpected data format";
    }
    ret.push_back(ostr.str());
  }
  return ret;
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
graphene_ttype_parse(const std::string & s){
  if (strcasecmp(s.c_str(), "TIME_V1")==0) return TIME_V1;
  if (strcasecmp(s.c_str(), "TIME_V2")==0) return TIME_V2;
  throw Err() << "Unknown time type: " << s;
}

std::string
graphene_ttype_name(const TimeType ttype){
  switch (ttype) {
    case TIME_V1: return "TIME_V1";
    case TIME_V2: return "TIME_V2";
  }
  throw Err() << "Unknown time type: " << ttype;
}

/********************************************************************/

TimeFMT graphene_tfmt_parse(const std::string & s){
  if (strcasecmp(s.c_str(),"def") == 0) {return TFMT_DEF;}
  if (strcasecmp(s.c_str(),"rel") == 0) {return TFMT_REL;}
  throw Err() << "Unknown time format: " << s;
}

std::string graphene_tfmt_name(const TimeFMT tfmt){
  switch (tfmt){
    case TFMT_DEF: return "def";
    case TFMT_REL: return "rel";
  }
  throw Err() << "Unknown time format: " << tfmt;
}


/********************************************************************/

uint64_t
graphene_time_unpack_v1(const std::string & t){
  if (t.size()!=sizeof(uint64_t))
    throw Err() << "Broken database: wrong timestamp size: " << t.size();
  return *(uint64_t *)t.data();
}

uint64_t
graphene_time_unpack_v2(const std::string & t){
  if (t.size()==sizeof(uint64_t))
    return *(uint64_t *)t.data();
  if (t.size()==sizeof(uint32_t)){
    uint64_t v = (uint64_t)(*(uint32_t *)t.data())<<32;
    if (v&0xFFFFFFFF > 999999999) throw Err()
       << "Bad timestamp: too large nanosecond field";
    return v;
  }
  throw Err() << "Broken database: wrong timestamp size: " << t.size();
}


std::string
graphene_time_pack_v2(const uint64_t & t){
  if (t&0xFFFFFFFF){
    std::string ret(sizeof(uint64_t), '\0');
    *(uint64_t *)ret.data() = t;
    return ret;
  }
  else {
    std::string ret(sizeof(uint32_t), '\0');
    *(uint32_t *)ret.data() = t>>32;
    return ret;
  }
}

/********************************************************************/
bool
graphene_time_parse_s(const std::string & str, uint64_t * t, const TimeType ttype){

  int digits = (ttype == TIME_V2)? 9:3;
  uint64_t maxval = (ttype == TIME_V2)? (uint32_t)-1 : (uint64_t)-1;

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
  uint64_t t1=0, t2=0;
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
    int n=digits-1;
    while (!s.eof()){
      if (c<'0'||c>'9')
        throw Err() << "Bad timestamp: can't read fractional part: " << str;
      if (n>=0) t2 += (c-'0') * pow(10,n);
      s >> c;
      n--;
    }
  }
  // add +/-
  if (add==+1){
    if (t2<pow(10,digits)-1) t2++;
    else {
      t2=0;
      t1 = t1<maxval? t1+1: 0;
    }
  }
  if (add==-1){
    if (t2>0) t2--;
    else {
      t2=pow(10,digits)-1;
      t1 = t1>0? t1-1: maxval;
    }
  }

  // check overfull and assemble time
  if (ttype == TIME_V2){
    if (t1 > maxval) throw Err()
      << "Bad timestamp: too large value: " << str;
    *t = ((uint64_t)t1<<32) + t2;
  }
  else {
      if (t1 > maxval/1000 || 1000*t1 > maxval-t2)
        throw Err() << "Bad timestamp: too large value: " << str;
    *t = t1*1000+t2;
  }
  return true;
}



std::string
graphene_time_parse(const std::string & str, const TimeType ttype){

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
      graphene_time_parse_s(str, &t, ttype);
    }
    return graphene_time_pack_v2(t);
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
      graphene_time_parse_s(str, &t, ttype);
    }
    std::string ret = std::string(sizeof(uint64_t), '\0');
    *(uint64_t *)ret.data() = t;
    return ret;
  }

  throw Err() << "Unknown time type: " << ttype;
}



/********************************************************************/
double
graphene_time_diff(const std::string & t1, const std::string & t2,
                   const TimeType ttype){
  switch (ttype){
    case TIME_V1: {
      uint64_t v1 = graphene_time_unpack_v1(t1);
      uint64_t v2 = graphene_time_unpack_v1(t2);
      return v1>v2 ? (double)(v1-v2)/1000.0 :
                    -(double)(v2-v1)/1000.0;
    }
    case TIME_V2: {
      uint64_t v1 = graphene_time_unpack_v2(t1);
      uint64_t v2 = graphene_time_unpack_v2(t2);
      // difference in seconds and in milliseconds
      int64_t d1 = (int64_t)(v1 >> 32) - (int64_t)(v2 >> 32);
      int64_t d2 = (int64_t)(v1 & 0xFFFFFFFF) - (int64_t)(v2 & 0xFFFFFFFF);
      return (double)d1 + (double)d2*1e-9;
    }
  }
  throw Err() << "Unknown time type: " << ttype;
}

int
graphene_time_cmp(const std::string & t1, const std::string & t2,
                  const TimeType ttype){
  switch (ttype){
    case TIME_V1: {
      uint64_t v1 = graphene_time_unpack_v1(t1);
      uint64_t v2 = graphene_time_unpack_v1(t2);
      if (v1==v2) return 0;
      return v1>v2 ? +1:-1;
    }
    case TIME_V2: {
      uint64_t v1 = graphene_time_unpack_v2(t1);
      uint64_t v2 = graphene_time_unpack_v2(t2);
      // difference in seconds and in milliseconds
      if (v1==v2) return 0;
      return v1>v2 ? 1:-1;
    }
  }
  throw Err() << "Unknown time type: " << ttype;
}

bool
graphene_time_zero( const std::string & t, const TimeType ttype){
  switch (ttype){
    case TIME_V1: return graphene_time_unpack_v1(t)==0;
    case TIME_V2: return graphene_time_unpack_v2(t)==0;
  }
  throw Err() << "Unknown time type: " << ttype;
}

std::string
graphene_time_add(const std::string & t1, const std::string & t2,
                  const TimeType ttype){
  switch (ttype){
    case TIME_V1: {
      uint64_t v1 = graphene_time_unpack_v1(t1);
      uint64_t v2 = graphene_time_unpack_v1(t2);
      std::string ret(sizeof(uint64_t), '\0');
      uint64_t v = v1+v2;
      if (v<v1 || v<v2) throw Err() << "graphene_time_add overfull";
      *(uint64_t *)ret.data() = v;
      return ret;
    }
    case TIME_V2: {
      uint64_t v1 = graphene_time_unpack_v2(t1);
      uint64_t v2 = graphene_time_unpack_v2(t2);
      uint64_t sum1 = (v1 >> 32) + (v2 >> 32);
      uint64_t sum2 = (v1 & 0xFFFFFFFF) + (v2 & 0xFFFFFFFF);
      while (sum2 > 999999999) {sum2-=1000000000; sum1++;}
      if (sum1 >= ((uint64_t)1<<32) ) throw Err() << "graphene_time_add overfull";
      return graphene_time_pack_v2((sum1<<32)+sum2);
    }
  }
  throw Err() << "Unknown time type: " << ttype;
}

std::string
graphene_time_print(const std::string & t, const TimeType ttype,
                    const TimeFMT tfmt, const std::string & t0){

  switch (tfmt){

    case TFMT_DEF:
      switch (ttype){
        case TIME_V1: {
          uint64_t v = graphene_time_unpack_v1(t);
          std::ostringstream ss;
          ss << v/1000;
          //if (t%1000)
          ss << "." << std::setw(9) << std::setfill('0') << (v%1000)*1000000;
          return ss.str();
        }
        case TIME_V2: {
          uint64_t v = graphene_time_unpack_v2(t);
          std::ostringstream ss;
          ss << (v>>32);
          //if (t&0xFFFFFFFF)
          ss << "." << std::setw(9) << std::setfill('0') << (v&0xFFFFFFFF);
          return ss.str();
        }
        default: throw Err() << "Unknown time type: " << ttype;
      }

    case TFMT_REL: {
      std::string t0s = graphene_time_parse(t0, ttype);
      std::ostringstream ss;
      ss << std::fixed << std::setprecision(9)
         << graphene_time_diff(t, t0s, ttype);
      return ss.str();
    }

    default: throw Err() << "Unknown time format " << tfmt;
  }
}


/********************************************************************/

std::string
graphene_interpolate(
        const std::string & k0,
        const std::string & k1, const std::string & k2,
        const std::string & v1, const std::string & v2,
        const TimeType ttype, const DataType dtype){

  double dt1 = graphene_time_diff(k0,k1, ttype);
  double dt2 = graphene_time_diff(k2,k0, ttype);
  double k = dt2/(dt1+dt2); // first point weight

  // check for correct value size
  size_t dsize = graphene_dtype_size(dtype);
  if (v1.size() % dsize != 0 || v2.size() % dsize != 0)
    throw Err() << "Broken database: wrong data length";

  // number of columns
  size_t cn1 = v1.size()/dsize;
  size_t cn2 = v2.size()/dsize;
  size_t cn0 = std::min(cn1,cn2);

  std::string v0(dsize*cn0, '\0');
  for (size_t i=0; i<cn0; i++){
    switch (dtype){
      case DATA_FLOAT:
        ((float*)v0.data())[i] = ((float*)v1.data())[i]*k
                               + ((float*)v2.data())[i]*(1-k);
        break;
      case DATA_DOUBLE:
        ((double*)v0.data())[i] = ((double*)v1.data())[i]*k
                                + ((double*)v2.data())[i]*(1-k);
        break;
      default: throw Err() << "FLOAT or DOUBLE data expected for interpolation";
    }
  }
  return v0;
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

/***********************************************************/
void
check_name(const std::string & name){
  static const char *reject = ".:+| \n\t/";
  if (strcspn(name.c_str(), reject)!=name.length())
    throw Err() << "symbols '.:+| \\n\\t/' are not allowed in the database name: " << name;
}

std::string
parse_ext_name(const std::string & name, int & col, int & flt){
  // extract column and filter
  col = -1;
  flt = -1;
  std::string dbname = name;
  size_t cp = name.rfind(':');
  if (cp!=std::string::npos && cp!=name.size()-1){
    try {
      auto suff = name.substr(cp+1,-1);
      if (suff.size()>1 && suff[0]=='f') {
        suff = suff.substr(1,-1);
        flt = str_to_type<int>(suff);
        if (flt<1) throw Err();
      }
      else {
       col = str_to_type<int>(suff);
       if (col<0) throw Err();
      }
    } catch (Err e){
      throw Err() << "bad column name: " << name.substr(cp+1,-1);
    }
    dbname = name.substr(0,cp);
  }

  check_name(dbname);
  return dbname;
}

