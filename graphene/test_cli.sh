#!/bin/bash

# test command line interface

. ../modules/test_lib.sh

###########################################################################
# help message
help_msg="$(./graphene -h)"

# no commands - help message
assert_cmd "./graphene" "Error: command is expected" 1
assert_cmd "./graphene -d ." "Error: command is expected" 1

# invalid option (error printed by getopt)
assert_cmd "./graphene -X . 2>&1" "./graphene: invalid option -- 'X'" 1

# unknown command
assert_cmd "./graphene -d . a" "Error: Unknown command: a" 1

assert_cmd "./graphene -d . *idn?" "Graphene database 2.10" 0

assert_cmd "./graphene -d . get_time a" "Error: too many parameters" 1

assert_cmd "./graphene -d . get_time | tr [0-9] x" "xxxxxxxxxx.xxxxxx" 0

###########################################################################
# Some error messages depend on libdb version. Extract major part:
maj="$(./graphene libdb_version | sed -r 's/.*([0-9]+)\.([0-9]+)\.([0-9]+).*/\1/')"
if [ $maj -ge 5 ]; then
  DB_NOTFOUND="BDB0073 DB_NOTFOUND: No matching key/data pair found"
else
  DB_NOTFOUND="DB_NOTFOUND: No matching key/data pair found"
fi
# (At the moment no such messages are used in the test)

###########################################################################
# database operations

# remove all test databases
for i in *.db; do
  [ -f $i ] || continue
  ./graphene -d . delete ${i%.db}
done

rm -f -- __db.* log.*

# create
assert_cmd "./graphene -d . create" "Error: database name expected" 1
assert_cmd "./graphene -d . create a dfmt" "Error: Unknown data type: dfmt" 1

assert_cmd "./graphene -d . create test_1" ""
assert_cmd "./graphene -d . create test_2 UINT16" ""
assert_cmd "./graphene -d . create test_3 uint32 \"Uint 32 database\"" ""
assert_cmd "./graphene -d . create test_4 uint32 Uint 32 database" ""
assert_cmd "./graphene -d . create test_1" "Error: test_1.db: File exists" 1

# info
assert_cmd "./graphene -d . info" "Error: database name expected" 1
assert_cmd "./graphene -d . info a b" "Error: too many parameters" 1
assert_cmd "./graphene -d . info test_x" "Error: test_x.db: No such file or directory" 1

assert_cmd "./graphene -d . info test_1" "DOUBLE"
assert_cmd "./graphene -d . info test_2" "UINT16"
assert_cmd "./graphene -d . info test_3" "UINT32	Uint 32 database"
assert_cmd "./graphene -d . info test_4" "UINT32	Uint 32 database"

# list
assert_cmd "./graphene -d . list a" "Error: too many parameters" 1
assert_cmd "./graphene -d . list | sort" "$(printf "test_1\ntest_2\ntest_3\ntest_4")"

# list_dbs
assert_cmd "./graphene -d . list_dbs a" "Error: too many parameters" 1
assert_cmd "./graphene -E lock -d . list_dbs" "Error: list_dbs can not by run in this environment type: lock" 1
assert_cmd "./graphene -R -d . list_dbs" "Error: list_dbs can not by run in this environment type: lock" 1

# list_logs
assert_cmd "./graphene -d . list_logs a" "Error: too many parameters" 1
assert_cmd "./graphene -E lock -d . list_logs" "Error: list_logs can not by run in this environment type: lock" 1
assert_cmd "./graphene -R -d . list_logs" "Error: list_logs can not by run in this environment type: lock" 1

# delete
assert_cmd "./graphene -d . delete" "Error: database name expected" 1
assert_cmd "./graphene -d . delete a b" "Error: too many parameters" 1
assert_cmd "./graphene -d . delete test_x" "Error: test_x.db: No such file or directory" 1
assert_cmd "./graphene -d . delete test_2" ""
assert_cmd "./graphene -d . delete test_2" "Error: test_2.db: No such file or directory" 1

