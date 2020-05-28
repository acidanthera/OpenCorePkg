STANDALONE     = 1

include ../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = macserial

OBJS          += macserial.o

ifeq ($(DIST),Darwin)
	LDFLAGS += -Wl,-framework,IOKit -Wl,-framework,CoreFoundation
endif

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)
	@if [ "${STRIP}" = "1" ]; then\
		echo "Stripping binary on non-DEBUG mode";\
		strip -x "${PRODUCT}";\
	fi

clean:
	rm -rf *.o $(PRODUCT)
