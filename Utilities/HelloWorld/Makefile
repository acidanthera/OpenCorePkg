## @file
# Copyright (c) 2020, PMheart. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

include ../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = HelloWorld

OBJS          += main.o

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)

clean:
	rm -rf *.o *.dSYM $(PRODUCT)