# rename
assert_cmd "./graphene -d . rename" "Error: database old and new names expected" 1
assert_cmd "./graphene -d . rename a" "Error: database old and new names expected" 1
assert_cmd "./graphene -d . rename a b c" "Error: too many parameters" 1
assert_cmd "./graphene -d . rename test_x test_y" "Error: renaming test_x.db -> test_y.db: No such file or directory" 1
assert_cmd "./graphene -d . rename test_3 test_1" "Error: renaming test_3.db -> test_1.db: Destination exists" 1
assert_cmd "./graphene -d . rename test_3 test_2" ""
assert_cmd "./graphene -d . info test_3" "Error: test_3.db: No such file or directory" 1
assert_cmd "./graphene -d . info test_2" "UINT32	Uint 32 database"

# set_descr
assert_cmd "./graphene -d . set_descr" "Error: database name and new description text expected" 1
assert_cmd "./graphene -d . set_descr a" "Error: database name and new description text expected" 1
assert_cmd "./graphene -d . set_descr test_x a" "Error: test_x.db: No such file or directory" 1
assert_cmd "./graphene -d . set_descr test_1 \"Test DB number 1\"" ""
assert_cmd "./graphene -d . info test_1" "DOUBLE	Test DB number 1"
assert_cmd "./graphene -d . set_descr test_1 Test DB number 1" ""
assert_cmd "./graphene -d . info test_1" "DOUBLE	Test DB number 1"

assert_cmd "./graphene -d . delete test_1" ""
assert_cmd "./graphene -d . delete test_2" ""
assert_cmd "./graphene -d . delete test_4" ""

###########################################################################
# list_dbs/list_logs need transactions
rm -f -- __db.* log.*

assert_cmd "./graphene -E txn -d . create test_1" ""
assert_cmd "./graphene -E txn -d . create test_2 UINT16" ""
assert_cmd "./graphene -E txn -d . create test_3 uint32 \"Uint 32 database\"" ""
assert_cmd "./graphene -E txn -d . create test_4 uint32 Uint 32 database" ""

assert_cmd "./graphene -E txn -d . list_dbs" "$(printf "test_1.db\ntest_2.db\ntest_3.db\ntest_4.db")"
assert_cmd "./graphene -E txn -d . list_logs" "log.0000000001"

assert_cmd "./graphene -E txn -d . delete test_1" ""
assert_cmd "./graphene -E txn -d . delete test_2" ""
assert_cmd "./graphene -E txn -d . delete test_3" ""
assert_cmd "./graphene -E txn -d . delete test_4" ""

rm -f -- __db.* log.*
###########################################################################
# DOUBLE database

assert_cmd "./graphene -d . create test_1" ""
assert_cmd "./graphene -d . put test_1 1234567890.000 0.1" ""
assert_cmd "./graphene -d . put test_1 2234567890.123 0.2" ""
# get_next
assert_cmd "./graphene -d . get_next test_1"                "1234567890.000000000 0.1" # first
assert_cmd "./graphene -d . get_next test_1 0"              "1234567890.000000000 0.1"
assert_cmd "./graphene -d . get_next test_1 1000000000.000" "1234567890.000000000 0.1"
assert_cmd "./graphene -d . get_next test_1 1234567890.000" "1234567890.000000000 0.1" # ==
assert_cmd "./graphene -d . get_next test_1 1234567895.000" "2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_next test_1 now"            "2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_next test_1 NoW"            "2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_next test_1 NoW_s"          "2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_next test_1 inf"            ""
assert_cmd "./graphene -d . get_next test_1 2000000000.124" "2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_next test_1 3000000000" ""
assert_cmd "./graphene -r -d . get_next test_1 2000000000.124" "234567889.999000013 0.2" # relative (note the rounding error)
# get_prev
assert_cmd "./graphene -d . get_prev test_1"                "2234567890.123000000 0.2" # last
assert_cmd "./graphene -d . get_prev test_1 0"              ""
assert_cmd "./graphene -d . get_prev test_1 1234567890"     "1234567890.000000000 0.1" # ==
assert_cmd "./graphene -d . get_prev test_1 2234567890.123" "2234567890.123000000 0.2" # ==
assert_cmd "./graphene -d . get_prev test_1 1234567895"     "1234567890.000000000 0.1"
assert_cmd "./graphene -d . get_prev test_1 now"            "1234567890.000000000 0.1"
assert_cmd "./graphene -d . get_prev test_1 inf"            "2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_prev test_1 3234567895"     "2234567890.123000000 0.2"
assert_cmd "./graphene -r -d . get_prev test_1 2234567895"     "-4.877000000 0.2" # relative

