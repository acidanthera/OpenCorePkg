CC ?= gcc
CFLAGS=-c -Wall -Wextra -pedantic -O3

all: readlabel

readlabel: readlabel.o
	$(CC) readlabel.o -o readlabel

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o readlabel
