#!/bin/sh -u

# test command line interface program ./stsdb

function assert(){
  if [ "$1" != "$2" ]; then
    printf "ERROR:\n"
    printf "  res:\n%s\n" "$1";
    printf "  exp:\n%s\n" "$2";
    exit 1
  fi
}

# help message
help_msg="$(./stsdb -h)"

# no commands - help message
assert "$(./stsdb)" "$help_msg"
assert "$(./stsdb -d .)" "$help_msg"

# invalid option (error printed by getopt)
assert "$(./stsdb -X . 2>&1)" "./stsdb: invalid option -- 'X'"

# unknown command
assert "$(./stsdb -d . a)" "Error: Unknown command"


# remove all test databases
rm -f test*.db

# create
assert "$(./stsdb -d . create)" "Error: database name expected"
assert "$(./stsdb -d . create a  tfmt dfmt descr e)" "Error: too many parameters"
assert "$(./stsdb -d . create a tfmt)" "Error: Unknown time format: tfmt"
assert "$(./stsdb -d . create a TIME_S dfmt)" "Error: Unknown data format: dfmt"

assert "$(./stsdb -d . create test_1)" ""
assert "$(./stsdb -d . create test_2 MS)" ""
assert "$(./stsdb -d . create test_3 MS UINT16)" ""
assert "$(./stsdb -d . create test_4 S  UINT32 "Uint 32 database")" ""
assert "$(./stsdb -d . create test_1)" "Error: test_1.db: File exists"

# info
assert "$(./stsdb -d . info)" "Error: database name expected"
assert "$(./stsdb -d . info a b)" "Error: too many parameters"
assert "$(./stsdb -d . info test_x)" "Error: test_x.db: No such file or directory"

assert "$(./stsdb -d . info test_1)" "S	DOUBLE"
assert "$(./stsdb -d . info test_2)" "MS	DOUBLE"
assert "$(./stsdb -d . info test_3)" "MS	UINT16"
assert "$(./stsdb -d . info test_4)" "S	UINT32	Uint 32 database"

# list
assert "$(./stsdb -d . list a)" "Error: too many parameters"
assert "$(./stsdb -d . list | sort)" "$(printf "test_1\ntest_2\ntest_3\ntest_4")"

# delete
assert "$(./stsdb -d . delete)" "Error: database name expected"
assert "$(./stsdb -d . delete a b)" "Error: too many parameters"
assert "$(./stsdb -d . delete test_x)" "Error: test_x.db: No such file or directory"
assert "$(./stsdb -d . delete test_4)" ""
assert "$(./stsdb -d . delete test_4)" "Error: test_4.db: No such file or directory"

# rename
assert "$(./stsdb -d . rename)" "Error: database old and new names expected"
assert "$(./stsdb -d . rename a)" "Error: database old and new names expected"
assert "$(./stsdb -d . rename a b c)" "Error: too many parameters"
assert "$(./stsdb -d . rename test_x test_y)" "Error: can't rename database: No such file or directory"
assert "$(./stsdb -d . rename test_3 test_2)" "Error: can't rename database: destination exists"
assert "$(./stsdb -d . rename test_3 test_4)" ""
assert "$(./stsdb -d . info test_3)" "Error: test_3.db: No such file or directory"
assert "$(./stsdb -d . info test_4)" "MS	UINT16"

# set_descr
assert "$(./stsdb -d . set_descr)" "Error: database name and new description text expected"
assert "$(./stsdb -d . set_descr a)" "Error: database name and new description text expected"
assert "$(./stsdb -d . set_descr a b c)" "Error: too many parameters"
assert "$(./stsdb -d . set_descr test_x a)" "Error: test_x.db: No such file or directory"
assert "$(./stsdb -d . set_descr test_1 "Test DB number 1")" ""
assert "$(./stsdb -d . info test_1)" "S	DOUBLE	Test DB number 1"

