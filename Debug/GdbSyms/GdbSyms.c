/** @file

  Bare-minimum GDB symbols needed for reloading symbols.

  This is not a "driver" and should not be placed in a FD.

  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiDxe.h"

#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Guid/DebugImageInfoTable.h>

/**
  Main entry point.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       Successfully initialized.

**/
EFI_STATUS
EFIAPI
Initialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_SYSTEM_TABLE_POINTER ESTP;
  EFI_DEBUG_IMAGE_INFO_TABLE_HEADER EDIITH;
  EFI_IMAGE_DOS_HEADER EIDH;
  EFI_IMAGE_OPTIONAL_HEADER_UNION EIOHU;
  EFI_IMAGE_SECTION_HEADER EISH;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY EIDDE;
  EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY EIDCNE;
  EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY EIDCRE;
  EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY EIDCME;
  UINTN Dummy =
    (UINTN) &ESTP   |
    (UINTN) &EDIITH |
    (UINTN) &EIDH   |
    (UINTN) &EIOHU  |
    (UINTN) &EISH   |
    (UINTN) &EIDDE  |
    (UINTN) &EIDCNE |
    (UINTN) &EIDCRE |
    (UINTN) &EIDCME |
     1
    ;
  DEBUG ((DEBUG_VERBOSE, "%a: %llx\n", __FUNCTION__, &ESTP));
  DEBUG ((DEBUG_VERBOSE, "%a: %llx\n", __FUNCTION__, &EDIITH));
  DEBUG ((DEBUG_VERBOSE, "%a: %llx\n", __FUNCTION__, &EIDH));
  DEBUG ((DEBUG_VERBOSE, "%a: %llx\n", __FUNCTION__, &EIOHU));
  DEBUG ((DEBUG_VERBOSE, "%a: %llx\n", __FUNCTION__, &EISH));
  DEBUG ((DEBUG_VERBOSE, "%a: %llx\n", __FUNCTION__, &EIDDE));
  DEBUG ((DEBUG_VERBOSE, "%a: %llx\n", __FUNCTION__, &EIDCNE));
  DEBUG ((DEBUG_VERBOSE, "%a: %llx\n", __FUNCTION__, &EIDCRE));
  DEBUG ((DEBUG_VERBOSE, "%a: %llx\n", __FUNCTION__, &EIDCME));
  return !!Dummy & EFI_SUCCESS;
}
