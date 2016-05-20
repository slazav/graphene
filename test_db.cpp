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

    {
      DBhead hh1; // default constructor
      DBhead hh2(TIME_MS, DATA_INT16);

      ASSERT_EQ(hh1.key, DEFAULT_TIMEFMT);
      ASSERT_EQ(hh1.val, DEFAULT_DATAFMT);
      ASSERT_EQ(hh1.dsize(), data_fmt_sizes[DEFAULT_DATAFMT]);

      ASSERT_EQ(hh2.key, TIME_MS);
      ASSERT_EQ(hh2.val, DATA_INT16);
      ASSERT_EQ(hh2.dsize(), 2); // int16 -> 2 bytes
      ASSERT_EQ(hh2.dname(), "INT16");
      ASSERT_EQ(hh2.tname(), "MS");
    }

    {
      // pack/unpack timestamps
      DBhead hh1(TIME_S, DATA_DOUBLE);
      DBhead hh2(TIME_MS, DATA_INT16);

      uint64_t ts  = 1463643547;    // s
      uint64_t tms = ts*1000 + 823; // ms
      string d1  = hh1.pack_time(ts);   // s->s
      string d1a = hh1.pack_time(tms);  // ms->s
      string d2  = hh2.pack_time(ts);   // s->ms
      string d2a = hh2.pack_time(tms);  // ms->ms

      ASSERT_EQ(d1.size(),  4);
      ASSERT_EQ(d1a.size(), 4);
      ASSERT_EQ(d2.size(),  8);
      ASSERT_EQ(d2a.size(), 8);

      ASSERT_EQ(hh1.unpack_time(d1),   ts);
      ASSERT_EQ(hh1.unpack_time(d1a),  ts);
      ASSERT_EQ(hh2.unpack_time(d2),   ts*1000); // data have been rounded
      ASSERT_EQ(hh2.unpack_time(d2a),  tms);

      ASSERT_EQ(hh1.unpack_time(hh1.pack_time(2234567890)), 2234567890);    // seconds
      ASSERT_EQ(hh1.unpack_time(hh1.pack_time(2234567890123)), 2234567890); //ms
      ASSERT_EQ(hh1.unpack_time(hh1.pack_time(0xFFFFFFFE)), 0xFFFFFFFE);    // how do we care about 2^31<t<2^32?
      ASSERT_EQ(hh1.unpack_time(hh1.pack_time(0xFFFFFFFF)), 0xFFFFFFFF/1000); // s - ms boundary
      ASSERT_EQ(hh1.unpack_time(hh1.pack_time(4294967295000)), 4294967295); // ms
      ASSERT_EQ(hh1.unpack_time(hh1.pack_time(5294967295000)), 4294967295); // we keep seconds in 32 bit, larger values are truncated

      ASSERT_EQ(hh2.unpack_time(hh2.pack_time(2234567890)), 2234567890000);    // seconds
      ASSERT_EQ(hh2.unpack_time(hh2.pack_time(2234567890123)), 2234567890123); //ms
      ASSERT_EQ(hh2.unpack_time(hh2.pack_time(4294967294)), 4294967294000);    // how do we care about 2^31<t<2^32?
      ASSERT_EQ(hh2.unpack_time(hh2.pack_time(4294967295)), 4294967295);       // s - ms boundary
      ASSERT_EQ(hh2.unpack_time(hh2.pack_time(4294967295000)), 4294967295000); // ms
      ASSERT_EQ(hh2.unpack_time(hh2.pack_time(5294967295000)), 5294967295000); // we keep timestamp in 64 bit, no need to truncate
    }

    {
      // pack/unpack data
      DBhead hh1(TIME_S, DATA_INT32);
      DBhead hh2(TIME_S, DATA_DOUBLE);
      DBhead hh3(TIME_S, DATA_TEXT);

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
