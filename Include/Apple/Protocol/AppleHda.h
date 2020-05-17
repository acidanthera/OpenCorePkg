/** @file
  Apple High Definition Audio protocol.

Copyright (C) 2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_HIGH_DEFINITION_AUDIO_H
#define APPLE_HIGH_DEFINITION_AUDIO_H

#include <Protocol/PciIo.h>

/**
  Apple High Definition Audio protocol GUID.
  3224B169-EC34-46D2-B779-E1B1687F525F
**/
#define APPLE_HIGH_DEFINITION_AUDIO_PROTOCOL_GUID \
  { 0x3224B169, 0xEC34, 0x46D2,                   \
    { 0xB7, 0x79, 0xE1, 0xB1, 0x68, 0x7F, 0x52, 0x5F } }


#define APPLE_HIGH_DEFINITION_AUDIO_PROTOCOL_VERSION 0x40000

/**
  Note, ToneLength and SilenceLength were in microseconds for older models:
  - MacBookPro1,1
  - MacBookPro1,1
  - MacBookPro1,2
  - MacBookPro2,1
  - MacBookPro2,2
  - MacBookP1,1
  - MacBookP1,2
  - MacBookP2,1
  - MacBookP2,2
  - MacBook1,1
  - MacBook2,1
  - Macmini1,1
  - Macmini2,1
  - iMac4,1
  - iMac4,2
  - iMac5,1
  - iMac5,2
  - iMac6,1
  - MacPro1,1
  - MacPro2,1
  - XServe1,1
**/

typedef struct APPLE_HIGH_DEFINITION_AUDIO_PROTOCOL_ APPLE_HIGH_DEFINITION_AUDIO_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN) (
  VOID
  );

/**
  Generate cycles of beep signals with silence afterwards, blocking.

  @param[in] This           This protocol instance.
  @param[in] ToneCount      Number of signals to produce.
  @param[in] ToneLength     Signal length in milliseconds.
  @param[in] SilenceLength  Silence length in milliseconds.
  @param[in] Frequency      Tone frequency, up to 44100 Hz, can be 0 for default.

  @retval EFI_SUCCESS after signal completion.
  @retval EFI_SUCCESS if ToneCount or ToneLength is 0.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_HIGH_DEFINITION_AUDIO_PLAY_TONE) (
  IN APPLE_HIGH_DEFINITION_AUDIO_PROTOCOL  *This,
  IN UINT32                                ToneCount,
  IN UINTN                                 ToneLength,
  IN UINTN                                 SilenceLength,
  IN UINTN                                 Frequency
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_HIGH_DEFINITION_AUDIO_PLAY) (
  IN     APPLE_HIGH_DEFINITION_AUDIO_PROTOCOL  *This,
  IN OUT UINTN                                 *Arg1,
  IN OUT UINTN                                 *Arg2,
  IN OUT UINTN                                 *Arg3
  );

struct APPLE_HIGH_DEFINITION_AUDIO_PROTOCOL_ {
  UINTN                                  Revision;
  EFI_PCI_IO_PROTOCOL                    *PciIo;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown01;
  APPLE_HIGH_DEFINITION_AUDIO_PLAY_TONE  PlayTone; ///< Can be NULL.
  APPLE_HIGH_DEFINITION_AUDIO_PLAY       Play;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown04;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown05;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown06;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown07;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown08;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown09;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown10;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown11;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown12;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown13;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown14;
  APPLE_HIGH_DEFINITION_AUDIO_UNKNOWN    Unknown15;
};

extern EFI_GUID gAppleHighDefinitionAudioProtocolGuid;

#endif // APPLE_HIGH_DEFINITION_AUDIO_H
