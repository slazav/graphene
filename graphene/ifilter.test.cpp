#include <iostream>
#include <vector>
#include <string>

#include "err/assert_err.h"

#include "ifilter.h"

using namespace std;
int main() {
  try{
  /***************/

  iFilter f1;
  assert_eq(f1.get_code(), "");

  f1.set_code("abc");
  assert_eq(f1.get_code(), "abc");

  std::string t("1234567890.123456789");
  std::vector<std::string> vs;
  vs.push_back("0.1");
  vs.push_back("0.2");

  assert_err(f1.run(t, vs),
    "ifilter: can't run TCL script: invalid command name \"abc\"\n"
    "    while executing\n\"abc\"");

  f1.set_code("return 1");
  assert_eq(f1.run(t, vs), true);

  assert_eq(vs.size(), 2);
  assert_eq(vs[0], "0.1");
  assert_eq(vs[1], "0.2");
  assert_eq(t, "1234567890.123456789");

  f1.set_code("return 0");
  assert_eq(f1.run(t, vs), false);

  f1.set_code("return 10");
  assert_eq(f1.run(t, vs), true);

  f1.set_code("return abc");
  assert_eq(f1.run(t, vs), true);

  // data and time modification, save old data to storage

  assert_eq(f1.get_storage(), "");
  f1.set_code("set time [expr $time+1]; set storage \"$time $data\";"
              "set data 0.34; return 1");
  assert_eq(f1.run(t, vs), true)
  assert_eq(vs.size(), 1);
  assert_eq(vs[0], "0.34");
  assert_eq(t, "1234567891.1234567");
  assert_eq(f1.get_storage(), "1234567891.1234567 0.1 0.2");

  // change storage format: list

  f1.set_code("set storage [list [list a b c] 1 2 3]; return true;");
  assert_eq(f1.run(t, vs), true)
  assert_eq(f1.get_storage(), "{a b c} 1 2 3");
  f1.set_code("set storage [lindex $storage 0]; return true;");
  assert_eq(f1.run(t, vs), true)
  assert_eq(f1.get_storage(), "a b c");

  /***************/
  } catch (Err E){
    std::cerr << E.str() << "\n";
    return 1;
  }
  return 0;
}
