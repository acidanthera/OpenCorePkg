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

#ifndef OC_MISC_LIB_H_
#define OC_MISC_LIB_H_

// PLATFORM_DATA_HEADER
typedef struct {
  struct {
    CHAR16 Reserved[8];
    UINT32 KeySize;
    UINT32 DataSize;
  } Hdr;
} PLATFORM_DATA_HEADER;

// Base64Decode
/**

  @param[in] EncodedData        A pointer to the data to convert.
  @param[in] EncodedDataLength  The length of data to convert.
  @param[in] DecodedSize        A pointer to location to store the decoded size.

  @retval  An ascii string representing the data.
**/
UINT8 *
Base64Decode (
  IN  CHAR8  *EncodedData,
  IN  UINTN  EncodedDataLength,
  OUT UINTN  *DecodedSize
  );

// ConvertDataToString
/** Attempt to convert the data into an ascii string.

  @param[in] Data      A pointer to the data to convert.
  @param[in] DataSize  The length of data to convert.

  @retval  An ascii string representing the data.
**/
CHAR8 *
ConvertDataToString (
  IN VOID   *Data,
  IN UINTN  DataSize
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

#endif // OC_MISC_LIB_H_
