/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_FIRMWARE_RUNTIME_PROTOCOL_H
#define OC_FIRMWARE_RUNTIME_PROTOCOL_H

#define OC_FIRMWARE_RUNTIME_REVISION 1

//
// OC_FIRMWARE_RUNTIME_PROTOCOL_GUID
// 9C820F96-F16C-4FFD-B266-DF0A8FDFC455
//
#define OC_FIRMWARE_RUNTIME_PROTOCOL_GUID   \
  { 0x9C820F96, 0xF16C, 0x4FFD,             \
    { 0xB2, 0x66, 0xDF, 0x0A, 0x8F, 0xDF, 0xC4, 0x55 } }

//
// Set NVRAM routing, returns previous value.
//
typedef
BOOLEAN
EFIAPI
(*OC_FWRT_NVRAM_REDIRECT) (
  IN BOOLEAN  NewValue
  );

//
// Set GetVariable override for customising values.
//
typedef
EFI_STATUS
EFIAPI
(*OC_FWRT_ON_GET_VARIABLE) (
  IN  EFI_GET_VARIABLE  GetVariable,
  OUT EFI_GET_VARIABLE  *OrgGetVariable  OPTIONAL
  );

//
// Check for revision to ensure binary compatibility.
//
typedef struct {
  UINTN                    Revision;
  OC_FWRT_NVRAM_REDIRECT   SetNvram;
  OC_FWRT_ON_GET_VARIABLE  OnGetVariable;
} OC_FIRMWARE_RUNTIME_PROTOCOL;

extern EFI_GUID gOcFirmwareRuntimeProtocolGuid;

#endif // OC_FIRMWARE_RUNTIME_PROTOCOL_H
