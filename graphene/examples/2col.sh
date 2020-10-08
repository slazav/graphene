#!/bin/sh -x
# 2 columns

PATH=../src:$PATH

graphene -d . create 2col

graphene -d . put 2col 1234567890 0.1 2
graphene -d . put 2col 1234567900 0.2 3
graphene -d . put 2col 1234567910 0.3 4
graphene -d . put 2col 1234567910 0.4

graphene -d . get_interp 2col 1234567895

graphene -d . get_prev  2col
graphene -d . get_next  2col
graphene -d . get_range 2col:0
graphene -d . get_range 2col:1

graphene -d . get_prev 2col:0
graphene -d . get_prev 2col:10

graphene -d . delete 2col

rm -f __db.*