/*
 * File: AudioProgress.h
 *
 * Copyright (c) 2026 llz121517
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

#ifndef EFI_AUDIO_PROGRESS_H
#define EFI_AUDIO_PROGRESS_H

#include <Uefi.h>

/**
  Audio Progress protocol GUID.
**/
#define EFI_AUDIO_PROGRESS_PROTOCOL_GUID \
  { 0x6F3A1D84, 0x52B9, 0x4C7E,          \
    { 0x9A, 0x05, 0x8D, 0x1F, 0x4B, 0x6E, 0x2C, 0x73 } }

typedef struct EFI_AUDIO_PROGRESS_PROTOCOL_ EFI_AUDIO_PROGRESS_PROTOCOL;

#define EFI_AUDIO_PROGRESS_PROTOCOL_REVISION  1

/**
  Gets the progress of the most recent playback request.

  The byte counts come from the controller's own DMA accounting, so they
  describe data the controller has consumed rather than data the listener has
  heard. The controller buffers ahead of the loudspeakers, which makes the
  reported position lead the audible output slightly.

  The counts outlive the playback request itself. Once a request completes or
  is stopped, they stop at the position reached instead of being cleared, and
  Playing becomes FALSE. Before the first request both counts are 0.

  BytesConsumed never exceeds BytesTotal, so a caller can compute a percentage
  without guarding against overshoot.

  That clamp has two observable edges. Playing can be TRUE while BytesConsumed
  already equals BytesTotal, during the last polls of a request before completion
  is declared, so a caller computing time remaining has to treat a full count as
  done rather than as a contradiction. And a playback that failed to start after
  its buffer was claimed keeps BytesTotal of the request with BytesConsumed 0,
  which is also what a stop before anything played leaves behind.

  @param[in]  This            A pointer to the EFI_AUDIO_PROGRESS_PROTOCOL instance.
  @param[out] BytesConsumed   Bytes consumed by the controller, optional.
  @param[out] BytesTotal      Total size of the playback request in bytes, optional.
  @param[out] Playing         Whether a playback request is currently running, optional.

  @retval EFI_SUCCESS           The progress was retrieved.
  @retval EFI_INVALID_PARAMETER This is NULL, or all output parameters are NULL.
  @retval EFI_NOT_READY         The output stream is not available yet.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_PROGRESS_GET_POSITION)(
  IN  EFI_AUDIO_PROGRESS_PROTOCOL  *This,
  OUT UINT32                       *BytesConsumed  OPTIONAL,
  OUT UINT32                       *BytesTotal     OPTIONAL,
  OUT BOOLEAN                      *Playing        OPTIONAL
  );

/**
  Gets the format of the most recent playback request.

  Together with EFI_AUDIO_PROGRESS_GET_POSITION this converts byte counts into
  a time position:

    ElapsedMs = BytesConsumed * 1000 / (SampleRate * Channels * BitsPerSample / 8)

  That needs 64-bit arithmetic once BytesConsumed exceeds 4 MB, which is about
  22 seconds at 48 kHz stereo 16-bit.

  The format is cached per codec when playback is set up, so an instance
  reports the format requested through its own EFI_AUDIO_IO_PROTOCOL.

  @param[in]  This            A pointer to the EFI_AUDIO_PROGRESS_PROTOCOL instance.
  @param[out] SampleRate      Sample rate in hertz, optional.
  @param[out] Channels        Channel count, optional.
  @param[out] BitsPerSample   Bits per sample, optional.

  @retval EFI_SUCCESS           The format was retrieved.
  @retval EFI_INVALID_PARAMETER This is NULL, or all output parameters are NULL.
  @retval EFI_NOT_READY         No playback has been set up for this codec yet.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_PROGRESS_GET_FORMAT)(
  IN  EFI_AUDIO_PROGRESS_PROTOCOL  *This,
  OUT UINT32                       *SampleRate     OPTIONAL,
  OUT UINT8                        *Channels       OPTIONAL,
  OUT UINT8                        *BitsPerSample  OPTIONAL
  );

/**
  Protocol struct.

  Revision comes first, as in EFI_AUDIO_IO_PROTOCOL, so that a consumer can
  confirm it is reading the layout it was built against before calling through
  the function pointers.
**/
struct EFI_AUDIO_PROGRESS_PROTOCOL_ {
  UINTN                              Revision;
  EFI_AUDIO_PROGRESS_GET_POSITION    GetPosition;
  EFI_AUDIO_PROGRESS_GET_FORMAT      GetFormat;
};

extern EFI_GUID  gEfiAudioProgressProtocolGuid;

#endif // EFI_AUDIO_PROGRESS_H
