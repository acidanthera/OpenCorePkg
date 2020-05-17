/** @file

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SmbiosGen.c

Abstract:

**/

#include "SmbiosGen.h"
extern UINT8                SmbiosGenDxeStrings[];
EFI_SMBIOS_PROTOCOL         *gSmbios;
EFI_HII_HANDLE              gStringHandle;

VOID *
GetSmbiosTablesFromHob (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS       *Table;
  EFI_PEI_HOB_POINTERS        GuidHob;

  GuidHob.Raw = GetFirstGuidHob (&gEfiSmbiosTableGuid);
  if (GuidHob.Raw != NULL) {
    Table = GET_GUID_HOB_DATA (GuidHob.Guid);
    if (Table != NULL) {
      return (VOID *)(UINTN)*Table;
    }
  }

  return NULL;
}


VOID
InstallProcessorSmbios (
  IN VOID                  *Smbios
  )
{
  SMBIOS_STRUCTURE_POINTER          SmbiosTable;
  CHAR8                             *AString;
  CHAR16                            *UString;
  EFI_STRING_ID                     Token;

  //
  // Processor info (TYPE 4)
  // 
  SmbiosTable = GetSmbiosTableFromType ((SMBIOS_TABLE_ENTRY_POINT *)Smbios, 4, 0);
  if (SmbiosTable.Raw == NULL) {
    DEBUG ((EFI_D_ERROR, "SmbiosTable: Type 4 (Processor Info) not found!\n"));
    return ;
  }

  //
  // Log Smbios Record Type4
  //
  LogSmbiosData(gSmbios,(UINT8*)SmbiosTable.Type4);

  //
  // Set ProcessorVersion string
  //
  AString = GetSmbiosString (SmbiosTable, SmbiosTable.Type4->ProcessorVersion);
  UString = AllocateZeroPool ((AsciiStrLen(AString) + 1) * sizeof(CHAR16));
  ASSERT (UString != NULL);
  AsciiStrToUnicodeStr (AString, UString);

  Token = HiiSetString (gStringHandle, 0, UString, NULL);
  if (Token == 0) {
    gBS->FreePool (UString);
    return ;
  }
  gBS->FreePool (UString);
  return ;
}

VOID
InstallCacheSmbios (
  IN VOID                  *Smbios
  )
{
  return ;
}

VOID
InstallMemorySmbios (
  IN VOID                  *Smbios
  )
{
  SMBIOS_STRUCTURE_POINTER          SmbiosTable;

  //
  // Generate Memory Array Mapped Address info (TYPE 19)
  //
  SmbiosTable = GetSmbiosTableFromType ((SMBIOS_TABLE_ENTRY_POINT *)Smbios, 19, 0);
  if (SmbiosTable.Raw == NULL) {
    DEBUG ((EFI_D_ERROR, "SmbiosTable: Type 19 (Memory Array Mapped Address Info) not found!\n"));
    return ;
  }

  //
  // Record Smbios Type 19
  //
  LogSmbiosData(gSmbios, (UINT8*)SmbiosTable.Type19);
  return ;
}

VOID
InstallMiscSmbios (
  IN VOID                  *Smbios
  )
{
  SMBIOS_STRUCTURE_POINTER          SmbiosTable;
  CHAR8                             *AString;
  CHAR16                            *UString;
  EFI_STRING_ID                     Token;

  //
  // BIOS information (TYPE 0)
  // 
  SmbiosTable = GetSmbiosTableFromType ((SMBIOS_TABLE_ENTRY_POINT *)Smbios, 0, 0);
  if (SmbiosTable.Raw == NULL) {
    DEBUG ((EFI_D_ERROR, "SmbiosTable: Type 0 (BIOS Information) not found!\n"));
    return ;
  }

  //
  // Record Type 2
  //
  AString = GetSmbiosString (SmbiosTable, SmbiosTable.Type0->BiosVersion);
  UString = AllocateZeroPool ((AsciiStrLen(AString) + 1) * sizeof(CHAR16) + sizeof(FIRMWARE_BIOS_VERSIONE));
  ASSERT (UString != NULL);
  CopyMem (UString, FIRMWARE_BIOS_VERSIONE, sizeof(FIRMWARE_BIOS_VERSIONE));
  AsciiStrToUnicodeStr (AString, UString + sizeof(FIRMWARE_BIOS_VERSIONE) / sizeof(CHAR16) - 1);

  Token = HiiSetString (gStringHandle, 0, UString, NULL);
  if (Token == 0) {
    gBS->FreePool (UString);
    return ;
  }
  gBS->FreePool (UString);

  //
  // Log Smios Type 0
  //
  LogSmbiosData(gSmbios, (UINT8*)SmbiosTable.Type0);
  
  //
  // System information (TYPE 1)
  // 
  SmbiosTable = GetSmbiosTableFromType ((SMBIOS_TABLE_ENTRY_POINT *)Smbios, 1, 0);
  if (SmbiosTable.Raw == NULL) {
    DEBUG ((EFI_D_ERROR, "SmbiosTable: Type 1 (System Information) not found!\n"));
    return ;
  }

  //
  // Record Type 3
  //
  AString = GetSmbiosString (SmbiosTable, SmbiosTable.Type1->ProductName);
  UString = AllocateZeroPool ((AsciiStrLen(AString) + 1) * sizeof(CHAR16) + sizeof(FIRMWARE_PRODUCT_NAME));
  ASSERT (UString != NULL);
  CopyMem (UString, FIRMWARE_PRODUCT_NAME, sizeof(FIRMWARE_PRODUCT_NAME));
  AsciiStrToUnicodeStr (AString, UString + sizeof(FIRMWARE_PRODUCT_NAME) / sizeof(CHAR16) - 1);

  Token = HiiSetString (gStringHandle, 0, UString, NULL);
  if (Token == 0) {
    gBS->FreePool (UString);
    return ;
  }
  gBS->FreePool (UString);

  //
  // Log Smbios Type 1
  //
  LogSmbiosData(gSmbios, (UINT8*)SmbiosTable.Type1);
  
  return ;
}

