## @file
# Copyright (c) 2020, vit9696. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PRODUCT = DiskImage$(SUFFIX)
OBJS    = DiskImage.o \
	FileDummy.o \
	OcAppleChunklistLib.o \
	OcAppleDiskImageLib.o \
	OcAppleDiskImageLibInternal.o \
	OcXmlLib.o \
	Sha2.o \
	BigNumWordMul64.o \
	BigNumPrimitives.o \
	BigNumMontgomery.o \
	RsaDigitalSign.o \
	SecureMem.o \
	OcAppleKeysLib.o \
	OcAsciiLib.o \
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
	../../Library/OcXmlLib:$\
	../../Library/OcCryptoLib:$\
	../../Library/OcCryptoLib/IA32:$\
	../../Library/OcAppleKeysLib:$\
	../../Library/OcStringLib:$\
	../../Library/OcFileLib:$\
	../../Library/OcCompressionLib/zlib
include ../../User/Makefile
