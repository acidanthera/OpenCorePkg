/** @file
  Entry point to a PEIM.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiPei.h>


#include <Library/PeimEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

/**
  The entry point of PE/COFF Image for a PEIM.

  This function is the entry point for a PEIM.  This function must call ProcessLibraryConstructorList()
  and ProcessModuleEntryPointList().  The return value from ProcessModuleEntryPointList() is returned.
  If _gPeimRevision is not zero and PeiServices->Hdr.Revision is less than _gPeimRevison, then ASSERT().

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval  EFI_SUCCESS   The PEIM executed normally.
  @retval  !EFI_SUCCESS  The PEIM failed to execute normally.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPointReal (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  if (_gPeimRevision != 0) {
    //
    // Make sure that the PEI spec revision of the platform is >= PEI spec revision of the driver
    //
    ASSERT ((*PeiServices)->Hdr.Revision >= _gPeimRevision);
  }

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (FileHandle, PeiServices);

  //
  // Call the driver entry point
  //
  return ProcessModuleEntryPointList (FileHandle, PeiServices);
}


/**
  Required by the EBC compiler and identical in functionality to _ModuleEntryPoint().

  This function is required to call _ModuleEntryPoint() passing in FileHandle and PeiServices.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS  The PEIM executed normally.
  @retval !EFI_SUCCESS The PEIM failed to execute normally.

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  return _ModuleEntryPoint (FileHandle, PeiServices);
}
