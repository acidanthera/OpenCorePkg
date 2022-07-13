## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

STANDALONE = 1
PROJECT    = RsaTool
PRODUCT    = $(PROJECT)$(INFIX)$(SUFFIX)
OBJS       = $(PROJECT).o
include ../../User/Makefile

ifeq ($(DIST),Darwin)
libressl/lib/libcrypto.a:
	./build_libressl.sh

$(OUT_DIR)/$(PROJECT).o: libressl/lib/libcrypto.a
endif

CFLAGS  += -Ilibressl/include -I/usr/local/opt/openssl/include -I/opt/local/include -Wno-deprecated-declarations
LDFLAGS += -Llibressl/lib -L/usr/local/opt/openssl/lib -L/opt/local/lib
LDLIBS  += -lcrypto
