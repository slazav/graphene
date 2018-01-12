LDLIBS=-lmicrohttpd -lm -ldb -ljansson
CPPFLAGS=-g
CC=g++

bindir         ?= /usr/bin
sharedstatedir ?= /var/lib
initdir       ?= /etc/init.d


all: graphene graphene_http graphene_json tests
install: all
	mkdir -p ${bindir} ${initdir}
	mkdir -m 02755 -p ${sharedstatedir}/graphene
	install graphene graphene_http ${bindir}
	install graphene_http.init ${initdir}/graphene_http

dbinfo.o:      dbinfo.h dbinfo.cpp
dbinfo_v1.o:   dbinfo.h dbinfo_v1.cpp
dbinfo_v2.o:   dbinfo.h dbinfo_v2.cpp
db.o:          dbinfo.h dbout.h db.h db.cpp
graphene.o:      dbpool.h dbinfo.h db.h dbout.h graphene.cpp
test_db.o:     tests.h dbinfo.h db.h dbout.h test_db.cpp
test_json.o:   tests.h json.h test_json.cpp
json.o:        dbinfo.h db.h dbout.h json.h json.cpp
graphene_json.o:  dbinfo.h db.h dbout.h json.h graphene_json.cpp
graphene_http.o:  dbinfo.h db.h dbout.h json.h graphene_http.cpp

graphene:      dbinfo.o dbinfo_v1.o dbinfo_v2.o db.o graphene.o
graphene_http: dbinfo.o dbinfo_v1.o dbinfo_v2.o db.o json.o graphene_http.o
graphene_json: dbinfo.o dbinfo_v1.o dbinfo_v2.o db.o json.o graphene_json.o
test_db:    dbinfo.o dbinfo_v1.o dbinfo_v2.o db.o test_db.o
test_json:  dbinfo.o dbinfo_v1.o dbinfo_v2.o db.o json.o test_json.o

tests: test_db test_json test_cli.sh
	rm -f __db.* *.db
	./test_db
	./test_json
	./test_cli.sh
	./test_json.sh
	./test_v1.sh

clean:
	rm -f *.o test_db test_json graphene graphene_http graphene_json *.db