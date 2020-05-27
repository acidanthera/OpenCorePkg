STANDALONE     = 1

CURR_DIR = $(shell pwd)

include $(CURR_DIR)/../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = disklabel

OBJS          += disklabel.o

ifeq ($(DIST),Darwin)
	LDFLAGS += -Wl,-framework,CoreFoundation -Wl,-framework,CoreGraphics -Wl,-framework,CoreText
endif

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)

clean:
	rm -rf *.o $(PRODUCT)
