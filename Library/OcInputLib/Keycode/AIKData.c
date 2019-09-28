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

#include "AIKData.h"

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

VOID
AIKDataReset (
  IN OUT AIK_DATA  *Data
  )
{
  Data->KeyBufferHead = Data->KeyBufferTail = Data->KeyBuffer;
  Data->KeyBufferSize = 0;
}

BOOLEAN
AIKDataEmpty (
  IN AIK_DATA  *Data
  )
{
  return Data->KeyBufferSize == 0;
}

EFI_STATUS
AIKDataReadEntry (
  IN OUT AIK_DATA          *Data,
     OUT AMI_EFI_KEY_DATA  *KeyData
  )
{
  if (Data->KeyBufferSize == 0) {
    return EFI_NOT_READY;
  }

  CopyMem (KeyData, Data->KeyBufferTail, sizeof (*KeyData));

  Data->KeyBufferSize--;
  Data->KeyBufferTail++;
  if (Data->KeyBufferTail == &Data->KeyBuffer[AIK_DATA_BUFFER_SIZE]) {
    Data->KeyBufferTail = Data->KeyBuffer;
  }

  return EFI_SUCCESS;
}

VOID
AIKDataWriteEntry (
  IN OUT AIK_DATA          *Data,
  IN     AMI_EFI_KEY_DATA  *KeyData
  )
{
  //
  // Eat the first entry if we have no room
  //
  if (Data->KeyBufferSize == AIK_DATA_BUFFER_SIZE) {
    Data->KeyBufferSize--;
    Data->KeyBufferTail++;
    if (Data->KeyBufferTail == &Data->KeyBuffer[AIK_DATA_BUFFER_SIZE]) {
      Data->KeyBufferTail = Data->KeyBuffer;
    }
  }

  Data->KeyBufferSize++;
  CopyMem (Data->KeyBufferHead, KeyData, sizeof (*KeyData));
  Data->KeyBufferHead++;
  if (Data->KeyBufferHead == &Data->KeyBuffer[AIK_DATA_BUFFER_SIZE]) {
    Data->KeyBufferHead = Data->KeyBuffer;
  }
}
