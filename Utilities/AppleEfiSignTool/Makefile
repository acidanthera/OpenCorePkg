## @file
# Copyright (c) 2018, savvas. All rights reserved.
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = AppleEfiSignTool
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o \
	PeCoffDebug.o \
	PeCoffHash.o \
	PeCoffInit.o \
	PeCoffLoad.o \
	PeCoffRelocate.o \
	PeCoffEntry.o \
	OcAppleImageVerification.o
VPATH   = ../../Library/OcPeCoffLib:$\
          ../../Library/OcAppleImageVerificationLib:$\

include ../../User/Makefile
