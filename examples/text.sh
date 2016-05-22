#!/bin/sh -x

PATH=$PATH:..

stsdb -d . create db TEXT

stsdb -d . put db 1000 "text"
stsdb -d . put db 2000 "text
text"
stsdb -d . get_interp db 1500

stsdb -d . get_prev  db
stsdb -d . get_next  db
stsdb -d . get_range db

stsdb -d . delete db
