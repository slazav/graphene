#!/bin/sh -x
# filters

PATH=..:$PATH

stsdb -d . create filt INT16

stsdb -d . put filt 1234567890 3 2
stsdb -d . put filt 1234567900 4 3
stsdb -d . put filt 1234567910 5 4
stsdb -d . put filt 1234567920 6 1

stsdb -d . get_range 'filt:1|f1'
stsdb -d . get_range 'filt:1|f2'
stsdb -d . delete filt
