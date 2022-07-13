## @file
#  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause
##

# CC=clang DEBUG=1 FUZZ=1 SANITIZE=1 make -j4 fuzz
PROJECT = TestExt4Dxe
PRODUCT = $(PROJECT)$(INFIX)$(SUFFIX)
OBJS    = $(PROJECT).o
OBJS   += BaseUcs2Utf8Lib.o BaseOrderedCollectionRedBlackTreeLib.o
OBJS   += BlockGroup.o Collation.o Crc16.o Crc32c.o Directory.o DiskUtil.o
OBJS   += Ext4Dxe.o Extents.o File.o Inode.o Partition.o Superblock.o

ifeq ($(UDK_PATH),)
	ifneq ($(PACKAGES_PATH),)
		UDK_PATH := $(PACKAGES_PATH)
	else
		UDK_PATH ?= ../../UDK
	endif
endif

OC_USER  = $(UDK_PATH)/OpenCorePkg

include  $(OC_USER)/User/Makefile

CFLAGS  += -I$(UDK_PATH)/RedfishPkg/Include
CFLAGS  += -I$(UDK_PATH)/edk2-platforms/Features/Ext4Pkg/Ext4Dxe

VPATH   += $(UDK_PATH)/RedfishPkg/Library/BaseUcs2Utf8Lib:$\
           $(UDK_PATH)/MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib:$\
           $(UDK_PATH)/edk2-platforms/Features/Ext4Pkg/Ext4Dxe
