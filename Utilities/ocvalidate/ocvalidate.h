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

INT64
GetCurrentTimestamp (
  VOID
  );

CHAR8 *
AsciiGetFilenameSuffix (
  IN  CONST CHAR8  *FileName
  );

BOOLEAN
AsciiFileNameHasSuffix (
  IN  CONST CHAR8  *FileName,
  IN  CONST CHAR8  *Suffix
  );

BOOLEAN
AsciiFileSystemPathIsLegal (
  IN  CONST CHAR8  *Path
  );

BOOLEAN
AsciiCommentIsLegal (
  IN  CONST CHAR8  *Comment
  );

BOOLEAN
AsciiIdentifierIsLegal (
  IN  CONST CHAR8  *Identifier
  );

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

STATIC CONFIG_CHECK ConfigCheckers[] = {
    &CheckACPI,
    &CheckBooter,
    &CheckDeviceProperties,
    &CheckKernel,
    &CheckMisc,
    &CheckNVRAM,
    &CheckPlatformInfo,
    &CheckUEFI
};

#endif // OC_USER_UTILITIES_OCVALIDATE_H
