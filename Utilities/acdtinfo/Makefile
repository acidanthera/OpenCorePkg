## @file
# Copyright (c) 2019, vit9696. All rights reserved.
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##
STANDALONE = 1
PROJECT    = acdtinfo
PRODUCT    = $(PROJECT)$(SUFFIX)
OBJS       = $(PROJECT).o
include ../../User/Makefile

ifeq ($(DIST),Darwin)
	LDFLAGS += -Wl,-framework,IOKit -Wl,-framework,CoreFoundation
endif
