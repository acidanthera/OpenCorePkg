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

#include <Uefi.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

EFI_MEMORY_DESCRIPTOR *
GetCurrentMemoryMap (
  OUT UINTN   *MemoryMapSize,
  OUT UINTN   *DescriptorSize,
  OUT UINTN   *MapKey             OPTIONAL,
  OUT UINT32  *DescriptorVersion  OPTIONAL
  )
{
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;
  EFI_STATUS              Status;
  UINTN                   MapKeyValue;
  UINT32                  DescriptorVersionValue;
  BOOLEAN                 Result;

  *MemoryMapSize = 0;
  Status = gBS->GetMemoryMap (
    MemoryMapSize,
    NULL,
    &MapKeyValue,
    DescriptorSize,
    &DescriptorVersionValue
    );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return NULL;
  }

  //
  // Apple uses 1024 as constant, however it will grow by at least
  // DescriptorSize.
  //
  Result = OcOverflowAddUN (
    *MemoryMapSize,
    MAX (*DescriptorSize, 1024),
    MemoryMapSize
    );

  if (Result) {
    return NULL;
  }

  MemoryMap = AllocatePool (*MemoryMapSize);
  if (MemoryMap == NULL) {
    return NULL;
  }

  Status = gBS->GetMemoryMap (
    MemoryMapSize,
    MemoryMap,
    &MapKeyValue,
    DescriptorSize,
    &DescriptorVersionValue
    );

  if (EFI_ERROR (Status)) {
    FreePool (MemoryMap);
    return NULL;
  }

  if (MapKey != NULL) {
    *MapKey = MapKeyValue;
  }

  if (DescriptorVersion != NULL) {
    *DescriptorVersion = DescriptorVersionValue;
  }

  return MemoryMap;
}
