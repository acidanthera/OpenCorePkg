CC ?= gcc
CFLAGS=-c -Wall -Wextra -pedantic -O3 -I../../Include -IIncludeDummy -include UefiCompat.h
OBJS=AppleEfiBinary.o Sha2.o BigNumWordMul64.o BigNumPrimitives.o BigNumMontgomery.o RsaDigitalSign.o OcAppleKeysLib.o main.o

all: AppleEfiSignTool

AppleEfiSignTool: $(OBJS)
	$(CC) $(OBJS) -o AppleEfiSignTool

Sha2.o:
	$(CC) $(CFLAGS) ../../Library/OcCryptoLib/Sha2.c -o $@

BigNumWordMul64.o:
	$(CC) $(CFLAGS) ../../Library/OcCryptoLib/IA32/BigNumWordMul64.c -o $@

BigNumPrimitives.o:
	$(CC) $(CFLAGS) ../../Library/OcCryptoLib/BigNumPrimitives.c -o $@

BigNumMontgomery.o:
	$(CC) $(CFLAGS) ../../Library/OcCryptoLib/BigNumMontgomery.c -o $@

RsaDigitalSign.o:
	$(CC) $(CFLAGS) -include OcCryptoConfig.h ../../Library/OcCryptoLib/RsaDigitalSign.c -o $@

OcAppleKeysLib.o:
	$(CC) $(CFLAGS) ../../Library/OcAppleKeysLib/OcAppleKeysLib.c -o $@

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o AppleEfiSignTool
