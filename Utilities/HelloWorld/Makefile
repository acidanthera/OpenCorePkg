include ../../User/Makefile

.DEFAULT_GOAL := all
PRODUCT       ?= HelloWorld

OBJS          += main.o

all: $(PRODUCT)

$(PRODUCT): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PRODUCT)

clean:
	rm -rf *.o *.dSYM $(PRODUCT)
