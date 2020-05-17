/** @file
Copyright (C) 2014 - 2016, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_BEEP_GEN_H
#define APPLE_BEEP_GEN_H

/**
  Apple Beep Generator protocol
  C32332DF-FC56-4FE1-9358-BA0D529B24CD
**/
#define APPLE_BEEP_GEN_PROTOCOL_GUID                 \
  { 0xC32332DF, 0xFC56, 0x4FE1,                      \
    { 0x93, 0x58, 0xBA, 0x0D, 0x52, 0x9B, 0x24, 0xCD } }

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

/**
  Generate cycles of beep signals with silence afterwards, blocking.

  @param[in] ToneCount      Number of signals to produce.
  @param[in] ToneLength     Signal length in milliseconds.
  @param[in] SilenceLength  Silence length in milliseconds.

  @retval EFI_SUCCESS after signal completion.
  @retval EFI_SUCCESS if ToneCount is 0.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_BEEP_GEN_BEEP) (
  IN UINT32  ToneCount,
  IN UINTN   ToneLength,
  IN UINTN   SilenceLength
  );

/**
  All protocol members are optional and can be NULL.
**/
typedef struct {
  APPLE_BEEP_GEN_BEEP  GenBeep; ///< Can be NULL.
} APPLE_BEEP_GEN_PROTOCOL;

extern EFI_GUID gAppleBeepGenProtocolGuid;

#endif // APPLE_BEEP_GEN_H
