## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

.DEFAULT_GOAL := all

.ONESHELL:

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
SHARED_CFLAGS := -c -fshort-wchar -Wall -Wextra -D EFIUSER

ifeq ($(STATIC),1)
	LDFLAGS += -static
endif

ifeq ($(WERROR),1)
	SHARED_CFLAGS += -Werror
endif

#
# On Windows, output file cannot be null.
# Ref: https://github.com/msys2/MINGW-packages/issues/10520
#
ifeq ($(DIST),Windows)
	OUT_BIN := /tmp/windows_out
else
	OUT_BIN := /dev/null
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-unused-parameter -x c -c - -o "${OUT_BIN}" 2>&1),)
	SHARED_CFLAGS += -Wno-unused-parameter
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-implicit-fallthrough -x c -c - -o "${OUT_BIN}" 2>&1),)
	SHARED_CFLAGS += -Wno-implicit-fallthrough
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-strict-aliasing -x c -c - -o "${OUT_BIN}" 2>&1),)
	SHARED_CFLAGS += -Wno-strict-aliasing
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-address -x c -c - -o "${OUT_BIN}" 2>&1),)
	SHARED_CFLAGS += -Wno-address
endif

ifeq ($(shell echo 'int a;' | "${CC}" -Wno-stringop-overflow -x c -c - -o "${OUT_BIN}" 2>&1),)
	SHARED_CFLAGS += -Wno-stringop-overflow
endif

ifeq ($(DIST),Darwin)
	ifneq ($(FUZZ),1)
		ifeq ($(FATBIN32),1)
			SHARED_CFLAGS	+= -mmacosx-version-min=10.6 -arch x86_64 -arch i386
			LDFLAGS		+= -mmacosx-version-min=10.6 -arch x86_64 -arch i386
		else
			SHARED_CFLAGS	+= -mmacosx-version-min=10.9 -arch x86_64 -arch arm64
			LDFLAGS		+= -mmacosx-version-min=10.9 -arch x86_64 -arch arm64
		endif
	endif
endif

ifeq ($(DIST),Windows)
	SUFFIX		:= .exe
	SHARED_CFLAGS	+= -D_ISOC99_SOURCE=1
endif

ifeq ($(SANITIZE),1)
	SHARED_CFLAGS	+= -fsanitize=undefined,address -D SANITIZE_TEST
	LDFLAGS		+= -fsanitize=undefined,address
endif

ifeq ($(FUZZ),1)
	SHARED_CFLAGS	+= -fsanitize=fuzzer -D FUZZING_TEST -D ENTRY_POINT=no_main
	LDFLAGS		+= -fsanitize=fuzzer
else
	SHARED_CFLAGS	+= -D ENTRY_POINT=main
endif

ifeq ($(FUZZ_DIR),)
	FUZZ_DIR = FUZZDICT
endif

ifeq ($(FUZZ_JOBS),)
	FUZZ_JOBS = 2
endif

ifeq ($(FUZZ_SECURITY_JOBS),)
	FUZZ_SECURITY_JOBS = 4
endif

ifeq ($(FUZZ_MEM),)
	FUZZ_MEM = 8192
endif

ifeq ($(COVERAGE),1)
	SHARED_CFLAGS += --coverage -D COVERAGE_TEST
	ifeq ($(DIST),Darwin)
	  LDFLAGS += --coverage
	else
	  LDFLAGS += --coverage -Wl,-wrap,llvm_gcda_emit_arcs
	endif
endif

ifeq ($(DEBUG),1)
	SHARED_CFLAGS += -g -O0 -D EFIUSER_DEBUG
	#
	# Forcibly disable stripping when enabling DEBUG mode.
	#
	STRIP   := @echo No strip in DEBUG mode
	STRIPFLAGS :=
else
	SHARED_CFLAGS += -O3
	#
	# Default strip command with overriding (e.g. i686-w64-mingw32-strip) allowed.
	#
	STRIP   ?= strip
	STRIPFLAGS ?= -x
endif

ifeq ($(SYDR),1)
	INFIX := .sydr
endif

#
# Search Paths.
#
SHARED_CFLAGS += -I$(OC_USER)/User/Include
SHARED_CFLAGS += -I$(OC_USER)/Include/Acidanthera -I$(OC_USER)/Include/Apple -I$(OC_USER)/Include/Apple/$(UDK_ARCH) -I$(OC_USER)/Include/Duet -I$(OC_USER)/Include/Generic -I$(OC_USER)/Include/Intel -I$(OC_USER)/Include/Microsoft -I$(OC_USER)/Include/Nvidia

