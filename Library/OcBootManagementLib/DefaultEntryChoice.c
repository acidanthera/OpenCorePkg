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

#include <Uefi.h>
#include <Library/OcMainLib.h>
#include "BootManagementInternal.h"

#include <Guid/AppleFile.h>
#include <Guid/AppleVariable.h>
#include <Guid/GlobalVariable.h>
#include <Guid/OcVariable.h>

#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OcFirmwareRuntime.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

///
/// Template for an OpenCore custom boot entry DevicePath node.
///
STATIC CONST OC_CUSTOM_BOOT_DEVICE_PATH_DECL mOcCustomBootDevPathTemplate = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      { sizeof (VENDOR_DEVICE_PATH), 0 }
    },
    OC_CUSTOM_BOOT_DEVICE_PATH_GUID
  },
  {
    MEDIA_DEVICE_PATH,
    MEDIA_FILEPATH_DP,
    { SIZE_OF_FILEPATH_DEVICE_PATH, 0 }
  }
};

CONST OC_CUSTOM_BOOT_DEVICE_PATH *
InternalGetOcCustomDevPath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  UINTN                            DevicePathSize;
  INTN                             CmpResult;
  CONST OC_CUSTOM_BOOT_DEVICE_PATH *CustomDevPath;

  DevicePathSize = GetDevicePathSize (DevicePath);
  if (DevicePathSize < SIZE_OF_OC_CUSTOM_BOOT_DEVICE_PATH) {
    return NULL;
  }

  CmpResult = CompareMem (
    DevicePath,
    &mOcCustomBootDevPathTemplate.Header,
    sizeof (mOcCustomBootDevPathTemplate.Header)
    );
  if (CmpResult != 0) {
    return NULL;
  }

  CustomDevPath = (CONST OC_CUSTOM_BOOT_DEVICE_PATH *) DevicePath;
  if (CustomDevPath->EntryName.Header.Type != MEDIA_DEVICE_PATH
   || CustomDevPath->EntryName.Header.SubType != MEDIA_FILEPATH_DP) {
    return NULL;
  }

  return CustomDevPath;
}

EFI_LOAD_OPTION *
InternalGetBootOptionData (
  OUT UINTN           *OptionSize,
  IN  UINT16          BootOption,
  IN  CONST EFI_GUID  *BootGuid
  )
{
  EFI_STATUS      Status;
  CHAR16          BootVarName[L_STR_LEN (L"Boot####") + 1];
  UINTN           LoadOptionSize;
  EFI_LOAD_OPTION *LoadOption;

  if (CompareGuid (BootGuid, &gOcVendorVariableGuid)) {
    UnicodeSPrint (
      BootVarName,
      sizeof (BootVarName),
      OC_VENDOR_BOOT_VARIABLE_PREFIX L"%04x",
      BootOption
      );
  } else {
    UnicodeSPrint (BootVarName, sizeof (BootVarName), L"Boot%04x", BootOption);   
  }

  Status = GetVariable2 (
    BootVarName,
    BootGuid,
    (VOID **) &LoadOption,
    &LoadOptionSize
    );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  if (LoadOptionSize < sizeof (*LoadOption)) {
    FreePool (LoadOption);
    return NULL;
  }

  *OptionSize = LoadOptionSize;
  return LoadOption;
}

EFI_DEVICE_PATH_PROTOCOL *
InternalGetBootOptionPath (
  IN EFI_LOAD_OPTION  *LoadOption,
  IN UINTN            LoadOptionSize
  )
{
  UINT8                    *LoadOptionPtr;

  CHAR16                   *Description;
  UINTN                    DescriptionSize;
  UINT16                   FilePathListSize;
  EFI_DEVICE_PATH_PROTOCOL *FilePathList;

  FilePathListSize = LoadOption->FilePathListLength;

  LoadOptionPtr   = (UINT8 *) (LoadOption + 1);
  LoadOptionSize -= sizeof (*LoadOption);

  if (FilePathListSize > LoadOptionSize) {
    return NULL;
  }

  LoadOptionSize -= FilePathListSize;

  STATIC_ASSERT (
    sizeof (*LoadOption) % OC_ALIGNOF (CHAR16) == 0,
    "The following accesses may be unaligned."
    );

  Description     = (CHAR16 *) (VOID *) LoadOptionPtr;
  DescriptionSize = StrnSizeS (Description, (LoadOptionSize / sizeof (CHAR16)));
  if (DescriptionSize > LoadOptionSize) {
    return NULL;
  }

  LoadOptionPtr += DescriptionSize;

  FilePathList = (EFI_DEVICE_PATH_PROTOCOL *) LoadOptionPtr;
  if (!IsDevicePathValid (FilePathList, FilePathListSize)) {
    return NULL;
  }

  return FilePathList;
}

