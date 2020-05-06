/** @file
Private structures definitions in HiiDatabase.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HII_DATABASE_PRIVATE_H__
#define __HII_DATABASE_PRIVATE_H__

#include <Uefi.h>

#include <Protocol/DevicePath.h>
#include <Protocol/HiiFont.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiImageEx.h>
#include <Protocol/HiiImageDecoder.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigKeyword.h>
#include <Protocol/SimpleTextOut.h>

#include <Guid/HiiKeyBoardLayout.h>
#include <Guid/GlobalVariable.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/VariableFormat.h>
#include <Guid/PcdDataBaseSignatureGuid.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>

#define MAX_STRING_LENGTH                  1024
#define MAX_FONT_NAME_LEN                  256
#define NARROW_BASELINE                    15
#define WIDE_BASELINE                      14
#define SYS_FONT_INFO_MASK                 0x37
#define REPLACE_UNKNOWN_GLYPH              0xFFFD
#define PROPORTIONAL_GLYPH                 0x80
#define NARROW_GLYPH                       0x40

#define BITMAP_LEN_1_BIT(Width, Height)  (((UINT32) (Width) + 7) / 8 * (UINT32) (Height))
#define BITMAP_LEN_4_BIT(Width, Height)  (((UINT32) (Width) + 1) / 2 * (UINT32) (Height))
#define BITMAP_LEN_8_BIT(Width, Height)  ((UINT32) (Width) * (UINT32) (Height))
#define BITMAP_LEN_24_BIT(Width, Height) ((UINT32) (Width) * (UINT32) (Height) * 3)

extern EFI_LOCK mHiiDatabaseLock;

//
// IFR data structure
//
// BASE_CR (a, IFR_DEFAULT_VALUE_DATA, Entry) to get the whole structure.

typedef struct {
  LIST_ENTRY            Entry;             // Link to VarStorage Default Data
  UINT16                DefaultId;
  VARIABLE_STORE_HEADER *VariableStorage;
} VARSTORAGE_DEFAULT_DATA;

typedef struct {
  LIST_ENTRY          Entry;             // Link to VarStorage
  EFI_GUID            Guid;
  CHAR16              *Name;
  UINT16              Size;
  UINT8               Type;
  LIST_ENTRY          BlockEntry;        // Link to its Block array
} IFR_VARSTORAGE_DATA;

typedef struct {
  LIST_ENTRY          Entry;             // Link to Block array
  UINT16              Offset;
  UINT16              Width;
  UINT16              BitOffset;
  UINT16              BitWidth;
  EFI_QUESTION_ID     QuestionId;
  UINT8               OpCode;
  UINT8               Scope;
  LIST_ENTRY          DefaultValueEntry; // Link to its default value array
  CHAR16              *Name;
  BOOLEAN             IsBitVar;
} IFR_BLOCK_DATA;

//
// Get default value from IFR data.
//
typedef enum {
  DefaultValueFromDefault = 0,     // Get from the minimum or first one when not set default value.
  DefaultValueFromOtherDefault,    // Get default vale from other default when no default(When other
                                   // defaults are more than one, use the default with smallest default id).
  DefaultValueFromFlag,            // Get default value from the default flag.
  DefaultValueFromOpcode           // Get default value from default opcode, highest priority.
} DEFAULT_VALUE_TYPE;

typedef struct {
  LIST_ENTRY          Entry;
  DEFAULT_VALUE_TYPE  Type;
  BOOLEAN             Cleaned;       // Whether this value is cleaned
                                     // TRUE  Cleaned, the value can't be used
                                     // FALSE Not cleaned, the value can  be used.
  UINT16              DefaultId;
  EFI_IFR_TYPE_VALUE  Value;
} IFR_DEFAULT_DATA;

//
// Storage types
//
#define EFI_HII_VARSTORE_BUFFER              0
#define EFI_HII_VARSTORE_NAME_VALUE          1
#define EFI_HII_VARSTORE_EFI_VARIABLE        2
#define EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER 3

//
// Keyword handler protocol filter type.
//
#define EFI_KEYWORD_FILTER_READONY           0x01
#define EFI_KEYWORD_FILTER_REAWRITE          0x02
#define EFI_KEYWORD_FILTER_BUFFER            0x10
#define EFI_KEYWORD_FILTER_NUMERIC           0x20
#define EFI_KEYWORD_FILTER_NUMERIC_1         0x30
#define EFI_KEYWORD_FILTER_NUMERIC_2         0x40
#define EFI_KEYWORD_FILTER_NUMERIC_4         0x50
#define EFI_KEYWORD_FILTER_NUMERIC_8         0x60


#define HII_FORMSET_STORAGE_SIGNATURE           SIGNATURE_32 ('H', 'S', 'T', 'G')
typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Entry;

  EFI_HII_HANDLE      HiiHandle;
  EFI_HANDLE          DriverHandle;

  UINT8               Type;   // EFI_HII_VARSTORE_BUFFER, EFI_HII_VARSTORE_NAME_VALUE, EFI_HII_VARSTORE_EFI_VARIABLE
  EFI_GUID            Guid;
  CHAR16              *Name;
  UINT16              Size;
} HII_FORMSET_STORAGE;


//
// String Package definitions
//
#define HII_STRING_PACKAGE_SIGNATURE    SIGNATURE_32 ('h','i','s','p')
typedef struct _HII_STRING_PACKAGE_INSTANCE {
  UINTN                                 Signature;
  EFI_HII_STRING_PACKAGE_HDR            *StringPkgHdr;
  UINT8                                 *StringBlock;
  LIST_ENTRY                            StringEntry;
  LIST_ENTRY                            FontInfoList;  // local font info list
  UINT8                                 FontId;
  EFI_STRING_ID                         MaxStringId;   // record StringId
} HII_STRING_PACKAGE_INSTANCE;

//
// Form Package definitions
//
#define HII_IFR_PACKAGE_SIGNATURE       SIGNATURE_32 ('h','f','r','p')
typedef struct _HII_IFR_PACKAGE_INSTANCE {
  UINTN                                 Signature;
  EFI_HII_PACKAGE_HEADER                FormPkgHdr;
  UINT8                                 *IfrData;
  LIST_ENTRY                            IfrEntry;
} HII_IFR_PACKAGE_INSTANCE;

//
// Simple Font Package definitions
//
#define HII_S_FONT_PACKAGE_SIGNATURE    SIGNATURE_32 ('h','s','f','p')
typedef struct _HII_SIMPLE_FONT_PACKAGE_INSTANCE {
  UINTN                                 Signature;
  EFI_HII_SIMPLE_FONT_PACKAGE_HDR       *SimpleFontPkgHdr;
  LIST_ENTRY                            SimpleFontEntry;
} HII_SIMPLE_FONT_PACKAGE_INSTANCE;

//
// Font Package definitions
//
#define HII_FONT_PACKAGE_SIGNATURE      SIGNATURE_32 ('h','i','f','p')
typedef struct _HII_FONT_PACKAGE_INSTANCE {
  UINTN                                 Signature;
  EFI_HII_FONT_PACKAGE_HDR              *FontPkgHdr;
  UINT16                                Height;
  UINT16                                BaseLine;
  UINT8                                 *GlyphBlock;
  LIST_ENTRY                            FontEntry;
  LIST_ENTRY                            GlyphInfoList;
} HII_FONT_PACKAGE_INSTANCE;

#define HII_GLYPH_INFO_SIGNATURE        SIGNATURE_32 ('h','g','i','s')
typedef struct _HII_GLYPH_INFO {
  UINTN                                 Signature;
  LIST_ENTRY                            Entry;
  CHAR16                                CharId;
  EFI_HII_GLYPH_INFO                    Cell;
} HII_GLYPH_INFO;

#define HII_FONT_INFO_SIGNATURE         SIGNATURE_32 ('h','l','f','i')
typedef struct _HII_FONT_INFO {
  UINTN                                 Signature;
  LIST_ENTRY                            Entry;
  LIST_ENTRY                            *GlobalEntry;
  UINT8                                 FontId;
} HII_FONT_INFO;

#define HII_GLOBAL_FONT_INFO_SIGNATURE  SIGNATURE_32 ('h','g','f','i')
typedef struct _HII_GLOBAL_FONT_INFO {
  UINTN                                 Signature;
  LIST_ENTRY                            Entry;
  HII_FONT_PACKAGE_INSTANCE             *FontPackage;
  UINTN                                 FontInfoSize;
  EFI_FONT_INFO                         *FontInfo;
} HII_GLOBAL_FONT_INFO;

//
// Image Package definitions
//

#define HII_PIXEL_MASK                  0x80

typedef struct _HII_IMAGE_PACKAGE_INSTANCE {
  EFI_HII_IMAGE_PACKAGE_HDR             ImagePkgHdr;
  UINT32                                ImageBlockSize;
  UINT32                                PaletteInfoSize;
  EFI_HII_IMAGE_BLOCK                   *ImageBlock;
  UINT8                                 *PaletteBlock;
} HII_IMAGE_PACKAGE_INSTANCE;

//
// Keyboard Layout Package definitions
//
#define HII_KB_LAYOUT_PACKAGE_SIGNATURE SIGNATURE_32 ('h','k','l','p')
typedef struct _HII_KEYBOARD_LAYOUT_PACKAGE_INSTANCE {
  UINTN                                 Signature;
  UINT8                                 *KeyboardPkg;
  LIST_ENTRY                            KeyboardEntry;
} HII_KEYBOARD_LAYOUT_PACKAGE_INSTANCE;

//
// Guid Package definitions
//
#define HII_GUID_PACKAGE_SIGNATURE      SIGNATURE_32 ('h','i','g','p')
typedef struct _HII_GUID_PACKAGE_INSTANCE {
  UINTN                                 Signature;
  UINT8                                 *GuidPkg;
  LIST_ENTRY                            GuidEntry;
} HII_GUID_PACKAGE_INSTANCE;

//
// A package list can contain only one or less than one device path package.
// This rule also applies to image package since ImageId can not be duplicate.
//
typedef struct _HII_DATABASE_PACKAGE_LIST_INSTANCE {
  EFI_HII_PACKAGE_LIST_HEADER           PackageListHdr;
  LIST_ENTRY                            GuidPkgHdr;
  LIST_ENTRY                            FormPkgHdr;
  LIST_ENTRY                            KeyboardLayoutHdr;
  LIST_ENTRY                            StringPkgHdr;
  LIST_ENTRY                            FontPkgHdr;
  HII_IMAGE_PACKAGE_INSTANCE            *ImagePkg;
  LIST_ENTRY                            SimpleFontPkgHdr;
  UINT8                                 *DevicePathPkg;
} HII_DATABASE_PACKAGE_LIST_INSTANCE;

#define HII_HANDLE_SIGNATURE            SIGNATURE_32 ('h','i','h','l')

typedef struct {
  UINTN               Signature;
  LIST_ENTRY          Handle;
  UINTN               Key;
} HII_HANDLE;

#define HII_DATABASE_RECORD_SIGNATURE   SIGNATURE_32 ('h','i','d','r')

typedef struct _HII_DATABASE_RECORD {
  UINTN                                 Signature;
  HII_DATABASE_PACKAGE_LIST_INSTANCE    *PackageList;
  EFI_HANDLE                            DriverHandle;
  EFI_HII_HANDLE                        Handle;
  LIST_ENTRY                            DatabaseEntry;
} HII_DATABASE_RECORD;

#define HII_DATABASE_NOTIFY_SIGNATURE   SIGNATURE_32 ('h','i','d','n')

typedef struct _HII_DATABASE_NOTIFY {
  UINTN                                 Signature;
  EFI_HANDLE                            NotifyHandle;
  UINT8                                 PackageType;
  EFI_GUID                              *PackageGuid;
  EFI_HII_DATABASE_NOTIFY               PackageNotifyFn;
  EFI_HII_DATABASE_NOTIFY_TYPE          NotifyType;
  LIST_ENTRY                            DatabaseNotifyEntry;
} HII_DATABASE_NOTIFY;

#define HII_DATABASE_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('H', 'i', 'D', 'p')

typedef struct _HII_DATABASE_PRIVATE_DATA {
  UINTN                                 Signature;
  LIST_ENTRY                            DatabaseList;
  LIST_ENTRY                            DatabaseNotifyList;
  EFI_HII_FONT_PROTOCOL                 HiiFont;
  EFI_HII_IMAGE_PROTOCOL                HiiImage;
  EFI_HII_IMAGE_EX_PROTOCOL             HiiImageEx;
  EFI_HII_STRING_PROTOCOL               HiiString;
  EFI_HII_DATABASE_PROTOCOL             HiiDatabase;
  EFI_HII_CONFIG_ROUTING_PROTOCOL       ConfigRouting;
  EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL   ConfigKeywordHandler;
  LIST_ENTRY                            HiiHandleList;
  INTN                                  HiiHandleCount;
  LIST_ENTRY                            FontInfoList;  // global font info list
  UINTN                                 Attribute;     // default system color
  EFI_GUID                              CurrentLayoutGuid;
  EFI_HII_KEYBOARD_LAYOUT               *CurrentLayout;
} HII_DATABASE_PRIVATE_DATA;

#define HII_FONT_DATABASE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      HII_DATABASE_PRIVATE_DATA, \
      HiiFont, \
      HII_DATABASE_PRIVATE_DATA_SIGNATURE \
      )

#define HII_IMAGE_DATABASE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      HII_DATABASE_PRIVATE_DATA, \
      HiiImage, \
      HII_DATABASE_PRIVATE_DATA_SIGNATURE \
      )

#define HII_IMAGE_EX_DATABASE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      HII_DATABASE_PRIVATE_DATA, \
      HiiImageEx, \
      HII_DATABASE_PRIVATE_DATA_SIGNATURE \
      )

#define HII_STRING_DATABASE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      HII_DATABASE_PRIVATE_DATA, \
      HiiString, \
      HII_DATABASE_PRIVATE_DATA_SIGNATURE \
      )

#define HII_DATABASE_DATABASE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      HII_DATABASE_PRIVATE_DATA, \
      HiiDatabase, \
      HII_DATABASE_PRIVATE_DATA_SIGNATURE \
      )

#define CONFIG_ROUTING_DATABASE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      HII_DATABASE_PRIVATE_DATA, \
      ConfigRouting, \
      HII_DATABASE_PRIVATE_DATA_SIGNATURE \
      )

#define CONFIG_KEYWORD_HANDLER_DATABASE_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      HII_DATABASE_PRIVATE_DATA, \
      ConfigKeywordHandler, \
      HII_DATABASE_PRIVATE_DATA_SIGNATURE \
      )

//
// Internal function prototypes.
//

/**
  Generate a sub string then output it.

  This is a internal function.

  @param  String                 A constant string which is the prefix of the to be
                                 generated string, e.g. GUID=

  @param  BufferLen              The length of the Buffer in bytes.

  @param  Buffer                 Points to a buffer which will be converted to be the
                                 content of the generated string.

  @param  Flag                   If 1, the buffer contains data for the value of GUID or PATH stored in
                                 UINT8 *; if 2, the buffer contains unicode string for the value of NAME;
                                 if 3, the buffer contains other data.

  @param  SubStr                 Points to the output string. It's caller's
                                 responsibility to free this buffer.


**/
VOID
GenerateSubStr (
  IN CONST EFI_STRING              String,
  IN  UINTN                        BufferLen,
  IN  VOID                         *Buffer,
  IN  UINT8                        Flag,
  OUT EFI_STRING                   *SubStr
  );

/**
  This function checks whether a handle is a valid EFI_HII_HANDLE.

  @param  Handle                  Pointer to a EFI_HII_HANDLE

  @retval TRUE                    Valid
  @retval FALSE                   Invalid

**/
BOOLEAN
IsHiiHandleValid (
  EFI_HII_HANDLE Handle
  );


/**
  This function checks whether EFI_FONT_INFO exists in current database. If
  FontInfoMask is specified, check what options can be used to make a match.
  Note that the masks relate to where the system default should be supplied
  are ignored by this function.

  @param  Private                 Hii database private structure.
  @param  FontInfo                Points to EFI_FONT_INFO structure.
  @param  FontInfoMask            If not NULL, describes what options can be used
                                  to make a match between the font requested and
                                  the font available. The caller must guarantee
                                  this mask is valid.
  @param  FontHandle              On entry, Points to the font handle returned by a
                                  previous  call to GetFontInfo() or NULL to start
                                  with the first font.
  @param  GlobalFontInfo          If not NULL, output the corresponding global font
                                  info.

  @retval TRUE                    Existed
  @retval FALSE                   Not existed

**/
BOOLEAN
IsFontInfoExisted (
  IN  HII_DATABASE_PRIVATE_DATA *Private,
  IN  EFI_FONT_INFO             *FontInfo,
  IN  EFI_FONT_INFO_MASK        *FontInfoMask,   OPTIONAL
  IN  EFI_FONT_HANDLE           FontHandle,      OPTIONAL
  OUT HII_GLOBAL_FONT_INFO      **GlobalFontInfo OPTIONAL
  );

/**

   This function invokes the matching registered function.

   @param  Private           HII Database driver private structure.
   @param  NotifyType        The type of change concerning the database.
   @param  PackageInstance   Points to the package referred to by the notification.
   @param  PackageType       Package type
   @param  Handle            The handle of the package list which contains the specified package.

   @retval EFI_SUCCESS            Already checked all registered function and invoked
                                  if matched.
   @retval EFI_INVALID_PARAMETER  Any input parameter is not valid.

**/
EFI_STATUS
InvokeRegisteredFunction (
  IN HII_DATABASE_PRIVATE_DATA    *Private,
  IN EFI_HII_DATABASE_NOTIFY_TYPE NotifyType,
  IN VOID                         *PackageInstance,
  IN UINT8                        PackageType,
  IN EFI_HII_HANDLE               Handle
  )
;

/**
  Retrieve system default font and color.

  @param  Private                 HII database driver private data.
  @param  FontInfo                Points to system default font output-related
                                  information. It's caller's responsibility to free
                                  this buffer.
  @param  FontInfoSize            If not NULL, output the size of buffer FontInfo.

  @retval EFI_SUCCESS             Cell information is added to the GlyphInfoList.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.
  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.

**/
EFI_STATUS
GetSystemFont (
  IN  HII_DATABASE_PRIVATE_DATA      *Private,
  OUT EFI_FONT_DISPLAY_INFO          **FontInfo,
  OUT UINTN                          *FontInfoSize OPTIONAL
  );


/**
  Parse all string blocks to find a String block specified by StringId.
  If StringId = (EFI_STRING_ID) (-1), find out all EFI_HII_SIBT_FONT blocks
  within this string package and backup its information. If LastStringId is
  specified, the string id of last string block will also be output.
  If StringId = 0, output the string id of last string block (EFI_HII_SIBT_STRING).

  @param  Private                 Hii database private structure.
  @param  StringPackage           Hii string package instance.
  @param  StringId                The string's id, which is unique within
                                  PackageList.
  @param  BlockType               Output the block type of found string block.
  @param  StringBlockAddr         Output the block address of found string block.
  @param  StringTextOffset        Offset, relative to the found block address, of
                                  the  string text information.
  @param  LastStringId            Output the last string id when StringId = 0 or StringId = -1.
  @param  StartStringId           The first id in the skip block which StringId in the block.

  @retval EFI_SUCCESS             The string text and font is retrieved
                                  successfully.
  @retval EFI_NOT_FOUND           The specified text or font info can not be found
                                  out.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
EFI_STATUS
FindStringBlock (
  IN HII_DATABASE_PRIVATE_DATA        *Private,
  IN  HII_STRING_PACKAGE_INSTANCE     *StringPackage,
  IN  EFI_STRING_ID                   StringId,
  OUT UINT8                           *BlockType, OPTIONAL
  OUT UINT8                           **StringBlockAddr, OPTIONAL
  OUT UINTN                           *StringTextOffset, OPTIONAL
  OUT EFI_STRING_ID                   *LastStringId, OPTIONAL
  OUT EFI_STRING_ID                   *StartStringId OPTIONAL
  );


/**
  Parse all glyph blocks to find a glyph block specified by CharValue.
  If CharValue = (CHAR16) (-1), collect all default character cell information
  within this font package and backup its information.

  @param  FontPackage             Hii string package instance.
  @param  CharValue               Unicode character value, which identifies a glyph
                                  block.
  @param  GlyphBuffer             Output the corresponding bitmap data of the found
                                  block. It is the caller's responsibility to free
                                  this buffer.
  @param  Cell                    Output cell information of the encoded bitmap.
  @param  GlyphBufferLen          If not NULL, output the length of GlyphBuffer.

  @retval EFI_SUCCESS             The bitmap data is retrieved successfully.
  @retval EFI_NOT_FOUND           The specified CharValue does not exist in current
                                  database.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
EFI_STATUS
FindGlyphBlock (
  IN  HII_FONT_PACKAGE_INSTANCE      *FontPackage,
  IN  CHAR16                         CharValue,
  OUT UINT8                          **GlyphBuffer, OPTIONAL
  OUT EFI_HII_GLYPH_INFO             *Cell, OPTIONAL
  OUT UINTN                          *GlyphBufferLen OPTIONAL
  );

/**
  This function exports Form packages to a buffer.
  This is a internal function.

  @param  Private                Hii database private structure.
  @param  Handle                 Identification of a package list.
  @param  PackageList            Pointer to a package list which will be exported.
  @param  UsedSize               The length of buffer be used.
  @param  BufferSize             Length of the Buffer.
  @param  Buffer                 Allocated space for storing exported data.
  @param  ResultSize             The size of the already exported content of  this
                                 package list.

  @retval EFI_SUCCESS            Form Packages are exported successfully.
  @retval EFI_INVALID_PARAMETER  Any input parameter is invalid.

**/
EFI_STATUS
ExportFormPackages (
  IN HII_DATABASE_PRIVATE_DATA          *Private,
  IN EFI_HII_HANDLE                     Handle,
  IN HII_DATABASE_PACKAGE_LIST_INSTANCE *PackageList,
  IN UINTN                              UsedSize,
  IN UINTN                              BufferSize,
  IN OUT VOID                           *Buffer,
  IN OUT UINTN                          *ResultSize
  );