#
# Skip including UDK when suggesting STANDALONE mode.
#
ifneq ($(STANDALONE),1)
	SHARED_CFLAGS += -D NO_MSABI_VA_FUNCS -D OC_TARGET_DEBUG
	SHARED_CFLAGS += -I$(UDK_PATH)/MdePkg/Include -I$(UDK_PATH)/MdePkg/Include/Library -I$(UDK_PATH)/MdePkg/Include/$(UDK_ARCH) -I$(UDK_PATH)/MdePkg/Library/BaseLib
	SHARED_CFLAGS += -I$(UDK_PATH)/MdeModulePkg/Include
	SHARED_CFLAGS += -I$(UDK_PATH)/UefiCpuPkg/Include
	#
	# Compatibility headers.
	#
	SHARED_CFLAGS += -include $(OC_USER)/User/Include/UserPcd.h -include $(OC_USER)/User/Include/UserGlobalVar.h
	#
	# UDK implementations.
	#
	SHARED_OBJS += UefiLib.o UefiLibPrint.o CpuDeadLoop.o BaseDebugPrintErrorLevelLib.o DebugLib.o PrintLib.o PrintLibInternal.o String.o SafeString.o SwapBytes16.o SwapBytes32.o LinkedList.o HighBitSet32.o HighBitSet64.o MtrrLib.o GetPowerOfTwo32.o GetPowerOfTwo64.o Cpu.o BmpSupportLib.o SafeIntLib.o X86GetInterruptState.o PciLib.o PciExpressLib.o DevicePathUtilities.o UefiDevicePathLib.o DevicePathToText.o DevicePathFromText.o BitField.o CheckSum.o UefiSortLib.o QuickSort.o UserFile.o CommonMemoryAllocationLib.o CommonMemoryAllocationLibEx.o BaseMemoryProfileLibNull.o
	ifeq ($(UDK_ARCH),X64)
		SHARED_OBJS += TdProbe.o
	else
	  SHARED_OBJS += IntelTdxNull.o
	endif
	#
	# Customised/Simplified implementations at userspace level.
	#
	SHARED_OBJS += UserBaseMemoryLib.o UserBootServices.o UserGlobalVar.o UserMath.o UserMisc.o UserPcd.o UserUnicodeCollation.o UserOcDummy.o UserEvent.o
	#
	# BaseOverflowLib targets.
	#
	SHARED_OBJS += BaseMath.o BaseBitOverflow.o BaseNativeOverflow.o BaseTripleOverflow.o BaseAlignment.o
	#
	# OcSerializeLib targets.
	#
	SHARED_OBJS += OcSerializeLib.o
	#
	# OcTemplateLib targets.
	#
	SHARED_OBJS += OcTemplateLib.o
	#
	# OcXmlLib targets.
	#
	SHARED_OBJS += OcXmlLib.o
	#
	# OcStringLib targets.
	#
	SHARED_OBJS += OcAsciiLib.o OcUnicodeLib.o
	#
	# OcCryptoLib targets.
	#
	SHARED_OBJS += RsaDigitalSign.o BigNumMontgomery.o BigNumPrimitives.o BigNumWordMul64.o Sha2.o SecureMem.o Sha512AccelDummy.o
	#
	# OcMachoLib targets.
	#
	SHARED_OBJS += CxxSymbols.o MachoFat.o Header.o Macho32.o Macho64.o Relocations.o Symbols.o
	#
	# OcAppleKeysLib targets.
	#
	SHARED_OBJS += OcAppleKeysLib.o
	#
	# OcCpuLib targets.
	#
	SHARED_OBJS += FrequencyDetect.o AppleCpuSupport.o OcCpuLib.o MeasureTicks.o
	#
	# OcMiscLib targets.
	#
	SHARED_OBJS += ProtocolSupport.o DataPatcher.o PlatformInfo.o
	#
	# OcAppleKernelLib targets.
	#
	SHARED_OBJS += KernelVersion.o
	#
	# OcUnicodeCollationEngLib targets.
	#
	SHARED_OBJS += OcUnicodeCollationEngCommon.o
	#
	# OcVariableLib targets.
	#
	SHARED_OBJS += OcVariableLib.o

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
				$(UDK_PATH)/MdePkg/Library/BaseOverflowLib:$\
				$(UDK_PATH)/MdePkg/Library/BaseSafeIntLib:$\
				$(UDK_PATH)/MdePkg/Library/BasePciLibPciExpress:$\
				$(UDK_PATH)/MdePkg/Library/BasePciExpressLib:$\
				$(UDK_PATH)/MdePkg/Library/UefiDevicePathLib:$\
				$(UDK_PATH)/MdeModulePkg/Library/BaseBmpSupportLib:$\
				$(UDK_PATH)/MdeModulePkg/Library/BaseMemoryProfileLibNull:$\
				$(UDK_PATH)/MdeModulePkg/Library/CommonMemoryAllocationLib:$\
				$(UDK_PATH)/MdeModulePkg/Library/UefiSortLib:$\
				$(UDK_PATH)/UefiCpuPkg/Library/MtrrLib:$\
				$(OC_USER)/Library/OcSerializeLib:$\
				$(OC_USER)/Library/OcTemplateLib:$\
				$(OC_USER)/Library/OcXmlLib:$\
				$(OC_USER)/Library/OcStringLib:$\
				$(OC_USER)/Library/OcCryptoLib:$\
				$(OC_USER)/Library/OcCryptoLib/Cpu32:$\
				$(OC_USER)/Library/OcMachoLib:$\
				$(OC_USER)/Library/OcAppleKeysLib:$\
				$(OC_USER)/Library/OcCpuLib:$\
				$(OC_USER)/Library/OcCpuLib/Ia32:$\
				$(OC_USER)/Library/OcMiscLib:$\
				$(OC_USER)/Library/OcAppleKernelLib:$\
				$(OC_USER)/Library/OcUnicodeCollationEngLib:$\
				$(OC_USER)/Library/OcVariableLib
