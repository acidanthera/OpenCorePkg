## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = ocvalidate
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o \
          OcValidateLib.o \
          ValidateAcpi.o \
          ValidateBooter.o \
          ValidateDeviceProperties.o \
          ValidateKernel.o \
          ValidateMisc.o \
          ValidateNVRAM.o \
          ValidatePlatformInfo.o \
          ValidateUEFI.o \
          OcConfigurationLib.o
VPATH   = ../../Library/OcConfigurationLib
include ../../User/Makefile
