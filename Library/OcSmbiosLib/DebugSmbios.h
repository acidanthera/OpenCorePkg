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

#ifndef DEBUG_SMBIOS_H_
#define DEBUG_SMBIOS_H_

// TODO: Use PCD?
#define LOG_SMBIOS

#ifdef LOG_SMBIOS
  #define DEBUG_SMBIOS(arg) DEBUG (arg)
#else
  #define DEBUG_SMBIOS(arg)
#endif

VOID
SmbiosDebugGeneric (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugBiosInformation (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugSystemInformation (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugBaseboardInformation (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugSystemEnclosure (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugProcessorInformation (
  IN  SMBIOS_TABLE_ENTRY_POINT       *Smbios,
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugCacheInformation (
  IN APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugSystemPorts (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugSystemSlots (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugPhysicalMemoryArray (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugMemoryDevice (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugType19Device (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugType20Device (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugPortableBatteryDevice (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugBootInformation (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugAppleFirmwareVolume (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugAppleProcessorType (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

VOID
SmbiosDebugAppleProcessorSpeed (
  IN  APPLE_SMBIOS_STRUCTURE_POINTER  Record
  );

#endif // DEBUG_SMBIOS_H_
