targets = Daemon Locking-1.1 Prectime-1.1 GPIB

all:
	for i in $(targets); do make -C $$i; done

clean:
	for i in $(targets); do make -C $$i clean; done
