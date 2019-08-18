CC = i686-w64-mingw32-gcc
CFLAGS=-static -c -DNDEBUG -Wall -Wextra -pedantic -Os

all: nvram

nvram: nvram.o
	$(CC) -s -static nvram.o -o nvram.exe

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o nvram.exe
