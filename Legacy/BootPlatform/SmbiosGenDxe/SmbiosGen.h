/** @file

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name: 

  DataHubGen.h

Abstract:

**/

#ifndef _SMBIOS_GEN_H_
#define _SMBIOS_GEN_H_

#include <IndustryStandard/SmBios.h>

#include <Guid/HobList.h>
#include <Guid/SmBios.h>

#include <Protocol/Smbios.h>
#include <Protocol/HiiDatabase.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiHiiServicesLib.h>

#define   PRODUCT_NAME                  L"DUET"
#define   PRODUCT_VERSION               L"Release"

#define   FIRMWARE_PRODUCT_NAME         (PRODUCT_NAME L": ")
#ifdef EFI32
#define   FIRMWARE_BIOS_VERSIONE        (PRODUCT_NAME L"(IA32.UEFI)" PRODUCT_VERSION L": ")
#else  // EFIX64
#define   FIRMWARE_BIOS_VERSIONE        (PRODUCT_NAME L"(X64.UEFI)"  PRODUCT_VERSION L": ")
#endif

SMBIOS_STRUCTURE_POINTER
GetSmbiosTableFromType (
  IN SMBIOS_TABLE_ENTRY_POINT  *Smbios,
  IN UINT8                 Type,
  IN UINTN                 Index
  );

CHAR8 *
GetSmbiosString (
  IN SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN SMBIOS_TABLE_STRING       String
  );

/**
  Logs SMBIOS record.

  @param  Smbios   Pointer to SMBIOS protocol instance.
  @param  Buffer   Pointer to the data buffer.

**/
VOID
LogSmbiosData (
  IN   EFI_SMBIOS_PROTOCOL        *Smbios,
  IN   UINT8                      *Buffer
  );

#endif