//
// EFI_HII_FONT_PROTOCOL protocol interfaces
//


/**
  Renders a string to a bitmap or to the display.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  Flags                   Describes how the string is to be drawn.
  @param  String                  Points to the null-terminated string to be
                                  displayed.
  @param  StringInfo              Points to the string output information,
                                  including the color and font.  If NULL, then the
                                  string will be output in the default system font
                                  and color.
  @param  Blt                     If this points to a non-NULL on entry, this
                                  points to the image, which is Width pixels   wide
                                  and Height pixels high. The string will be drawn
                                  onto this image and
                                  EFI_HII_OUT_FLAG_CLIP is implied. If this points
                                  to a NULL on entry, then a              buffer
                                  will be allocated to hold the generated image and
                                  the pointer updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  BltX                    Together with BltX, Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  BltY                    Together with BltY, Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  RowInfoArray            If this is non-NULL on entry, then on exit, this
                                  will point to an allocated buffer    containing
                                  row information and RowInfoArraySize will be
                                  updated to contain the        number of elements.
                                  This array describes the characters which were at
                                  least partially drawn and the heights of the
                                  rows. It is the caller's responsibility to free
                                  this buffer.
  @param  RowInfoArraySize        If this is non-NULL on entry, then on exit it
                                  contains the number of elements in RowInfoArray.
  @param  ColumnInfoArray         If this is non-NULL, then on return it will be
                                  filled with the horizontal offset for each
                                  character in the string on the row where it is
                                  displayed. Non-printing characters will     have
                                  the offset ~0. The caller is responsible to
                                  allocate a buffer large enough so that    there
                                  is one entry for each character in the string,
                                  not including the null-terminator. It is possible
                                  when character display is normalized that some
                                  character cells overlap.

  @retval EFI_SUCCESS             The string was successfully rendered.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate an output buffer for
                                  RowInfoArray or Blt.
  @retval EFI_INVALID_PARAMETER The String or Blt.
  @retval EFI_INVALID_PARAMETER Flags were invalid combination..

**/
EFI_STATUS
EFIAPI
HiiStringToImage (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  EFI_HII_OUT_FLAGS              Flags,
  IN  CONST EFI_STRING               String,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo       OPTIONAL,
  IN  OUT EFI_IMAGE_OUTPUT           **Blt,
  IN  UINTN                          BltX,
  IN  UINTN                          BltY,
  OUT EFI_HII_ROW_INFO               **RowInfoArray    OPTIONAL,
  OUT UINTN                          *RowInfoArraySize OPTIONAL,
  OUT UINTN                          *ColumnInfoArray  OPTIONAL
  );


/**
  Render a string to a bitmap or the screen containing the contents of the specified string.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  Flags                   Describes how the string is to be drawn.
  @param  PackageList             The package list in the HII database to search
                                  for the specified string.
  @param  StringId                The string's id, which is unique within
                                  PackageList.
  @param  Language                Points to the language for the retrieved string.
                                  If NULL, then the current system language is
                                  used.
  @param  StringInfo              Points to the string output information,
                                  including the color and font.  If NULL, then the
                                  string will be output in the default system font
                                  and color.
  @param  Blt                     If this points to a non-NULL on entry, this
                                  points to the image, which is Width pixels   wide
                                  and Height pixels high. The string will be drawn
                                  onto this image and
                                  EFI_HII_OUT_FLAG_CLIP is implied. If this points
                                  to a NULL on entry, then a              buffer
                                  will be allocated to hold the generated image and
                                  the pointer updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  BltX                    Together with BltX, Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  BltY                    Together with BltY, Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  RowInfoArray            If this is non-NULL on entry, then on exit, this
                                  will point to an allocated buffer    containing
                                  row information and RowInfoArraySize will be
                                  updated to contain the        number of elements.
                                  This array describes the characters which were at
                                  least partially drawn and the heights of the
                                  rows. It is the caller's responsibility to free
                                  this buffer.
  @param  RowInfoArraySize        If this is non-NULL on entry, then on exit it
                                  contains the number of elements in RowInfoArray.
  @param  ColumnInfoArray         If this is non-NULL, then on return it will be
                                  filled with the horizontal offset for each
                                  character in the string on the row where it is
                                  displayed. Non-printing characters will     have
                                  the offset ~0. The caller is responsible to
                                  allocate a buffer large enough so that    there
                                  is one entry for each character in the string,
                                  not including the null-terminator. It is possible
                                  when character display is normalized that some
                                  character cells overlap.

  @retval EFI_SUCCESS             The string was successfully rendered.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate an output buffer for
                                  RowInfoArray or Blt.
  @retval EFI_INVALID_PARAMETER The Blt or PackageList was NULL.
  @retval EFI_INVALID_PARAMETER Flags were invalid combination.
  @retval EFI_NOT_FOUND         The specified PackageList is not in the Database or the stringid is not
                          in the specified PackageList.

**/
EFI_STATUS
EFIAPI
HiiStringIdToImage (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  EFI_HII_OUT_FLAGS              Flags,
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_STRING_ID                  StringId,
  IN  CONST CHAR8*                   Language,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo       OPTIONAL,
  IN  OUT EFI_IMAGE_OUTPUT           **Blt,
  IN  UINTN                          BltX,
  IN  UINTN                          BltY,
  OUT EFI_HII_ROW_INFO               **RowInfoArray    OPTIONAL,
  OUT UINTN                          *RowInfoArraySize OPTIONAL,
  OUT UINTN                          *ColumnInfoArray  OPTIONAL
  );


/**
  Convert the glyph for a single character into a bitmap.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  Char                    Character to retrieve.
  @param  StringInfo              Points to the string font and color information
                                  or NULL if the string should use the default
                                  system font and color.
  @param  Blt                     Thus must point to a NULL on entry. A buffer will
                                  be allocated to hold the output and the pointer
                                  updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  Baseline                Number of pixels from the bottom of the bitmap to
                                  the baseline.

  @retval EFI_SUCCESS             Glyph bitmap created.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate the output buffer Blt.
  @retval EFI_WARN_UNKNOWN_GLYPH  The glyph was unknown and was replaced with the
                                  glyph for Unicode character 0xFFFD.
  @retval EFI_INVALID_PARAMETER   Blt is NULL or *Blt is not NULL.

**/
EFI_STATUS
EFIAPI
HiiGetGlyph (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  CHAR16                         Char,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo,
  OUT EFI_IMAGE_OUTPUT               **Blt,
  OUT UINTN                          *Baseline OPTIONAL
  );


/**
  This function iterates through fonts which match the specified font, using
  the specified criteria. If String is non-NULL, then all of the characters in
  the string must exist in order for a candidate font to be returned.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  FontHandle              On entry, points to the font handle returned by a
                                   previous call to GetFontInfo() or NULL to start
                                  with the  first font. On return, points to the
                                  returned font handle or points to NULL if there
                                  are no more matching fonts.
  @param  StringInfoIn            Upon entry, points to the font to return information
                                  about. If NULL, then the information about the system
                                  default font will be returned.
  @param  StringInfoOut           Upon return, contains the matching font's information.
                                  If NULL, then no information is returned. This buffer
                                  is allocated with a call to the Boot Service AllocatePool().
                                  It is the caller's responsibility to call the Boot
                                  Service FreePool() when the caller no longer requires
                                  the contents of StringInfoOut.
  @param  String                  Points to the string which will be tested to
                                  determine  if all characters are available. If
                                  NULL, then any font  is acceptable.

  @retval EFI_SUCCESS             Matching font returned successfully.
  @retval EFI_NOT_FOUND           No matching font was found.
  @retval EFI_INVALID_PARAMETER   StringInfoIn is NULL.
  @retval EFI_INVALID_PARAMETER   StringInfoIn->FontInfoMask is an invalid combination.
  @retval EFI_OUT_OF_RESOURCES    There were insufficient resources to complete the
                                  request.
**/
EFI_STATUS
EFIAPI
HiiGetFontInfo (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  OUT   EFI_FONT_HANDLE          *FontHandle,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfoIn, OPTIONAL
  OUT       EFI_FONT_DISPLAY_INFO    **StringInfoOut,
  IN  CONST EFI_STRING               String OPTIONAL
  );

//
// EFI_HII_IMAGE_PROTOCOL interfaces
//

/**
  Get the image id of last image block: EFI_HII_IIBT_END_BLOCK when input
  ImageId is zero, otherwise return the address of the
  corresponding image block with identifier specified by ImageId.

  This is a internal function.

  @param ImageBlocks     Points to the beginning of a series of image blocks stored in order.
  @param ImageId         If input ImageId is 0, output the image id of the EFI_HII_IIBT_END_BLOCK;
                         else use this id to find its corresponding image block address.

  @return The image block address when input ImageId is not zero; otherwise return NULL.

**/
EFI_HII_IMAGE_BLOCK *
GetImageIdOrAddress (
  IN EFI_HII_IMAGE_BLOCK *ImageBlocks,
  IN OUT EFI_IMAGE_ID    *ImageId
  );

/**
  Return the HII package list identified by PackageList HII handle.

  @param Database    Pointer to HII database list header.
  @param PackageList HII handle of the package list to locate.

  @retval The HII package list instance.
**/
HII_DATABASE_PACKAGE_LIST_INSTANCE *
LocatePackageList (
  IN  LIST_ENTRY                     *Database,
  IN  EFI_HII_HANDLE                 PackageList
  );

