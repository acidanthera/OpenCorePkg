/** @file
  Copyright (C) 2013, dmazar. All rights reserved.
  Copyright (C) 2017, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "BootCompatInternal.h"

#include <IndustryStandard/AppleHibernate.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/OcFirmwareRuntime.h>

/**
  Helper function to call ExitBootServices that can handle outdated MapKey issues.

  @param[in]  ExitBootServices  ExitBootServices function pointer, optional.
  @param[in]  GetMemoryMap      GetMemoryMap function pointer, optional.
  @param[in]  ImageHandle       Image handle to call ExitBootServices on.
  @param[in]  MapKey            MapKey to call ExitBootServices on.

  @retval EFI_SUCCESS on success.
**/
STATIC
EFI_STATUS
ForceExitBootServices (
  IN EFI_HANDLE              ImageHandle,
  IN UINTN                   MapKey,
  IN EFI_EXIT_BOOT_SERVICES  ExitBootServices  OPTIONAL,
  IN EFI_GET_MEMORY_MAP      GetMemoryMap  OPTIONAL
  )
{
  EFI_STATUS               Status;
  EFI_MEMORY_DESCRIPTOR    *MemoryMap;
  UINTN                    MemoryMapSize;
  UINTN                    DescriptorSize;
  UINT32                   DescriptorVersion;

  if (ExitBootServices == NULL) {
    ExitBootServices = gBS->ExitBootServices;
  }

  if (GetMemoryMap == NULL) {
    GetMemoryMap = gBS->GetMemoryMap;
  }

  //
  // Firstly try the easy way.
  //
  Status = ExitBootServices (ImageHandle, MapKey);

  if (EFI_ERROR (Status)) {
    //
    // It is too late to free memory map here, but it does not matter, because boot.efi has an old one
    // and will freely use the memory.
    // It is technically forbidden to allocate pool memory here, but we should not hit this code
    // in the first place, and for older firmwares, where it was necessary (?), it worked just fine.
    //
    Status = GetCurrentMemoryMapAlloc (
      &MemoryMapSize,
      &MemoryMap,
      &MapKey,
      &DescriptorSize,
      &DescriptorVersion,
      GetMemoryMap,
      NULL
      );
    if (Status == EFI_SUCCESS) {
      //
      // We have the latest memory map and its key, try again!
      //
      Status = ExitBootServices (ImageHandle, MapKey);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OCABC: ExitBootServices failed twice - %r\n", Status));
      }
    } else {
      DEBUG ((DEBUG_WARN, "OCABC: Failed to get MapKey for ExitBootServices - %r\n", Status));
      Status = EFI_INVALID_PARAMETER;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCABC: Waiting 10 secs...\n"));
      gBS->Stall (SECONDS_TO_MICROSECONDS (10));
    }
  }

  return Status;
}

/**
  UEFI Boot Services StartImage override. Called to start an efi image.
  If this is boot.efi, then our overrides are enabled.
**/
STATIC
EFI_STATUS
EFIAPI
OcStartImage (
  IN     EFI_HANDLE  ImageHandle,
     OUT UINTN       *ExitDataSize,
     OUT CHAR16      **ExitData  OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *AppleLoadedImage;
  BOOT_COMPAT_CONTEXT         *BootCompat;

  BootCompat        = GetBootCompatContext ();
  AppleLoadedImage  = OcGetAppleBootLoadedImage (ImageHandle);

  //
  // Clear monitoring vars
  //
  BootCompat->ServiceState.MinAllocatedAddr = 0;
  BootCompat->ServiceState.MaxAllocatedAddr = 0;

  if (AppleLoadedImage != NULL) {
    //
    // Report about macOS being loaded.
    //
    ++BootCompat->ServiceState.AppleBootNestedCount;
    BootCompat->ServiceState.AppleHibernateWake = OcIsAppleHibernateWake ();
    BootCompat->ServiceState.AppleCustomSlide = OcCheckArgumentFromEnv (
      AppleLoadedImage,
      BootCompat->ServicePtrs.GetVariable,
      "slide=",
      L_STR_LEN ("slide=")
      );

    if (BootCompat->Settings.EnableAppleSmSlide) {
      // FIXME: Implement.
      //UnlockSlideSupportForSafeMode (
      //  (UINT8 *) AppleLoadedImage->ImageBase,
      //  AppleLoadedImage->ImageSize
      //  );
    }

    if (BootCompat->Settings.SetupAppleMap) {
      AppleMapPrepareBooterState (
        BootCompat,
        AppleLoadedImage,
        BootCompat->ServicePtrs.GetMemoryMap
        );
    }
  }

  Status = BootCompat->ServicePtrs.StartImage (
    ImageHandle,
    ExitDataSize,
    ExitData
    );

  if (AppleLoadedImage != NULL) {
    //
    // We failed but other operating systems should be loadable.
    //
    --BootCompat->ServiceState.AppleBootNestedCount;
  }

  return Status;
}

