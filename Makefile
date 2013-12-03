CC=gcc
OPTIONS= -g 

TESTCASESDIR=testcases

all: compare_trans compare_heavy_trans basic multi abort multi-abort truncate

rvm:
	$(CC) $(OPTIONS) -c rvm.c
	ar rsv librvm.a rvm.o

basic: rvm
	$(CC) $(OPTIONS) -o basic $(TESTCASESDIR)/basic.c librvm.a

multi: rvm
	$(CC) $(OPTIONS) -o multi $(TESTCASESDIR)/multi.c librvm.a

abort: rvm
	$(CC) $(OPTIONS) -o abort $(TESTCASESDIR)/abort.c librvm.a

multi-abort: rvm
	$(CC) $(OPTIONS) -o multi-abort $(TESTCASESDIR)/multi-abort.c librvm.a

truncate: rvm
	$(CC) $(OPTIONS) -o truncate $(TESTCASESDIR)/truncate.c librvm.a

compare_trans: rvm timer.o
	$(CC) $(OPTIONS) -o compare_trans compare_trans.c librvm.a timer.o

compare_heavy_trans: rvm timer.o
	$(CC) $(OPTIONS) -o compare_heavy_trans compare_heavy_trans.c librvm.a timer.o

timer.o: timer.h

clean:
	rm -rf *.o main basic multi rvm.log rvm_segments abort rvm.a multi-abort compare_trans compare_heavy_trans truncate rvm_undo.log

#clean:
#	rm librvm.a compare_trans compare_heavy_trans
