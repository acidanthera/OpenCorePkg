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

ifeq ($(DIST),Darwin)
ifeq ($(shell uname -p),arm)
OPENSSL_PATH = /opt/homebrew/opt/openssl
else
OPENSSL_PATH = /usr/local/opt/openssl
endif
endif

CFLAGS  += -Ilibressl/include -I$(OPENSSL_PATH)/include -I/opt/local/include -Wno-deprecated-declarations
LDFLAGS += -Llibressl/lib -L$(OPENSSL_PATH)/lib -L/opt/local/lib
LDLIBS  += -lcrypto
