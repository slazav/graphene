#!/bin/sh -x
# filters

PATH=../src:$PATH

graphene -d . create filt INT16

graphene -d . put filt 1234567890 3 2
graphene -d . put filt 1234567900 4 3
graphene -d . put filt 1234567910 5 4
graphene -d . put filt 1234567920 6 1

graphene -d . get_range 'filt:1|f1'
graphene -d . get_range 'filt:1|f2'
graphene -d . delete filt