endif

#
# Miscellaneous implementations that do not depend on UDK.
#
VPATH		+= $(OC_USER)/User/Library:$
SHARED_OBJS	+= UserPseudoRandom.o

#
# Project-specific object files.
#
OUT_DIR	:= $(DIST)_$(UDK_ARCH)$(INFIX)
OBJS	:= $(addprefix $(OUT_DIR)/,$(OBJS))

$(OUT_DIR)/%.o: %.c
	@$(MKDIR) $(OUT_DIR)
	$(CC) -MMD -MT $@ -MF $(OUT_DIR)/$*.d $(SHARED_CFLAGS) $(CFLAGS) $< -o $@

DEP := $(OBJS:%.o=%.d)
-include $(DEP)

#
# To avoid recompiling multiple times with the same flags we compile shared
# object files to their own directory.
# Note 1: When not using USE_SHARED_OBJS=1, we specify $(CURDIR)/$(OUT_DIR)
# rather than $(OUT_DIR) to trigger a different rule from that for $(OUT_DIR)
# above.
# Note 2: We do not apply project-specific CFLAGS to SHARED_OBJS, regardless
# of USE_SHARED_OBJS setting. This avoids unexpected differences in behaviour
# between tools built one at a time and tools built in a batch using USE_SHARED_OBJS=1.
# Note 3: USE_SHARED_OBJS=1 does not work if applied to projects which are
# built using separate instances of make running in parallel, because it
# leads to race conditions. This applies to UDK/BaseTools ImageTool and MicroTool.
#
ifeq ($(USE_SHARED_OBJS),1)
	SHARED_DIR	:= ../Shared/$(OUT_DIR)
else
	SHARED_DIR	:= $(CURDIR)/$(OUT_DIR)
endif
SHARED_OBJS	:= $(addprefix $(SHARED_DIR)/,$(SHARED_OBJS))

$(SHARED_DIR)/%.o: %.c
	@$(MKDIR) $(SHARED_DIR)
	$(CC) -MMD -MT $@ -MF $(SHARED_DIR)/$*.d $(SHARED_CFLAGS) $< -o $@

SHARED_DEP := $(SHARED_OBJS:%.o=%.d)
-include $(SHARED_DEP)

all: $(PRODUCT)

$(PRODUCT): $(SHARED_OBJS) $(OBJS)
	$(CC) $(LDFLAGS) $(SHARED_OBJS) $(OBJS) $(LDLIBS) -o $(PRODUCT)
	$(STRIP) $(STRIPFLAGS) $(PRODUCT)

