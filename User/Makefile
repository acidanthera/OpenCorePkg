## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

.DEFAULT_GOAL := all

LCOV          ?= lcov
GENHTML       ?= genhtml
CC            ?= gcc
MKDIR         := mkdir -p

ifeq ($(UDK_PATH),)
	ifneq ($(PACKAGES_PATH),)
		UDK_PATH := $(PACKAGES_PATH)
	else
		UDK_PATH ?= ../../UDK
	endif
endif

ifeq ($(OC_USER),)
	OC_USER := ../..
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

ifeq ($(STATIC),1)
	LDFLAGS += -static
endif

ifeq ($(WERROR),1)
	CFLAGS += -Werror
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-unused-parameter -x c -c - -o /dev/null 2>&1),)
	CFLAGS += -Wno-unused-parameter
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-implicit-fallthrough -x c -c - -o /dev/null 2>&1),)
	CFLAGS += -Wno-implicit-fallthrough
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-strict-aliasing -x c -c - -o /dev/null 2>&1),)
	CFLAGS += -Wno-strict-aliasing
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-address -x c -c - -o /dev/null 2>&1),)
	CFLAGS += -Wno-address
endif

ifeq ($(DIST),Darwin)
	ifneq ($(FUZZ),1)
		CFLAGS  += -mmacosx-version-min=10.6 --target=x86_64-apple-darwin
		LDFLAGS += -mmacosx-version-min=10.6 --target=x86_64-apple-darwin
	endif
endif

ifeq ($(DIST),Windows)
	SUFFIX := .exe
	CFLAGS += -D_ISOC99_SOURCE=1
endif

ifeq ($(SANITIZE),1)
	CFLAGS  += -fsanitize=undefined,address -D SANITIZE_TEST
	LDFLAGS += -fsanitize=undefined,address
endif

ifeq ($(FUZZ),1)
	CFLAGS  += -fsanitize=fuzzer -D FUZZING_TEST -D ENTRY_POINT=no_main
	LDFLAGS += -fsanitize=fuzzer
else
	CFLAGS  += -D ENTRY_POINT=main
endif

ifeq ($(FUZZ_JOBS),)
	FUZZ_JOBS = 4
endif

ifeq ($(FUZZ_MEM),)
	FUZZ_MEM = 4096
endif

ifeq ($(COVERAGE),1)
	CFLAGS  += -fprofile-arcs -ftest-coverage -D COVERAGE_TEST
	LDFLAGS += --coverage
endif

