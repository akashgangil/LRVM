CC=clang
OPTIONS= -g -std=c99

all: simple_test

simple_test:
	$(CC) $(OPTIONS) -o simple_test.o simple_test.c

clean:
	rm *.o
