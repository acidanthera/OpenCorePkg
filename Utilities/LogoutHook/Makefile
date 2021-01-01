## @file
# Created by Rodion Shynharov on 27.12.20.
# Copyright © 2020 Rodion Shingarev. All rights reserved.
# based on the original Apple NVRAM utility
# Copyright (c) 2000-2016 Apple Inc. All rights reserved.
# https://opensource.apple.com/source/system_cmds/system_cmds-854.40.2/nvram.tproj/nvram.c
##

STANDALONE = 1
PROJECT    = nvramdump
PRODUCT    = $(PROJECT)$(SUFFIX)
OBJS       = $(PROJECT).o
include ../../User/Makefile

ifeq ($(DIST),Darwin)
	LDFLAGS += -Wl,-framework,IOKit -Wl,-framework,CoreFoundation
endif
