DESTDIR ?= bin
BOOTSECTORS = boot0 boot1f32
MKDIR ?= mkdir

BOOTSECTOR_SRCS = $(addsuffix .nasm, $(BOOTSECTORS))
BOOTSECTOR_BINS = $(addprefix $(DESTDIR)/, $(BOOTSECTORS))

ifdef NASM_PREFIX
NASM=$(NASM_PREFIX)nasm
else
NASM=nasm
endif

all: $(BOOTSECTOR_BINS)

$(BOOTSECTOR_BINS): $(BOOTSECTOR_SRCS)
	@$(MKDIR) -p bin
	@echo "[NASM] $(@F).nasm -> $@"
	@"$(NASM)" $(@F).nasm -f bin -o $@

clean:
	rm -f $(BOOTSECTOR_BINS) *~
