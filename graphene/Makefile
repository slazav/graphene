LDLIBS=-lmicrohttpd -lm -ldb -ljansson
CPPFLAGS=-g
CC=g++

bindir         ?= /usr/bin
initdir       ?= /etc/init.d


all: graphene graphene_http graphene_json tests
install: all
	mkdir -p ${bindir} ${initdir}
	install graphene graphene_http ${bindir}
	install graphene_http.init ${initdir}/graphene_http
	install scripts/graphene_tab ${bindir}
	install scripts/graphene_int ${bindir}

dbinfo.o:      dbinfo.h dbinfo.cpp
dbinfo_v1.o:   dbinfo.h dbinfo_v1.cpp
dbinfo_v2.o:   dbinfo.h dbinfo_v2.cpp
dbgr.o:         dbinfo.h dbout.h dbgr.h dbgr.cpp
dbpool.o:      dbpool.h dbgr.h
dbout.o:       dbout.h dbgr.h
graphene.o:       dbinfo.h dbgr.h dbout.h dbpool.h graphene.cpp
test_json.o:      json.h tests.h test_json.cpp
test_db.o:        dbinfo.h dbgr.h dbout.h dbpool.h tests.h test_db.cpp
json.o:           dbinfo.h dbgr.h dbout.h dbpool.h json.h json.cpp
graphene_json.o:  dbinfo.h dbgr.h dbout.h dbpool.h json.h graphene_json.cpp
graphene_http.o:  dbinfo.h dbgr.h dbout.h dbpool.h json.h graphene_http.cpp

graphene:      dbinfo.o dbinfo_v1.o dbinfo_v2.o dbgr.o dbpool.o dbout.o graphene.o
graphene_http: dbinfo.o dbinfo_v1.o dbinfo_v2.o dbgr.o dbpool.o dbout.o json.o graphene_http.o
graphene_json: dbinfo.o dbinfo_v1.o dbinfo_v2.o dbgr.o dbpool.o dbout.o json.o graphene_json.o
test_db:    dbinfo.o dbinfo_v1.o dbinfo_v2.o dbgr.o dbpool.o dbout.o test_db.o
test_json:  dbinfo.o dbinfo_v1.o dbinfo_v2.o dbgr.o dbpool.o dbout.o json.o test_json.o

tests: test_db test_json test_cli.sh
	rm -f __db.* *.db
	./test_db
	./test_json
	./test_cli.sh
	./test_json.sh
	./test_v1.sh

clean:
	rm -f *.o test_db test_json graphene graphene_http graphene_json *.db