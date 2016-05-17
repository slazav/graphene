#include <iostream>
#include <vector>
#include <string>

#include "tests.h"
#include "stsdb.h"

using namespace std;
int main() {
  try{
/***************************************************************/
    // FMT object, data and time formats
    ASSERT_EQ(DEFAULT_TIMEFMT, TIME_S);
    ASSERT_EQ(DEFAULT_DATAFMT, DATA_DOUBLE);

    DBhead f1; // default constructor
    ASSERT_EQ(f1.key, DEFAULT_TIMEFMT);
    ASSERT_EQ(f1.val, DEFAULT_DATAFMT);
    ASSERT_EQ(f1.dsize(), data_fmt_sizes[DEFAULT_DATAFMT]);

    DBhead f2(TIME_MS, DATA_INT16);
    ASSERT_EQ(f2.key, TIME_MS);
    ASSERT_EQ(f2.val, DATA_INT16);
    ASSERT_EQ(f2.dsize(), 2); // int16 -> 2 bytes
    ASSERT_EQ(f2.dname(), "INT16");
    ASSERT_EQ(f2.tname(), "MS");

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

    ASSERT_EQ(time_fmt_names[TIME_S],   "S"  );
    ASSERT_EQ(time_fmt_names[TIME_MS],  "MS" );

    ASSERT_EQ(DATA_TEXT,   DBhead::str2datafmt("TEXT"  ));
    ASSERT_EQ(DATA_INT8,   DBhead::str2datafmt("INT8"  ));
    ASSERT_EQ(DATA_UINT8,  DBhead::str2datafmt("UINT8" ));
    ASSERT_EQ(DATA_INT16,  DBhead::str2datafmt("INT16" ));
    ASSERT_EQ(DATA_UINT16, DBhead::str2datafmt("UINT16"));
    ASSERT_EQ(DATA_INT32,  DBhead::str2datafmt("INT32" ));
    ASSERT_EQ(DATA_UINT32, DBhead::str2datafmt("UINT32"));
    ASSERT_EQ(DATA_INT64,  DBhead::str2datafmt("INT64" ));
    ASSERT_EQ(DATA_UINT64, DBhead::str2datafmt("UINT64"));
    ASSERT_EQ(DATA_FLOAT,  DBhead::str2datafmt("FLOAT" ));
    ASSERT_EQ(DATA_DOUBLE, DBhead::str2datafmt("DOUBLE"));
    ASSERT_EX(DBhead::str2datafmt("X"), "Unknown data format: X");

    ASSERT_EQ(TIME_S,   DBhead::str2timefmt("S"));
    ASSERT_EQ(TIME_MS,  DBhead::str2timefmt("MS"));
    ASSERT_EX(DBhead::str2timefmt("X"), "Unknown time format: X");
/***************************************************************/
    // creating database, writing/reading data format
    DBhead hh1(TIME_MS, DATA_INT16, "AAA");
    DBhead hh2;
    {
      DBsts db(".", "test", DB_CREATE | DB_TRUNCATE);
      db.write_info(hh1);
      hh2 = db.read_info();
      ASSERT_EQ(hh1.key, hh2.key);
      ASSERT_EQ(hh1.val, hh2.val);
      ASSERT_EQ(hh1.descr, hh2.descr);

      hh1.val = DATA_DOUBLE;
      hh1.descr = "Description";
      db.write_info(hh1);
      hh2 = db.read_info();
      ASSERT_EQ(hh1.key, hh2.key);
      ASSERT_EQ(hh1.val, hh2.val);
      ASSERT_EQ(hh1.descr, hh2.descr);
    }
    {
      DBsts db1(".", "test", DB_RDONLY);
      hh2 = db1.read_info();
      ASSERT_EQ(hh1.key, hh2.key);
      ASSERT_EQ(hh1.val, hh2.val);
      ASSERT_EQ(hh1.descr, hh2.descr);
    }



/***************************************************************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
  }
}
