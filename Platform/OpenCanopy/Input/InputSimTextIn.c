/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Protocol/AppleKeyMapAggregator.h>

#include <Library/DebugLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "../GuiIo.h"

struct GUI_KEY_CONTEXT_ {
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap;
  OC_PICKER_CONTEXT                  *Context;
};

GUI_KEY_CONTEXT *
GuiKeyConstruct (
  IN OC_PICKER_CONTEXT  *PickerContext
  )
{
  STATIC GUI_KEY_CONTEXT  mContext;
  mContext.KeyMap  = OcAppleKeyMapInstallProtocols (FALSE);
  mContext.Context = PickerContext;
  if (mContext.KeyMap == NULL) {
    DEBUG ((DEBUG_WARN, "OCUI: Missing AppleKeyMapAggregator\n"));
    return NULL;
  }

  return &mContext;
}

EFI_STATUS
EFIAPI
GuiKeyRead (
  IN OUT GUI_KEY_CONTEXT  *Context,
  OUT    INTN             *KeyIndex,
  OUT    BOOLEAN          *Modifier
  )
{

  ASSERT (Context != NULL);

  *Modifier = FALSE;
  *KeyIndex = Context->Context->GetKeyIndex (
    Context->Context,
    Context->KeyMap,
    Modifier
    );

  //
  // No key was pressed.
  //
  if (*KeyIndex == OC_INPUT_TIMEOUT) {
    return EFI_NOT_FOUND;
  }

  //
  // Internal key was pressed and handled.
  //
  if (*KeyIndex == OC_INPUT_INTERNAL) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
GuiKeyReset (
  IN OUT GUI_KEY_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  //
  // Flush console here?
  //
}

VOID
GuiKeyDestruct (
  IN GUI_KEY_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ZeroMem (Context, sizeof (*Context));
}
