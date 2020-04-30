objects = U Q
all:$(objects)
CC = gcc
CFLAGS = -Wall -pthread
DEPS = 

%.objects: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Q1: Q1.o 
	$(CC) -o Q1 Q1.o -lrt -lpthread
U1: U1.o
	$(CC) -o U1 U1.o -lrt -lpthread


.PHONY: clean

clean:
	rm -f $(objects) *.o *.d *.i *.s
