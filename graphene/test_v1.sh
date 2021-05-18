#!/bin/bash -u

# test if new graphene program can read v1 databases
# correctly

. ../modules/test_lib.sh

function assert(){
  if [ "$1" != "$2" ]; then
    printf "ERROR:\n"
    printf "  res:\n%s\n" "$1";
    printf "  exp:\n%s\n" "$2";
    exit 1
  fi
}
rm -f -- v1/__db.* v1/log.*

prog="./graphene -d v1"

assert_cmd "$prog info tab1" "INT16	Int-16 database"
assert_cmd "$prog info tab2" "DOUBLE	Double database"
assert_cmd "$prog info tab3" "TEXT	Text database"


assert_cmd "$prog get_range tab1" "\
1234567890.000000000 0
1234567891.000000000 1
1234567892.001000000 2
1234567893.002000000 3"

assert_cmd "$prog get_range tab2" "\
1234567890.000000000 0
1234567891.000000000 0.1
1234567892.001000000 0.2
1234567893.002000000 0.3"

assert_cmd "$prog get_range tab3" "\
1234567890.000000000 text 1
1234567891.000000000 text 2
1234567892.001000000 text 3
1234567893.002000000 text 4"

assert_cmd "$prog get_prev tab1 1234567891.500" "1234567891.000000000 1"
assert_cmd "$prog get_prev tab2 1234567891.500" "1234567891.000000000 0.1"
assert_cmd "$prog get_prev tab3 1234567891.500" "1234567891.000000000 text 2"

assert_cmd "$prog get_next tab1 1234567891.500" "1234567892.001000000 2"
assert_cmd "$prog get_next tab2 1234567891.500" "1234567892.001000000 0.2"
assert_cmd "$prog get_next tab3 1234567891.500" "1234567892.001000000 text 3"

assert_cmd "$prog get tab1 1234567890.500" "1234567890.000000000 0"
assert_cmd "$prog get tab2 1234567890.500" "1234567890.500000000 0.05"
assert_cmd "$prog get tab3 1234567890.500" "1234567890.000000000 text 1"



