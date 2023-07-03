## @file
#  Copyright (c) 2023, Savva Mitrofanov. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = ext4read
PRODUCT = $(PROJECT)$(INFIX)$(SUFFIX)
OBJS    = $(PROJECT).o
OBJS   += BaseUcs2Utf8Lib.o BaseOrderedCollectionRedBlackTreeLib.o
OBJS   += BlockGroup.o BlockMap.o Directory.o DiskUtil.o
OBJS   += Extents.o File.o Inode.o Partition.o Superblock.o Symlink.o

include  ../../User/Makefile

CFLAGS  += -I$(UDK_PATH)/RedfishPkg/Include
CFLAGS  += -I$(UDK_PATH)/Ext4Pkg/Ext4Dxe

VPATH   += $(UDK_PATH)/RedfishPkg/Library/BaseUcs2Utf8Lib:$\
           $(UDK_PATH)/MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib:$\
           $(UDK_PATH)/Ext4Pkg/Ext4Dxe:$
