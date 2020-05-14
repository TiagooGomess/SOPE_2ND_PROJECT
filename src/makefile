objects = U2 Q2
all:$(objects)
CC = gcc
CFLAGS = -Wall -pthread
DEPS = 

%.objects: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Q2: Q2.o 
	$(CC) -o Q2 Q2.o -lrt -lpthread
U2: U2.o
	$(CC) -o U2 U2.o -lrt -lpthread


.PHONY: clean

clean:
	rm -f $(objects) *.o *.d *.i *.s
