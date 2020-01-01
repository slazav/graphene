/* test some C-functions */
#include <iostream>
#include <vector>
#include <string>

#include "dbpool.h"
#include "dbgr.h"
#include "dbout.h"
#include "err/err.h"
#include "err/assert_err.h"

using namespace std;
int main() {
  try{
/***************************************************************/
    // FMT object, data and time formats
    assert_eq(DEFAULT_DATAFMT, DATA_DOUBLE);

    {  // check names and extract column numbers
      check_name("abc");
      std::string e1("symbols '.:+| \\n\\t/' are not allowed in the database name: ");
      assert_err(check_name("abc/def"), e1+"abc/def");
      assert_err(check_name("./abc/def" ), e1+"./abc/def" );
      assert_err(check_name("/abc/def:1"), e1+"/abc/def:1");
      assert_err(check_name("/abc/def+1"), e1+"/abc/def+1");
      assert_err(check_name("/abc/def 1"), e1+"/abc/def 1");
      assert_err(check_name("/abc/def\t"), e1+"/abc/def\t");
      assert_err(check_name("/abc/def\n"), e1+"/abc/def\n");

      {DBout dbn(".","abc",cout);    assert_eq(dbn.name, "abc");
                            assert_eq(dbn.col, -1);}
      {DBout dbn(".","abc_",cout);   assert_eq(dbn.name, "abc_");}

      {DBout dbn(".","abc:1",cout);
         assert_eq(dbn.name, "abc"); assert_eq(dbn.col, 1);}
      {DBout dbn(".","abc:-1",cout); assert_eq(dbn.col, -1);}
      {DBout dbn(".","abc:-2",cout); assert_eq(dbn.col, -1);}
    }
    {
      DBinfo hh1; // default constructor
      DBinfo hh2(DATA_INT16);

      assert_eq(hh1.val, DEFAULT_DATAFMT);
      assert_eq(hh1.dsize(), data_fmt_sizes[DEFAULT_DATAFMT]);

      assert_eq(hh2.val, DATA_INT16);
      assert_eq(hh2.dsize(), 2); // int16 -> 2 bytes
      assert_eq(hh2.dname(), "INT16");
    }

    {
      // pack/unpack timestamps - v1
      DBinfo hh1(DATA_DOUBLE);
      DBinfo hh2(DATA_INT16);

      std::string ts1  = "1234567890.123000000";
      std::string ts2  = "0.000000000";
      std::string ts3  = "18446744073709551.615000000";

      string d1  = hh1.parse_time_v1(ts1);
      assert_eq(d1.size(),  8);
      assert_eq(hh1.print_time_v1(d1), ts1);

      string d2  = hh1.parse_time_v1(ts2);
      assert_eq(d2.size(),  8);
      assert_eq(hh1.print_time_v1(d2), ts2);

      assert_eq(hh2.print_time_v1(hh1.parse_time_v1(ts3)), ts3); //max

      assert_eq(hh2.print_time_v1(hh1.parse_time_v1("1")), "1.000000000");
      assert_eq(hh2.print_time_v1(hh1.parse_time_v1("1.")), "1.000000000");
      assert_eq(hh2.print_time_v1(hh1.parse_time_v1("1.000")), "1.000000000");
      assert_eq(hh2.print_time_v1(hh1.parse_time_v1("1.123")), "1.123000000");
      assert_eq(hh2.print_time_v1(hh1.parse_time_v1("1.12345")), "1.123000000");

    }
    {
      // pack/unpack timestamps - v2
      DBinfo hh1(DATA_DOUBLE);

      std::string ts1  = "1234567890.000000000";
      std::string ts2  = "1234567890.123456789";
      string d1  = hh1.parse_time_v2(ts1);
      assert_eq(d1.size(),  4);
      assert_eq(hh1.print_time_v2(d1), ts1);
      string d2  = hh1.parse_time_v2(ts2);
      assert_eq(d2.size(),  8);
      assert_eq(hh1.print_time_v2(d2), ts2);

      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("0.0")), "0.000000000");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("1")), "1.000000000");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("1.")), "1.000000000");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("1.1")), "1.100000000");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("1.001")), "1.001000000");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("123456789.123456789")), "123456789.123456789");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("123456789.12345678999")), "123456789.123456789");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("4294967295.12345678999")), "4294967295.123456789"); // max
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("123456789.12345678999+")), "123456789.123456790");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("123456789.12345678999-")), "123456789.123456788");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("0+")), "0.000000001");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("0-")), "4294967295.999999999");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("0.-")), "4294967295.999999999");
      assert_eq(hh1.print_time_v2(hh1.parse_time_v2("0.00-")), "4294967295.999999999");


      assert_err(hh1.parse_time_v2(""), "Bad timestamp: can't read seconds: ");
      assert_err(hh1.parse_time_v2("a"), "Bad timestamp: can't read seconds: a");
      assert_err(hh1.parse_time_v2("1234567890123"), "Bad timestamp: can't read seconds: 1234567890123"); // >2^32
      assert_err(hh1.parse_time_v2("1,"), "Bad timestamp: can't read decimal dot: 1,");
      assert_err(hh1.parse_time_v2("1.a"), "Bad timestamp: can't read nanoseconds: 1.a");
      assert_err(hh1.parse_time_v2("1.+a"), "Bad timestamp: can't read nanoseconds: 1.+a");
      assert_err(hh1.parse_time_v2("1.00-a"), "Bad timestamp: can't read nanoseconds: 1.00-a");
      assert_err(hh1.parse_time_v2("1.00-1"), "Bad timestamp: can't read nanoseconds: 1.00-1");
      assert_err(hh1.parse_time_v2("1.00a-"), "Bad timestamp: can't read nanoseconds: 1.00a-");
    }
    {
      // cmp_time_v1
      DBinfo hh1(DATA_DOUBLE);
      assert_eq(hh1.cmp_time_v1(hh1.parse_time_v1("0.0"), hh1.parse_time_v1("0.0")), 0);
      assert_eq(hh1.cmp_time_v1(hh1.parse_time_v1("1.2"), hh1.parse_time_v1("1.1")), 1);
      assert_eq(hh1.cmp_time_v1(hh1.parse_time_v1("999.2"), hh1.parse_time_v1("1000.1")), -1);
    }
    {
      // cmp_time_v2
      DBinfo hh1(DATA_DOUBLE);
      assert_eq(hh1.cmp_time_v2(hh1.parse_time_v2("0.0"), hh1.parse_time_v2("0.0")), 0);
      assert_eq(hh1.cmp_time_v2(hh1.parse_time_v2("1.2"), hh1.parse_time_v2("1.1")), 1);
      assert_eq(hh1.cmp_time_v2(hh1.parse_time_v2("999.2"), hh1.parse_time_v2("1000.1")), -1);
    }
    {
      // is_zero_time_v1
      DBinfo hh1(DATA_DOUBLE);
      assert_eq(hh1.is_zero_time_v1(hh1.parse_time_v1("0.0")), 1);
      assert_eq(hh1.is_zero_time_v1(hh1.parse_time_v1("0.001")), 0);
      assert_eq(hh1.is_zero_time_v1(hh1.parse_time_v1("0.0001")), 1); // ms precision!
      assert_eq(hh1.is_zero_time_v1(hh1.parse_time_v1("100")), 0);
    }
    {
      // is_zero_time_v2
      DBinfo hh1(DATA_DOUBLE);
      assert_eq(hh1.is_zero_time_v2(hh1.parse_time_v2("0.0")), 1);
      assert_eq(hh1.is_zero_time_v2(hh1.parse_time_v2("0.001")), 0);
      assert_eq(hh1.is_zero_time_v2(hh1.parse_time_v2("0.0001")), 0);
      assert_eq(hh1.is_zero_time_v2(hh1.parse_time_v2("0.0000000001")), 1); // ns precision
      assert_eq(hh1.is_zero_time_v2(hh1.parse_time_v2("100")), 0);
    }
    {
      // add_time_v1
      DBinfo hh1(DATA_DOUBLE);
      assert_eq(hh1.print_time_v1(hh1.add_time_v1(hh1.parse_time_v1("1.5"), hh1.parse_time_v1("0.5"))), "2.000000000");
      assert_eq(hh1.print_time_v1(hh1.add_time_v1(hh1.parse_time_v1("1.999"), hh1.parse_time_v1("1.999"))), "3.998000000");
      assert_eq(hh1.print_time_v1(hh1.add_time_v1(hh1.parse_time_v1("10"), hh1.parse_time_v1("10"))), "20.000000000");
    }
    {
      // add_time_v2
      DBinfo hh1(DATA_DOUBLE);
      assert_eq(hh1.print_time_v2(hh1.add_time_v2(hh1.parse_time_v2("1.5"), hh1.parse_time_v2("0.5"))), "2.000000000");
      assert_eq(hh1.print_time_v2(hh1.add_time_v2(hh1.parse_time_v2("1.999"), hh1.parse_time_v2("1.999"))), "3.998000000");
      assert_eq(hh1.print_time_v2(hh1.add_time_v2(hh1.parse_time_v2("10"), hh1.parse_time_v2("10"))), "20.000000000");
      // 2^31+2^31 >= 2^32
      assert_err(hh1.add_time_v2(hh1.parse_time_v2("2147483648"), hh1.parse_time_v2("2147483648")), "add_time overfull");
    }

    {
      // time_diff_v1
      DBinfo hh1(DATA_DOUBLE);
      assert_eq(hh1.time_diff_v1(hh1.parse_time_v1("1.5"), hh1.parse_time_v1("0.5")), 1.0);
      assert_eq(hh1.time_diff_v1(hh1.parse_time_v1("1.999"), hh1.parse_time_v1("1.999")), 0.0);
      assert_eq(hh1.time_diff_v1(hh1.parse_time_v1("1.999"), hh1.parse_time_v1("2.9999")), -1.0); // ms precision!
      assert_eq(hh1.time_diff_v1(hh1.parse_time_v1("2147483648.001"), hh1.parse_time_v1("2147483648.000")), 0.001);
    }
    {
      // time_diff_v2
      DBinfo hh1(DATA_DOUBLE);
      assert_eq(hh1.time_diff_v2(hh1.parse_time_v2("1.5"), hh1.parse_time_v2("0.5")), 1.0);
      assert_eq(hh1.time_diff_v2(hh1.parse_time_v2("1.999"), hh1.parse_time_v2("1.999")), 0.0);
      assert_eq(hh1.time_diff_v2(hh1.parse_time_v2("1.999"), hh1.parse_time_v2("2.9999")), -1.0009); // ns precision!
      assert_eq(hh1.time_diff_v2(hh1.parse_time_v2("2147483648.001"), hh1.parse_time_v2("2147483648.000")), 0.001);
      assert_eq(hh1.time_diff_v2(hh1.parse_time_v2("2147483648.000000001"), hh1.parse_time_v2("2147483648.0")), 1e-9);
      assert_eq(hh1.time_diff_v2(hh1.parse_time_v2("2147483648"), hh1.parse_time_v2("2147483648.000000001")), -1e-9);
      assert_eq(hh1.time_diff_v2(hh1.parse_time_v2("2147483648.999999999"), hh1.parse_time_v2("0")), 2147483648.999999999);
      assert_eq(hh1.time_diff_v2(hh1.parse_time_v2("10"), hh1.parse_time_v2("10")), 00.0);
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
      assert_eq(hh1.print_data(d0), "0.4 1.4");
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
      assert_eq(hh1.print_data(hh1.parse_data(v1)), "314 628");
      assert_err(hh1.print_data(hh1.parse_data(v2)), "Can't put value into INT32 database: 3.1415");
      assert_err(hh1.print_data(hh1.parse_data(v3)), "Can't put value into INT32 database: pi"); //!!!

      // store in double DB
      assert_eq(hh2.print_data(hh2.parse_data(v1)), "314 628");
      assert_eq(hh2.print_data(hh2.parse_data(v2)), "3.1415 6.283");
      assert_err(hh2.print_data(hh2.parse_data(v3)), "Can't put value into DOUBLE database: pi");

      // store in text DB
      assert_eq(hh3.print_data(hh3.parse_data(v1)), "314 628");
      assert_eq(hh3.print_data(hh3.parse_data(v2)), "3.1415 6.2830");
      assert_eq(hh3.print_data(hh3.parse_data(v3)), "pi 2pi");

      // colums
      assert_eq(hh1.print_data(hh1.parse_data(v1), 0), "314");
      assert_eq(hh1.print_data(hh1.parse_data(v1), 1), "628");
      assert_eq(hh1.print_data(hh1.parse_data(v1), 2), "NaN");

      assert_eq(hh2.print_data(hh2.parse_data(v2), 0), "3.1415");
      assert_eq(hh2.print_data(hh2.parse_data(v2), 1), "6.283");
      assert_eq(hh2.print_data(hh2.parse_data(v2), 2), "NaN");

      // column is ignored for the text database
      assert_eq(hh3.print_data(hh3.parse_data(v2), 0), "3.1415 6.2830");
      assert_eq(hh3.print_data(hh3.parse_data(v2), 1), "3.1415 6.2830");
      assert_eq(hh3.print_data(hh3.parse_data(v2), 2), "3.1415 6.2830");
    }

    // sizes and names
    assert_eq(data_fmt_sizes[DATA_TEXT],   1);
    assert_eq(data_fmt_sizes[DATA_INT8],   1);
    assert_eq(data_fmt_sizes[DATA_UINT8],  1);
    assert_eq(data_fmt_sizes[DATA_INT16],  2);
    assert_eq(data_fmt_sizes[DATA_UINT16], 2);
    assert_eq(data_fmt_sizes[DATA_INT32],  4);
    assert_eq(data_fmt_sizes[DATA_UINT32], 4);
    assert_eq(data_fmt_sizes[DATA_INT64],  8);
    assert_eq(data_fmt_sizes[DATA_UINT64], 8);
    assert_eq(data_fmt_sizes[DATA_FLOAT],  4);
    assert_eq(data_fmt_sizes[DATA_DOUBLE], 8);

    assert_eq(data_fmt_names[DATA_TEXT],   "TEXT"  );
    assert_eq(data_fmt_names[DATA_INT8],   "INT8"  );
    assert_eq(data_fmt_names[DATA_UINT8],  "UINT8" );
    assert_eq(data_fmt_names[DATA_INT16],  "INT16" );
    assert_eq(data_fmt_names[DATA_UINT16], "UINT16");
    assert_eq(data_fmt_names[DATA_INT32],  "INT32" );
    assert_eq(data_fmt_names[DATA_UINT32], "UINT32");
    assert_eq(data_fmt_names[DATA_INT64],  "INT64" );
    assert_eq(data_fmt_names[DATA_UINT64], "UINT64");
    assert_eq(data_fmt_names[DATA_FLOAT],  "FLOAT" );
    assert_eq(data_fmt_names[DATA_DOUBLE], "DOUBLE");

    assert_eq(DATA_TEXT,   DBinfo::str2datafmt("TEXT"  ));
    assert_eq(DATA_INT8,   DBinfo::str2datafmt("INT8"  ));
    assert_eq(DATA_UINT8,  DBinfo::str2datafmt("UINT8" ));
    assert_eq(DATA_INT16,  DBinfo::str2datafmt("INT16" ));
    assert_eq(DATA_UINT16, DBinfo::str2datafmt("UINT16"));
    assert_eq(DATA_INT32,  DBinfo::str2datafmt("INT32" ));
    assert_eq(DATA_UINT32, DBinfo::str2datafmt("UINT32"));
    assert_eq(DATA_INT64,  DBinfo::str2datafmt("INT64" ));
    assert_eq(DATA_UINT64, DBinfo::str2datafmt("UINT64"));
    assert_eq(DATA_FLOAT,  DBinfo::str2datafmt("FLOAT" ));
    assert_eq(DATA_DOUBLE, DBinfo::str2datafmt("DOUBLE"));
    assert_err(DBinfo::str2datafmt("X"), "Unknown data format: X");

/***************************************************************/
    // creating database, writing/reading data format
    DBinfo hh1(DATA_INT16, "AAA");
    DBinfo hh2;
    {
      DBpool pool(".", false, "txn");
      DBgr db = pool.get("test", DB_CREATE);
      db.write_info(hh1);
      hh2 = db.read_info();
      assert_eq(hh1.val, hh2.val);
      assert_eq(hh1.descr, hh2.descr);

      hh1.val = DATA_DOUBLE;
      hh1.descr = "Description";
      db.write_info(hh1);
      hh2 = db.read_info();
      assert_eq(hh1.val, hh2.val);
      assert_eq(hh1.descr, hh2.descr);
    }

    {
      DBpool pool(".", true, "txn");
      DBgr db1 = pool.get("test", DB_RDONLY);
      hh2 = db1.read_info();
      assert_eq(hh1.val, hh2.val);
      assert_eq(hh1.descr, hh2.descr);
    }

/***************************************************************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
  }
}
