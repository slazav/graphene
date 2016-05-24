#JANSSON_CFLAGS := $(shell pkg-config --cflags jansson)
#JANSSON_LIBS := $(shell pkg-config --libs jansson)

LDLIBS=../jansson/libjansson.a
CFLAGS=-I../jansson

all: test1 test2

test_jsonxx: test_jsonxx.cpp jsonxx.h Makefile
	$(CXX) -o $@ -g -O0 -Wall $(CFLAGS) test_jsonxx.cpp $(LDLIBS)

test_jsonval: test_jsonval.cpp jsonval.cpp jsonxx.h Makefile
	$(CXX) -o $@ -g -O0 -Wall $(CFLAGS) test_jsonval.cpp jsonval.cpp $(LDLIBS)

test1: test_jsonxx
	./test_jsonxx

test2: test_jsonval
	./test_jsonval

clean:
	rm -f test_jsonxx  test_jsonval
