## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

.DEFAULT_GOAL := all

CC            ?= gcc
MKDIR         := mkdir -p

ifeq ($(UDK_PATH),)
	ifneq ($(PACKAGES_PATH),)
		UDK_PATH := $(PACKAGES_PATH)
	else
		UDK_PATH ?= ../../UDK
	endif
endif

ifeq ($(OS),Windows_NT)
	DIST ?= Windows
else
	DIST ?= $(shell uname)
endif

#
# Primary architecture. 
#
UDK_ARCH ?= X64

#
# Primary CFLAGS.
#
CFLAGS   := -c -fshort-wchar -Wall -Wextra -D EFIUSER

ifeq ($(WERROR),1)
	CFLAGS  += -Werror
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-unused-parameter -x c -c - -o /dev/null 2>&1),)
	CFLAGS  += -Wno-unused-parameter
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-implicit-fallthrough -x c -c - -o /dev/null 2>&1),)
	CFLAGS  += -Wno-implicit-fallthrough
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-strict-aliasing -x c -c - -o /dev/null 2>&1),)
	CFLAGS  += -Wno-strict-aliasing
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-address -x c -c - -o /dev/null 2>&1),)
	CFLAGS  += -Wno-address
endif

ifeq ($(DIST),Darwin)
	CFLAGS  += -mmacosx-version-min=10.6
	LDFLAGS += -mmacosx-version-min=10.6
endif

ifeq ($(DIST),Windows)
	SUFFIX  := .exe
	CFLAGS  += -D_ISOC99_SOURCE=1
endif

ifeq ($(SANITIZE),1)
	CFLAGS  += -fsanitize=undefined,address
	LDFLAGS += -fsanitize=undefined,address
endif

ifeq ($(DEBUG),1)
	CFLAGS  += -g -O0
	#
	# Forcibly disable stripping when enabling DEBUG mode.
	#
	STRIP   := @echo No strip in DEBUG mode
	STRIPFLAGS :=
else
	CFLAGS  += -O3
	#
	# Default strip command with overriding (e.g. i686-w64-mingw32-strip) allowed.
	#
	STRIP   ?= strip
	STRIPFLAGS ?= -x
endif

#
# Search Paths.
#
CFLAGS += -I../../User/Include
CFLAGS += -I../../Include/Acidanthera -I../../Include/Apple -I../../Include/Apple/$(UDK_ARCH) -I../../Include/Generic -I../../Include/Intel -I../../Include/Microsoft

