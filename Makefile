CC=gcc
OPTIONS= -g 

all: main

rvm:
	$(CC) $(OPTIONS) -c rvm.c

main: rvm
	$(CC) $(OPTIONS) -o main simple_test.c rvm.o

clean:
	rm *.o