# get_range
assert_cmd "./graphene -d . get_range test_1 0 1234567880"   ""
assert_cmd "./graphene -d . get_range test_1 0 1234567880 2" ""
assert_cmd "./graphene -d . get_range test_1 0 1234567890"   "1234567890.000000000 0.1"
assert_cmd "./graphene -d . get_range test_1 0 1234567890 3" "1234567890.000000000 0.1"

assert_cmd "./graphene -d . get_range test_1 2234567891 3234567890" ""
assert_cmd "./graphene -d . get_range test_1 2234567891 3234567890 2" ""
assert_cmd "./graphene -d . get_range test_1 2234567890.123 3234567890"   "2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_range test_1 2234567890.123 3234567890 2" "2234567890.123000000 0.2"

assert_cmd "./graphene -d . get_range test_1 0 2000000000"   "1234567890.000000000 0.1"
assert_cmd "./graphene -d . get_range test_1 0 2000000000 3" "1234567890.000000000 0.1"
assert_cmd "./graphene -d . get_range test_1 2000000000 3000000000"   "2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_range test_1 2000000000 3000000000 3" "2234567890.123000000 0.2"

assert_cmd "./graphene -d . get_range test_1" "1234567890.000000000 0.1
2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_range test_1 0 3384967290 2" "1234567890.000000000 0.1
2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_range test_1 1234567890 2234567890.123 2" "1234567890.000000000 0.1
2234567890.123000000 0.2"

assert_cmd "./graphene -r -d . get_range test_1 1234567890 2234567890.123 2" "0.000000000 0.1
1000000000.123000026 0.2" # relative (note rounding error)

assert_cmd "./graphene -d . get_range test_1 1234567890 2234567890.123 1200000000" "1234567890.000000000 0.1"

# -inf +inf nan values:
assert_cmd "./graphene -d . put test_1 1 -inf -Inf nan NaN nAn +inf +Inf" ""
assert_cmd "./graphene -d . get test_1 1" "1.000000000 -inf -inf nan nan nan inf inf"

# interpolation
assert_cmd "./graphene -d . put test_1 3 10 10 10 10 10 10 10" ""
assert_cmd "./graphene -d . get test_1 2" "2.000000000 -inf -inf nan nan nan inf inf"


###########################################################################
# get_count
assert_cmd "./graphene -d . get_count test_1" "1.000000000 -inf -inf nan nan nan inf inf
3.000000000 10 10 10 10 10 10 10
1234567890.000000000 0.1
2234567890.123000000 0.2"

assert_cmd "./graphene -d . get_count test_1 2 10" "3.000000000 10 10 10 10 10 10 10
1234567890.000000000 0.1
2234567890.123000000 0.2"

assert_cmd "./graphene -d . get_count test_1 2 2" "3.000000000 10 10 10 10 10 10 10
1234567890.000000000 0.1"

assert_cmd "./graphene -d . get_count test_1 1234567890 2" "1234567890.000000000 0.1
2234567890.123000000 0.2"

