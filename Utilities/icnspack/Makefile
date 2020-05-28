STANDALONE     = 1

include ../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT        = icnspack

OBJS          += icnspack.o

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)
	$(STRIP) $(PRODUCT)

clean:
	rm -rf *.o $(PRODUCT)