ifeq ($(DEBUG),1)
	CFLAGS  += -g -O0 -D EFIUSER_DEBUG
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
CFLAGS += -I$(OC_USER)/User/Include
CFLAGS += -I$(OC_USER)/Include/Acidanthera -I$(OC_USER)/Include/Apple -I$(OC_USER)/Include/Apple/$(UDK_ARCH) -I$(OC_USER)/Include/Generic -I$(OC_USER)/Include/Intel -I$(OC_USER)/Include/Microsoft -I$(OC_USER)/Include/Nvidia

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
	CFLAGS  += -include $(OC_USER)/User/Include/UserPcd.h -include $(OC_USER)/User/Include/UserGlobalVar.h
	#
	# UDK implementations.
	#
	OBJS    += UefiLib.o UefiLibPrint.o CpuDeadLoop.o BaseDebugPrintErrorLevelLib.o DebugLib.o PrintLib.o PrintLibInternal.o String.o SafeString.o SwapBytes16.o SwapBytes32.o LinkedList.o HighBitSet32.o HighBitSet64.o MtrrLib.o GetPowerOfTwo32.o GetPowerOfTwo64.o Cpu.o BmpSupportLib.o SafeIntLib.o X86GetInterruptState.o PciLib.o PciExpressLib.o DevicePathUtilities.o UefiDevicePathLib.o DevicePathToText.o DevicePathFromText.o BitField.o CheckSum.o
	#
	# Customised/Simplified implementations at userspace level.
	#
	OBJS    += UserBaseMemoryLib.o UserBootServices.o UserGlobalVar.o UserMath.o UserMisc.o UserPcd.o UserOcDummy.o
	#
	# OcGuardLib targets.
	#
	OBJS    += BitOverflow.o NativeOverflow.o TripleOverflow.o Alignment.o
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
	OBJS    += RsaDigitalSign.o BigNumMontgomery.o BigNumPrimitives.o BigNumWordMul64.o Sha2.o SecureMem.o Sha512AvxDummy.o
	#
	# OcMachoLib targets.
	#
	OBJS    += CxxSymbols.o Fat.o Header.o Macho32.o Macho64.o Relocations.o Symbols.o
	#
	# OcAppleKeysLib targets.
	#
	OBJS    += OcAppleKeysLib.o
	#
	# OcCpuLib targets.
	#
	OBJS    += FrequencyDetect.o AppleCpuSupport.o OcCpuLib.o MeasureTicks.o
	#
	# OcMiscLib targets.
	#
	OBJS    += Math.o ProtocolSupport.o DataPatcher.o PlatformInfo.o
	#
	# OcAppleKernelLib targets.
	#
	OBJS    += KernelVersion.o

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
				$(OC_USER)/Library/OcGuardLib:$\
				$(OC_USER)/Library/OcSerializeLib:$\
				$(OC_USER)/Library/OcTemplateLib:$\
				$(OC_USER)/Library/OcXmlLib:$\
				$(OC_USER)/Library/OcStringLib:$\
				$(OC_USER)/Library/OcCryptoLib:$\
				$(OC_USER)/Library/OcCryptoLib/$(UDK_ARCH):$\
				$(OC_USER)/Library/OcMachoLib:$\
				$(OC_USER)/Library/OcAppleKeysLib:$\
				$(OC_USER)/Library/OcCpuLib:$\
				$(OC_USER)/Library/OcCpuLib/Ia32:$\
				$(OC_USER)/Library/OcMiscLib:$\
				$(OC_USER)/Library/OcAppleKernelLib
endif

#
# Miscellaneous implementations that do not depend on UDK.
#
VPATH   += $(OC_USER)/User/Library:$
OBJS    += UserFile.o UserPseudoRandom.o

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

fuzz: $(PRODUCT) FORCE
	@rm -f fuzz*.log
	@$(MKDIR) FUZZDICT
	UBSAN_OPTIONS='halt_on_error=1' ./$(PRODUCT) -jobs=$(FUZZ_JOBS) -workers=$(FUZZ_JOBS) -rss_limit_mb=$(FUZZ_MEM) FUZZDICT

coverage: $(PRODUCT) FORCE
	@$(LCOV) --version
	@rm -rf COVERAGE
	@$(MKDIR) COVERAGE
	$(LCOV) --no-checksum --zerocounters --directory .
	$(LCOV) --no-checksum --capture --initial --directory . --output-file COVERAGE/trace.lcov_base
	for f in FUZZDICT/*; do  ./$(PRODUCT) $$f ; done || true
	$(LCOV) --no-checksum --capture --directory . --rc lcov_branch_coverage=1  --rc lcov_excl_br_line='LCOV_EXCL_BR_LINE|ASSERT' --output-file COVERAGE/trace.lcov_info || exit 1
	$(LCOV) --no-checksum -a COVERAGE/trace.lcov_base -a COVERAGE/trace.lcov_info --rc lcov_branch_coverage=1 --rc lcov_excl_br_line='LCOV_EXCL_BR_LINE|ASSERT' --output-file COVERAGE/trace.lcov_tmp || exit 1
	$(LCOV) --no-checksum -r COVERAGE/trace.lcov_tmp /usr/include/\* --rc lcov_branch_coverage=1  --rc lcov_excl_br_line='LCOV_EXCL_BR_LINE|ASSERT' --output-file COVERAGE/trace.lcov_info_final || exit 1
	$(GENHTML) --branch-coverage --output-directory COVERAGE COVERAGE/trace.lcov_info_final

clean: FORCE
	rm -rf $(OUT_DIR) $(PRODUCT) $(PRODUCT).exe

FORCE:
