/** @file
  Entry point to a EFI/DXE driver.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/



#include <Uefi.h>

#include <Protocol/LoadedImage.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>

/**
  Unloads an image from memory.

  This function is a callback that a driver registers to do cleanup
  when the UnloadImage boot service function is called.

  @param  ImageHandle The handle to the image to unload.

  @return Status returned by all unload().

**/
EFI_STATUS
EFIAPI
_DriverUnloadHandler (
  EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS  Status;

  //
  // If an UnloadImage() handler is specified, then call it
  //
  Status = ProcessModuleUnloadList (ImageHandle);

  //
  // If the driver specific unload handler does not return an error, then call all of the
  // library destructors.  If the unload handler returned an error, then the driver can not be
  // unloaded, and the library destructors should not be called
  //
  if (!EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (ImageHandle, gST);
  }

  //
  // Return the status from the driver specific unload handler
  //
  return Status;
}


/**
  The entry point of PE/COFF Image for a DXE Driver, DXE Runtime Driver, DXE SMM
  Driver, or UEFI Driver.

  This function is the entry point for a DXE Driver, DXE Runtime Driver, DXE SMM Driver,
  or UEFI Driver.  This function must call ProcessLibraryConstructorList() and
  ProcessModuleEntryPointList(). If the return status from ProcessModuleEntryPointList()
  is an error status, then ProcessLibraryDestructorList() must be called. The return
  value from ProcessModuleEntryPointList() is returned. If _gDriverUnloadImageCount
  is greater than zero, then an unload handler must be registered for this image
  and the unload handler must invoke ProcessModuleUnloadList().
  If _gUefiDriverRevision is not zero and SystemTable->Hdr.Revision is less than
  _gUefiDriverRevison, then return EFI_INCOMPATIBLE_VERSION.


  @param  ImageHandle  The image handle of the DXE Driver, DXE Runtime Driver,
                       DXE SMM Driver, or UEFI Driver.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The DXE Driver, DXE Runtime Driver, DXE SMM
                                     Driver, or UEFI Driver exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than
                                    SystemTable->Hdr.Revision.
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
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  if (_gUefiDriverRevision != 0) {
    //
    // Make sure that the EFI/UEFI spec revision of the platform is >= EFI/UEFI spec revision of the driver
    //
    if (SystemTable->Hdr.Revision < _gUefiDriverRevision) {
      return EFI_INCOMPATIBLE_VERSION;
    }
  }

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (ImageHandle, SystemTable);

  //
  //  Install unload handler...
  //
  if (_gDriverUnloadImageCount != 0) {
    Status = gBS->HandleProtocol (
                    ImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    ASSERT_EFI_ERROR (Status);
    LoadedImage->Unload = _DriverUnloadHandler;
  }

  //
  // Call the driver entry point
  //
  Status = ProcessModuleEntryPointList (ImageHandle, SystemTable);

  //
  // If all of the drivers returned errors, then invoke all of the library destructors
  //
  if (EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (ImageHandle, SystemTable);
  }

  //
  // Return the cummalative return status code from all of the driver entry points
  //
  return Status;
}


/**
  Required by the EBC compiler and identical in functionality to _ModuleEntryPoint().

  This function is required to call _ModuleEntryPoint() passing in ImageHandle,
  and SystemTable.

  @param  ImageHandle  The image handle of the DXE Driver, DXE Runtime Driver, DXE
                       SMM Driver, or UEFI Driver.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The DXE Driver, DXE Runtime Driver, DXE SMM
                                     Driver, or UEFI Driver exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than
                                     SystemTable->Hdr.Revision.
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
