CC=gcc
OPTIONS= -g 

TESTCASESDIR=testcases

all: main basic multi abort

rvm:
	$(CC) $(OPTIONS) -c rvm.c
	ar rsv rvm.a rvm.o

main: rvm
	$(CC) $(OPTIONS) -o main simple_test.c rvm.a

basic: rvm
	$(CC) $(OPTIONS) -o basic $(TESTCASESDIR)/basic.c rvm.a

multi: rvm
	$(CC) $(OPTIONS) -o multi $(TESTCASESDIR)/multi.c rvm.a

abort: rvm
	$(CC) $(OPTIONS) -o abort $(TESTCASESDIR)/abort.c rvm.a

clean:
	rm -rf *.o main basic multi rvm.log rvm_segments abort rvm.a
