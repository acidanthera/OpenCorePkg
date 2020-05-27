## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

CURR_DIR = $(shell pwd)

include $(CURR_DIR)/../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = ConfigValidity

OBJS          += ConfigValidity.o 
OBJS          += OcConfigurationLib.o OcTemplateLib.o OcSerializeLib.o OcXmlLib.o OcAsciiLib.o

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)

OcConfigurationLib.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcConfigurationLib/OcConfigurationLib.c -o $@

OcTemplateLib.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcTemplateLib/OcTemplateLib.c -o $@

OcSerializeLib.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcSerializeLib/OcSerializeLib.c -o $@

OcXmlLib.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcXmlLib/OcXmlLib.c -o $@

OcAsciiLib.o:
	$(CC) $(CFLAGS) $(CURR_DIR)/../../Library/OcStringLib/OcAsciiLib.c -o $@

clean:
	rm -rf *.o *.dSYM $(PRODUCT)
