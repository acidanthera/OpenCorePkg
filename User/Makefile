## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

.DEFAULT_GOAL := all

CC       ?= gcc

ifeq ($(UDK_PATH),)
	ifneq ($(PACKAGES_PATH),)
		UDK_PATH = $(PACKAGES_PATH)
	else
		UDK_PATH ?= ../../UDK
	endif
endif

UDK_ARCH ?= X64

ifeq ($(OS),Windows_NT)
	DIST ?= Windows
else
	DIST ?= $(shell uname)
endif

CFLAGS    = -c -fshort-wchar -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter
CFLAGS   += -I../../User/Include
CFLAGS   += -I../../Include/Acidanthera -I../../Include/Apple -I../../Include/Generic -I../../Include/Intel

ifeq ($(DIST),Darwin)
	CFLAGS  += -mmacosx-version-min=10.6
	LDFLAGS += -mmacosx-version-min=10.6
endif

ifeq ($(DIST),Windows)
	SUFFIX  := .exe
endif

ifeq ($(SANITIZE),1)
	CFLAGS  += -fsanitize=undefined,address
	LDFLAGS += -fsanitize=undefined,address
endif

ifeq ($(DEBUG),1)
	CFLAGS  += -g -O0
	STRIP   := @echo No strip in DEBUG mode
else
	CFLAGS  += -O3
	STRIP   := strip -x
endif

#
# Skip including UDK when suggesting STANDALONE mode.
#
ifneq ($(STANDALONE),1)
	CFLAGS  += -D NO_MSABI_VA_FUNCS -D OC_TARGET_DEBUG
	CFLAGS  += -I$(UDK_PATH)/MdePkg/Include -I$(UDK_PATH)/MdePkg/Include/Library -I$(UDK_PATH)/MdePkg/Include/$(UDK_ARCH)
	CFLAGS  += -I$(UDK_PATH)/UefiCpuPkg/Include
	#
	# Compatibility headers.
	#
	CFLAGS  += -include ../../User/Include/Pcd.h -include ../../User/Include/EfiVar.h
	#
	# UDK implementations.
	#
	OBJS    += UefiLibPrint.o CpuDeadLoop.o BaseDebugPrintErrorLevelLib.o DebugLib.o PrintLib.o PrintLibInternal.o String.o SafeString.o SwapBytes16.o SwapBytes32.o LinkedList.o
	#
	# Customised/Simplified implementations at userspace level.
	#
	OBJS    += BaseMemoryLib.o BootServices.o DebugBreak.o EfiVar.o UserMath.o Pcd.o
	#
	# OcGuardLib targets.
	#
	OBJS    += BitOverflow.o NativeOverflow.o TripleOverflow.o

	#
	# Add source searchpath for transparent compilation.
	# This way make will find any file in path in VPATH and apply "%.o: %.c" rule.
	#
	VPATH += :$(UDK_PATH)/MdePkg/Library/UefiLib:$\
		$(UDK_PATH)/MdePkg/Library/BaseLib:$\
		$(UDK_PATH)/MdePkg/Library/BaseDebugPrintErrorLevelLib:$\
		$(UDK_PATH)/MdePkg/Library/UefiDebugLibConOut:$\
		$(UDK_PATH)/MdePkg/Library/BasePrintLib:$\
		$(UDK_PATH)/MdePkg/Library/BasePrintLib:$\
		../../User/Library:$\
		../../Library/OcGuardLib
endif

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)
	$(STRIP) $(PRODUCT)

clean:
	rm -rf *.o *.dSYM $(PRODUCT)
