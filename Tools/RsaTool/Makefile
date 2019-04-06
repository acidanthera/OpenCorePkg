CC ?= gcc
CFLAGS=-c -Wall -Wextra -pedantic -O3 -I/usr/local/opt/openssl/include -I/opt/local/include
LDFLAGS=-L/usr/local/opt/openssl/lib -L/opt/local/lib -lcrypto

all: RsaTool

RsaTool: RsaTool.o
	$(CC) RsaTool.o -o RsaTool $(LDFLAGS)

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o RsaTool
