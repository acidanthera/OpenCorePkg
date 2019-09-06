CC ?= gcc
CFLAGS=-c -Wall -Wextra -pedantic -O3 -DDEBUG -I../../Include -include UefiCompat.h
OBJS=AppleEfiBinary.o Sha2.o Rsa2048Sha256.o OcAppleKeysLib.o main.o

all: AppleEfiSignTool

AppleEfiSignTool: $(OBJS)
	$(CC) $(OBJS) -o AppleEfiSignTool

Sha2.o:
	$(CC) $(CFLAGS) ../../Library/OcCryptoLib/Sha2.c -o $@

Rsa2048Sha256.o:
	$(CC) $(CFLAGS) ../../Library/OcCryptoLib/Rsa2048Sha256.c -o $@

OcAppleKeysLib.o:
	$(CC) $(CFLAGS) ../../Library/OcAppleKeysLib/OcAppleKeysLib.c -o $@

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o AppleEfiSignTool
