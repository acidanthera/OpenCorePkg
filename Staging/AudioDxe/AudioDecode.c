/*
 * File: AudioDxe.c
 *
 * Copyright (c) 2021 vit9696
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "AudioDxe.h"
#include <Library/OcMp3Lib.h>
#include <Library/OcWaveLib.h>

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
STATIC
EFI_STATUS
EFIAPI
AudioDecodeWave (
  IN  EFI_AUDIO_DECODE_PROTOCOL      *This,
  IN  CONST VOID                     *InBuffer,
  IN  UINT32                         InBufferSize,
  OUT VOID                           **OutBuffer,
  OUT UINT32                         *OutBufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ     *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS     *Bits,
  OUT UINT8                          *Channels,
  IN  BOOLEAN                        InPlace
  )
{
  EFI_STATUS  Status;

  Status = OcDecodeWave (
    (UINT8 *) InBuffer,
    InBufferSize,
    (UINT8 **) OutBuffer,
    OutBufferSize,
    Frequency,
    Bits,
    Channels
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!InPlace) {
    *OutBuffer = AllocateCopyPool (*OutBufferSize, *OutBuffer);
    if (*OutBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

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
STATIC
EFI_STATUS
EFIAPI
AudioDecodeMp3 (
  IN  EFI_AUDIO_DECODE_PROTOCOL      *This,
  IN  CONST VOID                     *InBuffer,
  IN  UINT32                         InBufferSize,
  OUT VOID                           **OutBuffer,
  OUT UINT32                         *OutBufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ     *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS     *Bits,
  OUT UINT8                          *Channels
  )
{
  EFI_STATUS  Status;

  Status = OcDecodeMp3 (
    InBuffer,
    InBufferSize,
    OutBuffer,
    OutBufferSize,
    Frequency,
    Bits,
    Channels
    );

  return Status;
}

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
STATIC
EFI_STATUS
EFIAPI
AudioDecodeAny (
  IN  EFI_AUDIO_DECODE_PROTOCOL      *This,
  IN  CONST VOID                     *InBuffer,
  IN  UINT32                         InBufferSize,
  OUT VOID                           **OutBuffer,
  OUT UINT32                         *OutBufferSize,
  OUT EFI_AUDIO_IO_PROTOCOL_FREQ     *Frequency,
  OUT EFI_AUDIO_IO_PROTOCOL_BITS     *Bits,
  OUT UINT8                          *Channels
  )
{
  EFI_STATUS  Status;

  Status = AudioDecodeMp3 (
    This,
    InBuffer,
    InBufferSize,
    OutBuffer,
    OutBufferSize,
    Frequency,
    Bits,
    Channels
    );
  if (EFI_ERROR (Status)) {
    Status = AudioDecodeWave (
      This,
      InBuffer,
      InBufferSize,
      OutBuffer,
      OutBufferSize,
      Frequency,
      Bits,
      Channels,
      FALSE
      );
  }

  return Status;
}

/**
  Protocol definition.
**/
EFI_AUDIO_DECODE_PROTOCOL
gEfiAudioDecodeProtocol = {
  .DecodeAny  = AudioDecodeAny,
  .DecodeWave = AudioDecodeWave,
  .DecodeMp3  = AudioDecodeMp3
};
