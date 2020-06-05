## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

STANDALONE = 1
PROJECT    = RsaTool
PRODUCT    = $(PROJECT)$(SUFFIX)
OBJS       = $(PROJECT).o
include ../../User/Makefile

CFLAGS  += -I/usr/local/opt/openssl/include -I/opt/local/include
LDFLAGS += -L/usr/local/opt/openssl/lib -L/opt/local/lib
LDLIBS  += -lcrypto
