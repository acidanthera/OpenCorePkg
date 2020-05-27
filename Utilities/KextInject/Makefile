## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

CURR_DIR = $(shell pwd)

include $(CURR_DIR)/../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = KextInject

OBJS          += KextInject.o
OBJS          += Lilu.o Vsmc.o
OBJS          += KextPatcher.o PrelinkedKext.o PrelinkedContext.o Vtables.o Link.o KernelReader.o DataPatcher.o CxxSymbols.o Header.o Symbols.o Relocations.o OcXmlLib.o OcAsciiLib.o lzss.o lzvn.o

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)

Lilu.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/Lilu.c -o $@

Vsmc.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/Vsmc.c -o $@

KextPatcher.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcAppleKernelLib/KextPatcher.c -o $@

PrelinkedKext.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcAppleKernelLib/PrelinkedKext.c -o $@

PrelinkedContext.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcAppleKernelLib/PrelinkedContext.c -o $@

Vtables.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcAppleKernelLib/Vtables.c -o $@

Link.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcAppleKernelLib/Link.c -o $@

KernelReader.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcAppleKernelLib/KernelReader.c -o $@

DataPatcher.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcMiscLib/DataPatcher.c -o $@

CxxSymbols.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcMachoLib/CxxSymbols.c -o $@

Header.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcMachoLib/Header.c -o $@

Symbols.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcMachoLib/Symbols.c -o $@

Relocations.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcMachoLib/Relocations.c -o $@

OcXmlLib.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcXmlLib/OcXmlLib.c -o $@

OcAsciiLib.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcStringLib/OcAsciiLib.c -o $@

lzss.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcCompressionLib/lzss/lzss.c -o $@

lzvn.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcCompressionLib/lzvn/lzvn.c -o $@

clean:
	rm -rf *.o *.dSYM $(PRODUCT)
