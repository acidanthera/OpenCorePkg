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

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleTextOut.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcGuardLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

STATIC
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
  EFI_DEVICE_PATH_PROTOCOL   *HandleFilePath;
  CHAR16                     *UnicodeDevicePath;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_HANDLE                 DeviceHandle;
  VOID                       *EntryData;
  UINT32                     EntryDataSize;

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
  } else if (BootEntry->IsCustom) {
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
    OptionalStatus = gBS->HandleProtocol (
                            ParentHandle,
                            &gEfiLoadedImageProtocolGuid,
                            (VOID **) &LoadedImage
                            );
    if (!EFI_ERROR (OptionalStatus)) {
      LoadedImage->LoadOptionsSize = BootEntry->LoadOptionsSize;
      LoadedImage->LoadOptions     = BootEntry->LoadOptions;

      if (BootEntry->IsCustom) {
        DEBUG ((
          DEBUG_INFO,
          "OCB: Custom DeviceHandle %p FilePath %p\n",
          LoadedImage->DeviceHandle,
          LoadedImage->FilePath
          ));

        HandleFilePath = DevicePath;
        DeviceHandle   = NULL;
        Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &HandleFilePath, &DeviceHandle);
        DEBUG ((DEBUG_INFO, "OCB: LDP (%r) %p vs %p %p\n", Status, DeviceHandle, DevicePath, HandleFilePath));
      }
    }
  } else {
    InternalUnloadDmg (DmgLoadContext);
  }

  return Status;
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

  //
  // Custom entries need no special description.
  //
  if (BootEntry->IsCustom) {
    return EFI_SUCCESS;
  }

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
  BootEntry->Name = InternalGetAppleDiskLabel (FileSystem, BootDirectoryName, L".contentDetails");
  if (BootEntry->Name == NULL) {
    BootEntry->Name = InternalGetAppleDiskLabel (FileSystem, BootDirectoryName, L".disk_label.contentDetails");
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
      BootEntry->Name = AllocateCopyPool (sizeof (L"BOOTCAMP Windows"), L"BOOTCAMP Windows");
    }
  }

  if (BootEntry->Name == NULL) {
    BootEntry->Name = GetVolumeLabel (FileSystem);
    if (BootEntry->Name != NULL
      && (!StrCmp (BootEntry->Name, L"Recovery HD")
       || !StrCmp (BootEntry->Name, L"Recovery"))) {
      BootEntry->IsRecovery = TRUE;
      RecoveryBootName = InternalGetAppleRecoveryName (FileSystem, BootDirectoryName);
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

UINTN
OcFillBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL      *BootPolicy,
  IN  UINT32                          Policy,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFs,
  OUT OC_BOOT_ENTRY                   *BootEntry,
  OUT OC_BOOT_ENTRY                   *AlternateBootEntry OPTIONAL,
  IN  BOOLEAN                         IsLoadHandle
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

  Status = InternalCheckScanPolicy (Handle, SimpleFs, Policy);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCB: Skipping handle %p due to scan policy %x\n", Handle, Policy));
    return 0;
  }

  //
  // Do not do normal scanning on load handle.
  // We only allow recovery there.
  //
  if (!IsLoadHandle) {
    Status = BootPolicy->GetBootFileEx (
      Handle,
      BootPolicyOk,
      &DevicePath
      );
  } else {
    Status = EFI_UNSUPPORTED;
  }

  //
  // Detect recovery on load handle and on a partition without
  // any bootloader. Never allow alternate in this case.
  //
  if (EFI_ERROR (Status)) {
    Status = InternalGetRecoveryOsBooter (Handle, &DevicePath, TRUE);
    AlternateBootEntry = NULL;
    if (EFI_ERROR (Status)) {
      return Count;
    }
  }

  BootEntry->DevicePath = DevicePath;
  InternalSetBootEntryFlags (BootEntry);

  ++Count;

  if (AlternateBootEntry != NULL) {
    Status = BootPolicy->GetPathNameOnApfsRecovery (
      DevicePath,
      L"\\",
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
      Status = InternalGetRecoveryOsBooter (Handle, &DevicePath, FALSE);
      if (EFI_ERROR (Status)) {
        return Count;
      }
    }

    AlternateBootEntry->DevicePath = DevicePath;
    InternalSetBootEntryFlags (AlternateBootEntry);
    ++Count;
  }

  return Count;
}

