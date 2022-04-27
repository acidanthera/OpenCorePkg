## @file
#  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = TestNtfsDxe
PRODUCT = $(PROJECT)$(INFIX)$(SUFFIX)
OBJS    = $(PROJECT).o
OBJS    += Compression.o Data.o Disc.o Index.o Info.o NTFS.o Open.o Position.o

ifeq ($(UDK_PATH),)
	ifneq ($(PACKAGES_PATH),)
		UDK_PATH := $(PACKAGES_PATH)
	else
		UDK_PATH ?= ../../UDK
	endif
endif

OC_USER  = $(UDK_PATH)/OpenCorePkg

include  $(OC_USER)/User/Makefile

CFLAGS  += -I$(OC_USER)/Platform/OpenNtfsDxe

VPATH   += $(OC_USER)/Platform/OpenNtfsDxe:$
