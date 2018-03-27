#!/bin/bash -u

# test command line interface program ./graphene

function assert(){
  if [ "$1" != "$2" ]; then
    printf "ERROR:\n"
    printf "  res:\n%s\n" "$1";
    printf "  exp:\n%s\n" "$2";
    exit 1
  fi
}

###########################################################################
# help message
help_msg="$(./graphene -h)"

# no commands - help message
assert "$(./graphene)" "#Error: command is expected"
assert "$(./graphene -d .)" "#Error: command is expected"

# invalid option (error printed by getopt)
assert "$(./graphene -X . 2>&1)" "./graphene: invalid option -- 'X'"

# unknown command
assert "$(./graphene -d . a)" "#Error: Unknown command: a"

assert "$(./graphene -d . *idn?)" "Graphene database 2.4"


###########################################################################
# database operations

# remove all test databases
for i in *.db; do
  [ -f $i ] || continue
  ./graphene -d . delete ${i%.db}
done

rm -f -- __db.* log.*

# create
assert "$(./graphene -d . create)" "#Error: database name expected"
assert "$(./graphene -d . create a dfmt)" "#Error: Unknown data format: dfmt"

assert "$(./graphene -d . create test_1)" ""
assert "$(./graphene -d . create test_2 UINT16)" ""
assert "$(./graphene -d . create test_3 uint32 "Uint 32 database")" ""
assert "$(./graphene -d . create test_4 uint32 Uint 32 database)" ""
assert "$(./graphene -d . create test_1)" "#Error: test_1.db: File exists"

# info
assert "$(./graphene -d . info)" "#Error: database name expected"
assert "$(./graphene -d . info a b)" "#Error: too many parameters"
assert "$(./graphene -d . info test_x)" "#Error: test_x.db: No such file or directory"

assert "$(./graphene -d . info test_1)" "DOUBLE"
assert "$(./graphene -d . info test_2)" "UINT16"
assert "$(./graphene -d . info test_3)" "UINT32	Uint 32 database"
assert "$(./graphene -d . info test_4)" "UINT32	Uint 32 database"

# list
assert "$(./graphene -d . list a)" "#Error: too many parameters"
assert "$(./graphene -d . list | sort)" "$(printf "test_1\ntest_2\ntest_3\ntest_4")"

# delete
assert "$(./graphene -d . delete)" "#Error: database name expected"
assert "$(./graphene -d . delete a b)" "#Error: too many parameters"
assert "$(./graphene -d . delete test_x)" "#Error: test_x.db: No such file or directory"
assert "$(./graphene -d . delete test_2)" ""
assert "$(./graphene -d . delete test_2)" "#Error: test_2.db: No such file or directory"

# rename
assert "$(./graphene -d . rename)" "#Error: database old and new names expected"
assert "$(./graphene -d . rename a)" "#Error: database old and new names expected"
assert "$(./graphene -d . rename a b c)" "#Error: too many parameters"
assert "$(./graphene -d . rename test_x test_y)" "#Error: renaming test_x.db -> test_y.db: No such file or directory"
assert "$(./graphene -d . rename test_3 test_1)" "#Error: renaming test_3.db -> test_1.db: Destination exists"
assert "$(./graphene -d . rename test_3 test_2)" ""
assert "$(./graphene -d . info test_3)" "#Error: test_3.db: No such file or directory"
assert "$(./graphene -d . info test_2)" "UINT32	Uint 32 database"

# set_descr
assert "$(./graphene -d . set_descr)" "#Error: database name and new description text expected"
assert "$(./graphene -d . set_descr a)" "#Error: database name and new description text expected"
assert "$(./graphene -d . set_descr test_x a)" "#Error: test_x.db: No such file or directory"
assert "$(./graphene -d . set_descr test_1 "Test DB number 1")" ""
assert "$(./graphene -d . info test_1)" "DOUBLE	Test DB number 1"
assert "$(./graphene -d . set_descr test_1 Test DB number 1)" ""
assert "$(./graphene -d . info test_1)" "DOUBLE	Test DB number 1"