#
# Skip including UDK when suggesting STANDALONE mode.
#
ifneq ($(STANDALONE),1)
	CFLAGS  += -D NO_MSABI_VA_FUNCS -D OC_TARGET_DEBUG
	CFLAGS  += -I$(UDK_PATH)/MdePkg/Include -I$(UDK_PATH)/MdePkg/Include/Library -I$(UDK_PATH)/MdePkg/Include/$(UDK_ARCH) -I$(UDK_PATH)/MdePkg/Library/BaseLib
	CFLAGS  += -I$(UDK_PATH)/MdeModulePkg/Include
	CFLAGS  += -I$(UDK_PATH)/UefiCpuPkg/Include
	#
	# Compatibility headers.
	#
	CFLAGS  += -include ../../User/Include/Pcd.h -include ../../User/Include/GlobalVar.h
	#
	# UDK implementations.
	#
	OBJS    += UefiLib.o UefiLibPrint.o CpuDeadLoop.o BaseDebugPrintErrorLevelLib.o DebugLib.o PrintLib.o PrintLibInternal.o String.o SafeString.o SwapBytes16.o SwapBytes32.o LinkedList.o HighBitSet32.o HighBitSet64.o MtrrLib.o GetPowerOfTwo32.o GetPowerOfTwo64.o Cpu.o BmpSupportLib.o SafeIntLib.o X86GetInterruptState.o PciLib.o PciExpressLib.o DevicePathUtilities.o UefiDevicePathLib.o DevicePathToText.o DevicePathFromText.o BitField.o CheckSum.o
	#
	# Customised/Simplified implementations at userspace level.
	#
	OBJS    += BaseMemoryLib.o BootServices.o GlobalVar.o UserMath.o UserMisc.o Pcd.o OcDummy.o
	#
	# OcGuardLib targets.
	#
	OBJS    += BitOverflow.o NativeOverflow.o TripleOverflow.o
	#
	# OcSerializeLib targets.
	#
	OBJS    += OcSerializeLib.o
	#
	# OcTemplateLib targets.
	#
	OBJS    += OcTemplateLib.o
	#
	# OcXmlLib targets.
	#
	OBJS    += OcXmlLib.o
	#
	# OcStringLib targets.
	#
	OBJS    += OcAsciiLib.o OcUnicodeLib.o
	#
	# OcCryptoLib targets.
	#
	OBJS    += RsaDigitalSign.o BigNumMontgomery.o BigNumPrimitives.o BigNumWordMul64.o Sha2.o SecureMem.o
	#
	# OcMachoLib targets.
	#
	OBJS    += CxxSymbols.o Header.o Symbols.o Relocations.o
	#
	# OcAppleKeysLib targets.
	#
	OBJS    += OcAppleKeysLib.o
	#
	# OcCpuLib targets.
	#
	OBJS    += FrequencyDetect.o AppleCpuSupport.o OcCpuLib.o
	#
	# OcMiscLib targets.
	#
	OBJS    += Math.o ProtocolSupport.o

	#
	# Add source searchpath for transparent compilation.
	# This way make will find any file in path in VPATH and apply "%.o: %.c" rule.
	#
	VPATH   += :$(UDK_PATH)/MdePkg/Library/UefiLib:$\
				$(UDK_PATH)/MdePkg/Library/BaseLib:$\
				$(UDK_PATH)/MdePkg/Library/BaseLib/$(UDK_ARCH):$\
				$(UDK_PATH)/MdePkg/Library/BaseDebugPrintErrorLevelLib:$\
				$(UDK_PATH)/MdePkg/Library/UefiDebugLibConOut:$\
				$(UDK_PATH)/MdePkg/Library/BasePrintLib:$\
				$(UDK_PATH)/MdePkg/Library/BaseSafeIntLib:$\
				$(UDK_PATH)/MdePkg/Library/BasePciLibPciExpress:$\
				$(UDK_PATH)/MdePkg/Library/BasePciExpressLib:$\
				$(UDK_PATH)/MdePkg/Library/UefiDevicePathLib:$\
				$(UDK_PATH)/MdeModulePkg/Library/BaseBmpSupportLib:$\
				$(UDK_PATH)/UefiCpuPkg/Library/MtrrLib:$\
				../../Library/OcGuardLib:$\
				../../Library/OcSerializeLib:$\
				../../Library/OcTemplateLib:$\
				../../Library/OcXmlLib:$\
				../../Library/OcStringLib:$\
				../../Library/OcCryptoLib:$\
				../../Library/OcCryptoLib/$(UDK_ARCH):$\
				../../Library/OcMachoLib:$\
				../../Library/OcAppleKeysLib:$\
				../../Library/OcCpuLib:$\
				../../Library/OcMiscLib
endif

#
# Miscellaneous implementations that do not depend on UDK.
#
VPATH   += ../../User/Library:$
OBJS    += File.o

#
# Directory where objects will be produced.
# As well, OBJS will be prepended with actual paths.
#
OUT_DIR := $(DIST)_$(UDK_ARCH)
OBJS    := $(addprefix $(OUT_DIR)/,$(OBJS))

$(OUT_DIR)/%.o: %.c
	@$(MKDIR) $(OUT_DIR)
	$(CC) $(CFLAGS) $< -o $@

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $(PRODUCT)
	$(STRIP) $(STRIPFLAGS) $(PRODUCT)

clean:
	rm -rf $(OUT_DIR) $(PRODUCT) $(PRODUCT).exe
