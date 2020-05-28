## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

include ../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = KextInject$(SUFFIX)

OBJS          += KextInject.o
OBJS          += Lilu.o Vsmc.o
OBJS          += KextPatcher.o PrelinkedKext.o PrelinkedContext.o Vtables.o Link.o KernelReader.o DataPatcher.o CxxSymbols.o Header.o Symbols.o Relocations.o OcXmlLib.o OcAsciiLib.o lzss.o lzvn.o

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)
	$(STRIP) $(PRODUCT)

Lilu.o:
	$(CC) $(CFLAGS) ./Lilu.c -o $@

Vsmc.o:
	$(CC) $(CFLAGS) ./Vsmc.c -o $@

KextPatcher.o:
	$(CC) $(CFLAGS) ../../Library/OcAppleKernelLib/KextPatcher.c -o $@

PrelinkedKext.o:
	$(CC) $(CFLAGS) ../../Library/OcAppleKernelLib/PrelinkedKext.c -o $@

PrelinkedContext.o:
	$(CC) $(CFLAGS) ../../Library/OcAppleKernelLib/PrelinkedContext.c -o $@

Vtables.o:
	$(CC) $(CFLAGS) ../../Library/OcAppleKernelLib/Vtables.c -o $@

Link.o:
	$(CC) $(CFLAGS) ../../Library/OcAppleKernelLib/Link.c -o $@

KernelReader.o:
	$(CC) $(CFLAGS) ../../Library/OcAppleKernelLib/KernelReader.c -o $@

DataPatcher.o:
	$(CC) $(CFLAGS) ../../Library/OcMiscLib/DataPatcher.c -o $@

CxxSymbols.o:
	$(CC) $(CFLAGS) ../../Library/OcMachoLib/CxxSymbols.c -o $@

Header.o:
	$(CC) $(CFLAGS) ../../Library/OcMachoLib/Header.c -o $@

Symbols.o:
	$(CC) $(CFLAGS) ../../Library/OcMachoLib/Symbols.c -o $@

Relocations.o:
	$(CC) $(CFLAGS) ../../Library/OcMachoLib/Relocations.c -o $@

OcXmlLib.o:
	$(CC) $(CFLAGS) ../../Library/OcXmlLib/OcXmlLib.c -o $@

OcAsciiLib.o:
	$(CC) $(CFLAGS) ../../Library/OcStringLib/OcAsciiLib.c -o $@

lzss.o:
	$(CC) $(CFLAGS) ../../Library/OcCompressionLib/lzss/lzss.c -o $@

lzvn.o:
	$(CC) $(CFLAGS) ../../Library/OcCompressionLib/lzvn/lzvn.c -o $@

clean:
	rm -rf *.o *.dSYM $(PRODUCT)
