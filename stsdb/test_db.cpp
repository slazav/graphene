/* test some C-functions */
#include <iostream>
#include <vector>
#include <string>

#include "tests.h"
#include "db.h"
#include "dbout.h"

using namespace std;
int main() {
  try{
/***************************************************************/
    // FMT object, data and time formats
    ASSERT_EQ(DEFAULT_DATAFMT, DATA_DOUBLE);

    {  // check names and extract column numbers
      DBsts::check_name("abc");
      DBsts::check_name("abc/def");
      const char *e1 = "symbols '.:+| \\n\\t' are not allowed in the database name";
      ASSERT_EX(DBsts::check_name("./abc/def"), e1);
      ASSERT_EX(DBsts::check_name("/abc/def:1"), e1);
      ASSERT_EX(DBsts::check_name("/abc/def+1"), e1);
      ASSERT_EX(DBsts::check_name("/abc/def 1"), e1);
      ASSERT_EX(DBsts::check_name("/abc/def\t"), e1);
      ASSERT_EX(DBsts::check_name("/abc/def\n"), e1);

      {DBout dbn("abc");    ASSERT_EQ(dbn.name, "abc");
                             ASSERT_EQ(dbn.col, -1);}
      {DBout dbn("abc/");   ASSERT_EQ(dbn.name, "abc/");}

      {DBout dbn("abc:1");
         ASSERT_EQ(dbn.name, "abc"); ASSERT_EQ(dbn.col, 1);}
      {DBout dbn("abc:-1"); ASSERT_EQ(dbn.col, -1);}
      {DBout dbn("abc:-2"); ASSERT_EQ(dbn.col, -1);}
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
      // pack/unpack timestamps
      DBinfo hh1(DATA_DOUBLE);
      DBinfo hh2(DATA_INT16);

      uint64_t ts  = 1234567890123;
      string d1  = hh1.pack_time(ts);
      ASSERT_EQ(d1.size(),  8);
      ASSERT_EQ(hh1.unpack_time(d1),   ts);

      ASSERT_EQ(hh2.unpack_time(hh1.pack_time(0)), 0);
      ASSERT_EQ(hh2.unpack_time(
        hh1.pack_time(0xFFFFFFFFFFFFFFFF)), 0xFFFFFFFFFFFFFFFF); //max
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
      ASSERT_EQ(hh1.unpack_data(hh1.pack_data(v1)), "314 628");
      ASSERT_EX(hh1.unpack_data(hh1.pack_data(v2)), "Can't put value into INT32 database: 3.1415");
      ASSERT_EX(hh1.unpack_data(hh1.pack_data(v3)), "Can't put value into INT32 database: pi"); //!!!

      // store in double DB
      ASSERT_EQ(hh2.unpack_data(hh2.pack_data(v1)), "314 628");
      ASSERT_EQ(hh2.unpack_data(hh2.pack_data(v2)), "3.1415 6.283");
      ASSERT_EX(hh2.unpack_data(hh2.pack_data(v3)), "Can't put value into DOUBLE database: pi");

      // store in text DB
      ASSERT_EQ(hh3.unpack_data(hh3.pack_data(v1)), "314 628");
      ASSERT_EQ(hh3.unpack_data(hh3.pack_data(v2)), "3.1415 6.2830");
      ASSERT_EQ(hh3.unpack_data(hh3.pack_data(v3)), "pi 2pi");

      // colums
      ASSERT_EQ(hh1.unpack_data(hh1.pack_data(v1), 0), "314");
      ASSERT_EQ(hh1.unpack_data(hh1.pack_data(v1), 1), "628");
      ASSERT_EQ(hh1.unpack_data(hh1.pack_data(v1), 2), "NaN");

      ASSERT_EQ(hh2.unpack_data(hh2.pack_data(v2), 0), "3.1415");
      ASSERT_EQ(hh2.unpack_data(hh2.pack_data(v2), 1), "6.283");
      ASSERT_EQ(hh2.unpack_data(hh2.pack_data(v2), 2), "NaN");

      // column is ignored for the text database
      ASSERT_EQ(hh3.unpack_data(hh3.pack_data(v2), 0), "3.1415 6.2830");
      ASSERT_EQ(hh3.unpack_data(hh3.pack_data(v2), 1), "3.1415 6.2830");
      ASSERT_EQ(hh3.unpack_data(hh3.pack_data(v2), 2), "3.1415 6.2830");
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
      DBsts db(".", "test", DB_CREATE | DB_TRUNCATE);
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
      DBsts db1(".", "test", DB_RDONLY);
      hh2 = db1.read_info();
      ASSERT_EQ(hh1.val, hh2.val);
      ASSERT_EQ(hh1.descr, hh2.descr);
    }

/***************************************************************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
  }
}
