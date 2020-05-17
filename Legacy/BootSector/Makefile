NASM ?= nasm
MKDIR ?= mkdir
RM ?= /bin/rm
PAGE_TABLE ?= 0x60000

BINDIR=bin
PRODUCTS32=$(BINDIR)/StartIA32.com $(BINDIR)/EfiIA32.com
PRODUCTS64=$(BINDIR)/StartX64_$(PAGE_TABLE).com $(BINDIR)/EfiX64.com

all: $(PRODUCTS32) $(PRODUCTS64)

clean:
	$(RM) -f $(BINDIR)/*.com

$(BINDIR)/StartIA32.com: start.nasm
	@$(MKDIR) -p bin
	$(NASM) -f bin -o $@ $<

$(BINDIR)/EfiIA32.com: efi32.nasm
	@$(MKDIR) -p bin
	$(NASM) -f bin -o $@ $<

$(BINDIR)/StartX64_$(PAGE_TABLE).com: start.nasm
	@$(MKDIR) -p bin
	$(NASM) -DX64 -DPAGE_TABLE=$(PAGE_TABLE) -f bin -o $@ $<

$(BINDIR)/EfiX64.com: efi64.nasm
	@$(MKDIR) -p bin
	$(NASM) -f bin -o $@ $<