VOID
InternalDebugBootEnvironment (
  IN CONST UINT16             *BootOrder,
  IN EFI_GUID                 *BootGuid,
  IN UINTN                    BootOrderCount
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *UefiDevicePath;
  UINTN                     UefiDevicePathSize;
  CHAR16                    *DevicePathText;
  UINTN                     Index;
  INT32                     Predefined;
  EFI_LOAD_OPTION           *LoadOption;
  UINTN                     LoadOptionSize;

  STATIC CONST CHAR16 *AppleDebugVariables[] = {
    L"efi-boot-device-data",
    L"efi-boot-next-data",
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
    for (Index = 0; Index < BootOrderCount; ++Index) {
      LoadOption = InternalGetBootOptionData (
        &LoadOptionSize,
        BootOrder[Index],
        BootGuid
        );
      if (LoadOption == NULL) {
        continue;
      }

      UefiDevicePath = InternalGetBootOptionPath (LoadOption, LoadOptionSize);
      if (UefiDevicePath == NULL) {
        DEBUG ((
          DEBUG_INFO,
          "OCB: %u -> Boot%04x - failed to read\n",
          (UINT32) Index,
          BootOrder[Index]
          ));
        FreePool (LoadOption);
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

      FreePool (LoadOption);
    }

    //
    // Redo with predefined.
    //
    if (Predefined == 0) {
      BootOrder      = &ApplePredefinedVariables[0];
      BootOrderCount = ARRAY_SIZE (ApplePredefinedVariables);
      DEBUG ((DEBUG_INFO, "OCB: Parsing predefined list...\n"));
    }
  }
}

STATIC
BOOLEAN
InternalMatchBootEntryByDevicePath (
  IN OUT OC_BOOT_ENTRY             *BootEntry,
  IN     EFI_DEVICE_PATH_PROTOCOL  *UefiDevicePath,
  IN     EFI_DEVICE_PATH_PROTOCOL  *UefiRemainingDevicePath,
  IN     UINTN                     UefiDevicePathSize,
  IN     BOOLEAN                   IsBootNext
  )
{
  INTN                     CmpResult;

  UINTN                    RootDevicePathSize;

  EFI_DEVICE_PATH_PROTOCOL *OcDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *OcRemainingDevicePath;

  RootDevicePathSize = ((UINT8 *)UefiRemainingDevicePath - (UINT8 *)UefiDevicePath);

  if (BootEntry->DevicePath == NULL || (BootEntry->Type & OC_BOOT_SYSTEM) != 0) {
    return FALSE;
  }

  OcDevicePath = BootEntry->DevicePath;

  if ((GetDevicePathSize (OcDevicePath) - END_DEVICE_PATH_LENGTH) < RootDevicePathSize) {
    return FALSE;
  }

  CmpResult = CompareMem (OcDevicePath, UefiDevicePath, RootDevicePathSize);
  if (CmpResult != 0) {
    return FALSE;
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
      return FALSE;
    }
  } else {
    //
    // Only use the BootNext path when it has a file path.
    //
    if (!IsDevicePathEnd (UefiRemainingDevicePath)) {
      //
      // TODO: Investigate whether macOS adds BootNext entries that are not
      //       possibly located by bless.
      //
      FreePool (BootEntry->DevicePath);
      BootEntry->DevicePath = AllocateCopyPool (
        UefiDevicePathSize,
        UefiDevicePath
        );
    }
  }

  return TRUE;
}

STATIC
BOOLEAN
InternalMatchCustomBootEntryByDevicePath (
  IN OUT OC_BOOT_ENTRY                     *BootEntry,
  IN     CONST OC_CUSTOM_BOOT_DEVICE_PATH  *DevicePath
  )
{
  INTN CmpResult;

  if (!BootEntry->IsCustom) {
    return FALSE;
  }

  CmpResult = StrCmp (BootEntry->Name, DevicePath->EntryName.PathName);
  if (CmpResult != 0) {
    return FALSE;
  }

  return TRUE;
}

STATIC
VOID
InternalClearNextVariables (
  IN  EFI_GUID  *BootVariableGuid,
  IN  BOOLEAN   ClearApplePayload
  )
{
  CHAR16  VariableName[32];
  CHAR16  *BootNextName;
  UINTN   Index;

  if (CompareGuid (BootVariableGuid, &gOcVendorVariableGuid)) {
    BootNextName = OC_VENDOR_BOOT_NEXT_VARIABLE_NAME;
  } else {
    BootNextName = EFI_BOOT_NEXT_VARIABLE_NAME;
  }

  //
  // Next variable data specified by UEFI spec.
  // For now we do not bother dropping the variable it points to.
  //
  gRT->SetVariable (
    BootNextName,
    BootVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
    0,
    NULL
    );

  //
  // Next variable string (in xml format) specified by Apple macOS.
  //
  gRT->SetVariable (
    L"efi-boot-next",
    &gAppleBootVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
    0,
    NULL
    );

  //
  // Next variable blob (in DevicePath format) specified by Apple macOS.
  //
  gRT->SetVariable (
    L"efi-boot-next-data",
    &gAppleBootVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
    0,
    NULL
    );

  if (ClearApplePayload) {
    for (Index = 0; Index <= 3; ++Index) {
      UnicodeSPrint (
        VariableName,
        sizeof (VariableName),
        L"efi-apple-payload%u%a",
        (UINT32) Index,
        "-data"
        );

      gRT->SetVariable (
        VariableName,
        &gAppleBootVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
        0,
        NULL
        );

      UnicodeSPrint (
        VariableName,
        sizeof (VariableName),
        L"efi-apple-payload%u%a",
        (UINT32) Index,
        ""
        );

      gRT->SetVariable (
        VariableName,
        &gAppleBootVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
        0,
        NULL
        );
    }
  }
}

