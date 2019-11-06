/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_MISC_LIB_H
#define OC_MISC_LIB_H

#include <Uefi.h>
#include <Library/OcStringLib.h>

/**
  The size, in Bits, of one Byte.
**/
#define OC_CHAR_BIT  8

/**
  Convert seconds to microseconds for use in e.g. gBS->Stall.
**/
#define SECONDS_TO_MICROSECONDS(x) ((x)*1000000)

/**
  TODO: EDK II has its implementation in BaseLib, but it is broken,
  as it fails to update DecodedLength:
  https://github.com/acidanthera/bugtracker/issues/372

  @param[in] EncodedData        A pointer to the data to convert.
  @param[in] EncodedLength      The length of data to convert.
  @param[in] DecodedData        A pointer to location to store the decoded data.
  @param[in] DecodedSize        A pointer to location to store the decoded size.

  @retval  TRUE on success.
**/
RETURN_STATUS
EFIAPI
OcBase64Decode (
  IN     CONST CHAR8  *EncodedData,
  IN     UINTN        EncodedLength,
     OUT UINT8        *DecodedData,
  IN OUT UINTN        *DecodedLength
  );

/**
  Allocate new System Table with disabled text output.

  @param[in] SystemTable     Base System Table.

  @retval non NULL  The System Table table was allocated successfully.
**/
EFI_SYSTEM_TABLE *
AllocateNullTextOutSystemTable (
  EFI_SYSTEM_TABLE  *SystemTable
  );

INT32
FindPattern (
  IN CONST UINT8   *Pattern,
  IN CONST UINT8   *PatternMask OPTIONAL,
  IN CONST UINT32  PatternSize,
  IN CONST UINT8   *Data,
  IN UINT32        DataSize,
  IN INT32         DataOff
  );

UINT32
ApplyPatch (
  IN CONST UINT8   *Pattern,
  IN CONST UINT8   *PatternMask OPTIONAL,
  IN CONST UINT32  PatternSize,
  IN CONST UINT8   *Replace,
  IN CONST UINT8   *ReplaceMask OPTIONAL,
  IN UINT8         *Data,
  IN UINT32        DataSize,
  IN UINT32        Count,
  IN UINT32        Skip
  );

/**
  @param[in] Protocol    The published unique identifier of the protocol. It is the caller’s responsibility to pass in
                         a valid GUID.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
UninstallAllProtocolInstances (
  EFI_GUID  *Protocol
  );

/**
  Release UEFI ownership from USB controllers at booting.
**/
EFI_STATUS
ReleaseUsbOwnership (
  VOID
  );

/**
  Perform cold reboot directly bypassing UEFI services. Does not return.
  Supposed to work in any modern physical or virtual environment.
**/
VOID
DirectRestCold (
  VOID
  );

/**
 Return the result of (Multiplicand * Multiplier / Divisor).

 @param Multiplicand A 64-bit unsigned value.
 @param Multiplier   A 64-bit unsigned value.
 @param Divisor      A 32-bit unsigned value.
 @param Remainder    A pointer to a 32-bit unsigned value. This parameter is
 optional and may be NULL.

 @return Multiplicand * Multiplier / Divisor.
 **/
UINT64
MultThenDivU64x64x32 (
  IN  UINT64  Multiplicand,
  IN  UINT64  Multiplier,
  IN  UINT32  Divisor,
  OUT UINT32  *Remainder  OPTIONAL
  );

#endif // OC_MISC_LIB_H
