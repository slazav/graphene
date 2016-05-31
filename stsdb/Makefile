LDLIBS=-lmicrohttpd -lm -ldb -ljansson
CPPFLAGS=-g
CC=g++

all: stsdb stsdb_http stsdb_json tests
install: all
	mkdir -p ${bindir} ${sharedstatedir}/stsdb
	install stsdb stsdb_http ${bindir}

dbinfo.o:      dbinfo.h dbinfo.cpp
db.o:          dbinfo.h db.h db.cpp
stsdb.o:       dbinfo.h db.h dbout.h stsdb.cpp
test_db.o:     tests.h dbinfo.h db.h dbout.h test_db.cpp
test_json.o:   tests.h json.h test_json.cpp
json.o:        dbinfo.h db.h dbout.h json.h json.cpp
stsdb_json.o:  dbinfo.h db.h dbout.h json.h stsdb_json.cpp
stsdb_http.o:  dbinfo.h db.h dbout.h json.h stsdb_http.cpp

stsdb:      dbinfo.o db.o dbout.o stsdb.o
stsdb_http: dbinfo.o db.o dbout.o json.o stsdb_http.o
stsdb_json: dbinfo.o db.o dbout.o json.o stsdb_json.o
test_db:    dbinfo.o db.o test_db.o
test_json:  dbinfo.o db.o json.o test_json.o

tests: test_db test_json test_cli.sh
	./test_db
	rm -f test.db
	./test_json
	./test_cli.sh
	./test_json.sh

clean:
	rm -f *.o test_db test_json stsdb stsdb_http stsdb_json *.db