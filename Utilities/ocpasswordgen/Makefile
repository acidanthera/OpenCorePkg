## @file
# Copyright (c) 2020, Marvin Häuser. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = ocpasswordgen
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o
#
# OcCryptoLib targets.
#
OBJS   += PasswordHash.o

VPATH   = ../../Library/OcCryptoLib
include ../../User/Makefile
