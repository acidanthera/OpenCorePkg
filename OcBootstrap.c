/** @file
  Bootstrap OpenCore driver.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenCore.h>
#include <Uefi.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OcBootstrap.h>
#include <Protocol/OcInterface.h>

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>

#include "GUI.h"
#include "BmfLib.h"
#include "GuiApp.h"

extern BOOT_PICKER_GUI_CONTEXT mGuiContext;

EFI_STATUS
OcShowMenuByOc (
  IN     BOOT_PICKER_GUI_CONTEXT  *GuiContext,
  IN OUT GUI_DRAWING_CONTEXT      *DrawContext,
  IN     OC_BOOT_ENTRY            *BootEntries,
  IN     UINTN                    Count,
  IN     UINTN                    DefaultEntry,
  OUT    OC_BOOT_ENTRY            **ChosenBootEntry
  )
{
  RETURN_STATUS Status;
  UINTN         Index;

  BootPickerEntriesEmpty ();

  for (Index = 0; Index < Count; ++Index) {
    Status = BootPickerEntriesAdd (
               GuiContext,
               BootEntries[Index].Name,
               &BootEntries[Index],
               BootEntries[Index].IsExternal,
               Index == DefaultEntry
               );
    if (RETURN_ERROR (Status)) {
      return Status;
    }
  }

  GuiDrawLoop (DrawContext);
  ASSERT (GuiContext->BootEntry != NULL);

  *ChosenBootEntry = GuiContext->BootEntry;
  return RETURN_SUCCESS;
}

RETURN_STATUS
InternalContextConstruct (
  OUT BOOT_PICKER_GUI_CONTEXT  *Context
  );

/**
  Add an entry to the log buffer

  @param[in] This          This protocol.
  @param[in] Storage       File system access storage.
  @param[in] Picker        User interface configuration.

  @retval does not return unless a fatal error happened.
**/
EFI_STATUS
EFIAPI
GuiOcInterfaceRun (
  IN OC_INTERFACE_PROTOCOL  *This,
  IN OC_STORAGE_CONTEXT     *Storage,
  IN OC_PICKER_CONTEXT      *Picker
  )
{
  EFI_STATUS                 Status;
  APPLE_BOOT_POLICY_PROTOCOL *AppleBootPolicy;
  OC_BOOT_ENTRY              *Chosen;
  OC_BOOT_ENTRY              *Entries;
  UINTN                      EntryCount;
  INTN                       DefaultEntry;

  GUI_DRAWING_CONTEXT        DrawContext;

  Status = GuiLibConstruct (0, 0);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = InternalContextConstruct (&mGuiContext);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = BootPickerViewInitialize (
             &DrawContext,
             &mGuiContext,
             InternalGetCursorImage
             );
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  AppleBootPolicy = OcAppleBootPolicyInstallProtocol (FALSE);
  if (AppleBootPolicy == NULL) {
    DEBUG ((DEBUG_ERROR, "OCB: AppleBootPolicy locate failure\n"));
    return EFI_NOT_FOUND;
  }

  while (TRUE) {
    DEBUG ((DEBUG_INFO, "OCB: Performing OcScanForBootEntries...\n"));

    Status = OcScanForBootEntries (
               AppleBootPolicy,
               Picker,
               &Entries,
               &EntryCount,
               NULL,
               TRUE
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCB: OcScanForBootEntries failure - %r\n", Status));
      return Status;
    }

    if (EntryCount == 0) {
      DEBUG ((DEBUG_WARN, "OCB: OcScanForBootEntries has no entries\n"));
      return EFI_NOT_FOUND;
    }

    DefaultEntry = OcGetDefaultBootEntry (Picker, Entries, EntryCount);
    Status = OcShowMenuByOc (
               &mGuiContext,
               &DrawContext,
               Entries,
               EntryCount,
               DefaultEntry,
               &Chosen
               );
    if (!EFI_ERROR (Status)) {
      OcLoadBootEntry (AppleBootPolicy, Picker, Chosen, gImageHandle);
      gBS->Stall (5000000);
    }

    OcFreeBootEntries (Entries, EntryCount);
  }
}

STATIC OC_INTERFACE_PROTOCOL mOcInterface = {
  OC_INTERFACE_REVISION,
  GuiOcInterfaceRun
};

