#include <iostream>
#include <vector>
#include <string>

#include "err/err.h"
#include "err/assert_err.h"

#include "data.h"

/***************************************************************/
/***************************************************************/

// analog of graphene_data_parse but with a single string argument
std::string
graphene_data_parse_str(const std::string & input, const DataType dtype){
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

  return graphene_data_parse(args, dtype);
}

// analog of graphene_data_print but with a single string return value
std::string
graphene_data_print_str(const std::string & input, const int col, const DataType dtype){
  auto data = graphene_data_print(input, col, dtype);

  std::string ret;
  for (auto const & d:data){
    ret += (ret.size()? " ":"") + d;
  }
  return ret;
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

    assert_eq(graphene_tfmt_name(TFMT_DEF), "def" );
    assert_eq(graphene_tfmt_name(TFMT_REL), "rel");

    assert_eq(DATA_TEXT,   graphene_dtype_parse("TEXT"  ));
    assert_eq(DATA_INT8,   graphene_dtype_parse("INT8"  ));
    assert_eq(DATA_UINT8,  graphene_dtype_parse("UINT8" ));
    assert_eq(DATA_INT16,  graphene_dtype_parse("INT16" ));
    assert_eq(DATA_UINT16, graphene_dtype_parse("UINT16"));
    assert_eq(DATA_INT32,  graphene_dtype_parse("INT32" ));
    assert_eq(DATA_UINT32, graphene_dtype_parse("UINT32"));
    assert_eq(DATA_INT64,  graphene_dtype_parse("INT64" ));
    assert_eq(DATA_UINT64, graphene_dtype_parse("UINT64"));
    assert_eq(DATA_FLOAT,  graphene_dtype_parse("FLOAT" ));
    assert_eq(DATA_DOUBLE, graphene_dtype_parse("DOUBLE"));

    assert_eq(TIME_V1,  graphene_ttype_parse("TIME_V1" ));
    assert_eq(TIME_V2,  graphene_ttype_parse("TIME_V2" ));

    assert_eq(TFMT_DEF, graphene_tfmt_parse("def" ));
    assert_eq(TFMT_REL, graphene_tfmt_parse("rel" ));

    assert_eq(DATA_DOUBLE, graphene_dtype_parse("DOUBLE"));
    assert_err(graphene_dtype_parse("X"), "Unknown data type: X");
    assert_err(graphene_ttype_parse("X"), "Unknown time type: X");

    assert_err(graphene_dtype_name(static_cast<DataType>(-1)),
      "Unknown data type: -1");

    assert_err(graphene_ttype_name(static_cast<TimeType>(-1)),
      "Unknown time type: -1");

    /***************************************************************/
    // parse data
    std::string s;

    // int8
    s = graphene_data_parse_str("1 -128 127", DATA_INT8);
    assert_eq(s.size(), 3);
    assert_eq(((int8_t *)s.data())[0], 1);
    assert_eq(((int8_t *)s.data())[1], -128);
    assert_eq(((int8_t *)s.data())[2], 127);

    assert_err(graphene_data_parse_str("-129", DATA_INT8),
      "Bad INT8 value: -129");

    assert_err(graphene_data_parse_str("1 128", DATA_INT8),
      "Bad INT8 value: 128");

    assert_err(graphene_data_parse_str("1.1 1", DATA_INT8),
      "Bad INT8 value: 1.1");

    assert_err(graphene_data_parse_str("1 a", DATA_INT8),
      "Bad INT8 value: a");

    assert_err(graphene_data_parse_str("", DATA_INT8),
      "Some data expected");

    // uint8
    s = graphene_data_parse_str("1 0 255", DATA_UINT8);
    assert_eq(s.size(), 3);
    assert_eq(((uint8_t *)s.data())[0], 1);
    assert_eq(((uint8_t *)s.data())[1], 0);
    assert_eq(((uint8_t *)s.data())[2], 255);

    assert_err(graphene_data_parse_str("0 -1", DATA_UINT8),
      "Bad UINT8 value: -1");

    assert_err(graphene_data_parse_str("2 256", DATA_UINT8),
      "Bad UINT8 value: 256");

    assert_err(graphene_data_parse_str("2.0 256", DATA_UINT8),
      "Bad UINT8 value: 2.0");

    assert_err(graphene_data_parse_str("12345678901234567890", DATA_UINT8),
      "Bad UINT8 value: 12345678901234567890");

    assert_err(graphene_data_parse_str("1 a", DATA_UINT8),
      "Bad UINT8 value: a");

    assert_err(graphene_data_parse_str("", DATA_UINT8),
      "Some data expected");

    // int16
    s = graphene_data_parse_str("1 -32768 32767", DATA_INT16);
    assert_eq(s.size(), 6);
    assert_eq(((int16_t *)s.data())[0], 1);
    assert_eq(((int16_t *)s.data())[1], -32768);
    assert_eq(((int16_t *)s.data())[2], 32767);

    assert_err(graphene_data_parse_str("-32769", DATA_INT16),
      "Bad INT16 value: -32769");

    assert_err(graphene_data_parse_str("1 32768", DATA_INT16),
      "Bad INT16 value: 32768");

    assert_err(graphene_data_parse_str("1.1 1", DATA_INT16),
      "Bad INT16 value: 1.1");

    assert_err(graphene_data_parse_str("1 a", DATA_INT16),
      "Bad INT16 value: a");

    assert_err(graphene_data_parse_str("", DATA_INT16),
      "Some data expected");

    // uint16
    s = graphene_data_parse_str("1 0 65535", DATA_UINT16);
    assert_eq(s.size(), 6);
    assert_eq(((uint16_t *)s.data())[0], 1);
    assert_eq(((uint16_t *)s.data())[1], 0);
    assert_eq(((uint16_t *)s.data())[2], 65535);

    assert_err(graphene_data_parse_str("0 -1", DATA_UINT16),
      "Bad UINT16 value: -1");

    assert_err(graphene_data_parse_str("2 65536", DATA_UINT16),
      "Bad UINT16 value: 65536");

    assert_err(graphene_data_parse_str("2.0 65536", DATA_UINT16),
      "Bad UINT16 value: 2.0");

    assert_err(graphene_data_parse_str("12345678901234567890", DATA_UINT16),
      "Bad UINT16 value: 12345678901234567890");

    assert_err(graphene_data_parse_str("1 a", DATA_UINT16),
      "Bad UINT16 value: a");

    assert_err(graphene_data_parse_str("", DATA_UINT16),
      "Some data expected");

    // int32
    s = graphene_data_parse_str("1 -2147483648 2147483647", DATA_INT32);
    assert_eq(s.size(), 12);
    assert_eq(((int32_t *)s.data())[0], 1);
    assert_eq(((int32_t *)s.data())[1], -2147483648);
    assert_eq(((int32_t *)s.data())[2], 2147483647);

    assert_err(graphene_data_parse_str("-2147483649", DATA_INT32),
      "Bad INT32 value: -2147483649");

    assert_err(graphene_data_parse_str("1 2147483648", DATA_INT32),
      "Bad INT32 value: 2147483648");

    assert_err(graphene_data_parse_str("1.1 1", DATA_INT32),
      "Bad INT32 value: 1.1");

    assert_err(graphene_data_parse_str("1 a", DATA_INT32),
      "Bad INT32 value: a");

    assert_err(graphene_data_parse_str("", DATA_INT32),
      "Some data expected");

    // uint32
    s = graphene_data_parse_str("1 0 4294967295", DATA_UINT32);
    assert_eq(s.size(), 12);
    assert_eq(((uint32_t *)s.data())[0], 1);
    assert_eq(((uint32_t *)s.data())[1], 0);
    assert_eq(((uint32_t *)s.data())[2], 4294967295);

    assert_err(graphene_data_parse_str("0 -1", DATA_UINT32),
      "Bad UINT32 value: -1");

    assert_err(graphene_data_parse_str("2 4294967296", DATA_UINT32),
      "Bad UINT32 value: 4294967296");

    assert_err(graphene_data_parse_str("2.0 4294967296", DATA_UINT32),
      "Bad UINT32 value: 2.0");

    assert_err(graphene_data_parse_str("12345678901234567890", DATA_UINT32),
      "Bad UINT32 value: 12345678901234567890");

    assert_err(graphene_data_parse_str("1 a", DATA_UINT32),
      "Bad UINT32 value: a");

    assert_err(graphene_data_parse_str("", DATA_UINT32),
      "Some data expected");

    // int32
    s = graphene_data_parse_str("1 -9223372036854775807 9223372036854775807", DATA_INT64);
    assert_eq(s.size(), 24);
    assert_eq(((int64_t *)s.data())[0], 1);
    assert_eq(((int64_t *)s.data())[1], -9223372036854775807l);
    assert_eq(((int64_t *)s.data())[2], 9223372036854775807l);

    assert_err(graphene_data_parse_str("-9223372036854775809", DATA_INT64),
      "Bad INT64 value: -9223372036854775809");

    assert_err(graphene_data_parse_str("1 9223372036854775808", DATA_INT64),
      "Bad INT64 value: 9223372036854775808");

    assert_err(graphene_data_parse_str("1.1 1", DATA_INT64),
      "Bad INT64 value: 1.1");

    assert_err(graphene_data_parse_str("1 a", DATA_INT64),
      "Bad INT64 value: a");

    assert_err(graphene_data_parse_str("", DATA_INT64),
      "Some data expected");

    // uint64
    s = graphene_data_parse_str("1 0 18446744073709551615", DATA_UINT64);
    assert_eq(s.size(), 24);
    assert_eq(((uint64_t *)s.data())[0], 1);
    assert_eq(((uint64_t *)s.data())[1], 0);
    assert_eq(((uint64_t *)s.data())[2], 18446744073709551615ul);

    assert_err(graphene_data_parse_str("0 -1", DATA_UINT64),
      "Bad UINT64 value: -1");

    assert_err(graphene_data_parse_str("2 18446744073709551616", DATA_UINT64),
      "Bad UINT64 value: 18446744073709551616");

    assert_err(graphene_data_parse_str("2.0 4294967296", DATA_UINT64),
      "Bad UINT64 value: 2.0");

    assert_err(graphene_data_parse_str("12345678901234567890123", DATA_UINT64),
      "Bad UINT64 value: 12345678901234567890123");

    assert_err(graphene_data_parse_str("1 a", DATA_UINT64),
      "Bad UINT64 value: a");

    assert_err(graphene_data_parse_str("", DATA_UINT64),
      "Some data expected");

    // float
    s = graphene_data_parse_str("0 0.1 1e-6 -inf +Inf Nan", DATA_FLOAT);
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

    assert_err(graphene_data_parse_str("1 a", DATA_FLOAT),
      "Bad FLOAT value: a");

    assert_err(graphene_data_parse_str("1 1.a", DATA_FLOAT),
      "Bad FLOAT value: 1.a");

    assert_err(graphene_data_parse_str("1e88", DATA_FLOAT),
      "Bad FLOAT value: 1e88");

    assert_err(graphene_data_parse_str("", DATA_FLOAT),
      "Some data expected");



    // double
    s = graphene_data_parse_str("0 0.1 1e-6 -inf +Inf Nan", DATA_DOUBLE);
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

    assert_err(graphene_data_parse_str("1 a", DATA_DOUBLE),
      "Bad DOUBLE value: a");

    assert_err(graphene_data_parse_str("1 1.a", DATA_DOUBLE),
      "Bad DOUBLE value: 1.a");

    assert_err(graphene_data_parse_str("", DATA_DOUBLE),
      "Some data expected");


    // text
    {
      std::string s1="0 0.1 1e-6 -inf +Inf Nan 12345";
      assert_eq(graphene_data_parse_str(s1, DATA_TEXT), s1);
      assert_eq(graphene_data_parse_str("a b", DATA_TEXT), "a b");
      assert_eq(graphene_data_parse_str("", DATA_TEXT), "");
    }

    {
      vector<string> v1,v2,v3;
      v1.push_back("314");
      v1.push_back("628");
      v2.push_back("3.1415");
      v2.push_back("6.2830");
      v3.push_back("pi");
      v3.push_back("2pi");

      // data_print
      // store in integer DB
      assert_eq(graphene_data_print_str(graphene_data_parse(v1, DATA_INT32), -1,DATA_INT32), "314 628");
      assert_err(graphene_data_print_str(graphene_data_parse(v2, DATA_INT32),-1,DATA_INT32), "Bad INT32 value: 3.1415");
      assert_err(graphene_data_print_str(graphene_data_parse(v3, DATA_INT32),-1,DATA_INT32), "Bad INT32 value: pi");

      // store in double DB
      assert_eq(graphene_data_print_str(graphene_data_parse(v1, DATA_DOUBLE), -1,DATA_DOUBLE), "314 628");
      assert_eq(graphene_data_print_str(graphene_data_parse(v2, DATA_DOUBLE), -1,DATA_DOUBLE), "3.1415 6.283");
      assert_err(graphene_data_print_str(graphene_data_parse(v3, DATA_DOUBLE),-1,DATA_DOUBLE), "Bad DOUBLE value: pi");

      // store in text DB
      assert_eq(graphene_data_print_str(graphene_data_parse(v1, DATA_TEXT),-1,DATA_TEXT), "314 628");
      assert_eq(graphene_data_print_str(graphene_data_parse(v2, DATA_TEXT),-1,DATA_TEXT), "3.1415 6.2830");
      assert_eq(graphene_data_print_str(graphene_data_parse(v3, DATA_TEXT),-1,DATA_TEXT), "pi 2pi");

      // colums
      assert_eq(graphene_data_print_str(graphene_data_parse(v1, DATA_INT32), 0, DATA_INT32), "314");
      assert_eq(graphene_data_print_str(graphene_data_parse(v1, DATA_INT32), 1, DATA_INT32), "628");
      assert_eq(graphene_data_print_str(graphene_data_parse(v1, DATA_INT32), 2, DATA_INT32), "NaN");

      assert_eq(graphene_data_print_str(graphene_data_parse(v2, DATA_DOUBLE), 0, DATA_DOUBLE), "3.1415");
      assert_eq(graphene_data_print_str(graphene_data_parse(v2, DATA_DOUBLE), 1, DATA_DOUBLE), "6.283");
      assert_eq(graphene_data_print_str(graphene_data_parse(v2, DATA_DOUBLE), 2, DATA_DOUBLE), "NaN");

      // column is ignored for the text database
      assert_eq(graphene_data_print_str(graphene_data_parse(v2, DATA_TEXT), 0, DATA_TEXT), "3.1415 6.2830");
      assert_eq(graphene_data_print_str(graphene_data_parse(v2, DATA_TEXT), 1, DATA_TEXT), "3.1415 6.2830");
      assert_eq(graphene_data_print_str(graphene_data_parse(v2, DATA_TEXT), 2, DATA_TEXT), "3.1415 6.2830");
    }

    /**************************************************************/
    // Interpolation
    /**************************************************************/

    {
      TimeType tt = TIME_V1;
      string d;
      d = graphene_interpolate(
        graphene_time_parse("1.1", tt),
        graphene_time_parse("1.0", tt),
        graphene_time_parse("1.4", tt),
        graphene_data_parse_str("0.2 1.2", DATA_DOUBLE),
        graphene_data_parse_str("1.0 2.0", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 0.4, 1e-6);
      assert_feq(((double *)d.data())[1], 1.4, 1e-6);

      d = graphene_interpolate(
        graphene_time_parse("1.4", tt),
        graphene_time_parse("1.0", tt),
        graphene_time_parse("1.4", tt),
        graphene_data_parse_str("0.2 1.2", DATA_DOUBLE),
        graphene_data_parse_str("1.0 2.0 3.4", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 1.0, 1e-6);
      assert_feq(((double *)d.data())[1], 2.0, 1e-6);

      d = graphene_interpolate(
        graphene_time_parse("0.9", tt),
        graphene_time_parse("1.0", tt),
        graphene_time_parse("1.4", tt),
        graphene_data_parse_str("0.2 1.2 1.2", DATA_DOUBLE),
        graphene_data_parse_str("1.0 2.0", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 0.0, 1e-6);
      assert_feq(((double *)d.data())[1], 1.0, 1e-6);

      d = graphene_interpolate(
        graphene_time_parse("1.1", tt),
        graphene_time_parse("1.0", tt),
        graphene_time_parse("1.4", tt),
        graphene_data_parse_str("0.2 1.2", DATA_FLOAT),
        graphene_data_parse_str("1.0 2.0", DATA_FLOAT), tt, DATA_FLOAT);
      assert_eq(d.size(), 2*sizeof(float));
      assert_feq(((float *)d.data())[0], 0.4, 1e-6);
      assert_feq(((float *)d.data())[1], 1.4, 1e-6);
    }

    {
      TimeType tt = TIME_V2;
      string d;
      d = graphene_interpolate(
        graphene_time_parse("1.1", tt),
        graphene_time_parse("1.0", tt),
        graphene_time_parse("1.4", tt),
        graphene_data_parse_str("0.2 1.2", DATA_DOUBLE),
        graphene_data_parse_str("1.0 2.0", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 0.4, 1e-6);
      assert_feq(((double *)d.data())[1], 1.4, 1e-6);

      d = graphene_interpolate(
        graphene_time_parse("1.4", tt),
        graphene_time_parse("1.0", tt),
        graphene_time_parse("1.4", tt),
        graphene_data_parse_str("0.2 1.2", DATA_DOUBLE),
        graphene_data_parse_str("1.0 2.0 3.4", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 1.0, 1e-6);
      assert_feq(((double *)d.data())[1], 2.0, 1e-6);

      d = graphene_interpolate(
        graphene_time_parse("0.9", tt),
        graphene_time_parse("1.0", tt),
        graphene_time_parse("1.4", tt),
        graphene_data_parse_str("0.2 1.2 1.2", DATA_DOUBLE),
        graphene_data_parse_str("1.0 2.0", DATA_DOUBLE), tt, DATA_DOUBLE);
      assert_eq(d.size(), 2*sizeof(double));
      assert_feq(((double *)d.data())[0], 0.0, 1e-6);
      assert_feq(((double *)d.data())[1], 1.0, 1e-6);

      d = graphene_interpolate(
        graphene_time_parse("1.1", tt),
        graphene_time_parse("1.0", tt),
        graphene_time_parse("1.4", tt),
        graphene_data_parse_str("0.2 1.2", DATA_FLOAT),
        graphene_data_parse_str("1.0 2.0", DATA_FLOAT), tt, DATA_FLOAT);
      assert_eq(d.size(), 2*sizeof(float));
      assert_feq(((float *)d.data())[0], 0.4, 1e-6);
      assert_feq(((float *)d.data())[1], 1.4, 1e-6);
    }

  } catch (Err E){
    std::cerr << E.str() << "\n";
    return 1;
  }
  return 0;
}