/**
  This function retrieves the image specified by ImageId which is associated with
  the specified PackageList and copies it into the buffer specified by Image.

  @param  Database               A pointer to the database list header.
  @param  PackageList            Handle of the package list where this image will
                                 be searched.
  @param  ImageId                The image's id,, which is unique within
                                 PackageList.
  @param  Image                  Points to the image.
  @param  BitmapOnly             TRUE to only return the bitmap type image.
                                 FALSE to locate image decoder instance to decode image.

  @retval EFI_SUCCESS            The new image was returned successfully.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the
                                 database. The specified PackageList is not in the database.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small to
                                 hold the image.
  @retval EFI_INVALID_PARAMETER  The Image or ImageSize was NULL.
  @retval EFI_OUT_OF_RESOURCES   The bitmap could not be retrieved because there was not
                                 enough memory.
**/
EFI_STATUS
IGetImage (
  IN  LIST_ENTRY                     *Database,
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_IMAGE_ID                   ImageId,
  OUT EFI_IMAGE_INPUT                *Image,
  IN  BOOLEAN                        BitmapOnly
  );

/**
  Return the first HII image decoder instance which supports the DecoderName.

  @param BlockType  The image block type.

  @retval Pointer to the HII image decoder instance.
**/
EFI_HII_IMAGE_DECODER_PROTOCOL *
LocateHiiImageDecoder (
  UINT8                          BlockType
  );

/**
  This function adds the image Image to the group of images owned by PackageList, and returns
  a new image identifier (ImageId).

  @param  This                    A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  PackageList             Handle of the package list where this image will
                                  be added.
  @param  ImageId                 On return, contains the new image id, which is
                                  unique within PackageList.
  @param  Image                   Points to the image.

  @retval EFI_SUCCESS             The new image was added successfully.
  @retval EFI_NOT_FOUND           The specified PackageList could not be found in
                                  database.
  @retval EFI_OUT_OF_RESOURCES    Could not add the image due to lack of resources.
  @retval EFI_INVALID_PARAMETER   Image is NULL or ImageId is NULL.

**/
EFI_STATUS
EFIAPI
HiiNewImage (
  IN  CONST EFI_HII_IMAGE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                 PackageList,
  OUT EFI_IMAGE_ID                   *ImageId,
  IN  CONST EFI_IMAGE_INPUT          *Image
  );


/**
  This function retrieves the image specified by ImageId which is associated with
  the specified PackageList and copies it into the buffer specified by Image.

  @param  This                    A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  PackageList             Handle of the package list where this image will
                                  be searched.
  @param  ImageId                 The image's id,, which is unique within
                                  PackageList.
  @param  Image                   Points to the image.

  @retval EFI_SUCCESS             The new image was returned successfully.
  @retval EFI_NOT_FOUND           The image specified by ImageId is not available.
                                                 The specified PackageList is not in the database.
  @retval EFI_BUFFER_TOO_SMALL    The buffer specified by ImageSize is too small to
                                  hold the image.
  @retval EFI_INVALID_PARAMETER   The Image or ImageSize was NULL.
  @retval EFI_OUT_OF_RESOURCES   The bitmap could not be retrieved because there was not
                                                       enough memory.

**/
EFI_STATUS
EFIAPI
HiiGetImage (
  IN  CONST EFI_HII_IMAGE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_IMAGE_ID                   ImageId,
  OUT EFI_IMAGE_INPUT                *Image
  );


/**
  This function updates the image specified by ImageId in the specified PackageListHandle to
  the image specified by Image.

  @param  This                    A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  PackageList             The package list containing the images.
  @param  ImageId                 The image's id,, which is unique within
                                  PackageList.
  @param  Image                   Points to the image.

  @retval EFI_SUCCESS             The new image was updated successfully.
  @retval EFI_NOT_FOUND           The image specified by ImageId is not in the
                                                database. The specified PackageList is not in the database.
  @retval EFI_INVALID_PARAMETER   The Image was NULL.

**/
EFI_STATUS
EFIAPI
HiiSetImage (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                  PackageList,
  IN EFI_IMAGE_ID                    ImageId,
  IN CONST EFI_IMAGE_INPUT           *Image
  );


/**
  This function renders an image to a bitmap or the screen using the specified
  color and options. It draws the image on an existing bitmap, allocates a new
  bitmap or uses the screen. The images can be clipped.

  @param  This                    A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  Flags                   Describes how the image is to be drawn.
  @param  Image                   Points to the image to be displayed.
  @param  Blt                     If this points to a non-NULL on entry, this
                                  points to the image, which is Width pixels wide
                                  and Height pixels high.  The image will be drawn
                                  onto this image and  EFI_HII_DRAW_FLAG_CLIP is
                                  implied. If this points to a  NULL on entry, then
                                  a buffer will be allocated to hold  the generated
                                  image and the pointer updated on exit. It is the
                                  caller's responsibility to free this buffer.
  @param  BltX                    Specifies the offset from the left and top edge
                                  of the  output image of the first pixel in the
                                  image.
  @param  BltY                    Specifies the offset from the left and top edge
                                  of the  output image of the first pixel in the
                                  image.

  @retval EFI_SUCCESS             The image was successfully drawn.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate an output buffer for Blt.
  @retval EFI_INVALID_PARAMETER   The Image or Blt was NULL.
  @retval EFI_INVALID_PARAMETER   Any combination of Flags is invalid.

**/
EFI_STATUS
EFIAPI
HiiDrawImage (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_DRAW_FLAGS              Flags,
  IN CONST EFI_IMAGE_INPUT           *Image,
  IN OUT EFI_IMAGE_OUTPUT            **Blt,
  IN UINTN                           BltX,
  IN UINTN                           BltY
  );


/**
  This function renders an image to a bitmap or the screen using the specified
  color and options. It draws the image on an existing bitmap, allocates a new
  bitmap or uses the screen. The images can be clipped.

  @param  This                    A pointer to the EFI_HII_IMAGE_PROTOCOL instance.
  @param  Flags                   Describes how the image is to be drawn.
  @param  PackageList             The package list in the HII database to search
                                  for the  specified image.
  @param  ImageId                 The image's id, which is unique within
                                  PackageList.
  @param  Blt                     If this points to a non-NULL on entry, this
                                  points to the image, which is Width pixels wide
                                  and Height pixels high. The image will be drawn
                                  onto this image and
                                  EFI_HII_DRAW_FLAG_CLIP is implied. If this points
                                  to a  NULL on entry, then a buffer will be
                                  allocated to hold  the generated image and the
                                  pointer updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  BltX                    Specifies the offset from the left and top edge
                                  of the  output image of the first pixel in the
                                  image.
  @param  BltY                    Specifies the offset from the left and top edge
                                  of the  output image of the first pixel in the
                                  image.

  @retval EFI_SUCCESS             The image was successfully drawn.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate an output buffer for Blt.
  @retval EFI_INVALID_PARAMETER  The Blt was NULL.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the database.
                           The specified PackageList is not in the database.

**/
EFI_STATUS
EFIAPI
HiiDrawImageId (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_DRAW_FLAGS              Flags,
  IN EFI_HII_HANDLE                  PackageList,
  IN EFI_IMAGE_ID                    ImageId,
  IN OUT EFI_IMAGE_OUTPUT            **Blt,
  IN UINTN                           BltX,
  IN UINTN                           BltY
  );

/**
  The prototype of this extension function is the same with EFI_HII_IMAGE_PROTOCOL.NewImage().
  This protocol invokes EFI_HII_IMAGE_PROTOCOL.NewImage() implicitly.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  PackageList            Handle of the package list where this image will
                                 be added.
  @param  ImageId                On return, contains the new image id, which is
                                 unique within PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was added successfully.
  @retval EFI_NOT_FOUND          The PackageList could not be found.
  @retval EFI_OUT_OF_RESOURCES   Could not add the image due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Image is NULL or ImageId is NULL.
**/
EFI_STATUS
EFIAPI
HiiNewImageEx (
  IN  CONST EFI_HII_IMAGE_EX_PROTOCOL *This,
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_IMAGE_ID                    *ImageId,
  IN  CONST EFI_IMAGE_INPUT           *Image
  );

/**
  Return the information about the image, associated with the package list.
  The prototype of this extension function is the same with EFI_HII_IMAGE_PROTOCOL.GetImage().

  This function is similar to EFI_HII_IMAGE_PROTOCOL.GetImage(). The difference is that
  this function will locate all EFI_HII_IMAGE_DECODER_PROTOCOL instances installed in the
  system if the decoder of the certain image type is not supported by the
  EFI_HII_IMAGE_EX_PROTOCOL. The function will attempt to decode the image to the
  EFI_IMAGE_INPUT using the first EFI_HII_IMAGE_DECODER_PROTOCOL instance that
  supports the requested image type.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  PackageList            The package list in the HII database to search for the
                                 specified image.
  @param  ImageId                The image's id, which is unique within PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was returned successfully.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not available. The specified
                                 PackageList is not in the Database.
  @retval EFI_INVALID_PARAMETER  Image was NULL or ImageId was 0.
  @retval EFI_OUT_OF_RESOURCES   The bitmap could not be retrieved because there
                                 was not enough memory.

**/
EFI_STATUS
EFIAPI
HiiGetImageEx (
  IN  CONST EFI_HII_IMAGE_EX_PROTOCOL *This,
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_IMAGE_ID                    ImageId,
  OUT EFI_IMAGE_INPUT                 *Image
  );

/**
  Change the information about the image.

  Same with EFI_HII_IMAGE_PROTOCOL.SetImage(), this protocol invokes
  EFI_HII_IMAGE_PROTOCOL.SetImage()implicitly.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  PackageList            The package list containing the images.
  @param  ImageId                The image's id, which is unique within PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was successfully updated.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the
                                 database. The specified PackageList is not in
                                 the database.
  @retval EFI_INVALID_PARAMETER  The Image was NULL, the ImageId was 0 or
                                 the Image->Bitmap was NULL.

**/
EFI_STATUS
EFIAPI
HiiSetImageEx (
  IN CONST EFI_HII_IMAGE_EX_PROTOCOL *This,
  IN EFI_HII_HANDLE                  PackageList,
  IN EFI_IMAGE_ID                    ImageId,
  IN CONST EFI_IMAGE_INPUT           *Image
  );

