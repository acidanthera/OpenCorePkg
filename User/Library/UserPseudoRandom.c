/** @file
  Copyright (c) 2020, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef __GNUC__
uint32_t arc4random(void) __attribute__((weak));
uint32_t arc4random_uniform(uint32_t upper_bound) __attribute__((weak));
#endif

// Taken from https://en.wikipedia.org/wiki/Xorshift#Example_implementation
// I am not positive what is better to use here (especially on Windows).
// Fortunately we only need something only looking random.
uint32_t pseudo_random(void) {
#if defined(__GNUC__) && !defined(__EMSCRIPTEN__)
    if (arc4random)
      return arc4random();
#endif

  static uint32_t state;

  if (!state) {
    fprintf(stderr, "Warning: arc4random is not available!\n");
    state = (uint32_t)time(NULL);
  }

  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = x;
  return x;
}

// Taken from https://opensource.apple.com/source/Libc/Libc-1082.50.1/gen/FreeBSD/arc4random.c
// Mac OS X 10.6.8 and earlier do not have arc4random_uniform, so we implement one.
uint32_t pseudo_random_between(uint32_t from, uint32_t to) {
  uint32_t upper_bound = to + 1 - from;

#if defined(__GNUC__) && !defined(__EMSCRIPTEN__)
  // Prefer native implementation if available.
  if (arc4random_uniform)
    return from + arc4random_uniform(upper_bound);
#endif

  uint32_t r, min;

  if (upper_bound < 2)
    return from;

#if (ULONG_MAX > 0xffffffffUL)
  min = 0x100000000UL % upper_bound;
#else
  if (upper_bound > 0x80000000)
    min = 1 + ~upper_bound;
  else
    min = ((0xffffffff - (upper_bound * 2)) + 1) % upper_bound;
#endif

  for (;;) {
    r = pseudo_random();
    if (r >= min)
      break;
  }

  return from + r % upper_bound;
}
