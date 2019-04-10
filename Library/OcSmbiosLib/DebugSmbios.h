/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef DEBUG_SMBIOS_H
#define DEBUG_SMBIOS_H

#include <IndustryStandard/AppleSmBios.h>

// TODO: Use PCD?
//#define LOG_SMBIOS

#ifdef LOG_SMBIOS
  #define DEBUG_SMBIOS(arg) DEBUG (arg)
#else
  #define DEBUG_SMBIOS(arg)
#endif

VOID
SmbiosDebugAnyStructure (
  IN APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

#endif // DEBUG_SMBIOS_H