EFI_STATUS
OcScanForBootEntries (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  OC_PICKER_CONTEXT           *Context,
  OUT OC_BOOT_ENTRY               **BootEntries,
  OUT UINTN                       *Count,
  OUT UINTN                       *AllocCount OPTIONAL,
  IN  BOOLEAN                     Describe
  )
{
  EFI_STATUS                       Status;
  UINTN                            NoHandles;
  EFI_HANDLE                       *Handles;
  UINTN                            Index;
  OC_BOOT_ENTRY                    *Entries;
  UINTN                            EntriesSize;
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

  if (!OcOverflowAddMulUN (NoHandles * 2, Context->CustomEntryCount, sizeof (OC_BOOT_ENTRY), &EntriesSize)) {
    Entries = AllocateZeroPool (EntriesSize);
  } else {
    Entries = NULL;
  }

  if (Entries == NULL) {
    FreePool (Handles);
    return EFI_OUT_OF_RESOURCES;
  }

  EntryIndex = 0;

  for (Index = 0; Index < NoHandles; ++Index) {
    Status = gBS->HandleProtocol (
      Handles[Index],
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID **) &SimpleFs
      );

    if (EFI_ERROR (Status)) {
      continue;
    }

    EntryCount = OcFillBootEntry (
      BootPolicy,
      Context->ScanPolicy,
      Handles[Index],
      SimpleFs,
      &Entries[EntryIndex],
      &Entries[EntryIndex+1],
      Context->ExcludeHandle == Handles[Index]
      );

    DEBUG_CODE_BEGIN ();
    VolumeLabel = GetVolumeLabel (SimpleFs);
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

      DevicePath = ConvertDevicePathToText (Entries[Index].DevicePath, FALSE, FALSE);
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

  for (Index = 0; Index < Context->CustomEntryCount; ++Index, ++EntryIndex) {
    Entries[EntryIndex].Name     = AsciiStrCopyToUnicode (Context->CustomEntries[Index].Name, 0);
    Entries[EntryIndex].PathName = AsciiStrCopyToUnicode (Context->CustomEntries[Index].Path, 0);
    if (Entries[EntryIndex].Name == NULL || Entries[EntryIndex].PathName == NULL) {
      OcFreeBootEntries (Entries, EntryIndex + 1);
      return EFI_OUT_OF_RESOURCES;
    }
    UnicodeUefiSlashes (Entries[EntryIndex].PathName);
    Entries[EntryIndex].IsCustom = TRUE;
  }

  *BootEntries = Entries;
  *Count       = EntryIndex;

  if (AllocCount != NULL) {
    *AllocCount = EntriesSize / sizeof (OC_BOOT_ENTRY);
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

EFI_STATUS
OcLoadBootEntry (
  IN  APPLE_BOOT_POLICY_PROTOCOL  *BootPolicy,
  IN  OC_PICKER_CONTEXT           *Context,
  IN  OC_BOOT_ENTRY               *BootEntry,
  IN  EFI_HANDLE                  ParentHandle
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 EntryHandle;
  INTERNAL_DMG_LOAD_CONTEXT  DmgLoadContext;

  Status = InternalLoadBootEntry (
    BootPolicy,
    Context,
    BootEntry,
    ParentHandle,
    &EntryHandle,
    &DmgLoadContext
    );
  if (!EFI_ERROR (Status)) {
    Status = Context->StartImage (BootEntry, EntryHandle, NULL, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCB: StartImage failed - %r\n", Status));
      //
      // Unload dmg if any.
      //
      InternalUnloadDmg (&DmgLoadContext);
    }
  } else {
    DEBUG ((DEBUG_ERROR, "OCB: LoadImage failed - %r\n", Status));
  }

  return Status;
}

EFI_STATUS
OcRunSimpleBootPicker (
  IN OC_PICKER_CONTEXT  *Context
  )
{
  EFI_STATUS                  Status;
  APPLE_BOOT_POLICY_PROTOCOL  *AppleBootPolicy;
  OC_BOOT_ENTRY               *Chosen;
  OC_BOOT_ENTRY               *Entries;
  OC_BOOT_ENTRY               *Entry;
  UINTN                       EntryCount;
  UINT32                      DefaultEntry;

  AppleBootPolicy = OcAppleBootPolicyInstallProtocol (FALSE);
  if (AppleBootPolicy == NULL) {
    DEBUG ((DEBUG_ERROR, "AppleBootPolicy locate failure\n"));
    return EFI_NOT_FOUND;
  }

  while (TRUE) {
    DEBUG ((DEBUG_INFO, "Performing OcScanForBootEntries...\n"));

    Status = OcScanForBootEntries (
      AppleBootPolicy,
      Context,
      &Entries,
      &EntryCount,
      NULL,
      TRUE
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OcScanForBootEntries failure - %r\n", Status));
      return Status;
    }

    if (EntryCount == 0) {
      DEBUG ((DEBUG_WARN, "OcScanForBootEntries has no entries\n"));
      return EFI_NOT_FOUND;
    }

    DEBUG ((DEBUG_INFO, "Performing OcShowSimpleBootMenu...\n"));

    DefaultEntry = 0;
    Entry = InternalGetDefaultBootEntry (Entries, EntryCount, Context->CustomBootGuid, Context->ExcludeHandle);
    if (Entry != NULL) {
      DefaultEntry = (UINT32)(Entry - Entries);
    }

    if (Context->ShowPicker) {
      Status = OcShowSimpleBootMenu (
        Entries,
        EntryCount,
        DefaultEntry,
        Context->TimeoutSeconds,
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

    Context->TimeoutSeconds = 0;

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
      Status = OcLoadBootEntry (
        AppleBootPolicy,
        Context,
        Chosen,
        gImageHandle
        );

      gBS->Stall (5000000);
    }

    OcFreeBootEntries (Entries, EntryCount);
  }
}
