## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PRODUCT = KextInject$(SUFFIX)
OBJS    = KextInject.o \
	Lilu.o \
	Vsmc.o \
	KextPatcher.o \
	PrelinkedKext.o \
	PrelinkedContext.o \
	Vtables.o \
	Link.o \
	KernelReader.o \
	DataPatcher.o \
	CxxSymbols.o \
	Header.o \
	Symbols.o \
	Relocations.o \
	OcXmlLib.o \
	OcAsciiLib.o \
	lzss.o \
	lzvn.o
VPATH   = ../../Library/OcAppleKernelLib:$\
	../../Library/OcMiscLib:$\
	../../Library/OcMachoLib:$\
	../../Library/OcXmlLib:$\
	../../Library/OcStringLib:$\
	../../Library/OcCompressionLib/lzss:$\
	../../Library/OcCompressionLib/lzvn
include ../../User/Makefile
