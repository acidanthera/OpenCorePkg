/** @file
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMp3Lib.h>
#include <Library/OcMiscLib.h>

#include <UserFile.h>

int
ENTRY_POINT (
  int   argc,
  char  *argv[]
  )
{
  UINT32  Size;
  UINT8   *Buffer;

  if ((Buffer = UserReadFile (argc > 1 ? argv[1] : "test.mp3", &Size)) == NULL) {
    DEBUG ((DEBUG_ERROR, "Read fail\n"));
    return -1;
  }

  VOID                        *OutBuffer;
  UINT32                      OutSize;
  EFI_AUDIO_IO_PROTOCOL_FREQ  Freq;
  EFI_AUDIO_IO_PROTOCOL_BITS  Bits;
  UINT8                       Channels;
  EFI_STATUS                  Status;

  Status = OcDecodeMp3 (
    Buffer,
    Size,
    &OutBuffer,
    &OutSize,
    &Freq,
    &Bits,
    &Channels
    );

  FreePool (Buffer);

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Decode success %u\n", OutSize));
    UserWriteFile ("test.bin", OutBuffer, OutSize);
    FreePool (OutBuffer);
    return 0;
  }

  DEBUG ((DEBUG_WARN, "Decode failure - %r\n", Status));
  return 1;
}

int
LLVMFuzzerTestOneInput (
  const uint8_t  *Data,
  size_t         Size
  )
{
  VOID        *OutBuffer;
  UINT32      OutSize;
  EFI_STATUS  Status;

  if (Size > 0) {
    EFI_AUDIO_IO_PROTOCOL_FREQ  Freq;
    EFI_AUDIO_IO_PROTOCOL_BITS  Bits;
    UINT8                       Channels;

    Status = OcDecodeMp3 (
      Data,
      Size,
      &OutBuffer,
      &OutSize,
      &Freq,
      &Bits,
      &Channels
      );
    if (!EFI_ERROR (Status)) {
      FreePool (OutBuffer);
    }
  }
  return 0;
}
