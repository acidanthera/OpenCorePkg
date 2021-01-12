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

#ifndef OC_WAVE_LIB_H
#define OC_WAVE_LIB_H

#include <Protocol/AudioIo.h>

/**
  Decode WAVE audio to PCM audio.

  @param[in]  Buffer         Buffer with mp3 audio data.
  @param[in]  BufferSize     Buffer size in bytes.
  @param[out] RawBuffer      Decoded PCM data pointing to Buffer.
  @param[out] RawBufferSize  Decoded PCM data size in bytes.
  @param[out] Frequency      Decoded PCM frequency.
  @param[out] Bits           Decoded bit count.
  @param[out] Channels       Decoded amount of channels.

  @retval EFI_SUCCESS on success.
  @retval EFI_UNSUPPORTED on format mismatch.
  @retval EFI_OUT_OF_RESOURCES on memory allocation failure.
**/
EFI_STATUS
OcDecodeWave (
  IN  UINT8                          *Buffer,
  IN  UINTN                          BufferSize,
  OUT UINT8                          **RawBuffer,
  OUT UINT32                         *RawBufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ     *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS     *Bits,
  OUT UINT8                          *Channels
  );

#endif // OC_WAVE_LIB_H
