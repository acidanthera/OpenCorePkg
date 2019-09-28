/** @file
  Key code ring

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef AIK_DATA_H
#define AIK_DATA_H

#include <Protocol/AmiKeycode.h>

//
// Maximum amount of keys queued for non-Apple protocols
//
#define AIK_DATA_BUFFER_SIZE 12

typedef struct {
  //
  // Stored key buffer for responding to non-Apple protocols
  //
  AMI_EFI_KEY_DATA    KeyBuffer[AIK_DATA_BUFFER_SIZE];
  AMI_EFI_KEY_DATA    *KeyBufferHead;
  AMI_EFI_KEY_DATA    *KeyBufferTail;
  UINTN               KeyBufferSize;
} AIK_DATA;

VOID
AIKDataReset (
  IN OUT AIK_DATA  *Data
  );

BOOLEAN
AIKDataEmpty (
  IN AIK_DATA  *Data
  );

EFI_STATUS
AIKDataReadEntry (
  IN OUT AIK_DATA          *Data,
     OUT AMI_EFI_KEY_DATA  *KeyData
  );

VOID
AIKDataWriteEntry (
  IN OUT AIK_DATA          *Data,
  IN     AMI_EFI_KEY_DATA  *KeyData
  );

#endif
