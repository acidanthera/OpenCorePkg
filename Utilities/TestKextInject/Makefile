## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = KextInject
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o \
	Lilu.o \
	Vsmc.o \
	KextPatcher.o \
	PrelinkedKext.o \
	PrelinkedContext.o \
	Vtables.o \
	Link.o \
	KernelReader.o \
	KernelCollection.o \
	DataPatcher.o \
	lzss.o \
	lzvn.o
VPATH   = ../../Library/OcAppleKernelLib:$\
	../../Library/OcMiscLib:$\
	../../Library/OcCompressionLib/lzss:$\
	../../Library/OcCompressionLib/lzvn
include ../../User/Makefile