EFI_STATUS
EFIAPI
SmbiosGenEntrypoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;
  VOID                    *Smbios;

  Smbios = GetSmbiosTablesFromHob ();
  if (Smbios == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID**)&gSmbios
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  gStringHandle = HiiAddPackages (
                    &gEfiCallerIdGuid,
                    NULL,
                    SmbiosGenDxeStrings,
                    NULL
                    );
  ASSERT (gStringHandle != NULL);

  InstallProcessorSmbios (Smbios);
  InstallCacheSmbios     (Smbios);
  InstallMemorySmbios    (Smbios);
  InstallMiscSmbios      (Smbios);

  return EFI_SUCCESS;
}

//
// Internal function
//

UINTN
SmbiosTableLength (
  IN SMBIOS_STRUCTURE_POINTER SmbiosTable
  )
{
  CHAR8  *AChar;
  UINTN  Length;

  AChar = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Hdr->Length);
  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar ++;
  }
  Length = ((UINTN)AChar - (UINTN)SmbiosTable.Raw + 2);
  
  return Length;
}

SMBIOS_STRUCTURE_POINTER
GetSmbiosTableFromType (
  IN SMBIOS_TABLE_ENTRY_POINT  *Smbios,
  IN UINT8                     Type,
  IN UINTN                     Index
  )
{
  SMBIOS_STRUCTURE_POINTER SmbiosTable;
  UINTN                    SmbiosTypeIndex;
  
  SmbiosTypeIndex = 0;
  SmbiosTable.Raw = (UINT8 *)(UINTN)Smbios->TableAddress;
  if (SmbiosTable.Raw == NULL) {
    return SmbiosTable;
  }
  while ((SmbiosTypeIndex != Index) || (SmbiosTable.Hdr->Type != Type)) {
    if (SmbiosTable.Hdr->Type == 127) {
      SmbiosTable.Raw = NULL;
      return SmbiosTable;
    }
    if (SmbiosTable.Hdr->Type == Type) {
      SmbiosTypeIndex ++;
    }
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosTableLength (SmbiosTable));
  }

  return SmbiosTable;
}

CHAR8 *
GetSmbiosString (
  IN SMBIOS_STRUCTURE_POINTER  SmbiosTable,
  IN SMBIOS_TABLE_STRING       String
  )
{
  CHAR8      *AString;
  UINT8      Index;

  Index = 1;
  AString = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Hdr->Length);
  while (Index != String) {
    while (*AString != 0) {
      AString ++;
    }
    AString ++;
    if (*AString == 0) {
      return AString;
    }
    Index ++;
  }

  return AString;
}


/**
  Logs SMBIOS record.

  @param  Smbios   Pointer to SMBIOS protocol instance.
  @param  Buffer   Pointer to the data buffer.

**/
VOID
LogSmbiosData (
  IN   EFI_SMBIOS_PROTOCOL        *Smbios,
  IN   UINT8                      *Buffer
  )
{
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  SmbiosHandle;
  
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (
                     Smbios,
                     NULL,
                     &SmbiosHandle,
                     (EFI_SMBIOS_TABLE_HEADER*)Buffer
                     );
  ASSERT_EFI_ERROR (Status);
}
