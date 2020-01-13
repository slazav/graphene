/* DBinfo class: information about databases and function for
   packing/unpacking/interpolating raw database records.
*/

#ifndef GRAPHENE_DBINFO_H
#define GRAPHENE_DBINFO_H

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <cstring> /* memset */
#include <db.h>
#include "err/err.h"
#include "data.h"

// current database version
#define DBVERSION 2

// bercleydb:
//  http://docs.oracle.com/cd/E17076_02/html/gsg/C/index.html
//  https://web.stanford.edu/class/cs276a/projects/docs/berkeleydb/reftoc.html

/************************************/
// Check database or filter name
// All names (not only for reading/writing, but
// also for moving or deleting should be checked).
void check_name(const std::string & name);

/***********************************************************/
// Class for the database information.
class DBinfo {
  public:
  DataType dtype;
  TimeType ttype;

  uint8_t  version;
  std::string descr;

  DBinfo(const DataType v = DATA_DOUBLE,
         const std::string &d = std::string())
            : dtype(v),ttype(TIME_V2),descr(d),version(DBVERSION) {}

  bool operator==(const DBinfo &o) const{
    return o.dtype==dtype && o.ttype==ttype && o.descr==descr && o.version==version; }

  // We keep time in a std::string storage, which
  // can be easily converted into Berkleydb data.
  // It is not a c-string!

  // Print timestamp
  std::string print_time(const std::string & s) const;

  // Add two packed time values, return packed string
  std::string add_time(const std::string & s1, const std::string & s2) const;

  // Print data
  std::string print_data(const std::string & s, const int col=-1) const;

  // interpolate data (for FLOAT and DOUBLE values)
  // k0,k1,k2,v1,v2 are packed strings!
  std::string interpolate(
        const std::string & k0,
        const std::string & k1, const std::string & k2,
        const std::string & v1, const std::string & v2);

  // Version-specific functions - use only inside object or in tests
//  private:

  // Pack/Unpack integer timestamp
  std::string pack_time_v1(const uint64_t t) const;
  uint64_t unpack_time_v1(const std::string & s) const;
  std::string print_time_v1(const std::string & s) const;
  int cmp_time_v1(const std::string & s1, const std::string & s2) const;
  std::string add_time_v1(const std::string & s1, const std::string & s2) const;
  double time_diff_v1(const std::string & s1, const std::string & s2) const;
  std::string interpolate_v1(
        const std::string & k0,
        const std::string & k1, const std::string & k2,
        const std::string & v1, const std::string & v2);

  // Pack/Unpack integer timestamp
  std::string pack_time_v2(const uint64_t t) const;
  uint64_t unpack_time_v2(const std::string & s) const;
  std::string print_time_v2(const std::string & s) const;
  std::string add_time_v2(const std::string & s1, const std::string & s2) const;
  std::string interpolate_v2(
        const std::string & k0,
        const std::string & k1, const std::string & k2,
        const std::string & v1, const std::string & v2);

};


#endif
