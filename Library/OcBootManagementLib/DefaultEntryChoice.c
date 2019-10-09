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

#include "BootManagementInternal.h"

#include <Guid/AppleFile.h>
#include <Guid/AppleVariable.h>
#include <Guid/GlobalVariable.h>
#include <Guid/OcVariables.h>

#include <Protocol/LoadedImage.h>

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
  IN  EFI_GUID *BootGuid,
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
             BootGuid,
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

STATIC
OC_BOOT_ENTRY *
InternalGetBootEntryByDevicePath (
  IN OUT OC_BOOT_ENTRY             *BootEntries,
  IN     UINTN                     NumBootEntries,
  IN     EFI_DEVICE_PATH_PROTOCOL  *UefiDevicePath,
  IN     EFI_DEVICE_PATH_PROTOCOL  *UefiRemainingDevicePath,
  IN     BOOLEAN                   IsBootNext
  )
{
  INTN                     CmpResult;

  UINTN                    RootDevicePathSize;

  EFI_DEVICE_PATH_PROTOCOL *OcDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *OcRemainingDevicePath;

  OC_BOOT_ENTRY            *BootEntry;
  UINTN                    Index;

  RootDevicePathSize = ((UINT8 *)UefiRemainingDevicePath - (UINT8 *)UefiDevicePath);

  for (Index = 0; Index < NumBootEntries; ++Index) {
    BootEntry = &BootEntries[Index];
    if (BootEntry->Type == OcBootCustom || BootEntry->Type == OcBootSystem) {
      continue;
    }

    OcDevicePath = BootEntry->DevicePath;
    ASSERT (OcDevicePath != NULL);

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
        BootEntry->DevicePath = UefiDevicePath;
      }
    }

    return BootEntry;
  }

  return NULL;
}

STATIC
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

