/** @file
  Null OcLogAggregatorLib instance.

  Copyright (c) 2023, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcDebugLogLib.h>

VOID
EFIAPI
OcPrintScreen (
  IN  CONST CHAR16  *Format,
  ...
  )
{
}

APPLE_DEBUG_LOG_PROTOCOL *
OcAppleDebugLogInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  return NULL;
}

VOID
OcAppleDebugLogConfigure (
  IN BOOLEAN  Enable
  )
{
}

VOID
OcAppleDebugLogPerfAllocated (
  IN OUT VOID   *PerfBuffer,
  IN     UINTN  PerfBufferSize
  )
{
}

EFI_STATUS
OcConfigureLogProtocol (
  IN OC_LOG_OPTIONS                   Options,
  IN CONST CHAR8                      *LogModules,
  IN UINT32                           DisplayDelay,
  IN UINTN                            DisplayLevel,
  IN UINTN                            HaltLevel,
  IN CONST CHAR16                     *LogPrefixPath  OPTIONAL,
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *LogFileSystem  OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}
