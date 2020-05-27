## @file
# Copyright (c) 2018, savvas. All rights reserved.
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

CURR_DIR = $(shell pwd)

include $(CURR_DIR)/../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = AppleEfiSignTool

OBJS          += main.o
OBJS          += AppleEfiBinary.o Sha2.o BigNumWordMul64.o BigNumPrimitives.o BigNumMontgomery.o RsaDigitalSign.o SecureMem.o OcAppleKeysLib.o

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAG) $(OBJS) -o $(PRODUCT)

Sha2.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcCryptoLib/Sha2.c -o $@

BigNumWordMul64.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcCryptoLib/IA32/BigNumWordMul64.c -o $@

BigNumPrimitives.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcCryptoLib/BigNumPrimitives.c -o $@

BigNumMontgomery.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcCryptoLib/BigNumMontgomery.c -o $@

RsaDigitalSign.o:
	$(CC) $(CFLAGS) -include $(CURR_DIR)/OcCryptoConfig.h $(CURR_DIR)/../../Library/OcCryptoLib/RsaDigitalSign.c -o $@

SecureMem.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcCryptoLib/SecureMem.c -o $@

OcAppleKeysLib.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcAppleKeysLib/OcAppleKeysLib.c -o $@

clean:
	rm -rf *.o $(PRODUCT)