STATIC
BOOLEAN
InternalHasFirmwareUpdateAsNext (
  IN EFI_GUID  *BootVariableGuid
  )
{
  EFI_STATUS                       Status;
  UINT32                           VariableAttributes;
  UINT16                           BootNext;
  CHAR16                           *BootNextName;
  UINTN                            VariableSize;
  OC_BOOT_ENTRY_TYPE               EntryType;
  EFI_DEVICE_PATH_PROTOCOL         *UefiDevicePath;
  EFI_LOAD_OPTION                  *LoadOption;
  UINTN                            LoadOptionSize;

  if (CompareGuid (BootVariableGuid, &gOcVendorVariableGuid)) {
    BootNextName = OC_VENDOR_BOOT_NEXT_VARIABLE_NAME;
  } else {
    BootNextName = EFI_BOOT_NEXT_VARIABLE_NAME;
  }

  VariableSize = sizeof (BootNext);
  Status = gRT->GetVariable (
    BootNextName,
    BootVariableGuid,
    &VariableAttributes,
    &VariableSize,
    &BootNext
    );
  if (EFI_ERROR (Status) || VariableSize != sizeof (BootNext)) {
    return FALSE;
  }

  LoadOption = InternalGetBootOptionData (
    &LoadOptionSize,
    BootNext,
    BootVariableGuid
    );
  if (LoadOption == NULL) {
    return FALSE;
  }

  UefiDevicePath = InternalGetBootOptionPath (LoadOption, LoadOptionSize);
  if (UefiDevicePath == NULL) {
    FreePool (LoadOption);
    return FALSE;
  }

  EntryType = OcGetBootDevicePathType (UefiDevicePath, NULL, NULL);
  DEBUG ((DEBUG_INFO, "OCB: Found BootNext %04x of type %u\n", BootNext, EntryType));
  FreePool (LoadOption);

  return EntryType == OC_BOOT_APPLE_FW_UPDATE;
}

BOOLEAN
InternalIsAppleLegacyLoadApp (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEV_PATH_PTR FwVolDevPath;

  ASSERT (DevicePath != NULL);

  if (DevicePath->Type == HARDWARE_DEVICE_PATH
   && DevicePath->SubType == HW_MEMMAP_DP) {
    FwVolDevPath.DevPath = NextDevicePathNode (DevicePath);
    if (FwVolDevPath.DevPath->Type == MEDIA_DEVICE_PATH
     && FwVolDevPath.DevPath->SubType == MEDIA_PIWG_FW_FILE_DP) {
      return CompareGuid (
               &FwVolDevPath.FirmwareFile->FvFileName,
               &gAppleLegacyLoadAppFileGuid
               );
    }
  }

  return FALSE;
}

UINT16 *
OcGetBootOrder (
  IN  EFI_GUID  *BootVariableGuid,
  IN  BOOLEAN   WithBootNext,
  OUT UINTN     *BootOrderCount,
  OUT BOOLEAN   *Deduplicated  OPTIONAL,
  OUT BOOLEAN   *HasBootNext   OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32      VariableAttributes;
  UINT16      BootNext;
  CHAR16      *BootOrderName;
  CHAR16      *BootNextName;
  UINT16      *BootOrder;
  UINTN       VariableSize;
  UINTN       Index;
  UINTN       Index2;
  BOOLEAN     BootOrderChanged;

  *BootOrderCount = 0;

  if (Deduplicated != NULL) {
    *Deduplicated = FALSE;
  }

  if (HasBootNext != NULL) {
    *HasBootNext = FALSE;
  }

  if (CompareGuid (BootVariableGuid, &gOcVendorVariableGuid)) {
    BootOrderName = OC_VENDOR_BOOT_ORDER_VARIABLE_NAME;
    BootNextName = OC_VENDOR_BOOT_NEXT_VARIABLE_NAME;
  } else {
    BootOrderName = EFI_BOOT_ORDER_VARIABLE_NAME;
    BootNextName = EFI_BOOT_NEXT_VARIABLE_NAME;
  }

  //
  // Precede variable with boot next.
  //
  if (WithBootNext) {
    VariableSize = sizeof (BootNext);
    Status = gRT->GetVariable (
      BootNextName,
      BootVariableGuid,
      &VariableAttributes,
      &VariableSize,
      &BootNext
      );
    if (!EFI_ERROR (Status) && VariableSize == sizeof (BootNext)) {
      if (HasBootNext != NULL) {
        *HasBootNext = TRUE;
      }
    } else {
      WithBootNext = FALSE;
    }
  }

  VariableSize = 0;
  Status = gRT->GetVariable (
    BootOrderName,
    BootVariableGuid,
    &VariableAttributes,
    &VariableSize,
    NULL
    );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    BootOrder = AllocatePool ((UINTN) WithBootNext * sizeof (BootNext) + VariableSize);
    if (BootOrder == NULL) {
      return NULL;
    }

    Status = gRT->GetVariable (
      BootOrderName,
      BootVariableGuid,
      &VariableAttributes,
      &VariableSize,
      BootOrder + (UINTN) WithBootNext
      );
    if (EFI_ERROR (Status)
      || VariableSize < sizeof (*BootOrder)
      || VariableSize % sizeof (*BootOrder) != 0) {
      FreePool (BootOrder);
      Status = EFI_UNSUPPORTED;
    }
  } else if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status)) {
    if (WithBootNext) {
      BootOrder = AllocateCopyPool (sizeof (BootNext), &BootNext);
      if (BootOrder != NULL) {
        *BootOrderCount = 1;
        return BootOrder;
      }
    }

    return NULL;
  }

  if (WithBootNext) {
    BootOrder[0] = BootNext;
    VariableSize += sizeof (*BootOrder);
  }

  BootOrderChanged = FALSE;

  for (Index = 1; Index < VariableSize / sizeof (BootOrder[0]); ++Index) {
    for (Index2 = 0; Index2 < Index; ++Index2) {
      if (BootOrder[Index] == BootOrder[Index2]) {
        //
        // Found duplicate.
        //
        BootOrderChanged = TRUE;
        CopyMem (
          &BootOrder[Index],
          &BootOrder[Index + 1],
          VariableSize - sizeof (BootOrder[0]) * (Index + 1)
          );
        VariableSize -= sizeof (BootOrder[0]);
        --Index;
        break;
      }
    }
  }

  *BootOrderCount = VariableSize / sizeof (*BootOrder);
  if (Deduplicated != NULL) {
    *Deduplicated = BootOrderChanged;
  }

  return BootOrder;
}

