VSRC	= ../..
PSRC	= ../src

SRCS	= $(MAIN) \
		$(PSRC)/tstmain.c \
		$(PSRC)/fsw_posix.c \
		\
		$(VSRC)/fsw_core.c \
		$(VSRC)/fsw_lib.c \
		$(VSRC)/fsw_hfs.c

HFLAGS	= \
	-DHOST_MSWIN \
	-DFSTYPE=hfs \
	-DVBOXHFS_BTREE_BINSEARCH \
	-DFSW_DEBUG_LEVEL=3 \
	-DITERATIONS=1 \
	-DFSW_DNODE_CACHE_SIZE=7 \


WFLAGS	= \
	-Dmode_t=int \
	-Doff_t=int64 \
	-Dssize_t=long \
	-Dbswap16=_byteswap_ushort \
	-Dbswap32=_byteswap_ulong \
	-Dbswap64=_byteswap_uint64 \
	-D_CRT_SECURE_NO_WARNINGS \
	-DWIN32 \
	-D_DEBUG \
	-D_CONSOLE \


CFLAGS	= $(WFLAGS) $(HFLAGS)

hfstest:	$(SRCS)
	$(CC) -I. -I$(PSRC) -I$(VSRC) $(CFLAGS) $(SRCS)

clean:
	del /f *.obj *.exe
