#!/usr/bin/env python3

"""
Quick and dirty unpacker for next generation Mac firmware.

Copyright (c) 2019, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
"""

import os
import struct
import sys

MAC_EFI_HEADER_FORMAT = '<8sII'
MAC_EFI_HEADER_SIZE   = 256
MAC_EFI_MAGIC         = b'\x5F\x4D\x45\x46\x49\x42\x49\x4E'
INTEL_EFI_MAGIC       = b'\x5A\xA5\xF0\x0F'
IM4P_MAC_EFI_MAGIC    = b'\x49\x4D\x34\x50\x16\x04\x6D\x65\x66\x69'

def save_macfw(firmware, path):
  # Hack for Apple images with an extra zero typo in NVRAM volume size.
  NVBUG_FIND = b'\x8D\x2B\xF1\xFF\x96\x76\x8B\x4C\xA9\x85\x27\x47\x07\x5B\x4F\x50\x00\x00\x2F\x00'
  NVBUG_REPL = b'\x8D\x2B\xF1\xFF\x96\x76\x8B\x4C\xA9\x85\x27\x47\x07\x5B\x4F\x50\x0C\xEF\x02\x00'

  with open(path, 'wb') as fd1:
    fd1.write(firmware.replace(NVBUG_FIND, NVBUG_REPL))
  return 0

def get_fwname(filename, outdir, suffix=''):
  outname = os.path.splitext(os.path.basename(filename))[0]
  return os.path.join(outdir, outname + suffix + '.rom')

def unwrap_macefi(filename, outdir, firmware, firmware_len):
  if firmware_len < MAC_EFI_HEADER_SIZE:
    raise RuntimeError('Invalid firmware size')

  magic, first, second = struct.unpack_from(MAC_EFI_HEADER_FORMAT, firmware)
  if magic != MAC_EFI_MAGIC:
    raise RuntimeError('Unsupported magic!')

  if first >= firmware_len - MAC_EFI_HEADER_SIZE:
    raise RuntimeError('Invalid first image offset 0x{:02X} for size 0x{:02X}'.format(first, firmware_len))

  if second >= firmware_len - MAC_EFI_HEADER_SIZE:
    raise RuntimeError('Invalid second image offset 0x{:02X} for size 0x{:02X}'.format(second, firmware_len))

  if first >= second:
    raise RuntimeError('First image offset 0x{:02X} comes after second 0x{:02X}'.format(first, second))

  save_macfw(firmware[first + MAC_EFI_HEADER_SIZE:second+MAC_EFI_HEADER_SIZE], get_fwname(filename, outdir, '1'))
  save_macfw(firmware[second + MAC_EFI_HEADER_SIZE:], get_fwname(filename, outdir, '2'))
  return 0

def unwrap_im4p(filename, outdir, firmware, firmware_len):
  # FIXME: This is ugly, as IM4P is actually an ASN1 encoded sequence.
  off = firmware.find(IM4P_MAC_EFI_MAGIC)
  if off < 0 or firmware[0] != 0x30:
    return unwrap_macefi(filename, outdir, firmware, firmware_len)

  off = firmware.find(MAC_EFI_MAGIC)
  if off >= 0:
    return unwrap_macefi(filename, outdir, firmware[off:], firmware_len - off)

  off = firmware.find(INTEL_EFI_MAGIC)
  if off >= 16:
    return save_macfw(firmware[off - 16:], get_fwname(filename, outdir))

  raise RuntimeError('Unsupported firmware format')

def unpack_image(filename, outdir):
  if not os.path.exists(filename):
    raise RuntimeError('Failed to find filename image {}!'.format(filename))

  with open(filename, 'rb') as fd:
    firmware     = fd.read()
    firmware_len = len(firmware)
    return unwrap_im4p(filename, outdir, firmware, firmware_len)

if __name__ == '__main__':
  if len(sys.argv) < 2:
    print('Usage: ./MacEfiUnpack.py filename [outdir]')
    sys.exit(-1)
  try:
    if len(sys.argv) > 2:
      outdir = sys.argv[2]
    else:
      outdir = os.path.dirname(sys.argv[1])
    sys.exit(unpack_image(sys.argv[1], outdir))
  except Exception as e:
    print('ERROR {}'.format(str(e)))
