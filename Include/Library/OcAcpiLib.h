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

#ifndef OC_ACPI_LIB_H
#define OC_ACPI_LIB_H

#include <IndustryStandard/Acpi62.h>

//
// RSDP and XSDT table definitions not provided by EDK2 due to no
// flexible array support.
//

#pragma pack(push, 1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT32                       Tables[];
} OC_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER  Header;
  UINT64                       Tables[];
} OC_ACPI_6_2_EXTENDED_SYSTEM_DESCRIPTION_TABLE;

#pragma pack(pop)

typedef struct {
  //
  // Pointer to original RSDP table.
  //
  EFI_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_POINTER   *Rsdp;
  //
  // Pointer to active RSDT table.
  //
  OC_ACPI_6_2_ROOT_SYSTEM_DESCRIPTION_TABLE      *Rsdt;
  //
  // Pointer to active XSDT table.
  //
  OC_ACPI_6_2_EXTENDED_SYSTEM_DESCRIPTION_TABLE  *Xsdt;
  //
  // Current list of tables allocated from heap.
  //
  EFI_ACPI_COMMON_HEADER                         **Tables;
  //
  // Number of tables.
  //
  UINT32                                         NumberOfTables;
  //
  // Number of allocated table slots.
  //
  UINT32                                         AllocatedTables;
} OC_ACPI_CONTEXT;

/** Find ACPI System Tables for later table configuration.

  @param Context  ACPI library context.

  @retval EFI_SUCCESS when Rsdp and Xsdt or Rsdt are found.
**/
EFI_STATUS
AcpiInitContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/** Free ACPI context dynamic resources.

  @param Context  ACPI library context.
**/
VOID
AcpiFreeContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/** Apply ACPI context to this system.

  @param Context  ACPI library context.
**/
EFI_STATUS
AcpiApplyContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/** Drop one ACPI table.

  @param Context     ACPI library context
  @param Signature   Table signature or 0.
  @param Length      Table length or 0.
  @param OemTableId  Table Id or 0.
**/
EFI_STATUS
AcpiDropTable (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     UINT32           Signature,
  IN     UINT32           Length,
  IN     UINT64           OemTableId
  );

/** Install one ACPI table. For DSDT this performs table replacement.

  @param Context     ACPI library context
  @param Data        Table data.
  @param Length      Table length.
**/
EFI_STATUS
AcpiInsertTable (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     CONST UINT8      *Data,
  IN     UINT32           Length
  );

/** Normalise ACPI headers to contain 7-bit ASCII.

  @param Context     ACPI library context
**/
VOID
AcpiNormalizeHeaders (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

#endif // OC_ACPI_LIB_H