assert "$(./graphene -d . delete test_1)" ""
assert "$(./graphene -d . delete test_2)" ""

###########################################################################
# DOUBLE database

assert "$(./graphene -d . create test_1)" ""
assert "$(./graphene -d . put test_1 1234567890.000 0.1)" ""
assert "$(./graphene -d . put test_1 2234567890.123 0.2)" ""
# get_next
assert "$(./graphene -d . get_next test_1)"                "1234567890.000000000 0.1" # first
assert "$(./graphene -d . get_next test_1 0)"              "1234567890.000000000 0.1"
assert "$(./graphene -d . get_next test_1 1000000000.000)" "1234567890.000000000 0.1"
assert "$(./graphene -d . get_next test_1 1234567890.000)" "1234567890.000000000 0.1" # ==
assert "$(./graphene -d . get_next test_1 1234567895.000)" "2234567890.123000000 0.2"
assert "$(./graphene -d . get_next test_1 now)"            "2234567890.123000000 0.2"
assert "$(./graphene -d . get_next test_1 NoW)"            "2234567890.123000000 0.2"
assert "$(./graphene -d . get_next test_1 NoW_s)"          "2234567890.123000000 0.2"
assert "$(./graphene -d . get_next test_1 inf)"            ""
assert "$(./graphene -d . get_next test_1 2000000000.124)" "2234567890.123000000 0.2"
assert "$(./graphene -d . get_next test_1 3000000000)" ""
assert "$(./graphene -r -d . get_next test_1 2000000000.124)" "234567889.999000013 0.2" # relative (note the rounding error)
# get_prev
assert "$(./graphene -d . get_prev test_1)"                "2234567890.123000000 0.2" # last
assert "$(./graphene -d . get_prev test_1 0)"              ""
assert "$(./graphene -d . get_prev test_1 1234567890)"     "1234567890.000000000 0.1" # ==
assert "$(./graphene -d . get_prev test_1 2234567890.123)" "2234567890.123000000 0.2" # ==
assert "$(./graphene -d . get_prev test_1 1234567895)"     "1234567890.000000000 0.1"
assert "$(./graphene -d . get_prev test_1 now)"            "1234567890.000000000 0.1"
assert "$(./graphene -d . get_prev test_1 inf)"            "2234567890.123000000 0.2"
assert "$(./graphene -d . get_prev test_1 3234567895)"     "2234567890.123000000 0.2"
assert "$(./graphene -r -d . get_prev test_1 2234567895)"     "-4.877000000 0.2" # relative

# get_range
assert "$(./graphene -d . get_range test_1 0 1234567880)"   ""
assert "$(./graphene -d . get_range test_1 0 1234567880 2)" ""
assert "$(./graphene -d . get_range test_1 0 1234567890)"   "1234567890.000000000 0.1"
assert "$(./graphene -d . get_range test_1 0 1234567890 3)" "1234567890.000000000 0.1"

assert "$(./graphene -d . get_range test_1 2234567891 3234567890)" ""
assert "$(./graphene -d . get_range test_1 2234567891 3234567890 2)" ""
assert "$(./graphene -d . get_range test_1 2234567890.123 3234567890)"   "2234567890.123000000 0.2"
assert "$(./graphene -d . get_range test_1 2234567890.123 3234567890 2)" "2234567890.123000000 0.2"

assert "$(./graphene -d . get_range test_1 0 2000000000)"   "1234567890.000000000 0.1"
assert "$(./graphene -d . get_range test_1 0 2000000000 3)" "1234567890.000000000 0.1"
assert "$(./graphene -d . get_range test_1 2000000000 3000000000)"   "2234567890.123000000 0.2"
assert "$(./graphene -d . get_range test_1 2000000000 3000000000 3)" "2234567890.123000000 0.2"

assert "$(./graphene -d . get_range test_1)" "1234567890.000000000 0.1
2234567890.123000000 0.2"
assert "$(./graphene -d . get_range test_1 0 3384967290 2)" "1234567890.000000000 0.1
2234567890.123000000 0.2"
assert "$(./graphene -d . get_range test_1 1234567890 2234567890.123 2)" "1234567890.000000000 0.1
2234567890.123000000 0.2"

