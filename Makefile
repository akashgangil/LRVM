CC=gcc
OPTIONS= -g 

TESTCASESDIR=testcases

all: main basic multi abort

rvm:
	$(CC) $(OPTIONS) -c rvm.c

main: rvm
	$(CC) $(OPTIONS) -o main simple_test.c rvm.o

basic: rvm
	$(CC) $(OPTIONS) -o basic $(TESTCASESDIR)/basic.c rvm.o

multi: rvm
	$(CC) $(OPTIONS) -o multi $(TESTCASESDIR)/multi.c rvm.o

abort: rvm
	$(CC) $(OPTIONS) -o abort $(TESTCASESDIR)/abort.c rvm.o

clean:
	rm -rf *.o main basic multi rvm.log rvm_segments abort
