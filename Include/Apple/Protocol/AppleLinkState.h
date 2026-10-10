/** @file
  Apple LinkState protocol.

  Copyright (c) 2026, ilikesn0w. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef APPLE_LINK_STATE_H
#define APPLE_LINK_STATE_H

/**
  Installed by internet drivers.
  BF71D7E3-4AE8-47F1-8605-9D576AAB4478
**/
#define APPLE_LINK_STATE_PROTOCOL_GUID \
  { 0xBF71D7E3, 0x4AE8, 0x47F1,   \
     {0x86, 0x05, 0x9D, 0x57, 0x6A, 0xAB, 0x44, 0x78 } }

extern EFI_GUID  gAppleLinkStateProtocolGuid;

#endif // APPLE_LINK_STATE_H
