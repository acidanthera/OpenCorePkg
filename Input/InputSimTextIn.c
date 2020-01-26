#include <Uefi.h>

#include <Protocol/SimpleTextIn.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "GuiIo.h"

struct GUI_KEY_CONTEXT_ {
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL TextIn;
};

GUI_KEY_CONTEXT *
GuiKeyConstruct (
  VOID
  )
{
  ASSERT (gST->ConIn != NULL);
  return (GUI_KEY_CONTEXT *)gST->ConIn;
}

EFI_STATUS
EFIAPI
GuiKeyRead (
  IN OUT GUI_KEY_CONTEXT  *Context,
  OUT    EFI_INPUT_KEY    *Key
  )
{
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *TextIn;

  ASSERT (Context != NULL);

  TextIn = &Context->TextIn;
  return TextIn->ReadKeyStroke (TextIn,  Key);
}

VOID
EFIAPI
GuiKeyReset (
  IN OUT GUI_KEY_CONTEXT  *Context
  )
{
  EFI_STATUS    Status;
  EFI_INPUT_KEY Key;

  ASSERT (Context != NULL);

  do {
    Status = GuiKeyRead (Context, &Key);
  } while (!EFI_ERROR (Status));
}

VOID
GuiKeyDestruct (
  IN GUI_KEY_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
}
