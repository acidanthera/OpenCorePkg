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
  passed-in structures OC_PICKER_ENTRY and OC_PICKER_ENTRY change.

  WARNING: This protocol is currently undergoing active design.
**/
#define OC_BOOT_ENTRY_PROTOCOL_REVISION 1

/**
  Forward declaration of OC_BOOT_ENTRY_PROTOCOL structure.
**/
typedef struct OC_BOOT_ENTRY_PROTOCOL_ OC_BOOT_ENTRY_PROTOCOL;

/**
  Return list of OpenCore boot entries associated with filesystem.

  @param[in]  PickerContext     Picker context.
  @param[in]  Device            The handle of the device to scan. NULL is passed in to
                                request custom entries. All implementations must support a
                                NULL input value, but may immediately return EFI_NOT_FOUND
                                if they do not provide any custom entries.
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
(EFIAPI *OC_GET_BOOT_ENTRIES) (
  IN           OC_PICKER_CONTEXT        *PickerContext,
  IN     CONST EFI_HANDLE               Device,
     OUT       OC_PICKER_ENTRY          **Entries,
     OUT       UINTN                    *NumEntries
  );

/**
  Free list of OpenCore boot entries from previous call to OC_GET_BOOT_ENTRIES.

  @param[in]  Entries           List of boot entries, as returned by previous call.
                                Correct implementation of interface should additionally
                                zero this pointer before returning.
  @param[in]  NumEntries        The number of entries, as returned by previous call.
**/
typedef
VOID
(EFIAPI *OC_FREE_BOOT_ENTRIES) (
  IN           OC_PICKER_ENTRY          **Entries,
  IN           UINTN                    NumEntries
  );

/**
  The structure exposed by the OC_BOOT_ENTRY_PROTOCOL.
**/
struct OC_BOOT_ENTRY_PROTOCOL_ {
  UINTN                 Revision;
  OC_GET_BOOT_ENTRIES   GetBootEntries;
  OC_FREE_BOOT_ENTRIES  FreeBootEntries;
};

extern EFI_GUID gOcBootEntryProtocolGuid;

#endif // OC_BOOT_ENTRY_PROTOCOL_H