UINT16 *
InternalGetBootOrderForBooting (
  IN  EFI_GUID  *BootVariableGuid,
  IN  BOOLEAN   BlacklistAppleUpdate,
  OUT UINTN     *BootOrderCount
  )
{
  UINT16                           *BootOrder;
  BOOLEAN                          HasFwBootNext;
  BOOLEAN                          HasBootNext;

  //
  // Precede variable with boot next unless we were forced to ignore it.
  //
  if (BlacklistAppleUpdate) {
    HasFwBootNext = InternalHasFirmwareUpdateAsNext (BootVariableGuid);
  } else {
    HasFwBootNext = FALSE;
  }

  BootOrder = OcGetBootOrder (
    BootVariableGuid,
    HasFwBootNext == FALSE,
    BootOrderCount,
    NULL,
    &HasBootNext
    );
  if (BootOrder == NULL) {
    DEBUG ((DEBUG_INFO, "OCB: BootOrder/BootNext are not present or unsupported\n"));
    return NULL;
  }

  DEBUG_CODE_BEGIN ();
  DEBUG ((
    DEBUG_INFO,
    "OCB: Found %u BootOrder entries with BootNext %a\n",
    (UINT32) *BootOrderCount,
    HasBootNext ? "included" : "excluded"
    ));
  InternalDebugBootEnvironment (BootOrder, BootVariableGuid, *BootOrderCount);
  DEBUG_CODE_END ();

  if (HasFwBootNext || HasBootNext) {
    InternalClearNextVariables (BootVariableGuid, HasFwBootNext);
  }

  return BootOrder;
}