assert "$(./graphene -r -d . get_range test_1 1234567890 2234567890.123 2)" "0.000000000 0.1
1000000000.123000026 0.2" # relative (note rounding error)

assert "$(./graphene -d . get_range test_1 1234567890 2234567890.123 1200000000)" "1234567890.000000000 0.1"
assert "$(./graphene -d . delete test_1)" ""

###########################################################################
# interpolation and columns (DOUBLE database)
assert "$(./graphene -d . create test_2 DOUBLE "double database")" ""

assert "$(./graphene -d . put test_2 1000 1 10 30)" ""
assert "$(./graphene -d . put test_2 2000 2 20)" ""

# get
assert "$(./graphene -d . get test_2 800)"    ""
assert "$(./graphene -d . get test_2 2200)"   "2000.000000000 2 20"
assert "$(./graphene -d . get test_2 1000)"   "1000.000000000 1 10 30"
assert "$(./graphene -d . get test_2 2000)"   "2000.000000000 2 20"
assert "$(./graphene -d . get test_2 1200)"   "1200.000000000 1.2 12"
assert "$(./graphene -d . get test_2 1800)"   "1800.000000000 1.8 18"
assert "$(./graphene -d . get test_2:1 1200)" "1200.000000000 12"
assert "$(./graphene -r -d . get test_2 2200)"   "-200.000000000 2 20" # relative
assert "$(./graphene -r -d . get test_2 1800)"   "0.000000000 1.8 18" # relative

# columns
assert "$(./graphene -d . get_next test_2:0)" "1000.000000000 1"
assert "$(./graphene -d . get_next test_2:1)" "1000.000000000 10"
assert "$(./graphene -d . get_next test_2:3)" "1000.000000000 NaN"
assert "$(./graphene -d . get_prev test_2:0)" "2000.000000000 2"
assert "$(./graphene -d . get_prev test_2:1)" "2000.000000000 20"
assert "$(./graphene -d . get_prev test_2:3)" "2000.000000000 NaN"

assert "$(./graphene -d . get_range test_2:0)" "1000.000000000 1
2000.000000000 2"

assert "$(./graphene -d . get_range test_2:3)" "1000.000000000 NaN
2000.000000000 NaN"
assert "$(./graphene -d . delete test_2)" ""

###########################################################################
# precision (DOUBLE database)
assert "$(./graphene -d . create test_2 DOUBLE "double database")" ""
assert "$(./graphene -d . create test_3 FLOAT  "float database")" ""

assert "$(./graphene -d . put test_2 1000 1.2345678901234567890123456789)" ""
assert "$(./graphene -d . put test_2 2000 1.2345678901234567890123456789e88)" ""
assert "$(./graphene -d . put test_3 1000 1.2345678901234567890123456789)" ""
assert "$(./graphene -d . put test_3 2000 1.2345678901234567890123456789e38)" ""

assert "$(./graphene -d . put test_3 2000 1e88)" "#Error: Can't put value into FLOAT database: 1e88"

assert "$(./graphene -d . get_range test_2)" "1000.000000000 1.234567890123457
2000.000000000 1.234567890123457e+88"
assert "$(./graphene -d . get_range test_3)" "1000.000000000 1.2345679
2000.000000000 1.2345679e+38"

assert "$(./graphene -d . delete test_2)" ""
assert "$(./graphene -d . delete test_3)" ""

###########################################################################
# deleting records

assert "$(./graphene -d . create test_3 UINT32 "Uint 32 database")" ""
assert "$(./graphene -d . put test_3 10 1)" ""
assert "$(./graphene -d . put test_3 11 2)" ""
assert "$(./graphene -d . put test_3 12 3)" ""
assert "$(./graphene -d . put test_3 13 4)" ""
assert "$(./graphene -d . put test_3 14 5)" ""
assert "$(./graphene -d . put test_3 15 6)" ""
assert "$(./graphene -d . get_range test_3 | wc)" "      6      12      90"
assert "$(./graphene -d . get_next test_3 12)" "12.000000000 3"
assert "$(./graphene -d . del test_3 12)" ""
assert "$(./graphene -d . get_next test_3 12)" "13.000000000 4"
assert "$(./graphene -d . del_range test_3 11 13)" ""
assert "$(./graphene -d . get_range test_3)" "10.000000000 1
14.000000000 5
15.000000000 6"

