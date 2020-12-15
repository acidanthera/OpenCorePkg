## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = KextInject
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o \
	CommonPatches.o \
	CpuidPatches.o \
	KextPatcher.o \
	KxldState.o \
	PrelinkedKext.o \
	PrelinkedContext.o \
	MkextContext.o \
	Vtables.o \
	Link.o \
	KernelReader.o \
	KernelCollection.o \
	lzss.o \
	lzvn.o \
	adler32.o \
	compress.o \
	crc32.o \
	deflate.o \
	infback.o \
	inffast.o \
	inflate.o \
	inftrees.o \
	trees.o \
	uncompr.o \
	zlib_uefi.o
VPATH   = ../../Library/OcAppleKernelLib:$\
	../../Library/OcCompressionLib/lzss:$\
	../../Library/OcCompressionLib/lzvn:$\
	../../Library/OcCompressionLib/zlib
include ../../User/Makefile
