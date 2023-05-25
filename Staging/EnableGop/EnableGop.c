/** @file
  Provide GOP on unsupported graphics cards on EFI-era MacPro and iMac.

  Copyright (c) 2022-2023, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>

#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Library/OcBootManagementLib.h>
#include <Library/OcConsoleLib.h>
#include <Library/OcDeviceMiscLib.h>

STATIC EFI_GET_MEMORY_SPACE_MAP  mOriginalGetMemorySpaceMap;

//
// Close to a very cut down OcLoadUefiOutputSupport.
//
STATIC
EFI_STATUS
LoadUefiOutputSupport (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = OcProvideConsoleGop (FALSE);
  if (EFI_ERROR (Status)) {
    //
    // If we've already got console GOP assume it's set up correctly, however enable
    // GopBurstMode as it can still provide a noticeable speed up (e.g. MP5,1 GT120).
    //
    if (Status == EFI_ALREADY_STARTED) {
      OcSetGopBurstMode ();
    }

    return Status;
  }

  OcSetConsoleResolution (
    0,
    0,
    0,
    FALSE
    );

  OcSetGopBurstMode ();

  if (FeaturePcdGet (PcdEnableGopDirect)) {
    OcUseDirectGop (-1);
  }

  OcSetupConsole (
    EfiConsoleControlScreenGraphics,
    OcConsoleRendererBuiltinGraphics,
    NULL,
    NULL,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    0,
    0
    );

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ProvideGop (
  VOID
  )
{
  OcUnlockAppleFirmwareUI ();

  return LoadUefiOutputSupport ();
}

//
// This memory map access happens twice during PlatformBdsPolicyBehavior, once
// in the equivalent of efi InitializeMemoryTest at the start of the function,
// and once during PlatformBdsDiagnostics, slightly later. The second call(s)
// (there is more than one code path, depending on the boot type) are after
// the default console has been connected, and therefore ideal for us.
//
EFI_STATUS
EFIAPI
WrappedGetMemorySpaceMap (
  OUT UINTN                            *NumberOfDescriptors,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  **MemorySpaceMap
  )
{
  STATIC UINTN  mGetMemorySpaceMapAccessCount = 0;

  mGetMemorySpaceMapAccessCount++;

  if (mGetMemorySpaceMapAccessCount == 2) {
    ProvideGop ();
  }

  return mOriginalGetMemorySpaceMap (
           NumberOfDescriptors,
           MemorySpaceMap
           );
}

STATIC
VOID
WrapGetMemorySpaceMap (
  VOID
  )
{
  mOriginalGetMemorySpaceMap = gDS->GetMemorySpaceMap;
  gDS->GetMemorySpaceMap     = WrappedGetMemorySpaceMap;

  gDS->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (gDS, gDS->Hdr.HeaderSize, &gDS->Hdr.CRC32);
}

//
// If we execute the entire console setup at Driver#### time then we have to forge UEFI, reload option ROMs
// and connect them, all at that point.
// This works on some systems but causes crashes others, or causes an empty picker with question mark folder.
// Current strategy is:
//  - Forge UEFI early; relatively easy: just do it immediately, as below, and insert this driver either
//    a) anywhere in main firmware volume, or b) in VBIOS anywhere before GOP driver which needs it
//  - Execute rest of payload after option roms have been loaded, _and_ after firmware has already connected
//    them (see WrapGetMemorySpaceMap strategy above).
// With this strategy we do not need to reload or reconnect any option ROMs, which is much more stable.
//
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  OcForgeUefiSupport (TRUE, TRUE);
  WrapGetMemorySpaceMap ();

  return EFI_SUCCESS;
}
