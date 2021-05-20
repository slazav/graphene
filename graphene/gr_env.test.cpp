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
    {
      GrapheneEnv env(".", false, "txn", "");
      env.dbcreate("test", "AAA", DATA_INT16);
    }
    {
      GrapheneEnv env1(".", true, "txn", "");
      assert_eq(env1.get_ttype("test"), TIME_V2);
      assert_eq(env1.get_dtype("test"), DATA_INT16);
      assert_eq(env1.get_descr("test"), "AAA");
    }

/***************************************************************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
    return 1;
  }
  return 0;
}
