/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_USER_FILE_H
#define OC_USER_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint8_t *UserReadFile(const char *str, uint32_t *size);

void UserWriteFile(const char *str, void *data, uint32_t size);

#endif // OC_USER_FILE_H
