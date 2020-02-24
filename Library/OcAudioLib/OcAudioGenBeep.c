/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

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
#include <Library/OcAudioLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/AppleHda.h>
#include <Protocol/AppleBeepGen.h>
#include <Protocol/AppleVoiceOver.h>

#include "OcAudioInternal.h"

#define OC_BEEP_AUDIO_FREQUENCY  EfiAudioIoFreq44kHz
#define OC_BEEP_AUDIO_BITS       EfiAudioIoBits16
#define OC_BEEP_AUDIO_CHANNELS   1
#define OC_BEEP_AUDIO_LENGTH     3810 ///< Microseconds

//
// TODO: This is a sine curve, need to properly approximate it.
//
STATIC INT16 mAudioBeepSignal[] = {
  -1504, -5272, -10116, -15172, -19368, -22302, -23904, -24295, -23519, -21508, -17967, -13030, -7321, -1404, 4370, 10050,
  15890, 21430, 25688, 27981, 28493, 28201, 27670, 26490, 24152, 20977, 17602, 14654, 12061, 9200, 5857, 2474, -527, -3031,
  -5103, -6973, -8623, -9690, -10039, -9923, -9535, -9031, -8553, -7876, -6875, -5608, -4026, -2370, -1121, -336, 131, 367,
  257, -396, -1404, -2396, -3258, -3866, -4198, -4613, -5018, -5082, -4812, -4197, -3242, -2204, -1307, -473, 470, 1514, 2245,
  2218, 1336, -32, -971, -1076, -936, -1275, -2143, -3144, -3688, -3565, -3090, -2382, -1042, 1532, 5614, 10709, 15757, 19722,
  22252, 23435, 23553, 22752, 20692, 17020, 12007, 6316, 570, -4909, -10447, -16295, -21831, -25958, -27987, -28310, -28113,
  -27797, -26762, -24515, -21306, -17897, -14966, -12300, -9235, -5690, -2042, 1325, 4035, 6121, 7910, 9433, 10360, 10655,
  10410, 9832, 9220, 8653, 7880, 6845, 5421, 3623, 1968, 898, 333, 19, -146, 97, 919, 1945, 2814, 3448, 3795, 4155, 4605,
  4751, 4500, 3993, 3218, 2196, 1248, 385, -354, -1161, -1758, -1857, -1105, 254, 1642, 2095, 1912, 1827, 2309, 2993, 3416,
  3203, 2549, 1911
};

EFI_STATUS
EFIAPI
InternalOcAudioGenBeep (
  IN UINT32  ToneCount,
  IN UINTN   ToneLength,
  IN UINTN   SilenceLength
  )
{
  EFI_STATUS                 Status;
  OC_AUDIO_PROTOCOL          *OcAudio;
  OC_AUDIO_PROTOCOL_PRIVATE  *Private;
  UINTN                      ToneSlots;
  UINTN                      Index;
  UINTN                      BufferSize;
  UINT8                      *Buffer;
  UINT8                      *BufferPtr;

  DEBUG ((DEBUG_INFO, "OCAU: Beep %u %u/%u\n", ToneCount, (UINT32) ToneLength, (UINT32) SilenceLength));

  if (ToneCount == 0 || (ToneLength == 0 && SilenceLength == 0)) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (
    &gOcAudioProtocolGuid,
    NULL,
    (VOID **) &OcAudio
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAU: Beep has missing OcAudio protocol - %r\n", Status));
    return EFI_ABORTED;
  }

  Private = OC_AUDIO_PROTOCOL_PRIVATE_FROM_OC_AUDIO (OcAudio);
  if (Private->AudioIo == NULL) {
    DEBUG ((DEBUG_INFO, "OCAU: Beep has AudioIo not configured\n"));
    return EFI_ABORTED;
  }

  //
  // Convert to microseconds.
  //
  if (OcOverflowMulUN (ToneLength, 1000, &ToneLength)
    || OcOverflowMulUN (SilenceLength, 1000, &SilenceLength)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Wait for enough microseconds if no signals are given.
  //
  if (ToneLength == 0) {
    for (Index = 0; Index < ToneCount; ++Index) {
      gBS->Stall (SilenceLength);
    }
    return EFI_SUCCESS;
  }

  //
  // Rough signal time calculation, not sure whether we need to overcomplicate things right now.
  //
  ToneSlots = MAX (ToneLength / OC_BEEP_AUDIO_LENGTH, 1);
  if (OcOverflowMulUN (ToneSlots, sizeof (mAudioBeepSignal), &BufferSize)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // TODO: Remove this piece of junk and fix SetupPlayback performance.
  //
  Buffer = AllocatePool (BufferSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BufferPtr = Buffer;
  for (Index = 0; Index < ToneSlots; ++Index) {
    CopyMem (BufferPtr, &mAudioBeepSignal[0], sizeof (mAudioBeepSignal));
    BufferPtr += sizeof (mAudioBeepSignal);
  }

  Private->OcAudio.StopPlayback (&Private->OcAudio, TRUE);

  Status = Private->AudioIo->SetupPlayback (
    Private->AudioIo,
    Private->OutputIndex,
    Private->Volume,
    OC_BEEP_AUDIO_FREQUENCY,
    OC_BEEP_AUDIO_BITS,
    OC_BEEP_AUDIO_CHANNELS
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAU: Beep playback setup failure - %r\n", Status));
    FreePool (Buffer);
    return EFI_ABORTED;
  }

  for (Index = 0; Index < ToneCount; ++Index) {
    Status = Private->AudioIo->StartPlayback (
      Private->AudioIo,
      Buffer,
      BufferSize,
      0
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAU: Beep playback failure - %r\n", Status));
      break;
    }

    if (SilenceLength > 0) {
      gBS->Stall (SilenceLength);
    }
  }

  FreePool (Buffer);

  return Status;
}
