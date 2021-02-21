/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <UserFile.h>

uint8_t *UserReadFile(const char *str, uint32_t *size) {
  FILE *f = fopen(str, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *string = malloc(fsize + 1);
  if (fsize > 0 && fread(string, fsize, 1, f) != 1)
    abort();
  fclose(f);

  string[fsize] = 0;
  *size = fsize;

  return string;
}

void UserWriteFile(const char *str, void *data, uint32_t size) {
  FILE *Fh = fopen(str, "wb");

  if (!Fh) abort();
  
  if (fwrite (data, size, 1, Fh) != 1)
    abort();
  fclose(Fh);
}
