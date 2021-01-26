/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Library/OcMainLib.h>
#include <Uefi.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OcBootstrap.h>
#include <Protocol/OcInterface.h>

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
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
extern CONST GUI_IMAGE         mBackgroundImage;

STATIC
GUI_DRAWING_CONTEXT
mDrawContext;

EFI_STATUS
EFIAPI
OcShowMenuByOc (
  IN     OC_BOOT_CONTEXT          *BootContext,
  IN     OC_BOOT_ENTRY            **BootEntries,
  OUT    OC_BOOT_ENTRY            **ChosenBootEntry
  )
{
  EFI_STATUS    Status;
  UINTN         Index;

  *ChosenBootEntry = NULL;
  mGuiContext.BootEntry = NULL;
  mGuiContext.ReadyToBoot = FALSE;
  mGuiContext.HideAuxiliary = BootContext->PickerContext->HideAuxiliary;
  mGuiContext.Refresh = FALSE;
  mGuiContext.PickerContext = BootContext->PickerContext;
  mGuiContext.AudioPlaybackTimeout = -1;

  Status = GuiLibConstruct (
    BootContext->PickerContext,
    mGuiContext.CursorDefaultX,
    mGuiContext.CursorDefaultY
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Extension for OpenCore builtin renderer to mark that we control text output here.
  //
  gST->ConOut->TestString (gST->ConOut, OC_CONSOLE_MARK_CONTROLLED);

  //
  // Do not play intro animation for blind.
  //
  if (BootContext->PickerContext->PickerAudioAssist) {
    mGuiContext.DoneIntroAnimation = TRUE;
  }

  Status = BootPickerViewInitialize (
    &mDrawContext,
    &mGuiContext,
    InternalGetCursorImage
    );
  if (EFI_ERROR (Status)) {
    GuiLibDestruct ();
    return Status;
  }

  for (Index = 0; Index < BootContext->BootEntryCount; ++Index) {
    Status = BootPickerEntriesAdd (
      BootContext->PickerContext,
      &mGuiContext,
      BootEntries[Index],
      Index == BootContext->DefaultEntry->EntryIndex - 1
      );
    if (EFI_ERROR (Status)) {
      GuiLibDestruct ();
      return Status;
    }
  }

  GuiRedrawAndFlushScreen (&mDrawContext);

  if (BootContext->PickerContext->PickerAudioAssist) {
    BootContext->PickerContext->PlayAudioFile (
      BootContext->PickerContext,
      OcVoiceOverAudioFileChooseOS,
      FALSE
      );
    for (Index = 0; Index < BootContext->BootEntryCount; ++Index) {
      BootContext->PickerContext->PlayAudioEntry (
        BootContext->PickerContext,
        BootEntries[Index]
        );
      if (BootContext->PickerContext->TimeoutSeconds > 0 && BootContext->DefaultEntry->EntryIndex - 1 == Index) {
        BootContext->PickerContext->PlayAudioFile (
          BootContext->PickerContext,
          OcVoiceOverAudioFileDefault,
          FALSE
          );
      }
    }
    BootContext->PickerContext->PlayAudioBeep (
      BootContext->PickerContext,
      OC_VOICE_OVER_SIGNALS_NORMAL,
      OC_VOICE_OVER_SIGNAL_NORMAL_MS,
      OC_VOICE_OVER_SILENCE_NORMAL_MS
      );
  }

  GuiDrawLoop (&mDrawContext, BootContext->PickerContext->TimeoutSeconds);
  ASSERT (mGuiContext.BootEntry != NULL || mGuiContext.Refresh);

  //
  // Note, it is important to destruct GUI here, as we must ensure
  // that keyboard/mouse polling does not conflict with FV2 ui.
  //
  GuiClearScreen (&mDrawContext, mBackgroundImage.Buffer);
  BootPickerViewDeinitialize (&mDrawContext, &mGuiContext);
  GuiLibDestruct ();

  //
  // Extension for OpenCore builtin renderer to mark that we no longer control text output here.
  //
  gST->ConOut->TestString (gST->ConOut, OC_CONSOLE_MARK_UNCONTROLLED);

  *ChosenBootEntry = mGuiContext.BootEntry;
  BootContext->PickerContext->HideAuxiliary = mGuiContext.HideAuxiliary;
  if (mGuiContext.Refresh) {
    return EFI_ABORTED;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
InternalContextConstruct (
  OUT BOOT_PICKER_GUI_CONTEXT  *Context,
  IN  OC_STORAGE_CONTEXT       *Storage,
  IN  OC_PICKER_CONTEXT        *Picker
  );

STATIC
EFI_STATUS
EFIAPI
GuiOcInterfacePopulate (
  IN OC_INTERFACE_PROTOCOL  *This,
  IN OC_STORAGE_CONTEXT     *Storage,
  IN OC_PICKER_CONTEXT      *Context
  )
{
  EFI_STATUS Status;

  Status = InternalContextConstruct (&mGuiContext, Storage, Context);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Context->ShowMenu = OcShowMenuByOc;

  return EFI_SUCCESS;
}

STATIC OC_INTERFACE_PROTOCOL mOcInterface = {
  OC_INTERFACE_REVISION,
  GuiOcInterfacePopulate
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
    DEBUG ((DEBUG_WARN, "OCUI: Another GUI is already present\n"));
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
    DEBUG ((DEBUG_INFO, "OCUI: Registered custom GUI protocol\n"));
  } else {
    DEBUG ((DEBUG_WARN, "OCUI: Failed to install GUI protocol - %r\n", Status));
  }

  return Status;
}
