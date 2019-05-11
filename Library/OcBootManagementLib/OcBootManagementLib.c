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

#include <AppleMacEfi.h>
#include <Uefi.h>

#include <Guid/AppleApfsInfo.h>
#include <Guid/AppleBless.h>
#include <Guid/AppleVariable.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/OcVariables.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/OcAppleChunklistLib.h>
#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcAppleKeysLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcXmlLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/LoadedImage.h>

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;
  OC_APPLE_DISK_IMAGE_CONTEXT    *DmgContext;
  EFI_HANDLE                     BlockIoHandle;
} INTERNAL_DMG_LOAD_CONTEXT;

STATIC
CHAR16 *
GetAppleDiskLabel (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName,
  IN  CONST CHAR16                     *LabelFilename
  )
{
  CHAR16   *DiskLabelPath;
  UINTN    DiskLabelPathSize;
  CHAR8    *AsciiDiskLabel;
  CHAR16   *UnicodeDiskLabel;
  UINT32   DiskLabelLength;

  DiskLabelPathSize = StrSize (BootDirectoryName) + StrSize (LabelFilename) - sizeof (CHAR16);
  DiskLabelPath     = AllocatePool (DiskLabelPathSize);

  if (DiskLabelPath == NULL) {
    return NULL;
  }

  UnicodeSPrint (DiskLabelPath, DiskLabelPathSize, L"%s%s", BootDirectoryName, LabelFilename);
  DEBUG ((DEBUG_INFO, "Trying to get label from %s\n", DiskLabelPath));

  AsciiDiskLabel = (CHAR8 *) ReadFile (FileSystem, DiskLabelPath, &DiskLabelLength, OC_MAX_VOLUME_LABEL_SIZE);
  FreePool (DiskLabelPath);

  if (AsciiDiskLabel != NULL) {
    UnicodeDiskLabel = AsciiStrCopyToUnicode (AsciiDiskLabel, DiskLabelLength);
    if (UnicodeDiskLabel != NULL) {
      UnicodeFilterString (UnicodeDiskLabel, TRUE);
    }
    FreePool (AsciiDiskLabel);
  } else {
    UnicodeDiskLabel = NULL;
  }

  return UnicodeDiskLabel;
}

STATIC
CHAR16 *
GetAppleRecoveryNameFromPlist (
  IN CHAR8   *SystemVersionData,
  IN UINT32  SystemVersionDataSize
  )
{
  XML_DOCUMENT        *Document;
  XML_NODE            *RootDict;
  UINT32              DictSize;
  UINT32              Index;
  CONST CHAR8         *CurrentKey;
  XML_NODE            *CurrentValue;
  CONST CHAR8         *Version;
  CHAR16              *RecoveryName;
  UINTN               RecoveryNameSize;

  Document = XmlDocumentParse (SystemVersionData, SystemVersionDataSize, FALSE);

  if (Document == NULL) {
    return NULL;
  }

  RootDict = PlistNodeCast (PlistDocumentRoot (Document), PLIST_NODE_TYPE_DICT);

  if (RootDict == NULL) {
    XmlDocumentFree (Document);
    return NULL;
  }

  RecoveryName = NULL;

  DictSize = PlistDictChildren (RootDict);
  for (Index = 0; Index < DictSize; Index++) {
    CurrentKey = PlistKeyValue (PlistDictChild (RootDict, Index, &CurrentValue));

    if (CurrentKey == NULL || AsciiStrCmp (CurrentKey, "ProductUserVisibleVersion") != 0) {
      continue;
    }

    if (PlistNodeCast (CurrentValue, PLIST_NODE_TYPE_STRING) != NULL) {
      Version = XmlNodeContent (CurrentValue);
      if (Version != NULL) {
        RecoveryNameSize = L_STR_SIZE(L"Recovery ") + AsciiStrLen (Version) * sizeof (CHAR16);
        RecoveryName = AllocatePool (RecoveryNameSize);
        if (RecoveryName != NULL) {
          UnicodeSPrint (RecoveryName, RecoveryNameSize, L"Recovery %a", Version);
          UnicodeFilterString (RecoveryName, TRUE);
        }
      }
    }

    break;
  }

  XmlDocumentFree (Document);
  return RecoveryName;
}

STATIC
CHAR16 *
GetAppleRecoveryName (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem,
  IN  CONST CHAR16                     *BootDirectoryName
  )
{
  CHAR16   *SystemVersionPath;
  UINTN    SystemVersionPathSize;
  CHAR8    *SystemVersionData;
  CHAR16   *UnicodeDiskLabel;
  UINT32   SystemVersionDataSize;

  SystemVersionPathSize = StrSize (BootDirectoryName) + L_STR_SIZE_NT (L"SystemVersion.plist");
  SystemVersionPath     = AllocatePool (SystemVersionPathSize);

  if (SystemVersionPath == NULL) {
    return NULL;
  }

  UnicodeSPrint (SystemVersionPath, SystemVersionPathSize, L"%sSystemVersion.plist", BootDirectoryName);
  DEBUG ((DEBUG_INFO, "Trying to get recovery from %s\n", SystemVersionPath));
  SystemVersionData = (CHAR8 *) ReadFile (FileSystem, SystemVersionPath, &SystemVersionDataSize, BASE_1MB);
  FreePool (SystemVersionPath);

  if (SystemVersionData != NULL) {
    UnicodeDiskLabel = GetAppleRecoveryNameFromPlist (SystemVersionData, SystemVersionDataSize);
    FreePool (SystemVersionData);
  } else {
    UnicodeDiskLabel = NULL;
  }

  return UnicodeDiskLabel;
}

