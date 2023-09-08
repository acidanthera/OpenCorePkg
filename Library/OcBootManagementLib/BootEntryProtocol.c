/** @file
  Boot Entry Protocol.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "BootManagementInternal.h"

#include <Protocol/OcBootEntry.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcDebugLogLib.h>

typedef struct {
  EFI_STATUS            ReturnStatus;
  OC_BOOT_CONTEXT       *BootContext;
  OC_BOOT_FILESYSTEM    *FileSystem;
  BOOLEAN               CreateDefault;
  BOOLEAN               CreateForHotKey;
  CONST VOID            *DefaultEntryId;
} BEP_ADD_ENTRIES_CONTEXT;

VOID
OcLocateBootEntryProtocolHandles (
  IN OUT EFI_HANDLE  **EntryProtocolHandles,
  IN OUT UINTN       *EntryProtocolHandleCount
  )
{
  EFI_STATUS  Status;

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
    *EntryProtocolHandles     = NULL;
  }
}

VOID
OcFreeBootEntryProtocolHandles (
  EFI_HANDLE  **EntryProtocolHandles
  )
{
  if (*EntryProtocolHandles == NULL) {
    return;
  }

  FreePool (*EntryProtocolHandles);
  *EntryProtocolHandles = NULL;
}

STATIC
BOOLEAN
EFIAPI
InternalAddEntriesFromProtocol (
  IN OUT  OC_PICKER_CONTEXT       *PickerContext,
  IN      EFI_HANDLE              BootEntryProtocolHandle,
  IN      OC_BOOT_ENTRY_PROTOCOL  *BootEntryProtocol,
  IN      VOID                    *Context
  )
{
  EFI_STATUS               Status;
  UINTN                    EntryIndex;
  OC_PICKER_ENTRY          *Entries;
  UINTN                    NumEntries;
  BEP_ADD_ENTRIES_CONTEXT  *AddEntriesContext;

  ASSERT (Context != NULL);
  AddEntriesContext = Context;

  ASSERT (!AddEntriesContext->CreateForHotKey || AddEntriesContext->CreateDefault);

  Status = EFI_NOT_FOUND;
  if (BootEntryProtocol->GetBootEntries) {
    Status = BootEntryProtocol->GetBootEntries (
                                  PickerContext,
                                  ((AddEntriesContext->FileSystem->Handle == OC_CUSTOM_FS_HANDLE) ? NULL : AddEntriesContext->FileSystem->Handle),
                                  &Entries,
                                  &NumEntries
                                  );
  }

  if (EFI_ERROR (Status)) {
    //
    // No entries for any given driver on any given filesystem is normal.
    //
    if (Status != EFI_NOT_FOUND) {
      DEBUG ((DEBUG_WARN, "BEP: Unable to fetch boot entries - %r\n", Status));
    }

    return TRUE;
  }

  for (EntryIndex = 0; EntryIndex < NumEntries; EntryIndex++) {
    if (Entries[EntryIndex].Id == NULL) {
      DEBUG ((DEBUG_WARN, "BEP: Entry->Id is required, ignoring entry.\n"));
    }

    if ((AddEntriesContext->DefaultEntryId == NULL) ||
        (((AddEntriesContext->CreateForHotKey
          ? AsciiStrCmp (AddEntriesContext->DefaultEntryId, Entries[EntryIndex].Id)
          : MixedStrCmp (AddEntriesContext->DefaultEntryId, Entries[EntryIndex].Id)) == 0) == AddEntriesContext->CreateDefault))
    {
      Status = InternalAddBootEntryFromCustomEntry (
                 AddEntriesContext->BootContext,
                 AddEntriesContext->FileSystem,
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
        AddEntriesContext->ReturnStatus = EFI_SUCCESS;

        //
        // Stop searching after first match for default entry. Possible additional
        // matches, e.g. older versions of Linux kernel, are normal.
        //
        if (AddEntriesContext->CreateDefault) {
          return FALSE;
        }
      }
    } else {
      //
      // Create remaining matches after skipping first match for pre-created entry.
      //
      if (!AddEntriesContext->CreateDefault) {
        AddEntriesContext->DefaultEntryId = NULL;
      }
    }
  }

  if (BootEntryProtocol->FreeBootEntries) {
    BootEntryProtocol->FreeBootEntries (
                         &Entries,
                         NumEntries
                         );
  }

  //
  // If not found, keep hunting for default entry on other installed drivers.
  //
  if (AddEntriesContext->CreateDefault) {
    if (AddEntriesContext->ReturnStatus == EFI_NOT_FOUND) {
      return TRUE;
    }

    return FALSE;
  }

  //
  // On other error adding entry, which should not fail, abort.
  // Context->ReturnStatus reflects whether any entry was added.
  //
  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    return FALSE;
  }

  return TRUE;
}

VOID
OcConsumeBootEntryProtocol (
  IN OUT  OC_PICKER_CONTEXT                 *PickerContext,
  IN      EFI_HANDLE                        *EntryProtocolHandles,
  IN      UINTN                             EntryProtocolHandleCount,
  IN      OC_CONSUME_ENTRY_PROTOCOL_ACTION  Action,
  IN      VOID                              *Context
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  OC_BOOT_ENTRY_PROTOCOL  *BootEntryProtocol;

  ASSERT (EntryProtocolHandleCount == 0 || EntryProtocolHandles != NULL);

  for (Index = 0; Index < EntryProtocolHandleCount; ++Index) {
    //
    // Previously marked as invalid.
    //
    if (EntryProtocolHandles[Index] == NULL) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    EntryProtocolHandles[Index],
                    &gOcBootEntryProtocolGuid,
                    (VOID **)&BootEntryProtocol
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "BEP: HandleProtocol failed - %r\n", Status));
      EntryProtocolHandles[Index] = NULL;
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

    if (!Action (PickerContext, EntryProtocolHandles[Index], BootEntryProtocol, Context)) {
      break;
    }
  }
}

EFI_STATUS
OcAddEntriesFromBootEntryProtocol (
  IN OUT OC_BOOT_CONTEXT *BootContext,
  IN OUT OC_BOOT_FILESYSTEM *FileSystem,
  IN     EFI_HANDLE *EntryProtocolHandles,
  IN     UINTN EntryProtocolHandleCount,
  IN     CONST VOID *DefaultEntryId, OPTIONAL
  IN     BOOLEAN        CreateDefault,
  IN     BOOLEAN        CreateForHotKey
  )
{
  BEP_ADD_ENTRIES_CONTEXT  AddEntriesContext;

  ASSERT (!CreateDefault || (DefaultEntryId != NULL));

  AddEntriesContext.ReturnStatus    = EFI_NOT_FOUND;
  AddEntriesContext.BootContext     = BootContext;
  AddEntriesContext.FileSystem      = FileSystem;
  AddEntriesContext.CreateDefault   = CreateDefault;
  AddEntriesContext.CreateForHotKey = CreateForHotKey;
  AddEntriesContext.DefaultEntryId  = DefaultEntryId;

  OcConsumeBootEntryProtocol (
    BootContext->PickerContext,
    EntryProtocolHandles,
    EntryProtocolHandleCount,
    InternalAddEntriesFromProtocol,
    &AddEntriesContext
    );

  return AddEntriesContext.ReturnStatus;
}
