/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_USER_UTILITIES_OCVALIDATE_H
#define OC_USER_UTILITIES_OCVALIDATE_H

#include <Library/OcConfigurationLib.h>

#include <sys/time.h>

//
// TODO: Doc functions and add comments where necessary.
//

/**
  Get current timestamp in milliseconds.
  
  @retval Current timestamp in milliseconds.
**/
INT64
GetCurrentTimestamp (
  VOID
  );

/**
  Get ASCII filename suffix.

  @param[in]  FileName          Filename given.

  @retval     Empty string      If the FileName is abnormal.
  @retval     FileName suffix   If the FileName is sane.
**/
CHAR8 *
AsciiGetFilenameSuffix (
  IN  CONST CHAR8  *FileName
  );

/**
  Check if filename has given suffix.

  @param[in]  FileName          Filename given.
  @param[in]  Suffix            Suffix to be matched and checked.

  @retval     TRUE              If FileName has Suffix in the end.
**/
BOOLEAN
AsciiFileNameHasSuffix (
  IN  CONST CHAR8  *FileName,
  IN  CONST CHAR8  *Suffix
  );

/**
  Check if a filesystem path contains only legal characters.

  @param[in]  Path              Filesystem path to be checked.

  @retval     TRUE              If Path only contains 0-9, A-Z, a-z, '_', '-', '.', '/', and '\'.
**/
BOOLEAN
AsciiFileSystemPathIsLegal (
  IN  CONST CHAR8  *Path
  );

/**
  Check if an OpenCore Configuration Comment contains only ASCII printable characters.

  @param[in]  Comment           Comment to be checked.

  @retval     TRUE              If Comment only contains ASCII printable characters.
**/
BOOLEAN
AsciiCommentIsLegal (
  IN  CONST CHAR8  *Comment
  );

/**
  Check if an OpenCore Configuration Identifier matches specific conventions.

  @param[in]  Identifier               Identifier to be checked.
  @param[in]  IsKernelPatchIdentifier  TRUE to perform special checks for Kernel->Patch->Identifier.

  @retval     TRUE                     If Identifier matches conventions.
**/
BOOLEAN
AsciiIdentifierIsLegal (
  IN  CONST CHAR8    *Identifier,
  IN        BOOLEAN  IsKernelIdentifier
  );

/**
  Check if an OpenCore Configuration Arch matches specific conventions.

  @param[in]  Arch                     Arch to be checked.

  @retval     TRUE                     If Arch has value of either: Any, i386, or x86_64.
**/
BOOLEAN
AsciiArchIsLegal (
  IN  CONST CHAR8  *Arch
  );

BOOLEAN
AsciiDevicePathIsLegal (
  IN  CONST CHAR8  *DevicePath
  );

BOOLEAN
AsciiDevicePropertyIsLegal (
  IN  CONST CHAR8  *DeviceProperty
  );

BOOLEAN
AsciiUefiDriverIsLegal (
  IN  CONST CHAR8  *Driver
  );

UINT32
ReportError (
  IN  CONST CHAR8  *FuncName,
  IN  UINT32       ErrorCount
  );

BOOLEAN
DataHasProperMasking (
  IN  CONST VOID   *Data,
  IN  CONST VOID   *Mask,
  IN        UINTN  Size
  );

UINT32
CheckACPI (
  IN  OC_GLOBAL_CONFIG  *Config
  );

UINT32
CheckBooter (
  IN  OC_GLOBAL_CONFIG  *Config
  );

UINT32
CheckDeviceProperties (
  IN  OC_GLOBAL_CONFIG  *Config
  );

UINT32
CheckKernel (
  IN  OC_GLOBAL_CONFIG  *Config
  );

UINT32
CheckMisc (
  IN  OC_GLOBAL_CONFIG  *Config
  );

UINT32
CheckNVRAM (
  IN  OC_GLOBAL_CONFIG  *Config
  );

UINT32
CheckPlatformInfo (
  IN  OC_GLOBAL_CONFIG  *Config
  );

UINT32
CheckUEFI (
  IN  OC_GLOBAL_CONFIG  *Config
  );

UINT32
CheckConfig (
  IN  OC_GLOBAL_CONFIG  *Config
  );

typedef
UINT32
(*CONFIG_CHECK) (
  IN  OC_GLOBAL_CONFIG  *Config
  );

#endif // OC_USER_UTILITIES_OCVALIDATE_H