/**
  Obtain default entry from the list.

  @param[in,out]  BootEntries      Described list of entries, may get updated.
  @param[in]      NumBootEntries   Positive number of boot entries.
  @param[in]      CustomBootGuid   Use custom GUID for Boot#### lookup.
  @param[in]      LoadHandle       Handle to skip (potential OpenCore handle).

  @retval  boot entry or NULL.
**/
STATIC
OC_BOOT_ENTRY *
InternalGetDefaultBootEntry (
  IN OUT OC_BOOT_ENTRY  *BootEntries,
  IN     UINTN          NumBootEntries,
  IN     BOOLEAN        CustomBootGuid,
  IN     EFI_HANDLE     LoadHandle  OPTIONAL
  )
{
  OC_BOOT_ENTRY            *BootEntry;

  EFI_STATUS               Status;
  INTN                     NumPatchedNodes;

  UINT32                   BootNextAttributes;
  UINTN                    BootNextSize;
  BOOLEAN                  IsBootNext;

  UINT16                   *BootOrder;
  UINTN                    BootOrderSize;

  EFI_DEVICE_PATH_PROTOCOL *UefiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *UefiRemainingDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL *PrevDevicePath;
  UINT32                   OptionalDataSize;
  VOID                     *OptionalData;
  EFI_GUID                 *BootVariableGuid;

  EFI_HANDLE               DeviceHandle;

  UINT16                   BootNextOptionIndex;

  BOOLEAN                  IsAppleLegacy;
  UINTN                    DevPathSize;
  EFI_DEVICE_PATH_PROTOCOL *EspDevicePath;

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

    UefiRemainingDevicePath = UefiDevicePath;
    Status = gBS->LocateDevicePath (
                    &gEfiSimpleFileSystemProtocolGuid,
                    &UefiRemainingDevicePath,
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
    DEBUG ((DEBUG_INFO, "OCB: BootNext retrieval failed - %r", Status));
    return NULL;
  }

  IsAppleLegacy = InternalIsAppleLegacyLoadApp (UefiDevicePath);
  if (IsAppleLegacy) {
    DEBUG ((DEBUG_INFO, "OCB: Default is AppleLegacyLoadApp\n"));
    FreePool (UefiDevicePath);
    Status = GetVariable2 (
               L"BootCampHD",
               &gAppleBootVariableGuid,
               (VOID **)&UefiDevicePath,
               &DevPathSize
               );
    if (EFI_ERROR (Status) || !IsDevicePathValid (UefiDevicePath, DevPathSize)) {
      if (OptionalData != NULL) {
        FreePool (OptionalData);
      }

      return NULL;
    }
  }

  DebugPrintDevicePath (DEBUG_INFO, "OCB: Default DP pre-fix", UefiDevicePath);

  UefiRemainingDevicePath = UefiDevicePath;
  NumPatchedNodes = OcFixAppleBootDevicePath (&UefiRemainingDevicePath);

  DebugPrintDevicePath (
    DEBUG_INFO,
    "OCB: Default DP post-fix",
    UefiDevicePath
    );
  DebugPrintDevicePath (
    DEBUG_INFO,
    "OCB: Default DP post-fix remainder",
    UefiRemainingDevicePath
    );

  if (NumPatchedNodes == -1) {
    DEBUG ((DEBUG_WARN, "OCB: Failed to fix the default boot Device Path\n"));
    return NULL;
  }

  if (IsAppleLegacy) {
    EspDevicePath = OcDiskFindSystemPartitionPath (
                      UefiDevicePath,
                      &DevPathSize
                      );

    FreePool (UefiDevicePath);

    if (EspDevicePath == NULL) {
      DEBUG ((DEBUG_INFO, "Failed to locate the disk's ESP\n"));
      if (OptionalData != NULL) {
        FreePool (OptionalData);
      }

      return NULL;
    }
    //
    // CAUTION: This must NOT be freed!
    //
    UefiDevicePath = EspDevicePath;
    //
    // The Device Path must be entirely locatable as
    // OcDiskFindSystemPartitionPath() guarantees to only return valid paths.
    //
    ASSERT (DevPathSize > END_DEVICE_PATH_LENGTH);
    DevPathSize -= END_DEVICE_PATH_LENGTH;
    UefiRemainingDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)(
                                (UINTN)EspDevicePath + DevPathSize
                                );

    DebugPrintDevicePath (
      DEBUG_INFO,
      "OCB: Default DP post-loc",
      UefiDevicePath
      );
  }

  if (UefiDevicePath != UefiRemainingDevicePath) {
    //
    // If the Device Path node was advanced, it cannot be a short-form.
    //
    BootEntry = InternalGetBootEntryByDevicePath (
                   BootEntries,
                   NumBootEntries,
                   UefiDevicePath,
                   UefiRemainingDevicePath,
                   IsBootNext
                   );
  } else {
    //
    // If the Device Path node was not advanced, it might be a short-form.
    //
    PrevDevicePath = NULL;
    do {
      FullDevicePath = OcGetNextLoadOptionDevicePath (
                         UefiDevicePath,
                         PrevDevicePath
                         );

      if (PrevDevicePath != NULL) {
        FreePool (PrevDevicePath);
      }

      if (FullDevicePath == NULL) {
        DEBUG ((DEBUG_INFO, "OCB: Short-form DP could not be expanded.\n"));
        BootEntry = NULL;
        break;
      }

      PrevDevicePath = FullDevicePath;

      DebugPrintDevicePath (DEBUG_INFO, "OCB: Expanded DP", FullDevicePath);

      UefiRemainingDevicePath = FullDevicePath;
      Status = gBS->LocateDevicePath (
                      &gEfiDevicePathProtocolGuid,
                      &UefiRemainingDevicePath,
                      &DeviceHandle
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      DebugPrintDevicePath (
        DEBUG_INFO,
        "OCB: Expanded DP remainder",
        UefiRemainingDevicePath
        );

      BootEntry = InternalGetBootEntryByDevicePath (
                    BootEntries,
                    NumBootEntries,
                    FullDevicePath,
                    UefiRemainingDevicePath,
                    IsBootNext
                    );
    } while (BootEntry == NULL);

    if (FullDevicePath != NULL) {
      if (!IsAppleLegacy) {
        FreePool (UefiDevicePath);
      }
      UefiDevicePath = FullDevicePath;
    }
  }

  if (BootEntry != NULL) {
#if 0
    if (IsBootNext) {
      //
      // BootNext is allowed to override both the exact file path as well as
      // the used load options.
      // TODO: Investigate whether Apple uses OptionalData, and exploit ways.
      //
      BootEntry->LoadOptionsSize = OptionalDataSize;
      BootEntry->LoadOptions     = OptionalData;
    } else
#endif
    if (OptionalData != NULL) {
      FreePool (OptionalData);
    }

    if (BootEntry->DevicePath != UefiDevicePath) {
      if (!IsAppleLegacy) {
        FreePool (UefiDevicePath);
      }
    } else {
      ASSERT (IsBootNext);
    }

    DEBUG ((
      DEBUG_INFO,
      "OCB: Matched default boot option: %s\n",
      BootEntry->Name
      ));

    return BootEntry;
  }

  if (OptionalData != NULL) {
    FreePool (OptionalData);
  }

  if (!IsAppleLegacy) {
    FreePool (UefiDevicePath);
  }

  DEBUG ((DEBUG_WARN, "OCB: Failed to match a default boot option\n"));

  return NULL;
}

