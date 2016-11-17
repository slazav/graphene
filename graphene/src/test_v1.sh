#!/bin/sh -u

# test if new graphene program can read v1 databases
# correctly

function assert(){
  if [ "$1" != "$2" ]; then
    printf "ERROR:\n"
    printf "  res:\n%s\n" "$1";
    printf "  exp:\n%s\n" "$2";
    exit 1
  fi
}

prog="graphene -d v1"

assert "$($prog info tab1)" "INT16	Int-16 database"
assert "$($prog info tab2)" "DOUBLE	Double database"
assert "$($prog info tab3)" "TEXT	Text database"


assert "$($prog get_range tab1)" "\
1234567890000 0
1234567891000 1
1234567892000 2
1234567893000 3"

assert "$($prog get_range tab2)" "\
1234567890000 0
1234567891000 0.1
1234567892000 0.2
1234567893000 0.3"

assert "$($prog get_range tab3)" "\
1234567890000 text 1
1234567891000 text 2
1234567892000 text 3
1234567893000 text 4"

assert "$($prog get_prev tab1 1234567891500)" "1234567891000 1"
assert "$($prog get_prev tab2 1234567891500)" "1234567891000 0.1"
assert "$($prog get_prev tab3 1234567891500)" "1234567891000 text 2"

assert "$($prog get_next tab1 1234567891500)" "1234567892000 2"
assert "$($prog get_next tab2 1234567891500)" "1234567892000 0.2"
assert "$($prog get_next tab3 1234567891500)" "1234567892000 text 3"

assert "$($prog get tab1 1234567891500)" "1234567891000 1"
assert "$($prog get tab2 1234567891500)" "1234567891500 0.15"
assert "$($prog get tab3 1234567891500)" "1234567891000 text 2"
