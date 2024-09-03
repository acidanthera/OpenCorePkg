/** @file
  Boot entry protocol handler for PXE and HTTP Boot.

  Copyright (c) 2024, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "NetworkBootInternal.h"

#define ENROLL_CERT       L"--enroll-cert"
#define DELETE_CERT       L"--delete-cert"
#define DELETE_ALL_CERTS  L"--delete-all-certs"

BOOLEAN  gRequireHttpsUri;

STATIC BOOLEAN  mAllowPxeBoot;
STATIC BOOLEAN  mAllowHttpBoot;
STATIC BOOLEAN  mAllowIpv4;
STATIC BOOLEAN  mAllowIpv6;
STATIC BOOLEAN  mAuxEntries;
STATIC CHAR16   *mHttpBootUri;

STATIC CHAR16  PxeBootId[]  = L"PXE Boot IPv";
STATIC CHAR16  HttpBootId[] = L"HTTP Boot IPv";

VOID
InternalFreePickerEntry (
  IN   OC_PICKER_ENTRY  *Entry
  )
{
  ASSERT (Entry != NULL);

  if (Entry == NULL) {
    return;
  }

  if (Entry->Id != NULL) {
    FreePool ((CHAR8 *)Entry->Id);
  }

  if (Entry->Name != NULL) {
    FreePool ((CHAR8 *)Entry->Name);
  }

  if (Entry->Path != NULL) {
    FreePool ((CHAR8 *)Entry->Path);
  }

  if (Entry->Arguments != NULL) {
    FreePool ((CHAR8 *)Entry->Arguments);
  }

  if (Entry->UnmanagedDevicePath != NULL) {
    FreePool (Entry->UnmanagedDevicePath);
  }
}

STATIC
VOID
EFIAPI
FreeNetworkBootEntries (
  IN   OC_PICKER_ENTRY  **Entries,
  IN   UINTN            NumEntries
  )
{
  UINTN  Index;

  ASSERT (Entries   != NULL);
  ASSERT (*Entries  != NULL);
  if ((Entries == NULL) || (*Entries == NULL)) {
    return;
  }

  for (Index = 0; Index < NumEntries; Index++) {
    InternalFreePickerEntry (&(*Entries)[Index]);
  }

  FreePool (*Entries);
  *Entries = NULL;
}

STATIC
EFI_STATUS
InternalAddEntry (
  OC_FLEX_ARRAY  *FlexPickerEntries,
  CHAR16         *Description,
  EFI_HANDLE     Handle,
  CHAR16         *HttpBootUri,
  BOOLEAN        IsIPv4,
  BOOLEAN        IsHttpBoot
  )
{
  EFI_STATUS                Status;
  OC_PICKER_ENTRY           *PickerEntry;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  UINTN                     IdLen;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "NTBT: Missing device path - %r\n",
      Status
      ));
    return Status;
  }

  PickerEntry = OcFlexArrayAddItem (FlexPickerEntries);
  if (PickerEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  IdLen           = StrLen (Description);
  PickerEntry->Id = AllocatePool ((IdLen + 1) * sizeof (PickerEntry->Id[0]));
  if (PickerEntry->Id == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeStrToAsciiStrS (Description, (CHAR8 *)PickerEntry->Id, IdLen + 1);

  PickerEntry->Name = AllocateCopyPool (IdLen + 1, PickerEntry->Id);
  if (PickerEntry->Name == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (IsHttpBoot && (HttpBootUri != NULL)) {
    Status = HttpBootAddUri (DevicePath, HttpBootUri, OcStringFormatUnicode, &NewDevicePath);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    NewDevicePath = DuplicateDevicePath (DevicePath);
    if (NewDevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  PickerEntry->UnmanagedDevicePath = NewDevicePath;

  if (IsHttpBoot) {
    PickerEntry->CustomRead = HttpBootCustomRead;
    PickerEntry->CustomFree = HttpBootCustomFree;
    PickerEntry->Flavour    = IsIPv4 ? OC_FLAVOUR_HTTP_BOOT4 : OC_FLAVOUR_HTTP_BOOT6;
  } else {
    PickerEntry->CustomRead = PxeBootCustomRead;
    PickerEntry->Flavour    = IsIPv4 ? OC_FLAVOUR_PXE_BOOT4 : OC_FLAVOUR_PXE_BOOT6;
  }

  PickerEntry->TextMode  = TRUE;
  PickerEntry->Auxiliary = mAuxEntries;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
GetNetworkBootEntries (
  IN OUT          OC_PICKER_CONTEXT  *PickerContext,
  IN     CONST EFI_HANDLE            Device OPTIONAL,
  OUT       OC_PICKER_ENTRY          **Entries,
  OUT       UINTN                    *NumEntries
  )
{
  EFI_STATUS     Status;
  UINTN          HandleCount;
  EFI_HANDLE     *HandleBuffer;
  UINTN          Index;
  CHAR16         *NetworkDescription;
  CHAR16         *IdStr;
  OC_FLEX_ARRAY  *FlexPickerEntries;
  BOOLEAN        IsIPv4;
  BOOLEAN        IsHttpBoot;

  //
  // Here we produce custom entries only, not entries found on filesystems.
  //
  if (Device != NULL) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadFileProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTBT: Load file protocol - %r\n", Status));
    return Status;
  }

  FlexPickerEntries = OcFlexArrayInit (sizeof (OC_PICKER_ENTRY), (OC_FLEX_ARRAY_FREE_ITEM)InternalFreePickerEntry);
  if (FlexPickerEntries == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    NetworkDescription = BmGetNetworkDescription (HandleBuffer[Index]);
    if (NetworkDescription == NULL) {
      DebugPrintDevicePathForHandle (DEBUG_INFO, "NTBT: LoadFile handle not PXE/HTTP boot DP", HandleBuffer[Index]);
    } else {
      //
      // Use fixed format network description which we control as shortcut
      // to identify PXE/HTTP and IPv4/6.
      //
      if ((IdStr = StrStr (NetworkDescription, PxeBootId)) != NULL) {
        IsIPv4 = IdStr[L_STR_LEN (PxeBootId)] == L'4';
        ASSERT (IsIPv4 || (IdStr[L_STR_LEN (PxeBootId)] == L'6'));
        IsHttpBoot = FALSE;
      } else if ((IdStr = StrStr (NetworkDescription, HttpBootId)) != NULL) {
        IsIPv4 = IdStr[L_STR_LEN (HttpBootId)] == L'4';
        ASSERT (IsIPv4 || (IdStr[L_STR_LEN (HttpBootId)] == L'6'));
        IsHttpBoot = TRUE;
      }

      if (  (IdStr != NULL)
         && ((IsIPv4 && mAllowIpv4) || (!IsIPv4 && mAllowIpv6))
         && ((IsHttpBoot && mAllowHttpBoot) || (!IsHttpBoot && mAllowPxeBoot))
            )
      {
        DEBUG ((DEBUG_INFO, "NTBT: Adding %s\n", NetworkDescription));
        Status = InternalAddEntry (
                   FlexPickerEntries,
                   NetworkDescription,
                   HandleBuffer[Index],
                   IsHttpBoot ? mHttpBootUri : NULL,
                   IsIPv4,
                   IsHttpBoot
                   );
      } else {
        DEBUG ((DEBUG_INFO, "NTBT: Ignoring %s\n", NetworkDescription));
      }

      FreePool (NetworkDescription);
    }

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  FreePool (HandleBuffer);

  if (EFI_ERROR (Status)) {
    OcFlexArrayFree (&FlexPickerEntries);
    return Status;
  }

  OcFlexArrayFreeContainer (&FlexPickerEntries, (VOID **)Entries, NumEntries);

  if (*NumEntries == 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EnrollCerts (
  OC_FLEX_ARRAY  *ParsedLoadOptions
  )
{
  EFI_STATUS     Status;
  UINTN          Index;
  OC_PARSED_VAR  *Option;
  EFI_GUID       *OwnerGuid;
  UINTN          CertSize;
  CHAR8          *CertData;
  BOOLEAN        EnrollCert;
  BOOLEAN        DeleteCert;
  BOOLEAN        DeleteAllCerts;
  UINTN          OptionLen;
  UINTN          DeletedCount;

  Status = EFI_SUCCESS;

  //
  // Find certs in options.
  //
  for (Index = 0; Index < ParsedLoadOptions->Count; ++Index) {
    Option = OcFlexArrayItemAt (ParsedLoadOptions, Index);

    EnrollCert     = FALSE;
    DeleteCert     = FALSE;
    DeleteAllCerts = FALSE;

    if (OcUnicodeStartsWith (Option->Unicode.Name, ENROLL_CERT, TRUE)) {
      EnrollCert = TRUE;
      OptionLen  = L_STR_LEN (ENROLL_CERT);
    } else if (OcUnicodeStartsWith (Option->Unicode.Name, DELETE_CERT, TRUE)) {
      DeleteCert = TRUE;
      OptionLen  = L_STR_LEN (DELETE_CERT);
    } else if (OcUnicodeStartsWith (Option->Unicode.Name, DELETE_ALL_CERTS, TRUE)) {
      DeleteAllCerts = TRUE;
      OptionLen      = L_STR_LEN (DELETE_ALL_CERTS);
    }

    if (  (EnrollCert || DeleteCert || DeleteAllCerts)
       && (Option->Unicode.Name[OptionLen] != CHAR_NULL)
       && (Option->Unicode.Name[OptionLen] != L':')
          )
    {
      EnrollCert     = FALSE;
      DeleteCert     = FALSE;
      DeleteAllCerts = FALSE;
    }

    if ((EnrollCert || DeleteCert) && (Option->Unicode.Value == NULL)) {
      DEBUG ((DEBUG_INFO, "NTBT: Ignoring %s option with no cert value\n", Option->Unicode.Name));
      EnrollCert = FALSE;
      DeleteCert = FALSE;
    }

    if (EnrollCert || DeleteCert || DeleteAllCerts) {
      OwnerGuid = AllocateZeroPool (sizeof (EFI_GUID));
      if (OwnerGuid == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      //
      // Use all zeros GUID if no user value supplied.
      //
      if (Option->Unicode.Name[OptionLen] == L':') {
        Status = StrToGuid (&Option->Unicode.Name[OptionLen + 1], OwnerGuid);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN, "NTBT: Cannot parse cert owner GUID from %s - %r\n", Option->Unicode.Name, Status));
          break;
        }
      }

      if (DeleteAllCerts) {
        Status = DeleteCertsForOwner (
                   EFI_TLS_CA_CERTIFICATE_VARIABLE,
                   &gEfiTlsCaCertificateGuid,
                   OwnerGuid,
                   0,
                   NULL,
                   &DeletedCount
                   );
        DEBUG ((DEBUG_INFO, "NTBT: %s %u deleted - %r\n", Option->Unicode.Name, DeletedCount, Status));
      } else {
        //
        // We do not include the terminating '\0' in the stored certificate,
        // which matches how stored by e.g. OVMF when loaded from file;
        // but we must allocate space for '\0' for Unicode to ASCII conversion.
        //
        CertSize = StrLen (Option->Unicode.Value);
        CertData = AllocateZeroPool (CertSize + 1);
        if (CertData == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }

        UnicodeStrToAsciiStrS (Option->Unicode.Value, CertData, CertSize + 1);

        if (DeleteCert) {
          Status = DeleteCertsForOwner (
                     EFI_TLS_CA_CERTIFICATE_VARIABLE,
                     &gEfiTlsCaCertificateGuid,
                     OwnerGuid,
                     CertSize,
                     CertData,
                     &DeletedCount
                     );
          DEBUG ((DEBUG_INFO, "NTBT: %s %u deleted - %r\n", Option->Unicode.Name, DeletedCount, Status));
        } else {
          Status = CertIsPresent (
                     EFI_TLS_CA_CERTIFICATE_VARIABLE,
                     &gEfiTlsCaCertificateGuid,
                     OwnerGuid,
                     CertSize,
                     CertData
                     );
          if (EFI_ERROR (Status)) {
            if (Status == EFI_ALREADY_STARTED) {
              DEBUG ((DEBUG_INFO, "NTBT: %s already present\n", Option->Unicode.Name));
              Status = EFI_SUCCESS;
            } else {
              DEBUG ((DEBUG_INFO, "NTBT: Error checking for cert presence - %r\n", Status));
            }
          } else {
            Status = EnrollX509toVariable (
                       EFI_TLS_CA_CERTIFICATE_VARIABLE,
                       &gEfiTlsCaCertificateGuid,
                       OwnerGuid,
                       CertSize,
                       CertData
                       );
            DEBUG ((DEBUG_INFO, "NTBT: %s - %r\n", Option->Unicode.Name, Status));
          }
        }

        FreePool (CertData);
      }

      FreePool (OwnerGuid);

      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  return Status;
}

STATIC
OC_BOOT_ENTRY_PROTOCOL
  mNetworkBootEntryProtocol = {
  OC_BOOT_ENTRY_PROTOCOL_REVISION,
  GetNetworkBootEntries,
  FreeNetworkBootEntries,
  NULL
};

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  OC_FLEX_ARRAY              *ParsedLoadOptions;
  CHAR16                     *TempUri;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mAllowIpv4       = FALSE;
  mAllowIpv6       = FALSE;
  mAllowPxeBoot    = FALSE;
  mAllowHttpBoot   = FALSE;
  gRequireHttpsUri = FALSE;
  mHttpBootUri     = NULL;

  Status = OcParseLoadOptions (LoadedImage, &ParsedLoadOptions);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_NOT_FOUND) {
      return Status;
    }

    Status = EFI_SUCCESS;
  } else {
    //
    // e.g. --https --uri=https://imageserver.org/OpenShell.efi
    //
    mAllowIpv4       = OcHasParsedVar (ParsedLoadOptions, L"-4", OcStringFormatUnicode);
    mAllowIpv6       = OcHasParsedVar (ParsedLoadOptions, L"-6", OcStringFormatUnicode);
    mAllowPxeBoot    = OcHasParsedVar (ParsedLoadOptions, L"--pxe", OcStringFormatUnicode);
    mAllowHttpBoot   = OcHasParsedVar (ParsedLoadOptions, L"--http", OcStringFormatUnicode);
    mAuxEntries      = OcHasParsedVar (ParsedLoadOptions, L"--aux", OcStringFormatUnicode);
    gRequireHttpsUri = OcHasParsedVar (ParsedLoadOptions, L"--https", OcStringFormatUnicode);

    TempUri = NULL;
    OcParsedVarsGetUnicodeStr (ParsedLoadOptions, L"--uri", &TempUri);
    if (TempUri != NULL) {
      mHttpBootUri = AllocateCopyPool (StrSize (TempUri), TempUri);
      if (mHttpBootUri == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
      }
    }

    if (!EFI_ERROR (Status)) {
      Status = EnrollCerts (ParsedLoadOptions);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "NTBT: Failed to enroll certs - %r\n", Status));
      }

      DEBUG_CODE_BEGIN ();
      LogInstalledCerts (EFI_TLS_CA_CERTIFICATE_VARIABLE, &gEfiTlsCaCertificateGuid);
      DEBUG_CODE_END ();
    }
  }

  if (!EFI_ERROR (Status)) {
    if (!mAllowIpv4 && !mAllowIpv6) {
      mAllowIpv4 = TRUE;
      mAllowIpv6 = TRUE;
    }

    if (!gRequireHttpsUri && !mAllowHttpBoot && !mAllowPxeBoot) {
      mAllowHttpBoot = TRUE;
      mAllowPxeBoot  = TRUE;
    }

    if (gRequireHttpsUri) {
      mAllowHttpBoot = TRUE;
    }

    if (mHttpBootUri != NULL) {
      if (!mAllowHttpBoot) {
        DEBUG ((DEBUG_INFO, "NTBT: URI specified but HTTP boot is disabled\n"));
      } else {
        if (gRequireHttpsUri && !HasHttpsUri (mHttpBootUri)) {
          DEBUG ((DEBUG_WARN, "NTBT: Invalid URI https:// is required\n"));
          mAllowHttpBoot = FALSE;
        }
      }
    }

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ImageHandle,
                    &gOcBootEntryProtocolGuid,
                    &mNetworkBootEntryProtocol,
                    NULL
                    );
  }

  if (ParsedLoadOptions != NULL) {
    OcFlexArrayFree (&ParsedLoadOptions);
  }

  if (EFI_ERROR (Status) && (mHttpBootUri != NULL)) {
    FreePool (mHttpBootUri);
  }

  return Status;
}
