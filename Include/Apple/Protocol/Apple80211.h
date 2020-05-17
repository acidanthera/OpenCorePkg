/** @file
  Apple 80211.

Copyright (C) 2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_80211_H
#define APPLE_80211_H

/**
  Installed by Wi-Fi drivers.
  71B4903C-14EC-42C4-BDC6-CE1449930E49
**/

#define APPLE_80211_PROTOCOL_GUID \
  { 0x71B4903C, 0x14EC, 0x42C4,   \
     {0xBD, 0xC6, 0xCE, 0x14, 0x49, 0x93, 0x0E, 0x49 } }

extern EFI_GUID gApple80211ProtocolGuid;

#endif // APPLE_80211_H
