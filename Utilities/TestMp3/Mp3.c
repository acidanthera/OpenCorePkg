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

#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdint.h>

#include <UserFile.h>

int ENTRY_POINT(int argc, char** argv) {
  uint32_t size;
  uint8_t *buffer;
  if ((buffer = UserReadFile(argc > 1 ? argv[1] : "test.mp3", &size)) == NULL) {
    printf("Read fail\n");
    return -1;
  }

  void *outbuffer;
  uint32_t outsize;
  EFI_AUDIO_IO_PROTOCOL_FREQ freq;
  EFI_AUDIO_IO_PROTOCOL_BITS bits;
  UINT8                      channels;

  EFI_STATUS Status = OcDecodeMp3 (
    buffer,
    size,
    &outbuffer,
    &outsize,
    &freq,
    &bits,
    &channels
    );

  FreePool(buffer);

  if (!EFI_ERROR (Status)) {
    printf("Decode success %u\n", outsize);
    UserWriteFile("test.bin", outbuffer, outsize);
    FreePool(outbuffer);
    return 0;
  }

  DEBUG ((DEBUG_WARN, "Decode failure - %r\n", Status));
  return 1;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  if (Size > 0) {
    void *outbuffer;
    uint32_t outsize;
    EFI_AUDIO_IO_PROTOCOL_FREQ freq;
    EFI_AUDIO_IO_PROTOCOL_BITS bits;
    UINT8                      channels;

    EFI_STATUS Status = OcDecodeMp3 (
      Data,
      Size,
      &outbuffer,
      &outsize,
      &freq,
      &bits,
      &channels
      );
    if (!EFI_ERROR (Status)) {
      FreePool(outbuffer);
    }
  }
  return 0;
}
