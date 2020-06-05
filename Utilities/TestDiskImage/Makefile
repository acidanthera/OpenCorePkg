## @file
# Copyright (c) 2020, vit9696. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = DiskImage
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o \
	FileDummy.o \
	OcAppleChunklistLib.o \
	OcAppleDiskImageLib.o \
	OcAppleDiskImageLibInternal.o \
	OcAppleRamDiskLib.o \
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
VPATH   = ../../Library/OcAppleChunklistLib:$\
	../../Library/OcAppleDiskImageLib:$\
	../../Library/OcAppleRamDiskLib:$\
	../../Library/OcCompressionLib/zlib
include ../../User/Makefile