STATIC
EFI_STATUS
GetRecoveryOsBooter (
  IN  EFI_HANDLE                Device,
  OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath,
  IN  BOOLEAN                   Basic
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  EFI_FILE_PROTOCOL                *Root;
  EFI_FILE_PROTOCOL                *Recovery;
  UINTN                            FilePathSize;
  EFI_DEVICE_PATH_PROTOCOL         *TmpPath;
  UINTN                            TmpPathSize;

  Status = gBS->HandleProtocol (
    Device,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FilePathSize = 0;

  if (!Basic) {
    *FilePath = (EFI_DEVICE_PATH_PROTOCOL *) GetFileInfo (
                  Root,
                  &gAppleBlessedOsxFolderInfoGuid,
                  sizeof (EFI_DEVICE_PATH_PROTOCOL),
                  &FilePathSize
                  );
  } else {
    //
    // Requested basic recovery support, i.e. only com.apple.recovery.boot folder check.
    // This is useful for locating empty USB sticks with just a dmg in them.
    //
    *FilePath = NULL;
  }

  if (*FilePath != NULL) {
    if (IsDevicePathValid (*FilePath, FilePathSize)) {
      //
      // We skip alternate entry when current one is the same.
      // This is to prevent recovery and volume duplicates on HFS+ systems.
      //

      TmpPath = (EFI_DEVICE_PATH_PROTOCOL *) GetFileInfo (
        Root,
        &gAppleBlessedSystemFolderInfoGuid,
        sizeof (EFI_DEVICE_PATH_PROTOCOL),
        &TmpPathSize
        );

      if (TmpPath != NULL) {
        if (IsDevicePathValid (TmpPath, TmpPathSize)
          && IsDevicePathEqual (TmpPath, *FilePath)) {
          DEBUG ((DEBUG_INFO, "Skipping equal alternate device path %p\n", Device));
          Status = EFI_ALREADY_STARTED;
          FreePool (*FilePath);
          *FilePath = NULL;
        }
        FreePool (TmpPath);
      }
    } else {
      FreePool (*FilePath);
      *FilePath = NULL;
      Status = EFI_NOT_FOUND;
    }
  } else {
    //
    // Ok, this one can still be FileVault 2 HFS+ recovery or just a hardcoded basic recovery.
    // Apple does add its path to so called "Alternate OS blessed file/folder", but this
    // path is not accessible from HFSPlus.efi driver. Just why???
    // Their SlingShot.efi app just bruteforces com.apple.recovery.boot directory existence,
    // and we have to copy.
    //

    Status = Root->Open (Root, &Recovery, L"\\com.apple.recovery.boot", EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR (Status)) {
      //
      // Do not do any extra checks for simplicity, as they will be done later either way.
      //
      Root->Close (Recovery);
      Status    = EFI_NOT_FOUND;
      TmpPath   = DevicePathFromHandle (Device);

      if (TmpPath != NULL) {
        *FilePath = AppendFileNameDevicePath (TmpPath, L"\\com.apple.recovery.boot");
        if (*FilePath != NULL) {
          Status = EFI_SUCCESS;
        }
      }
    } else {
      Status = EFI_NOT_FOUND;
    }
  }

  Root->Close (Root);

  return Status;
}

//
// TODO: This should be less hardcoded.
//
STATIC
VOID
SetBootEntryFlags (
  IN OUT OC_BOOT_ENTRY   *BootEntry
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathWalker;
  CONST CHAR16              *Path;

  BootEntry->IsRecovery = FALSE;
  BootEntry->IsWindows  = FALSE;

  DevicePathWalker = BootEntry->DevicePath;

  if (DevicePathWalker == NULL) {
    return;
  }

  while (!IsDevicePathEnd (DevicePathWalker)) {
    if ((DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH)
     && (DevicePathSubType (DevicePathWalker) == MEDIA_FILEPATH_DP)) {
      Path   = ((FILEPATH_DEVICE_PATH *) DevicePathWalker)->PathName;
      if (StrStr (Path, L"com.apple.recovery.boot") != NULL) {
        BootEntry->IsRecovery = TRUE;
      }
    }

    DevicePathWalker = NextDevicePathNode (DevicePathWalker);
  }
}

EFI_STATUS
OcDescribeBootEntry (
  IN     APPLE_BOOT_POLICY_PROTOCOL *BootPolicy,
  IN OUT OC_BOOT_ENTRY              *BootEntry
  )
{
  EFI_STATUS                       Status;
  CHAR16                           *BootDirectoryName;
  CHAR16                           *RecoveryBootName;
  EFI_HANDLE                       Device;
  EFI_HANDLE                       ApfsVolumeHandle;
  UINT32                           BcdSize;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  Status = BootPolicy->GetBootInfo (
    BootEntry->DevicePath,
    &BootDirectoryName,
    &Device,
    &ApfsVolumeHandle
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
    Device,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID **) &FileSystem
    );

  if (EFI_ERROR (Status)) {
    FreePool (BootDirectoryName);
    return Status;
  }

  //
  // Try to use APFS-style label or legacy HFS one.
  //
  BootEntry->Name = GetAppleDiskLabel (FileSystem, BootDirectoryName, L".contentDetails");
  if (BootEntry->Name == NULL) {
    BootEntry->Name = GetAppleDiskLabel (FileSystem, BootDirectoryName, L".disk_label.contentDetails");
  }

  //
  // With FV2 encryption on HFS+ the actual boot happens from "Recovery HD/S/L/CoreServices".
  // For some reason "Recovery HD/S/L/CoreServices/.disk_label" may not get updated immediately,
  // and will contain "Recovery HD" despite actually pointing to "Macintosh HD".
  // This also spontaneously happens with renamed APFS volumes. The workaround is to manually
  // edit the file or sometimes choose the boot volume once more in preferences.
  //
  // TODO: Bugreport this to Apple, as this is clearly their bug, which should be reproducible
  // on original hardware.
  //
  // There exists .root_uuid, which contains real partition UUID in ASCII, however, Apple
  // BootPicker only uses it for entry deduplication, and we cannot figure out the name
  // on an encrypted volume anyway.
  //

  //
  // Windows boot entry may have a custom name, so ensure IsWindows is set correctly.
  //
  DEBUG ((DEBUG_INFO, "Trying to detect Microsoft BCD\n"));
  Status = ReadFileSize (FileSystem, L"\\EFI\\Microsoft\\Boot\\BCD", &BcdSize);
  if (!EFI_ERROR (Status)) {
    BootEntry->IsWindows = TRUE;
    if (BootEntry->Name == NULL) {
      BootEntry->Name      = AllocateCopyPool(sizeof (L"BOOTCAMP Windows"), L"BOOTCAMP Windows");
    }
  }

  if (BootEntry->Name == NULL) {
    BootEntry->Name = GetVolumeLabel (FileSystem);
    if (BootEntry->Name != NULL 
      && (!StrCmp (BootEntry->Name, L"Recovery HD")
       || !StrCmp (BootEntry->Name, L"Recovery"))) {
      BootEntry->IsRecovery = TRUE;
      RecoveryBootName = GetAppleRecoveryName (FileSystem, BootDirectoryName);
      if (RecoveryBootName != NULL) {
        FreePool (BootEntry->Name);
        BootEntry->Name = RecoveryBootName;
      }
    }
  }

  if (BootEntry->Name == NULL) {
    FreePool (BootDirectoryName);
    return EFI_NOT_FOUND;
  }

  BootEntry->PathName = BootDirectoryName;

  return EFI_SUCCESS;
}

