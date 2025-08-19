/** @file
  OcTextInputCommon - Common definitions for SimpleTextInputEx compatibility

  This header contains shared structures, constants, and macros used by both
  the OcTextInputLib library and the standalone OcTextInputDxe driver.

  Copyright (c) 2025, OpenCore Team. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OC_TEXT_INPUT_COMMON_H
#define OC_TEXT_INPUT_COMMON_H

#include <Uefi.h>
#include <Library/DebugLib.h>

//
// Debug macros for conditional compilation
//
#define OCTI_DEBUG_INFO(Format, ...)     DEBUG ((DEBUG_INFO, Format, ##__VA_ARGS__))
#define OCTI_DEBUG_VERBOSE(Format, ...)  DEBUG ((DEBUG_VERBOSE, Format, ##__VA_ARGS__))
#define OCTI_DEBUG_ERROR(Format, ...)    DEBUG ((DEBUG_ERROR, Format, ##__VA_ARGS__))
#define OCTI_DEBUG_WARN(Format, ...)     DEBUG ((DEBUG_WARN, Format, ##__VA_ARGS__))

//
// Memory management macros
//
#define OCTI_ALLOC_POOL(Size)       AllocatePool (Size)
#define OCTI_FREE_POOL(Buffer)      FreePool (Buffer)
#define OCTI_ALLOC_ZERO_POOL(Size)  AllocateZeroPool (Size)

//
// Conditional string inclusion for memory optimization
//
#if defined (DEBUG_POINTER)
#define OCTI_DEBUG_STRING(String)  DEBUG_POINTER (String)
#else
#define OCTI_DEBUG_STRING(String)  (String)
#endif

/**
  Control character mapping structure for hotkey descriptions.
**/
typedef struct {
  UINT8    ControlChar;    ///< Control character code (0x01-0x1F)
  CHAR8    *Description;   ///< Human-readable description (e.g., "CTRL+A")
  CHAR8    *ShellFunction; ///< Shell function name (e.g., "StartOfLine")
} OCTI_CONTROL_CHAR_MAPPING;