UINT32
OcGetDefaultBootEntry (
  IN     OC_PICKER_CONTEXT  *Context,
  IN OUT OC_BOOT_ENTRY      *BootEntries,
  IN     UINTN              NumBootEntries
  )
{
  UINT32          BootEntryIndex;
  OC_BOOT_ENTRY   *BootEntry;
  UINTN           Index;

  BootEntry = InternalGetDefaultBootEntry (
    BootEntries,
    NumBootEntries,
    Context->CustomBootGuid,
    Context->ExcludeHandle
    );

  if (BootEntry != NULL) {
    BootEntryIndex = (UINT32) (BootEntry - BootEntries);
    DEBUG ((DEBUG_INFO, "OCB: Initial default is %u\n", BootEntryIndex));
  } else {
    BootEntryIndex = 0;
    DEBUG ((DEBUG_INFO, "OCB: Initial default is 0, fallback\n"));
  }

  if (Context->PickerCommand == OcPickerBootApple) {
    if (BootEntries[BootEntryIndex].Type != OcBootApple) {
      for (Index = 0; Index < NumBootEntries; ++Index) {
        if (BootEntries[Index].Type == OcBootApple) {
          BootEntryIndex = (UINT32) Index;
          DEBUG ((DEBUG_INFO, "OCB: Override default to Apple %u\n", BootEntryIndex));
          break;
        }
      }
    }
  } else if (Context->PickerCommand == OcPickerBootAppleRecovery) {
    if (BootEntries[BootEntryIndex].Type != OcBootAppleRecovery) {
      if (BootEntryIndex + 1 < NumBootEntries
        && BootEntries[BootEntryIndex + 1].Type == OcBootAppleRecovery) {
        BootEntryIndex = BootEntryIndex + 1;
        DEBUG ((DEBUG_INFO, "OCB: Override default to Apple Recovery %u, next\n", BootEntryIndex));
      } else {
        for (Index = 0; Index < NumBootEntries; ++Index) {
          if (BootEntries[Index].Type == OcBootAppleRecovery) {
            BootEntryIndex = (UINT32) Index;
            DEBUG ((DEBUG_INFO, "OCB: Override default option to Apple Recovery %u\n", BootEntryIndex));
            break;
          }
        }
      }
    }
  }

  return BootEntryIndex;
}

#if 0
STATIC
VOID
InternalReportLoadOption (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_GUID                  *BootGuid
  )
{
  EFI_STATUS          Status;
  UINTN               DevicePathSize;
  UINTN               LoadOptionSize;
  EFI_LOAD_OPTION     *LoadOption;
  UINT16              LoadOptionNo;
  EFI_LOAD_OPTION     *CurrLoadOption;
  CONST CHAR16        *LoadOptionName;
  UINTN               LoadOptionNameSize;
  UINTN               CurrLoadOptionSize;

  //
  // Always report valid option in BootCurrent.
  // Unless done there is no way for Windows to properly hibernate.
  //

  LoadOptionName     = L"OC Boot";
  LoadOptionNameSize = L_STR_SIZE (L"OC Boot");
  DevicePathSize     = GetDevicePathSize (DevicePath);
  LoadOptionSize     = sizeof (EFI_LOAD_OPTION) + LoadOptionNameSize + DevicePathSize;

  LoadOption = AllocatePool (LoadOptionSize);
  if (LoadOption == NULL) {
    DEBUG ((DEBUG_INFO, "OCB: Failed to allocate BootFFFF (%u)\n", (UINT32) LoadOptionSize));
    return;
  }

  LoadOption->Attributes         = LOAD_OPTION_HIDDEN;
  LoadOption->FilePathListLength = DevicePathSize;
  CopyMem (LoadOption + 1, LoadOptionName, LoadOptionNameSize);
  CopyMem ((UINT8 *) (LoadOption + 1) + LoadOptionNameSize, DevicePath, DevicePathSize);

  CurrLoadOption = NULL;
  CurrLoadOptionSize = 0;
  Status = GetVariable2 (
    L"BootFFFF",
    BootGuid,
    (VOID **) &CurrLoadOption,
    &CurrLoadOptionSize
    );
  if (EFI_ERROR (Status)
    || CurrLoadOptionSize != LoadOptionSize
    || CompareMem (CurrLoadOption, LoadOption, LoadOptionSize) != 0) {

    DEBUG ((
      DEBUG_INFO,
      "OCB: Overwriting BootFFFF (%r/%u)\n",
      Status,
      (UINT32) CurrLoadOptionSize,
      (UINT32) LoadOptionSize
      ));

    gRT->SetVariable (
      L"BootFFFF",
      BootGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS
        | EFI_VARIABLE_RUNTIME_ACCESS
        | EFI_VARIABLE_NON_VOLATILE,
      LoadOptionSize,
      LoadOption
      );
  } else {
    DEBUG ((DEBUG_INFO, "OCB: Accepting same BootFFFF\n"));
  }

  if (CurrLoadOption != NULL) {
    FreePool (CurrLoadOption);
  }
  FreePool (LoadOption);

  LoadOptionNo = 0xFFFF;
  gRT->SetVariable (
    L"BootCurrent",
    BootGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS
      | EFI_VARIABLE_RUNTIME_ACCESS,
    sizeof (LoadOptionNo),
    &LoadOptionNo
    );
}
#endif

