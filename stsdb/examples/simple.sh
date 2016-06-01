#!/bin/sh -x

PATH=..:$PATH
stsdb -d . create pressure INT16 "Some text"
stsdb -d . delete pressure

stsdb -d . create pressure DOUBLE "Some text"
stsdb -d . rename pressure press
stsdb -d . set_descr press "Measured pressure"

stsdb -d . info press
stsdb -d . list

stsdb -d . put press  1234567890 0.1
stsdb -d . put press  1234567900 0.2
stsdb -d . get_interp press 1234567895

stsdb -d . put press now 0.1
stsdb -d . put press now 0.2
stsdb -d . put press now 0.3
stsdb -d . get_prev press
stsdb -d . get_next press
stsdb -d . get_range press

stsdb -d . delete press
