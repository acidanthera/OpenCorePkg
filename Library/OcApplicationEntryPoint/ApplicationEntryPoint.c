/** @file
  Entry point library instance to a UEFI application.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>

/**
  Entry point to UEFI Application.

  This function is the entry point for a UEFI Application. This function must call
  ProcessLibraryConstructorList(), ProcessModuleEntryPointList(), and ProcessLibraryDestructorList().
  The return value from ProcessModuleEntryPointList() is returned.
  If _gUefiDriverRevision is not zero and SystemTable->Hdr.Revision is less than _gUefiDriverRevison,
  then return EFI_INCOMPATIBLE_VERSION.

  @param  ImageHandle                The image handle of the UEFI Application.
  @param  SystemTable                A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The UEFI Application exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than SystemTable->Hdr.Revision.
  @retval  Other                     Return value from ProcessModuleEntryPointList().

**/
EFI_STATUS
EFIAPI
_ModuleEntryPointReal (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;

  if (_gUefiDriverRevision != 0) {
    //
    // Make sure that the EFI/UEFI spec revision of the platform is >= EFI/UEFI spec revision of the application.
    //
    if (SystemTable->Hdr.Revision < _gUefiDriverRevision) {
      return EFI_INCOMPATIBLE_VERSION;
    }
  }

  //
  // Call constructor for all libraries.
  //
  ProcessLibraryConstructorList (ImageHandle, SystemTable);

  //
  // Call the module's entry point
  //
  Status = ProcessModuleEntryPointList (ImageHandle, SystemTable);

  //
  // Process destructor for all libraries.
  //
  ProcessLibraryDestructorList (ImageHandle, SystemTable);

  //
  // Return the return status code from the driver entry point
  //
  return Status;
}


/**
  Invokes the library destructors for all dependent libraries and terminates
  the UEFI Application.

  This function calls ProcessLibraryDestructorList() and the EFI Boot Service Exit()
  with a status specified by Status.

  @param  Status  Status returned by the application that is exiting.

**/
VOID
EFIAPI
Exit (
  IN EFI_STATUS  Status
  )

{
  ProcessLibraryDestructorList (gImageHandle, gST);

  gBS->Exit (gImageHandle, Status, 0, NULL);
}


/**
  Required by the EBC compiler and identical in functionality to _ModuleEntryPoint().

  @param  ImageHandle  The image handle of the UEFI Application.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The UEFI Application exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than SystemTable->Hdr.Revision.
  @retval  Other                     Return value from ProcessModuleEntryPointList().

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return _ModuleEntryPoint (ImageHandle, SystemTable);
}