/**
  Retrieves booting relevant data from an UEFI Boot#### option.
  If BootName is NULL, a BDS-style process is assumed and inactive as well as
  non-Boot type applications are ignored.

  @param[in]  BootOption        The boot option's index.
  @param[out] BootName          On output, the boot option's description.
  @param[out] OptionalDataSize  On output, the optional data size.
  @param[out] OptionalData      On output, a pointer to the optional data.

**/
STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalGetBootOptionData (
  IN  UINT16   BootOption,
  IN  EFI_GUID *BootGud,
  OUT CHAR16   **BootName  OPTIONAL,
  OUT UINT32   *OptionalDataSize  OPTIONAL,
  OUT VOID     **OptionalData  OPTIONAL
  )
{
  EFI_STATUS               Status;
  CHAR16                   BootVarName[L_STR_LEN (L"Boot####") + 1];

  UINTN                    LoadOptionSize;
  EFI_LOAD_OPTION          *LoadOption;
  UINT8                    *LoadOptionPtr;

  UINT32                   Attributes;
  CONST CHAR16             *Description;
  UINTN                    DescriptionSize;
  UINT16                   FilePathListSize;
  EFI_DEVICE_PATH_PROTOCOL *FilePathList;

  CHAR16                   *BootOptionName;
  VOID                     *OptionalDataBuffer;

  UnicodeSPrint (BootVarName, sizeof (BootVarName), L"Boot%04x", BootOption);

  Status = GetVariable2 (
             BootVarName,
             BootGud,
             (VOID **)&LoadOption,
             &LoadOptionSize
             );
  if (EFI_ERROR (Status) || (LoadOptionSize < sizeof (*LoadOption))) {
    return NULL;
  }

  Attributes = LoadOption->Attributes;
  if ((BootName == NULL)
   && (((Attributes & LOAD_OPTION_ACTIVE) == 0)
    || ((Attributes & LOAD_OPTION_CATEGORY) != LOAD_OPTION_CATEGORY_BOOT))) {
    FreePool (LoadOption);
    return NULL;
  }

  FilePathListSize = LoadOption->FilePathListLength;

  LoadOptionPtr   = (UINT8 *)(LoadOption + 1);
  LoadOptionSize -= sizeof (*LoadOption);

  if (FilePathListSize > LoadOptionSize) {
    FreePool (LoadOption);
    return NULL;
  }

  LoadOptionSize -= FilePathListSize;

  Description     = (CHAR16 *)LoadOptionPtr;
  DescriptionSize = StrnSizeS (Description, (LoadOptionSize / sizeof (CHAR16)));
  if (DescriptionSize > LoadOptionSize) {
    FreePool (LoadOption);
    return NULL;
  }

  LoadOptionPtr  += DescriptionSize;
  LoadOptionSize -= DescriptionSize;

  FilePathList = (EFI_DEVICE_PATH_PROTOCOL *)LoadOptionPtr;
  if (!IsDevicePathValid (FilePathList, FilePathListSize)) {
    FreePool (LoadOption);
    return NULL;
  }

  LoadOptionPtr += FilePathListSize;

  BootOptionName = NULL;

  if (BootName != NULL) {
    BootOptionName = AllocateCopyPool (DescriptionSize, Description);
  }

  OptionalDataBuffer = NULL;

  if (OptionalDataSize != NULL) {
    ASSERT (OptionalData != NULL);
    if (LoadOptionSize > 0) {
      OptionalDataBuffer = AllocateCopyPool (LoadOptionSize, LoadOptionPtr);
      if (OptionalDataBuffer == NULL) {
        LoadOptionSize = 0;
      }
    }

    *OptionalDataSize = (UINT32)LoadOptionSize;
  }
  //
  // Use the allocated Load Option buffer for the Device Path.
  //
  CopyMem (LoadOption, FilePathList, FilePathListSize);
  FilePathList = (EFI_DEVICE_PATH_PROTOCOL *)LoadOption;

  if (BootName != NULL) {
    *BootName = BootOptionName;
  }

  if (OptionalData != NULL) {
    *OptionalData = OptionalDataBuffer;
  }

  return FilePathList;
}

STATIC
VOID
InternalDebugBootEnvironment (
  IN CONST UINT16             *BootOrder,
  IN EFI_GUID                 *BootGuid,
  IN UINTN                    BootOrderSize
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *UefiDevicePath;
  UINTN                     UefiDevicePathSize;
  CHAR16                    *DevicePathText;
  UINTN                     Index;
  INT32                     Predefined;

  STATIC CONST CHAR16 *AppleDebugVariables[] = {
    L"efi-boot-device-data",
    L"efi-backup-boot-device-data",
    L"efi-apple-recovery-data"
  };

  STATIC CONST UINT16  ApplePredefinedVariables[] = {
    0x80, 0x81, 0x82
  };

  for (Index = 0; Index < ARRAY_SIZE (AppleDebugVariables); ++Index) {
    Status = GetVariable2 (
               AppleDebugVariables[Index],
               &gAppleBootVariableGuid,
               (VOID **)&UefiDevicePath,
               &UefiDevicePathSize
               );
    if (!EFI_ERROR (Status) && IsDevicePathValid (UefiDevicePath, UefiDevicePathSize)) {
      DevicePathText = ConvertDevicePathToText (UefiDevicePath, FALSE, FALSE);
      if (DevicePathText != NULL) {
        DEBUG ((DEBUG_INFO, "OCB: %s = %s\n", AppleDebugVariables[Index], DevicePathText));
        FreePool (DevicePathText);
        FreePool (UefiDevicePath);
        continue;
      }

      FreePool (UefiDevicePath);
    }
    DEBUG ((DEBUG_INFO, "OCB: %s - %r\n", AppleDebugVariables[Index], Status));
  }

  DEBUG ((DEBUG_INFO, "OCB: Dumping BootOrder\n"));
  
  for (Predefined = 0; Predefined < 2; ++Predefined) {
    for (Index = 0; Index < (BootOrderSize / sizeof (*BootOrder)); ++Index) {
      UefiDevicePath = InternalGetBootOptionData (
                         BootOrder[Index],
                         BootGuid,
                         NULL,
                         NULL,
                         NULL
                         );
      if (UefiDevicePath == NULL) {
        DEBUG ((
          DEBUG_INFO,
          "OCB: %u -> Boot%04x - failed to read\n",
          (UINT32) Index,
          BootOrder[Index]
          ));
        continue;
      }

      DevicePathText = ConvertDevicePathToText (UefiDevicePath, FALSE, FALSE);
      DEBUG ((
        DEBUG_INFO,
        "OCB: %u -> Boot%04x = %s\n",
        (UINT32) Index,
        BootOrder[Index],
        DevicePathText
        ));
      if (DevicePathText != NULL) {
        FreePool (DevicePathText);
      }

      FreePool (UefiDevicePath);
    }

    //
    // Redo with predefined.
    //
    BootOrder     = &ApplePredefinedVariables[0];
    BootOrderSize = sizeof (ApplePredefinedVariables);
    DEBUG ((DEBUG_INFO, "OCB: Predefined list\n"));
  }
}

