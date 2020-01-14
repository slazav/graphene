#include <iostream>
#include <vector>
#include <string>

#include "err/err.h"
#include "err/assert_err.h"

#include "data.h"

/***************************************************************/
/***************************************************************/

// analog of graphene_parse_data but with a single string argument
std::string
graphene_parse_data_str(const std::string & input, const DataType dtype){
  // split input
  std::vector<std::string> args;

  if (dtype==DATA_TEXT && input != "") {
    args.push_back(input);
  }

  else {
    std::istringstream in(input);
    while (1) {
      std::string a;
      in >> a;
      if (!in) break;
      args.push_back(a);
    }
  }

  return graphene_parse_data(args, dtype);
}


/***************************************************************/
/***************************************************************/

using namespace std;
int main() {
  try{

/***************************************************************/

    // sizes and names
    assert_eq(graphene_dtype_size(DATA_TEXT),   1);
    assert_eq(graphene_dtype_size(DATA_INT8),   1);
    assert_eq(graphene_dtype_size(DATA_UINT8),  1);
    assert_eq(graphene_dtype_size(DATA_INT16),  2);
    assert_eq(graphene_dtype_size(DATA_UINT16), 2);
    assert_eq(graphene_dtype_size(DATA_INT32),  4);
    assert_eq(graphene_dtype_size(DATA_UINT32), 4);
    assert_eq(graphene_dtype_size(DATA_INT64),  8);
    assert_eq(graphene_dtype_size(DATA_UINT64), 8);
    assert_eq(graphene_dtype_size(DATA_FLOAT),  4);
    assert_eq(graphene_dtype_size(DATA_DOUBLE), 8);

    assert_eq(graphene_dtype_name(DATA_TEXT),   "TEXT"  );
    assert_eq(graphene_dtype_name(DATA_INT8),   "INT8"  );
    assert_eq(graphene_dtype_name(DATA_UINT8),  "UINT8" );
    assert_eq(graphene_dtype_name(DATA_INT16),  "INT16" );
    assert_eq(graphene_dtype_name(DATA_UINT16), "UINT16");
    assert_eq(graphene_dtype_name(DATA_INT32),  "INT32" );
    assert_eq(graphene_dtype_name(DATA_UINT32), "UINT32");
    assert_eq(graphene_dtype_name(DATA_INT64),  "INT64" );
    assert_eq(graphene_dtype_name(DATA_UINT64), "UINT64");
    assert_eq(graphene_dtype_name(DATA_FLOAT),  "FLOAT" );
    assert_eq(graphene_dtype_name(DATA_DOUBLE), "DOUBLE");

    assert_eq(graphene_ttype_name(TIME_V1),  "TIME_V1" );
    assert_eq(graphene_ttype_name(TIME_V2),  "TIME_V2");

    assert_eq(DATA_TEXT,   graphene_parse_dtype("TEXT"  ));
    assert_eq(DATA_INT8,   graphene_parse_dtype("INT8"  ));
    assert_eq(DATA_UINT8,  graphene_parse_dtype("UINT8" ));
    assert_eq(DATA_INT16,  graphene_parse_dtype("INT16" ));
    assert_eq(DATA_UINT16, graphene_parse_dtype("UINT16"));
    assert_eq(DATA_INT32,  graphene_parse_dtype("INT32" ));
    assert_eq(DATA_UINT32, graphene_parse_dtype("UINT32"));
    assert_eq(DATA_INT64,  graphene_parse_dtype("INT64" ));
    assert_eq(DATA_UINT64, graphene_parse_dtype("UINT64"));
    assert_eq(DATA_FLOAT,  graphene_parse_dtype("FLOAT" ));
    assert_eq(DATA_DOUBLE, graphene_parse_dtype("DOUBLE"));

    assert_eq(TIME_V1,  graphene_parse_ttype("TIME_V1" ));
    assert_eq(TIME_V2,  graphene_parse_ttype("TIME_V2" ));

    assert_eq(DATA_DOUBLE, graphene_parse_dtype("DOUBLE"));
    assert_err(graphene_parse_dtype("X"), "Unknown data type: X");
    assert_err(graphene_parse_ttype("X"), "Unknown time type: X");

    assert_err(graphene_dtype_name(static_cast<DataType>(-1)),
      "Unknown data type: -1");

    assert_err(graphene_ttype_name(static_cast<TimeType>(-1)),
      "Unknown time type: -1");

    /***************************************************************/
    // parse data
    std::string s;

    // int8
    s = graphene_parse_data_str("1 -128 127", DATA_INT8);
    assert_eq(s.size(), 3);
    assert_eq(((int8_t *)s.data())[0], 1);
    assert_eq(((int8_t *)s.data())[1], -128);
    assert_eq(((int8_t *)s.data())[2], 127);

    assert_err(graphene_parse_data_str("-129", DATA_INT8),
      "Bad INT8 value: -129");

    assert_err(graphene_parse_data_str("1 128", DATA_INT8),
      "Bad INT8 value: 128");

    assert_err(graphene_parse_data_str("1.1 1", DATA_INT8),
      "Bad INT8 value: 1.1");

    assert_err(graphene_parse_data_str("1 a", DATA_INT8),
      "Bad INT8 value: a");

    assert_err(graphene_parse_data_str("", DATA_INT8),
      "Some data expected");

    // uint8
    s = graphene_parse_data_str("1 0 255", DATA_UINT8);
    assert_eq(s.size(), 3);
    assert_eq(((uint8_t *)s.data())[0], 1);
    assert_eq(((uint8_t *)s.data())[1], 0);
    assert_eq(((uint8_t *)s.data())[2], 255);

    assert_err(graphene_parse_data_str("0 -1", DATA_UINT8),
      "Bad UINT8 value: -1");

    assert_err(graphene_parse_data_str("2 256", DATA_UINT8),
      "Bad UINT8 value: 256");

    assert_err(graphene_parse_data_str("2.0 256", DATA_UINT8),
      "Bad UINT8 value: 2.0");

    assert_err(graphene_parse_data_str("12345678901234567890", DATA_UINT8),
      "Bad UINT8 value: 12345678901234567890");

    assert_err(graphene_parse_data_str("1 a", DATA_UINT8),
      "Bad UINT8 value: a");

    assert_err(graphene_parse_data_str("", DATA_UINT8),
      "Some data expected");

    // int16
    s = graphene_parse_data_str("1 -32768 32767", DATA_INT16);
    assert_eq(s.size(), 6);
    assert_eq(((int16_t *)s.data())[0], 1);
    assert_eq(((int16_t *)s.data())[1], -32768);
    assert_eq(((int16_t *)s.data())[2], 32767);

    assert_err(graphene_parse_data_str("-32769", DATA_INT16),
      "Bad INT16 value: -32769");

    assert_err(graphene_parse_data_str("1 32768", DATA_INT16),
      "Bad INT16 value: 32768");

    assert_err(graphene_parse_data_str("1.1 1", DATA_INT16),
      "Bad INT16 value: 1.1");

    assert_err(graphene_parse_data_str("1 a", DATA_INT16),
      "Bad INT16 value: a");

    assert_err(graphene_parse_data_str("", DATA_INT16),
      "Some data expected");

    // uint16
    s = graphene_parse_data_str("1 0 65535", DATA_UINT16);
    assert_eq(s.size(), 6);
    assert_eq(((uint16_t *)s.data())[0], 1);
    assert_eq(((uint16_t *)s.data())[1], 0);
    assert_eq(((uint16_t *)s.data())[2], 65535);

    assert_err(graphene_parse_data_str("0 -1", DATA_UINT16),
      "Bad UINT16 value: -1");

    assert_err(graphene_parse_data_str("2 65536", DATA_UINT16),
      "Bad UINT16 value: 65536");

    assert_err(graphene_parse_data_str("2.0 65536", DATA_UINT16),
      "Bad UINT16 value: 2.0");

    assert_err(graphene_parse_data_str("12345678901234567890", DATA_UINT16),
      "Bad UINT16 value: 12345678901234567890");

    assert_err(graphene_parse_data_str("1 a", DATA_UINT16),
      "Bad UINT16 value: a");

    assert_err(graphene_parse_data_str("", DATA_UINT16),
      "Some data expected");

    // int32
    s = graphene_parse_data_str("1 -2147483648 2147483647", DATA_INT32);
    assert_eq(s.size(), 12);
    assert_eq(((int32_t *)s.data())[0], 1);
    assert_eq(((int32_t *)s.data())[1], -2147483648);
    assert_eq(((int32_t *)s.data())[2], 2147483647);

    assert_err(graphene_parse_data_str("-2147483649", DATA_INT32),
      "Bad INT32 value: -2147483649");

    assert_err(graphene_parse_data_str("1 2147483648", DATA_INT32),
      "Bad INT32 value: 2147483648");

    assert_err(graphene_parse_data_str("1.1 1", DATA_INT32),
      "Bad INT32 value: 1.1");

    assert_err(graphene_parse_data_str("1 a", DATA_INT32),
      "Bad INT32 value: a");

    assert_err(graphene_parse_data_str("", DATA_INT32),
      "Some data expected");

    // uint32
    s = graphene_parse_data_str("1 0 4294967295", DATA_UINT32);
    assert_eq(s.size(), 12);
    assert_eq(((uint32_t *)s.data())[0], 1);
    assert_eq(((uint32_t *)s.data())[1], 0);
    assert_eq(((uint32_t *)s.data())[2], 4294967295);

    assert_err(graphene_parse_data_str("0 -1", DATA_UINT32),
      "Bad UINT32 value: -1");

    assert_err(graphene_parse_data_str("2 4294967296", DATA_UINT32),
      "Bad UINT32 value: 4294967296");

    assert_err(graphene_parse_data_str("2.0 4294967296", DATA_UINT32),
      "Bad UINT32 value: 2.0");

    assert_err(graphene_parse_data_str("12345678901234567890", DATA_UINT32),
      "Bad UINT32 value: 12345678901234567890");

    assert_err(graphene_parse_data_str("1 a", DATA_UINT32),
      "Bad UINT32 value: a");

    assert_err(graphene_parse_data_str("", DATA_UINT32),
      "Some data expected");

    // int32
    s = graphene_parse_data_str("1 -9223372036854775807 9223372036854775807", DATA_INT64);
    assert_eq(s.size(), 24);
    assert_eq(((int64_t *)s.data())[0], 1);
    assert_eq(((int64_t *)s.data())[1], -9223372036854775807l);
    assert_eq(((int64_t *)s.data())[2], 9223372036854775807l);

    assert_err(graphene_parse_data_str("-9223372036854775809", DATA_INT64),
      "Bad INT64 value: -9223372036854775809");

    assert_err(graphene_parse_data_str("1 9223372036854775808", DATA_INT64),
      "Bad INT64 value: 9223372036854775808");

    assert_err(graphene_parse_data_str("1.1 1", DATA_INT64),
      "Bad INT64 value: 1.1");

    assert_err(graphene_parse_data_str("1 a", DATA_INT64),
      "Bad INT64 value: a");

    assert_err(graphene_parse_data_str("", DATA_INT64),
      "Some data expected");

    // uint64
    s = graphene_parse_data_str("1 0 18446744073709551615", DATA_UINT64);
    assert_eq(s.size(), 24);
    assert_eq(((uint64_t *)s.data())[0], 1);
    assert_eq(((uint64_t *)s.data())[1], 0);
    assert_eq(((uint64_t *)s.data())[2], 18446744073709551615ul);

    assert_err(graphene_parse_data_str("0 -1", DATA_UINT64),
      "Bad UINT64 value: -1");

    assert_err(graphene_parse_data_str("2 18446744073709551616", DATA_UINT64),
      "Bad UINT64 value: 18446744073709551616");

    assert_err(graphene_parse_data_str("2.0 4294967296", DATA_UINT64),
      "Bad UINT64 value: 2.0");

    assert_err(graphene_parse_data_str("12345678901234567890123", DATA_UINT64),
      "Bad UINT64 value: 12345678901234567890123");

    assert_err(graphene_parse_data_str("1 a", DATA_UINT64),
      "Bad UINT64 value: a");

    assert_err(graphene_parse_data_str("", DATA_UINT64),
      "Some data expected");

    // float
    s = graphene_parse_data_str("0 0.1 1e-6 -inf +Inf Nan", DATA_FLOAT);
    assert_eq(s.size(), 6*4);

    {
      float *f = ((float *)s.data());

      assert_eq(f[0], 0);
      assert_feq(f[1], 0.1, 1e-8);
      assert_feq(f[2], 1e-6, 1e-14);

      assert_eq(isinf(f[3]), true);
      assert_eq(signbit(f[3]), true);

      assert_eq(isinf(f[4]), true);
      assert_eq(signbit(f[4]), false);
      assert_eq(isnan(f[5]), true);
    }

    assert_err(graphene_parse_data_str("1 a", DATA_FLOAT),
      "Bad FLOAT value: a");

    assert_err(graphene_parse_data_str("1 1.a", DATA_FLOAT),
      "Bad FLOAT value: 1.a");

    assert_err(graphene_parse_data_str("1e88", DATA_FLOAT),
      "Bad FLOAT value: 1e88");

    assert_err(graphene_parse_data_str("", DATA_FLOAT),
      "Some data expected");



    // double
    s = graphene_parse_data_str("0 0.1 1e-6 -inf +Inf Nan", DATA_DOUBLE);
    assert_eq(s.size(), 6*8);

    {
      double *f = ((double *)s.data());

      assert_eq(f[0], 0);
      assert_feq(f[1], 0.1, 1e-16);
      assert_feq(f[2], 1e-6, 1e-24);

      assert_eq(isinf(f[3]), true);
      assert_eq(signbit(f[3]), true);

      assert_eq(isinf(f[4]), true);
      assert_eq(signbit(f[4]), false);
      assert_eq(isnan(f[5]), true);
    }

    assert_err(graphene_parse_data_str("1 a", DATA_DOUBLE),
      "Bad DOUBLE value: a");

    assert_err(graphene_parse_data_str("1 1.a", DATA_DOUBLE),
      "Bad DOUBLE value: 1.a");

    assert_err(graphene_parse_data_str("", DATA_DOUBLE),
      "Some data expected");


    // text
    {
      std::string s1="0 0.1 1e-6 -inf +Inf Nan 12345";
      assert_eq(graphene_parse_data_str(s1, DATA_TEXT), s1);
      assert_eq(graphene_parse_data_str("a b", DATA_TEXT), "a b");
      assert_eq(graphene_parse_data_str("", DATA_TEXT), "");
    }


    // time_v1
    TimeType tt = TIME_V1;

    s = graphene_parse_time("0", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 0);

    s = graphene_parse_time("1000.999", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 1000999);

    s = graphene_parse_time("1000.9999", tt); // value is truncated to ms
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 1000999);

    s = graphene_parse_time("now", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data())>1578046981l*1000, true);
    assert_eq(*((uint64_t *)s.data())<4102437600l*1000, true);

    s = graphene_parse_time("now_s", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data())>1578046981l*1000, true);
    assert_eq(*((uint64_t *)s.data())<4102437600l*1000, true);
    assert_eq(*((uint64_t *)s.data())%1000==0, true);

    s = graphene_parse_time("18446744073709551.615", tt); // largest timestamp
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (uint64_t)-1);

    s = graphene_parse_time("inf", tt); // largest timestamp
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (uint64_t)-1);

    assert_err(graphene_parse_time("18446744073709551.616", tt),
      "Bad V1 timestamp: too large value: 18446744073709551.616");

    assert_err(graphene_parse_time("18446744073709552.000", tt),
      "Bad V1 timestamp: too large value: 18446744073709552.000");

    assert_err(graphene_parse_time("184467440737095520000", tt),
      "Bad V1 timestamp: can't read seconds: 184467440737095520000");

    assert_err(graphene_parse_time("-2", tt),
      "Bad V1 timestamp: positive value expected: -2");

    assert_err(graphene_parse_time("a", tt),
      "Bad V1 timestamp: can't read seconds: a");

    assert_err(graphene_parse_time("1a", tt),
      "Bad V1 timestamp: can't read decimal dot: 1a");

    assert_err(graphene_parse_time("1.a", tt),
      "Bad V1 timestamp: can't read milliseconds: 1.a");

    assert_err(graphene_parse_time("", tt),
      "Empty timestamp");

    // time_v2
    uint64_t max = (uint32_t)-1;
    tt = TIME_V2;

    s = graphene_parse_time("0", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 0);

    s = graphene_parse_time("0.0", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 0);

    s = graphene_parse_time("1", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 1);

    s = graphene_parse_time("1.", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 1);

    s = graphene_parse_time("1.0", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 1);

    s = graphene_parse_time("1.001", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 1e6);

    s = graphene_parse_time("4294967295.0", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), max);

    s = graphene_parse_time("4294967295.999999999", tt); // largest value
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (max<<32) + 999999999);

    s = graphene_parse_time("4294967295.99999999999", tt); // sub-ns fraction is truncated
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (max<<32) + 999999999);

    s = graphene_parse_time("inf", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (max<<32) + 999999999);

    s = graphene_parse_time("now", tt);
    if (s.size() == 8){
      assert_eq(*((uint64_t *)s.data()) > (1578046981l<<32), true);
      assert_eq(*((uint64_t *)s.data()) < (4102437600l<<32), true);
    }
    else{
      assert_eq(s.size(), 4);
      assert_eq(*((uint32_t *)s.data()) > 1578046981l, true);
      assert_eq(*((uint32_t *)s.data()) < 4102437600l, true);
    }

    s = graphene_parse_time("now_s", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()) > 1578046981l, true);
    assert_eq(*((uint32_t *)s.data()) < 4102437600l, true);

    // +/-


    s = graphene_parse_time("0+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 1);

    s = graphene_parse_time("0-", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (max<<32) + 999999999);

    s = graphene_parse_time("1+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 1);

    s = graphene_parse_time("1.+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 1);

    s = graphene_parse_time("1.000+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 1);

    s = graphene_parse_time("1-", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), 999999999);

    s = graphene_parse_time("1.999999999+", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 2);

    s = graphene_parse_time("1.123456789+", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 123456790);

    s = graphene_parse_time("1.123456789-", tt);
    assert_eq(s.size(), 8);
    assert_eq(*((uint64_t *)s.data()), (1l<<32) + 123456788);

    s = graphene_parse_time("4294967295.999999999+", tt);
    assert_eq(s.size(), 4);
    assert_eq(*((uint32_t *)s.data()), 0);

    assert_err(graphene_parse_time("4294967296.0", tt),
      "Bad timestamp: can't read seconds: 4294967296.0");

    assert_err(graphene_parse_time("-2", tt),
      "Bad timestamp: positive value expected: -2");

    assert_err(graphene_parse_time("a", tt),
      "Bad timestamp: can't read seconds: a");

    assert_err(graphene_parse_time("1a", tt),
      "Bad timestamp: can't read decimal dot: 1a");

    assert_err(graphene_parse_time("1.a", tt),
      "Bad timestamp: can't read nanoseconds: 1.a");

    assert_err(graphene_parse_time("", tt),
      "Empty timestamp");

    /**************************************************************/
    // Time diff
    /**************************************************************/

    tt=TIME_V1;

    assert_feq (graphene_time_diff(
      graphene_parse_time("1", tt),
      graphene_parse_time("0", tt), tt), +1, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("0", tt),
      graphene_parse_time("1", tt), tt), -1, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("0", tt),
      graphene_parse_time("0", tt), tt), 0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("0.123", tt),
      graphene_parse_time("0.123", tt), tt), 0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("2.02", tt),
      graphene_parse_time("1.03", tt), tt), 0.99, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("2.02", tt),
      graphene_parse_time("1.01", tt), tt), 1.01, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("1.03", tt),
      graphene_parse_time("2.02", tt), tt), -0.99, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("1.01", tt),
      graphene_parse_time("2.02", tt), tt), -1.01, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("inf", tt),
      graphene_parse_time("0", tt), tt), 18446744073709551.616, 1);

    assert_feq (graphene_time_diff(
      graphene_parse_time("0", tt),
      graphene_parse_time("inf", tt), tt), -18446744073709551.616, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("inf", tt),
      graphene_parse_time("inf", tt), tt), 0, 1e-10);


    // same with V2
    tt = TIME_V2;

    assert_feq (graphene_time_diff(
      graphene_parse_time("1", tt),
      graphene_parse_time("0", tt), tt), +1, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("0", tt),
      graphene_parse_time("1", tt), tt), -1, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("0", tt),
      graphene_parse_time("0", tt), tt), 0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("0.123", tt),
      graphene_parse_time("0.123", tt), tt), 0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("2.02", tt),
      graphene_parse_time("1.03", tt), tt), 0.99, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("2.02", tt),
      graphene_parse_time("1.01", tt), tt), 1.01, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("1.03", tt),
      graphene_parse_time("2.02", tt), tt), -0.99, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("1.01", tt),
      graphene_parse_time("2.02", tt), tt), -1.01, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("inf", tt),
      graphene_parse_time("0", tt), tt), 4294967296.0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("0", tt),
      graphene_parse_time("inf", tt), tt), -4294967296.0, 1e-10);

    assert_feq (graphene_time_diff(
      graphene_parse_time("inf", tt),
      graphene_parse_time("inf", tt), tt), 0, 1e-10);

    /**************************************************************/
    // Time cmp
    /**************************************************************/

    tt = TIME_V1;
    assert_eq (graphene_time_cmp(
      graphene_parse_time("1", tt),
      graphene_parse_time("0", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("0", tt),
      graphene_parse_time("1", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("0", tt),
      graphene_parse_time("0", tt), tt), 0);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("0.123", tt),
      graphene_parse_time("0.123", tt), tt), 0);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("2.02", tt),
      graphene_parse_time("1.03", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("2.02", tt),
      graphene_parse_time("1.01", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("1.03", tt),
      graphene_parse_time("2.02", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("1.01", tt),
      graphene_parse_time("2.02", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("inf", tt),
      graphene_parse_time("0", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("0", tt),
      graphene_parse_time("inf", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("inf", tt),
      graphene_parse_time("inf", tt), tt), 0);


    // same with V2
    tt = TIME_V2;
    assert_eq (graphene_time_cmp(
      graphene_parse_time("1", tt),
      graphene_parse_time("0", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("0", tt),
      graphene_parse_time("1", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("0", tt),
      graphene_parse_time("0", tt), tt), 0);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("0.123", tt),
      graphene_parse_time("0.123", tt), tt), 0);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("2.02", tt),
      graphene_parse_time("1.03", tt), tt), 1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("2.02", tt),
      graphene_parse_time("1.01", tt), tt), 1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("1.03", tt),
      graphene_parse_time("2.02", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("1.01", tt),
      graphene_parse_time("2.02", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("inf", tt),
      graphene_parse_time("0", tt), tt), +1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("0", tt),
      graphene_parse_time("inf", tt), tt), -1);

    assert_eq (graphene_time_cmp(
      graphene_parse_time("inf", tt),
      graphene_parse_time("inf", tt), tt), 0);


    /**************************************************************/
    // Time zero
    /**************************************************************/
    tt = TIME_V1;
    assert_eq(graphene_time_zero(graphene_parse_time("0.0", tt),tt), 1);
    assert_eq(graphene_time_zero(graphene_parse_time("0.001", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_parse_time("0.0001", tt),tt), 1); // ms precision!
    assert_eq(graphene_time_zero(graphene_parse_time("100", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_parse_time("inf", tt),tt), 0);

    tt = TIME_V2;
    assert_eq(graphene_time_zero(graphene_parse_time("0.0", tt),tt), 1);
    assert_eq(graphene_time_zero(graphene_parse_time("0.001", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_parse_time("0.000001", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_parse_time("0.0000000001", tt),tt), 1); // ns precision!
    assert_eq(graphene_time_zero(graphene_parse_time("100", tt),tt), 0);
    assert_eq(graphene_time_zero(graphene_parse_time("inf", tt),tt), 0);

    /**************************************************************/
    // Time add
    /**************************************************************/

    tt = TIME_V1;

    assert_eq (graphene_time_add(
      graphene_parse_time("1", tt),
      graphene_parse_time("2", tt), tt),
      graphene_parse_time("3", tt));

    assert_eq (graphene_time_add(
      graphene_parse_time("1.999", tt),
      graphene_parse_time("2.999", tt), tt),
      graphene_parse_time("4.998", tt));

    assert_err (graphene_time_add(
      graphene_parse_time("inf", tt),
      graphene_parse_time("0.01", tt), tt),
      "graphene_time_add overfull");

    assert_err (graphene_time_add(
      graphene_parse_time("inf", tt),
      graphene_parse_time("inf", tt), tt),
      "graphene_time_add overfull");

    assert_eq (graphene_time_add(
      graphene_parse_time("0", tt),
      graphene_parse_time("inf", tt), tt),
      graphene_parse_time("inf", tt));

    tt = TIME_V2;

    assert_eq (graphene_time_add(
      graphene_parse_time("1", tt),
      graphene_parse_time("2", tt), tt),
      graphene_parse_time("3", tt));

    assert_eq (graphene_time_add(
      graphene_parse_time("1.999", tt),
      graphene_parse_time("2.999", tt), tt),
      graphene_parse_time("4.998", tt));

    assert_err (graphene_time_add(
      graphene_parse_time("inf", tt),
      graphene_parse_time("0.01", tt), tt),
      "graphene_time_add overfull");

    assert_err (graphene_time_add(
      graphene_parse_time("inf", tt),
      graphene_parse_time("inf", tt), tt),
      "graphene_time_add overfull");

    assert_eq (graphene_time_add(
      graphene_parse_time("0", tt),
      graphene_parse_time("inf", tt), tt),
      graphene_parse_time("inf", tt));

    /**************************************************************/
    // Interpolation
    /**************************************************************/

    {
      tt = TIME_V1;
      string d;
      d = graphene_interpolate(
        graphene_parse_time("1.1", tt),
        graphene_parse_time("1.0", tt),
        graphene_parse_time("1.4", tt),
        graphene_parse_data_str("0.2 1.2", DATA_DOUBLE),
        graphene_parse_data_str("1.0 2.0", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 0.4, 1e-6);
      assert_feq(((double *)d.data())[1], 1.4, 1e-6);

      d = graphene_interpolate(
        graphene_parse_time("1.4", tt),
        graphene_parse_time("1.0", tt),
        graphene_parse_time("1.4", tt),
        graphene_parse_data_str("0.2 1.2", DATA_DOUBLE),
        graphene_parse_data_str("1.0 2.0 3.4", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 1.0, 1e-6);
      assert_feq(((double *)d.data())[1], 2.0, 1e-6);

      d = graphene_interpolate(
        graphene_parse_time("0.9", tt),
        graphene_parse_time("1.0", tt),
        graphene_parse_time("1.4", tt),
        graphene_parse_data_str("0.2 1.2 1.2", DATA_DOUBLE),
        graphene_parse_data_str("1.0 2.0", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 0.0, 1e-6);
      assert_feq(((double *)d.data())[1], 1.0, 1e-6);

      d = graphene_interpolate(
        graphene_parse_time("1.1", tt),
        graphene_parse_time("1.0", tt),
        graphene_parse_time("1.4", tt),
        graphene_parse_data_str("0.2 1.2", DATA_FLOAT),
        graphene_parse_data_str("1.0 2.0", DATA_FLOAT), tt, DATA_FLOAT);
      assert_eq(d.size(), 2*sizeof(float));
      assert_feq(((float *)d.data())[0], 0.4, 1e-6);
      assert_feq(((float *)d.data())[1], 1.4, 1e-6);
    }

    {
      tt = TIME_V2;
      string d;
      d = graphene_interpolate(
        graphene_parse_time("1.1", tt),
        graphene_parse_time("1.0", tt),
        graphene_parse_time("1.4", tt),
        graphene_parse_data_str("0.2 1.2", DATA_DOUBLE),
        graphene_parse_data_str("1.0 2.0", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 0.4, 1e-6);
      assert_feq(((double *)d.data())[1], 1.4, 1e-6);

      d = graphene_interpolate(
        graphene_parse_time("1.4", tt),
        graphene_parse_time("1.0", tt),
        graphene_parse_time("1.4", tt),
        graphene_parse_data_str("0.2 1.2", DATA_DOUBLE),
        graphene_parse_data_str("1.0 2.0 3.4", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 1.0, 1e-6);
      assert_feq(((double *)d.data())[1], 2.0, 1e-6);

      d = graphene_interpolate(
        graphene_parse_time("0.9", tt),
        graphene_parse_time("1.0", tt),
        graphene_parse_time("1.4", tt),
        graphene_parse_data_str("0.2 1.2 1.2", DATA_DOUBLE),
        graphene_parse_data_str("1.0 2.0", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 0.0, 1e-6);
      assert_feq(((double *)d.data())[1], 1.0, 1e-6);

      d = graphene_interpolate(
        graphene_parse_time("1.1", tt),
        graphene_parse_time("1.0", tt),
        graphene_parse_time("1.4", tt),
        graphene_parse_data_str("0.2 1.2", DATA_FLOAT),
        graphene_parse_data_str("1.0 2.0", DATA_FLOAT), tt, DATA_FLOAT);
      assert_eq(d.size(), 2*sizeof(float));
      assert_feq(((float *)d.data())[0], 0.4, 1e-6);
      assert_feq(((float *)d.data())[1], 1.4, 1e-6);
    }

    /**************************************************************/
    // SPP text
    /**************************************************************/
    assert_eq(graphene_spp_text("a"), "a");
    assert_eq(graphene_spp_text("a\n"), "a\n");
    assert_eq(graphene_spp_text("#a"), "##a");
    assert_eq(graphene_spp_text("\n\n#a"), "\n\n##a");
    assert_eq(graphene_spp_text("\n\n#a\n"), "\n\n##a\n");
    assert_eq(graphene_spp_text(
      "#0 0.1 100.001\n#abc\n #cde\nff\n"),
      "##0 0.1 100.001\n##abc\n #cde\nff\n");

  } catch (Err E){
    std::cerr << E.str() << "\n";
    return 1;
  }
  return 0;
}
