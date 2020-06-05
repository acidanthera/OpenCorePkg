## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = Bmf
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o
#
# From OpenCanopy.
#
OBJS   += BitmapFont.o OpenCanopy.o InputSimTextIn.o InputSimAbsPtr.o OutputStGop.o BootPicker.o GuiApp.o
#
# From OpenCore.
#
OBJS   += OcPng.o lodepng.o OcCompressionLib.o OcTimerLib.o OcAppleKeyMapLib.o HotKeySupport.o BootArguments.o BootEntryInfo.o OcAppleBootPolicyLib.o OcDevicePathLib.o DebugPrint.o GetFileInfo.o GetVolumeLabel.o ReadFile.o OpenFile.o FileProtocol.o OcStorageLib.o

VPATH   = ../../Platform/OpenCanopy:$\
          ../../Platform/OpenCanopy/Input:$\
          ../../Platform/OpenCanopy/Output:$\
          ../../Platform/OpenCanopy/Views:$\
          ../../Library/OcPngLib:$\
          ../../Library/OcCompressionLib:$\
          ../../Library/OcTimerLib:$\
          ../../Library/OcAppleKeyMapLib:$\
          ../../Library/OcBootManagementLib:$\
          ../../Library/OcAppleBootPolicyLib:$\
          ../../Library/OcDevicePathLib:$\
          ../../Library/OcStringLib:$\
          ../../Library/OcDebugLogLib:$\
          ../../Library/OcFileLib:$\
          ../../Library/OcStorageLib:$\
          ../../Library/OcTemplateLib

include ../../User/Makefile

CFLAGS += -I../../Platform/OpenCanopy
