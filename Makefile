CC=clang
OPTIONS= -g -std=c99

all: rvm simple_test

rvm:
	$(CC) $(OPTIONS) -o rvm.o -c rvm.c

simple_test:
	$(CC) $(OPTIONS) -o simple_test.o simple_test.c rvm.o

clean:
	rm *.o
