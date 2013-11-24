CC=gcc
OPTIONS= -g 

TESTCASESDIR=testcases

all: main basic

rvm:
	$(CC) $(OPTIONS) -c rvm.c

main: rvm
	$(CC) $(OPTIONS) -o main simple_test.c rvm.o

basic: rvm
	$(CC) $(OPTIONS) -o basic $(TESTCASESDIR)/basic.c rvm.o

clean:
	rm *.o main basic rvm.log