/**
  Renders an image to a bitmap or to the display.

  The prototype of this extension function is the same with
  EFI_HII_IMAGE_PROTOCOL.DrawImage(). This protocol invokes
  EFI_HII_IMAGE_PROTOCOL.DrawImage() implicitly.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  Flags                  Describes how the image is to be drawn.
  @param  Image                  Points to the image to be displayed.
  @param  Blt                    If this points to a non-NULL on entry, this points
                                 to the image, which is Width pixels wide and
                                 Height pixels high.  The image will be drawn onto
                                 this image and  EFI_HII_DRAW_FLAG_CLIP is implied.
                                 If this points to a NULL on entry, then a buffer
                                 will be allocated to hold the generated image and
                                 the pointer updated on exit. It is the caller's
                                 responsibility to free this buffer.
  @param  BltX                   Specifies the offset from the left and top edge of
                                 the output image of the first pixel in the image.
  @param  BltY                   Specifies the offset from the left and top edge of
                                 the output image of the first pixel in the image.

  @retval EFI_SUCCESS            The image was successfully drawn.
  @retval EFI_OUT_OF_RESOURCES   Unable to allocate an output buffer for Blt.
  @retval EFI_INVALID_PARAMETER  The Image or Blt was NULL.

**/
EFI_STATUS
EFIAPI
HiiDrawImageEx (
  IN CONST EFI_HII_IMAGE_EX_PROTOCOL *This,
  IN EFI_HII_DRAW_FLAGS              Flags,
  IN CONST EFI_IMAGE_INPUT           *Image,
  IN OUT EFI_IMAGE_OUTPUT            **Blt,
  IN UINTN                           BltX,
  IN UINTN                           BltY
  );

/**
  Renders an image to a bitmap or the screen containing the contents of the specified
  image.

  This function is similar to EFI_HII_IMAGE_PROTOCOL.DrawImageId(). The difference is that
  this function will locate all EFI_HII_IMAGE_DECODER_PROTOCOL instances installed in the
  system if the decoder of the certain image type is not supported by the
  EFI_HII_IMAGE_EX_PROTOCOL. The function will attempt to decode the image to the
  EFI_IMAGE_INPUT using the first EFI_HII_IMAGE_DECODER_PROTOCOL instance that
  supports the requested image type.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  Flags                  Describes how the image is to be drawn.
  @param  PackageList            The package list in the HII database to search for
                                 the  specified image.
  @param  ImageId                The image's id, which is unique within PackageList.
  @param  Blt                    If this points to a non-NULL on entry, this points
                                 to the image, which is Width pixels wide and
                                 Height pixels high. The image will be drawn onto
                                 this image and EFI_HII_DRAW_FLAG_CLIP is implied.
                                 If this points to a NULL on entry, then a buffer
                                 will be allocated to hold  the generated image
                                 and the pointer updated on exit. It is the caller's
                                 responsibility to free this buffer.
  @param  BltX                   Specifies the offset from the left and top edge of
                                 the output image of the first pixel in the image.
  @param  BltY                   Specifies the offset from the left and top edge of
                                 the output image of the first pixel in the image.

  @retval EFI_SUCCESS            The image was successfully drawn.
  @retval EFI_OUT_OF_RESOURCES   Unable to allocate an output buffer for Blt.
  @retval EFI_INVALID_PARAMETER  The Blt was NULL or ImageId was 0.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the database.
                                 The specified PackageList is not in the database.

**/
EFI_STATUS
EFIAPI
HiiDrawImageIdEx (
  IN CONST EFI_HII_IMAGE_EX_PROTOCOL *This,
  IN EFI_HII_DRAW_FLAGS              Flags,
  IN EFI_HII_HANDLE                  PackageList,
  IN EFI_IMAGE_ID                    ImageId,
  IN OUT EFI_IMAGE_OUTPUT            **Blt,
  IN UINTN                           BltX,
  IN UINTN                           BltY
  );

/**
  This function returns the image information to EFI_IMAGE_OUTPUT. Only the width
  and height are returned to the EFI_IMAGE_OUTPUT instead of decoding the image
  to the buffer. This function is used to get the geometry of the image. This function
  will try to locate all of the EFI_HII_IMAGE_DECODER_PROTOCOL installed on the
  system if the decoder of image type is not supported by the EFI_HII_IMAGE_EX_PROTOCOL.

  @param  This                   A pointer to the EFI_HII_IMAGE_EX_PROTOCOL instance.
  @param  PackageList            Handle of the package list where this image will
                                 be searched.
  @param  ImageId                The image's id, which is unique within PackageList.
  @param  Image                  Points to the image.

  @retval EFI_SUCCESS            The new image was returned successfully.
  @retval EFI_NOT_FOUND          The image specified by ImageId is not in the
                                 database. The specified PackageList is not in the database.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small to
                                 hold the image.
  @retval EFI_INVALID_PARAMETER  The Image was NULL or the ImageId was 0.
  @retval EFI_OUT_OF_RESOURCES   The bitmap could not be retrieved because there
                                 was not enough memory.

**/
EFI_STATUS
EFIAPI
HiiGetImageInfo (
  IN CONST  EFI_HII_IMAGE_EX_PROTOCOL       *This,
  IN        EFI_HII_HANDLE                  PackageList,
  IN        EFI_IMAGE_ID                    ImageId,
  OUT       EFI_IMAGE_OUTPUT                *Image
  );
//
// EFI_HII_STRING_PROTOCOL
//


/**
  This function adds the string String to the group of strings owned by PackageList, with the
  specified font information StringFontInfo and returns a new string id.

  @param  This                    A pointer to the EFI_HII_STRING_PROTOCOL
                                  instance.
  @param  PackageList             Handle of the package list where this string will
                                  be added.
  @param  StringId                On return, contains the new strings id, which is
                                  unique within PackageList.
  @param  Language                Points to the language for the new string.
  @param  LanguageName            Points to the printable language name to
                                  associate with the passed in  Language field.If
                                  LanguageName is not NULL and the string package
                                  header's LanguageName  associated with a given
                                  Language is not zero, the LanguageName being
                                  passed  in will be ignored.
  @param  String                  Points to the new null-terminated string.
  @param  StringFontInfo          Points to the new string's font information or
                                  NULL if the string should have the default system
                                  font, size and style.

  @retval EFI_SUCCESS             The new string was added successfully.
  @retval EFI_NOT_FOUND           The specified PackageList could not be found in
                                  database.
  @retval EFI_OUT_OF_RESOURCES    Could not add the string due to lack of
                                  resources.
  @retval EFI_INVALID_PARAMETER   String is NULL or StringId is NULL or Language is
                                  NULL.

**/
EFI_STATUS
EFIAPI
HiiNewString (
  IN  CONST EFI_HII_STRING_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST CHAR8                     *Language,
  IN  CONST CHAR16                    *LanguageName, OPTIONAL
  IN  CONST EFI_STRING                String,
  IN  CONST EFI_FONT_INFO             *StringFontInfo OPTIONAL
  );


/**
  This function retrieves the string specified by StringId which is associated
  with the specified PackageList in the language Language and copies it into
  the buffer specified by String.

  @param  This                    A pointer to the EFI_HII_STRING_PROTOCOL
                                  instance.
  @param  Language                Points to the language for the retrieved string.
  @param  PackageList             The package list in the HII database to search
                                  for the  specified string.
  @param  StringId                The string's id, which is unique within
                                  PackageList.
  @param  String                  Points to the new null-terminated string.
  @param  StringSize              On entry, points to the size of the buffer
                                  pointed to by  String, in bytes. On return,
                                  points to the length of the string, in bytes.
  @param  StringFontInfo          If not NULL, points to the string's font
                                  information.  It's caller's responsibility to
                                  free this buffer.

  @retval EFI_SUCCESS             The string was returned successfully.
  @retval EFI_NOT_FOUND           The string specified by StringId is not
                                  available.
                                  The specified PackageList is not in the database.
  @retval EFI_INVALID_LANGUAGE    The string specified by StringId is available but
                                  not in the specified language.
  @retval EFI_BUFFER_TOO_SMALL    The buffer specified by StringSize is too small
                                  to  hold the string.
  @retval EFI_INVALID_PARAMETER   The Language or StringSize was NULL.
  @retval EFI_INVALID_PARAMETER   The value referenced by StringSize was not zero
                                  and String was NULL.
  @retval EFI_OUT_OF_RESOURCES    There were insufficient resources to complete the
                                   request.

**/
EFI_STATUS
EFIAPI
InternalHiiGetString (
  IN  CONST EFI_HII_STRING_PROTOCOL   *This,
  IN  CONST CHAR8                     *Language,
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize,
  OUT EFI_FONT_INFO                   **StringFontInfo OPTIONAL
  );


/**
  This function updates the string specified by StringId in the specified PackageList to the text
  specified by String and, optionally, the font information specified by StringFontInfo.

  @param  This                    A pointer to the EFI_HII_STRING_PROTOCOL
                                  instance.
  @param  PackageList             The package list containing the strings.
  @param  StringId                The string's id, which is unique within
                                  PackageList.
  @param  Language                Points to the language for the updated string.
  @param  String                  Points to the new null-terminated string.
  @param  StringFontInfo          Points to the string's font information or NULL
                                  if the string font information is not changed.

  @retval EFI_SUCCESS             The string was updated successfully.
  @retval EFI_NOT_FOUND           The string specified by StringId is not in the
                                  database.
  @retval EFI_INVALID_PARAMETER   The String or Language was NULL.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
EFI_STATUS
EFIAPI
InternalHiiSetString (
  IN CONST EFI_HII_STRING_PROTOCOL    *This,
  IN EFI_HII_HANDLE                   PackageList,
  IN EFI_STRING_ID                    StringId,
  IN CONST CHAR8                      *Language,
  IN CONST EFI_STRING                 String,
  IN CONST EFI_FONT_INFO              *StringFontInfo OPTIONAL
  );


/**
  This function returns the list of supported languages, in the format specified
  in Appendix M of UEFI 2.1 spec.

  @param  This                    A pointer to the EFI_HII_STRING_PROTOCOL
                                  instance.
  @param  PackageList             The package list to examine.
  @param  Languages               Points to the buffer to hold the returned
                                  null-terminated ASCII string.
  @param  LanguagesSize           On entry, points to the size of the buffer
                                  pointed to by  Languages, in bytes. On  return,
                                  points to the length of Languages, in bytes.

  @retval EFI_SUCCESS             The languages were returned successfully.
  @retval EFI_INVALID_PARAMETER   The LanguagesSize was NULL.
  @retval EFI_INVALID_PARAMETER   The value referenced by LanguagesSize is not zero and Languages is NULL.
  @retval EFI_BUFFER_TOO_SMALL    The LanguagesSize is too small to hold the list
                                  of  supported languages. LanguageSize is updated
                                  to contain the required size.
  @retval EFI_NOT_FOUND           Could not find string package in specified
                                  packagelist.

**/
EFI_STATUS
EFIAPI
HiiGetLanguages (
  IN CONST EFI_HII_STRING_PROTOCOL    *This,
  IN EFI_HII_HANDLE                   PackageList,
  IN OUT CHAR8                        *Languages,
  IN OUT UINTN                        *LanguagesSize
  );


