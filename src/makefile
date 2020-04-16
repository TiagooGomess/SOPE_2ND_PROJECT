objects = U Q
all:$(objects)
CC = gcc
CFLAGS = -Wall -pthread
DEPS = 

%.objects: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Q: Q.o 
	$(CC) -o Q Q.o -lrt -lpthread
U: U.o
	$(CC) -o U U.o


.PHONY: clean

clean:
	rm -f $(objects) *.o *.d *.i *.s
