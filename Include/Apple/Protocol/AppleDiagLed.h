/** @file
  Apple Diag Led protocol.

Copyright (C) 2021, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DIAG_LED_PROTOCOL_H
#define APPLE_DIAG_LED_PROTOCOL_H

#define APPLE_DIAG_LED_PROTOCOL_GUID   \
  { 0xA9FBF34B, 0xE2A2, 0x41D1,        \
    { 0xBA, 0x00, 0xA2, 0x74, 0xA5, 0x5C, 0xD1, 0x64 } }

extern EFI_GUID gAppleDiagLedProtocolGuid;

#endif // APPLE_DIAG_LED_PROTOCOL_H
