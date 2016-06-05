#!/bin/sh -u

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
assert "$(./graphene)" "$help_msg"
assert "$(./graphene -d .)" "$help_msg"

# invalid option (error printed by getopt)
assert "$(./graphene -X . 2>&1)" "./graphene: invalid option -- 'X'"

# unknown command
assert "$(./graphene -d . a)" "Error: Unknown command"


###########################################################################
# database operations

# remove all test databases
rm -f test*.db

# create
assert "$(./graphene -d . create)" "Error: database name expected"
assert "$(./graphene -d . create a dfmt descr e)" "Error: too many parameters"
assert "$(./graphene -d . create a dfmt)" "Error: Unknown data format: dfmt"

assert "$(./graphene -d . create test_1)" ""
assert "$(./graphene -d . create test_2 UINT16)" ""
assert "$(./graphene -d . create test_3 UINT32 "Uint 32 database")" ""
assert "$(./graphene -d . create test_1)" "Error: test_1.db: File exists"

# info
assert "$(./graphene -d . info)" "Error: database name expected"
assert "$(./graphene -d . info a b)" "Error: too many parameters"
assert "$(./graphene -d . info test_x)" "Error: test_x.db: No such file or directory"

assert "$(./graphene -d . info test_1)" "DOUBLE"
assert "$(./graphene -d . info test_2)" "UINT16"
assert "$(./graphene -d . info test_3)" "UINT32	Uint 32 database"

# list
assert "$(./graphene -d . list a)" "Error: too many parameters"
assert "$(./graphene -d . list | sort)" "$(printf "test_1\ntest_2\ntest_3")"

# delete
assert "$(./graphene -d . delete)" "Error: database name expected"
assert "$(./graphene -d . delete a b)" "Error: too many parameters"
assert "$(./graphene -d . delete test_x)" "Error: test_x.db: No such file or directory"
assert "$(./graphene -d . delete test_2)" ""
assert "$(./graphene -d . delete test_2)" "Error: test_2.db: No such file or directory"

# rename
assert "$(./graphene -d . rename)" "Error: database old and new names expected"
assert "$(./graphene -d . rename a)" "Error: database old and new names expected"
assert "$(./graphene -d . rename a b c)" "Error: too many parameters"
assert "$(./graphene -d . rename test_x test_y)" "Error: can't rename database: No such file or directory"
assert "$(./graphene -d . rename test_3 test_1)" "Error: can't rename database, destination exists: test_1.db"
assert "$(./graphene -d . rename test_3 test_2)" ""
assert "$(./graphene -d . info test_3)" "Error: test_3.db: No such file or directory"
assert "$(./graphene -d . info test_2)" "UINT32	Uint 32 database"

# set_descr
assert "$(./graphene -d . set_descr)" "Error: database name and new description text expected"
assert "$(./graphene -d . set_descr a)" "Error: database name and new description text expected"
assert "$(./graphene -d . set_descr a b c)" "Error: too many parameters"
assert "$(./graphene -d . set_descr test_x a)" "Error: test_x.db: No such file or directory"
assert "$(./graphene -d . set_descr test_1 "Test DB number 1")" ""
assert "$(./graphene -d . info test_1)" "DOUBLE	Test DB number 1"

assert "$(./graphene -d . delete test_1)" ""
assert "$(./graphene -d . delete test_2)" ""

###########################################################################
# DOUBLE database

assert "$(./graphene -d . create test_1)" ""
assert "$(./graphene -d . put test_1 1234567890000 0.1)" ""
assert "$(./graphene -d . put test_1 2234567890123 0.2)" ""
# get_next
assert "$(./graphene -d . get_next test_1)"               "1234567890000 0.1" # first
assert "$(./graphene -d . get_next test_1 0)"             "1234567890000 0.1"
assert "$(./graphene -d . get_next test_1 1000000000000)" "1234567890000 0.1"
assert "$(./graphene -d . get_next test_1 1234567890000)" "1234567890000 0.1" # ==
assert "$(./graphene -d . get_next test_1 1234567895000)" "2234567890123 0.2"
assert "$(./graphene -d . get_next test_1 now)"           "2234567890123 0.2"
assert "$(./graphene -d . get_next test_1 NoW)"           "2234567890123 0.2"
assert "$(./graphene -d . get_next test_1 2000000000124)" "2234567890123 0.2"
assert "$(./graphene -d . get_next test_1 3000000000000)" ""
assert "$(./graphene -d . get_next test_1 1234567890000000)"    ""

