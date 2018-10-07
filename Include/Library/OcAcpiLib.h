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

#ifndef OC_ACPI_LIB_H_
#define OC_ACPI_LIB_H_

// TODO: remove this nasty temporary workaround

#include <Protocol/AcpiSupport.h>

// AcpiFindLegacyRsdPtr
/** Find RSD_PTR Table In Legacy Area

  @retval EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER
**/
EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER *
AcpiFindLegacyRsdPtr (
  VOID
  );

// AcpiFindRsdPtr
/** Find RSD_PTR Table From System Configuration Tables

  @retval EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER
**/
EFI_ACPI_6_0_ROOT_SYSTEM_DESCRIPTION_POINTER *
AcpiFindRsdPtr (
  VOID
  );

// AcpiLocateTable
/** Locate an existing ACPI table.

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
  );

#endif // OC_ACPI_LIB_H_
