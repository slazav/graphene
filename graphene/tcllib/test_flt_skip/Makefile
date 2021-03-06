all: data1_skip.dat data2_skip.dat data2_avrg.dat data1_avrg.dat 

data1.dat: mk_test_data
	./$<
data2.dat: mk_test_data2
	./$<

FLT1="flt_skip 1.5 0 50 0 0"
FLT2="flt_skip 1.5 0 50 0 1"

# There are two test programs:
# test_graphene creates a database, and uses it for filtering, as in real life.
# test_simple just run the filter command in non-safe tcl interpreter,
# good for debugging.

#TEST_PRG=./test_graphene
TEST_PRG=./test_simple

data1_skip.dat: data1.dat ../flt_skip.tcl $(TEST_PRG)
	$(TEST_PRG) $(FLT1) < $< > $@

data2_skip.dat: data2.dat ../flt_skip.tcl $(TEST_PRG)
	$(TEST_PRG) $(FLT1) < $< > $@

data1_avrg.dat: data1.dat ../flt_skip.tcl $(TEST_PRG)
	$(TEST_PRG) $(FLT2) < $< > $@

data2_avrg.dat: data2.dat ../flt_skip.tcl $(TEST_PRG)
	$(TEST_PRG) $(FLT2) < $< > $@

clean:
	rm -f *.dat *.png