/**
  UEFI Boot Services AllocatePages override.
  Returns pages from free memory block to boot.efi for kernel boot image.
**/
STATIC
EFI_STATUS
EFIAPI
OcAllocatePages (
  IN     EFI_ALLOCATE_TYPE     Type,
  IN     EFI_MEMORY_TYPE       MemoryType,
  IN     UINTN                 NumberOfPages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    UpperAddr;
  BOOT_COMPAT_CONTEXT     *BootCompat;

  BootCompat = GetBootCompatContext ();

  Status = BootCompat->ServicePtrs.AllocatePages (
    Type,
    MemoryType,
    NumberOfPages,
    Memory
    );

  if (!EFI_ERROR (Status) && BootCompat->ServiceState.AppleBootNestedCount > 0) {
    if (Type == AllocateAddress && MemoryType == EfiLoaderData) {
      //
      // Called from boot.efi
      //
      UpperAddr = *Memory + EFI_PAGES_TO_SIZE (NumberOfPages);

      //
      // Store min and max mem: they can be used later to determine
      // start and end of kernel boot or hibernation images.
      //
      if (BootCompat->ServiceState.MinAllocatedAddr == 0
        || *Memory < BootCompat->ServiceState.MinAllocatedAddr) {
        BootCompat->ServiceState.MinAllocatedAddr = *Memory;
      }

      if (UpperAddr > BootCompat->ServiceState.MaxAllocatedAddr) {
        BootCompat->ServiceState.MaxAllocatedAddr = UpperAddr;
      }
    } else if (BootCompat->ServiceState.AppleHibernateWake
      && Type == AllocateAnyPages && MemoryType == EfiLoaderData
      && BootCompat->ServiceState.HibernateImageAddress == 0) {
      //
      // Called from boot.efi during hibernate wake,
      // first such allocation is for hibernate image
      //
      BootCompat->ServiceState.HibernateImageAddress = *Memory;
    }
  }

  return Status;
}

/**
  UEFI Boot Services GetMemoryMap override.
  Returns shrinked memory map as XNU can handle up to PMAP_MEMORY_REGIONS_SIZE (128) entries.
  Also applies any further memory map alterations as necessary.
**/
STATIC
EFI_STATUS
EFIAPI
OcGetMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
     OUT UINTN                  *MapKey,
     OUT UINTN                  *DescriptorSize,
     OUT UINT32                 *DescriptorVersion
  )
{
  EFI_STATUS            Status;
  BOOT_COMPAT_CONTEXT   *BootCompat;

  BootCompat = GetBootCompatContext ();

  Status = BootCompat->ServicePtrs.GetMemoryMap (
    MemoryMapSize,
    MemoryMap,
    MapKey,
    DescriptorSize,
    DescriptorVersion
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BootCompat->ServiceState.AppleBootNestedCount > 0) {
    if (BootCompat->Settings.ProtectCsmRegion) {
      // FIXME: Implement.
      //ProtectCsmRegion (
      //  *MemoryMapSize,
      //  MemoryMap,
      //  *DescriptorSize
      //  );
    }

    if (BootCompat->Settings.ShrinkMemoryMap) {
      ShrinkMemoryMap (
        MemoryMapSize,
        MemoryMap,
        *DescriptorSize
        );
    }

    //
    // Remember some descriptor size, since we will not have it later
    // during hibernate wake to be able to iterate memory map.
    //
    BootCompat->ServiceState.MemoryMapDescriptorSize = *DescriptorSize;
  }

  return Status;
}

