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

#ifndef EFI_AUDIO_DECODE_H
#define EFI_AUDIO_DECODE_H

#include <Uefi.h>
#include <Protocol/AudioIo.h>

/**
  Audio decoding protocol GUID.
**/
#define EFI_AUDIO_DECODE_PROTOCOL_GUID \
  { 0xAF3F6C23, 0x8132, 0x4880,        \
    { 0xB3, 0x29, 0x04, 0x8D, 0xF7, 0x1D, 0xD8, 0x6A } }

typedef struct EFI_AUDIO_DECODE_PROTOCOL_ EFI_AUDIO_DECODE_PROTOCOL;

/**
  Decode any supported audio to PCM audio.

  @param[in]  This           Audio decode protocol instance.
  @param[in]  InBuffer       Buffer with audio data.
  @param[in]  InBufferSize   InBuffer size in bytes.
  @param[out] OutBuffer      Decoded PCM data allocated from pool (needs to be freed).
  @param[out] OutBufferSize  Decoded PCM data size in bytes.
  @param[out] Frequency      Decoded PCM frequency.
  @param[out] Bits           Decoded bit count.
  @param[out] Channels       Decoded amount of channels.

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER for null pointers.
  @retval EFI_UNSUPPORTED on format mismatch.
  @retval EFI_OUT_OF_RESOURCES on memory allocation failure.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_DECODE_ANY) (
  IN  EFI_AUDIO_DECODE_PROTOCOL      *This,
  IN  CONST VOID                     *InBuffer,
  IN  UINT32                         InBufferSize,
  OUT VOID                           **OutBuffer,
  OUT UINT32                         *OutBufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ     *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS     *Bits,
  OUT UINT8                          *Channels
  );

/**
  Decode WAVE audio to PCM audio.

  @param[in]  This           Audio decode protocol instance.
  @param[in]  InBuffer       Buffer with audio data.
  @param[in]  InBufferSize   InBuffer size in bytes.
  @param[out] OutBuffer      Decoded PCM data.
  @param[out] OutBufferSize  Decoded PCM data size in bytes.
  @param[out] Frequency      Decoded PCM frequency.
  @param[out] Bits           Decoded bit count.
  @param[out] Channels       Decoded amount of channels.
  @param[in]  InPlace        Do not allocate OutBuffer, but reuse InBuffer.
                             Otherwise allocated from pool and needs to be freed.

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER for null pointers.
  @retval EFI_UNSUPPORTED on format mismatch.
  @retval EFI_OUT_OF_RESOURCES on memory allocation failure.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_DECODE_WAVE) (
  IN  EFI_AUDIO_DECODE_PROTOCOL      *This,
  IN  CONST VOID                     *InBuffer,
  IN  UINT32                         InBufferSize,
  OUT VOID                           **OutBuffer,
  OUT UINT32                         *OutBufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ     *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS     *Bits,
  OUT UINT8                          *Channels,
  IN  BOOLEAN                        InPlace
  );

/**
  Decode MP3 audio to PCM audio.

  @param[in]  This           Audio decode protocol instance.
  @param[in]  InBuffer       Buffer with audio data.
  @param[in]  InBufferSize   InBuffer size in bytes.
  @param[out] OutBuffer      Decoded PCM data allocated from pool (needs to be freed).
  @param[out] OutBufferSize  Decoded PCM data size in bytes.
  @param[out] Frequency      Decoded PCM frequency.
  @param[out] Bits           Decoded bit count.
  @param[out] Channels       Decoded amount of channels.

  @retval EFI_SUCCESS on success.
  @retval EFI_INVALID_PARAMETER for null pointers.
  @retval EFI_UNSUPPORTED on format mismatch.
  @retval EFI_OUT_OF_RESOURCES on memory allocation failure.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_DECODE_MP3) (
  IN  EFI_AUDIO_DECODE_PROTOCOL      *This,
  IN  CONST VOID                     *InBuffer,
  IN  UINT32                         InBufferSize,
  OUT VOID                           **OutBuffer,
  OUT UINT32                         *OutBufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ     *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS     *Bits,
  OUT UINT8                          *Channels
  );

/**
  Protocol struct.
**/
struct EFI_AUDIO_DECODE_PROTOCOL_ {
  EFI_AUDIO_DECODE_ANY       DecodeAny;
  EFI_AUDIO_DECODE_WAVE      DecodeWave;
  EFI_AUDIO_DECODE_MP3       DecodeMp3;
};

extern EFI_GUID gEfiAudioDecodeProtocolGuid;

#endif // EFI_AUDIO_DECODE_H
