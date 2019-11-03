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

#define OC_HEX_LOWER(x) "0123456789ABCDEF"[((UINT32) (x) & 0x0FU)]
#define OC_HEX_UPPER(x) "0123456789ABCDEF"[((UINT32) (x) & 0xF0U) >> 4U]

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
  Install or update the OcLog protocol with specified options.

  @param[in] Options       Logging options.
  @param[in] Delay         Delay in microseconds after each log entry.
  @param[in] DisplayLevel  Console visible error level.
  @param[in] HaltLevel     Error level causing CPU halt.
  @param[in] LogPrefixPath Log path (without timestamp).
  @param[in] LogFileSystem Log filesystem, optional.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
OcConfigureLogProtocol (
  IN OC_LOG_OPTIONS                   Options,
  IN UINT32                           DisplayDelay,
  IN UINTN                            DisplayLevel,
  IN UINTN                            HaltLevel,
  IN CONST CHAR16                     *LogPrefixPath  OPTIONAL,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *LogFileSystem  OPTIONAL
  );

/**
  Prints via gST->ConOut without any pool allocations.
  Otherwise equivalent to Print.
  Note: EFIAPI must be present for VA_ARGS forwarding (causes bugs with gcc).

  @param[in]  Format  Formatted string.
**/
VOID
EFIAPI
OcPrintScreen (
  IN  CONST CHAR16   *Format,
  ...
  );

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
  CONST CHAR16 *Message
  );

/**
  Default index mapping macros.
**/
#define OC_INPUT_STR      "123456789ABCDEFGHIJKLMNOPQRSTUVXWZ"
#define OC_INPUT_MAX      L_STR_LEN (OC_INPUT_STR)
#define OC_INPUT_ABORTED  -1 ///< Esc or 0
#define OC_INPUT_INVALID  -2 ///< Some other key
#define OC_INPUT_TIMEOUT  -3 ///< Timeout

/**
  Obtains key index from user input.

  @param[in]  Time  Timeout to wait for.

  @returns key index [0, OC_INPUT_MAX), OC_INPUT_ABORTED, or OC_INPUT_INVALID, OC_INPUT_MAX.
**/
INTN
WaitForKeyIndex (
  IN UINTN  Timeout
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
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
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
