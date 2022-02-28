/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_DEBUG_LOG_LIB_H
#define OC_DEBUG_LOG_LIB_H

#include <Library/DebugLib.h>
#include <Protocol/OcLog.h>
#include <Protocol/AppleDebugLog.h>

#define OC_HEX_LOWER(x) "0123456789ABCDEF"[((UINT32) (x) & 0x0FU)]
#define OC_HEX_UPPER(x) "0123456789ABCDEF"[((UINT32) (x) & 0xF0U) >> 4U]

/**
  Expand device path to human readable string.
**/
#define OC_HUMAN_STRING(TextDevicePath) \
  ((TextDevicePath) == NULL ? L"<nil>" : (TextDevicePath)[0] == '\0' ? L"<empty>" : (TextDevicePath))

/**
  Debug information that is not logged when NVRAM logging is on.
**/
#ifndef DEBUG_BULK_INFO
  #define DEBUG_BULK_INFO (DEBUG_VERBOSE|DEBUG_INFO)
#endif

/**
  This is a place print debug messages when they happen after ExitBootServices.
**/
#define RUNTIME_DEBUG(x) do { } while (0)

/**
  Pointer debug kit.
**/
#if defined(OC_TARGET_DEBUG) || defined(OC_TARGET_NOOPT)
#define DEBUG_POINTER(x) x
#elif defined(OC_TARGET_RELEASE)
#define DEBUG_POINTER(x) NULL
#else
#error "Define target macro: OC_TARGET_<TARGET>!"
#endif

/**
  Dummy function that debuggers may break on.
**/
VOID
DebugBreak (
  VOID
  );

/**
  Wait for user input after printing message.

  @param[in] Message   Message to print.
**/
VOID
WaitForKeyPress (
  IN  CONST CHAR16 *Message
  );

/**
  Print Device Path to log.

  @param[in] ErrorLevel  Debug error level.
  @param[in] Message     Prefixed message.
  @param[in] DevicePath  Device path to print.
**/
VOID
DebugPrintDevicePath (
  IN UINTN                     ErrorLevel,
  IN CONST CHAR8               *Message,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath  OPTIONAL
  );

/**
  Print Device Path corresponding to EFI Handle to log.

  @param[in] ErrorLevel  Debug error level.
  @param[in] Message     Prefixed message.
  @param[in] Handle      Handle corresponding to Device path to print.
**/
VOID
DebugPrintDevicePathForHandle (
  IN UINTN                     ErrorLevel,
  IN CONST CHAR8               *Message,
  IN EFI_HANDLE                Handle       OPTIONAL
  );

/**
  Print hex dump to log.

  @param[in] ErrorLevel  Debug error level.
  @param[in] Message     Prefixed message.
  @param[in] Bytes       Byte sequence.
  @param[in] Size        Byte sequence size.
**/
VOID
DebugPrintHexDump (
  IN UINTN                     ErrorLevel,
  IN CONST CHAR8               *Message,
  IN UINT8                     *Bytes,
  IN UINTN                     Size
  );

#endif // OC_DEBUG_LOG_LIB_H
