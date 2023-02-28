## @file
#  Copyright (c) 2023, Savva Mitrofanov. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = TestFatDxe
PRODUCT = $(PROJECT)$(INFIX)$(SUFFIX)
OBJS    = $(PROJECT).o
OBJS    += DirectoryCache.o DiskCache.o FileName.o Hash.o
OBJS    += DirectoryManage.o ReadWrite.o OpenVolume.o Open.o
OBJS    += Misc.o Init.o Info.o FileSpace.o Flush.o Delete.o Data.o
OBJS    += UnicodeCollation.o

include  ../../User/Makefile

CFLAGS  += -I$(UDK_PATH)/FatPkg/EnhancedFatDxe

ifeq ($(MEMORY_MUTATIONS),1)
	CFLAGS += -DMEMORY_MUTATIONS
endif

VPATH   += $(UDK_PATH)/FatPkg/EnhancedFatDxe:$