assert "$(./graphene -d . delete test_3)" ""

###########################################################################
# filters

echo "#!/bin/sh" > test_flt
echo "while read t v; do echo \$t \$((\$v*\$v)); done" >> test_flt
chmod 755 test_flt

assert "$(./graphene -d . create test_3)" ""
assert "$(./graphene -d . put test_3 10 1 4)" ""
assert "$(./graphene -d . put test_3 11 2 3)" ""
assert "$(./graphene -d . put test_3 12 3 2)" ""
assert "$(./graphene -d . put test_3 13 4 1)" ""
assert "$(./graphene -d . get_range test_3:0)" "10.000000000 1
11.000000000 2
12.000000000 3
13.000000000 4"
assert "$(./graphene -d . get_range 'test_3:0|test_flt')" "10.000000000 1
11.000000000 4
12.000000000 9
13.000000000 16"

rm -f test_flt
assert "$(./graphene -d . delete test_3)" ""
assert "$(./graphene -d . delete test_4)" ""

###########################################################################
# TEXT database
assert "$(./graphene -d . create test_4 TEXT)" ""

assert "$(./graphene -d . put test_4 1000 text1)" ""
assert "$(./graphene -d . put test_4 2000 "text2
2")" "" # line break will be converted to space
# get_next
assert "$(./graphene -d . get_next test_4)"      "1000.000000000 text1" # first
assert "$(./graphene -d . get_next test_4 0)"    "1000.000000000 text1"
assert "$(./graphene -d . get_next test_4 1000)" "1000.000000000 text1" # ==
assert "$(./graphene -d . get_next test_4 1500)" "2000.000000000 text2
2"
assert "$(./graphene -d . get_next test_4 2000)" "2000.000000000 text2
2"
assert "$(./graphene -d . get_next test_4 2001)" ""

# get_prev
assert "$(./graphene -d . get_prev test_4)"      "2000.000000000 text2
2" # last
assert "$(./graphene -d . get_prev test_4 0)"    ""
assert "$(./graphene -d . get_prev test_4 1000)" "1000.000000000 text1" # ==
assert "$(./graphene -d . get_prev test_4 2000)" "2000.000000000 text2
2" # ==
assert "$(./graphene -d . get_prev test_4 1500)" "1000.000000000 text1"
assert "$(./graphene -d . get_prev test_4 2001)" "2000.000000000 text2
2"

# get - same
assert "$(./graphene -d . get test_4)"      "2000.000000000 text2
2" # last
assert "$(./graphene -d . get test_4 0)"    ""
assert "$(./graphene -d . get test_4 1000)" "1000.000000000 text1" # ==
assert "$(./graphene -d . get test_4 2000)" "2000.000000000 text2
2" # ==
assert "$(./graphene -d . get test_4 1500)" "1000.000000000 text1"
assert "$(./graphene -d . get test_4 2001)" "2000.000000000 text2
2"

# get_range
assert "$(./graphene -d . get_range test_4 0 999)" ""
assert "$(./graphene -d . get_range test_4 0 999 2)" ""
assert "$(./graphene -d . get_range test_4 0 1000)"   "1000.000000000 text1"
assert "$(./graphene -d . get_range test_4 0 1000 3)" "1000.000000000 text1"

assert "$(./graphene -d . get_range test_4 2001 3000)" ""
assert "$(./graphene -d . get_range test_4 2001 3000 2)" ""
assert "$(./graphene -d . get_range test_4 2000 3000)" "2000.000000000 text2"
assert "$(./graphene -d . get_range test_4 2000 3000 2)" "2000.000000000 text2"