assert_cmd "./graphene -d . get_count test_1 1234567889.1 1" "1234567890.000000000 0.1"
assert_cmd "./graphene -d . get_count test_1 1234567890.1" "2234567890.123000000 0.2"
assert_cmd "./graphene -d . get_count test_1 2234567891.1" ""
assert_cmd "./graphene -d . get_count test_1 1 0" ""


assert_cmd "./graphene -d . delete test_1" ""

###########################################################################
# interpolation and columns (DOUBLE database)
assert_cmd "./graphene -d . create test_2 DOUBLE \"double database\"" ""

assert_cmd "./graphene -d . put test_2 1000 1 10 30" ""
assert_cmd "./graphene -d . put test_2 2000 2 20" ""

# get
assert_cmd "./graphene -d . get test_2 800"    ""
assert_cmd "./graphene -d . get test_2 2200"   "2000.000000000 2 20"
assert_cmd "./graphene -d . get test_2 1000"   "1000.000000000 1 10 30"
assert_cmd "./graphene -d . get test_2 2000"   "2000.000000000 2 20"
assert_cmd "./graphene -d . get test_2 1200"   "1200.000000000 1.2 12"
assert_cmd "./graphene -d . get test_2 1800"   "1800.000000000 1.8 18"
assert_cmd "./graphene -d . get test_2:1 1200" "1200.000000000 12"
assert_cmd "./graphene -r -d . get test_2 2200"   "-200.000000000 2 20" # relative
assert_cmd "./graphene -r -d . get test_2 1800"   "0.000000000 1.8 18" # relative

# columns
assert_cmd "./graphene -d . get_next test_2:0" "1000.000000000 1"
assert_cmd "./graphene -d . get_next test_2:1" "1000.000000000 10"
assert_cmd "./graphene -d . get_next test_2:3" "1000.000000000 NaN"
assert_cmd "./graphene -d . get_prev test_2:0" "2000.000000000 2"
assert_cmd "./graphene -d . get_prev test_2:1" "2000.000000000 20"
assert_cmd "./graphene -d . get_prev test_2:3" "2000.000000000 NaN"

assert_cmd "./graphene -d . get_range test_2:0" "1000.000000000 1
2000.000000000 2"

assert_cmd "./graphene -d . get_range test_2:3" "1000.000000000 NaN
2000.000000000 NaN"
assert_cmd "./graphene -d . delete test_2" ""

###########################################################################
# precision (DOUBLE database)
assert_cmd "./graphene -d . create test_2 DOUBLE \"double database\"" ""
assert_cmd "./graphene -d . create test_3 FLOAT  \"float database\"" ""

assert_cmd "./graphene -d . put test_2 1000 1.2345678901234567890123456789" ""
assert_cmd "./graphene -d . put test_2 2000 1.2345678901234567890123456789e88" ""
assert_cmd "./graphene -d . put test_3 1000 1.2345678901234567890123456789" ""
assert_cmd "./graphene -d . put test_3 2000 1.2345678901234567890123456789e38" ""

assert_cmd "./graphene -d . put test_3 2000 1e88" "Error: Bad FLOAT value: 1e88" 1

assert_cmd "./graphene -d . get_range test_2" "1000.000000000 1.234567890123457
2000.000000000 1.234567890123457e+88"
assert_cmd "./graphene -d . get_range test_3" "1000.000000000 1.2345679
2000.000000000 1.2345679e+38"

assert_cmd "./graphene -d . delete test_2" ""
assert_cmd "./graphene -d . delete test_3" ""

###########################################################################
# deleting records

