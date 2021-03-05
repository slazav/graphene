all: data1_skip.dat

data1.dat: mk_test_data
	./$<

FLT1="flt_skip 0.02 0 30 0"

data1_skip.dat: data1.dat ../flt_skip.tcl test_ifilter
	./test_ifilter $(FLT1) data1.dat > data1_skip.dat

clean:
	rm -f *.dat *.png