assert "$(./graphene -d . get_range test_4)" "1000.000000000 text1
2000.000000000 text2"
assert "$(./graphene -d . get_range test_4 1000 2000)" "1000.000000000 text1
2000.000000000 text2"
assert "$(./graphene -d . get_range test_4 1000 2000 1000)" "1000.000000000 text1
2000.000000000 text2"
assert "$(./graphene -d . get_range test_4 1000 2000 1001)" "1000.000000000 text1"

# columns are not important
assert "$(./graphene -d . get_next test_4:5)" "1000.000000000 text1"
assert "$(./graphene -d . delete test_4)" ""


###########################################################################
# interactive mode

#prompt
assert "$(./graphene -d . -i 1)" "#Error: too many argumens for the interactive mode"
prompt="$(printf "" | ./graphene  -d . -i)"
assert "$prompt" "#SPP001
Graphene database. Type cmdlist to see list of commands
#OK"

# create
assert "$(printf 'x' | ./graphene -d . -i)"\
       "$(printf "$prompt\n#Error: Unknown command: x")"
assert "$(printf 'create' | ./graphene -d . -i)"\
       "$(printf "$prompt\n#Error: database name expected")"

assert "$(printf 'create test_1
                  info test_1' | ./graphene -i -d .)"\
       "$(printf "$prompt\n#OK\nDOUBLE\n#OK")"

assert "$(printf 'create test_2
                  put test_1 10 0
                  put test_1 20 10
                  put test_2 30 20
                  put test_2 40 30
                  get test_1 15
                  get test_2 38' | ./graphene -i -d .)"\
        "$(printf "$prompt\n#OK\n#OK\n#OK\n#OK\n#OK\n15.000000000 5\n#OK\n38.000000000 28\n#OK")"
assert "$(./graphene -d . delete test_1)" ""
assert "$(./graphene -d . delete test_2)" ""

###########################################################################
# duplicated keys (replace by default)

assert "$(./graphene -d . create test_1 UINT32)" ""
assert "$(./graphene -d . put test_1 1  1)" ""
assert "$(./graphene -d . put test_1 1  2)" ""
assert "$(./graphene -d . get_range test_1)" "1.000000000 2"

assert "$(./graphene -d . -D replace put test_1 1 3)" ""
assert "$(./graphene -d . get_range test_1)" "1.000000000 3"

assert "$(./graphene -d . -D skip put test_1 1 4)" ""
assert "$(./graphene -d . get_range test_1)" "1.000000000 3"
assert "$(./graphene -d . -D sshift put test_1 1 5)" ""
assert "$(./graphene -d . -D sshift put test_1 1 6)" ""
assert "$(./graphene -d . get_range test_1)" "\
1.000000000 3
2.000000000 5
3.000000000 6"

assert "$(./graphene -d . -D nsshift put test_1 1 7)" ""
assert "$(./graphene -d . -D nsshift put test_1 1 8)" ""
assert "$(./graphene -d . get_range test_1)" "\
1.000000000 3
1.000000001 7
1.000000002 8
2.000000000 5
3.000000000 6"

assert "$(./graphene -d . -D aaa put test_1 1 8)" "#Error: Unknown dpolicy setting: aaa"

assert "$(./graphene -d . -D error put test_1 1 8)" "#Error: test_1.db: Timestamp exists"
assert "$(./graphene -d . delete test_1)" ""

###########################################################################
# 32- and 64-bit timestamps

assert "$(./graphene -d . create test_1 UINT32)" ""
assert "$(./graphene -d . put test_1 1    1)" ""
assert "$(./graphene -d . put test_1 1.5  2)" ""
assert "$(./graphene -d . put test_1 2    3)" ""
assert "$(./graphene -d . get_range test_1)" "\
1.000000000 1
1.500000000 2
2.000000000 3"

db_dump test_1.db > test_1.tmp
./graphene -d . load test_2 test_1.tmp
db_dump test_2.db > test_2.tmp
assert "$(diff test_2.tmp test_1.tmp)" ""

assert "$(./graphene -d . delete test_1)" ""
assert "$(./graphene -d . delete test_2)" ""


###########################################################################
# remove all test databases
for i in *.db; do
  [ -f $i ] || continue
  ./graphene -d . delete ${i%.db}
done
rm -f -- __db.* log.*
