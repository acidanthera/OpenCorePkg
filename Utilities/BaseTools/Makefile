DIST ?= $(shell uname)
CC ?= gcc
ifeq ($(DIST),Darwin)
	CFLAGS ?= -O3
else
	CFLAGS ?= -O3 -s
endif
MKDIR ?= mkdir
RM ?= /bin/rm

ifeq ($(DIST),Darwin)
	CFLAGS += -arch x86_64 -arch arm64 -mmacosx-version-min=10.9
endif

BINDIR=bin.$(DIST)
PRODUCTS=$(BINDIR)/GenPage $(BINDIR)/EfiLdrImage

all: $(PRODUCTS)

clean:
	$(RM) -f $(PRODUCTS)

$(BINDIR)/GenPage: GenPage.c VirtualMemory.h
	@$(MKDIR) -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BINDIR)/EfiLdrImage: EfiLdrImage.c
	@$(MKDIR) -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<
