/** @file
  OpenCore Boot Entry Protocol.

  Copyright (C) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_BOOT_ENTRY_PROTOCOL_H
#define OC_BOOT_ENTRY_PROTOCOL_H

#include <Uefi.h>
#include <Library/OcBootManagementLib.h>

#include <Protocol/SimpleFileSystem.h>

/**
  8604716E-ADD4-45B4-8495-08E36D497F4F
**/
#define OC_BOOT_ENTRY_PROTOCOL_GUID                                                 \
  {                                                                                 \
    0x8604716E, 0xADD4, 0x45B4, { 0x84, 0x95, 0x08, 0xE3, 0x6D, 0x49, 0x7F, 0x4F }  \
  }

/**
  Currently supported OC_BOOT_ENTRY_PROTOCOL protocol revision.
  Needs to be changed every time the contract changes, including when
  passed-in structures OC_PICKER_CONTEXT and OC_PICKER_ENTRY change.

  WARNING: This protocol is currently undergoing active design.
**/
#define OC_BOOT_ENTRY_PROTOCOL_REVISION  4

/**
  Forward declaration of OC_BOOT_ENTRY_PROTOCOL structure.
**/
typedef struct OC_BOOT_ENTRY_PROTOCOL_ OC_BOOT_ENTRY_PROTOCOL;

/**
  Return list of OpenCore boot entries associated with filesystem.

  @param[in]  PickerContext     Picker context.
  @param[in]  Device            The handle of the device to scan. NULL is passed in to request
                                custom entries (e.g. system actions), non-NULL for entries found
                                by scanning a given device. All implementations must support NULL
                                and non-NULL values, but may immediately return EFI_NOT_FOUND
                                if they do not provide entries of the corresponding type.
  @param[out] BootEntries       List of boot entries associated with the filesystem.
                                On EFI_SUCCESS BootEntries must be freed by the caller
                                with FreePool after use, and each individual boot entry
                                should eventually be freed by FreeBootEntry (as for boot
                                entries created within OC itself).
                                Does not point to allocated memory on return, if any status
                                other than EFI_SUCCESS was returned.
  @param[out] NumEntries        The number of entries returned in the BootEntries list.
                                If any status other than EFI_SUCCESS was returned, this
                                value may not have been initialised and should be ignored
                                by the caller.

  @retval EFI_SUCCESS           At least one matching entry was found, and the list and
                                count of boot entries has been returned.
  @retval EFI_NOT_FOUND         No matching boot entries were found.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failure.
  @retval other                 An error returned by a sub-operation.
**/
typedef
EFI_STATUS
(EFIAPI *OC_BOOT_ENTRY_PROTOCOL_GET_ENTRIES)(
  IN OUT         OC_PICKER_CONTEXT        *PickerContext,
  IN     CONST EFI_HANDLE               Device OPTIONAL,
  OUT       OC_PICKER_ENTRY          **Entries,
  OUT       UINTN                    *NumEntries
  );

/**
  Free list of OpenCore boot entries from previous call to OC_BOOT_ENTRY_PROTOCOL_GET_ENTRIES.

  @param[in]  Entries           List of boot entries, as returned by previous call.
                                Correct implementation of interface should additionally
                                zero this pointer before returning.
  @param[in]  NumEntries        The number of entries, as returned by previous call.
**/
typedef
VOID
(EFIAPI *OC_BOOT_ENTRY_PROTOCOL_FREE_ENTRIES)(
  IN           OC_PICKER_ENTRY          **Entries,
  IN           UINTN                    NumEntries
  );

/**
  Detect hotkeys for boot entry protocol. If non-NULL is returned, it is the
  entry ID of the entry from this protocol which should be started immediately
  with no picker shown, similarly to a default boot entry. If protocol does not
  in fact return an entry with this ID, then picker with all auxiliary entries
  will be shown.

  @param[in]  PickerContext     Picker context.
  @param[in]  NumKeys           Number of keys held down. Recommended to check Keys and
                                NumKeys using OcKeyMapHasKey.
  @param[out] Modifiers         Keyboard modifiers.
  @param[out] Keys              Keys held down. Recommended to check Keys and NumKeys
                                using OcKeyMapHasKey.

  @retval ID of entry to start immediately, or NULL.
**/
typedef
CHAR8 *
(EFIAPI *OC_BOOT_ENTRY_PROTOCOL_CHECK_HOTKEYS)(
  IN OUT OC_PICKER_CONTEXT  *Context,
  IN UINTN               NumKeys,
  IN APPLE_MODIFIER_MAP  Modifiers,
  IN APPLE_KEY_CODE      *Keys
  );

/**
  Action to perform for each boot entry protocol instance during OcConsumeBootEntryProtocol.

  @param[in,out] PickerContext              Picker context.
  @param[in]     BootEntryProtocolHandle    Boot entry protocol handle.
  @param[in]     BootEntryProtocol          Boot entry protocol.
  @param[in]     Context                    Action context.

  @retval TRUE, proceed to next protocol instance.
  @retval FALSE, do not proceed to next protocol instance.
**/
typedef
BOOLEAN
(EFIAPI *OC_CONSUME_ENTRY_PROTOCOL_ACTION)(
  IN OUT OC_PICKER_CONTEXT  *PickerContext,
  IN EFI_HANDLE             BootEntryProtocolHandle,
  IN OC_BOOT_ENTRY_PROTOCOL *BootEntryProtocol,
  IN VOID                   *Context
  );

/**
  Consume boot entry protocol.

  @param[in,out] PickerContext              Picker context.
  @param[in]     EntryProtocolHandles       Boot entry protocol handles, or NULL if none.
  @param[in]     EntryProtocolHandleCount   Count of boot entry protocol handles.
  @param[in]     Action                     Action to perform for each protocol instance.
  @param[in]     Context                    Context.
**/
VOID
OcConsumeBootEntryProtocol (
  IN OUT  OC_PICKER_CONTEXT                 *PickerContext,
  IN      EFI_HANDLE                        *EntryProtocolHandles,
  IN      UINTN                             EntryProtocolHandleCount,
  IN      OC_CONSUME_ENTRY_PROTOCOL_ACTION  Action,
  IN      VOID                              *Context
  );

/**
  The structure exposed by OC_BOOT_ENTRY_PROTOCOL.
**/
struct OC_BOOT_ENTRY_PROTOCOL_ {
  //
  // Protocol revision.
  //
  UINTN                                   Revision;
  //
  // Get boot entries. Optional.
  //
  OC_BOOT_ENTRY_PROTOCOL_GET_ENTRIES      GetBootEntries;
  //
  // Free boot entries.
  // Optional, NULL may set here by a protocol provider if the returned
  // entries are in statically allocated memory and do not need freeing.
  //
  OC_BOOT_ENTRY_PROTOCOL_FREE_ENTRIES     FreeBootEntries;
  //
  // Hotkey action. Optional.
  //
  OC_BOOT_ENTRY_PROTOCOL_CHECK_HOTKEYS    CheckHotKeys;
};

extern EFI_GUID  gOcBootEntryProtocolGuid;

#endif // OC_BOOT_ENTRY_PROTOCOL_H
