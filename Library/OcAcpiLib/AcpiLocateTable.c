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

// AcpiLocateTable
/**

  @param[in]      Signature
  @param[in, out] Table
  @param[in, out] Handle
  @param[in, out] Version

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_INVALID_PARAMETER
**/
EFI_STATUS
AcpiLocateTable (
  IN     UINT32                       Signature,
  IN OUT EFI_ACPI_DESCRIPTION_HEADER  **Table,
  IN OUT UINTN                        *Handle,
  IN OUT EFI_ACPI_TABLE_VERSION       *Version
  )
{
  EFI_ACPI_SUPPORT_PROTOCOL *AcpiSupport;

  EFI_STATUS  Status;
  INTN        Index;

  AcpiSupport = NULL;
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSupportProtocolGuid,
                  NULL,
                  (VOID **)&AcpiSupport
                  );

  if (!EFI_ERROR (Status)) {
    Index = 0;

    // Iterate The Tables To Find Matching Table

    do {
      Status = AcpiSupport->GetAcpiTable (
                              AcpiSupport,
                              Index,
                              (VOID **)Table,
                              Version,
                              Handle
                              );

      if (Status == EFI_NOT_FOUND) {
        break;
      }

      ++Index;
    } while ((*Table)->Signature != Signature);
  }

  return Status;
}
