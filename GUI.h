#ifndef GUI_H
#define GUI_H

#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextIn.h>

typedef struct GUI_OBJ_             GUI_OBJ;
typedef struct GUI_DRAWING_CONTEXT_ GUI_DRAWING_CONTEXT;

enum {
  GuiPointerPrimaryDown,
  GuiPointerPrimaryHold,
  GuiPointerPrimaryUp
};

typedef UINT8 GUI_PTR_EVENT;

typedef
VOID
(*GUI_OBJ_DRAW)(
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height,
  IN     BOOLEAN              RequestDraw
  );

typedef
GUI_OBJ *
(*GUI_OBJ_PTR_EVENT)(
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     GUI_PTR_EVENT        Event,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     INT64                OffsetX,
  IN     INT64                OffsetY
  );

typedef
VOID
(*GUI_OBJ_KEY_EVENT)(
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     CONST EFI_INPUT_KEY  *Key
  );

typedef
BOOLEAN
(*GUI_ANIMATE)(
  IN     VOID                 *Context OPTIONAL,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     UINT64               CurrentTime
  );

typedef struct {
  LIST_ENTRY  Link;
  VOID        *Context;
  GUI_ANIMATE Animate;
} GUI_ANIMATION;

struct GUI_OBJ_ {
  INT64             OffsetX;
  INT64             OffsetY;
  UINT32            Width;
  UINT32            Height;
  GUI_OBJ_DRAW      Draw;
  GUI_OBJ_PTR_EVENT PtrEvent;
  GUI_OBJ_KEY_EVENT KeyEvent;
  LIST_ENTRY        Children;
};

typedef struct {
  LIST_ENTRY Link;
  GUI_OBJ    *Parent;
  GUI_OBJ    Obj;
} GUI_OBJ_CHILD;

typedef struct {
  UINT32                        Width;
  UINT32                        Height;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer;
} GUI_IMAGE;

typedef struct {
  GUI_IMAGE BaseImage;
  GUI_IMAGE HoldImage;
} GUI_CLICK_IMAGE;

typedef struct GUI_SCREEN_CURSOR_ GUI_SCREEN_CURSOR;

typedef
CONST GUI_IMAGE *
(*GUI_CURSOR_GET_IMAGE)(
  IN OUT GUI_SCREEN_CURSOR  *This,
  IN     VOID               *Context
  );

typedef
BOOLEAN
(*GUI_EXIT_LOOP)(
  IN VOID  *Context
  );

struct GUI_SCREEN_CURSOR_ {
  UINT32 X;
  UINT32 Y;
};

struct GUI_DRAWING_CONTEXT_ {
  //
  // Scene objects
  //
  GUI_OBJ              *Screen;
  GUI_CURSOR_GET_IMAGE GetCursorImage;
  GUI_EXIT_LOOP        ExitLoop;
  LIST_ENTRY           Animations;
  VOID                 *GuiContext;
};

RETURN_STATUS
GuiPngToImage (
  IN OUT GUI_IMAGE  *Image,
  IN     VOID       *BmpImage,
  IN     UINTN      BmpImageSize
  );

VOID
GuiObjDrawDelegate (
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height,
  IN     BOOLEAN              ParentRedrawn
  );

GUI_OBJ *
GuiObjDelegatePtrEvent (
  IN OUT GUI_OBJ              *This,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     VOID                 *Context OPTIONAL,
  IN     GUI_PTR_EVENT        Event,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     INT64                OffsetX,
  IN     INT64                OffsetY
  );

BOOLEAN
GuiClipChildBounds (
  IN     INT64   ObjectOffset,
  IN     UINT32  ObjectLength,
  IN OUT UINT32  *ReqOffset,
  IN OUT UINT32  *ReqLength
  );

VOID
GuiDrawToBuffer (
  IN     CONST GUI_IMAGE      *Image,
  IN     UINT8                Opacity,
  IN     BOOLEAN              Fill,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     UINT32               OffsetX,
  IN     UINT32               OffsetY,
  IN     UINT32               Width,
  IN     UINT32               Height,
  IN     BOOLEAN              RequestDraw
  );

VOID
GuiDrawScreen (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                X,
  IN     INT64                Y,
  IN     UINT32               Width,
  IN     UINT32               Height,
  IN     BOOLEAN              RequestDraw
  );

VOID
GuiRedrawObject (
  IN OUT GUI_OBJ              *Obj,
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext,
  IN     INT64                BaseX,
  IN     INT64                BaseY,
  IN     BOOLEAN              RequestDraw
  );

VOID
GuiViewInitialize (
  OUT    GUI_DRAWING_CONTEXT   *DrawContext,
  IN OUT GUI_OBJ               *Screen,
  IN     GUI_CURSOR_GET_IMAGE  CursorDraw,
  IN     GUI_EXIT_LOOP         ExitLoop,
  IN     VOID                  *GuiContext
  );

VOID
GuiDrawLoop (
  IN OUT GUI_DRAWING_CONTEXT  *DrawContext
  );

EFI_STATUS
GuiLibConstruct (
  IN UINT32  CursorDefaultX,
  IN UINT32  CursorDefaultY
  );

VOID
GuiLibDestruct (
  VOID
  );

VOID
GuiBlendPixel (
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BackPixel,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *FrontPixel,
  IN     UINT8                                Opacity
  );

RETURN_STATUS
GuiCreateHighlightedImage (
  OUT GUI_IMAGE                            *SelectedImage,
  IN  CONST GUI_IMAGE                      *SourceImage,
  IN  CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HighlightPixel
  );

typedef enum {
  GuiInterpolTypeLinear,
  GuiInterpolTypeSmooth
} GUI_INTERPOL_TYPE;

typedef struct {
  GUI_INTERPOL_TYPE Type;
  UINT64            StartTime;
  UINT64            Duration;
  UINT32            StartValue;
  UINT32            EndValue;
} GUI_INTERPOLATION;

UINT32
GuiGetInterpolatedValue (
  IN CONST GUI_INTERPOLATION  *Interpol,
  IN       UINT64             CurrentTime
  );

RETURN_STATUS
GuiPngToClickImage (
  IN OUT GUI_CLICK_IMAGE                      *Image,
  IN     VOID                                 *BmpImage,
  IN     UINTN                                BmpImageSize,
  IN     CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HighlightPixel
  );

#endif // GUI_H
