## @file
# Copyright (c) 2021, vit9696. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

PROJECT = Mp3
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o \
	OcMp3Lib.o \
	bitstream.o \
	buffers.o \
	dct32.o \
	dequant.o \
	dqchan.o \
	huffman.o \
	hufftabs.o \
	imdct.o \
	mp3dec.o \
	mp3tabs.o \
	polyphase.o \
	scalfact.o \
	stproc.o \
	subband.o \
	trigtabs.o
VPATH   = ../../Library/OcMp3Lib:$\
	../../Library/OcMp3Lib/helix
include ../../User/Makefile
ifneq ($(SANITIZE),)
	CFLAGS += -fno-sanitize=shift
endif