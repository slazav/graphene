#include <iostream>
#include <vector>
#include <string>

#include "err/err.h"
#include "err/assert_err.h"

#include "dbpool.h"

using namespace std;
int main() {
  try{
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