STATIC
EFI_STATUS
LoadOpenCore (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  EFI_HANDLE                       ParentImageHandle,
  OUT EFI_HANDLE                       *ImageHandle
  )
{
  EFI_STATUS                 Status;
  VOID                       *Buffer;
  UINT32                     BufferSize;

  ASSERT (FileSystem != NULL);
  ASSERT (ParentImageHandle != NULL);
  ASSERT (ImageHandle != NULL);

  BufferSize = 0;
  Buffer = ReadFile (FileSystem, OPEN_CORE_IMAGE_PATH, &BufferSize, BASE_16MB);
  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "BS: Failed to locate valid OpenCore image - %p!\n", Buffer));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "BS: Read OpenCore image of %Lu bytes\n", (UINT64) BufferSize));

  //
  // Run OpenCore image
  //
  *ImageHandle = NULL;
  Status = gBS->LoadImage (
    FALSE,
    ParentImageHandle,
    NULL,
    Buffer,
    BufferSize,
    ImageHandle
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BS: Failed to load OpenCore image - %r\n", Status));
    FreePool (Buffer);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "BS: Loaded OpenCore image at %p handle\n", *ImageHandle));

  Status = gBS->StartImage (
    *ImageHandle,
    NULL,
    NULL
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "BS: Failed to start OpenCore image - %r\n", Status));
    gBS->UnloadImage (*ImageHandle);
  }

  FreePool (Buffer);

  return Status;
}

STATIC
VOID
StartOpenCore (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *FileSystem,
  IN EFI_HANDLE                        LoadHandle,
  IN EFI_DEVICE_PATH_PROTOCOL          *LoadPath  OPTIONAL
  )
{
  EFI_STATUS                Status;
  OC_BOOTSTRAP_PROTOCOL     *Bootstrap;
  EFI_DEVICE_PATH_PROTOCOL  *AbsPath;

  Bootstrap = NULL;
  Status = gBS->LocateProtocol (
    &gOcBootstrapProtocolGuid,
    NULL,
    (VOID **) &Bootstrap
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "BS: Failed to locate bootstrap protocol - %r\n", Status));
    return;
  }

  if (Bootstrap->Revision != OC_BOOTSTRAP_PROTOCOL_REVISION) {
    DEBUG ((
      DEBUG_WARN,
      "BS: Unsupported bootstrap protocol %u vs %u\n",
      Bootstrap->Revision,
      OC_BOOTSTRAP_PROTOCOL_REVISION
      ));
    return;
  }

  AbsPath = AbsoluteDevicePath (LoadHandle, LoadPath);

  gBS->InstallMultipleProtocolInterfaces (
    &gImageHandle,
    &gOcInterfaceProtocolGuid,
    &mOcInterface,
    NULL
  );

  Bootstrap->ReRun (Bootstrap, FileSystem, AbsPath);

  if (AbsPath != NULL) {
    FreePool (AbsPath);
  }
}

EFI_STATUS
EFIAPI
OcBootstrapMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *FileSystem;
  EFI_HANDLE                        OcImageHandle;


  DEBUG ((DEBUG_INFO, "BS: Starting OpenCore...\n"));

  //
  // We have just started at EFI/BOOT/BOOTx64.efi.
  // We need to run OpenCore on this partition as it failed automatically.
  // The image is optionally located at OPEN_CORE_IMAGE_PATH file.
  //

  LoadedImage = NULL;
  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **) &LoadedImage
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "BS: Failed to locate loaded image - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  DebugPrintDevicePath (DEBUG_INFO, "BS: Booter path", LoadedImage->FilePath);

  //
  // Obtain the file system device path
  //
  FileSystem = LocateFileSystem (
    LoadedImage->DeviceHandle,
    LoadedImage->FilePath
    );

  if (FileSystem == NULL) {
    DEBUG ((DEBUG_ERROR, "BS: Failed to obtain own file system\n"));
    return EFI_NOT_FOUND;
  }

  //
  // Try to start previously loaded OpenCore
  //

  DEBUG ((DEBUG_INFO, "BS: Trying to start loaded OpenCore image...\n"));
  StartOpenCore (FileSystem, LoadedImage->DeviceHandle, LoadedImage->FilePath);

  DEBUG ((DEBUG_INFO, "BS: Trying to load OpenCore image...\n"));
  Status = LoadOpenCore (FileSystem, ImageHandle, &OcImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "BS: Failed to load OpenCore from disk - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  StartOpenCore (FileSystem, LoadedImage->DeviceHandle, LoadedImage->FilePath);
  DEBUG ((DEBUG_WARN, "BS: Failed to start OpenCore image...\n"));

  return EFI_NOT_FOUND;
}
