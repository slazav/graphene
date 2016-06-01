#!/bin/sh -x

PATH=..:$PATH

stsdb -d . create 2ch

stsdb -d . put 2ch 1234567890 0.1 2
stsdb -d . put 2ch 1234567900 0.2 3
stsdb -d . put 2ch 1234567910 0.3 4
stsdb -d . put 2ch 1234567910 0.4

stsdb -d . get_interp 2ch 1234567895

stsdb -d . get_prev  2ch
stsdb -d . get_next  2ch
stsdb -d . get_range 2ch:0
stsdb -d . get_range 2ch:1

stsdb -d . get_prev 2ch:0
stsdb -d . get_prev 2ch:10

stsdb -d . delete 2ch
