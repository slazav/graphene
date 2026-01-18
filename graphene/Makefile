MOD_HEADERS := gr_db.h gr_env.h gr_tcl.h json.h data.h
MOD_SOURCES := gr_db.cpp gr_env.cpp gr_tcl.cpp json.cpp data.cpp

SIMPLE_TESTS := gr_env json0 data1 data2
SCRIPT_TESTS := json1
OTHER_TESTS := test_cli.sh test_v1.sh\
   graphene_http.test1 graphene_http.test2

PROGRAMS := graphene graphene_http graphene_meas

# use "DEBIAN=1 make" for building in Debian

ifndef DEBIAN
  PKG_CONFIG := libmicrohttpd jansson tcl
  LDLIBS=-lm -ldb
else
  PKG_CONFIG := libmicrohttpd libdb jansson
  LDLIBS = -lm -ltcl
  CXXFLAGS = -I/usr/include/tcl
endif

MODDIR      := ../modules
include $(MODDIR)/Makefile.inc
