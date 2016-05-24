LDLIBS=-lmicrohttpd -lm -ldb ./jansson/libjansson.a
CPPFLAGS=-g -I./jansson
CC=g++

all: stsdb stsdb_http tests

stsdb.o:     db.h stsdb.cpp
test_db.o:   tests.h db.h test_db.cpp
test_json.o: tests.h json.h json.cpp test_json.cpp

test_json: json.o test_json.o
stsdb_http: json.o stsdb_http.o

tests: test_db test_json test_cli.sh
	./test_db
	rm -f test.db
	./test_json
	rm -f test.db
	./test_cli.sh

clean:
	rm -f *.o test_db test_json stsdb *.db