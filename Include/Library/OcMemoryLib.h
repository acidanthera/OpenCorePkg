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

#ifndef OC_MEMORY_LIB_H
#define OC_MEMORY_LIB_H

/**
  Lock the legacy region specified to enable modification.

  @param[in] LegacyAddress  The address of the region to lock.
  @param[in] LegacyLength   The size of the region to lock.

  @retval EFI_SUCCESS  The region was locked successfully.
**/
EFI_STATUS
LegacyRegionLock (
  IN UINT32  LegacyAddress,
  IN UINT32  LegacyLength
  );

/**
  Unlock the legacy region specified to enable modification.

  @param[in] LegacyAddress  The address of the region to unlock.
  @param[in] LegacyLength   The size of the region to unlock.

  @retval EFI_SUCCESS  The region was unlocked successfully.
**/
EFI_STATUS
LegacyRegionUnlock (
  IN UINT32  LegacyAddress,
  IN UINT32  LegacyLength
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

#endif // OC_MEMORY_LIB_H
