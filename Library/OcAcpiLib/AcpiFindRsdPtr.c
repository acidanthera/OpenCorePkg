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

// AcpiFindRsdPtr
/** Find RSD_PTR Table From System Configuration Tables

  @retval EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER
**/
EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER *
AcpiFindRsdPtr (
  VOID
  )
{
  EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER *RsdPtr;

  UINTN                                        Index;

  RsdPtr = NULL;

  // Find ACPI table RSD_PTR from system table

  for (Index = 0; Index < gST->NumberOfTableEntries; ++Index) {
    if (CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi20TableGuid)
     || CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi10TableGuid)
     || CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpiTableGuid)) {
      RsdPtr = gST->ConfigurationTable[Index].VendorTable;

      break;
    }
  }

  return RsdPtr;
}