EFI_STATUS
OcSetDefaultBootEntry (
  IN OC_PICKER_CONTEXT  *Context,
  IN OC_BOOT_ENTRY      *Entry
  )
{
  EFI_STATUS       Status;
  EFI_DEVICE_PATH  *BootOptionDevicePath;
  EFI_DEVICE_PATH  *BootOptionRemainingDevicePath;
  EFI_HANDLE       DeviceHandle;
  BOOLEAN          MatchedEntry;
  EFI_GUID         *BootVariableGuid;
  CHAR16           *BootOrderName;
  CHAR16           *BootVariableName;
  UINT16           *BootOrder;
  UINT16           *NewBootOrder;
  UINT16           BootTmp;
  UINTN            BootOrderCount;
  UINTN            BootChosenIndex;
  UINTN            Index;
  UINTN            DevicePathSize;
  UINTN            LoadOptionSize;
  UINTN            LoadOptionNameSize;
  EFI_LOAD_OPTION  *LoadOption;

  CONST OC_CUSTOM_BOOT_DEVICE_PATH *CustomDevPath;
  OC_CUSTOM_BOOT_DEVICE_PATH       *DestCustomDevPath;
  EFI_DEVICE_PATH_PROTOCOL         *DestCustomEndNode;

  //
  // Do not allow when prohibited.
  //
  if (!Context->AllowSetDefault) {
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Ignore entries without device paths.
  //
  if (Entry->DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Context->CustomBootGuid) {
    BootVariableGuid = &gOcVendorVariableGuid;
    BootOrderName = OC_VENDOR_BOOT_ORDER_VARIABLE_NAME;
    BootVariableName = OC_VENDOR_BOOT_VARIABLE_PREFIX L"0080";
  } else {
    BootVariableGuid = &gEfiGlobalVariableGuid;
    BootOrderName = EFI_BOOT_ORDER_VARIABLE_NAME;
    BootVariableName = L"Boot0080";
  }

  BootOrder = OcGetBootOrder (
    BootVariableGuid,
    FALSE,
    &BootOrderCount,
    NULL,
    NULL
    );

  MatchedEntry    = FALSE;
  BootChosenIndex = BootOrderCount;
  for (Index = 0; Index < BootOrderCount; ++Index) {
    if (BootOrder[Index] == 0x80) {
      BootChosenIndex = Index;
    }

    if (MatchedEntry) {
      if (BootChosenIndex != BootOrderCount) {
        break;
      }
      continue;
    }

    LoadOption = InternalGetBootOptionData (
      &LoadOptionSize,
      BootOrder[Index],
      BootVariableGuid
      );
    if (LoadOption == NULL) {
      continue;
    }

    BootOptionDevicePath = InternalGetBootOptionPath (
      LoadOption,
      LoadOptionSize
      );
    if (BootOptionDevicePath == NULL) {
      FreePool (LoadOption);
      continue;
    }

    BootOptionRemainingDevicePath = BootOptionDevicePath;
    Status = gBS->LocateDevicePath (
      &gEfiSimpleFileSystemProtocolGuid,
      &BootOptionRemainingDevicePath,
      &DeviceHandle
      );

    if (!EFI_ERROR (Status)) {
      MatchedEntry = InternalMatchBootEntryByDevicePath (
        Entry,
        BootOptionDevicePath,
        BootOptionRemainingDevicePath,
        LoadOption->FilePathListLength,
        FALSE
        );
    } else {
      CustomDevPath = InternalGetOcCustomDevPath (BootOptionDevicePath);
      if (CustomDevPath != NULL) {
        MatchedEntry = InternalMatchCustomBootEntryByDevicePath (
          Entry,
          CustomDevPath
          );
      }
    }

    FreePool (LoadOption);
  }

  if (!MatchedEntry) {
    //
    // Write to Boot0080
    //
    LoadOptionNameSize = StrSize (Entry->Name);
    
    if (!Entry->IsCustom) {
      DevicePathSize = GetDevicePathSize (Entry->DevicePath);
    } else {
      DevicePathSize = SIZE_OF_OC_CUSTOM_BOOT_DEVICE_PATH + LoadOptionNameSize + sizeof (EFI_DEVICE_PATH_PROTOCOL);
    }

    LoadOptionSize     = sizeof (EFI_LOAD_OPTION) + LoadOptionNameSize + DevicePathSize;

    LoadOption = AllocatePool (LoadOptionSize);
    if (LoadOption == NULL) {
      DEBUG ((DEBUG_INFO, "OCB: Failed to allocate default option (%u)\n", (UINT32) LoadOptionSize));
      if (BootOrder != NULL) {
        FreePool (BootOrder);
      }
      return EFI_OUT_OF_RESOURCES;
    }

    LoadOption->Attributes         = LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_BOOT;
    LoadOption->FilePathListLength = (UINT16) DevicePathSize;
    CopyMem (LoadOption + 1, Entry->Name, LoadOptionNameSize);

    if (!Entry->IsCustom) {
      CopyMem ((UINT8 *) (LoadOption + 1) + LoadOptionNameSize, Entry->DevicePath, DevicePathSize);
    } else {
      DestCustomDevPath = (OC_CUSTOM_BOOT_DEVICE_PATH *) (
        (UINT8 *) (LoadOption + 1) + LoadOptionNameSize
        );
      CopyMem (
        DestCustomDevPath,
        &mOcCustomBootDevPathTemplate,
        sizeof (mOcCustomBootDevPathTemplate)
        );
      CopyMem (
        DestCustomDevPath->EntryName.PathName,
        Entry->Name,
        LoadOptionNameSize
        );
      //
      // FIXME: This may theoretically overflow.
      //
      DestCustomDevPath->EntryName.Header.Length[0] += (UINT8) LoadOptionNameSize;

      DestCustomEndNode = (EFI_DEVICE_PATH_PROTOCOL *) (
        (UINT8 *) DestCustomDevPath + SIZE_OF_OC_CUSTOM_BOOT_DEVICE_PATH + LoadOptionNameSize
        );
      SetDevicePathEndNode (DestCustomEndNode);

      ASSERT (GetDevicePathSize (&DestCustomDevPath->Hdr.Header) == DevicePathSize);
    }

    Status = gRT->SetVariable (
      BootVariableName,
      BootVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS
        | EFI_VARIABLE_RUNTIME_ACCESS
        | EFI_VARIABLE_NON_VOLATILE,
      LoadOptionSize,
      LoadOption
      );

    FreePool (LoadOption);

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Failed to set default entry Boot0080 - %r\n",
        Status
        ));
      if (BootOrder != NULL) {
        FreePool (BootOrder);
      }
      return Status;
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCB: Matched default entry in BootOrder\n"));
    BootChosenIndex = Index;
  }

  //
  // Update BootOrder to contain new option.
  //
  if (BootChosenIndex != BootOrderCount) {
    BootTmp                    = BootOrder[0];
    BootOrder[0]               = BootOrder[BootChosenIndex];
    BootOrder[BootChosenIndex] = BootTmp;
    NewBootOrder               = BootOrder;

    DEBUG ((
      DEBUG_INFO,
      "OCB: Found default entry in BootOrder, reordering %X <-> %X\n",
      NewBootOrder[0],
      NewBootOrder[BootChosenIndex]
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "OCB: Adding default entry Boot0080 to BootOrder\n"
      ));

    NewBootOrder = AllocatePool ((BootOrderCount + 1) * sizeof (*BootOrder));
    if (NewBootOrder == NULL) {
      if (BootOrder != NULL) {
        FreePool (BootOrder);
      }
      return EFI_OUT_OF_RESOURCES;
    }

    NewBootOrder[0] = 0x80;
    CopyMem (&NewBootOrder[1], &BootOrder[0], BootOrderCount * sizeof (*BootOrder));

    if (BootOrder != NULL) {
      FreePool (BootOrder);
    }

    ++BootOrderCount;
  }

  Status = gRT->SetVariable (
    BootOrderName,
    BootVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS
      | EFI_VARIABLE_RUNTIME_ACCESS
      | EFI_VARIABLE_NON_VOLATILE,
    BootOrderCount * sizeof (*BootOrder),
    NewBootOrder
    );

  FreePool (NewBootOrder);

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCB: Failed to set default BootOrder - %r\n",
      Status
      ));
  }

  return Status;
}

