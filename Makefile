LDLIBS=-lmicrohttpd -lm -ldb
CFLAGS=-g
CC=g++

all: stsdb tests

stsdb.o:   stsdb.h stsdb.cpp
test_db.o: tests.h stsdb.h test_db.cpp

tests: test_db
	./test_db
	rm -f test.db
	./test_db.sh

clean:
	rm -f *.o test_db stsdb *.db