/* Graphene data handling
*/

#ifndef GRAPHENE_DATA_H
#define GRAPHENE_DATA_H

#include <string>

/********************************************************************/
// Enum for the data format
enum DataType { DATA_TEXT,
         DATA_INT8, DATA_UINT8, DATA_INT16, DATA_UINT16,
         DATA_INT32, DATA_UINT32, DATA_INT64, DATA_UINT64,
         DATA_FLOAT, DATA_DOUBLE};

// Convert string into DataType number.
DataType graphene_parse_dtype(const std::string & s);

// Convert DataType to string.
std::string graphene_dtype_name(const DataType dtype);

// Return number of bytes per record for a given format.
// For TEXT 1 is returned.
size_t graphene_dtype_size(const DataType dtype);


// Enum for the time format (later can be joined to data)
enum TimeType {TIME_V1, TIME_V2};

// Convert string into TimeType number.
TimeType graphene_parse_ttype(const std::string & s);

// Convert TimeType to string.
std::string graphene_ttype_name(const TimeType ttype);

/********************************************************************/

// Parse and pack data.
// Data is coming from user interface as a vector<string>.
// On output std::string is used as a convenient storage, which
// can be easily converted into Berkleydb data.
// Output string is not a c-string, it may contain zeros!
std::string graphene_parse_data(
  const std::vector<std::string> & strs,
  const DataType dtype
);

// Parse and pack time.
// Data is coming from user interface as a string.
// On output std::string is used as a convenient storage, which
// can be easily converted into Berkleydb data.
// Output string is not a c-string, it may contain zeros!
std::string graphene_parse_time(
  const std::string & str,
  const TimeType ttype
);

/********************************************************************/

// Protect # symbol in beginning of each line for SPP protocol
std::string graphene_spp_text(const std::string & data);

#endif