/**
  Each string package has associated with it a single primary language and zero
  or more secondary languages. This routine returns the secondary languages
  associated with a package list.

  @param  This                    A pointer to the EFI_HII_STRING_PROTOCOL
                                  instance.
  @param  PackageList             The package list to examine.
  @param  PrimaryLanguage         Points to the null-terminated ASCII string that specifies
                                  the primary language. Languages are specified in the
                                  format specified in Appendix M of the UEFI 2.0 specification.
  @param  SecondaryLanguages      Points to the buffer to hold the returned null-terminated
                                  ASCII string that describes the list of
                                  secondary languages for the specified
                                  PrimaryLanguage. If there are no secondary
                                  languages, the function returns successfully,
                                  but this is set to NULL.
  @param  SecondaryLanguagesSize  On entry, points to the size of the buffer
                                  pointed to by SecondaryLanguages, in bytes. On
                                  return, points to the length of SecondaryLanguages
                                  in bytes.

  @retval EFI_SUCCESS             Secondary languages were correctly returned.
  @retval EFI_INVALID_PARAMETER   PrimaryLanguage or SecondaryLanguagesSize was NULL.
  @retval EFI_INVALID_PARAMETER   The value referenced by SecondaryLanguagesSize is not
                                  zero and SecondaryLanguages is NULL.
  @retval EFI_BUFFER_TOO_SMALL    The buffer specified by SecondaryLanguagesSize is
                                  too small to hold the returned information.
                                  SecondaryLanguageSize is updated to hold the size of
                                  the buffer required.
  @retval EFI_INVALID_LANGUAGE    The language specified by PrimaryLanguage is not
                                  present in the specified package list.
  @retval EFI_NOT_FOUND           The specified PackageList is not in the Database.

**/
EFI_STATUS
EFIAPI
HiiGetSecondaryLanguages (
  IN CONST EFI_HII_STRING_PROTOCOL   *This,
  IN EFI_HII_HANDLE                  PackageList,
  IN CONST CHAR8                     *PrimaryLanguage,
  IN OUT CHAR8                       *SecondaryLanguages,
  IN OUT UINTN                       *SecondaryLanguagesSize
  );

//
// EFI_HII_DATABASE_PROTOCOL protocol interfaces
//


/**
  This function adds the packages in the package list to the database and returns a handle. If there is a
  EFI_DEVICE_PATH_PROTOCOL associated with the DriverHandle, then this function will
  create a package of type EFI_PACKAGE_TYPE_DEVICE_PATH and add it to the package list.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  PackageList             A pointer to an EFI_HII_PACKAGE_LIST_HEADER
                                  structure.
  @param  DriverHandle            Associate the package list with this EFI handle.
                                  If a NULL is specified, this data will not be associate
                                  with any drivers and cannot have a callback induced.
  @param  Handle                  A pointer to the EFI_HII_HANDLE instance.

  @retval EFI_SUCCESS             The package list associated with the Handle was
                                  added to the HII database.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate necessary resources for the
                                  new database contents.
  @retval EFI_INVALID_PARAMETER   PackageList is NULL or Handle is NULL.

**/
EFI_STATUS
EFIAPI
HiiNewPackageList (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN CONST EFI_HII_PACKAGE_LIST_HEADER  *PackageList,
  IN CONST EFI_HANDLE                   DriverHandle, OPTIONAL
  OUT EFI_HII_HANDLE                    *Handle
  );


/**
  This function removes the package list that is associated with a handle Handle
  from the HII database. Before removing the package, any registered functions
  with the notification type REMOVE_PACK and the same package type will be called.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  Handle                  The handle that was registered to the data that
                                  is requested  for removal.

  @retval EFI_SUCCESS             The data associated with the Handle was removed
                                  from  the HII database.
  @retval EFI_NOT_FOUND           The specified Handle is not in database.

**/
EFI_STATUS
EFIAPI
HiiRemovePackageList (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                     Handle
  );


/**
  This function updates the existing package list (which has the specified Handle)
  in the HII databases, using the new package list specified by PackageList.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  Handle                  The handle that was registered to the data that
                                  is  requested to be updated.
  @param  PackageList             A pointer to an EFI_HII_PACKAGE_LIST_HEADER
                                  package.

  @retval EFI_SUCCESS             The HII database was successfully updated.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate enough memory for the updated
                                  database.
  @retval EFI_INVALID_PARAMETER  PackageList was NULL.
  @retval EFI_NOT_FOUND          The specified Handle is not in database.

**/
EFI_STATUS
EFIAPI
HiiUpdatePackageList (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                     Handle,
  IN CONST EFI_HII_PACKAGE_LIST_HEADER  *PackageList
  );


/**
  This function returns a list of the package handles of the specified type
  that are currently active in the database. The pseudo-type
  EFI_HII_PACKAGE_TYPE_ALL will cause all package handles to be listed.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  PackageType             Specifies the package type of the packages to
                                  list or EFI_HII_PACKAGE_TYPE_ALL for all packages
                                  to be listed.
  @param  PackageGuid             If PackageType is EFI_HII_PACKAGE_TYPE_GUID, then
                                  this  is the pointer to the GUID which must match
                                  the Guid field of EFI_HII_GUID_PACKAGE_GUID_HDR.
                                  Otherwise,  it must be NULL.
  @param  HandleBufferLength      On input, a pointer to the length of the handle
                                  buffer.  On output, the length of the handle
                                  buffer that is required for the handles found.
  @param  Handle                  An array of EFI_HII_HANDLE instances returned.

  @retval EFI_SUCCESS             The matching handles are outputted successfully.
                                  HandleBufferLength is updated with the actual length.
  @retval EFI_BUFFER_TO_SMALL     The HandleBufferLength parameter indicates that
                                  Handle is too small to support the number of
                                  handles. HandleBufferLength is updated with a
                                  value that will  enable the data to fit.
  @retval EFI_NOT_FOUND           No matching handle could not be found in
                                  database.
  @retval EFI_INVALID_PARAMETER   HandleBufferLength was NULL.
  @retval EFI_INVALID_PARAMETER   The value referenced by HandleBufferLength was not
                                  zero and Handle was NULL.
  @retval EFI_INVALID_PARAMETER   PackageType is not a EFI_HII_PACKAGE_TYPE_GUID but
                                  PackageGuid is not NULL, PackageType is a EFI_HII_
                                  PACKAGE_TYPE_GUID but PackageGuid is NULL.

**/
EFI_STATUS
EFIAPI
HiiListPackageLists (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  UINT8                             PackageType,
  IN  CONST EFI_GUID                    *PackageGuid,
  IN  OUT UINTN                         *HandleBufferLength,
  OUT EFI_HII_HANDLE                    *Handle
  );


/**
  This function will export one or all package lists in the database to a buffer.
  For each package list exported, this function will call functions registered
  with EXPORT_PACK and then copy the package list to the buffer.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  Handle                  An EFI_HII_HANDLE that corresponds to the desired
                                  package list in the HII database to export or
                                  NULL to indicate  all package lists should be
                                  exported.
  @param  BufferSize              On input, a pointer to the length of the buffer.
                                  On output, the length of the buffer that is
                                  required for the exported data.
  @param  Buffer                  A pointer to a buffer that will contain the
                                  results of  the export function.

  @retval EFI_SUCCESS             Package exported.
  @retval EFI_BUFFER_TO_SMALL     The HandleBufferLength parameter indicates that
                                  Handle is too small to support the number of
                                  handles.      HandleBufferLength is updated with
                                  a value that will enable the data to fit.
  @retval EFI_NOT_FOUND           The specified Handle could not be found in the
                                  current database.
  @retval EFI_INVALID_PARAMETER   BufferSize was NULL.
  @retval EFI_INVALID_PARAMETER   The value referenced by BufferSize was not zero
                                  and Buffer was NULL.

**/
EFI_STATUS
EFIAPI
HiiExportPackageLists (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                    Handle,
  IN  OUT UINTN                         *BufferSize,
  OUT EFI_HII_PACKAGE_LIST_HEADER       *Buffer
  );


/**
  This function registers a function which will be called when specified actions related to packages of
  the specified type occur in the HII database. By registering a function, other HII-related drivers are
  notified when specific package types are added, removed or updated in the HII database.
  Each driver or application which registers a notification should use
  EFI_HII_DATABASE_PROTOCOL.UnregisterPackageNotify() before exiting.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  PackageType             Specifies the package type of the packages to
                                  list or EFI_HII_PACKAGE_TYPE_ALL for all packages
                                  to be listed.
  @param  PackageGuid             If PackageType is EFI_HII_PACKAGE_TYPE_GUID, then
                                  this is the pointer to the GUID which must match
                                  the Guid field of
                                  EFI_HII_GUID_PACKAGE_GUID_HDR. Otherwise, it must
                                  be NULL.
  @param  PackageNotifyFn         Points to the function to be called when the
                                  event specified by
                                  NotificationType occurs.
  @param  NotifyType              Describes the types of notification which this
                                  function will be receiving.
  @param  NotifyHandle            Points to the unique handle assigned to the
                                  registered notification. Can be used in
                                  EFI_HII_DATABASE_PROTOCOL.UnregisterPackageNotify()
                                  to stop notifications.

  @retval EFI_SUCCESS             Notification registered successfully.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate necessary data structures
  @retval EFI_INVALID_PARAMETER   NotifyHandle is NULL.
  @retval EFI_INVALID_PARAMETER   PackageGuid is not NULL when PackageType is not
                                  EFI_HII_PACKAGE_TYPE_GUID.
  @retval EFI_INVALID_PARAMETER   PackageGuid is NULL when PackageType is
                                  EFI_HII_PACKAGE_TYPE_GUID.

**/
EFI_STATUS
EFIAPI
HiiRegisterPackageNotify (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  UINT8                             PackageType,
  IN  CONST EFI_GUID                    *PackageGuid,
  IN  CONST EFI_HII_DATABASE_NOTIFY     PackageNotifyFn,
  IN  EFI_HII_DATABASE_NOTIFY_TYPE      NotifyType,
  OUT EFI_HANDLE                        *NotifyHandle
  );


/**
  Removes the specified HII database package-related notification.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  NotificationHandle      The handle of the notification function being
                                  unregistered.

  @retval EFI_SUCCESS             Notification is unregistered successfully.
  @retval EFI_NOT_FOUND          The incoming notification handle does not exist
                           in current hii database.

**/
EFI_STATUS
EFIAPI
HiiUnregisterPackageNotify (
  IN CONST EFI_HII_DATABASE_PROTOCOL    *This,
  IN EFI_HANDLE                         NotificationHandle
  );


