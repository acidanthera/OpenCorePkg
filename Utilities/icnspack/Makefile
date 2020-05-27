STANDALONE     = 1

CURR_DIR = $(shell pwd)

include $(CURR_DIR)/../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = icnspack

OBJS          += icnspack.o

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)

clean:
	rm -rf *.o $(PRODUCT)