assert_cmd "./graphene -d . create test_3 UINT32 \"Uint 32 database\"" ""
assert_cmd "./graphene -d . put test_3 10 1" ""
assert_cmd "./graphene -d . put test_3 11 2" ""
assert_cmd "./graphene -d . put test_3 12 3" ""
assert_cmd "./graphene -d . put test_3 13 4" ""
assert_cmd "./graphene -d . put test_3 14 5" ""
assert_cmd "./graphene -d . put test_3 15 6" ""
assert_cmd "./graphene -d . get_range test_3 | wc" "      6      12      90"
assert_cmd "./graphene -d . get_next test_3 12" "12.000000000 3"
assert_cmd "./graphene -d . del test_3 12" ""
assert_cmd "./graphene -d . get_next test_3 12" "13.000000000 4"
assert_cmd "./graphene -d . del_range test_3 11 13" ""
assert_cmd "./graphene -d . get_range test_3" "10.000000000 1
14.000000000 5
15.000000000 6"

assert_cmd "./graphene -d . delete test_3" ""

###########################################################################
# TEXT database
assert_cmd "./graphene -d . create test_4 TEXT" ""

assert_cmd "./graphene -d . put test_4 1000 text1" ""
./graphene -d . put test_4 2000 'text2
2'
# get_next
assert_cmd "./graphene -d . get_next test_4"      "1000.000000000 text1" # first
assert_cmd "./graphene -d . get_next test_4 0"    "1000.000000000 text1"
assert_cmd "./graphene -d . get_next test_4 1000" "1000.000000000 text1" # ==
assert_cmd "./graphene -d . get_next test_4 1500" "2000.000000000 text2
2"
assert_cmd "./graphene -d . get_next test_4 2000" "2000.000000000 text2
2"
assert_cmd "./graphene -d . get_next test_4 2001" ""

# get_prev
assert_cmd "./graphene -d . get_prev test_4"      "2000.000000000 text2
2" # last
assert_cmd "./graphene -d . get_prev test_4 0"    ""
assert_cmd "./graphene -d . get_prev test_4 1000" "1000.000000000 text1" # ==
assert_cmd "./graphene -d . get_prev test_4 2000" "2000.000000000 text2
2" # ==
assert_cmd "./graphene -d . get_prev test_4 1500" "1000.000000000 text1"
assert_cmd "./graphene -d . get_prev test_4 2001" "2000.000000000 text2
2"

# get - same
assert_cmd "./graphene -d . get test_4"      "2000.000000000 text2
2" # last
assert_cmd "./graphene -d . get test_4 0"    ""
assert_cmd "./graphene -d . get test_4 1000" "1000.000000000 text1" # ==
assert_cmd "./graphene -d . get test_4 2000" "2000.000000000 text2
2" # ==
assert_cmd "./graphene -d . get test_4 1500" "1000.000000000 text1"
assert_cmd "./graphene -d . get test_4 2001" "2000.000000000 text2
2"

# get_range
assert_cmd "./graphene -d . get_range test_4 0 999" ""
assert_cmd "./graphene -d . get_range test_4 0 999 2" ""
assert_cmd "./graphene -d . get_range test_4 0 1000"   "1000.000000000 text1"
assert_cmd "./graphene -d . get_range test_4 0 1000 3" "1000.000000000 text1"

assert_cmd "./graphene -d . get_range test_4 2001 3000" ""
assert_cmd "./graphene -d . get_range test_4 2001 3000 2" ""
assert_cmd "./graphene -d . get_range test_4 2000 3000" "2000.000000000 text2"
assert_cmd "./graphene -d . get_range test_4 2000 3000 2" "2000.000000000 text2"

assert_cmd "./graphene -d . get_range test_4" "1000.000000000 text1
2000.000000000 text2"
assert_cmd "./graphene -d . get_range test_4 1000 2000" "1000.000000000 text1
2000.000000000 text2"
assert_cmd "./graphene -d . get_range test_4 1000 2000 1000" "1000.000000000 text1
2000.000000000 text2"
assert_cmd "./graphene -d . get_range test_4 1000 2000 1001" "1000.000000000 text1"

# columns are not important
assert_cmd "./graphene -d . get_next test_4:5" "1000.000000000 text1"
assert_cmd "./graphene -d . delete test_4" ""


###########################################################################
# interactive mode

