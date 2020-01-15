MOD_HEADERS := dbgr.h dbout.h dbpool.h json.h data.h
MOD_SOURCES := dbgr.cpp dbout.cpp dbpool.cpp json.cpp data.cpp

SIMPLE_TESTS := dbout dbpool json0 data
SCRIPT_TESTS := json1
OTHER_TESTS := test_cli.sh test_v1.sh

PROGRAMS := graphene graphene_http

PKG_CONFIG := libmicrohttpd libdb jansson
LDLIBS=-lm

MODDIR      := ../modules
include $(MODDIR)/Makefile.inc
