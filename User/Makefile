CC       ?= clang

UDK_PATH ?= ../../UDK
UDK_ARCH ?= X64

CFLAGS   ?= -g -c -fshort-wchar -D NO_MSABI_VA_FUNCS
CFLAGS   += -I$(UDK_PATH)/MdePkg/Include -I$(UDK_PATH)/MdePkg/Include/Library -I$(UDK_PATH)/MdePkg/Include/$(UDK_ARCH)
CFLAGS   += -I../../User/Include
CFLAGS   += -I../../Include/Acidanthera -I../../Include/Apple -I../../Include/Generic
CFLAGS   += -include ../../User/Include/Pcd.h -include ../../User/Include/EfiVar.h

OBJS     ?= UefiLibPrint.o CpuDeadLoop.o BaseDebugPrintErrorLevelLib.o DebugLib.o DebugLibConstructor.o PrintLib.o PrintLibInternal.o String.o SafeString.o SwapBytes16.o SwapBytes32.o
OBJS     += BaseMemoryLib.o BootServices.o DebugBreak.o EfiVar.o Math.o Pcd.o

LDFLAGS  ?= -fsanitize=undefined,address

UefiLibPrint.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/UefiLib/UefiLibPrint.c -o $@

CpuDeadLoop.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/BaseLib/CpuDeadLoop.c -o $@

BaseDebugPrintErrorLevelLib.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.c -o $@

DebugLib.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/UefiDebugLibConOut/DebugLib.c -o $@

DebugLibConstructor.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/UefiDebugLibConOut/DebugLibConstructor.c -o $@

PrintLib.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/BasePrintLib/PrintLib.c -o $@

PrintLibInternal.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/BasePrintLib/PrintLibInternal.c -o $@

String.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/BaseLib/String.c -o $@

SafeString.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/BaseLib/SafeString.c -o $@

SwapBytes16.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/BaseLib/SwapBytes16.c -o $@

SwapBytes32.o:
	$(CC) $(CFLAGS) $(UDK_PATH)/MdePkg/Library/BaseLib/SwapBytes32.c -o $@

BaseMemoryLib.o:
	$(CC) $(CFLAGS) ../../User/Library/BaseMemoryLib.c -o $@

BootServices.o:
	$(CC) $(CFLAGS) ../../User/Library/BootServices.c -o $@

DebugBreak.o:
	$(CC) $(CFLAGS) ../../User/Library/DebugBreak.c -o $@

EfiVar.o:
	$(CC) $(CFLAGS) ../../User/Library/EfiVar.c -o $@

Math.o:
	$(CC) $(CFLAGS) ../../User/Library/Math.c -o $@

Pcd.o:
	$(CC) $(CFLAGS) ../../User/Library/Pcd.c -o $@

.c:
	$(CC) $(CFLAGS) ..< -o $@