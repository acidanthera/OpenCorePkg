STANDALONE     = 1

include ../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = macserial$(SUFFIX)

OBJS          += macserial.o

ifeq ($(DIST),Darwin)
	LDFLAGS += -Wl,-framework,IOKit -Wl,-framework,CoreFoundation
endif

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)
	$(STRIP) $(PRODUCT)

clean:
	rm -rf *.o $(PRODUCT)
