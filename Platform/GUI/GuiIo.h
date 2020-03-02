#ifndef GUI_IO_H
#define GUI_IO_H

#include "GUI.h"

typedef struct GUI_OUTPUT_CONTEXT_  GUI_OUTPUT_CONTEXT;
typedef struct GUI_POINTER_CONTEXT_ GUI_POINTER_CONTEXT;
typedef struct GUI_KEY_CONTEXT_     GUI_KEY_CONTEXT;

typedef struct {
  UINT32  X;
  UINT32  Y;
  BOOLEAN PrimaryDown;
  BOOLEAN SecondaryDown;
} GUI_POINTER_STATE;

GUI_OUTPUT_CONTEXT *
GuiOutputConstruct (
  VOID
  );

EFI_STATUS
EFIAPI
GuiOutputBlt (
  IN GUI_OUTPUT_CONTEXT                 *Context,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer OPTIONAL,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN UINTN                              SourceX,
  IN UINTN                              SourceY,
  IN UINTN                              DestinationX,
  IN UINTN                              DestinationY,
  IN UINTN                              Width,
  IN UINTN                              Height,
  IN UINTN                              Delta OPTIONAL
  );

CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *
GuiOutputGetInfo (
  IN GUI_OUTPUT_CONTEXT  *Context
  );

VOID
GuiOutputDestruct (
  IN GUI_OUTPUT_CONTEXT  *Context
  );

EFI_STATUS
GuiPointerGetState (
  IN OUT GUI_POINTER_CONTEXT  *Context,
  OUT    GUI_POINTER_STATE    *State
  );

VOID
GuiPointerReset (
  IN OUT GUI_POINTER_CONTEXT  *Context
  );

GUI_POINTER_CONTEXT *
GuiPointerConstruct (
  IN UINT32  DefaultX,
  IN UINT32  DefaultY,
  IN UINT32  Width,
  IN UINT32  Height
  );

VOID
GuiPointerDestruct (
  IN GUI_POINTER_CONTEXT  *Context
  );

GUI_KEY_CONTEXT *
GuiKeyConstruct (
  VOID
  );

VOID
EFIAPI
GuiKeyReset (
  IN OUT GUI_KEY_CONTEXT  *Context
  );

EFI_STATUS
EFIAPI
GuiKeyRead (
  IN OUT GUI_KEY_CONTEXT  *Context,
  OUT    EFI_INPUT_KEY    *Key
  );

VOID
GuiKeyDestruct (
  IN GUI_KEY_CONTEXT  *Context
  );

////////////////
typedef struct GUI_OUTPUT_CONTEXT_GOP_ST_ GUI_OUTPUT_CONTEXT_GOP_ST;

GUI_OUTPUT_CONTEXT_GOP_ST *
GuiOutputConstructStGop (
  VOID
  );

EFI_STATUS
EFIAPI
GuiOutputBltStGop (
  IN GUI_OUTPUT_CONTEXT_GOP_ST          *Context,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer OPTIONAL,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN UINTN                              SourceX,
  IN UINTN                              SourceY,
  IN UINTN                              DestinationX,
  IN UINTN                              DestinationY,
  IN UINTN                              Width,
  IN UINTN                              Height,
  IN UINTN                              Delta OPTIONAL
  );

CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *
GuiOutputGetInfoStGop (
  IN GUI_OUTPUT_CONTEXT_GOP_ST  *Context
  );

VOID
GuiOutputDestructStGop (
  IN GUI_OUTPUT_CONTEXT_GOP_ST  *Context
  );

#endif