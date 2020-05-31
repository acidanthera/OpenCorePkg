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
#include <Protocol/SimpleFileSystem.h>

#define OC_ACPI_NAME_SIZE 4

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

//
// Operation region structure.
//
typedef struct {
  //
  // Region address.
  //
  UINT32  Address;
  //
  // Region name, not guaranteed to be null-terminated.
  //
  CHAR8   Name[OC_ACPI_NAME_SIZE+1];
} OC_ACPI_REGION;

//
// Main ACPI context describing current tableset worked on.
//
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
  // Pointer to active FADT table.
  //
  EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE      *Fadt;
  //
  // Pointer to active DSDT table.
  //
  EFI_ACPI_DESCRIPTION_HEADER                    *Dsdt;
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
  //
  // Detected operation regions if any.
  //
  OC_ACPI_REGION                                 *Regions;
  //
  // Number of regions.
  //
  UINT32                                         NumberOfRegions;
  //
  // Number of allocated region slots.
  //
  UINT32                                         AllocatedRegions;
} OC_ACPI_CONTEXT;

//
// ACPI patch structure.
//
typedef struct {
  //
  // Find bytes.
  //
  CONST UINT8  *Find;
  //
  // Replace bytes.
  //
  CONST UINT8  *Replace;
  //
  // Find mask or NULL.
  //
  CONST UINT8  *Mask;
  //
  // Replace mask or NULL.
  //
  CONST UINT8  *ReplaceMask;
  //
  // Patch size.
  //
  UINT32       Size;
  //
  // Replace count or 0 for all.
  //
  UINT32       Count;
  //
  // Skip count or 0 to start from 1 match.
  //
  UINT32       Skip;
  //
  // Limit replacement size to this value or 0, which assumes table size.
  //
  UINT32       Limit;
  //
  // ACPI Table signature or 0.
  //
  UINT32       TableSignature;
  //
  // ACPI Table length or 0.
  //
  UINT32       TableLength;
  //
  // ACPI Table Id or 0.
  //
  UINT64       OemTableId;
} OC_ACPI_PATCH;

/**
  Find ACPI System Tables for later table configuration.

  @param[in,out] Context  ACPI library context.

  @return EFI_SUCCESS when Rsdp and Xsdt or Rsdt are found.
**/
EFI_STATUS
AcpiInitContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/**
  Free ACPI context dynamic resources.

  @param[in,out] Context  ACPI library context.
**/
VOID
AcpiFreeContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/**
  Apply ACPI context to this system.

  @param[in,out] Context  ACPI library context.
**/
EFI_STATUS
AcpiApplyContext (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/**
  Delete one ACPI table.

  @param[in,out] Context     ACPI library context.
  @param[in]     Signature   Table signature or 0.
  @param[in]     Length      Table length or 0.
  @param[in]     OemTableId  Table Id or 0.
  @param[in]     All         Delete all tables or first matched.
**/
EFI_STATUS
AcpiDeleteTable (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     UINT32           Signature,
  IN     UINT32           Length,
  IN     UINT64           OemTableId,
  IN     BOOLEAN          All
  );

/**
  Install one ACPI table. For DSDT this performs table replacement.

  @param[in,out] Context     ACPI library context.
  @param[in]     Data        Table data.
  @param[in]     Length      Table length.
**/
EFI_STATUS
AcpiInsertTable (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     CONST UINT8      *Data,
  IN     UINT32           Length
  );

/**
  Normalise ACPI headers to contain 7-bit ASCII.

  @param[in,out] Context     ACPI library context.
**/
VOID
AcpiNormalizeHeaders (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/**
  Patch ACPI tables.

  @param[in,out] Context     ACPI library context.
  @param[in]     Patch       ACPI patch.
**/
EFI_STATUS
AcpiApplyPatch (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     OC_ACPI_PATCH    *Patch
  );

/**
  Try to load ACPI regions.

  @param[in,out] Context     ACPI library context.
**/
EFI_STATUS
AcpiLoadRegions (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/**
  Attempt to relocate ACPI regions based on loaded ones.

  @param[in,out] Context     ACPI library context.
**/
VOID
AcpiRelocateRegions (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/**
  Upgrade FADT to support reset register needed on very
  old legacy hardware.

  @param[in,out] Context     ACPI library context.

  @return EFI_SUCCESS if successful or not needed.
**/
EFI_STATUS
AcpiFadtEnableReset (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/**
  Reset BGRT Displayed status.

  @param[in,out] Context     ACPI library context.
**/
VOID
AcpiResetLogoStatus (
  IN OUT OC_ACPI_CONTEXT  *Context
  );

/**
  Log and reset FACS hardware signature.

  @param[in,out] Context     ACPI library context.
  @param[in]     Reset       Perform reset.
**/
VOID
AcpiHandleHardwareSignature (
  IN OUT OC_ACPI_CONTEXT  *Context,
  IN     BOOLEAN          Reset
  );

/**
  Dump ACPI tables to the specified directory.

  @param[in] Root  Directory to write tables.
**/
EFI_STATUS
AcpiDumpTables (
  IN EFI_FILE_PROTOCOL  *Root
  );

#endif // OC_ACPI_LIB_H
