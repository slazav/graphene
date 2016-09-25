CFLAGS=-O9 -fpic -Wall
LDFLAGS=-shared
LDLIBS=-lgpib -ltcl

libgpib.so: gpib.c
	$(CC) $(LDFLAGS) $(CFLAGS) $< $(LDLIBS) -o $@

