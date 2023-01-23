/** @file
  Copyright (c) 2020, joevt. All rights reserved.
  Copyright (C) 2021-2023, vit9696, mikebeaton. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <PiDxe.h>

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC
EFI_STATUS
EFIAPI
OcCreateEventEx (
  IN       UINT32            Type,
  IN       EFI_TPL           NotifyTpl,
  IN       EFI_EVENT_NOTIFY  NotifyFunction OPTIONAL,
  IN CONST VOID              *NotifyContext OPTIONAL,
  IN CONST EFI_GUID          *EventGroup    OPTIONAL,
  OUT      EFI_EVENT         *Event
  )
{
  if ((Type == EVT_NOTIFY_SIGNAL) && CompareGuid (EventGroup, &gEfiEventExitBootServicesGuid)) {
    return gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  NotifyTpl,
                  NotifyFunction,
                  (VOID *)NotifyContext,
                  Event
                  );
  }

  if ((Type == EVT_NOTIFY_SIGNAL) && CompareGuid (EventGroup, &gEfiEventVirtualAddressChangeGuid)) {
    return gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  NotifyTpl,
                  NotifyFunction,
                  (VOID *)NotifyContext,
                  Event
                  );
  }

  gBS->CreateEvent (
         Type,
         NotifyTpl,
         NotifyFunction,
         (VOID *)NotifyContext,
         Event
         );
  return EFI_SUCCESS;
}

//
// The Trash strategy relies on old Apple firmware allocating gBS and gDS consecutively.
// This layout is directly inherited from standard edk EFI code.
// The strategy trashes the DXE_SERVICES_SIGNATURE value in gDS->Hdr.Signature, which
// happily is only used when the memory is being loaded (when we check for references
// to DXE_SERVICES_SIGNATURE throughout the edk code).
// For the Trash strategy to work, we are required to trash exactly that QWORD of memory,
// but in the targeted firmware we can confirm that it is harmless to do so before proceeding.
//
EFI_STATUS
OcForgeUefiSupport (
  IN BOOLEAN  Forge,
  IN BOOLEAN  Trash
  )
{
  EFI_BOOT_SERVICES  *NewBS;

  DEBUG ((
    DEBUG_INFO,
    "OCDM: Found 0x%X/0x%X UEFI version (%u bytes, %u %a to %u) gST %p gBS %p gBS->CreateEventEx %p &gBS %p\n",
    gST->Hdr.Revision,
    gBS->Hdr.Revision,
    gBS->Hdr.HeaderSize,
    Forge,
    Trash ? "trashing" : "rebuilding",
    (UINT32)sizeof (EFI_BOOT_SERVICES),
    gST,
    gBS,
    gBS->CreateEventEx,
    &gBS
    ));

  if (!Forge) {
    return EFI_SUCCESS;
  }

  //
  // Already too new.
  // This check will replace any earlier forge to 2.0 <= UEFI < 2.3.
  // This is desirable in some cases and harmless in others.
  //
  if (gST->Hdr.Revision >= EFI_2_30_SYSTEM_TABLE_REVISION) {
    return EFI_ALREADY_STARTED;
  }

  if (gBS->Hdr.HeaderSize > OFFSET_OF (EFI_BOOT_SERVICES, CreateEventEx)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Trash) {
    if ((VOID *)&gBS->CreateEventEx != (VOID *)gDS) {
      DEBUG ((
        DEBUG_WARN,
        "OCDM: Aborting trash strategy, gDS does not follow gBS\n"
        ));
      return EFI_UNSUPPORTED;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCDM: Trashing gDS->Hdr.Signature with gBS->CreateEventEx\n"
      ));
    NewBS = gBS;
  } else {
    NewBS = AllocateZeroPool (sizeof (EFI_BOOT_SERVICES));
    if (NewBS == NULL) {
      DEBUG ((DEBUG_INFO, "OCDM: Failed to allocate BS copy\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (NewBS, gBS, gBS->Hdr.HeaderSize);
  }

  NewBS->CreateEventEx  = OcCreateEventEx;
  NewBS->Hdr.HeaderSize = sizeof (EFI_BOOT_SERVICES);
  NewBS->Hdr.Revision   = EFI_2_30_SYSTEM_TABLE_REVISION;
  NewBS->Hdr.CRC32      = 0;
  NewBS->Hdr.CRC32      = CalculateCrc32 (NewBS, NewBS->Hdr.HeaderSize);
  gBS                   = NewBS;

  gST->BootServices = NewBS;
  gST->Hdr.Revision = EFI_2_30_SYSTEM_TABLE_REVISION;
  gST->Hdr.CRC32    = 0;
  gST->Hdr.CRC32    = CalculateCrc32 (gST, gST->Hdr.HeaderSize);

  if (Trash) {
    gDS->Hdr.CRC32 = 0;
    gDS->Hdr.CRC32 = CalculateCrc32 (gDS, gDS->Hdr.HeaderSize);
  }

  return EFI_SUCCESS;
}
