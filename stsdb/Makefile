LDLIBS=-lmicrohttpd -lm -ldb -ljansson
CPPFLAGS=-g
CC=g++

all: stsdb stsdb_http tests
install: all
	mkdir -p ${bindir} ${sharedstatedir}/stsdb
	install stsdb stsdb_http ${bindir}

stsdb.o:     db.h stsdb.cpp
test_db.o:   tests.h db.h test_db.cpp
test_json.o: tests.h json.h test_json.cpp

stsdb:      db.o stsdb.o
stsdb_http: db.o json.o stsdb_http.o
test_db:    db.o test_db.o
test_json:  db.o json.o test_json.o

tests: test_db test_json test_cli.sh
	./test_db
	rm -f test.db
	./test_json
	rm -f test.db
	./test_cli.sh

clean:
	rm -f *.o test_db test_json stsdb *.db