OC_BOOT_ENTRY *
InternalGetDefaultBootEntry (
  IN OUT OC_BOOT_ENTRY  *BootEntries,
  IN     UINTN          NumBootEntries,
  IN     BOOLEAN        CustomBootGuid,
  IN     EFI_HANDLE     LoadHandle  OPTIONAL
  )
{
  EFI_STATUS               Status;
  BOOLEAN                  Result;
  INTN                     CmpResult;

  UINT32                   BootNextAttributes;
  UINTN                    BootNextSize;
  BOOLEAN                  IsBootNext;

  UINT16                   *BootOrder;
  UINTN                    BootOrderSize;

  UINTN                    RootDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL *UefiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *UefiRemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *OcDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *OcRemainingDevicePath;
  UINT32                   OptionalDataSize;
  VOID                     *OptionalData;
  CHAR16                   *DevicePathText1;
  CHAR16                   *DevicePathText2;
  EFI_GUID                 *BootVariableGuid;

  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_HANDLE               DeviceHandle;

  UINT16                   BootNextOptionIndex;

  OC_BOOT_ENTRY            *BootEntry;
  UINTN                    Index;

  ASSERT (BootEntries != NULL);
  ASSERT (NumBootEntries > 0);

  IsBootNext   = FALSE;
  OptionalData = NULL;

  if (CustomBootGuid) {
    BootVariableGuid = &gOcVendorVariableGuid;
  } else {
    BootVariableGuid = &gEfiGlobalVariableGuid;
  }

  BootNextSize = sizeof (BootNextOptionIndex);
  Status = gRT->GetVariable (
                  EFI_BOOT_NEXT_VARIABLE_NAME,
                  BootVariableGuid,
                  &BootNextAttributes,
                  &BootNextSize,
                  &BootNextOptionIndex
                  );
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_INFO, "OCB: BootNext has not been found\n"));

    Status = GetVariable2 (
               EFI_BOOT_ORDER_VARIABLE_NAME,
               BootVariableGuid,
               (VOID **)&BootOrder,
               &BootOrderSize
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCB: BootOrder is unavailable - %r\n", Status));
      return NULL;
    }

    if (BootOrderSize < sizeof (*BootOrder)) {
      DEBUG ((DEBUG_WARN, "OCB: BootOrder is malformed - %x\n", (UINT32) BootOrderSize));
      FreePool (BootOrder);
      return NULL;
    }

    DEBUG_CODE_BEGIN ();
    InternalDebugBootEnvironment (BootOrder, BootVariableGuid, BootOrderSize);
    DEBUG_CODE_END ();

    UefiDevicePath = InternalGetBootOptionData (
                       BootOrder[0],
                       BootVariableGuid,
                       NULL,
                       NULL,
                       NULL
                       );
    if (UefiDevicePath == NULL) {
      FreePool (BootOrder);
      return NULL;
    }

    DevicePath = UefiDevicePath;
    Status = gBS->LocateDevicePath (
                    &gEfiSimpleFileSystemProtocolGuid,
                    &DevicePath,
                    &DeviceHandle
                    );
    if (!EFI_ERROR (Status) && (DeviceHandle == LoadHandle)) {
      DEBUG ((DEBUG_INFO, "OCB: Skipping OC bootstrap application\n"));
      //
      // Skip BOOTx64.EFI at BootOrder[0].
      //
      FreePool (UefiDevicePath);

      if (BootOrderSize < (2 * sizeof (*BootOrder))) {
        FreePool (BootOrder);
        return NULL;
      }

      UefiDevicePath = InternalGetBootOptionData (
                         BootOrder[1],
                         BootVariableGuid,
                         NULL,
                         NULL,
                         NULL
                         );
      if (UefiDevicePath == NULL) {
        FreePool (BootOrder);
        return NULL;
      }
    }

    FreePool (BootOrder);
  } else if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCB: BootNext: %x\n", BootNextOptionIndex));
    //
    // BootNext must be deleted before attempting to start the image - delete
    // it here because not attempting to boot the image implies user's choice.
    //
    gRT->SetVariable (
           EFI_BOOT_NEXT_VARIABLE_NAME,
           BootVariableGuid,
           BootNextAttributes,
           0,
           NULL
           );
    IsBootNext = TRUE;

    UefiDevicePath = InternalGetBootOptionData (
                       BootNextOptionIndex,
                       BootVariableGuid,
                       NULL,
                       &OptionalDataSize,
                       &OptionalData
                       );
    if (UefiDevicePath == NULL) {
      return NULL;
    }
  } else {
    return NULL;
  }

  UefiRemainingDevicePath = UefiDevicePath;
  Result = OcFixAppleBootDevicePath (&UefiRemainingDevicePath);

  DEBUG_CODE_BEGIN ();
  DevicePathText1 = ConvertDevicePathToText (
                      UefiDevicePath,
                      FALSE,
                      FALSE
                      );
  DevicePathText2 = ConvertDevicePathToText (
                      UefiRemainingDevicePath,
                      FALSE,
                      FALSE
                      );
  
  DEBUG ((
    DEBUG_INFO,
    "OCB: Default boot device path: %s | remainder: %s | %s\n",
    DevicePathText1,
    DevicePathText2,
    (Result ? L"success" : L"failure")
    ));

  if (DevicePathText1 != NULL) {
    FreePool (DevicePathText1);
  }

  if (DevicePathText2 != NULL) {
    FreePool (DevicePathText2);
  }
  DEBUG_CODE_END ();

  if (!Result) {
    return NULL;
  }

  RootDevicePathSize = ((UINT8 *)UefiRemainingDevicePath - (UINT8 *)UefiDevicePath);

  for (Index = 0; Index < NumBootEntries; ++Index) {
    BootEntry    = &BootEntries[Index];
    OcDevicePath = BootEntry->DevicePath;

    if ((GetDevicePathSize (OcDevicePath) - END_DEVICE_PATH_LENGTH) < RootDevicePathSize) {
      continue;
    }

    CmpResult = CompareMem (OcDevicePath, UefiDevicePath, RootDevicePathSize);
    if (CmpResult != 0) {
      continue;
    }
    //
    // FIXME: Ensure that all the entries get properly filtered against any
    // malicious sources. The drive itself should already be safe, but it is
    // unclear whether a potentially safe device path can be transformed into
    // an unsafe one.
    //
    OcRemainingDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)(
                              (UINT8 *)OcDevicePath + RootDevicePathSize
                              );
    if (!IsBootNext) {
      //
      // For non-BootNext boot, the File Paths must match for the entries to be
      // matched. Startup Disk however only stores the drive's Device Path
      // excluding the booter path, which we treat as a match as well.
      //
      if (!IsDevicePathEnd (UefiRemainingDevicePath)
       && !IsDevicePathEqual (UefiRemainingDevicePath, OcRemainingDevicePath)
        ) {
        continue;
      }

      FreePool (UefiDevicePath);
    } else {
      //
      // BootNext is allowed to override both the exact file path as well as
      // the used load options.
      // TODO: Investigate whether Apple uses OptionalData, and exploit ways.
      //
      BootEntry->LoadOptionsSize = OptionalDataSize;
      BootEntry->LoadOptions     = OptionalData;
      //
      // Only use the BootNext path when it has a file path.
      //
      if (!IsDevicePathEnd (UefiRemainingDevicePath)) {
        //
        // TODO: Investigate whether macOS adds BootNext entries that are not
        //       possibly located by bless.
        //
        FreePool (BootEntry->DevicePath);
        BootEntry->DevicePath = UefiDevicePath;
      } else {
        FreePool (UefiDevicePath);
      }
    }

    DEBUG ((DEBUG_INFO, "OCB: Matched default boot option: %s\n", BootEntry->Name));

    return BootEntry;
  }

  if (OptionalData != NULL) {
    FreePool (OptionalData);
  }

  FreePool (UefiDevicePath);

  DEBUG ((DEBUG_WARN, "OCB: Failed to match a default boot option\n"));

  return NULL;
}

VOID
OcResetBootEntry (
  IN OUT OC_BOOT_ENTRY              *BootEntry
  )
{
  if (BootEntry->DevicePath != NULL) {
    FreePool (BootEntry->DevicePath);
    BootEntry->DevicePath = NULL;
  }

  if (BootEntry->Name != NULL) {
    FreePool (BootEntry->Name);
    BootEntry->Name = NULL;
  }

  if (BootEntry->PathName != NULL) {
    FreePool (BootEntry->PathName);
    BootEntry->PathName = NULL;
  }
}

VOID
OcFreeBootEntries (
  IN OUT OC_BOOT_ENTRY              *BootEntries,
  IN     UINTN                      Count
  )
{
  UINTN  Index;

  for (Index = 0; Index < Count; ++Index) {
    OcResetBootEntry (&BootEntries[Index]);
  }

  FreePool (BootEntries);
}