#prompt
assert_cmd "./graphene -d . -i 1" "Error: too many arguments for the interactive mode" 1
assert_cmd "printf \"\" | ./graphene  -d . -i" "#SPP001
Graphene database. Type cmdlist to see list of commands
#OK"
prompt="$(printf "" | ./graphene  -d . -i)"

# create
assert_cmd "printf 'x' | ./graphene -d . -i"\
       "$(printf "$prompt\n#Error: Unknown command: x")" 0
assert_cmd "printf 'create' | ./graphene -d . -i"\
       "$(printf "$prompt\n#Error: database name expected")" 0

assert_cmd "printf 'create test_1\n
                  info test_1' | ./graphene -i -d ."\
       "$(printf "$prompt\n#OK\nDOUBLE\n#OK")"

assert_cmd "printf 'create test_2\n
                  put test_1 10 0\n
                  put test_1 20 10\n
                  put test_2 30 20\n
                  put test_2 40 30\n
                  get test_1 15\n
                  get test_2 38\n' | ./graphene -i -d ."\
        "$(printf "$prompt\n#OK\n#OK\n#OK\n#OK\n#OK\n15.000000000 5\n#OK\n38.000000000 28\n#OK")"
assert_cmd "./graphene -d . delete test_1" ""
assert_cmd "./graphene -d . delete test_2" ""

# #symbols in interactive text mode
assert_cmd "./graphene -d . create text_3 text" ""
assert_cmd "./graphene -d . put text_3 10 AAA" ""
assert_cmd "./graphene -d . put text_3 20 \#BBB" ""
./graphene -d . put text_3 30 '#BBB
#CCC
#DDD #EEE'

assert_cmd "./graphene -d . get text_3" "30.000000000 #BBB
#CCC
#DDD #EEE"

assert_cmd "printf 'get text_3' | ./graphene -i -d ."\
  "$(printf "$prompt\n30.000000000 #BBB\n##CCC\n##DDD #EEE\n#OK")"

# quoting in interactive mode:
assert_cmd "cat test_data/commands1 | ./graphene -i -d ."\
  "$(cat test_data/answer1)"

###########################################################################
# duplicated keys (replace by default)

assert_cmd "./graphene -d . create test_1 UINT32" ""
assert_cmd "./graphene -d . put test_1 1  1" ""
assert_cmd "./graphene -d . put test_1 1  2" ""
assert_cmd "./graphene -d . get_range test_1" "1.000000000 2"

assert_cmd "./graphene -d . -D replace put test_1 1 3" ""
assert_cmd "./graphene -d . get_range test_1" "1.000000000 3"

assert_cmd "./graphene -d . -D skip put test_1 1 4" ""
assert_cmd "./graphene -d . get_range test_1" "1.000000000 3"
assert_cmd "./graphene -d . -D sshift put test_1 1 5" ""
assert_cmd "./graphene -d . -D sshift put test_1 1 6" ""
assert_cmd "./graphene -d . get_range test_1" "\
1.000000000 3
2.000000000 5
3.000000000 6"

assert_cmd "./graphene -d . -D nsshift put test_1 1 7" ""
assert_cmd "./graphene -d . -D nsshift put test_1 1 8" ""
assert_cmd "./graphene -d . get_range test_1" "\
1.000000000 3
1.000000001 7
1.000000002 8
2.000000000 5
3.000000000 6"

assert_cmd "./graphene -d . -D aaa put test_1 1 8" "Error: Unknown dpolicy setting: aaa" 1

assert_cmd "./graphene -d . -D error put test_1 1 8" "Error: test_1.db: Timestamp exists" 1
assert_cmd "./graphene -d . delete test_1" ""

###########################################################################
# 32- and 64-bit timestamps

