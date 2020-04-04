/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_AFTER_BOOT_COMPAT_PROTOCOL_H
#define OC_AFTER_BOOT_COMPAT_PROTOCOL_H

#define OC_AFTER_BOOT_COMPAT_PROTOCOL_REVISION  0x010000

//
// OC_AFTER_BOOT_COMPAT_PROTOCOL_GUID
// C7CBA84E-CC77-461D-9E3C-6BE0CB79A7C1
//
#define OC_AFTER_BOOT_COMPAT_PROTOCOL_GUID  \
  { 0xC7CBA84E, 0xCC77, 0x461D,             \
    { 0x9E, 0x3C, 0x6B, 0xE0, 0xCB, 0x79, 0xA7, 0xC1 } }

//
// Includes a revision for debugging reasons
//
typedef struct {
  UINTN                   Revision;
} OC_AFTER_BOOT_COMPAT_PROTOCOL;

extern EFI_GUID gOcAfterBootCompatProtocolGuid;

#endif // OC_AFTER_BOOT_COMPAT_PROTOCOL_H
