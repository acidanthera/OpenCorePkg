## @file
# Copyright (c) 2018, savvas. All rights reserved.
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PRODUCT = AppleEfiSignTool$(SUFFIX)
OBJS    = main.o \
	AppleEfiBinary.o \
	Sha2.o \
	BigNumWordMul64.o \
	BigNumPrimitives.o \
	BigNumMontgomery.o \
	RsaDigitalSign.o \
	SecureMem.o \
	OcAppleKeysLib.o
VPATH   = ../../Library/OcCryptoLib:$\
	../../Library/OcCryptoLib/IA32:$\
	../../Library/OcAppleKeysLib
include ../../User/Makefile
