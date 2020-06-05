## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = Img4
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o
#
# From OcAppleImg4Lib.
#
OBJS   += OcAppleImg4Lib.o DER_Img4Manifest.o DER_Keys.o DER_Decode.o DER_CertCrl.o oids.o Img4oids.o

VPATH   = ../../Library/OcAppleImg4Lib:$\
          ../../Library/OcAppleImg4Lib/libDERImg4:$\
          ../../Library/OcAppleImg4Lib/libDER:$\

include ../../User/Makefile
CFLAGS += -I../../Library/OcAppleImg4Lib