/**
  UEFI Boot Services ExitBootServices override.
  Patches kernel entry point with jump to our KernelEntryPatchJumpBack().
**/
STATIC
EFI_STATUS
EFIAPI
OcExitBootServices (
  IN EFI_HANDLE  ImageHandle,
  IN UINTN       MapKey
  )
{
  EFI_STATUS               Status;
  BOOT_COMPAT_CONTEXT      *BootCompat;

  BootCompat = GetBootCompatContext ();

  //
  // For non-macOS operating systems return directly.
  //
  if (BootCompat->ServiceState.AppleBootNestedCount == 0) {
    return BootCompat->ServicePtrs.ExitBootServices (
      ImageHandle,
      MapKey
      );
  }

  //
  // We need hibernate image address for wake, check it quickly before
  // killing boot services to be able to print the error.
  //
  if (BootCompat->Settings.SetupAppleMap
    && BootCompat->ServiceState.AppleHibernateWake
    && BootCompat->ServiceState.HibernateImageAddress == 0) {
    DEBUG ((DEBUG_ERROR, "OCABC: Failed to find hibernate image address\n"));
    gBS->Stall (SECONDS_TO_MICROSECONDS (5));
    return EFI_INVALID_PARAMETER;
  }

  if (BootCompat->Settings.ForceExitBootServices) {
    Status = ForceExitBootServices (
      ImageHandle,
      MapKey,
      BootCompat->ServicePtrs.ExitBootServices,
      BootCompat->ServicePtrs.GetMemoryMap
      );
  } else {
    Status = BootCompat->ServicePtrs.ExitBootServices (
      ImageHandle,
      MapKey
      );
  }

  //
  // Abort on error or when we are not supposed to do extra mapping.
  //
  if (EFI_ERROR (Status) || !BootCompat->Settings.SetupAppleMap) {
    return Status;
  }

  if (!BootCompat->ServiceState.AppleHibernateWake) {
    AppleMapPrepareKernelJump (
      BootCompat,
      (UINTN) BootCompat->ServiceState.MinAllocatedAddr,
      FALSE
      );
  } else {
    AppleMapPrepareKernelJump (
      BootCompat,
      (UINTN) BootCompat->ServiceState.HibernateImageAddress,
      TRUE
      );
  }

  return Status;
}

/**
  UEFI Runtime Services SetVirtualAddressMap override.
  Fixes virtualizing of RT services.
**/
STATIC
EFI_STATUS
EFIAPI
OcSetVirtualAddressMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap
  )
{
  EFI_STATUS             Status;
  BOOT_COMPAT_CONTEXT    *BootCompat;

  BootCompat = GetBootCompatContext ();

  //
  // This is the time for us to remove our hacks.
  // Make SetVirtualAddressMap useable once again.
  // We do not need to recover BS, since they already are invalid.
  //
  gRT->SetVirtualAddressMap = BootCompat->ServicePtrs.SetVirtualAddressMap;
  gRT->Hdr.CRC32 = 0;
  gRT->Hdr.CRC32 = CalculateCrc32 (gRT, gRT->Hdr.HeaderSize);

  //
  // For non-macOS operating systems return directly.
  // Also do nothing for custom mapping.
  //
  if (BootCompat->ServiceState.AppleBootNestedCount == 0
    || !BootCompat->Settings.SetupAppleMap) {
    Status = gRT->SetVirtualAddressMap (
      MemoryMapSize,
      DescriptorSize,
      DescriptorVersion,
      MemoryMap
      );
  } else {
    Status = AppleMapPrepareVmState (
      BootCompat,
      MemoryMapSize,
      DescriptorSize,
      DescriptorVersion,
      MemoryMap
      );
  }

  return Status;
}

