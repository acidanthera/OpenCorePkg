#!/usr/bin/env python

"""
Validates schema files (e.g. OcConfigurationLib.c) for being sorted.
"""

import re
import sys

if len(sys.argv) < 2:
  print('Pass file to check')
  sys.exit(-1)

with open(sys.argv[1], 'r') as f:
  prev = ''
  content = [l.strip() for l in f.readlines()]
  for i, l in enumerate(content):
    if l == 'OC_SCHEMA':
      print('Checking schema {}'.format(re.match(r'^\w+', content[i+1]).group(0)))
      prev = ''
      continue
    x = re.search(r'"([^"]+)"', l)
    if x:
      if x.group(1) < prev:
        print('ERROR: {} precedes {}'.format(prev, x.group(1)))
        sys.exit(1)
      prev = x.group(1)
