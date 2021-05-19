MOD_HEADERS := gr_db.h gr_env.h json.h data.h filter.h
MOD_SOURCES := gr_db.cpp gr_env.cpp json.cpp data.cpp filter.cpp

SIMPLE_TESTS := gr_env json0 data1 data2 filter
SCRIPT_TESTS := json1
OTHER_TESTS := test_cli.sh test_v1.sh\
   graphene_http.test1 graphene_http.test2

PROGRAMS := graphene graphene_http

PKG_CONFIG := libmicrohttpd libdb jansson tcl
LDLIBS=-lm

MODDIR      := ../modules
include $(MODDIR)/Makefile.inc