# get_prev
assert "$(./graphene -d . get_prev test_1)"               "2234567890123 0.2" # last
assert "$(./graphene -d . get_prev test_1 0)"             ""
assert "$(./graphene -d . get_prev test_1 1234567890000)" "1234567890000 0.1" # ==
assert "$(./graphene -d . get_prev test_1 2234567890123)" "2234567890123 0.2" # ==
assert "$(./graphene -d . get_prev test_1 1234567895000)" "1234567890000 0.1"
assert "$(./graphene -d . get_prev test_1 now)"           "1234567890000 0.1"
assert "$(./graphene -d . get_prev test_1 3234567895000)" "2234567890123 0.2"

# get_range
assert "$(./graphene -d . get_range test_1 0 1234567880000)" ""
assert "$(./graphene -d . get_range test_1 0 1234567880000 2)" ""
assert "$(./graphene -d . get_range test_1 0 1234567890000)"   "1234567890000 0.1"
assert "$(./graphene -d . get_range test_1 0 1234567890000 3)" "1234567890000 0.1"

assert "$(./graphene -d . get_range test_1 2234567891000 3234567890000)" ""
assert "$(./graphene -d . get_range test_1 2234567891000 3234567890000 2)" ""
assert "$(./graphene -d . get_range test_1 2234567890123 3234567890000)"   "2234567890123 0.2"
assert "$(./graphene -d . get_range test_1 2234567890123 3234567890000 2)" "2234567890123 0.2"

assert "$(./graphene -d . get_range test_1 0 2000000000000)"   "1234567890000 0.1"
assert "$(./graphene -d . get_range test_1 0 2000000000000 3)" "1234567890000 0.1"
assert "$(./graphene -d . get_range test_1 2000000000000 3000000000000)"   "2234567890123 0.2"
assert "$(./graphene -d . get_range test_1 2000000000000 3000000000000 3)" "2234567890123 0.2"

assert "$(./graphene -d . get_range test_1)" "1234567890000 0.1
2234567890123 0.2"
assert "$(./graphene -d . get_range test_1 0 3384967290000 2)" "1234567890000 0.1
2234567890123 0.2"
assert "$(./graphene -d . get_range test_1 1234567890000 2234567890123 2)" "1234567890000 0.1
2234567890123 0.2"

assert "$(./graphene -d . get_range test_1 1234567890000 2234567890123 1200000000000)" "1234567890000 0.1"
assert "$(./graphene -d . delete test_1)" ""

###########################################################################
# interpolation and columns (DOUBLE database)
assert "$(./graphene -d . create test_2 DOUBLE "double database")" ""

assert "$(./graphene -d . put test_2 1000 1 10 30)" ""
assert "$(./graphene -d . put test_2 2000 2 20)" ""

# get
assert "$(./graphene -d . get test_2 800)"  ""
assert "$(./graphene -d . get test_2 2200)" "2000 2 20"
assert "$(./graphene -d . get test_2 1000)" "1000 1 10 30"
assert "$(./graphene -d . get test_2 2000)" "2000 2 20"
assert "$(./graphene -d . get test_2 1200)" "1200 1.2 12"
assert "$(./graphene -d . get test_2 1800)" "1800 1.8 18"
assert "$(./graphene -d . get test_2:1 1200)" "1200 12"

# columns
assert "$(./graphene -d . get_next test_2:0)" "1000 1"
assert "$(./graphene -d . get_next test_2:1)" "1000 10"
assert "$(./graphene -d . get_next test_2:3)" "1000 NaN"
assert "$(./graphene -d . get_prev test_2:0)" "2000 2"
assert "$(./graphene -d . get_prev test_2:1)" "2000 20"
assert "$(./graphene -d . get_prev test_2:3)" "2000 NaN"

assert "$(./graphene -d . get_range test_2:0)" "1000 1
2000 2"

assert "$(./graphene -d . get_range test_2:3)" "1000 NaN
2000 NaN"
assert "$(./graphene -d . delete test_2)" ""

###########################################################################
# deleting records

