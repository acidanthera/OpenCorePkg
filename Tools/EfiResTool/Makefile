CC ?= gcc
CFLAGS=-c -Wall -Wextra -pedantic -O3 -DDEBUG

all: EfiResTool

EfiResTool: EfiResTool.o
	$(CC) EfiResTool.o -o EfiResTool

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o EfiResTool
