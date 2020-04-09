MOD_HEADERS := dbgr.h dbout.h dbpool.h json.h data.h filter.h
MOD_SOURCES := dbgr.cpp dbpool.cpp json.cpp data.cpp filter.cpp

SIMPLE_TESTS := dbout dbpool json0 data1 data2 filter
SCRIPT_TESTS := json1
OTHER_TESTS := test_cli.sh test_v1.sh\
   graphene_http.test1 graphene_http.test2

PROGRAMS := graphene graphene_http

PKG_CONFIG := libmicrohttpd libdb jansson tcl
LDLIBS=-lm

MODDIR      := ../modules
include $(MODDIR)/Makefile.inc
