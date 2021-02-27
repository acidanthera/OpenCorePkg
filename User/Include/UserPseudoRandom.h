/** @file
  Copyright (c) 2020, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_USER_PSEUDO_RANDOM_H
#define OC_USER_PSEUDO_RANDOM_H

#include <stdint.h>

uint32_t pseudo_random(void);
uint32_t pseudo_random_between(uint32_t from, uint32_t to);

#endif // OC_USER_PSEUDO_RANDOM_H