/**
  This routine retrieves an array of GUID values for each keyboard layout that
  was previously registered in the system.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  KeyGuidBufferLength     On input, a pointer to the length of the keyboard
                                  GUID  buffer. On output, the length of the handle
                                  buffer  that is required for the handles found.
  @param  KeyGuidBuffer           An array of keyboard layout GUID instances
                                  returned.

  @retval EFI_SUCCESS             KeyGuidBuffer was updated successfully.
  @retval EFI_BUFFER_TOO_SMALL    The KeyGuidBufferLength parameter indicates
                                  that KeyGuidBuffer is too small to support the
                                  number of GUIDs. KeyGuidBufferLength is
                                  updated with a value that will enable the data to
                                  fit.
  @retval EFI_INVALID_PARAMETER   The KeyGuidBufferLength is NULL.
  @retval EFI_INVALID_PARAMETER   The value referenced by KeyGuidBufferLength is not
                                  zero and KeyGuidBuffer is NULL.
  @retval EFI_NOT_FOUND           There was no keyboard layout.

**/
EFI_STATUS
EFIAPI
HiiFindKeyboardLayouts (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  OUT UINT16                        *KeyGuidBufferLength,
  OUT EFI_GUID                          *KeyGuidBuffer
  );


/**
  This routine retrieves the requested keyboard layout. The layout is a physical description of the keys
  on a keyboard and the character(s) that are associated with a particular set of key strokes.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  KeyGuid                 A pointer to the unique ID associated with a
                                  given keyboard layout. If KeyGuid is NULL then
                                  the current layout will be retrieved.
  @param  KeyboardLayoutLength    On input, a pointer to the length of the
                                  KeyboardLayout buffer.  On output, the length of
                                  the data placed into KeyboardLayout.
  @param  KeyboardLayout          A pointer to a buffer containing the retrieved
                                  keyboard layout.

  @retval EFI_SUCCESS             The keyboard layout was retrieved successfully.
  @retval EFI_NOT_FOUND           The requested keyboard layout was not found.
  @retval EFI_INVALID_PARAMETER   The KeyboardLayout or KeyboardLayoutLength was
                                  NULL.

**/
EFI_STATUS
EFIAPI
HiiGetKeyboardLayout (
  IN  CONST EFI_HII_DATABASE_PROTOCOL   *This,
  IN  CONST EFI_GUID                          *KeyGuid,
  IN OUT UINT16                         *KeyboardLayoutLength,
  OUT EFI_HII_KEYBOARD_LAYOUT           *KeyboardLayout
  );


/**
  This routine sets the default keyboard layout to the one referenced by KeyGuid. When this routine
  is called, an event will be signaled of the EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID
  group type. This is so that agents which are sensitive to the current keyboard layout being changed
  can be notified of this change.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  KeyGuid                 A pointer to the unique ID associated with a
                                  given keyboard layout.

  @retval EFI_SUCCESS             The current keyboard layout was successfully set.
  @retval EFI_NOT_FOUND           The referenced keyboard layout was not found, so
                                  action was taken.
  @retval EFI_INVALID_PARAMETER   The KeyGuid was NULL.

**/
EFI_STATUS
EFIAPI
HiiSetKeyboardLayout (
  IN CONST EFI_HII_DATABASE_PROTOCOL          *This,
  IN CONST EFI_GUID                           *KeyGuid
  );


/**
  Return the EFI handle associated with a package list.

  @param  This                    A pointer to the EFI_HII_DATABASE_PROTOCOL
                                  instance.
  @param  PackageListHandle       An EFI_HII_HANDLE that corresponds to the desired
                                  package list in the HIIdatabase.
  @param  DriverHandle            On return, contains the EFI_HANDLE which was
                                  registered with the package list in
                                  NewPackageList().

  @retval EFI_SUCCESS             The DriverHandle was returned successfully.
  @retval EFI_INVALID_PARAMETER   The PackageListHandle was not valid or
                                  DriverHandle was NULL.

**/
EFI_STATUS
EFIAPI
HiiGetPackageListHandle (
  IN  CONST EFI_HII_DATABASE_PROTOCOL         *This,
  IN  EFI_HII_HANDLE                    PackageListHandle,
  OUT EFI_HANDLE                        *DriverHandle
  );

//
// EFI_HII_CONFIG_ROUTING_PROTOCOL interfaces
//


/**
  This function allows a caller to extract the current configuration
  for one or more named elements from one or more drivers.

  @param  This                    A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                  instance.
  @param  Request                 A null-terminated Unicode string in
                                  <MultiConfigRequest> format.
  @param  Progress                On return, points to a character in the Request
                                  string. Points to the string's null terminator if
                                  request was successful. Points to the most recent
                                  & before the first failing name / value pair (or
                                  the beginning of the string if the failure is in
                                  the first name / value pair) if the request was
                                  not successful.
  @param  Results                 Null-terminated Unicode string in
                                  <MultiConfigAltResp> format which has all values
                                  filled in for the names in the Request string.
                                  String to be allocated by the called function.

  @retval EFI_SUCCESS             The Results string is filled with the values
                                  corresponding to all requested names.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the parts of the
                                  results that must be stored awaiting possible
                                  future        protocols.
  @retval EFI_NOT_FOUND           Routing data doesn't match any known driver.
                                     Progress set to the "G" in "GUID" of the
                                  routing  header that doesn't match. Note: There
                                  is no         requirement that all routing data
                                  be validated before any configuration extraction.
  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL for the Request
                                  parameter would result in this type of error. The
                                  Progress parameter is set to NULL.
  @retval EFI_INVALID_PARAMETER   Illegal syntax. Progress set to most recent &
                                  before the error or the beginning of the string.
  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points to the & before the
                                  name in question.

**/
EFI_STATUS
EFIAPI
HiiConfigRoutingExtractConfig (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );


/**
  This function allows the caller to request the current configuration for the
  entirety of the current HII database and returns the data in a null-terminated Unicode string.

  @param  This                    A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                  instance.
  @param  Results                 Null-terminated Unicode string in
                                  <MultiConfigAltResp> format which has all values
                                  filled in for the entirety of the current HII
                                  database. String to be allocated by the  called
                                  function. De-allocation is up to the caller.

  @retval EFI_SUCCESS             The Results string is filled with the values
                                  corresponding to all requested names.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the parts of the
                                  results that must be stored awaiting possible
                                  future        protocols.
  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL for the Results
                                  parameter would result in this type of error.

**/
EFI_STATUS
EFIAPI
HiiConfigRoutingExportConfig (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  OUT EFI_STRING                             *Results
  );


/**
  This function processes the results of processing forms and routes it to the
  appropriate handlers or storage.

  @param  This                    A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                  instance.
  @param  Configuration           A null-terminated Unicode string in
                                  <MulltiConfigResp> format.
  @param  Progress                A pointer to a string filled in with the offset
                                  of the most recent & before the first failing
                                  name / value pair (or the beginning of the string
                                  if the failure is in the first name / value pair)
                                  or the terminating NULL if all was successful.

  @retval EFI_SUCCESS             The results have been distributed or are awaiting
                                  distribution.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the parts of the
                                  results that must be stored awaiting possible
                                  future        protocols.
  @retval EFI_INVALID_PARAMETER   Passing in a NULL for the Configuration parameter
                                  would result in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data was not
                                  found.

**/
EFI_STATUS
EFIAPI
HiiConfigRoutingRouteConfig (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  );



/**
  This helper function is to be called by drivers to map configuration data stored
  in byte array ("block") formats such as UEFI Variables into current configuration strings.

  @param  This                    A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                  instance.
  @param  ConfigRequest           A null-terminated Unicode string in
                                  <ConfigRequest> format.
  @param  Block                   Array of bytes defining the block's
                                  configuration.
  @param  BlockSize               Length in bytes of Block.
  @param  Config                  Filled-in configuration string. String allocated
                                  by  the function. Returned only if call is
                                  successful.
  @param  Progress                A pointer to a string filled in with the offset
                                  of  the most recent & before the first failing
                                  name/value pair (or the beginning of the string
                                  if the failure is in the first name / value pair)
                                  or the terminating NULL if all was successful.

  @retval EFI_SUCCESS             The request succeeded. Progress points to the
                                  null terminator at the end of the ConfigRequest
                                        string.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to allocate Config.
                                  Progress points to the first character of
                                  ConfigRequest.
  @retval EFI_INVALID_PARAMETER   Passing in a NULL for the ConfigRequest or
                                  Block parameter would result in this type of
                                  error. Progress points to the first character of
                                  ConfigRequest.
  @retval EFI_NOT_FOUND           Target for the specified routing data was not
                                  found. Progress points to the "G" in "GUID" of
                                  the      errant routing data.
  @retval EFI_DEVICE_ERROR        Block not large enough. Progress undefined.
  @retval EFI_INVALID_PARAMETER   Encountered non <BlockName> formatted string.
                                       Block is left updated and Progress points at
                                  the '&' preceding the first non-<BlockName>.

**/
EFI_STATUS
EFIAPI
HiiBlockToConfig (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN  CONST EFI_STRING                       ConfigRequest,
  IN  CONST UINT8                            *Block,
  IN  CONST UINTN                            BlockSize,
  OUT EFI_STRING                             *Config,
  OUT EFI_STRING                             *Progress
  );


/**
  This helper function is to be called by drivers to map configuration strings
  to configurations stored in byte array ("block") formats such as UEFI Variables.

  @param  This                    A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                  instance.
  @param  ConfigResp              A null-terminated Unicode string in <ConfigResp>
                                  format.
  @param  Block                   A possibly null array of bytes representing the
                                  current  block. Only bytes referenced in the
                                  ConfigResp string  in the block are modified. If
                                  this parameter is null or if the *BlockSize
                                  parameter is (on input) shorter than required by
                                  the Configuration string, only the BlockSize
                                  parameter is updated and an appropriate status
                                  (see below)  is returned.
  @param  BlockSize               The length of the Block in units of UINT8.  On
                                  input, this is the size of the Block. On output,
                                  if successful, contains the largest index of the
                                  modified byte in the Block, or the required buffer
                                  size if the Block is not large enough.
  @param  Progress                On return, points to an element of the ConfigResp
                                   string filled in with the offset of the most
                                  recent '&' before the first failing name / value
                                  pair (or  the beginning of the string if the
                                  failure is in the  first name / value pair) or
                                  the terminating NULL if all was successful.

  @retval EFI_SUCCESS             The request succeeded. Progress points to the
                                  null terminator at the end of the ConfigResp
                                  string.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to allocate Config.
                                  Progress points to the first character of
                                  ConfigResp.
  @retval EFI_INVALID_PARAMETER   Passing in a NULL for the ConfigResp or
                                  Block parameter would result in this type of
                                  error. Progress points to the first character of
                                           ConfigResp.
  @retval EFI_NOT_FOUND           Target for the specified routing data was not
                                  found. Progress points to the "G" in "GUID" of
                                  the      errant routing data.
  @retval EFI_INVALID_PARAMETER   Encountered non <BlockName> formatted name /
                                  value pair. Block is left updated and
                                  Progress points at the '&' preceding the first
                                  non-<BlockName>.
  @retval EFI_BUFFER_TOO_SMALL    Block not large enough. Progress undefined.
                                  BlockSize is updated with the required buffer size.

**/
EFI_STATUS
EFIAPI
HiiConfigToBlock (
  IN     CONST EFI_HII_CONFIG_ROUTING_PROTOCOL *This,
  IN     CONST EFI_STRING                      ConfigResp,
  IN OUT UINT8                                 *Block,
  IN OUT UINTN                                 *BlockSize,
  OUT    EFI_STRING                            *Progress
  );


