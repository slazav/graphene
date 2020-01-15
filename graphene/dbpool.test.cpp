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

      DBpool pool1(".", true, "txn");
      DBgr db1 = pool1.get("test", DB_RDONLY);
      assert_eq(db1.ttype, TIME_V2);
      assert_eq(db1.dtype, DATA_INT16);
      assert_eq(db1.descr, "AAA");

/***************************************************************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
    return 1;
  }
  return 0;
}