/*
  Retrieves the Bootstrap Load Option data, matching it from BootOrder by
  finding a path ending with MatchSuffix.

  @param[out] LoadOptionSize  The size, in bytes, of the Load Option data.
  @param[out] BootOption      The index of the Boot Option.
  @param[out] LoadPath        Pointer into the Load Option data to the
                              Device Path.
  @param[in] BootOptions      The list of Boot Option indices to match.
  @param[in] NumBootOptions   The number of elements in BootOptions.
  @param[in] MatchSuffix      The file Device Path suffix of a matching option.
  @param[in] MatchSuffixLen   The length, in characters, of MatchSuffix.
*/
EFI_LOAD_OPTION *
InternalGetBoostrapOptionData (
  OUT UINTN                    *LoadOptionSize,
  OUT UINT16                   *BootOption,
  OUT EFI_DEVICE_PATH_PROTOCOL **LoadPath,
  IN  UINT16                   *BootOptions,
  IN  UINTN                    NumBootOptions,
  IN  CONST CHAR16             *MatchSuffix,
  IN  UINTN                    MatchSuffixLen
  )
{
  UINTN                    BootOptionIndex;
  EFI_LOAD_OPTION          *CurrLoadOption;
  EFI_DEVICE_PATH_PROTOCOL *CurrDevicePath;
  BOOLEAN                  IsBooptstrap;
  //
  // Check all boot options for trailing "\Bootstrap\Bootstrap.efi".
  //
  for (BootOptionIndex = 0; BootOptionIndex < NumBootOptions; ++BootOptionIndex) {
    CurrLoadOption = InternalGetBootOptionData (
      LoadOptionSize,
      BootOptions[BootOptionIndex],
      &gEfiGlobalVariableGuid
      );
    if (CurrLoadOption == NULL) {
      continue;
    }

    CurrDevicePath = InternalGetBootOptionPath (
      CurrLoadOption,
      *LoadOptionSize
      );
    if (CurrDevicePath == NULL) {
      FreePool (CurrDevicePath);
      continue;
    }

    IsBooptstrap = OcDevicePathHasFilePathSuffix (
      CurrDevicePath,
      MatchSuffix,
      MatchSuffixLen
      );
    if (IsBooptstrap) {
      break;
    }

    FreePool (CurrLoadOption);
  }

  if (BootOptionIndex == NumBootOptions) {
    return NULL;
  }

  *LoadPath   = CurrDevicePath;
  *BootOption = BootOptions[BootOptionIndex];
  return CurrLoadOption;
}