STATIC
BOOLEAN
InternalDevicePathIsTrailed (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CONST FILEPATH_DEVICE_PATH *FilePath;
  CONST FILEPATH_DEVICE_PATH *FilePathWalker;
  UINTN                      PathNameSize;
  UINTN                      PathNameLength;

  ASSERT (DevicePath != NULL);
  ASSERT (IsDevicePathValid (DevicePath, 0));

  FilePathWalker = NULL;
  do {
    FilePath = FilePathWalker;
    FilePathWalker = (FILEPATH_DEVICE_PATH *)(
                       FindDevicePathNodeWithType (
                         DevicePath,
                         MEDIA_DEVICE_PATH,
                         MEDIA_FILEPATH_DP
                         )
                       );
  } while (FilePathWalker != NULL);

  if (FilePath != NULL) {
    PathNameSize   = DevicePathNodeLength (FilePath);
    PathNameSize  -= SIZE_OF_FILEPATH_DEVICE_PATH;
    PathNameLength = ((PathNameSize / sizeof (*FilePath->PathName)) - 1);
    if (PathNameLength > 0) {
      return (FilePath->PathName[PathNameLength - 1] == L'\\');
    }
  }

  return FALSE;
}

UINTN
OcFillBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  UINT32                      Policy,
  IN  EFI_HANDLE                  Handle,
  OUT OC_BOOT_ENTRY               *BootEntry,
  OUT OC_BOOT_ENTRY               *AlternateBootEntry OPTIONAL,
  IN  BOOLEAN                     IsLoadHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *RecoveryPath;
  VOID                      *Reserved;
  EFI_FILE_PROTOCOL         *RecoveryRoot;
  EFI_HANDLE                RecoveryDeviceHandle;
  UINTN                     Count;

  Count = 0;

  //
  // Do not do normal scanning on load handle.
  // We only allow recovery there.
  //
  if (!IsLoadHandle) {
    //
    // This function is supposed to never return a folder path.
    // If it does, we are not supposed to treat it as such and hence just fail
    // on image load.
    //
    Status = BootPolicy->GetBootFileEx (
      Handle,
      APPLE_BOOT_POLICY_MODE_1,
      &DevicePath
      );
  } else {
    Status = EFI_UNSUPPORTED;
  }

  BootEntry->IsFolder = FALSE;
  //
  // Detect recovery on load handle and on a partition without
  // any bootloader. Never allow alternate in this case.
  //
  if (EFI_ERROR (Status)) {
    //
    // This function is guaranteed to never return a non-folder path.
    //
    Status = GetRecoveryOsBooter (Handle, &DevicePath, TRUE);
    AlternateBootEntry = NULL;
    if (EFI_ERROR (Status)) {
      return Count;
    }

    BootEntry->IsFolder = TRUE;
  }

  BootEntry->DevicePath = DevicePath;
  SetBootEntryFlags (BootEntry);

  ++Count;

  if (AlternateBootEntry != NULL) {
    //
    // This function call is guaranteed to never return a non-folder path.
    //
    Status = BootPolicy->GetPathNameOnApfsRecovery (
      DevicePath,
      L"",
      &RecoveryPath,
      &Reserved,
      &RecoveryRoot,
      &RecoveryDeviceHandle
      );

    if (!EFI_ERROR (Status)) {
      DevicePath = FileDevicePath (RecoveryDeviceHandle, RecoveryPath);
      FreePool (RecoveryPath);
      RecoveryRoot->Close (RecoveryRoot);
    } else {
      //
      // This function is guaranteed to never return a non-folder path.
      //
      Status = GetRecoveryOsBooter (Handle, &DevicePath, FALSE);
      if (EFI_ERROR (Status)) {
        return Count;
      }
    }

    AlternateBootEntry->DevicePath = DevicePath;
    AlternateBootEntry->IsFolder   = TRUE;
    SetBootEntryFlags (AlternateBootEntry);
    ++Count;
  }
  //
  // These are asserted because of firmwares not supporting opening filepaths
  // (directories) with a trailing slash in the end.
  // More details in a852f85986c1fe23fc3a429605e3c560ea800c54 OpenCorePkg commit.
  //
  ASSERT (!InternalDevicePathIsTrailed (BootEntry->DevicePath));
  if (Count > 1) {
    ASSERT (!InternalDevicePathIsTrailed (AlternateBootEntry->DevicePath));
  }

  return Count;
}

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalGetFirstDeviceBootFilePath (
  IN APPLE_BOOT_POLICY_PROTOCOL      *BootPolicy,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DmgDevicePath,
  IN UINTN                           DmgDevicePathSize
  )
{
  EFI_DEVICE_PATH_PROTOCOL       *BootDevicePath;

  EFI_STATUS                     Status;
  INTN                           CmpResult;

  CONST EFI_DEVICE_PATH_PROTOCOL *FsDevicePath;
  UINTN                          FsDevicePathSize;

  UINTN                          NumHandles;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          Index;

  ASSERT (BootPolicy != NULL);
  ASSERT (DmgDevicePath != NULL);
  ASSERT (DmgDevicePathSize >= END_DEVICE_PATH_LENGTH);

  BootDevicePath = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (Index = 0; Index < NumHandles; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&FsDevicePath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    FsDevicePathSize = GetDevicePathSize (FsDevicePath);

    if (FsDevicePathSize < DmgDevicePathSize) {
      continue;
    }

    CmpResult = CompareMem (
                  FsDevicePath,
                  DmgDevicePath,
                  (DmgDevicePathSize - END_DEVICE_PATH_LENGTH)
                  );
    if (CmpResult != 0) {
      continue;
    }

    Status = BootPolicy->GetBootFileEx (
                           HandleBuffer[Index],
                           APPLE_BOOT_POLICY_MODE_1,
                           &BootDevicePath
                           );
    if (!EFI_ERROR (Status)) {
      break;
    }

    BootDevicePath = NULL;
  }

  FreePool (HandleBuffer);

  return BootDevicePath;
}

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalGetDiskImageBootFile (
  OUT INTERNAL_DMG_LOAD_CONTEXT   *Context,
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  UINT32                      Policy,
  IN  UINTN                       DmgFileSize,
  IN  VOID                        *ChunklistBuffer OPTIONAL,
  IN  UINT32                      ChunklistBufferSize OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL       *DevPath;

  BOOLEAN                        Result;
  OC_APPLE_CHUNKLIST_CONTEXT     ChunklistContext;

  CONST EFI_DEVICE_PATH_PROTOCOL *DmgDevicePath;
  UINTN                          DmgDevicePathSize;

  ASSERT (Context != NULL);
  ASSERT (BootPolicy != NULL);
  ASSERT (DmgFileSize > 0);

  if (ChunklistBuffer == NULL) {
    if ((Policy & OC_LOAD_REQUIRE_APPLE_SIGN) != 0) {
      return NULL;
    }
  } else if ((Policy & (OC_LOAD_VERIFY_APPLE_SIGN | OC_LOAD_REQUIRE_TRUSTED_KEY)) != 0) {
    ASSERT (ChunklistBufferSize > 0);

    Result = OcAppleChunklistInitializeContext (
                &ChunklistContext,
                ChunklistBuffer,
                ChunklistBufferSize
                );
    if (!Result) {
      return NULL;
    }

    if ((Policy & OC_LOAD_REQUIRE_TRUSTED_KEY) != 0) {
      Result = FALSE;
      //
      // FIXME: Properly abstract OcAppleKeysLib.
      //
      if ((Policy & OC_LOAD_TRUST_APPLE_V1_KEY) != 0) {
        Result = OcAppleChunklistVerifySignature (
                   &ChunklistContext,
                   (RSA_PUBLIC_KEY *)&PkDataBase[0].PublicKey
                   );
      }

      if (!Result && ((Policy & OC_LOAD_TRUST_APPLE_V2_KEY) != 0)) {
        Result = OcAppleChunklistVerifySignature (
                   &ChunklistContext,
                   (RSA_PUBLIC_KEY *)&PkDataBase[1].PublicKey
                   );
      }

      if (!Result) {
        return NULL;
      }
    }

    Result = OcAppleDiskImageVerifyData (
               Context->DmgContext,
               &ChunklistContext
               );
    if (!Result) {
      //
      // FIXME: Warn user instead of aborting when OC_LOAD_REQUIRE_TRUSTED_KEY
      //        is not set.
      //
      return NULL;
    }
  }

  Context->BlockIoHandle = OcAppleDiskImageInstallBlockIo (
                             Context->DmgContext,
                             DmgFileSize,
                             &DmgDevicePath,
                             &DmgDevicePathSize
                             );
  if (Context->BlockIoHandle == NULL) {
    return NULL;
  }

  DevPath = InternalGetFirstDeviceBootFilePath (
              BootPolicy,
              DmgDevicePath,
              DmgDevicePathSize
              );
  if (DevPath == NULL) {
    OcAppleDiskImageUninstallBlockIo (
      Context->DmgContext,
      Context->BlockIoHandle
      );
    return NULL;
  }

  return DevPath;
}

