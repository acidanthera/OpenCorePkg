/** @file
  Boot Entry Protocol.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "BootEntryProtocolInternal.h"
#include "BootManagementInternal.h"

#include <Protocol/OcBootEntry.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcDebugLogLib.h>

VOID
LocateBootEntryProtocolHandles (
  IN OUT EFI_HANDLE        **EntryProtocolHandles,
  IN OUT UINTN             *EntryProtocolHandleCount
  )
{
  EFI_STATUS        Status;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gOcBootEntryProtocolGuid,
    NULL,
    EntryProtocolHandleCount,
    EntryProtocolHandles
    );

  if (EFI_ERROR (Status)) {
    //
    // No loaded drivers is fine
    //
    if (Status != EFI_NOT_FOUND) {
      DEBUG ((DEBUG_ERROR, "BEP: Error locating driver handles - %r\n", Status));
    }

    *EntryProtocolHandleCount = 0;
    *EntryProtocolHandles = NULL;
  }
}

VOID
FreeBootEntryProtocolHandles (
  EFI_HANDLE        **EntryProtocolHandles
  )
{
  if (*EntryProtocolHandles == NULL) {
    return;
  }

  FreePool (*EntryProtocolHandles);
  *EntryProtocolHandles = NULL;
}

EFI_STATUS
AddEntriesFromBootEntryProtocol (
  IN OUT OC_BOOT_CONTEXT      *BootContext,
  IN OUT OC_BOOT_FILESYSTEM   *FileSystem,
  IN     EFI_HANDLE           *EntryProtocolHandles,
  IN     UINTN                EntryProtocolHandleCount,
  IN     CONST CHAR16         *DefaultEntryId,           OPTIONAL
  IN     CONST BOOLEAN        CreateDefault
  )
{
  EFI_STATUS                    ReturnStatus;
  EFI_STATUS                    Status;
  UINTN                         Index;
  UINTN                         EntryIndex;
  OC_BOOT_ENTRY_PROTOCOL        *BootEntryProtocol;
  OC_PICKER_ENTRY               *Entries;
  UINTN                         NumEntries;

  DEBUG_CODE_BEGIN ();
  if (CreateDefault) {
    ASSERT ((DefaultEntryId != NULL));
  }
  DEBUG_CODE_END ();

  ReturnStatus = EFI_NOT_FOUND;

  for (Index = 0; Index < EntryProtocolHandleCount; ++Index) {
    //
    // Previously marked as invalid protocol revision.
    //
    if (EntryProtocolHandles[Index] == NULL) {
      continue;
    }

    Status = gBS->HandleProtocol (
      EntryProtocolHandles[Index],
      &gOcBootEntryProtocolGuid,
      (VOID **) &BootEntryProtocol
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "BEP: HandleProtocol failed - %r\n", Status));
      continue;
    }

    if (BootEntryProtocol->Revision != OC_BOOT_ENTRY_PROTOCOL_REVISION) {
      DEBUG ((
        DEBUG_ERROR,
        "BEP: Invalid revision %u (!= %u) in loaded driver\n",
        BootEntryProtocol->Revision,
        OC_BOOT_ENTRY_PROTOCOL_REVISION
        ));
      EntryProtocolHandles[Index] = NULL;
      continue;
    }

    Status = BootEntryProtocol->GetBootEntries (
      BootContext->PickerContext,
      FileSystem->Handle == OC_CUSTOM_FS_HANDLE ? NULL : FileSystem->Handle,
      &Entries,
      &NumEntries
      );

    if (EFI_ERROR (Status)) {
      //
      // No entries for any given driver on any given filesystem is normal.
      //
      if (Status != EFI_NOT_FOUND) {
        DEBUG ((DEBUG_WARN, "BEP: Unable to fetch boot entries - %r\n", Status));
      }
      continue;
    }

    for (EntryIndex = 0; EntryIndex < NumEntries; EntryIndex++) {
      if (Entries[EntryIndex].Id == NULL) {
        DEBUG ((DEBUG_WARN, "BEP: Entry->Id is required, ignoring entry.\n"));
      }
      if (DefaultEntryId == NULL ||
        (MixedStrCmp (DefaultEntryId, Entries[EntryIndex].Id) == 0) == CreateDefault) {
        Status = InternalAddBootEntryFromCustomEntry (
          BootContext,
          FileSystem,
          &Entries[EntryIndex],
          TRUE
          );
          
        if (EFI_ERROR (Status)) {
          //
          // EFI_UNSUPPORTED is auxiliary entry when HideAuxiliary=true.
          //
          if (Status != EFI_UNSUPPORTED) {
            DEBUG ((DEBUG_WARN, "BEP: Error adding entries - %r\n", Status));
            break;
          }
        } else {
          ReturnStatus = EFI_SUCCESS;
        
          //
          // Stop searching after first match for default entry. Possible additional
          // matches, e.g. older versions of Linux kernel, are normal.
          //
          if (CreateDefault) {
            break;
          }
        }
      } else {
        //
        // Create remaining matches after skipping first match for pre-created entry.
        //
        if (!CreateDefault) {
          DefaultEntryId = NULL;
        }
      }
    }
    
    BootEntryProtocol->FreeBootEntries (
      &Entries,
      NumEntries
      );

    //
    // If not found, keep hunting for default entry on other installed drivers.
    //
    if (CreateDefault) {
      if (ReturnStatus == EFI_NOT_FOUND) {
        continue;
      }
      break;
    }

    //
    // On other error adding entry (should not fail), abort.
    //
    if (EFI_ERROR (Status) && Status != EFI_UNSUPPORTED) {
      break;
    }
  }

  return ReturnStatus;
}
