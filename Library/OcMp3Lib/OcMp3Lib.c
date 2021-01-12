/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMp3Lib.h>
#include "helix/mp3dec.h"

/**
  Ensure that buffer always has enough memory to hold one frame.

  @param[in,out]  Buffer      Pointer to buffer memory.
  @param[in,out]  BufferCurr  Pointer to buffer current position.
  @param[in,out]  BufferSize  Pointer to buffer size.

  @retval TRUE on success.
  @retval FALSE on allocation falure.
**/
STATIC
BOOLEAN
BufferResize (
  IN OUT VOID            **Buffer,
  IN OUT VOID            **BufferCurr,
  IN OUT UINT32          *BufferSize
  )
{
  UINT32  OrgSize;
  UINT32  OrgOffset;
  UINT32  RemainingSize;
  VOID    *NewBuffer;

  if (*BufferSize == 0) {
    OrgOffset     = 0;
    RemainingSize = 0;
  } else {
    OrgOffset     = (UINT32) ((UINTN) *BufferCurr - (UINTN) *Buffer);
    RemainingSize = *BufferSize - OrgOffset;
  }

  if (RemainingSize >= MAX_NCHAN * MAX_NGRAN * MAX_NSAMP) {
    return TRUE;
  }

  if (*BufferSize == 0) {
    *BufferSize = MAX_NCHAN * MAX_NGRAN * MAX_NSAMP * 10;
    *BufferCurr = *Buffer = AllocatePool (*BufferSize);
    return *Buffer != NULL;
  }

  OrgSize = *BufferSize;
  if (OcOverflowMulU32 (OrgSize, 2, BufferSize)) {
    FreePool (*Buffer);
    return FALSE;
  }

  ASSERT (OrgSize + MAX_NCHAN * MAX_NGRAN * MAX_NSAMP <= *BufferSize);

  NewBuffer = ReallocatePool (
    OrgSize,
    *BufferSize,
    *Buffer
    );
  if (NewBuffer == NULL) {
    FreePool (*Buffer);
    return FALSE;
  }

  *Buffer     = NewBuffer;
  *BufferCurr = (UINT8 *) NewBuffer + OrgOffset;
  return TRUE;
}

EFI_STATUS
OcDecodeMp3 (
  IN  CONST VOID                     *InBuffer,
  IN  UINT32                         InBufferSize,
  OUT VOID                           **OutBuffer,
  OUT UINT32                         *OutBufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ     *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS     *Bits,
  OUT UINT8                          *Channels
  )
{
  HMP3Decoder     Decoder;
  MP3FrameInfo    FrameInfo;
  unsigned char   *Walker;
  VOID            *OutBufferCurr;
  int             ErrorCode;
  int             BytesLeft;
  int             SyncOffset;

  Decoder = MP3InitDecoder ();
  if (Decoder == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (&FrameInfo, sizeof (FrameInfo));

  Walker         = (VOID *) InBuffer;
  BytesLeft      = (int) InBufferSize;
  *OutBuffer     = NULL;
  OutBufferCurr  = NULL;
  *OutBufferSize = 0;

  while (BytesLeft > 0) {
    //
    // Find next frame.
    //
    SyncOffset = MP3FindSyncWord (
      Walker,
      BytesLeft
      );
    if (SyncOffset < 0) {
      //
      // Not a valid file.
      //
      if (OutBufferCurr == NULL) {
        MP3FreeDecoder (Decoder);
        return EFI_OUT_OF_RESOURCES;
      }

      break;
    }

    Walker    += SyncOffset;
    BytesLeft -= SyncOffset;

    if (!BufferResize (OutBuffer, &OutBufferCurr, OutBufferSize)) {
      MP3FreeDecoder (Decoder);
      return EFI_OUT_OF_RESOURCES;
    }

    ErrorCode = MP3Decode (
      Decoder,
      &Walker,
      &BytesLeft,
      OutBufferCurr,
      0
      );

    //
    // Do nothing, we will get enough data on the next frame.
    //
    if (ErrorCode == ERR_MP3_MAINDATA_UNDERFLOW) {
      continue;
    }

    if (ErrorCode < 0) {
      MP3FreeDecoder (Decoder);
      FreePool (*OutBuffer);
      return EFI_UNSUPPORTED;
    }

    MP3GetLastFrameInfo (Decoder, &FrameInfo);
    OutBufferCurr = (UINT8 *) OutBufferCurr + FrameInfo.bitsPerSample / 8 * FrameInfo.outputSamps;
  }

  MP3FreeDecoder (Decoder);

  switch (FrameInfo.bitsPerSample) {
    case 8:
      *Bits = EfiAudioIoBits8;
      break;
    case 16:
      *Bits = EfiAudioIoBits16;
      break;
    case 20:
      *Bits = EfiAudioIoBits16;
      break;
    case 24:
      *Bits = EfiAudioIoBits24;
      break;
    case 32:
      *Bits = EfiAudioIoBits32;
      break;
    default:
      FreePool (*OutBuffer);
      return EFI_UNSUPPORTED;
  }

  switch (FrameInfo.samprate) {
    case 8000:
      *Frequency = EfiAudioIoFreq8kHz;
      break;
    case 11025:
      *Frequency = EfiAudioIoFreq11kHz;
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
    default:
      FreePool (*OutBuffer);
      return EFI_UNSUPPORTED;
  }

  *Channels = (UINT8) FrameInfo.nChans;
  *OutBufferSize = (UINT32) ((UINT8 *) OutBufferCurr - (UINT8 *) *OutBuffer);

  return EFI_SUCCESS;
}
