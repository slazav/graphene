CFLAGS = -O2 -Wall

locktcl.so: locktcl.c locklib.h locklib.o
	$(CC) -shared $(CFLAGS) -fpic -o locktcl.so \
            -I/rota/software/tcl/include \
            locktcl.c locklib.o

locklib.o: locklib.c locklib.h