STATIC
EFI_STATUS
InternalRegisterBootstrapBootOption (
  IN CONST CHAR16    *OptionName,
  IN EFI_HANDLE      DeviceHandle,
  IN CONST CHAR16    *FilePath,
  IN BOOLEAN         ShortForm,
  IN CONST CHAR16    *MatchSuffix,
  IN UINTN           MatchSuffixLen
  )
{
  EFI_STATUS                 Status;
  EFI_LOAD_OPTION            *Option;
  UINTN                      OptionNameSize;
  UINTN                      ReferencePathSize;
  UINTN                      OptionSize;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *CurrDevicePath;
  UINTN                      Index;
  UINT16                     *BootOrder;
  UINTN                      BootOrderSize;
  UINT32                     BootOrderAttributes;
  BOOLEAN                    CurrOptionExists;
  BOOLEAN                    CurrOptionValid;
  EFI_DEVICE_PATH_PROTOCOL   *ShortFormPath;
  EFI_DEVICE_PATH_PROTOCOL   *ReferencePath;
  CHAR16                     BootOptionVariable[L_STR_LEN (L"Boot####") + 1];
  UINT16                     BootOptionIndex;
  UINTN                      OrderIndex;

  Status = gBS->HandleProtocol (
    DeviceHandle,
    &gEfiDevicePathProtocolGuid,
    (VOID **) &DevicePath
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCB: Failed to obtain device path for boot option - %r\n", Status));
    return Status;
  }

  DevicePath = AppendFileNameDevicePath (DevicePath, (CHAR16 *) FilePath);
  if (DevicePath == NULL) {
    DEBUG ((DEBUG_INFO, "OCB: Failed to append %s loader path for boot option - %r\n", FilePath));
    return EFI_OUT_OF_RESOURCES;
  }

  ReferencePath = DevicePath;

  if (ShortForm) {
    ShortFormPath = FindDevicePathNodeWithType (
      DevicePath,
      MEDIA_DEVICE_PATH,
      MEDIA_HARDDRIVE_DP
      );
    if (ShortFormPath != NULL) {
      ReferencePath = ShortFormPath;
    }
  }

  CurrOptionValid  = FALSE;
  CurrOptionExists = FALSE;

  BootOrderSize = 0;
  Status = gRT->GetVariable (
    EFI_BOOT_ORDER_VARIABLE_NAME,
    &gEfiGlobalVariableGuid,
    &BootOrderAttributes,
    &BootOrderSize,
    NULL
    );

  DEBUG ((
    DEBUG_INFO,
    "OCB: Have existing order of size %u - %r\n",
    (UINT32) BootOrderSize,
    Status
    ));

  BootOrder = NULL;

  if (Status == EFI_BUFFER_TOO_SMALL && BootOrderSize > 0 && BootOrderSize % sizeof (UINT16) == 0) {
    BootOrder = AllocateZeroPool (BootOrderSize + sizeof (UINT16));
    if (BootOrder == NULL) {
      DEBUG ((DEBUG_INFO, "OCB: Failed to allocate boot order\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gRT->GetVariable (
      EFI_BOOT_ORDER_VARIABLE_NAME,
      &gEfiGlobalVariableGuid,
      &BootOrderAttributes,
      &BootOrderSize,
      (VOID *) (BootOrder + 1)
      );

    if (EFI_ERROR (Status) || BootOrderSize == 0 || BootOrderSize % sizeof (UINT16) != 0) {
      DEBUG ((DEBUG_INFO, "OCB: Failed to obtain boot order %u - %r\n", (UINT32) BootOrderSize, Status));
      FreePool (BootOrder);
      return EFI_OUT_OF_RESOURCES;
    }

    Option = InternalGetBoostrapOptionData (
      &OptionSize,
      &BootOptionIndex,
      &CurrDevicePath,
      &BootOrder[1],
      BootOrderSize / sizeof (*BootOrder),
      MatchSuffix,
      MatchSuffixLen
      );
    CurrOptionExists = Option != NULL;
    if (CurrOptionExists) {
      CurrOptionValid  = IsDevicePathEqual (ReferencePath, CurrDevicePath);
      FreePool (Option);
    }
  } else {
    BootOrderSize = 0;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCB: %a existing option at Boot%04x, %a\n",
    CurrOptionExists ? "Have" : "No",
    BootOrder != NULL ? BootOrder[1] : 0,
    CurrOptionValid ? "valid" : "invalid"
    ));

  if (!CurrOptionValid) {
    //
    // Locate a free boot option index when no Bootstrap entry could be found.
    //
    if (!CurrOptionExists) {
      //
      // High magic numbers cause entry purging on e.g. HP 15-ab237ne, InsydeH2O.
      //
      // Find the lowest unused Boot#### index. In the absolutely unrealistic case
      // that all entries are occupied, always overwrite BootFFFF.
      //
      // Boot0000 is reserved on ASUS boards and is treated like a deleted entry.
      // Setting Boot0000 will essentially cause entries to duplicate and eventual
      // BIOS brick as ASUS boards simply zero removed boot entries instead of
      // shrinking BootOrder size. Reproduced on ASUS ROG STRIX Z370-F GAMING.
      //
      for (BootOptionIndex = 1; BootOptionIndex < 0xFFFF; ++BootOptionIndex) {
        for (OrderIndex = 0; OrderIndex < BootOrderSize / sizeof (*BootOrder); ++OrderIndex) {
          if (BootOrder[OrderIndex + 1] == BootOptionIndex) {
            break;
          }
        }

        if (OrderIndex == BootOrderSize / sizeof (*BootOrder)) {
          break;
        }
      }
    }

    UnicodeSPrint (
      BootOptionVariable,
      sizeof (BootOptionVariable),
      L"Boot%04x",
      BootOptionIndex
      );

    OptionNameSize    = StrSize (OptionName);
    ReferencePathSize = GetDevicePathSize (ReferencePath);
    OptionSize        = sizeof (EFI_LOAD_OPTION) + OptionNameSize + ReferencePathSize;

    DEBUG ((DEBUG_INFO, "OCB: Creating boot option %s of %u bytes\n", OptionName, (UINT32) OptionSize));

    Option = AllocatePool (OptionSize);
    if (Option == NULL) {
      DEBUG ((DEBUG_INFO, "OCB: Failed to allocate boot option (%u)\n", (UINT32) OptionSize));
      FreePool (DevicePath);
      return EFI_OUT_OF_RESOURCES;
    }

    Option->Attributes         = LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_BOOT;
    Option->FilePathListLength = (UINT16) ReferencePathSize;
    CopyMem (Option + 1, OptionName, OptionNameSize);
    CopyMem ((UINT8 *) (Option + 1) + OptionNameSize, ReferencePath, ReferencePathSize);

    Status = gRT->SetVariable (
      BootOptionVariable,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS
        | EFI_VARIABLE_RUNTIME_ACCESS
        | EFI_VARIABLE_NON_VOLATILE,
      OptionSize,
      Option
      );

    FreePool (Option);
    FreePool (DevicePath);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCB: Failed to store boot option - %r\n", Status));
      return Status;
    }
  }

  if (BootOrderSize != 0) {
    if (BootOrder[1] == BootOptionIndex) {
      DEBUG ((DEBUG_INFO, "OCB: Boot order has first option as the default option\n"));
      FreePool (BootOrder);
      return EFI_SUCCESS;
    }

    BootOrder[0] = BootOptionIndex;

    Index = 1;
    while (Index <= BootOrderSize / sizeof (UINT16)) {
      if (BootOrder[Index] == BootOptionIndex) {
        DEBUG ((DEBUG_INFO, "OCB: Moving boot option to the front from %u position\n", (UINT32) Index));
        CopyMem (
          &BootOrder[Index],
          &BootOrder[Index + 1],
          BootOrderSize - Index * sizeof (UINT16)
          );
        BootOrderSize -= sizeof (UINT16);
      } else {
        ++Index;
      }
    }

    Status = gRT->SetVariable (
      EFI_BOOT_ORDER_VARIABLE_NAME,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS
        | EFI_VARIABLE_RUNTIME_ACCESS
        | EFI_VARIABLE_NON_VOLATILE,
      BootOrderSize + sizeof (UINT16),
      BootOrder
      );

    FreePool (BootOrder);
  } else {
    Status = gRT->SetVariable (
      EFI_BOOT_ORDER_VARIABLE_NAME,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS
        | EFI_VARIABLE_RUNTIME_ACCESS
        | EFI_VARIABLE_NON_VOLATILE,
      sizeof (UINT16),
      &BootOptionIndex
      );
  }

  DEBUG ((DEBUG_INFO, "OCB: Wrote new boot order with boot option - %r\n", Status));
  return EFI_SUCCESS;
}

EFI_STATUS
OcRegisterBootstrapBootOption (
  IN CONST CHAR16    *OptionName,
  IN EFI_HANDLE      DeviceHandle,
  IN CONST CHAR16    *FilePath,
  IN BOOLEAN         ShortForm,
  IN CONST CHAR16    *MatchSuffix,
  IN UINTN           MatchSuffixLen
  )
{
  EFI_STATUS                    Status;
  OC_FIRMWARE_RUNTIME_PROTOCOL  *FwRuntime;
  OC_FWRT_CONFIG                Config;

  Status = gBS->LocateProtocol (
    &gOcFirmwareRuntimeProtocolGuid,
    NULL,
    (VOID **) &FwRuntime
    );

  if (!EFI_ERROR (Status) && FwRuntime->Revision == OC_FIRMWARE_RUNTIME_REVISION) {
    ZeroMem (&Config, sizeof (Config));
    FwRuntime->SetOverride (&Config);
    DEBUG ((DEBUG_INFO, "OCB: Found FW NVRAM, full access %d\n", Config.BootVariableRedirect));
  } else {
    FwRuntime = NULL;
    DEBUG ((DEBUG_INFO, "OCB: Missing FW NVRAM, going on...\n"));
  }

  Status = InternalRegisterBootstrapBootOption (
    OptionName,
    DeviceHandle,
    FilePath,
    ShortForm,
    MatchSuffix,
    MatchSuffixLen
    );

  if (FwRuntime != NULL) {
    FwRuntime->SetOverride (NULL);
  }

  return Status;
}

