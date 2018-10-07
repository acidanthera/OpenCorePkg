/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcPrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Guid/DebugImageInfoTable.h>
#include <Guid/DxeServices.h>
#include <Guid/HobList.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/Mps.h>
#include <Guid/PropertiesTable.h>
#include <Guid/SalSystemTable.h>
#include <Guid/SmBios.h>
#include <Guid/SystemResourceTable.h>
#include <Guid/LzmaDecompress.h>

#include <Protocol/AppleKeyMapAggregator.h>
#include <Protocol/AppleKeyMapDatabase.h>
#include <Protocol/Decompress.h>
#include <Protocol/LoadedImage.h>

#include "GuidTable.h"

GLOBAL_REMOVE_IF_UNREFERENCED CONST GUID_NAME_MAP GuidStringMap[] = {
  // Apple Guids

  { &gEfiDevicePathPropertyDatabaseProtocolGuid, "AppleDevicePathPropertyDatabaseProtocol" },
  { &gAppleFirmwarePasswordProtocolGuid, "AppleFirmwarePasswordProtocol" },
  { &gAppleKeyMapAggregatorProtocolGuid, "AppleKeyMapAggregatorProtocol" },
  { &gAppleKeyMapDatabaseProtocolGuid, "AppleKeyMapDatabaseProtocol" },

  // Tianocore Guids

  { &gEfiDecompressProtocolGuid, "EfiDecompressProtocol" },

  { &gLzmaCustomDecompressGuid, "LZMA Compress" },

  { &gEfiAcpiTableGuid, "ACPI Table" },
  { &gEfiAcpi10TableGuid, "ACPI 1.0 Table" },
  { &gEfiAcpi20TableGuid, "ACPI 2.0 Table" },

  { &gEfiDxeServicesTableGuid, "DXE Services" },
  { &gEfiSalSystemTableGuid, "SAL System Table" },

  { &gEfiSmbiosTableGuid, "SMBIOS Table" },
  { &gEfiSmbios3TableGuid, "SMBIOS 3 Table" },

  { &gEfiMpsTableGuid, "MPS Table" },
  { &gEfiHobListGuid, "HOB List Table" },
  { &gEfiMemoryTypeInformationGuid, "Memory Type Information" },

  { &gEfiPropertiesTableGuid, "EFI Propterties Table" },
  { &gEfiDebugImageInfoTableGuid, "EFI Debug Image Info Table" },
  { &gEfiSystemResourceTableGuid, "EFI System Resource Table" },

  { NULL, NULL }
};

// OcGuidToString
/**

  @param[in] Guid  A pointer to the guid to convert to string.

  @retval  A pointer to the buffer containg the string representation of the guid.
**/
CHAR8 *
EFIAPI
OcGuidToString (
  IN EFI_GUID  *Guid
  )
{
  UINTN        Index;

  STATIC CHAR8 GuidStringBuffer[36];

  for (Index = 0; GuidStringMap[Index].Guid != NULL; ++Index) {
    if (CompareGuid (GuidStringMap[Index].Guid, Guid)) {
      return GuidStringMap[Index].ShortName;
    }
  }

  // Output numeric version of guid instead.
  OcSPrint (&GuidStringBuffer, sizeof (GuidStringBuffer), (FORMAT_ASCII | OUTPUT_ASCII), "%G", Guid);
  
  return (CHAR8 *)&GuidStringBuffer[0];
}