assert_cmd "./graphene -d . create test_1 UINT32" ""
assert_cmd "./graphene -d . put test_1 1    1" ""
assert_cmd "./graphene -d . put test_1 1.5  2" ""
assert_cmd "./graphene -d . put test_1 2    3" ""
assert_cmd "./graphene -d . get_range test_1" "\
1.000000000 1
1.500000000 2
2.000000000 3"

./graphene -d . dump test_1 test_1.tmp
./graphene -d . load test_2 test_1.tmp
./graphene -d . dump test_2 test_2.tmp
db_dump test_2.db > test_3.tmp
assert_cmd "diff test_2.tmp test_1.tmp" ""
assert_cmd "diff test_2.tmp test_3.tmp" ""

assert_cmd "./graphene -d . delete test_1" ""
assert_cmd "./graphene -d . delete test_2" ""

rm -f test_*.tmp


###########################################################################
# readonly mode

assert_cmd "./graphene -d . create test_1 UINT32" ""
assert_cmd "./graphene -d . put test_1 1    1" ""
assert_cmd "./graphene -d . put test_1 1.5  2" ""
assert_cmd "./graphene -d . put test_1 2    3" ""
assert_cmd "./graphene -d . -R put test_1 3    4" "Error: can't write to database in readonly mode" 1
assert_cmd "./graphene -d . -R  get_range test_1" "\
1.000000000 1
1.500000000 2
2.000000000 3"

assert_cmd "./graphene -d . -R delete test_1" "Error: can't remove database in readonly mode" 1
assert_cmd "./graphene -d . -R rename test_1 test_2" "Error: can't rename database in readonly mode" 1
assert_cmd "./graphene -d . delete test_1" ""

###########################################################################
# backup system

# in the new database backup timer is set to 0
assert_cmd "./graphene -d . create test_1 UINT32" ""
assert_cmd "./graphene -d . backup_print test_1" "0.000000000"
assert_cmd "./graphene -d . backup_start test_1" "0.000000000"

# on put operations timer shifts to lower times
assert_cmd "./graphene -d . put test_1 1234 1" ""
assert_cmd "./graphene -d . backup_start test_1" "0.000000000"
assert_cmd "./graphene -d . backup_end test_1" "" # commit tmp timer

assert_cmd "./graphene -d . backup_start test_1" "4294967295.999999999"
assert_cmd "./graphene -d . put test_1 1234 1" ""
assert_cmd "./graphene -d . backup_end test_1" "" # commit tmp timer
assert_cmd "./graphene -d . put test_1 1235 1" ""

assert_cmd "./graphene -d . backup_start test_1" "1234.000000000"
assert_cmd "./graphene -d . backup_end test_1" "" # commit tmp timer

assert_cmd "./graphene -d . put test_1 1233 1" ""
assert_cmd "./graphene -d . backup_start test_1" "1233.000000000"
assert_cmd "./graphene -d . backup_start test_1" "1233.000000000"
assert_cmd "./graphene -d . backup_end test_1" "" # commit tmp timer

# delete operation
assert_cmd "./graphene -d . del test_1 1235" ""
assert_cmd "./graphene -d . backup_start test_1" "1235.000000000"

# if delete fails, lastmod does not change
assert_cmd "./graphene -d . del test_1 10" "Error: test_1.db: No such record: 10" 1
assert_cmd "./graphene -d . backup_start test_1" "1235.000000000"
assert_cmd "./graphene -d . backup_end test_1" ""

# real change will be at 1235
assert_cmd "./graphene -D sshift -d . put test_1 1233 10" ""
assert_cmd "./graphene -d . backup_start test_1" "1235.000000000"

assert_cmd "./graphene -d . del_range test_1 0 10" ""
assert_cmd "./graphene -d . backup_start test_1" "1235.000000000"

assert_cmd "./graphene -d . del_range test_1 100 1234" ""
assert_cmd "./graphene -d . backup_start test_1" "1233.000000000"

