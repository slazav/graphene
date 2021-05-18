#include <iostream>
#include <vector>
#include <string>

#include "err/err.h"
#include "err/assert_err.h"

#include "gr_env.h"

using namespace std;
int main() {
  try{
    // creating database, writing/reading data format
      GrapheneEnv env(".", false, "txn");
      GrapheneDB db = env.getdb("test", DB_CREATE);
      db.set_dtype(DATA_INT16);
      db.set_descr("AAA");

      GrapheneEnv env1(".", true, "txn");
      GrapheneDB db1 = env1.getdb("test", DB_RDONLY);
      assert_eq(db1.get_ttype(), TIME_V2);
      assert_eq(db1.get_dtype(), DATA_INT16);
      assert_eq(db1.get_descr(), "AAA");

/***************************************************************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
    return 1;
  }
  return 0;
}
