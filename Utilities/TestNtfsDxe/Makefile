## @file
#  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = TestNtfsDxe
PRODUCT = $(PROJECT)$(INFIX)$(SUFFIX)
OBJS    = $(PROJECT).o
OBJS    += Compression.o Data.o Disc.o Index.o Info.o NTFS.o Open.o Position.o

include  ../../User/Makefile

CFLAGS  += -I../../Platform/OpenNtfsDxe

VPATH   += ../../Platform/OpenNtfsDxe:$