# reset, end with timestamp
assert_cmd "./graphene -d . backup_reset test_1" ""
assert_cmd "./graphene -d . backup_print test_1" "0.000000000"
assert_cmd "./graphene -d . backup_start test_1" "0.000000000"
assert_cmd "./graphene -d . put test_1 1234 1" ""
assert_cmd "./graphene -d . backup_print test_1" "0.000000000"
assert_cmd "./graphene -d . backup_end test_1 1000" ""
assert_cmd "./graphene -d . backup_print test_1" "1000.000000000"
assert_cmd "./graphene -d . backup_start test_1" "1000.000000000"
assert_cmd "./graphene -d . put test_1 1234 1" ""
assert_cmd "./graphene -d . backup_end test_1 2000" ""
assert_cmd "./graphene -d . backup_print test_1" "1234.000000000"

assert_cmd "./graphene -d . delete test_1" ""

###########################################################################
## input filter
assert_cmd "./graphene -d . create test_1 DOUBLE" ""


# put every third record; round time to int; add 1 to first data element,
# replace others with counter value
code='
  if {$storage ne ""} {incr storage} else {set storage 1}
  set time [expr int($time)]
  set data [list [expr [lindex $data 0] + 1] $storage]
  return [expr $storage%3==1]'

./graphene -d . set_filter test_1 0 "$code"
assert_cmd "./graphene -d . print_filter test_1 0" "$code"
assert_cmd "./graphene -d . put_flt test_1 123.456 10 20 30" ""
assert_cmd "./graphene -d . put_flt test_1 200.1  15" ""
assert_cmd "./graphene -d . put_flt test_1 201.1  16" ""
assert_cmd "./graphene -d . put_flt test_1 202.1  17" ""
assert_cmd "./graphene -d . put_flt test_1 203.1  18" ""

assert_cmd "./graphene -d . get_range test_1" \
"123.000000000 11 1
202.000000000 18 4"

assert_cmd "./graphene -d . print_f0data test_1" "5"

assert_cmd "./graphene -d . clear_f0data test_1" ""
assert_cmd "./graphene -d . print_f0data test_1" ""
assert_cmd "./graphene -d . put_flt test_1 200 1" ""
assert_cmd "./graphene -d . print_f0data test_1" "1"

# setting of the filter resets also storage information
./graphene -d . set_filter test_1 0 "$code"
assert_cmd "./graphene -d . put_flt test_1 523.456 10 20 30" ""
assert_cmd "./graphene -d . get test_1" "523.000000000 11 1"

# code with error: no stdout in the safe TCL interpreter
code='puts "text"; return 1'
./graphene -d . set_filter test_1 0 "$code"
assert_cmd "./graphene -d . print_filter test_1 0" "$code"

assert_cmd "./graphene -d . put_flt test_1 123.456 10 20 30"\
  "Error: filter: can't run TCL script: can not find channel named \"stdout\"     while executing \"puts \"text\"\"" 1

###########################################################################
## output filter
code='
  set time [expr int($time)+2]
  set data [list [expr [lindex $data 0]*2]]
  return [expr $time < 500]'

assert_cmd "./graphene -d . set_filter test_1 -1 \"$code\"" "Error: filter number out of range: -1" 1
assert_cmd "./graphene -d . set_filter test_1 1000 \"$code\"" "Error: filter number out of range: 1000" 1
./graphene -d . set_filter test_1 5 "$code"
assert_cmd "./graphene -d . print_filter test_1 5" "$code"

# f1 is not set, get original data
assert_cmd "./graphene -d . get_range test_1:f1" \
"123.000000000 11 1
200.000000000 2 1
202.000000000 18 4
523.000000000 11 1"

assert_cmd "./graphene -d . get_range test_1:f5" \
"125 22
202 4
204 36"

assert_cmd "./graphene -d . delete test_1" ""

###########################################################################
# remove all test databases
for i in *.db; do
  [ -f $i ] || continue
  ./graphene -d . delete ${i%.db}
done
rm -f -- __db.* log.*
