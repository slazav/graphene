LDLIBS=-lmicrohttpd -lm -ldb -ljansson
CPPFLAGS=-g
CC=g++

all: graphene graphene_http graphene_json tests
install: all
	mkdir -p ${bindir} ${sharedstatedir}/graphene
	install graphene graphene_http ${bindir}

dbinfo.o:      dbinfo.h dbinfo.cpp
dbinfo_v1.o:   dbinfo.h dbinfo_v1.cpp
dbinfo_v2.o:   dbinfo.h dbinfo_v2.cpp
db.o:          dbinfo.h db.h db.cpp
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
	./test_db
	rm -f test.db
	./test_json
	./test_cli.sh
	./test_json.sh
	./test_v1.sh

clean:
	rm -f *.o test_db test_json graphene graphene_http graphene_json *.db