assert "$(./graphene -d . create test_3 UINT32 "Uint 32 database")" ""
assert "$(./graphene -d . put test_3 10 1)" ""
assert "$(./graphene -d . put test_3 11 2)" ""
assert "$(./graphene -d . put test_3 12 3)" ""
assert "$(./graphene -d . put test_3 13 4)" ""
assert "$(./graphene -d . put test_3 14 5)" ""
assert "$(./graphene -d . put test_3 15 6)" ""
assert "$(./graphene -d . get_range test_3 | wc)" "      6      12      30"
assert "$(./graphene -d . get_next test_3 12)" "12 3"
assert "$(./graphene -d . del test_3 12)" ""
assert "$(./graphene -d . get_next test_3 12)" "13 4"
assert "$(./graphene -d . del_range test_3 11 13)" ""
assert "$(./graphene -d . get_range test_3)" "10 1
14 5
15 6"

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
assert "$(./graphene -d . get_range test_3:0)" "10 1
11 2
12 3
13 4"
assert "$(./graphene -d . get_range 'test_3:0|test_flt')" "10 1
11 4
12 9
13 16"

rm -f test_flt
assert "$(./graphene -d . delete test_3)" ""

###########################################################################
# TEXT database
assert "$(./graphene -d . create test_4 TEXT)" ""

assert "$(./graphene -d . put test_4 1000 text1)" ""
assert "$(./graphene -d . put test_4 2000 "text2
2")" "" # line break will be converted to space
# get_next
assert "$(./graphene -d . get_next test_4)"      "1000 text1" # first
assert "$(./graphene -d . get_next test_4 0)"    "1000 text1"
assert "$(./graphene -d . get_next test_4 1000)" "1000 text1" # ==
assert "$(./graphene -d . get_next test_4 1500)" "2000 text2 2"
assert "$(./graphene -d . get_next test_4 2000)" "2000 text2 2"
assert "$(./graphene -d . get_next test_4 2001)" ""
assert "$(./graphene -d . get_next test_4 1234567890000000)"    ""

# get_prev
assert "$(./graphene -d . get_prev test_4)"      "2000 text2 2" # last
assert "$(./graphene -d . get_prev test_4 0)"    ""
assert "$(./graphene -d . get_prev test_4 1000)" "1000 text1" # ==
assert "$(./graphene -d . get_prev test_4 2000)" "2000 text2 2" # ==
assert "$(./graphene -d . get_prev test_4 1500)" "1000 text1"
assert "$(./graphene -d . get_prev test_4 2001)" "2000 text2 2"

# get - same
assert "$(./graphene -d . get test_4)"      "2000 text2 2" # last
assert "$(./graphene -d . get test_4 0)"    ""
assert "$(./graphene -d . get test_4 1000)" "1000 text1" # ==
assert "$(./graphene -d . get test_4 2000)" "2000 text2 2" # ==
assert "$(./graphene -d . get test_4 1500)" "1000 text1"
assert "$(./graphene -d . get test_4 2001)" "2000 text2 2"

# get_range
assert "$(./graphene -d . get_range test_4 0 999)" ""
assert "$(./graphene -d . get_range test_4 0 999 2)" ""
assert "$(./graphene -d . get_range test_4 0 1000)"   "1000 text1"
assert "$(./graphene -d . get_range test_4 0 1000 3)" "1000 text1"

assert "$(./graphene -d . get_range test_4 2001 3000)" ""
assert "$(./graphene -d . get_range test_4 2001 3000 2)" ""
assert "$(./graphene -d . get_range test_4 2000 3000)" "2000 text2 2"
assert "$(./graphene -d . get_range test_4 2000 3000 2)" "2000 text2 2"

assert "$(./graphene -d . get_range test_4)" "1000 text1
2000 text2 2"
assert "$(./graphene -d . get_range test_4 1000 2000)" "1000 text1
2000 text2 2"
assert "$(./graphene -d . get_range test_4 1000 2000 1000)" "1000 text1
2000 text2 2"
assert "$(./graphene -d . get_range test_4 1000 2000 1001)" "1000 text1"

# columns are not important
assert "$(./graphene -d . get_next test_4:5)" "1000 text1"
assert "$(./graphene -d . delete test_4)" ""


###########################################################################
# interactive mode


# create
assert "$(./graphene -d . interactive 1)" "Error: too many parameters"
assert "$(printf 'x' | ./graphene -d . interactive)" "Error: Unknown command"
assert "$(printf 'create' | ./graphene -d . interactive)" "Error: database name expected"

assert "$(printf 'create test_1
                  info test_1' | ./graphene -d . interactive)" "DOUBLE"

assert "$(printf 'create test_2
                  put test_1 10 0
                  put test_1 20 10
                  put test_2 30 20
                  put test_2 40 30
                  get test_1 15
                  get test_2 38' | ./graphene -d . interactive)" "15 5
38 28"

###########################################################################
# remove all test databases
rm -f test*.db