EFI_STATUS
InternalLoadBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
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
  CHAR16                     *UnicodeDevicePath;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  VOID                       *EntryData;
  UINT32                     EntryDataSize;
  CONST CHAR8                *Args;
  UINT32                     ArgsLen;

  ASSERT (BootPolicy != NULL);
  ASSERT (BootEntry != NULL);
  //
  // System entries are not loaded but called directly.
  //
  ASSERT (BootEntry->Type != OcBootSystem);
  ASSERT (Context != NULL);
  ASSERT (DmgLoadContext != NULL);

  //
  // TODO: support Apple loaded image, policy, and dmg boot.
  //

  ZeroMem (DmgLoadContext, sizeof (*DmgLoadContext));

  EntryData    = NULL;
  EntryDataSize = 0;

  if (BootEntry->IsFolder) {
    if ((Context->LoadPolicy & OC_LOAD_ALLOW_DMG_BOOT) == 0) {
      return EFI_SECURITY_VIOLATION;
    }

    DmgLoadContext->DevicePath = BootEntry->DevicePath;
    DevicePath = InternalLoadDmg (
                   DmgLoadContext,
                   BootPolicy,
                   Context->LoadPolicy
                   );
    if (DevicePath == NULL) {
      return EFI_UNSUPPORTED;
    }
  } else if (BootEntry->Type == OcBootCustom && BootEntry->DevicePath == NULL) {
    ASSERT (Context->CustomRead != NULL);

    Status = Context->CustomRead (
      Context->CustomEntryContext,
      BootEntry,
      &EntryData,
      &EntryDataSize,
      &DevicePath
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
#if 0
    InternalReportLoadOption (
      DevicePath,
      Context->CustomBootGuid ? &gOcVendorVariableGuid : &gEfiGlobalVariableGuid
      );
#endif

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

      if (BootEntry->LoadOptions == NULL
        && (BootEntry->Type == OcBootApple || BootEntry->Type == OcBootAppleRecovery)) {
        Args    = Context->AppleBootArgs;
        ArgsLen = (UINT32)AsciiStrLen (Args);
      } else {
        Args    = BootEntry->LoadOptions;
        ArgsLen = BootEntry->LoadOptionsSize;
        ASSERT (ArgsLen == ((Args == NULL) ? 0 : (UINT32)AsciiStrLen (Args)));
      }

      if (ArgsLen > 0) {
        LoadedImage->LoadOptions = AsciiStrCopyToUnicode (Args, ArgsLen);
        if (LoadedImage->LoadOptions != NULL) {
          LoadedImage->LoadOptionsSize = ArgsLen * sizeof (CHAR16) + sizeof (CHAR16);
        }
      }

      if (BootEntry->Type == OcBootCustom) {
        DEBUG ((
          DEBUG_INFO,
          "OCB: Custom DeviceHandle %p FilePath %p\n",
          LoadedImage->DeviceHandle,
          LoadedImage->FilePath
          ));
      }
    }
  } else {
    InternalUnloadDmg (DmgLoadContext);
  }

  return Status;
}
