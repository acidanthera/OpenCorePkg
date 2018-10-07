/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/AcpiAml.h>

#include <Guid/Acpi.h>

#include <Protocol/AcpiSupport.h>

#include <Library/OcAcpiLib.h>

#include <Macros.h>

// AcpiFindLegacyRsdPtr
/** Find RSD_PTR Table In Legacy Area

  @retval EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER
**/
EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER *
AcpiFindLegacyRsdPtr (
  VOID
  )
{
  EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER *RsdPtr;

  UINTN                                        Address;
  UINTN                                        Index;

  // First Search 0x0E0000 - 0x0FFFFF for RSD_PTR

  RsdPtr = NULL;

  for (Address = 0x0E0000; Address < 0x0FFFFF; Address += 16) {
    if (*(UINT64 *)Address == EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
      RsdPtr = (EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)Address;
    }
  }

  // Then Search EBDA 0x40E - 0x800

  if (RsdPtr == NULL) {
    Address = ((*(UINT16 *)(UINTN)0x040E) << 4);

    for (Index = 0; Index < 0x0400; Index += 16) {
      if (*(UINT64 *)(Address + Index) == EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE) {
        RsdPtr = (EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)Address;
      }
    }
  }

  return RsdPtr;
}
