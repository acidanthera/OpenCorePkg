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
#include <Guid/OcVariable.h>

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
    for (Index = 0; Index < BootOrderCount; ++Index) {
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
    if (Predefined == 0) {
      BootOrder      = &ApplePredefinedVariables[0];
      BootOrderCount = ARRAY_SIZE (ApplePredefinedVariables);
      DEBUG ((DEBUG_INFO, "OCB: Parsing predefined list...\n"));
    }
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
    if (BootEntry->DevicePath == NULL || BootEntry->Type == OC_BOOT_SYSTEM) {
      continue;
    }

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

  //
  // Precede variable with boot next.
  //
  if (WithBootNext) {
    VariableSize = sizeof (BootNext);
    Status = gRT->GetVariable (
      EFI_BOOT_NEXT_VARIABLE_NAME,
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
    EFI_BOOT_ORDER_VARIABLE_NAME,
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
      EFI_BOOT_ORDER_VARIABLE_NAME,
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
  OUT UINTN     *BootOrderCount
  )
{
  UINT16                           *BootOrder;
  BOOLEAN                          HasBootNext;

  BootOrder = OcGetBootOrder (
    BootVariableGuid,
    TRUE,
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

  if (HasBootNext) {
    gRT->SetVariable (
      EFI_BOOT_NEXT_VARIABLE_NAME,
      BootVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
      0,
      NULL
      );
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
  OC_BOOT_ENTRY    *MatchedEntry;
  EFI_GUID         *BootVariableGuid;
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
  } else {
    BootVariableGuid = &gEfiGlobalVariableGuid;
  }

  BootOrder = OcGetBootOrder (
    BootVariableGuid,
    FALSE,
    &BootOrderCount,
    NULL,
    NULL
    );

  MatchedEntry    = NULL;
  BootChosenIndex = BootOrderCount;
  for (Index = 0; Index < BootOrderCount; ++Index) {
    if (BootOrder[Index] == 0x80) {
      BootChosenIndex = Index;
    }

    if (MatchedEntry != NULL) {
      if (BootChosenIndex != BootOrderCount) {
        break;
      }
      continue;
    }

    BootOptionDevicePath = InternalGetBootOptionData (
      BootOrder[Index],
      BootVariableGuid,
      NULL,
      NULL,
      NULL
      );

    if (BootOptionDevicePath == NULL) {
      continue;
    }

    BootOptionRemainingDevicePath = BootOptionDevicePath;
    Status = gBS->LocateDevicePath (
      &gEfiSimpleFileSystemProtocolGuid,
      &BootOptionRemainingDevicePath,
      &DeviceHandle
      );

    if (!EFI_ERROR (Status)) {
      MatchedEntry = InternalGetBootEntryByDevicePath (
        Entry,
        1,
        BootOptionDevicePath,
        BootOptionRemainingDevicePath,
        FALSE
        );
    }
  }

  if (MatchedEntry == NULL) {
    //
    // Write to Boot0080
    //
    LoadOptionNameSize = StrSize (Entry->Name);
    DevicePathSize     = GetDevicePathSize (Entry->DevicePath);
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
    CopyMem ((UINT8 *) (LoadOption + 1) + LoadOptionNameSize, Entry->DevicePath, DevicePathSize);

    Status = gRT->SetVariable (
      L"Boot0080",
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
    EFI_BOOT_ORDER_VARIABLE_NAME,
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

STATIC
EFI_STATUS
InternalRegisterBootOption (
  IN CONST CHAR16    *OptionName,
  IN EFI_HANDLE      DeviceHandle,
  IN CONST CHAR16    *FilePath
  )
{
  EFI_STATUS                 Status;
  EFI_LOAD_OPTION            *Option;
  UINTN                      OptionNameSize;
  UINTN                      DevicePathSize;
  UINTN                      OptionSize;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL   *CurrDevicePath;
  UINTN                      Index;
  UINT16                     *BootOrder;
  UINTN                      BootOrderSize;
  UINT32                     BootOrderAttributes;
  UINT16                     NewBootOrder;
  BOOLEAN                    CurrOptionValid;

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

  CurrDevicePath = InternalGetBootOptionData (OC_BOOT_OPTION, &gEfiGlobalVariableGuid, NULL, NULL, NULL);
  if (CurrDevicePath != NULL) {
    CurrOptionValid = IsDevicePathEqual (DevicePath, CurrDevicePath);
    FreePool (CurrDevicePath);
  } else {
    CurrOptionValid = FALSE;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCB: Have existing option %d, valid %d\n",
    CurrDevicePath != NULL,
    CurrOptionValid
    ));

  if (!CurrOptionValid) {
    OptionNameSize = StrSize (OptionName);
    DevicePathSize = GetDevicePathSize (DevicePath);
    OptionSize     = sizeof (EFI_LOAD_OPTION) + OptionNameSize + DevicePathSize;

    DEBUG ((DEBUG_INFO, "OCB: Creating boot option %s of %u bytes\n", OptionName, (UINT32) OptionSize));

    Option = AllocatePool (OptionSize);
    if (Option == NULL) {
      DEBUG ((DEBUG_INFO, "OCB: Failed to allocate boot option (%u)\n", (UINT32) OptionSize));
      FreePool (DevicePath);
      return EFI_OUT_OF_RESOURCES;
    }

    Option->Attributes         = LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_BOOT;
    Option->FilePathListLength = (UINT16) DevicePathSize;
    CopyMem (Option + 1, OptionName, OptionNameSize);
    CopyMem ((UINT8 *) (Option + 1) + OptionNameSize, DevicePath, DevicePathSize);

    Status = gRT->SetVariable (
      OC_BOOT_OPTION_VARIABLE_NAME,
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

  if (Status == EFI_BUFFER_TOO_SMALL && BootOrderSize > 0 && BootOrderSize % sizeof (UINT16) == 0) {
    BootOrder = AllocatePool (BootOrderSize + sizeof (UINT16));
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
      if (!EFI_ERROR (Status)) {
        FreePool (BootOrder);
      }
      return EFI_OUT_OF_RESOURCES;
    }

    if (BootOrder[1] == OC_BOOT_OPTION) {
      DEBUG ((DEBUG_INFO, "OCB: Boot order has first option as the default option\n"));
      FreePool (BootOrder);
      return EFI_SUCCESS;
    }

    BootOrder[0] = OC_BOOT_OPTION;

    Index = 1;
    while (Index <= BootOrderSize / sizeof (UINT16)) {
      if (BootOrder[Index] == OC_BOOT_OPTION) {
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
    NewBootOrder = OC_BOOT_OPTION;
    Status = gRT->SetVariable (
      EFI_BOOT_ORDER_VARIABLE_NAME,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS
        | EFI_VARIABLE_RUNTIME_ACCESS
        | EFI_VARIABLE_NON_VOLATILE,
      sizeof (UINT16),
      &NewBootOrder
      );
  }

  DEBUG ((DEBUG_INFO, "OCB: Wrote new boot order with boot option - %r\n", Status));
  return EFI_SUCCESS;
}

EFI_STATUS
OcRegisterBootOption (
  IN CONST CHAR16    *OptionName,
  IN EFI_HANDLE      DeviceHandle,
  IN CONST CHAR16    *FilePath
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

  Status = InternalRegisterBootOption (
    OptionName,
    DeviceHandle,
    FilePath
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
  EFI_HANDLE                 ParentDeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL   *ParentFilePath;
  CHAR16                     *UnicodeDevicePath;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  VOID                       *EntryData;
  UINT32                     EntryDataSize;
  CONST CHAR8                *Args;
  UINT32                     ArgsLen;

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

  EntryData          = NULL;
  EntryDataSize      = 0;
  ParentDeviceHandle = NULL;
  ParentFilePath     = NULL;

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
      Context->CustomEntryContext,
      BootEntry,
      &EntryData,
      &EntryDataSize,
      &DevicePath,
      &ParentDeviceHandle,
      &ParentFilePath
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
        ArgsLen = (UINT32) AsciiStrLen (Args);
      } else {
        Args    = BootEntry->LoadOptions;
        ArgsLen = BootEntry->LoadOptionsSize;
        ASSERT (ArgsLen == ((Args == NULL) ? 0 : (UINT32) AsciiStrLen (Args)));
      }

      if (ArgsLen > 0) {
        LoadedImage->LoadOptions = AsciiStrCopyToUnicode (Args, ArgsLen);
        if (LoadedImage->LoadOptions != NULL) {
          LoadedImage->LoadOptionsSize = ArgsLen * sizeof (CHAR16) + sizeof (CHAR16);
        }
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
        // Some fragile firmwares fail to properly set LoadedImage file source
        // fields to our custom device path, so we fix it up here.
        // REF: https://github.com/acidanthera/bugtracker/issues/712
        //
        if (LoadedImage->DeviceHandle == NULL && ParentDeviceHandle != NULL) {
          if (LoadedImage->FilePath != NULL) {
            FreePool (LoadedImage->FilePath);
          }
          LoadedImage->DeviceHandle = ParentDeviceHandle;
          LoadedImage->FilePath     = DuplicateDevicePath (ParentFilePath);
        }
      }
    }
  } else {
    InternalUnloadDmg (DmgLoadContext);
  }

  return Status;
}