#1 S-DOUBLE DB test_1
assert "$(./stsdb -d . put test_1 1234567890    0.1)" ""
assert "$(./stsdb -d . put test_1 2234567890123 0.2)" ""
# get_next
assert "$(./stsdb -d . get_next test_1)"               "1234567890 0.1" # first
assert "$(./stsdb -d . get_next test_1 0)"             "1234567890 0.1" # first
assert "$(./stsdb -d . get_next test_1 1000000000)"    "1234567890 0.1"
assert "$(./stsdb -d . get_next test_1 1000000000000)" "1234567890 0.1" # ms
assert "$(./stsdb -d . get_next test_1 1234567890)"    "1234567890 0.1" # ==
assert "$(./stsdb -d . get_next test_1 1234567895)"    "2234567890 0.2"
assert "$(./stsdb -d . get_next test_1 1234567895000)" "2234567890 0.2" # ms
assert "$(./stsdb -d . get_next test_1 now)"           "2234567890 0.2"
assert "$(./stsdb -d . get_next test_1 NoW)"           "2234567890 0.2"
assert "$(./stsdb -d . get_next test_1 2000000000124)" "2234567890 0.2"
assert "$(./stsdb -d . get_next test_1 3000000000)"    ""
assert "$(./stsdb -d . get_next test_1 3000000000000)" ""
assert "$(./stsdb -d . get_next test_1 3384967290)"    ""
assert "$(./stsdb -d . get_next test_1 1234567890000000)"    ""
# get_prev
assert "$(./stsdb -d . get_prev test_1)"               "2234567890 0.2" # last
assert "$(./stsdb -d . get_prev test_1 1)"             ""
assert "$(./stsdb -d . get_prev test_1 1234567890)"    "1234567890 0.1" # ==
assert "$(./stsdb -d . get_prev test_1 2234567890123)" "2234567890 0.2" # ==
assert "$(./stsdb -d . get_prev test_1 1234567895)"    "1234567890 0.1"
assert "$(./stsdb -d . get_prev test_1 1234567895000)" "1234567890 0.1"
#assert "$(./stsdb -d . get_prev test_1 now)"           "1234567890 0.1"
#assert "$(./stsdb -d . get_prev test_1 3234567895)"    "2234567890 0.2"
#assert "$(./stsdb -d . get_prev test_1 3234567895000)" "2234567890 0.2"

# get_range
assert "$(./stsdb -d . get_range test_1 0 1234567880)" ""
assert "$(./stsdb -d . get_range test_1 0 1234567880 2)" ""
assert "$(./stsdb -d . get_range test_1 0 1234567890)" "1234567890 0.1"
assert "$(./stsdb -d . get_range test_1 0 1234567890 3)" "1234567890 0.1"

assert "$(./stsdb -d . get_range test_1 2234567891 3234567890)" ""
assert "$(./stsdb -d . get_range test_1 2234567891 3234567890 2)" ""
assert "$(./stsdb -d . get_range test_1 2234567890 3234567890)" "2234567890 0.2"
assert "$(./stsdb -d . get_range test_1 2234567890 3234567890 2)" "2234567890 0.2"

assert "$(./stsdb -d . get_range test_1 0 2000000000)" "1234567890 0.1"
assert "$(./stsdb -d . get_range test_1 0 2000000000 3)" "1234567890 0.1"
assert "$(./stsdb -d . get_range test_1 2000000000 3000000000)" "2234567890 0.2"
assert "$(./stsdb -d . get_range test_1 2000000000 3000000000 3)" "2234567890 0.2"

assert "$(./stsdb -d . get_range test_1)" "1234567890 0.1
2234567890 0.2"
assert "$(./stsdb -d . get_range test_1 0 3384967290 2)" "1234567890 0.1
2234567890 0.2"
assert "$(./stsdb -d . get_range test_1 1234567890 2234567890 2)" "1234567890 0.1
2234567890 0.2"

assert "$(./stsdb -d . get_range test_1 1234567890 2234567890 1200000000)" "1234567890 0.1"

# get_interp
assert "$(./stsdb -d . put test_2 1000 0.1 10 30)" ""
assert "$(./stsdb -d . put test_2 2000 0.2 20)" ""
assert "$(./stsdb -d . get_interp test_2 800)"  ""
assert "$(./stsdb -d . get_interp test_2 2200)" ""
assert "$(./stsdb -d . get_interp test_2 1000)" "1000000 0.1 10 30"
assert "$(./stsdb -d . get_interp test_2 2000)" "2000000 0.2 20"
assert "$(./stsdb -d . get_interp test_2 1200)" "1200000 0.12 12"
assert "$(./stsdb -d . get_interp test_2 1800)" "1800000 0.18 18"

# remove all test databases
rm -f test*.db
