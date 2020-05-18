/** @file
  Copyright (C) 2018, John Davis. All rights reserved.
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <IndustryStandard/Riff.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAudioLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/AppleHda.h>
#include <Protocol/AppleBeepGen.h>
#include <Protocol/AppleVoiceOver.h>
#include <Protocol/HdaIo.h>

#include "OcAudioInternal.h"

EFI_STATUS
InternalGetRawData (
  IN  UINT8                          *Buffer,
  IN  UINTN                          BufferSize,
  OUT UINT8                          **RawBuffer,
  OUT UINTN                          *RawBufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ     *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS     *Bits,
  OUT UINT8                          *Channels
  )
{
  RIFF_CHUNK        *TmpChunk;
  WAVE_FORMAT_DATA  *WaveFormat;
  RIFF_CHUNK        *DataChunk;
  UINT8             *BufferPtr;
  UINT8             *BufferEnd;

  if (BufferSize < sizeof (RIFF_CHUNK) + RIFF_CHUNK_ID_SIZE) {
    return EFI_UNSUPPORTED;
  }

  TmpChunk = (RIFF_CHUNK *) Buffer;

  //
  // Ensure:
  // - File size validity
  // - Chunk ID is RIFF
  // - First 4 bytes of data are WAVE.
  //
  if ((UINT64) TmpChunk->Size + sizeof (RIFF_CHUNK) != BufferSize
    || AsciiStrnCmp (TmpChunk->Id, RIFF_CHUNK_ID, RIFF_CHUNK_ID_SIZE) != 0
    || AsciiStrnCmp ((CHAR8*) TmpChunk->Data, WAVE_CHUNK_ID, RIFF_CHUNK_ID_SIZE) != 0) {
    return EFI_UNSUPPORTED;
  }

  WaveFormat = NULL;
  DataChunk  = NULL;
  BufferPtr  = Buffer + sizeof (RIFF_CHUNK) + RIFF_CHUNK_ID_SIZE;
  BufferEnd  = Buffer + BufferSize;

  //
  // Find format and data chunks.
  //
  while (BufferEnd - BufferPtr >= sizeof (RIFF_CHUNK)) {
    TmpChunk = (RIFF_CHUNK *) BufferPtr;

    //
    // If chunk size exceeds file size, abort.
    //
    if ((UINT64) TmpChunk->Size + sizeof (RIFF_CHUNK) > (UINTN) (BufferEnd - BufferPtr)) {
      return EFI_INVALID_PARAMETER;
    }

    if (AsciiStrnCmp (TmpChunk->Id, WAVE_FORMAT_CHUNK_ID, RIFF_CHUNK_ID_SIZE) == 0) {
      if (TmpChunk->Size < sizeof (WAVE_FORMAT_DATA)) {
        return EFI_INVALID_PARAMETER;
      }
      WaveFormat = (WAVE_FORMAT_DATA *) TmpChunk->Data;
    } else if (AsciiStrnCmp (TmpChunk->Id, WAVE_DATA_CHUNK_ID, RIFF_CHUNK_ID_SIZE) == 0) {
      DataChunk   = TmpChunk;
    }

    if (WaveFormat != NULL && DataChunk != NULL) {
      break;
    }

    BufferPtr += TmpChunk->Size + sizeof (RIFF_CHUNK);
  }

  if (WaveFormat == NULL || DataChunk == NULL) {
    return EFI_UNSUPPORTED;
  }

  *RawBuffer     = DataChunk->Data;
  *RawBufferSize = DataChunk->Size;

  switch (WaveFormat->SamplesPerSec) {
    case 8000:
      *Frequency = EfiAudioIoFreq8kHz;
      break;
    case 11000:
      *Frequency = EfiAudioIoFreq11kHz;
      break;
    case 16000:
      *Frequency = EfiAudioIoFreq16kHz;
      break;
    case 22050:
      *Frequency = EfiAudioIoFreq22kHz;
      break;
    case 32000:
      *Frequency = EfiAudioIoFreq32kHz;
      break;
    case 44100:
      *Frequency = EfiAudioIoFreq44kHz;
      break;
    case 48000:
      *Frequency = EfiAudioIoFreq48kHz;
      break;
    case 88000:
      *Frequency = EfiAudioIoFreq88kHz;
      break;
    case 96000:
      *Frequency = EfiAudioIoFreq96kHz;
      break;
    case 192000:
      *Frequency = EfiAudioIoFreq192kHz;
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  switch (WaveFormat->BitsPerSample) {
    case 8:
      *Bits = EfiAudioIoBits8;
      break;
    case 16:
      *Bits = EfiAudioIoBits16;
      break;
    case 20:
      *Bits = EfiAudioIoBits20;
      break;
    case 24:
      *Bits = EfiAudioIoBits24;
      break;
    case 32:
      *Bits = EfiAudioIoBits32;
      break;
    default:
      return EFI_UNSUPPORTED;
  }

  *Channels = (UINT8) WaveFormat->Channels;

  return EFI_SUCCESS;
}
