/* test some C-functions */
#include <iostream>
#include <vector>
#include <string>

#include "tests.h"
#include "dbpool.h"
#include "db.h"
#include "dbout.h"

using namespace std;
int main() {
  try{
/***************************************************************/
    // FMT object, data and time formats
    ASSERT_EQ(DEFAULT_DATAFMT, DATA_DOUBLE);

    {  // check names and extract column numbers
      ASSERT_EQ(check_name("abc"), "abc");
      ASSERT_EQ(check_name("abc/def"), "abc/def");
      const char *e1 = "symbols '.:+| \\n\\t' are not allowed in the database name";
      ASSERT_EX(check_name("./abc/def"), e1);
      ASSERT_EX(check_name("/abc/def:1"), e1);
      ASSERT_EX(check_name("/abc/def+1"), e1);
      ASSERT_EX(check_name("/abc/def 1"), e1);
      ASSERT_EX(check_name("/abc/def\t"), e1);
      ASSERT_EX(check_name("/abc/def\n"), e1);

      {DBout dbn(".","abc",cout);    ASSERT_EQ(dbn.name, "abc");
                            ASSERT_EQ(dbn.col, -1);}
      {DBout dbn(".","abc/",cout);   ASSERT_EQ(dbn.name, "abc/");}

      {DBout dbn(".","abc:1",cout);
         ASSERT_EQ(dbn.name, "abc"); ASSERT_EQ(dbn.col, 1);}
      {DBout dbn(".","abc:-1",cout); ASSERT_EQ(dbn.col, -1);}
      {DBout dbn(".","abc:-2",cout); ASSERT_EQ(dbn.col, -1);}
    }
    {
      DBinfo hh1; // default constructor
      DBinfo hh2(DATA_INT16);

      ASSERT_EQ(hh1.val, DEFAULT_DATAFMT);
      ASSERT_EQ(hh1.dsize(), data_fmt_sizes[DEFAULT_DATAFMT]);

      ASSERT_EQ(hh2.val, DATA_INT16);
      ASSERT_EQ(hh2.dsize(), 2); // int16 -> 2 bytes
      ASSERT_EQ(hh2.dname(), "INT16");
    }

    {
      // pack/unpack timestamps - v1
      DBinfo hh1(DATA_DOUBLE);
      DBinfo hh2(DATA_INT16);

      std::string ts1  = "1234567890.123000000";
      std::string ts2  = "0.000000000";
      std::string ts3  = "18446744073709551.615000000";

      string d1  = hh1.parse_time_v1(ts1);
      ASSERT_EQ(d1.size(),  8);
      ASSERT_EQ(hh1.print_time_v1(d1), ts1);

      string d2  = hh1.parse_time_v1(ts2);
      ASSERT_EQ(d2.size(),  8);
      ASSERT_EQ(hh1.print_time_v1(d2), ts2);

      ASSERT_EQ(hh2.print_time_v1(hh1.parse_time_v1(ts3)), ts3); //max

      ASSERT_EQ(hh2.print_time_v1(hh1.parse_time_v1("1")), "1.000000000");
      ASSERT_EQ(hh2.print_time_v1(hh1.parse_time_v1("1.")), "1.000000000");
      ASSERT_EQ(hh2.print_time_v1(hh1.parse_time_v1("1.000")), "1.000000000");
      ASSERT_EQ(hh2.print_time_v1(hh1.parse_time_v1("1.123")), "1.123000000");
      ASSERT_EQ(hh2.print_time_v1(hh1.parse_time_v1("1.12345")), "1.123000000");

    }
    {
      // pack/unpack timestamps - v2
      DBinfo hh1(DATA_DOUBLE);

      std::string ts1  = "1234567890.000000000";
      std::string ts2  = "1234567890.123456789";
      string d1  = hh1.parse_time_v2(ts1);
      ASSERT_EQ(d1.size(),  4);
      ASSERT_EQ(hh1.print_time_v2(d1), ts1);
      string d2  = hh1.parse_time_v2(ts2);
      ASSERT_EQ(d2.size(),  8);
      ASSERT_EQ(hh1.print_time_v2(d2), ts2);

      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("0.0")), "0.000000000");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("1")), "1.000000000");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("1.")), "1.000000000");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("1.1")), "1.100000000");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("1.001")), "1.001000000");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("123456789.123456789")), "123456789.123456789");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("123456789.12345678999")), "123456789.123456789");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("4294967295.12345678999")), "4294967295.123456789"); // max
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("123456789.12345678999+")), "123456789.123456790");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("123456789.12345678999-")), "123456789.123456788");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("0+")), "0.000000001");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("0-")), "4294967295.999999999");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("0.-")), "4294967295.999999999");
      ASSERT_EQ(hh1.print_time_v2(hh1.parse_time_v2("0.00-")), "4294967295.999999999");


      ASSERT_EX(hh1.parse_time_v2(""), "Bad timestamp: can't read seconds: ");
      ASSERT_EX(hh1.parse_time_v2("a"), "Bad timestamp: can't read seconds: a");
      ASSERT_EX(hh1.parse_time_v2("1234567890123"), "Bad timestamp: can't read seconds: 1234567890123"); // >2^32
      ASSERT_EX(hh1.parse_time_v2("1,"), "Bad timestamp: can't read decimal dot: 1,");
      ASSERT_EX(hh1.parse_time_v2("1.a"), "Bad timestamp: can't read nanoseconds: 1.a");
      ASSERT_EX(hh1.parse_time_v2("1.+a"), "Bad timestamp: can't read nanoseconds: 1.+a");
      ASSERT_EX(hh1.parse_time_v2("1.00-a"), "Bad timestamp: can't read nanoseconds: 1.00-a");
      ASSERT_EX(hh1.parse_time_v2("1.00-1"), "Bad timestamp: can't read nanoseconds: 1.00-1");
      ASSERT_EX(hh1.parse_time_v2("1.00a-"), "Bad timestamp: can't read nanoseconds: 1.00a-");
    }
    {
      // cmp_time_v1
      DBinfo hh1(DATA_DOUBLE);
      ASSERT_EQ(hh1.cmp_time_v1(hh1.parse_time_v1("0.0"), hh1.parse_time_v1("0.0")), 0);
      ASSERT_EQ(hh1.cmp_time_v1(hh1.parse_time_v1("1.2"), hh1.parse_time_v1("1.1")), 1);
      ASSERT_EQ(hh1.cmp_time_v1(hh1.parse_time_v1("999.2"), hh1.parse_time_v1("1000.1")), -1);
    }
    {
      // cmp_time_v2
      DBinfo hh1(DATA_DOUBLE);
      ASSERT_EQ(hh1.cmp_time_v2(hh1.parse_time_v2("0.0"), hh1.parse_time_v2("0.0")), 0);
      ASSERT_EQ(hh1.cmp_time_v2(hh1.parse_time_v2("1.2"), hh1.parse_time_v2("1.1")), 1);
      ASSERT_EQ(hh1.cmp_time_v2(hh1.parse_time_v2("999.2"), hh1.parse_time_v2("1000.1")), -1);
    }
    {
      // is_zero_time_v1
      DBinfo hh1(DATA_DOUBLE);
      ASSERT_EQ(hh1.is_zero_time_v1(hh1.parse_time_v1("0.0")), 1);
      ASSERT_EQ(hh1.is_zero_time_v1(hh1.parse_time_v1("0.001")), 0);
      ASSERT_EQ(hh1.is_zero_time_v1(hh1.parse_time_v1("0.0001")), 1); // ms precision!
      ASSERT_EQ(hh1.is_zero_time_v1(hh1.parse_time_v1("100")), 0);
    }
    {
      // is_zero_time_v2
      DBinfo hh1(DATA_DOUBLE);
      ASSERT_EQ(hh1.is_zero_time_v2(hh1.parse_time_v2("0.0")), 1);
      ASSERT_EQ(hh1.is_zero_time_v2(hh1.parse_time_v2("0.001")), 0);
      ASSERT_EQ(hh1.is_zero_time_v2(hh1.parse_time_v2("0.0001")), 0);
      ASSERT_EQ(hh1.is_zero_time_v2(hh1.parse_time_v2("0.0000000001")), 1); // ns precision
      ASSERT_EQ(hh1.is_zero_time_v2(hh1.parse_time_v2("100")), 0);
    }
    {
      // add_time_v1
      DBinfo hh1(DATA_DOUBLE);
      ASSERT_EQ(hh1.print_time_v1(hh1.add_time_v1(hh1.parse_time_v1("1.5"), hh1.parse_time_v1("0.5"))), "2.000000000");
      ASSERT_EQ(hh1.print_time_v1(hh1.add_time_v1(hh1.parse_time_v1("1.999"), hh1.parse_time_v1("1.999"))), "3.998000000");
      ASSERT_EQ(hh1.print_time_v1(hh1.add_time_v1(hh1.parse_time_v1("10"), hh1.parse_time_v1("10"))), "20.000000000");
    }
    {
      // add_time_v2
      DBinfo hh1(DATA_DOUBLE);
      ASSERT_EQ(hh1.print_time_v2(hh1.add_time_v2(hh1.parse_time_v2("1.5"), hh1.parse_time_v2("0.5"))), "2.000000000");
      ASSERT_EQ(hh1.print_time_v2(hh1.add_time_v2(hh1.parse_time_v2("1.999"), hh1.parse_time_v2("1.999"))), "3.998000000");
      ASSERT_EQ(hh1.print_time_v2(hh1.add_time_v2(hh1.parse_time_v2("10"), hh1.parse_time_v2("10"))), "20.000000000");
      // 2^31+2^31 >= 2^32
      ASSERT_EX(hh1.add_time_v2(hh1.parse_time_v2("2147483648"), hh1.parse_time_v2("2147483648")), "add_time overfull");
    }

    {
      // time_diff_v1
      DBinfo hh1(DATA_DOUBLE);
      ASSERT_EQ(hh1.time_diff_v1(hh1.parse_time_v1("1.5"), hh1.parse_time_v1("0.5")), 1.0);
      ASSERT_EQ(hh1.time_diff_v1(hh1.parse_time_v1("1.999"), hh1.parse_time_v1("1.999")), 0.0);
      ASSERT_EQ(hh1.time_diff_v1(hh1.parse_time_v1("1.999"), hh1.parse_time_v1("2.9999")), -1.0); // ms precision!
      ASSERT_EQ(hh1.time_diff_v1(hh1.parse_time_v1("2147483648.001"), hh1.parse_time_v1("2147483648.000")), 0.001);
    }
    {
      // time_diff_v2
      DBinfo hh1(DATA_DOUBLE);
      ASSERT_EQ(hh1.time_diff_v2(hh1.parse_time_v2("1.5"), hh1.parse_time_v2("0.5")), 1.0);
      ASSERT_EQ(hh1.time_diff_v2(hh1.parse_time_v2("1.999"), hh1.parse_time_v2("1.999")), 0.0);
      ASSERT_EQ(hh1.time_diff_v2(hh1.parse_time_v2("1.999"), hh1.parse_time_v2("2.9999")), -1.0009); // ns precision!
      ASSERT_EQ(hh1.time_diff_v2(hh1.parse_time_v2("2147483648.001"), hh1.parse_time_v2("2147483648.000")), 0.001);
      ASSERT_EQ(hh1.time_diff_v2(hh1.parse_time_v2("2147483648.000000001"), hh1.parse_time_v2("2147483648.0")), 1e-9);
      ASSERT_EQ(hh1.time_diff_v2(hh1.parse_time_v2("2147483648"), hh1.parse_time_v2("2147483648.000000001")), -1e-9);
      ASSERT_EQ(hh1.time_diff_v2(hh1.parse_time_v2("2147483648.999999999"), hh1.parse_time_v2("0")), 2147483648.999999999);
      ASSERT_EQ(hh1.time_diff_v2(hh1.parse_time_v2("10"), hh1.parse_time_v2("10")), 00.0);
    }

    {
      // interpolate_v2
      DBinfo hh1(DATA_DOUBLE);
      vector<string> d1,d2;
      d1.push_back("0.2");
      d1.push_back("1.2");
      d2.push_back("1.0");
      d2.push_back("2.0");
      string d0 = hh1.interpolate_v2(
        hh1.parse_time_v2("1.1"),
        hh1.parse_time_v2("1.0"),
        hh1.parse_time_v2("1.4"),
        hh1.parse_data(d1),
        hh1.parse_data(d2));
      ASSERT_EQ(hh1.print_data(d0), "0.4 1.4");
    }
    {
      // pack/unpack data
      DBinfo hh1(DATA_INT32);
      DBinfo hh2(DATA_DOUBLE);
      DBinfo hh3(DATA_TEXT);

      vector<string> v1,v2,v3;
      v1.push_back("314");
      v1.push_back("628");
      v2.push_back("3.1415");
      v2.push_back("6.2830");
      v3.push_back("pi");
      v3.push_back("2pi");

      // store in integer DB
      ASSERT_EQ(hh1.print_data(hh1.parse_data(v1)), "314 628");
      ASSERT_EX(hh1.print_data(hh1.parse_data(v2)), "Can't put value into INT32 database: 3.1415");
      ASSERT_EX(hh1.print_data(hh1.parse_data(v3)), "Can't put value into INT32 database: pi"); //!!!

      // store in double DB
      ASSERT_EQ(hh2.print_data(hh2.parse_data(v1)), "314 628");
      ASSERT_EQ(hh2.print_data(hh2.parse_data(v2)), "3.1415 6.283");
      ASSERT_EX(hh2.print_data(hh2.parse_data(v3)), "Can't put value into DOUBLE database: pi");

      // store in text DB
      ASSERT_EQ(hh3.print_data(hh3.parse_data(v1)), "314 628");
      ASSERT_EQ(hh3.print_data(hh3.parse_data(v2)), "3.1415 6.2830");
      ASSERT_EQ(hh3.print_data(hh3.parse_data(v3)), "pi 2pi");

      // colums
      ASSERT_EQ(hh1.print_data(hh1.parse_data(v1), 0), "314");
      ASSERT_EQ(hh1.print_data(hh1.parse_data(v1), 1), "628");
      ASSERT_EQ(hh1.print_data(hh1.parse_data(v1), 2), "NaN");

      ASSERT_EQ(hh2.print_data(hh2.parse_data(v2), 0), "3.1415");
      ASSERT_EQ(hh2.print_data(hh2.parse_data(v2), 1), "6.283");
      ASSERT_EQ(hh2.print_data(hh2.parse_data(v2), 2), "NaN");

      // column is ignored for the text database
      ASSERT_EQ(hh3.print_data(hh3.parse_data(v2), 0), "3.1415 6.2830");
      ASSERT_EQ(hh3.print_data(hh3.parse_data(v2), 1), "3.1415 6.2830");
      ASSERT_EQ(hh3.print_data(hh3.parse_data(v2), 2), "3.1415 6.2830");
    }

    // sizes and names
    ASSERT_EQ(data_fmt_sizes[DATA_TEXT],   1);
    ASSERT_EQ(data_fmt_sizes[DATA_INT8],   1);
    ASSERT_EQ(data_fmt_sizes[DATA_UINT8],  1);
    ASSERT_EQ(data_fmt_sizes[DATA_INT16],  2);
    ASSERT_EQ(data_fmt_sizes[DATA_UINT16], 2);
    ASSERT_EQ(data_fmt_sizes[DATA_INT32],  4);
    ASSERT_EQ(data_fmt_sizes[DATA_UINT32], 4);
    ASSERT_EQ(data_fmt_sizes[DATA_INT64],  8);
    ASSERT_EQ(data_fmt_sizes[DATA_UINT64], 8);
    ASSERT_EQ(data_fmt_sizes[DATA_FLOAT],  4);
    ASSERT_EQ(data_fmt_sizes[DATA_DOUBLE], 8);

    ASSERT_EQ(data_fmt_names[DATA_TEXT],   "TEXT"  );
    ASSERT_EQ(data_fmt_names[DATA_INT8],   "INT8"  );
    ASSERT_EQ(data_fmt_names[DATA_UINT8],  "UINT8" );
    ASSERT_EQ(data_fmt_names[DATA_INT16],  "INT16" );
    ASSERT_EQ(data_fmt_names[DATA_UINT16], "UINT16");
    ASSERT_EQ(data_fmt_names[DATA_INT32],  "INT32" );
    ASSERT_EQ(data_fmt_names[DATA_UINT32], "UINT32");
    ASSERT_EQ(data_fmt_names[DATA_INT64],  "INT64" );
    ASSERT_EQ(data_fmt_names[DATA_UINT64], "UINT64");
    ASSERT_EQ(data_fmt_names[DATA_FLOAT],  "FLOAT" );
    ASSERT_EQ(data_fmt_names[DATA_DOUBLE], "DOUBLE");

    ASSERT_EQ(DATA_TEXT,   DBinfo::str2datafmt("TEXT"  ));
    ASSERT_EQ(DATA_INT8,   DBinfo::str2datafmt("INT8"  ));
    ASSERT_EQ(DATA_UINT8,  DBinfo::str2datafmt("UINT8" ));
    ASSERT_EQ(DATA_INT16,  DBinfo::str2datafmt("INT16" ));
    ASSERT_EQ(DATA_UINT16, DBinfo::str2datafmt("UINT16"));
    ASSERT_EQ(DATA_INT32,  DBinfo::str2datafmt("INT32" ));
    ASSERT_EQ(DATA_UINT32, DBinfo::str2datafmt("UINT32"));
    ASSERT_EQ(DATA_INT64,  DBinfo::str2datafmt("INT64" ));
    ASSERT_EQ(DATA_UINT64, DBinfo::str2datafmt("UINT64"));
    ASSERT_EQ(DATA_FLOAT,  DBinfo::str2datafmt("FLOAT" ));
    ASSERT_EQ(DATA_DOUBLE, DBinfo::str2datafmt("DOUBLE"));
    ASSERT_EX(DBinfo::str2datafmt("X"), "Unknown data format: X");

/***************************************************************/
    // creating database, writing/reading data format
    DBinfo hh1(DATA_INT16, "AAA");
    DBinfo hh2;
    {
      DBpool pool(".");
      DBgr db = pool.get("test", DB_CREATE);
      db.write_info(hh1);
      hh2 = db.read_info();
      ASSERT_EQ(hh1.val, hh2.val);
      ASSERT_EQ(hh1.descr, hh2.descr);

      hh1.val = DATA_DOUBLE;
      hh1.descr = "Description";
      db.write_info(hh1);
      hh2 = db.read_info();
      ASSERT_EQ(hh1.val, hh2.val);
      ASSERT_EQ(hh1.descr, hh2.descr);
    }

    {
      DBpool pool(".");
      DBgr db1 = pool.get("test", DB_RDONLY);
      hh2 = db1.read_info();
      ASSERT_EQ(hh1.val, hh2.val);
      ASSERT_EQ(hh1.descr, hh2.descr);
    }

/***************************************************************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
  }
}