fuzz: $(PRODUCT) FORCE
	@rm -f fuzz*.log
	@$(MKDIR) $(FUZZ_DIR)
	UBSAN_OPTIONS='halt_on_error=1' ./$(PRODUCT) -jobs=$(FUZZ_JOBS) -workers=$(FUZZ_JOBS) -rss_limit_mb=$(FUZZ_MEM) $(FUZZ_DIR)

sydr-fuzz: $(PROJECT).sydr$(SUFFIX) $(PROJECT)$(SUFFIX) FORCE
	@$(MKDIR) $(FUZZ_DIR)
	@rm -rf sydr-fuzz-out
	@cat <<- EOF > sydr-fuzz.toml
		[sydr]
		args = "--solving-timeout 60 -s 900 -j $(FUZZ_JOBS) -m $(FUZZ_MEM)"
		target = "$(PROJECT).sydr$(SUFFIX) @@"
		jobs = $(FUZZ_JOBS)

		[libfuzzer]
		path = "$(PROJECT)$(SUFFIX)"
		args = "-jobs=1000 -workers=$(FUZZ_JOBS) -rss_limit_mb=$(FUZZ_MEM) $(FUZZ_DIR)"
		cmin = false
	EOF
	UBSAN_OPTIONS='halt_on_error=1' sydr-fuzz -l debug run -f

sydr-fuzz-security: $(PROJECT).sydr$(SUFFIX) $(PROJECT)$(SUFFIX) FORCE
	sydr-fuzz -l debug security --jobs $(FUZZ_SECURITY_JOBS)

sydr-fuzz-import: $(PROJECT).sydr$(SUFFIX) $(PROJECT)$(SUFFIX) FORCE
	@$(MKDIR) sydr-fuzz-out/corpus
	@$(MKDIR) sydr-fuzz-out/security
	./$(PROJECT)$(SUFFIX) -merge=1 $(FUZZ_DIR) sydr-fuzz-out/corpus
	./$(PROJECT)$(SUFFIX) -merge=1 $(FUZZ_DIR) sydr-fuzz-out/security

sydr-import-check: $(PRODUCT) FORCE
	@for f in $(FUZZ_DIR)/*; do  ./$(PRODUCT) $$f > /dev/null ; done || true
	@for f in sydr-fuzz-out/corpus/*
	do
		if [ -n "$$(./$(PRODUCT) $$f | grep CoverageHit)" ]; then \
				cp $$f $(FUZZ_DIR)/; \
		fi
	done || true
	@for f in sydr-fuzz-out/security/*
	do
		if [ -n "$$(./$(PRODUCT) $$f | grep CoverageHit)" ]; then \
				cp $$f $(FUZZ_DIR)/; \
		fi
	done || true

coverage: $(PRODUCT) FORCE
	@$(LCOV) --version
	@rm -rf COVERAGE
	@$(MKDIR) COVERAGE
	$(LCOV) --no-checksum --zerocounters --directory .
	$(LCOV) --no-checksum --capture --initial --directory . --output-file COVERAGE/trace.lcov_base
	for f in $(FUZZ_DIR)/*; do  ./$(PRODUCT) $$f > /dev/null ; done || true
	$(LCOV) --no-checksum --capture --directory . --rc lcov_branch_coverage=1  --rc lcov_excl_br_line='LCOV_EXCL_BR_LINE|ASSERT|DEBUG|assert' --output-file COVERAGE/trace.lcov_info || exit 1
	$(LCOV) --no-checksum -a COVERAGE/trace.lcov_base -a COVERAGE/trace.lcov_info --rc lcov_branch_coverage=1 --rc lcov_excl_br_line='LCOV_EXCL_BR_LINE|ASSERT|DEBUG|assert' --output-file COVERAGE/trace.lcov_tmp || exit 1
	$(LCOV) --no-checksum -r COVERAGE/trace.lcov_tmp /usr/include/\* $(LCOV_EXCL) --rc lcov_branch_coverage=1  --rc lcov_excl_br_line='LCOV_EXCL_BR_LINE|ASSERT|DEBUG|assert' --output-file COVERAGE/trace.lcov_info_final || exit 1
	$(GENHTML) --branch-coverage --output-directory COVERAGE COVERAGE/trace.lcov_info_final

clean: FORCE
	rm -rf $(OUT_DIR) $(SHARED_DIR) $(PRODUCT) $(PRODUCT).sydr $(PRODUCT).exe $(PRODUCT).sydr.exe sydr-fuzz.toml

FORCE:
