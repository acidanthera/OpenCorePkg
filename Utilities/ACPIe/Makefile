## @file
# Copyright (c) 2020, Ubsefor, koralexa. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = ACPIe
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o AcpiParser.o
VPATH   = ../../Library/OcAcpiLib:$

include ../../User/Makefile

ifeq ($(DEBUG),1)
 	CFLAGS  += -DVERBOSE
endif