//
// Control character lookup table - shared between library and driver
//
#define OCTI_CONTROL_CHAR_TABLE_INIT  { \
  { 0x01, OCTI_DEBUG_STRING ("CTRL+A"),  OCTI_DEBUG_STRING ("StartOfLine")              }, \
  { 0x02, OCTI_DEBUG_STRING ("CTRL+B"),  OCTI_DEBUG_STRING ("CursorLeft")               }, \
  { 0x03, OCTI_DEBUG_STRING ("CTRL+C"),  OCTI_DEBUG_STRING ("Break")                    }, \
  { 0x04, OCTI_DEBUG_STRING ("CTRL+D"),  OCTI_DEBUG_STRING ("Delete")                   }, \
  { 0x05, OCTI_DEBUG_STRING ("CTRL+E"),  OCTI_DEBUG_STRING ("EndOfLine")                }, \
  { 0x06, OCTI_DEBUG_STRING ("CTRL+F"),  OCTI_DEBUG_STRING ("CursorRight")              }, \
  { 0x07, OCTI_DEBUG_STRING ("CTRL+G"),  OCTI_DEBUG_STRING ("Bell")                     }, \
  { 0x08, OCTI_DEBUG_STRING ("CTRL+H"),  OCTI_DEBUG_STRING ("Backspace")                }, \
  { 0x09, OCTI_DEBUG_STRING ("CTRL+I"),  OCTI_DEBUG_STRING ("Tab")                      }, \
  { 0x0A, OCTI_DEBUG_STRING ("CTRL+J"),  OCTI_DEBUG_STRING ("LineFeed")                 }, \
  { 0x0B, OCTI_DEBUG_STRING ("CTRL+K"),  OCTI_DEBUG_STRING ("KillLine")                 }, \
  { 0x0C, OCTI_DEBUG_STRING ("CTRL+L"),  OCTI_DEBUG_STRING ("ClearScreen")              }, \
  { 0x0D, OCTI_DEBUG_STRING ("CTRL+M"),  OCTI_DEBUG_STRING ("CarriageReturn")           }, \
  { 0x0E, OCTI_DEBUG_STRING ("CTRL+N"),  OCTI_DEBUG_STRING ("NextLine")                 }, \
  { 0x0F, OCTI_DEBUG_STRING ("CTRL+O"),  OCTI_DEBUG_STRING ("OpenFile")                 }, \
  { 0x10, OCTI_DEBUG_STRING ("CTRL+P"),  OCTI_DEBUG_STRING ("PrevLine")                 }, \
  { 0x11, OCTI_DEBUG_STRING ("CTRL+Q"),  OCTI_DEBUG_STRING ("XON")                      }, \
  { 0x12, OCTI_DEBUG_STRING ("CTRL+R"),  OCTI_DEBUG_STRING ("Refresh")                  }, \
  { 0x13, OCTI_DEBUG_STRING ("CTRL+S"),  OCTI_DEBUG_STRING ("SaveFile")                 }, \
  { 0x14, OCTI_DEBUG_STRING ("CTRL+T"),  OCTI_DEBUG_STRING ("Transpose")                }, \
  { 0x15, OCTI_DEBUG_STRING ("CTRL+U"),  OCTI_DEBUG_STRING ("KillToStart")              }, \
  { 0x16, OCTI_DEBUG_STRING ("CTRL+V"),  OCTI_DEBUG_STRING ("Paste")                    }, \
  { 0x17, OCTI_DEBUG_STRING ("CTRL+W"),  OCTI_DEBUG_STRING ("KillWord")                 }, \
  { 0x18, OCTI_DEBUG_STRING ("CTRL+X"),  OCTI_DEBUG_STRING ("Cut")                      }, \
  { 0x19, OCTI_DEBUG_STRING ("CTRL+Y"),  OCTI_DEBUG_STRING ("Yank")                     }, \
  { 0x1A, OCTI_DEBUG_STRING ("CTRL+Z"),  OCTI_DEBUG_STRING ("Suspend")                  }, \
  { 0x1B, OCTI_DEBUG_STRING ("CTRL+["),  OCTI_DEBUG_STRING ("Escape")                   }, \
  { 0x1C, OCTI_DEBUG_STRING ("CTRL+\\"), OCTI_DEBUG_STRING ("FileSeparator")            }, \
  { 0x1D, OCTI_DEBUG_STRING ("CTRL+]"),  OCTI_DEBUG_STRING ("GroupSeparator")           }, \
  { 0x1E, OCTI_DEBUG_STRING ("CTRL+^"),  OCTI_DEBUG_STRING ("RecordSeparator")          }, \
  { 0x1F, OCTI_DEBUG_STRING ("CTRL+_"),  OCTI_DEBUG_STRING ("UnitSeparator")            }  \
}

/**
  Get control character mapping for a given character.

  @param[in]  ControlCharTable  Pointer to the control character table.
  @param[in]  TableSize         Size of the control character table.
  @param[in]  ControlChar       The control character to get mapping for.

  @retval  Pointer to OCTI_CONTROL_CHAR_MAPPING structure, or NULL if not found.
**/
STATIC
OCTI_CONTROL_CHAR_MAPPING *
OctiGetControlCharMapping (
  IN OCTI_CONTROL_CHAR_MAPPING  *ControlCharTable,
  IN UINTN                      TableSize,
  IN CHAR16                     ControlChar
  )
{
  UINTN  Index;

  for (Index = 0; Index < TableSize; Index++) {
    if (ControlCharTable[Index].ControlChar == (UINT8)ControlChar) {
      return &ControlCharTable[Index];
    }
  }

  return NULL;
}

/**
  Log control character information for debugging.

  @param[in]  ControlChar  The control character that was detected.
**/
STATIC
VOID
OctiLogControlChar (
  IN CHAR16  ControlChar
  )
{
 #ifdef DEBUG
  STATIC OCTI_CONTROL_CHAR_MAPPING  mControlCharTable[] = OCTI_CONTROL_CHAR_TABLE_INIT;
  OCTI_CONTROL_CHAR_MAPPING         *Mapping;

  Mapping = OctiGetControlCharMapping (mControlCharTable, ARRAY_SIZE (mControlCharTable), ControlChar);
  if (Mapping != NULL) {
    OCTI_DEBUG_VERBOSE (
      "Control character detected: 0x%02X (%s)\n",
      (UINT8)ControlChar,
      Mapping->Description
      );
  } else {
    OCTI_DEBUG_VERBOSE (
      "Unknown control character: 0x%02X\n",
      (UINT8)ControlChar
      );
  }

 #endif
}

#endif // OC_TEXT_INPUT_COMMON_H