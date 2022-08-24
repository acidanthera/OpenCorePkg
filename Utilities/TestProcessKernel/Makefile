## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = ProcessKernel
PRODUCT = $(PROJECT)$(INFIX)$(SUFFIX)
OBJS    = $(PROJECT).o \
	ProcessKernelDummy.o \
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
	zlib_uefi.o \
	zutil.o
#
# OcConfigurationLib targets.
#
OBJS   += OcConfigurationLib.o
#
# OpenCoreKernel targets.
#
OBJS   += OpenCoreKernel.o
OBJS   += OpenCoreKernelPatch.o

VPATH   = ../../Library/OcConfigurationLib:$\
  ../../Library/OcAppleKernelLib:$\
	../../Library/OcMainLib:$\
	../../Library/OcCompressionLib/lzss:$\
	../../Library/OcCompressionLib/lzvn:$\
	../../Library/OcCompressionLib/zlib
include ../../User/Makefile
