/** @file

Read macOS .disk_label (.disk_label_2x) file and convert to .ppm.
Reference: http://refit.sourceforge.net/info/vollabel.html

Copyright (c) 2019, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static uint8_t palette[256] = {
  [0x00] = 255,
  [0xf6] = 238,
  [0xf7] = 221,
  [0x2a] = 204,
  [0xf8] = 187,
  [0xf9] = 170,
  [0x55] = 153,
  [0xfa] = 136,
  [0xfb] = 119,
  [0x80] = 102,
  [0xfc] = 85,
  [0xfd] = 68,
  [0xab] = 51,
  [0xfe] = 34,
  [0xff] = 17,
  [0xd6] = 0
};

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

static int write_ppm(const char *filename, size_t width, size_t height, uint8_t *pixel) {
  FILE *fh = fopen(filename, "wb");
  if (!fh) {
    fprintf(stderr, "Failed to open out file %s!\n", filename);
    return -1;
  }

  if (fprintf(fh, "P6\n%zu %zu\n255\n", width, height) < 0) {
    fprintf(stderr, "Failed to write ppm header to %s!\n", filename);
    fclose(fh);
    return -1;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      uint8_t col[3];
      col[0] = col[1] = col[2] = palette[*pixel];
      if (fwrite(col, sizeof(col), 1, fh) != 1) {
        fprintf(stderr, "Failed to write ppm pixel %zux%zu to %s!\n", x, y, filename);
        fclose(fh);
        return -1;
      }
      ++pixel;
    }
  }

  fclose(fh);

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Pass file and outfile!\n");
    return -1;
  }

  uint8_t *buffer;
  size_t size;

  if (read_file(argv[1], &buffer, &size)) {
    return -1;
  }

  size_t header_size = sizeof(uint8_t) + sizeof(uint16_t)*2;

  if (size < header_size) {
    fprintf(stderr, "Too low size %zu!\n", size);
    free(buffer);
    return -1;
  }

  if (buffer[0] != 1) {
    fprintf(stderr, "Invalid magic %02X!\n", buffer[0]);
    free(buffer);
    return -1;
  }

  size_t width = ((size_t) buffer[1] << 8U) | (size_t) buffer[2];
  size_t height = ((size_t) buffer[3] << 8U) | (size_t) buffer[4];

  if (width * height == 0 || size - header_size != width*height) {
    fprintf(stderr, "Image mismatch %zux%zu with size %zu!\n", width, height, size);
    free(buffer);
    return -1;
  }

  (void)remove(argv[2]);

  if (write_ppm(argv[2], width, height, buffer + header_size)) {
    free(buffer);
    return -1;
  }

  free(buffer);
  return 0;
}