STATIC
EFI_FILE_INFO *
InternalFindFirstDmgFileName (
  IN  EFI_FILE_PROTOCOL  *Directory,
  OUT UINTN              *FileNameLen
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *FileInfo;
  BOOLEAN       NoFile;
  UINTN         ExtOffset;
  INTN          Result;

  ASSERT (Directory != NULL);

  for (
    Status = FileHandleFindFirstFile (Directory, &FileInfo), NoFile = FALSE;
    (!EFI_ERROR (Status) && !NoFile);
    Status = FileHandleFindNextFile (Directory, FileInfo, &NoFile)
    ) {
    if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
      continue;
    }

    ExtOffset = (StrLen (FileInfo->FileName) - L_STR_LEN (L".dmg"));
    Result    = StrCmp (&FileInfo->FileName[ExtOffset], L".dmg");
    if (Result == 0) {
      if (FileNameLen != NULL) {
        *FileNameLen = ExtOffset;
      }

      return FileInfo;
    }
  }

  return NULL;
}

STATIC
EFI_FILE_INFO *
InternalFindDmgChunklist (
  IN EFI_FILE_PROTOCOL  *Directory,
  IN CONST CHAR16       *DmgFileName,
  IN UINTN              DmgFileNameLen
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *FileInfo;
  BOOLEAN       NoFile;
  UINTN         NameLen;
  INTN          Result;
  UINTN         ChunklistFileNameLen;

  ChunklistFileNameLen = (DmgFileNameLen + L_STR_LEN (".chunklist"));

  for (
    Status = FileHandleFindFirstFile (Directory, &FileInfo), NoFile = FALSE;
    (!EFI_ERROR (Status) && !NoFile);
    Status = FileHandleFindNextFile (Directory, FileInfo, &NoFile)
    ) {
    if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
      continue;
    }

    NameLen = StrLen (FileInfo->FileName);

    if (NameLen != ChunklistFileNameLen) {
      continue;
    }

    Result = StrnCmp (FileInfo->FileName, DmgFileName, DmgFileNameLen);
    if (Result != 0) {
      continue;
    }

    Result = StrCmp (&FileInfo->FileName[DmgFileNameLen], L".chunklist");
    if (Result == 0) {
      return FileInfo;
    }
  }

  return NULL;
}

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalLoadDmg (
  IN OUT INTERNAL_DMG_LOAD_CONTEXT   *Context,
  IN     APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN     UINT32                      Policy
  )
{
  EFI_DEVICE_PATH_PROTOCOL *DevPath;

  EFI_STATUS               Status;
  BOOLEAN                  Result;

  EFI_FILE_PROTOCOL        *DmgDir;

  UINTN                    DmgFileNameLen;
  EFI_FILE_INFO            *DmgFileInfo;
  EFI_FILE_PROTOCOL        *DmgFile;
  UINT32                   DmgFileSize;

  EFI_FILE_INFO            *ChunklistFileInfo;
  EFI_FILE_PROTOCOL        *ChunklistFile;
  UINT32                   ChunklistFileSize;
  VOID                     *ChunklistBuffer;

  ASSERT (Context != NULL);
  ASSERT (BootPolicy != NULL);

  Status = OcOpenFileByDevicePath (
             &Context->DevicePath,
             &DmgDir,
             EFI_FILE_MODE_READ,
             EFI_FILE_DIRECTORY
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  DmgFileInfo = InternalFindFirstDmgFileName (DmgDir, &DmgFileNameLen);
  if (DmgFileInfo == NULL) {
    DmgDir->Close (DmgDir);
    return NULL;
  }

  Status = DmgDir->Open (
                     DmgDir,
                     &DmgFile,
                     DmgFileInfo->FileName,
                     EFI_FILE_MODE_READ,
                     0
                     );
  if (EFI_ERROR (Status)) {
    FreePool (DmgFileInfo);
    DmgDir->Close (DmgDir);
    return NULL;
  }

  Status = GetFileSize (DmgFile, &DmgFileSize);
  if (EFI_ERROR (Status)) {
    FreePool (DmgFileInfo);
    DmgDir->Close (DmgDir);
    DmgFile->Close (DmgFile);
    return NULL;
  }

  Context->DmgContext = AllocatePool (sizeof (*Context->DmgContext));
  if (Context->DmgContext == NULL) {
    return NULL;
  }

  Result = OcAppleDiskImageInitializeFromFile (Context->DmgContext, DmgFile);

  DmgFile->Close (DmgFile);

  if (!Result) {
    FreePool (DmgFileInfo);
    FreePool (Context->DmgContext);
    DmgDir->Close (DmgDir);
    return NULL;
  }

  ChunklistBuffer   = NULL;
  ChunklistFileSize = 0;

  ChunklistFileInfo = InternalFindDmgChunklist (
                        DmgDir,
                        DmgFileInfo->FileName,
                        DmgFileNameLen
                        );
  if (ChunklistFileInfo != NULL) {
    Status = DmgDir->Open (
                       DmgDir,
                       &ChunklistFile,
                       ChunklistFileInfo->FileName,
                       EFI_FILE_MODE_READ,
                       0
                       );
    if (!EFI_ERROR (Status)) {
      Status = GetFileSize (ChunklistFile, &ChunklistFileSize);
      if (Status == EFI_SUCCESS) {
        ChunklistBuffer = AllocatePool (ChunklistFileSize);

        if (ChunklistBuffer == NULL) {
          ChunklistFileSize = 0;
        } else {
          Status = GetFileData (ChunklistFile, 0, ChunklistFileSize, ChunklistBuffer);
          if (EFI_ERROR (Status)) {
            FreePool (ChunklistBuffer);
            ChunklistBuffer   = NULL;
            ChunklistFileSize = 0;
          }
        }
      }

      ChunklistFile->Close (ChunklistFile);
    }

    FreePool (ChunklistFileInfo);
  }

  FreePool (DmgFileInfo);

  DmgDir->Close (DmgDir);

  DevPath = InternalGetDiskImageBootFile (
              Context,
              BootPolicy,
              Policy,
              DmgFileSize,
              ChunklistBuffer,
              ChunklistFileSize
              );
  Context->DevicePath = DevPath;

  if (DevPath == NULL) {
    OcAppleDiskImageFreeFile (Context->DmgContext);
    FreePool (Context->DmgContext);
  }

  if (ChunklistBuffer != NULL) {
    FreePool (ChunklistBuffer);
  }

  return DevPath;
}

STATIC
VOID
InternalUnloadDmg (
  IN INTERNAL_DMG_LOAD_CONTEXT  *DmgLoadContext
  )
{
  if (DmgLoadContext->DevicePath != NULL) {
    FreePool (DmgLoadContext->DevicePath);
    OcAppleDiskImageUninstallBlockIo (
      DmgLoadContext->DmgContext,
      DmgLoadContext->BlockIoHandle
      );
    OcAppleDiskImageFreeContext (DmgLoadContext->DmgContext);
    FreePool (DmgLoadContext->DmgContext);
    DmgLoadContext->DevicePath = NULL;
  }
}

EFI_STATUS
OcScanForBootEntries (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  UINT32                      Policy,
  OUT OC_BOOT_ENTRY               **BootEntries,
  OUT UINTN                       *Count,
  OUT UINTN                       *AllocCount OPTIONAL,
  IN  EFI_HANDLE                  LoadHandle  OPTIONAL,
  IN  BOOLEAN                     Describe
  )
{
  EFI_STATUS                       Status;
  UINTN                            NoHandles;
  EFI_HANDLE                       *Handles;
  UINTN                            Index;
  OC_BOOT_ENTRY                    *Entries;
  UINTN                            EntryIndex;
  CHAR16                           *DevicePath;
  CHAR16                           *VolumeLabel;
  UINTN                            EntryCount;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFs;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NoHandles,
                  &Handles
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "OCB: Found %u potentially bootable filesystems\n", (UINT32) NoHandles));

  if (NoHandles == 0) {
    FreePool (Handles);
    return EFI_NOT_FOUND;
  }

  Entries = AllocateZeroPool (NoHandles * 2 * sizeof (OC_BOOT_ENTRY));
  if (Entries == NULL) {
    FreePool (Handles);
    return EFI_OUT_OF_RESOURCES;
  }

  EntryIndex = 0;

  for (Index = 0; Index < NoHandles; ++Index) {
    EntryCount = OcFillBootEntry (
      BootPolicy,
      Policy,
      Handles[Index],
      &Entries[EntryIndex],
      &Entries[EntryIndex+1],
      LoadHandle == Handles[Index]
      );

    DEBUG_CODE_BEGIN ();
    Status = gBS->HandleProtocol (
      Handles[Index],
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID **) &SimpleFs
      );
    if (!EFI_ERROR (Status)) {
      VolumeLabel = GetVolumeLabel (SimpleFs);
    } else {
      VolumeLabel = NULL;
    }
    DEBUG ((
      DEBUG_INFO,
      "OCB: Filesystem %u (%p) named %s (%r) has %u entries\n",
      (UINT32) Index,
      Handles[Index],
      VolumeLabel != NULL ? VolumeLabel : L"<Null>",
      Status,
      (UINT32) EntryCount
      ));
    if (VolumeLabel != NULL) {
      FreePool (VolumeLabel);
    }
    DEBUG_CODE_END ();

    EntryIndex += EntryCount;
  }

  FreePool (Handles);

  if (Describe) {
    DEBUG ((DEBUG_INFO, "Scanning got %u entries\n", (UINT32) EntryIndex));

    for (Index = 0; Index < EntryIndex; ++Index) {
      Status = OcDescribeBootEntry (BootPolicy, &Entries[Index]);
      if (EFI_ERROR (Status)) {
        break;
      }

      DEBUG ((
        DEBUG_INFO,
        "Entry %u is %s at %s (W:%d|R:%d|F:%d)\n",
        (UINT32) Index,
        Entries[Index].Name,
        Entries[Index].PathName,
        Entries[Index].IsWindows,
        Entries[Index].IsRecovery,
        Entries[Index].IsFolder
        ));

      DevicePath = ConvertDevicePathToText(Entries[Index].DevicePath, FALSE, FALSE);
      if (DevicePath != NULL) {
        DEBUG ((
          DEBUG_INFO,
          "Entry %u is %s at dp %s\n",
          (UINT32) Index,
          Entries[Index].Name,
          DevicePath
          ));
        FreePool (DevicePath);
      }
    }

    if (EFI_ERROR (Status)) {
      OcFreeBootEntries (Entries, EntryIndex);
      return Status;
    }
  }

  *BootEntries = Entries;
  *Count       = EntryIndex;

  if (AllocCount != NULL) {
    *AllocCount = NoHandles * 2;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
OcShowSimpleBootMenu (
  IN OC_BOOT_ENTRY                *BootEntries,
  IN UINTN                        Count,
  IN UINTN                        DefaultEntry,
  IN UINTN                        TimeOutSeconds,
  OUT OC_BOOT_ENTRY               **ChosenBootEntry
  )
{
  UINTN   Index;
  INTN    KeyIndex;
  CHAR16  Code[2];

  Code[1] = '\0';

  while (TRUE) {
    gST->ConOut->ClearScreen (gST->ConOut);
    gST->ConOut->OutputString (gST->ConOut, L"OpenCore Boot Menu\r\n\r\n");

    for (Index = 0; Index < MIN (Count, OC_INPUT_MAX); ++Index) {
      Code[0] = OC_INPUT_STR[Index];
      gST->ConOut->OutputString (gST->ConOut, DefaultEntry == Index && TimeOutSeconds > 0 ? L"* " : L"  ");
      gST->ConOut->OutputString (gST->ConOut, Code);
      gST->ConOut->OutputString (gST->ConOut, L". ");
      gST->ConOut->OutputString (gST->ConOut, BootEntries[Index].Name);
      if (BootEntries[Index].IsFolder) {
        gST->ConOut->OutputString (gST->ConOut, L" (dmg)");
      }
      gST->ConOut->OutputString (gST->ConOut, L"\r\n");
    }

    if (Index < Count) {
      gST->ConOut->OutputString (gST->ConOut, L"WARN: Some entries were skipped!\r\n");
    }

    gST->ConOut->OutputString (gST->ConOut, L"\r\nChoose boot entry: ");

    while (TRUE) {
      KeyIndex = WaitForKeyIndex (TimeOutSeconds);
      if (KeyIndex == OC_INPUT_TIMEOUT) {
        *ChosenBootEntry = &BootEntries[DefaultEntry];
        gST->ConOut->OutputString (gST->ConOut, L"Timeout\r\n");
        return EFI_SUCCESS;
      } else if (KeyIndex == OC_INPUT_ABORTED) {
        gST->ConOut->OutputString (gST->ConOut, L"Aborted\r\n");
        return EFI_ABORTED;
      } else if (KeyIndex != OC_INPUT_INVALID && (UINTN)KeyIndex < Count) {
	    ASSERT (KeyIndex >= 0);
        *ChosenBootEntry = &BootEntries[KeyIndex];
        Code[0] = OC_INPUT_STR[KeyIndex];
        gST->ConOut->OutputString (gST->ConOut, Code);
        gST->ConOut->OutputString (gST->ConOut, L"\r\n");
        return EFI_SUCCESS;
      }

      if (TimeOutSeconds > 0) {
        TimeOutSeconds = 0;
        break;
      }
    }
  }

  ASSERT (FALSE);
}

STATIC
EFI_STATUS
InternalLoadBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  OC_BOOT_ENTRY               *BootEntry,
  IN  UINT32                      Policy,
  IN  EFI_HANDLE                  ParentHandle,
  OUT EFI_HANDLE                  *EntryHandle,
  OUT INTERNAL_DMG_LOAD_CONTEXT   *DmgLoadContext
  )
{
  EFI_STATUS                 Status;
  EFI_STATUS                 OptionalStatus;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  CHAR16                     *UnicodeDevicePath;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  //
  // TODO: support Apple loaded image, policy, and dmg boot.
  //

  ZeroMem (DmgLoadContext, sizeof (*DmgLoadContext));

  if (BootEntry->IsFolder) {
    if ((Policy & OC_LOAD_ALLOW_DMG_BOOT) == 0) {
      return EFI_SECURITY_VIOLATION;
    }

    DmgLoadContext->DevicePath = BootEntry->DevicePath;
    DevicePath = InternalLoadDmg (
                   DmgLoadContext,
                   BootPolicy,
                   Policy
                   );
    if (DevicePath == NULL) {
      return EFI_UNSUPPORTED;
    }

    DEBUG_CODE_BEGIN ();
    UnicodeDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
    if (UnicodeDevicePath != NULL) {
      DEBUG ((
        DEBUG_INFO,
        "Dmg boot %s to dp %s\n",
        BootEntry->Name,
        UnicodeDevicePath
        ));
      FreePool (UnicodeDevicePath);
    }
    DEBUG_CODE_END ();

  } else {
    DevicePath = BootEntry->DevicePath;
  }

  Status = gBS->LoadImage (FALSE, ParentHandle, DevicePath, NULL, 0, EntryHandle);

  if (!EFI_ERROR (Status)) {
    OptionalStatus = gBS->HandleProtocol (
                            ParentHandle,
                            &gEfiLoadedImageProtocolGuid,
                            (VOID **)&LoadedImage
                            );
    if (!EFI_ERROR (OptionalStatus)) {
      LoadedImage->LoadOptionsSize = BootEntry->LoadOptionsSize;
      LoadedImage->LoadOptions     = BootEntry->LoadOptions;
    }
  } else {
    InternalUnloadDmg (DmgLoadContext);
  }

  return Status;
}

