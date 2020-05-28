/** @file

Create .icns from 1x and 2x png files.

Copyright (c) 2020, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define BigEndianToNative32(x) __builtin_bswap32(x)
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BigEndianToNative32(x) (x)
#else
#include <arpa/inet.h>
#define BigEndianToNative32(x) ntohl(x)
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int read_file(const char *filename, uint8_t **buffer, size_t *size) {
  FILE *fh = fopen(filename, "rb");
  if (!fh) {
    fprintf(stderr, "Missing file %s!\n", filename);
    return -1;
  }

  if (fseek(fh, 0, SEEK_END)) {
    fprintf(stderr, "Failed to find end of %s!\n", filename);
    fclose(fh);
    return -1;
  }

  long pos = ftell(fh);

  if (pos <= 0) {
    fprintf(stderr, "Invalid file size (%ld) of %s!\n", pos, filename);
    fclose(fh);
    return -1;
  }

  if (fseek(fh, 0, SEEK_SET)) {
    fprintf(stderr, "Failed to rewind %s!\n", filename);
    fclose(fh);
    return -1;
  }

  *size = (size_t)pos;
  *buffer = (uint8_t *)malloc(*size);

  if (!*buffer) {
    fprintf(stderr, "Failed to allocate %zu bytes for %s!\n", *size, filename);
    fclose(fh);
    return -1;
  }

  if (fread(*buffer, *size, 1, fh) != 1) {
    fprintf(stderr, "Failed to read %zu bytes from %s!\n", *size, filename);
    fclose(fh);
    free(*buffer);
    return -1;
  }

  fclose(fh);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 4 ) {
    fprintf(stderr,
      "Usage:\n"
      " icnspack image.icns 1x.png 2x.png\n");
    return -1;
  }

  (void) remove(argv[1]);
  uint8_t *buffer1x;
  size_t size1x;
  int r1 = read_file(argv[2], &buffer1x, &size1x);
  uint8_t *buffer2x;
  size_t size2x;
  int r2 = read_file(argv[3], &buffer2x, &size2x);

  FILE *fh = NULL;
  if (r1 == 0 && r2 == 0) {
    fh = fopen (argv[1], "wb");
    if (fh != NULL) {
      uint32_t size = BigEndianToNative32(size1x + size2x + sizeof(uint32_t)*6);
      fwrite("icns", sizeof(uint32_t), 1, fh);
      fwrite(&size, sizeof(uint32_t), 1, fh);

      size = BigEndianToNative32(size1x + sizeof(uint32_t)*2);
      fwrite("ic07", sizeof(uint32_t), 1, fh);
      fwrite(&size, sizeof(uint32_t), 1, fh);
      fwrite(buffer1x, size1x, 1, fh);

      size = BigEndianToNative32(size2x + sizeof(uint32_t)*2);
      fwrite("ic13", sizeof(uint32_t), 1, fh);
      fwrite(&size, sizeof(uint32_t), 1, fh);
      fwrite(buffer2x, size2x, 1, fh);

      fclose(fh);
    }
  }

  if (r1 == 0) free(buffer1x);
  if (r2 == 0) free(buffer2x);

  return fh != NULL ? 0 : -1;
}
