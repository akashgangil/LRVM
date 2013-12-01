CC=gcc
OPTIONS= -g 

TESTCASESDIR=testcases

all: basic multi abort multi-abort truncate compare_trans compare_heavy_trans

rvm:
	$(CC) $(OPTIONS) -c rvm.c
	ar rsv rvm.a rvm.o

basic: rvm
	$(CC) $(OPTIONS) -o basic $(TESTCASESDIR)/basic.c rvm.a

multi: rvm
	$(CC) $(OPTIONS) -o multi $(TESTCASESDIR)/multi.c rvm.a

abort: rvm
	$(CC) $(OPTIONS) -o abort $(TESTCASESDIR)/abort.c rvm.a

multi-abort: rvm
	$(CC) $(OPTIONS) -o multi-abort $(TESTCASESDIR)/multi-abort.c rvm.a

truncate: rvm
	$(CC) $(OPTIONS) -o truncate $(TESTCASESDIR)/truncate.c rvm.a

compare_trans: rvm
	$(CC) $(OPTIONS) -o compare_trans $(TESTCASESDIR)/compare_trans.c rvm.a

compare_heavy_trans: rvm
	$(CC) $(OPTIONS) -o compare_heavy_trans $(TESTCASESDIR)/compare_heavy_trans.c rvm.a

clean:
	rm -rf *.o main basic multi rvm.log rvm_segments abort rvm.a multi-abort compare truncate rvm_undo.log