EFI_STATUS
OcLoadBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  OC_BOOT_ENTRY               *BootEntry,
  IN  UINT32                      Policy,
  IN  EFI_HANDLE                  ParentHandle,
  OUT EFI_HANDLE                  *EntryHandle
  )
{
  //
  // FIXME: Think of something when starting dmg fails.
  //
  INTERNAL_DMG_LOAD_CONTEXT  DmgLoadContext;

  return InternalLoadBootEntry (
    BootPolicy,
    BootEntry,
    Policy,
    ParentHandle,
    EntryHandle,
    &DmgLoadContext
    );
}

EFI_STATUS
OcRunSimpleBootPicker (
  IN  UINT32           LookupPolicy,
  IN  UINT32           BootPolicy,
  IN  UINT32           TimeoutSeconds,
  IN  OC_IMAGE_START   StartImage,
  IN  BOOLEAN          ShowPicker,
  IN  BOOLEAN          CustomBootGuid,
  IN  EFI_HANDLE       LoadHandle  OPTIONAL
  )
{
  EFI_STATUS                  Status;
  APPLE_BOOT_POLICY_PROTOCOL  *AppleBootPolicy;
  OC_BOOT_ENTRY               *Chosen;
  OC_BOOT_ENTRY               *Entries;
  OC_BOOT_ENTRY               *Entry;
  UINTN                       EntryCount;
  EFI_HANDLE                  BooterHandle;
  UINT32                      DefaultEntry;
  INTERNAL_DMG_LOAD_CONTEXT   DmgLoadContext;

  AppleBootPolicy = OcAppleBootPolicyInstallProtocol (FALSE);
  if (AppleBootPolicy == NULL) {
    DEBUG ((DEBUG_ERROR, "AppleBootPolicy locate failure\n"));
    return EFI_NOT_FOUND;
  }

  while (TRUE) {
    DEBUG ((DEBUG_INFO, "Performing OcScanForBootEntries...\n"));

    Status = OcScanForBootEntries (
      AppleBootPolicy,
      LookupPolicy,
      &Entries,
      &EntryCount,
      NULL,
      LoadHandle,
      TRUE
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OcScanForBootEntries failure - %r\n", Status));
      return Status;
    }

    DEBUG ((DEBUG_INFO, "Performing OcShowSimpleBootMenu...\n"));

    DefaultEntry = 0;
    Entry = InternalGetDefaultBootEntry (Entries, EntryCount, CustomBootGuid, LoadHandle);
    if (Entry != NULL) {
      DefaultEntry = (UINT32)(Entry - Entries);
    }

    if (ShowPicker) {
      Status = OcShowSimpleBootMenu (
        Entries,
        EntryCount,
        DefaultEntry,
        TimeoutSeconds,
        &Chosen
        );
    } else {
      Chosen = &Entries[DefaultEntry];
      Status = EFI_SUCCESS;
    }

    if (EFI_ERROR (Status) && Status != EFI_ABORTED) {
      DEBUG ((DEBUG_ERROR, "OcShowSimpleBootMenu failed - %r\n", Status));
      OcFreeBootEntries (Entries, EntryCount);
      return Status;
    }

    TimeoutSeconds = 0;

    if (!EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "Should boot from %s (W:%d|R:%d|F:%d)\n",
        Chosen->Name,
        Chosen->IsWindows,
        Chosen->IsRecovery,
        Chosen->IsFolder
        ));
    }

    if (!EFI_ERROR (Status)) {
      Status = InternalLoadBootEntry (
        AppleBootPolicy,
        Chosen,
        BootPolicy,
        gImageHandle,
        &BooterHandle,
        &DmgLoadContext
        );
      if (!EFI_ERROR (Status)) {
        Status = StartImage (Chosen, BooterHandle, NULL, NULL);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "StartImage failed - %r\n", Status));
          //
          // Unload dmg if any.
          //
          InternalUnloadDmg (&DmgLoadContext);
        }
      } else {
        DEBUG ((DEBUG_ERROR, "LoadImage failed - %r\n", Status));
      }

      gBS->Stall (5000000);
    }

    OcFreeBootEntries (Entries, EntryCount);
  }
}
