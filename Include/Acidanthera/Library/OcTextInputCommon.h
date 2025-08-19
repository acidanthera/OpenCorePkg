/** @file
  OcTextInputCommon - Common definiti//
// Control character lookup table - shared between library and driver
//OctiLogControlChar (
  IN CHAR16  ControlChar
  )
{
#ifdef DEBUG
  STATIC OCTI_CONTROL_CHAR_MAPPING  mControlCharTable[] = OCTI_CONTROL_CHAR_TABLE_INIT;
  OCTI_CONTROL_CHAR_MAPPING         *Mapping;

  Mapping = OctiGetControlCharMapping (mControlCharTable, ARRAY_SIZE (mControlCharTable), ControlChar);OCTI_CONTROL_CHAR_TABLE_INIT  { \for SimpleTextInputEx compatibility
  
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
// Debug prefix for all OcTextInput components
//
#define OCTI_DEBUG_PREFIX  "OCTI: "

//
// Debug macros for consistent logging
//
#define OCTI_DEBUG_INFO(Format, ...)     DEBUG ((DEBUG_INFO, OCTI_DEBUG_PREFIX Format, ##__VA_ARGS__))
#define OCTI_DEBUG_VERBOSE(Format, ...)  DEBUG ((DEBUG_VERBOSE, OCTI_DEBUG_PREFIX Format, ##__VA_ARGS__))
#define OCTI_DEBUG_WARN(Format, ...)     DEBUG ((DEBUG_WARN, OCTI_DEBUG_PREFIX Format, ##__VA_ARGS__))
#define OCTI_DEBUG_ERROR(Format, ...)    DEBUG ((DEBUG_ERROR, OCTI_DEBUG_PREFIX Format, ##__VA_ARGS__))

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
  UINT8   ControlChar;    ///< Control character code (0x01-0x1F)
  CHAR16  *Description;   ///< Human-readable description (e.g., "CTRL+A")
  CHAR16  *ShellFunction; ///< Shell function name (e.g., "StartOfLine")
} OCTI_CONTROL_CHAR_MAPPING;

//
// Control character lookup table - shared between library and driver
//
#define OCTI_CONTROL_CHAR_TABLE_INIT { \
  { 0x01, OCTI_DEBUG_STRING (L"CTRL+A"),  OCTI_DEBUG_STRING (L"StartOfLine")              }, \
  { 0x02, OCTI_DEBUG_STRING (L"CTRL+B"),  OCTI_DEBUG_STRING (L"CursorLeft")               }, \
  { 0x03, OCTI_DEBUG_STRING (L"CTRL+C"),  OCTI_DEBUG_STRING (L"Break")                    }, \
  { 0x04, OCTI_DEBUG_STRING (L"CTRL+D"),  OCTI_DEBUG_STRING (L"Delete")                   }, \
  { 0x05, OCTI_DEBUG_STRING (L"CTRL+E"),  OCTI_DEBUG_STRING (L"EndOfLine")                }, \
  { 0x06, OCTI_DEBUG_STRING (L"CTRL+F"),  OCTI_DEBUG_STRING (L"CursorRight")              }, \
  { 0x07, OCTI_DEBUG_STRING (L"CTRL+G"),  OCTI_DEBUG_STRING (L"Bell")                     }, \
  { 0x08, OCTI_DEBUG_STRING (L"CTRL+H"),  OCTI_DEBUG_STRING (L"Backspace")                }, \
  { 0x09, OCTI_DEBUG_STRING (L"CTRL+I"),  OCTI_DEBUG_STRING (L"Tab")                      }, \
  { 0x0A, OCTI_DEBUG_STRING (L"CTRL+J"),  OCTI_DEBUG_STRING (L"LineFeed")                 }, \
  { 0x0B, OCTI_DEBUG_STRING (L"CTRL+K"),  OCTI_DEBUG_STRING (L"VerticalTab")              }, \
  { 0x0C, OCTI_DEBUG_STRING (L"CTRL+L"),  OCTI_DEBUG_STRING (L"FormFeed")                 }, \
  { 0x0D, OCTI_DEBUG_STRING (L"CTRL+M"),  OCTI_DEBUG_STRING (L"CarriageReturn")           }, \
  { 0x0E, OCTI_DEBUG_STRING (L"CTRL+N"),  OCTI_DEBUG_STRING (L"ShiftOut")                 }, \
  { 0x0F, OCTI_DEBUG_STRING (L"CTRL+O"),  OCTI_DEBUG_STRING (L"ShiftIn")                  }, \
  { 0x10, OCTI_DEBUG_STRING (L"CTRL+P"),  OCTI_DEBUG_STRING (L"DataLinkEscape")           }, \
  { 0x11, OCTI_DEBUG_STRING (L"CTRL+Q"),  OCTI_DEBUG_STRING (L"DeviceControl1")           }, \
  { 0x12, OCTI_DEBUG_STRING (L"CTRL+R"),  OCTI_DEBUG_STRING (L"DeviceControl2")           }, \
  { 0x13, OCTI_DEBUG_STRING (L"CTRL+S"),  OCTI_DEBUG_STRING (L"DeviceControl3")           }, \
  { 0x14, OCTI_DEBUG_STRING (L"CTRL+T"),  OCTI_DEBUG_STRING (L"DeviceControl4")           }, \
  { 0x15, OCTI_DEBUG_STRING (L"CTRL+U"),  OCTI_DEBUG_STRING (L"NegativeAcknowledge")      }, \
  { 0x16, OCTI_DEBUG_STRING (L"CTRL+V"),  OCTI_DEBUG_STRING (L"SynchronousIdle")          }, \
  { 0x17, OCTI_DEBUG_STRING (L"CTRL+W"),  OCTI_DEBUG_STRING (L"EndOfTransmissionBlock")   }, \
  { 0x18, OCTI_DEBUG_STRING (L"CTRL+X"),  OCTI_DEBUG_STRING (L"Cancel")                   }, \
  { 0x19, OCTI_DEBUG_STRING (L"CTRL+Y"),  OCTI_DEBUG_STRING (L"EndOfMedium")              }, \
  { 0x1A, OCTI_DEBUG_STRING (L"CTRL+Z"),  OCTI_DEBUG_STRING (L"Substitute")               }, \
  { 0x1B, OCTI_DEBUG_STRING (L"CTRL+["),  OCTI_DEBUG_STRING (L"Escape")                   }, \
  { 0x1C, OCTI_DEBUG_STRING (L"CTRL+\\"), OCTI_DEBUG_STRING (L"FileSeparator")            }, \
  { 0x1D, OCTI_DEBUG_STRING (L"CTRL+]"),  OCTI_DEBUG_STRING (L"GroupSeparator")           }, \
  { 0x1E, OCTI_DEBUG_STRING (L"CTRL+^"),  OCTI_DEBUG_STRING (L"RecordSeparator")          }, \
  { 0x1F, OCTI_DEBUG_STRING (L"CTRL+_"),  OCTI_DEBUG_STRING (L"UnitSeparator")            }  \
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
