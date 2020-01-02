MOD_HEADERS := dbgr.h dbinfo.h dbout.h dbpool.h json.h
MOD_SOURCES := dbgr.cpp dbinfo.cpp dbinfo_v2.cpp dbinfo_v1.cpp dbout.cpp dbpool.cpp json.cpp

SIMPLE_TESTS := dbout dbpool json0
SCRIPT_TESTS := json1
OTHER_TESTS := test_cli.sh test_v1.sh

PROGRAMS := graphene graphene_http

PKG_CONFIG := libmicrohttpd libdb jansson
LDLIBS=-lm

MODDIR      := ../modules
include $(MODDIR)/Makefile.inc