EFI_STATUS
InternalLoadBootEntry (
  IN  OC_PICKER_CONTEXT           *Context,
  IN  OC_BOOT_ENTRY               *BootEntry,
  IN  EFI_HANDLE                  ParentHandle,
  OUT EFI_HANDLE                  *EntryHandle,
  OUT INTERNAL_DMG_LOAD_CONTEXT   *DmgLoadContext
  )
{
  EFI_STATUS                 Status;
  EFI_STATUS                 OptionalStatus;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_HANDLE                 StorageHandle;
  EFI_DEVICE_PATH_PROTOCOL   *StoragePath;
  CHAR16                     *UnicodeDevicePath;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  VOID                       *EntryData;
  UINT32                     EntryDataSize;
  CONST CHAR8                *Args;

  ASSERT (BootEntry != NULL);
  //
  // System entries are not loaded but called directly.
  //
  ASSERT ((BootEntry->Type & OC_BOOT_SYSTEM) == 0);
  ASSERT (Context != NULL);
  ASSERT (DmgLoadContext != NULL);

  //
  // TODO: support Apple loaded image, policy, and dmg boot.
  //

  ZeroMem (DmgLoadContext, sizeof (*DmgLoadContext));

  EntryData     = NULL;
  EntryDataSize = 0;
  StorageHandle = NULL;
  StoragePath   = NULL;

  if (BootEntry->IsFolder) {
    if (Context->DmgLoading == OcDmgLoadingDisabled) {
      return EFI_SECURITY_VIOLATION;
    }

    DmgLoadContext->DevicePath = BootEntry->DevicePath;
    DevicePath = InternalLoadDmg (DmgLoadContext, Context->DmgLoading);
    if (DevicePath == NULL) {
      return EFI_UNSUPPORTED;
    }
  } else if (BootEntry->Type == OC_BOOT_EXTERNAL_TOOL) {
    ASSERT (Context->CustomRead != NULL);

    Status = Context->CustomRead (
      Context->StorageContext,
      BootEntry,
      &EntryData,
      &EntryDataSize,
      &DevicePath,
      &StorageHandle,
      &StoragePath
      );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    DevicePath = BootEntry->DevicePath;
  }

  DEBUG_CODE_BEGIN ();
  ASSERT (DevicePath != NULL);
  UnicodeDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  DEBUG ((
    DEBUG_INFO,
    "OCB: Perform boot %s to dp %s (%p/%u)\n",
    BootEntry->Name,
    UnicodeDevicePath != NULL ? UnicodeDevicePath : L"<null>",
    EntryData,
    EntryDataSize
    ));
  if (UnicodeDevicePath != NULL) {
    FreePool (UnicodeDevicePath);
  }
  DEBUG_CODE_END ();

  Status = gBS->LoadImage (
    FALSE,
    ParentHandle,
    DevicePath,
    EntryData,
    EntryDataSize,
    EntryHandle
    );

  if (EntryData != NULL) {
    FreePool (EntryData);
  }

  if (!EFI_ERROR (Status)) {
    OptionalStatus = gBS->HandleProtocol (
      *EntryHandle,
      &gEfiLoadedImageProtocolGuid,
      (VOID **) &LoadedImage
      );
    if (!EFI_ERROR (OptionalStatus)) {
      DEBUG ((
        DEBUG_INFO,
        "OCB: Matching <%a> args on type %u %p\n",
        Context->AppleBootArgs,
        BootEntry->Type,
        BootEntry->LoadOptions
        ));

      LoadedImage->LoadOptionsSize = 0;
      LoadedImage->LoadOptions     = NULL;

      if (BootEntry->LoadOptions == NULL && (BootEntry->Type & OC_BOOT_APPLE_ANY) != 0) {
        Args    = Context->AppleBootArgs;
      } else {
        Args    = BootEntry->LoadOptions;
      }

      if (Args != NULL && Args[0] != '\0') {
        OcAppendArgumentsToLoadedImage (
          LoadedImage,
          &Args,
          1,
          TRUE
          );
      }

      if (BootEntry->Type == OC_BOOT_EXTERNAL_OS || BootEntry->Type == OC_BOOT_EXTERNAL_TOOL) {
        DEBUG ((
          DEBUG_INFO,
          "OCB: Custom (%u) DeviceHandle %p FilePath %p\n",
          BootEntry->Type,
          LoadedImage->DeviceHandle,
          LoadedImage->FilePath
          ));

        //
        // Some fragile firmware fail to properly set LoadedImage file source
        // fields to our custom device path, so we fix it up here.
        // REF: https://github.com/acidanthera/bugtracker/issues/712
        //
        if (LoadedImage->DeviceHandle == NULL && StorageHandle != NULL) {
          if (LoadedImage->FilePath != NULL) {
            FreePool (LoadedImage->FilePath);
          }
          LoadedImage->DeviceHandle = StorageHandle;
          LoadedImage->FilePath     = StoragePath;
        }
      }
    }
  } else {
    InternalUnloadDmg (DmgLoadContext);
  }

  return Status;
}
