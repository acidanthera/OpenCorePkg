## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

include ../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = ConfigValidity

OBJS          += ConfigValidity.o 
OBJS          += OcConfigurationLib.o OcTemplateLib.o OcSerializeLib.o OcXmlLib.o OcAsciiLib.o

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)
	$(STRIP) $(PRODUCT)

OcConfigurationLib.o:
	$(CC) $(CFLAGS) ../../Library/OcConfigurationLib/OcConfigurationLib.c -o $@

OcTemplateLib.o:
	$(CC) $(CFLAGS) ../../Library/OcTemplateLib/OcTemplateLib.c -o $@

OcSerializeLib.o:
	$(CC) $(CFLAGS) ../../Library/OcSerializeLib/OcSerializeLib.c -o $@

OcXmlLib.o:
	$(CC) $(CFLAGS) ../../Library/OcXmlLib/OcXmlLib.c -o $@

OcAsciiLib.o:
	$(CC) $(CFLAGS) ../../Library/OcStringLib/OcAsciiLib.c -o $@

clean:
	rm -rf *.o *.dSYM $(PRODUCT)
