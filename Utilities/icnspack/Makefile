CC ?= gcc
CFLAGS=-c -Wall -Wextra -pedantic -O3
LDFLAGS ?=

all: icnspack

icnspack: icnspack.o
	$(CC) $(LDFLAGS) icnspack.o -o icnspack

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o icnspack
