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
      DBpool pool(".", false, "txn");
      DBgr db = pool.get("test", DB_CREATE);
      db.dtype = DATA_INT16;
      db.descr = "AAA";
      db.write_info();
      db.read_info();
      assert_eq(db.version, 2);
      assert_eq(db.ttype, TIME_V2);
      assert_eq(db.dtype, DATA_INT16);
      assert_eq(db.descr, "AAA");

      DBpool pool1(".", true, "txn");
      DBgr db1 = pool1.get("test", DB_RDONLY);
      db1.read_info();
      assert_eq(db.ttype, db1.ttype);
      assert_eq(db.dtype, db1.dtype);
      assert_eq(db.descr, db1.descr);

/***************************************************************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
    return 1;
  }
  return 0;
}
