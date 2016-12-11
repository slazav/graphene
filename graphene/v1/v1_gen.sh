#!/bin/sh -u

# create v1 database (use olny with old graphene program!)

graphene -d . create tab1 INT16  "Int-16 database"
graphene -d . create tab2 DOUBLE "Double database"
graphene -d . create tab3 TEXT   "Text database"

graphene -d . put tab1 1234567890000 0
graphene -d . put tab1 1234567891000 1
graphene -d . put tab1 1234567892001 2
graphene -d . put tab1 1234567893002 3

graphene -d . put tab2 1234567890000 0.0
graphene -d . put tab2 1234567891000 0.1
graphene -d . put tab2 1234567892001 0.2
graphene -d . put tab2 1234567893002 0.3

graphene -d . put tab3 1234567890000 text 1
graphene -d . put tab3 1234567891000 text 2
graphene -d . put tab3 1234567892001 text 3
graphene -d . put tab3 1234567893002 text 4

