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

#ifndef OC_LOG_AGGREGATOR_LIB_H
#define OC_LOG_AGGREGATOR_LIB_H

#include <Library/DebugLib.h>
#include <Protocol/OcLog.h>
#include <Protocol/AppleDebugLog.h>

/**
  Install or update the OcLog protocol with specified options.

  @param[in] Options        Logging options.
  @param[in] DisplayDelay   Delay in microseconds after each log entry.
  @param[in] DisplayLevel   Console visible error level.
  @param[in] HaltLevel      Error level causing CPU halt.
  @param[in] LogPrefixPath  Log path (without timestamp).
  @param[in] LogFileSystem  Log filesystem, optional.

  Note: If LogFileSystem is specified, and it is not writable, then
  the first writable file system is chosen.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
OcConfigureLogProtocol (
  IN OC_LOG_OPTIONS                   Options,
  IN CONST CHAR8                      *LogModules,
  IN UINT32                           DisplayDelay,
  IN UINTN                            DisplayLevel,
  IN UINTN                            HaltLevel,
  IN CONST CHAR16                     *LogPrefixPath  OPTIONAL,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *LogFileSystem  OPTIONAL
  );

/**
  Install and initialise the Apple Debug Log protocol.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed or located protocol.
  @retval NULL  There was an error locating or installing the protocol.
**/
APPLE_DEBUG_LOG_PROTOCOL *
OcAppleDebugLogInstallProtocol (
  IN BOOLEAN  Reinstall
  );

/**
  Configure Apple Debug Log protocol.

  @param[in] Enable  Enable logging to OcLog.
**/
VOID
OcAppleDebugLogConfigure (
  IN BOOLEAN  Enable
  );

/**
  Configure Apple performance log location.

  @param[in,out]  PerfBuffer       Performance buffer location.
  @param[in]      PerfBufferSize   Performance buffer size.
**/
VOID
OcAppleDebugLogPerfAllocated (
  IN OUT VOID  *PerfBuffer,
  IN     UINTN  PerfBufferSize
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

#endif // OC_LOG_AGGREGATOR_LIB_H
