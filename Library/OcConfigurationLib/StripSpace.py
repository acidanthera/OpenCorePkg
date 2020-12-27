#!/usr/bin/env python3

#  Copyright (c) 2020, Mike Beaton. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause

"""
Make it easier to compare generated and original files by converting all multiple spaces to single space.
"""

import sys

if len(sys.argv) < 2:
  print('provide filename to strip')
  sys.exit(-1)

handle = open(sys.argv[1], 'r')
all_chars = handle.read()
handle.close()

length = len(all_chars)
while True:
  all_chars = all_chars.replace('  ', ' ')
  new_len = len(all_chars)
  if new_len == length:
    break;
  length = new_len

print(all_chars, end='')
