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

// LegacyRegionlock
/** Lock the legacy region specified to enable modification.

  @param[in] LegacyAddress  The address of the region to lock.
  @param[in] LegacyLength   The size of the region to lock.

  @retval EFI_SUCCESS  The region was locked successfully.
**/
EFI_STATUS
LegacyRegionLock (
  IN UINT32  LegacyAddress,
  IN UINT32  LegacyLength
  );

// LegacyRegionUnlock
/** Unlock the legacy region specified to enable modification.

  @param[in] LegacyAddress  The address of the region to unlock.
  @param[in] LegacyLength   The size of the region to unlock.

  @retval EFI_SUCCESS  The region was unlocked successfully.
**/
EFI_STATUS
LegacyRegionUnlock (
  IN UINT32  LegacyAddress,
  IN UINT32  LegacyLength
  );

/** Log the boot options passed

  @param[in] BootOrder        A pointer to the boot order list.
  @param[in] BootOrderLength  Size of the boot order list.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
LogBootOrder (
  IN  INT16   *BootOrder,
  IN  UINTN   BootOrderSize
  );

// LogHexDump
/** Convert memory locations into hex strings and output to the boot log

  @param[in] Address       The address of the region to dump hex from.
  @param[in] Address2      The address to show when dumping hex.
  @param[in] Length        The length of the string to show.
  @param[in] LineSize      How many bytes to show per line.
  @param[in] DisplayAscii  Flag to show ascii charater also.

  @retval EFI_SUCCESS  The region was unlocked successfully.
**/
EFI_STATUS
LogHexDump (
  IN VOID     *Address,
  IN VOID     *Address2,
  IN UINTN    Length,
  IN UINTN    LineSize,
  IN BOOLEAN  DisplayAscii
  );

// SetPlatformData
/**

  @param[in] DataRecordGuid  The guid of the record to use.
  @param[in] Key             A pointer to the ascii key string.
  @param[in] Data            A pointer to the data to store.
  @param[in] DataSize        The length of the data to store.

  @retval EFI_SUCCESS  The datahub  was updated successfully.
**/
EFI_STATUS
SetPlatformData (
  IN EFI_GUID  *DataRecordGuid,
  IN CHAR8     *Key,
  IN VOID      *Data,
  IN UINT32    DataSize
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

/**
  Dummy function that debuggers may break on.
**/
VOID
DebugBreak (
  VOID
  );

/**
  Wait for user input after printing message.

  @param[in] Message   Message to print.
**/
VOID
WaitForKeyPress (
  CONST CHAR16 *Message
  );

/**
  Default index mapping macros.
**/
#define OC_INPUT_STR      "123456789ABCDEFGHIJKLMNOPQRSTUVXWZ"
#define OC_INPUT_MAX      L_STR_LEN (OC_INPUT_STR)
#define OC_INPUT_ABORTED  -1 ///< Esc or 0
#define OC_INPUT_INVALID  -2 ///< Some other key
#define OC_INPUT_TIMEOUT  -3 ///< Timeout

/**
  Obtains key index from user input.

  @param TimeOutSeconds  Timeout to wait for.

  @returns key index [0, OC_INPUT_MAX), OC_INPUT_ABORTED, or OC_INPUT_INVALID.
**/
INTN
WaitForKeyIndex (
  UINTN  TimeOutSeconds
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
  Get current memory map allocated on pool.

  @param[out]  MemoryMapSize      Resulting memory map size in bytes.
  @param[out]  DescriptorSize     Resulting memory map descriptor size in bytes.
  @param[out]  MapKey             Memory map key, optional.
  @param[out]  DescriptorVersion  Memory map version, optional.

  @retval current memory map or NULL.
**/
EFI_MEMORY_DESCRIPTOR *
GetCurrentMemoryMap (
  OUT UINTN   *MemoryMapSize,
  OUT UINTN   *DescriptorSize,
  OUT UINTN   *MapKey             OPTIONAL,
  OUT UINT32  *DescriptorVersion  OPTIONAL
  );

#endif // OC_MISC_LIB_H
