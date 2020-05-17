CC ?= gcc
CFLAGS ?= -O3 -s
MKDIR ?= mkdir
RM ?= /bin/rm

ifneq ($(OS),Windows_NT)
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		CFLAGS+=-mmacosx-version-min=10.6
	endif
endif

BINDIR=bin.$(shell uname)
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
