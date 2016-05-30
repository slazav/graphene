/* DBinfo class: information about databases and function for
   packing/unpacking/interpolating raw database records.
*/

#ifndef STSDB_DBINFO_H
#define STSDB_DBINFO_H

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>

// bercleydb:
//  http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
//  https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/reftoc.html

/***********************************************************/
// Class for exceptions.
// Usage: throw Err() << "something";
//
class Err {
  std::ostringstream s;
  public:
    Err(){}
    Err(const Err & o) { s << o.s.str(); }
    template <typename T>
    Err & operator<<(const T & o){ s << o; return *this; }
    std::string str()   const { return s.str(); }
};

/***********************************************************/
// Enum for the data format
enum DataFMT { DATA_TEXT,
         DATA_INT8, DATA_UINT8, DATA_INT16, DATA_UINT16,
         DATA_INT32, DATA_UINT32, DATA_INT64, DATA_UINT64,
         DATA_FLOAT, DATA_DOUBLE};

/// default value
const DataFMT DEFAULT_DATAFMT = DATA_DOUBLE;

// last values -- for loops and array dimensions
const DataFMT LAST_DATAFMT = DATA_DOUBLE;

// string names of data formats
const std::string data_fmt_names[LAST_DATAFMT+1] =
  {"TEXT", "INT8", "UINT8", "INT16", "UINT16",
   "INT32", "UINT32", "INT64", "UINT64", "FLOAT", "DOUBLE"};

// sizes of data elements, bytes
const static size_t data_fmt_sizes[LAST_DATAFMT+1] =
      {1,1,1,2,2,4,4,8,8,4,8}; // bytes


/***********************************************************/
// Class for the database information.
class DBinfo {
  public:
  DataFMT val;
  std::string descr;

  DBinfo(const DataFMT v = DATA_DOUBLE,
         const std::string &d = std::string())
            : val(v),descr(d) {}

  // return size and name of the data format
  size_t dsize() const { return data_fmt_sizes[val]; }
  std::string dname() const { return data_fmt_names[val]; }

  // convert string into enum member
  static DataFMT str2datafmt(const std::string & s){
    for (int i = 0; i<=LAST_DATAFMT; i++)
      if (s == data_fmt_names[i]) return static_cast<DataFMT>(i);
    throw Err() << "Unknown data format: " << s;
  }
  // ...and back
  static std::string datafmt2str(const DataFMT s){
    return data_fmt_names[s]; }

  bool operator==(const DBinfo &o) const{
    return o.val==val && o.descr==descr; }

  // Pack timestamp according with time format.
  // std::string is used as a convenient data storage, which
  // can be easily converted into Berkleydb data.
  // It is not a c-string!
  std::string pack_time(const uint64_t t) const;

  // same, but with string on input
  std::string pack_time(const std::string & ts) const;

  // Unpack timestamp
  uint64_t unpack_time(const std::string & s) const;

  // Pack data according with data format
  // std::string is used as a convenient data storage, which
  // can be easily converted into Berkleydb data.
  // Output string is not a c-string!
  std::string pack_data(const std::vector<std::string> & strs) const;

  // Unpack data
  std::string unpack_data(const std::string & s, const int col=-1) const;

  // Unpack data to a double value (for json output)
  // only one column are returned, 0 by default
  double unpack_data_d(const std::string & s, const int col=-1) const;

  // interpolate data (for FLOAT and DOUBLE values)
  // k1,k2,v1,v2 are packed strings!
  std::string interpolate(
        const uint64_t t0,
        const std::string & k1, const std::string & k2,
        const std::string & v1, const std::string & v2);

};


#endif
