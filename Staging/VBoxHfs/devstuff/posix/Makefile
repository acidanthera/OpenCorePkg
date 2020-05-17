VSRC	= ../..
PSRC	= ../src

SRCS	= ${MAIN} \
		${PSRC}/tstmain.c \
		${PSRC}/fsw_posix.c \
		\
		${VSRC}/fsw_core.c \
		${VSRC}/fsw_lib.c \
		${VSRC}/fsw_hfs.c

CFLAGS	= \
	-Dbswap16=__builtin_bswap16 \
	-Dbswap32=__builtin_bswap32 \
	-Dbswap64=__builtin_bswap64 \
	\
	-DHOST_POSIX \
	-DFSTYPE=hfs \
	-DFSW_DEBUG_LEVEL=3 \
	-DITERATIONS=1 \
	-DO_BINARY=0 \
	\


hfstest:	${SRCS}
	${CC} -g -Wall -I${PSRC} -I${VSRC} ${CFLAGS} ${SRCS}

clean:
	/bin/rm -fr a.out* *.plist *.o *.gmon

cppcheck:
	cppcheck --enable=all --inconclusive --std=posix -I/usr/include -D__x86_64__ -I${PSRC} -I${VSRC} ${CFLAGS} ${SRCS}
