## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = Smbios
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o
#
# From OpenCore.
#
OBJS   += SmbiosPatch.o SmbiosInternal.o DebugSmbios.o LegacyRegionLock.o LegacyRegionUnLock.o

VPATH   = ../../Library/OcSmbiosLib:$\
          ../../Library/OcMemoryLib

include ../../User/Makefile