/**
  UEFI Runtime Services GetVariable override.
  Used to return customised values for boot-args and csr-active-config variables.
**/
STATIC
EFI_STATUS
EFIAPI
OcGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes OPTIONAL,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data
  )
{
  EFI_STATUS             Status;
  BOOT_COMPAT_CONTEXT    *BootCompat;

  BootCompat = GetBootCompatContext ();

  if (BootCompat->Settings.SetupAppleSlide) {
    // FIXME: Implement
  }

  Status = BootCompat->ServicePtrs.GetVariable (
    VariableName,
    VendorGuid,
    Attributes,
    DataSize,
    Data
    );

  return Status;
}

/**
  UEFI Runtime Services GetVariable override event handler.
  We do not override GetVariable ourselves but let our runtime do that.

  @param[in]  Event    Event handle.
  @param[in]  Context  Services pointers context.
**/
STATIC
VOID
EFIAPI
SetGetVariableHookHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                    Status;
  OC_FIRMWARE_RUNTIME_PROTOCOL  *FwRuntime;
  UEFI_SERVICES_POINTERS        *ServicePtrs;

  ServicePtrs = (UEFI_SERVICES_POINTERS *) Context;

  if (ServicePtrs->GetVariable == NULL) {
    Status = gBS->LocateProtocol (
      &gOcFirmwareRuntimeProtocolGuid,
      NULL,
      (VOID **) &FwRuntime
      );

    if (!EFI_ERROR (Status) && FwRuntime->Revision == OC_FIRMWARE_RUNTIME_REVISION) {
      FwRuntime->OnGetVariable (OcGetVariable, &ServicePtrs->GetVariable);
    }
  }
}

VOID
InstallServiceOverrides (
  IN OUT BOOT_COMPAT_CONTEXT  *BootCompat
  )
{
  EFI_STATUS              Status;
  VOID                    *Registration;
  UEFI_SERVICES_POINTERS  *ServicePtrs;

  ServicePtrs = &BootCompat->ServicePtrs;

  ServicePtrs->AllocatePages        = gBS->AllocatePages;
  ServicePtrs->GetMemoryMap         = gBS->GetMemoryMap;
  ServicePtrs->ExitBootServices     = gBS->ExitBootServices;
  ServicePtrs->StartImage           = gBS->StartImage;
  ServicePtrs->SetVirtualAddressMap = gRT->SetVirtualAddressMap;

  gBS->AllocatePages        = OcAllocatePages;
  gBS->GetMemoryMap         = OcGetMemoryMap;
  gBS->ExitBootServices     = OcExitBootServices;
  gBS->StartImage           = OcStartImage;
  gRT->SetVirtualAddressMap = OcSetVirtualAddressMap;

  gBS->Hdr.CRC32 = 0;
  gBS->Hdr.CRC32 = CalculateCrc32 (gBS, gBS->Hdr.HeaderSize);

  gRT->Hdr.CRC32 = 0;
  gRT->Hdr.CRC32 = CalculateCrc32 (gRT, gRT->Hdr.HeaderSize);

  //
  // Allocate memory pool if needed.
  //
  if (BootCompat->Settings.SetupAppleMap) {
    AppleMapPrepareMemoryPool (
      BootCompat
      );
  }

  //
  // Update GetVariable handle with the help of external runtime services.
  //
  SetGetVariableHookHandler (NULL, ServicePtrs);

  if (ServicePtrs->GetVariable != NULL) {
    return;
  }

  Status = gBS->CreateEvent (
    EVT_NOTIFY_SIGNAL,
    TPL_NOTIFY,
    SetGetVariableHookHandler,
    ServicePtrs,
    &BootCompat->ServiceState.GetVariableEvent
    );

  if (!EFI_ERROR (Status)) {
    Status = gBS->RegisterProtocolNotify (
      &gOcFirmwareRuntimeProtocolGuid,
      BootCompat->ServiceState.GetVariableEvent,
      &Registration
      );

    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (&BootCompat->ServiceState.GetVariableEvent);
    }
  }
}
