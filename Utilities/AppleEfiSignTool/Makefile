## @file
# Copyright (c) 2018, savvas. All rights reserved.
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = AppleEfiSignTool
PRODUCT = $(PROJECT)$(INFIX)$(SUFFIX)
OBJS    = $(PROJECT).o \
	PeCoffDebug.o \
	PeCoffHash.o \
	PeCoffInfo.o \
	PeCoffInit.o \
	PeCoffLoad.o \
	PeCoffRelocate.o \
	OcPeCoffExtLib.o \
	OcPeCoffFixup.o
VPATH   = $(UDK_PATH)/MdePkg/Library/BasePeCoffLib2:$\
          ../../Library/OcPeCoffExtLib:$\

include ../../User/Makefile
