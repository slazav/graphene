#!/bin/sh -x
# 2 columns

PATH=..:$PATH

stsdb -d . create 2col

stsdb -d . put 2col 1234567890 0.1 2
stsdb -d . put 2col 1234567900 0.2 3
stsdb -d . put 2col 1234567910 0.3 4
stsdb -d . put 2col 1234567910 0.4

stsdb -d . get_interp 2col 1234567895

stsdb -d . get_prev  2col
stsdb -d . get_next  2col
stsdb -d . get_range 2col:0
stsdb -d . get_range 2col:1

stsdb -d . get_prev 2col:0
stsdb -d . get_prev 2col:10

stsdb -d . delete 2col
