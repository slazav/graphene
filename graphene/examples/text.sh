#!/bin/sh -x

PATH=../src:$PATH

graphene -d . create db TEXT

graphene -d . put db 1000 "text"
graphene -d . put db 2000 "text
text"
graphene -d . get_interp db 1500

graphene -d . get_prev  db
graphene -d . get_next  db
graphene -d . get_range db

graphene -d . delete db

rm -f __db.*
