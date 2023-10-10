/* Graphene data handling
*/

#ifndef GRAPHENE_DATA_H
#define GRAPHENE_DATA_H

#include <string>
#include <cstdint>

/********************************************************************/
// Enum for the data type
enum DataType { DATA_TEXT,
         DATA_INT8, DATA_UINT8, DATA_INT16, DATA_UINT16,
         DATA_INT32, DATA_UINT32, DATA_INT64, DATA_UINT64,
         DATA_FLOAT, DATA_DOUBLE};

// Convert string into DataType number.
DataType graphene_dtype_parse(const std::string & s);

// Convert DataType to string.
std::string graphene_dtype_name(const DataType dtype);

// Return number of bytes per record for a given format.
// For TEXT 1 is returned.
size_t graphene_dtype_size(const DataType dtype);

// Parse and pack data.
// Data is coming from user interface as a vector<string>.
// On output std::string is used as a convenient storage, which
// can be easily converted into Berkleydb data.
// Output string is not a c-string, it may contain zeros!
std::string graphene_data_parse(
  const std::vector<std::string> & strs,
  const DataType dtype
);

// Print packed data for output
std::vector<std::string> graphene_data_print(
  const std::string & s,
  const int col,
  const DataType dtype
);


/********************************************************************/

// Enum for the timestamp type (later can be joined to data)
enum TimeType {TIME_V1, TIME_V2};

// Convert string into TimeType number.
TimeType graphene_ttype_parse(const std::string & s);

// Convert TimeType to string.
std::string graphene_ttype_name(const TimeType ttype);


// Format for time printing.
enum TimeFMT {TFMT_DEF, TFMT_REL};

// Convert string into TimeFMT.
TimeFMT graphene_tfmt_parse(const std::string & s);

// Convert TimeFMT to string.
std::string graphene_tfmt_name(const TimeFMT tfmt);


// Parse and pack time.
// Data is coming from user interface as a string.
// On output std::string is used as a convenient storage, which
// can be easily converted into Berkleydb data.
// Output string is not a c-string, it may contain zeros!
std::string graphene_time_parse(
  const std::string & str,
  const TimeType ttype
);

// Calculate time difference (t1-t2) for two packed times,
// return number of seconds as double value
double graphene_time_diff(
  const std::string & t1,
  const std::string & t2,
  const TimeType ttype);

// Return -1, 0 or 1 if t1<t2, t1==t2, t1>t2
int graphene_time_cmp(
  const std::string & t1,
  const std::string & t2,
  const TimeType ttype);

// Check if time is zero
bool graphene_time_zero(
  const std::string & t,
  const TimeType ttype);

// Add two timestamps represented as packed strings, return
// result as a packed string.
std::string graphene_time_add(
  const std::string & t1,
  const std::string & t2,
  const TimeType ttype);


// Print timestamp.
// t0 is the reference time for relative output (non-parsed text string!).
std::string graphene_time_print(
  const std::string & t,
  const TimeType ttype,
  const TimeFMT tfmt = TFMT_DEF,
  const std::string & t0 = "");


/********************************************************************/

// Interpolate data (for FLOAT and DOUBLE values).
// Arguments k0,k1,k2,v1,v2 and return value are packed strings!

std::string graphene_interpolate(
        const std::string & k0,
        const std::string & k1, const std::string & k2,
        const std::string & v1, const std::string & v2,
        const TimeType ttype, const DataType dtype);

/********************************************************************/

// Protect # symbol in beginning of each line for SPP protocol
std::string graphene_spp_text(const std::string & data);

/***********************************************************/
// Check database or filter name
// All names (not only for reading/writing, but
// also for moving or deleting should be checked).
void check_name(const std::string & name);

// Parse extended dataset name (<dbname>:<column>) fill
// column and filter variables, return dbname
std::string parse_ext_name(const std::string & name, int & col, int & flt);

#endif
