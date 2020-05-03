CC ?= gcc
CFLAGS=-c -Wall -Wextra -pedantic -O3
LDFLAGS ?=

ifneq ($(OS),Windows_NT)
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		CFLAGS+=-mmacosx-version-min=10.6
		LDFLAGS+=-Wl,-framework,CoreFoundation -Wl,-framework,CoreGraphics -Wl,-framework,CoreText -mmacosx-version-min=10.6
	endif
endif

all: disklabel

disklabel: disklabel.o
	$(CC) $(LDFLAGS) disklabel.o -o disklabel

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o disklabel
