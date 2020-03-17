/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
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

#include "OpenCanopy.h"
#include "BmfLib.h"
#include "GuiApp.h"

extern BOOT_PICKER_GUI_CONTEXT mGuiContext;

STATIC
GUI_DRAWING_CONTEXT
mDrawContext;

EFI_STATUS
OcShowMenuByOc (
  IN     OC_PICKER_CONTEXT        *Context,
  IN     OC_BOOT_ENTRY            *BootEntries,
  IN     UINTN                    Count,
  IN     UINTN                    DefaultEntry,
  OUT    OC_BOOT_ENTRY            **ChosenBootEntry
  )
{
  RETURN_STATUS Status;
  UINTN         Index;

  *ChosenBootEntry = NULL;
  mGuiContext.BootEntry = NULL;
  mGuiContext.HideAuxiliary = Context->HideAuxiliary;
  mGuiContext.Refresh = FALSE;

  BootPickerEntriesEmpty ();

  for (Index = 0; Index < Count; ++Index) {
    Status = BootPickerEntriesAdd (
               Context,
               &mGuiContext,
               &BootEntries[Index],
               Index == DefaultEntry
               );
    if (RETURN_ERROR (Status)) {
      return Status;
    }
  }

  GuiDrawLoop (&mDrawContext);
  ASSERT (mGuiContext.BootEntry != NULL || mGuiContext.Refresh);

  *ChosenBootEntry = mGuiContext.BootEntry;
  Context->HideAuxiliary = mGuiContext.HideAuxiliary;
  if (mGuiContext.Refresh) {
    return RETURN_ABORTED;
  }
  return RETURN_SUCCESS;
}

RETURN_STATUS
InternalContextConstruct (
  OUT BOOT_PICKER_GUI_CONTEXT  *Context,
  IN  OC_STORAGE_CONTEXT       *Storage
  );

/**
  Add an entry to the log buffer

  @param[in] This          This protocol.
  @param[in] Storage       File system access storage.
  @param[in] Context        User interface configuration.

  @retval does not return unless a fatal error happened.
**/
EFI_STATUS
EFIAPI
GuiOcInterfaceRun (
  IN OC_INTERFACE_PROTOCOL  *This,
  IN OC_STORAGE_CONTEXT     *Storage,
  IN OC_PICKER_CONTEXT      *Context
  )
{
  EFI_STATUS Status;

  Status = GuiLibConstruct (0, 0);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = InternalContextConstruct (&mGuiContext, Storage);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = BootPickerViewInitialize (
    &mDrawContext,
    &mGuiContext,
    InternalGetCursorImage
    );

  Context->ShowMenu = OcShowMenuByOc;

  return OcRunBootPicker (Context);
}

STATIC OC_INTERFACE_PROTOCOL mOcInterface = {
  OC_INTERFACE_REVISION,
  GuiOcInterfaceRun
};

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *PrevInterface;
  EFI_HANDLE  NewHandle;

  //
  // Check for previous GUI protocols.
  //
  Status = gBS->LocateProtocol (
    &gOcInterfaceProtocolGuid,
    NULL,
    &PrevInterface
    );

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCE: Another GUI is already present\n"));
    return EFI_ALREADY_STARTED;
  }

  //
  // Install new GUI protocol
  //
  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &NewHandle,
    &gOcInterfaceProtocolGuid,
    &mOcInterface,
    NULL
    );

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCE: Registered custom GUI protocol\n"));
  } else {
    DEBUG ((DEBUG_ERROR, "OCE: Failed to install GUI protocol - %r\n", Status));
  }

  return Status;
}
