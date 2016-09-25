#!/bin/sh -x

PATH=../src:$PATH
graphene -d . create pressure INT16 "Some text"
graphene -d . delete pressure

graphene -d . create pressure DOUBLE "Some text"
graphene -d . rename pressure press
graphene -d . set_descr press "Measured pressure"

graphene -d . info press
graphene -d . list

graphene -d . put press  1234567890000 0.1
graphene -d . put press  1234567900000 0.2
graphene -d . get press  1234567895000

graphene -d . put press now 0.1
graphene -d . put press now 0.2
graphene -d . put press now 0.3
graphene -d . get_prev press
graphene -d . get_next press
graphene -d . get_range press

graphene -d . delete press