/**
  This helper function is to be called by drivers to extract portions of
  a larger configuration string.

  @param  This                    A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                  instance.
  @param  Configuration           A null-terminated Unicode string in
                                  <MultiConfigAltResp> format.
  @param  Guid                    A pointer to the GUID value to search for in the
                                  routing portion of the ConfigResp string when
                                  retrieving  the requested data. If Guid is NULL,
                                  then all GUID  values will be searched for.
  @param  Name                    A pointer to the NAME value to search for in the
                                  routing portion of the ConfigResp string when
                                  retrieving  the requested data. If Name is NULL,
                                  then all Name  values will be searched for.
  @param  DevicePath              A pointer to the PATH value to search for in the
                                  routing portion of the ConfigResp string when
                                  retrieving  the requested data. If DevicePath is
                                  NULL, then all  DevicePath values will be
                                  searched for.
  @param  AltCfgId                A pointer to the ALTCFG value to search for in
                                  the  routing portion of the ConfigResp string
                                  when retrieving  the requested data.  If this
                                  parameter is NULL,  then the current setting will
                                  be retrieved.
  @param  AltCfgResp              A pointer to a buffer which will be allocated by
                                  the  function which contains the retrieved string
                                  as requested.   This buffer is only allocated if
                                  the call was successful.

  @retval EFI_SUCCESS             The request succeeded. The requested data was
                                  extracted  and placed in the newly allocated
                                  AltCfgResp buffer.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to allocate AltCfgResp.
  @retval EFI_INVALID_PARAMETER   Any parameter is invalid.
  @retval EFI_NOT_FOUND           Target for the specified routing data was not
                                  found.

**/
EFI_STATUS
EFIAPI
HiiGetAltCfg (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL    *This,
  IN  CONST EFI_STRING                         Configuration,
  IN  CONST EFI_GUID                           *Guid,
  IN  CONST EFI_STRING                         Name,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL           *DevicePath,
  IN  CONST UINT16                             *AltCfgId,
  OUT EFI_STRING                               *AltCfgResp
  );

/**

  This function accepts a <MultiKeywordResp> formatted string, finds the associated
  keyword owners, creates a <MultiConfigResp> string from it and forwards it to the
  EFI_HII_ROUTING_PROTOCOL.RouteConfig function.

  If there is an issue in resolving the contents of the KeywordString, then the
  function returns an error and also sets the Progress and ProgressErr with the
  appropriate information about where the issue occurred and additional data about
  the nature of the issue.

  In the case when KeywordString containing multiple keywords, when an EFI_NOT_FOUND
  error is generated during processing the second or later keyword element, the system
  storage associated with earlier keywords is not modified. All elements of the
  KeywordString must successfully pass all tests for format and access prior to making
  any modifications to storage.

  In the case when EFI_DEVICE_ERROR is returned from the processing of a KeywordString
  containing multiple keywords, the state of storage associated with earlier keywords
  is undefined.


  @param This             Pointer to the EFI_KEYWORD_HANDLER _PROTOCOL instance.

  @param KeywordString    A null-terminated string in <MultiKeywordResp> format.

  @param Progress         On return, points to a character in the KeywordString.
                          Points to the string's NULL terminator if the request
                          was successful. Points to the most recent '&' before
                          the first failing name / value pair (or the beginning
                          of the string if the failure is in the first name / value
                          pair) if the request was not successful.

  @param ProgressErr      If during the processing of the KeywordString there was
                          a failure, this parameter gives additional information
                          about the possible source of the problem. The various
                          errors are defined in "Related Definitions" below.


  @retval EFI_SUCCESS             The specified action was completed successfully.

  @retval EFI_INVALID_PARAMETER   One or more of the following are TRUE:
                                  \c 1. KeywordString is NULL.
                                  \c 2. Parsing of the KeywordString resulted in an
                                     error. See Progress and ProgressErr for more data.

  @retval EFI_NOT_FOUND           An element of the KeywordString was not found.
                                  See ProgressErr for more data.

  @retval EFI_OUT_OF_RESOURCES    Required system resources could not be allocated.
                                  See ProgressErr for more data.

  @retval EFI_ACCESS_DENIED       The action violated system policy. See ProgressErr
                                  for more data.

  @retval EFI_DEVICE_ERROR        An unexpected system error occurred. See ProgressErr
                                  for more data.

**/
EFI_STATUS
EFIAPI
EfiConfigKeywordHandlerSetData (
  IN EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL *This,
  IN CONST EFI_STRING                    KeywordString,
  OUT EFI_STRING                         *Progress,
  OUT UINT32                             *ProgressErr
  );

/**

  This function accepts a <MultiKeywordRequest> formatted string, finds the underlying
  keyword owners, creates a <MultiConfigRequest> string from it and forwards it to the
  EFI_HII_ROUTING_PROTOCOL.ExtractConfig function.

  If there is an issue in resolving the contents of the KeywordString, then the function
  returns an EFI_INVALID_PARAMETER and also set the Progress and ProgressErr with the
  appropriate information about where the issue occurred and additional data about the
  nature of the issue.

  In the case when KeywordString is NULL, or contains multiple keywords, or when
  EFI_NOT_FOUND is generated while processing the keyword elements, the Results string
  contains values returned for all keywords processed prior to the keyword generating the
  error but no values for the keyword with error or any following keywords.


  @param This           Pointer to the EFI_KEYWORD_HANDLER _PROTOCOL instance.

  @param NameSpaceId    A null-terminated string containing the platform configuration
                        language to search through in the system. If a NULL is passed
                        in, then it is assumed that any platform configuration language
                        with the prefix of "x-UEFI-" are searched.

  @param KeywordString  A null-terminated string in <MultiKeywordRequest> format. If a
                        NULL is passed in the KeywordString field, all of the known
                        keywords in the system for the NameSpaceId specified are
                        returned in the Results field.

  @param Progress       On return, points to a character in the KeywordString. Points
                        to the string's NULL terminator if the request was successful.
                        Points to the most recent '&' before the first failing name / value
                        pair (or the beginning of the string if the failure is in the first
                        name / value pair) if the request was not successful.

  @param ProgressErr    If during the processing of the KeywordString there was a
                        failure, this parameter gives additional information about the
                        possible source of the problem. See the definitions in SetData()
                        for valid value definitions.

  @param Results        A null-terminated string in <MultiKeywordResp> format is returned
                        which has all the values filled in for the keywords in the
                        KeywordString. This is a callee-allocated field, and must be freed
                        by the caller after being used.

  @retval EFI_SUCCESS             The specified action was completed successfully.

  @retval EFI_INVALID_PARAMETER   One or more of the following are TRUE:
                                  1.Progress, ProgressErr, or Results is NULL.
                                  2.Parsing of the KeywordString resulted in an error. See
                                    Progress and ProgressErr for more data.


  @retval EFI_NOT_FOUND           An element of the KeywordString was not found. See
                                  ProgressErr for more data.

  @retval EFI_NOT_FOUND           The NamespaceId specified was not found.  See ProgressErr
                                  for more data.

  @retval EFI_OUT_OF_RESOURCES    Required system resources could not be allocated.  See
                                  ProgressErr for more data.

  @retval EFI_ACCESS_DENIED       The action violated system policy.  See ProgressErr for
                                  more data.

  @retval EFI_DEVICE_ERROR        An unexpected system error occurred.  See ProgressErr
                                  for more data.

**/
EFI_STATUS
EFIAPI
EfiConfigKeywordHandlerGetData (
  IN EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL  *This,
  IN CONST EFI_STRING                     NameSpaceId, OPTIONAL
  IN CONST EFI_STRING                     KeywordString, OPTIONAL
  OUT EFI_STRING                          *Progress,
  OUT UINT32                              *ProgressErr,
  OUT EFI_STRING                          *Results
  );

/**
  Compare whether two names of languages are identical.

  @param  Language1              Name of language 1 from StringPackage
  @param  Language2              Name of language 2 to be compared with language 1.

  @retval TRUE                   same
  @retval FALSE                  not same

**/
BOOLEAN
HiiCompareLanguage (
  IN  CHAR8  *Language1,
  IN  CHAR8  *Language2
  )
;

/**
  Retrieves a pointer to a Null-terminated ASCII string containing the list
  of languages that an HII handle in the HII Database supports.  The returned
  string is allocated using AllocatePool().  The caller is responsible for freeing
  the returned string using FreePool().  The format of the returned string follows
  the language format assumed the HII Database.

  If HiiHandle is NULL, then ASSERT().

  @param[in]  HiiHandle  A handle that was previously registered in the HII Database.

  @retval NULL   HiiHandle is not registered in the HII database
  @retval NULL   There are not enough resources available to retrieve the supported
                 languages.
  @retval NULL   The list of supported languages could not be retrieved.
  @retval Other  A pointer to the Null-terminated ASCII string of supported languages.

**/
CHAR8 *
GetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  );

/**
This function mainly use to get HiiDatabase information.

@param  This                   A pointer to the EFI_HII_DATABASE_PROTOCOL instance.

@retval EFI_SUCCESS            Get the information successfully.
@retval EFI_OUT_OF_RESOURCES   Not enough memory to store the Hiidatabase data.

**/
EFI_STATUS
HiiGetDatabaseInfo (
  IN CONST EFI_HII_DATABASE_PROTOCOL        *This
  );

/**
This function mainly use to get and update ConfigResp string.

@param  This                   A pointer to the EFI_HII_DATABASE_PROTOCOL instance.

@retval EFI_SUCCESS            Get the information successfully.
@retval EFI_OUT_OF_RESOURCES   Not enough memory to store the Configuration Setting data.

**/
EFI_STATUS
HiiGetConfigRespInfo (
  IN CONST EFI_HII_DATABASE_PROTOCOL        *This
  );

//
// Global variables
//
extern EFI_EVENT gHiiKeyboardLayoutChanged;
extern BOOLEAN   gExportAfterReadyToBoot;

#endif
