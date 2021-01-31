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

#include <Guid/AppleVariable.h>
#include <Guid/OcVariable.h>

#include <IndustryStandard/AppleHibernate.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcOSInfoLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/OcFirmwareRuntime.h>
#include <Protocol/VMwareMac.h>

/**
  Helper function to mark OpenRuntime as executable with proper permissions.

  @param[in]  BootCompat  Boot compatibility context.
**/
STATIC
VOID
FixRuntimeAttributes (
  IN BOOT_COMPAT_CONTEXT     *BootCompat,
  IN UINT32                  Type
  )
{
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    Address;
  UINTN                   Pages;

  if (Type != EfiRuntimeServicesCode && Type != EfiRuntimeServicesData) {
    return;
  }

  if (BootCompat->Settings.SyncRuntimePermissions && BootCompat->ServiceState.FwRuntime != NULL) {
    //
    // Be very careful of recursion here, who knows what the firmware can call.
    //
    BootCompat->Settings.SyncRuntimePermissions = FALSE;

    Status = BootCompat->ServiceState.FwRuntime->GetExecArea (&Address, &Pages);

    if (!EFI_ERROR (Status)) {
      OcRebuildAttributes (Address, BootCompat->ServicePtrs.GetMemoryMap);
    }

    //
    // Permit syncing runtime permissions again.
    //
    BootCompat->Settings.SyncRuntimePermissions = TRUE;
  }
}

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
    // in the first place, and for older firmware, where it was necessary (?), it worked just fine.
    //
    Status = OcGetCurrentMemoryMapAlloc (
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
  Protect regions in memory map.

  @param[in,out]  MemoryMapSize      Memory map size in bytes, updated on shrink.
  @param[in,out]  MemoryMap          Memory map to shrink.
  @param[in]      DescriptorSize     Memory map descriptor size in bytes.
**/
STATIC
VOID
ProtectMemoryRegions (
  IN     UINTN                  MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize
  )
{
  UINTN                   NumEntries;
  UINTN                   Index;
  EFI_MEMORY_DESCRIPTOR   *Desc;
  UINTN                   PhysicalEnd;

  //
  // AMI CSM module allocates up to two regions for legacy video output.
  // 1. For PMM and EBDA areas.
  //    On Ivy Bridge and below it ends at 0xA0000-0x1000-0x1 and has EfiBootServicesCode type.
  //    On Haswell and above it is allocated below 0xA0000 address with the same type.
  // 2. For Intel RC S3 reserved area, fixed from 0x9F000 to 0x9FFFF.
  //    On Sandy Bridge and below it is not present in memory map.
  //    On Ivy Bridge and newer it is present as EfiRuntimeServicesData.
  //    Starting from at least SkyLake it is present as EfiReservedMemoryType.
  //
  // Prior to AptioMemoryFix EfiRuntimeServicesData could have been relocated by boot.efi,
  // and the 2nd region could have been overwritten by the kernel. Now it is no longer the
  // case, and only the 1st region may need special handling.
  // For the 1st region there appear to be (unconfirmed) reports that it may still be accessed
  // after waking from sleep. This does not seem to be valid according to AMI code, but we still
  // protect it in case such systems really exist.
  //
  // Initially researched and fixed on GIGABYTE boards by Slice.
  //

  Desc       = MemoryMap;
  NumEntries = MemoryMapSize / DescriptorSize;

  for (Index = 0; Index < NumEntries; ++Index) {
    if (Desc->NumberOfPages > 0 && Desc->Type == EfiBootServicesData) {
      ASSERT (LAST_DESCRIPTOR_ADDR (Desc) < MAX_UINTN);
      PhysicalEnd = (UINTN)LAST_DESCRIPTOR_ADDR (Desc) + 1;

      if (PhysicalEnd >= 0x9E000 && PhysicalEnd < 0xA0000) {
        Desc->Type = EfiACPIMemoryNVS;
        break;
      }
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }

  //
  // Some types of firmware may leave MMIO regions as reserved memory with runtime flag,
  // which will not get mapped by macOS kernel. This will cause boot failures due
  // to such firmware accessing these regions at runtime for NVRAM support.
  // REF: https://github.com/acidanthera/bugtracker/issues/791#issuecomment-608959387
  //

  Desc = MemoryMap;

  for (Index = 0; Index < NumEntries; ++Index) {
    if (Desc->Type == EfiReservedMemoryType && (Desc->Attribute & EFI_MEMORY_RUNTIME) != 0) {
      Desc->Type = EfiMemoryMappedIO;
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }
}

/**
  Mark MMIO virtual memory regions as non-runtime to reduce the amount
  of virtual memory required by boot.efi.

  @param[in]      Context            Boot compatibility context.
  @param[in,out]  MemoryMapSize      Memory map size in bytes, updated on devirtualisation.
  @param[in,out]  MemoryMap          Memory map to devirtualise.
  @param[in]      DescriptorSize     Memory map descriptor size in bytes.
**/
STATIC
VOID
DevirtualiseMmio (
  IN     VOID                        *Context,
  IN     UINTN                       MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR       *MemoryMap,
  IN     UINTN                       DescriptorSize
  )
{
  UINTN                       NumEntries;
  UINTN                       Index;
  UINTN                       Index2;
  EFI_MEMORY_DESCRIPTOR       *Desc;
  CONST EFI_PHYSICAL_ADDRESS  *Whitelist;
  UINTN                       WhitelistSize;
  BOOLEAN                     Skipped;
  UINT64                      PagesSaved;

  Whitelist     = ((BOOT_COMPAT_CONTEXT *) Context)->Settings.MmioWhitelist;
  WhitelistSize = ((BOOT_COMPAT_CONTEXT *) Context)->Settings.MmioWhitelistSize;

  //
  // Some types of firmware (typically Haswell and earlier) need certain MMIO areas to have
  // virtual addresses due to their firmware implementations to access NVRAM.
  // For example, on Intel Haswell with APTIO that would be:
  // 0xFED1C000 (SB_RCBA) is a 0x4 page memory region, containing SPI_BASE at 0x3800 (SPI_BASE_ADDRESS).
  // 0xFF000000 (PCI root) is a 0x1000 page memory region.
  // One can make exceptions with Whitelist, as it is required on certain laptops.
  //

  Desc       = MemoryMap;
  NumEntries = MemoryMapSize / DescriptorSize;
  PagesSaved = 0;

  if (!((BOOT_COMPAT_CONTEXT *) Context)->ServiceState.ReportedMmio) {
    DEBUG ((DEBUG_INFO, "OCABC: MMIO devirt start\n"));
  }

  for (Index = 0; Index < NumEntries; ++Index) {
    if (Desc->NumberOfPages > 0
      && Desc->Type == EfiMemoryMappedIO
      && (Desc->Attribute & EFI_MEMORY_RUNTIME) != 0) {

      Skipped = FALSE;

      for (Index2 = 0; Index2 < WhitelistSize; ++Index2) {
        if (AREA_WITHIN_DESCRIPTOR (Desc, Whitelist[Index2], 1)) {
          Skipped = TRUE;
          break;
        }
      }

      if (!((BOOT_COMPAT_CONTEXT *) Context)->ServiceState.ReportedMmio) {
        DEBUG ((
          DEBUG_INFO,
          "OCABC: MMIO devirt 0x%Lx (0x%Lx pages, 0x%Lx) skip %d\n",
          (UINT64) Desc->PhysicalStart,
          (UINT64) Desc->NumberOfPages,
          (UINT64) Desc->Attribute,
          Skipped
          ));
      }

      if (!Skipped) {
        Desc->Attribute &= ~EFI_MEMORY_RUNTIME;
        PagesSaved      += Desc->NumberOfPages;
      }
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }

  if (!((BOOT_COMPAT_CONTEXT *) Context)->ServiceState.ReportedMmio) {
    DEBUG ((
      DEBUG_INFO,
      "OCABC: MMIO devirt end, saved %Lu KB\n",
      EFI_PAGES_TO_SIZE (PagesSaved) / BASE_1KB
      ));
    ((BOOT_COMPAT_CONTEXT *) Context)->ServiceState.ReportedMmio = TRUE;
  }
}

/**
  Apply single booter patch.

  @param[in,out]  ImageBase      Booter image to be patched.
  @param[in]      ImageSize      Size of booter image.
  @param[in]      Patch          Single patch to be applied to booter.
**/
STATIC
VOID
ApplyBooterPatch (
  IN OUT UINT8            *ImageBase,
  IN     UINTN            ImageSize,
  IN     OC_BOOTER_PATCH  *Patch
  )
{
  UINT32  ReplaceCount;

  if (ImageSize < Patch->Size) {
    DEBUG ((DEBUG_INFO, "OCABC: Image size is even smaller than patch size\n"));
    return;
  }

  if (Patch->Limit > 0 && Patch->Limit < ImageSize) {
    ImageSize = Patch->Limit;
  }

  ReplaceCount = ApplyPatch (
    Patch->Find,
    Patch->Mask,
    Patch->Size,
    Patch->Replace,
    Patch->ReplaceMask,
    ImageBase,
    (UINT32) ImageSize,
    Patch->Count,
    Patch->Skip
    );

  if (ReplaceCount > 0 && Patch->Count > 0 && ReplaceCount != Patch->Count) {
    DEBUG ((
      DEBUG_INFO,
      "OCABC: Booter patch (%a) performed only %u replacements out of %u\n",
      Patch->Comment,
      ReplaceCount,
      Patch->Count
      ));
  } else if (ReplaceCount == 0) {
    DEBUG ((
      DEBUG_INFO,
      "OCABC: Failed to apply Booter patch (%a) - not found\n",
      Patch->Comment
      ));
  } else {
    DEBUG ((DEBUG_INFO, "OCABC: Booter patch (%a) replace count - %u\n", Patch->Comment, ReplaceCount));
  }
}

/**
  Iterate through user booter patches and apply them.

  @param[in]      ImageHandle      Loaded image handle to patch.
  @param[in]      IsApple          Whether the booter is Apple-made.
  @param[in]      Patches          Array of patches to be applied.
  @param[in]      PatchesCount     Size of patches to be applied.
**/
STATIC
VOID
ApplyBooterPatches (
  IN  EFI_HANDLE       ImageHandle,
  IN  BOOLEAN          IsApple,
  IN  OC_BOOTER_PATCH  *Patches,
  IN  UINT32           PatchCount
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;
  UINT32                      Index;
  BOOLEAN                     UsePatch;
  CONST CHAR8                 *UserIdentifier;
  CHAR16                      *UserIdentifierUnicode;

  Status = gBS->HandleProtocol (
    ImageHandle,
    &gEfiLoadedImageProtocolGuid,
    (VOID **)&LoadedImage
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCABC: Failed to handle LoadedImage protocol - %r", Status));
    return;
  }

  for (Index = 0; Index < PatchCount; ++Index) {
    UserIdentifier = Patches[Index].Identifier;

    if (UserIdentifier[0] == '\0' || AsciiStrCmp (UserIdentifier, "Any") == 0) {
      UsePatch = TRUE;
    } else if (AsciiStrCmp (UserIdentifier, "Apple") == 0) {
      UsePatch = IsApple;
    } else {
      UserIdentifierUnicode = AsciiStrCopyToUnicode (UserIdentifier, 0);
      if (UserIdentifierUnicode == NULL) {
        DEBUG ((DEBUG_INFO, "OCABC: Booter patch (%a) for %a is out of memory\n", Patches[Index].Comment, UserIdentifier));
        continue;
      }
      UsePatch = OcDevicePathHasFilePathSuffix (LoadedImage->FilePath, UserIdentifierUnicode, StrLen (UserIdentifierUnicode));
      FreePool (UserIdentifierUnicode);
    }

    if (UsePatch) {
      ApplyBooterPatch (
        (UINT8 *) LoadedImage->ImageBase,
        (UINTN)LoadedImage->ImageSize,
        &Patches[Index]
        );
    }
  }
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
  BOOT_COMPAT_CONTEXT     *BootCompat;
  BOOLEAN                 IsPerfAlloc;
  BOOLEAN                 IsCallGateAlloc;

  //
  // Filter out garbage right away.
  //
  if (Memory == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BootCompat      = GetBootCompatContext ();
  IsPerfAlloc     = FALSE;
  IsCallGateAlloc = FALSE;

  if (BootCompat->ServiceState.AwaitingPerfAlloc) {
    if (BootCompat->ServiceState.AppleBootNestedCount > 0) {
      if (Type == AllocateMaxAddress
        && MemoryType == EfiACPIReclaimMemory
        && *Memory == BASE_4GB - 1) {
        IsPerfAlloc = TRUE;
      }
    } else {
      BootCompat->ServiceState.AwaitingPerfAlloc = FALSE;
    }
  }

  if (BootCompat->ServiceState.AppleBootNestedCount > 0
    && Type == AllocateMaxAddress
    && MemoryType == EfiLoaderCode
    && *Memory == BASE_4GB - 1
    && NumberOfPages == 1) {
    IsCallGateAlloc = TRUE;
  }

  if (BootCompat->Settings.AllowRelocationBlock
    && BootCompat->ServiceState.AppleBootNestedCount > 0
    && Type == AllocateAddress
    && MemoryType == EfiLoaderData) {
    Status = AppleRelocationAllocatePages (
      BootCompat,
      BootCompat->ServicePtrs.GetMemoryMap,
      BootCompat->ServicePtrs.AllocatePages,
      NumberOfPages,
      Memory
      );
  } else {
    Status = EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status)) {
    Status = BootCompat->ServicePtrs.AllocatePages (
      Type,
      MemoryType,
      NumberOfPages,
      Memory
      );
  }

  DEBUG ((DEBUG_VERBOSE, "OCABC: AllocPages %u 0x%Lx (%u) - %r\n", Type, *Memory, NumberOfPages, Status));

  if (!EFI_ERROR (Status)) {
    FixRuntimeAttributes (BootCompat, MemoryType);

    if (BootCompat->ServiceState.AppleBootNestedCount > 0) {
      if (IsCallGateAlloc) {
        //
        // Called from boot.efi.
        // Memory allocated for boot.efi to kernel trampoline.
        //
        BootCompat->ServiceState.KernelCallGate = *Memory;
      } else if (IsPerfAlloc) {
        //
        // Called from boot.efi.
        // New perf data, it can be reallocated multiple times.
        //
        OcAppleDebugLogPerfAllocated ((VOID *)(UINTN) *Memory, EFI_PAGES_TO_SIZE (NumberOfPages));
      }
    }
  }

  return Status;
}

/**
  UEFI Boot Services FreePages override.
  Ensures synchronised memory attribute table.
**/
STATIC
EFI_STATUS
EFIAPI
OcFreePages (
  IN  EFI_PHYSICAL_ADDRESS         Memory,
  IN  UINTN                        Pages
  )
{
  EFI_STATUS              Status;
  BOOT_COMPAT_CONTEXT     *BootCompat;

  BootCompat  = GetBootCompatContext ();

  Status = BootCompat->ServicePtrs.FreePages (
    Memory,
    Pages
    );

  if (!EFI_ERROR (Status)) {
    FixRuntimeAttributes (BootCompat, EfiRuntimeServicesData);
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
  EFI_STATUS            Status2;
  BOOT_COMPAT_CONTEXT   *BootCompat;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 Pages;
  UINTN                 OriginalSize;

  BootCompat = GetBootCompatContext ();

  OriginalSize = MemoryMapSize != 0 ? *MemoryMapSize : 0;
  Status = BootCompat->ServicePtrs.GetMemoryMap (
    MemoryMapSize,
    MemoryMap,
    MapKey,
    DescriptorSize,
    DescriptorVersion
    );

  //
  // Reserve larger area for the memory map when we need to split it.
  //
  if (BootCompat->ServiceState.AppleBootNestedCount > 0 && Status == EFI_BUFFER_TOO_SMALL) {
    *MemoryMapSize += OcCountSplitDescriptors () * *DescriptorSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BootCompat->Settings.SyncRuntimePermissions && BootCompat->ServiceState.FwRuntime != NULL) {
    //
    // Some types of firmware mark runtime drivers loaded after EndOfDxe as EfiRuntimeServicesData:
    // REF: https://github.com/acidanthera/bugtracker/issues/791#issuecomment-607935508
    //
    Status2 = BootCompat->ServiceState.FwRuntime->GetExecArea (&Address, &Pages);

    if (!EFI_ERROR (Status2)) {
      OcUpdateDescriptors (
        *MemoryMapSize,
        MemoryMap,
        *DescriptorSize,
        Address,
        EfiRuntimeServicesCode,
        0,
        0
        );
    }
  }

  if (BootCompat->ServiceState.AppleBootNestedCount > 0) {
    if (BootCompat->Settings.ProtectMemoryRegions) {
      ProtectMemoryRegions (
        *MemoryMapSize,
        MemoryMap,
        *DescriptorSize
        );
    }

    if (BootCompat->Settings.DevirtualiseMmio) {
      DevirtualiseMmio (
        BootCompat,
        *MemoryMapSize,
        MemoryMap,
        *DescriptorSize
        );
    }

    if (BootCompat->Settings.RebuildAppleMemoryMap) {
      OcSortMemoryMap (*MemoryMapSize, MemoryMap, *DescriptorSize);

      Status2 = OcSplitMemoryMapByAttributes (
        OriginalSize,
        MemoryMapSize,
        MemoryMap,
        *DescriptorSize
        );
      if (EFI_ERROR (Status2) && Status2 != EFI_UNSUPPORTED) {
        DEBUG ((DEBUG_INFO, "OCABC: Cannot rebuild memory map - %r\n", Status));
      }

      OcShrinkMemoryMap (
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
  UEFI Boot Services AllocatePool override.
  Ensures synchronised memory attribute table.
**/
STATIC
EFI_STATUS
EFIAPI
OcAllocatePool (
  IN  EFI_MEMORY_TYPE              PoolType,
  IN  UINTN                        Size,
  OUT VOID                         **Buffer
  )
{
  EFI_STATUS              Status;
  BOOT_COMPAT_CONTEXT     *BootCompat;

  BootCompat  = GetBootCompatContext ();

  Status = BootCompat->ServicePtrs.AllocatePool (
    PoolType,
    Size,
    Buffer
    );

  if (!EFI_ERROR (Status)) {
    FixRuntimeAttributes (BootCompat, PoolType);
  }

  return Status;
}

/**
  UEFI Boot Services FreePool override.
  Ensures synchronised memory attribute table.
**/
STATIC
EFI_STATUS
EFIAPI
OcFreePool (
  IN VOID                         *Buffer
  )
{
  EFI_STATUS              Status;
  BOOT_COMPAT_CONTEXT     *BootCompat;

  BootCompat  = GetBootCompatContext ();

  Status = BootCompat->ServicePtrs.FreePool (
    Buffer
    );

  if (!EFI_ERROR (Status)) {
    FixRuntimeAttributes (BootCompat, EfiRuntimeServicesData);
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
  IN  EFI_HANDLE  ImageHandle,
  OUT UINTN       *ExitDataSize,
  OUT CHAR16      **ExitData  OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *AppleLoadedImage;
  EFI_OS_INFO_PROTOCOL        *OSInfo;
  BOOT_COMPAT_CONTEXT         *BootCompat;
  OC_FWRT_CONFIG              Config;
  UINTN                       DataSize;

  BootCompat        = GetBootCompatContext ();
  AppleLoadedImage  = OcGetAppleBootLoadedImage (ImageHandle);

  //
  // Recover firmware-replaced GetMemoryMap pointer.
  //
  if (BootCompat->Settings.ProtectUefiServices
    && BootCompat->ServicePtrs.GetMemoryMap != OcGetMemoryMap) {
    DEBUG ((DEBUG_INFO, "OCABC: Recovering trashed GetMemoryMap pointer\n"));
    gBS->GetMemoryMap = OcGetMemoryMap;
    gBS->Hdr.CRC32    = 0;
    gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &gBS->Hdr.CRC32);
  }

  FixRuntimeAttributes (BootCompat, EfiRuntimeServicesData);

  //
  // Clear monitoring vars
  //
  BootCompat->ServiceState.KernelCallGate = 0;

  if (AppleLoadedImage != NULL) {
    //
    // Report about macOS being loaded.
    //
    ++BootCompat->ServiceState.AppleBootNestedCount;

    //
    // VMware uses OSInfo->SetName call by EfiBoot to ensure that we are allowed
    // to run this version of macOS on VMware. The relevant EfiBoot image handle
    // is determined by the return address of OSInfo->SetName call, which will
    // be found in the list they make within their StartImage wrapper.
    //
    // The problem here happens with Apple Secure Boot, which makes their StartImage
    // wrapper to not be called and therefore OSInfo->SetName to fail to install anything.
    // Simply install a protocol here. Maybe make it a quirk if necessary.
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
      &ImageHandle,
      &gVMwareMacProtocolGuid,
      NULL,
      NULL
      );
    DEBUG ((DEBUG_INFO, "OCABC: VMware Mac installed on %p - %r\n", ImageHandle, Status));

    BootCompat->ServiceState.AppleHibernateWake = OcIsAppleHibernateWake ();
    BootCompat->ServiceState.AppleCustomSlide = OcCheckArgumentFromEnv (
      AppleLoadedImage,
      BootCompat->ServicePtrs.GetVariable,
      "slide=",
      L_STR_LEN ("slide="),
      NULL
      );

    if (BootCompat->Settings.EnableSafeModeSlide) {
      ASSERT (AppleLoadedImage->ImageSize <= MAX_UINTN);
      AppleSlideUnlockForSafeMode (
        (UINT8 *) AppleLoadedImage->ImageBase,
        (UINTN)AppleLoadedImage->ImageSize
        );
    }

    AppleMapPrepareBooterState (
      BootCompat,
      AppleLoadedImage,
      BootCompat->ServicePtrs.GetMemoryMap
      );
  } else if (BootCompat->Settings.SignalAppleOS) {
    Status = gBS->LocateProtocol (
      &gEfiOSInfoProtocolGuid,
      NULL,
      (VOID *) &OSInfo
      );

    if (!EFI_ERROR (Status)) {
      //
      // Older versions of the protocol have less fields.
      // For instance, VMware installs 0x1 version.
      //
      if (OSInfo->Revision >= EFI_OS_INFO_PROTOCOL_REVISION_VENDOR) {
        OSInfo->OSVendor (EFI_OS_INFO_APPLE_VENDOR_NAME);
      }
      if (OSInfo->Revision >= EFI_OS_INFO_PROTOCOL_REVISION_NAME) {
        OSInfo->OSName ("Mac OS X 10.15");
      }
    }
  }

  //
  // Apply customised booter patches.
  //
  ApplyBooterPatches (
    ImageHandle,
    AppleLoadedImage != NULL,
    BootCompat->Settings.BooterPatches,
    BootCompat->Settings.BooterPatchesSize
    );

  if (BootCompat->ServiceState.FwRuntime != NULL) {
    BootCompat->ServiceState.FwRuntime->GetCurrent (&Config);

    //
    // Support for ReadOnly and WriteOnly variables is OpenCore & Lilu security basics.
    // For now always enable it.
    //
    Config.RestrictedVariables = TRUE;

    //
    // Restrict secure boot variables and never let them slip unless once restricted.
    //
    Config.ProtectSecureBoot = BootCompat->Settings.ProtectSecureBoot;

    //
    // Enable Boot#### variable redirection if OpenCore requested it.
    // Do NOT disable it once enabled for stability reasons.
    //
    DataSize = sizeof (Config.BootVariableRedirect);
    BootCompat->ServicePtrs.GetVariable (
      OC_BOOT_REDIRECT_VARIABLE_NAME,
      &gOcVendorVariableGuid,
      NULL,
      &DataSize,
      &Config.BootVariableRedirect
      );

    //
    // Enable Apple-specific changes if requested.
    // Disable them when this is no longer Apple.
    //
    if (BootCompat->ServiceState.AppleBootNestedCount > 0) {
      Config.WriteProtection  = BootCompat->Settings.DisableVariableWrite;
      Config.WriteUnprotector = BootCompat->Settings.EnableWriteUnprotector;
    } else {
      Config.WriteProtection  = FALSE;
      Config.WriteUnprotector = FALSE;
    }

    BootCompat->ServiceState.FwRuntime->SetMain (
      &Config
      );
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

    if (BootCompat->ServiceState.AppleBootNestedCount == 0) {
      AppleRelocationRelease (BootCompat);
    }
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
  UINTN                    Index;

  BootCompat = GetBootCompatContext ();

  //
  // Handle events in case we have any.
  //
  if (BootCompat->Settings.ExitBootServicesHandlers != NULL) {
    for (Index = 0; BootCompat->Settings.ExitBootServicesHandlers[Index] != NULL; ++Index) {
      BootCompat->Settings.ExitBootServicesHandlers[Index] (
        NULL,
        BootCompat->Settings.ExitBootServicesHandlerContexts[Index]
        );
      //
      // Even if ExitBootServices fails, do not subsequently call the events we handled.
      //
      BootCompat->Settings.ExitBootServicesHandlers[Index] = NULL;
    }
  }

  FixRuntimeAttributes (BootCompat, EfiRuntimeServicesData);

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
  // Enable custom SetVirtualAddressMap.
  //
  if (BootCompat->ServiceState.FwRuntime != NULL) {
    BootCompat->ServiceState.FwRuntime->OnSetAddressMap (NULL, TRUE);
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
  // Abort on error.
  //
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AppleMapPrepareKernelJump (
    BootCompat,
    (UINTN) BootCompat->ServiceState.KernelCallGate
    );

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

  ASSERT (BootCompat->ServiceState.AppleBootNestedCount > 0);
  
  Status = AppleMapPrepareMemState (
    BootCompat,
    MemoryMapSize,
    DescriptorSize,
    DescriptorVersion,
    MemoryMap
    );

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
  BOOLEAN                IsApple;

  BootCompat = GetBootCompatContext ();
  IsApple = BootCompat->ServiceState.AppleBootNestedCount > 0;

  if (IsApple && BootCompat->Settings.ProvideCustomSlide) {
    Status = AppleSlideGetVariable (
      BootCompat,
      BootCompat->ServicePtrs.GetVariable,
      BootCompat->ServicePtrs.GetMemoryMap,
      BootCompat->Settings.DevirtualiseMmio ? DevirtualiseMmio : NULL,
      BootCompat,
      VariableName,
      VendorGuid,
      Attributes,
      DataSize,
      Data
      );
  } else {
    Status = BootCompat->ServicePtrs.GetVariable (
      VariableName,
      VendorGuid,
      Attributes,
      DataSize,
      Data
      );
  }

  //
  // Catch performance record allocation.
  //
  if (IsApple
    && Status == EFI_BUFFER_TOO_SMALL
    && CompareGuid (VendorGuid, &gAppleBootVariableGuid)
    && StrCmp (VariableName, APPLE_EFI_BOOT_PERF_VARIABLE_NAME) == 0) {
    BootCompat->ServiceState.AwaitingPerfAlloc = TRUE;
    DEBUG ((DEBUG_INFO, "OCABC: Caught successful request for %s\n", VariableName));
  }

  return Status;
}

/**
  UEFI Runtime Services GetVariable override event handler.
  We do not override GetVariable ourselves but let our runtime do that.

  @param[in]  Event    Event handle.
  @param[in]  Context  Apple boot compatibility context.
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
  BOOT_COMPAT_CONTEXT           *BootCompat;

  BootCompat = (BOOT_COMPAT_CONTEXT *) Context;

  if (BootCompat->ServicePtrs.GetVariable == NULL) {
    Status = gBS->LocateProtocol (
      &gOcFirmwareRuntimeProtocolGuid,
      NULL,
      (VOID **) &FwRuntime
      );

    if (!EFI_ERROR (Status)) {
      if (FwRuntime->Revision == OC_FIRMWARE_RUNTIME_REVISION) {
        DEBUG ((DEBUG_INFO, "OCABC: Got rendezvous with OpenRuntime r%u\n", OC_FIRMWARE_RUNTIME_REVISION));
        DEBUG ((DEBUG_INFO, "OCABC: MAT support is %d\n", OcGetMemoryAttributes (NULL) != NULL));
        Status = FwRuntime->OnGetVariable (OcGetVariable, &BootCompat->ServicePtrs.GetVariable);
        if (!EFI_ERROR (Status)) {
          //
          // We override virtual address mapping function to perform
          // runtime area protection to prevent boot.efi
          // defragmentation and setup virtual memory for firmware
          // accessing it after exit boot services.
          //
          // We cannot override it to non-RT area since this is prohibited
          // by spec and at least Linux does not support it.
          //
          Status = FwRuntime->OnSetAddressMap (OcSetVirtualAddressMap, FALSE);
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_INFO, "OCABC: OnSetAddressMap failure - %r\n", Status));
          }
        } else {
          DEBUG ((DEBUG_INFO, "OCABC: OnGetVariable failure - %r\n", Status));
        }
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "OCABC: Incompatible OpenRuntime r%u, require r%u\n",
          (UINT32) FwRuntime->Revision,
          (UINT32) OC_FIRMWARE_RUNTIME_REVISION
          ));
        CpuDeadLoop ();
      }
    } else {
      DEBUG ((DEBUG_INFO, "OCABC: Awaiting rendezvous with OpenRuntime r%u\n", OC_FIRMWARE_RUNTIME_REVISION));
      Status = EFI_UNSUPPORTED;
    }

    //
    // Mark protocol as useable.
    //
    if (!EFI_ERROR (Status)) {
      BootCompat->ServiceState.FwRuntime = FwRuntime;
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
  EFI_TPL                 OriginalTpl;

  ServicePtrs = &BootCompat->ServicePtrs;

  OriginalTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  ServicePtrs->AllocatePages        = gBS->AllocatePages;
  ServicePtrs->FreePages            = gBS->FreePages;
  ServicePtrs->GetMemoryMap         = gBS->GetMemoryMap;
  ServicePtrs->AllocatePool         = gBS->AllocatePool;
  ServicePtrs->FreePool             = gBS->FreePool;
  ServicePtrs->ExitBootServices     = gBS->ExitBootServices;
  ServicePtrs->StartImage           = gBS->StartImage;

  gBS->AllocatePages        = OcAllocatePages;
  gBS->FreePages            = OcFreePages;
  gBS->GetMemoryMap         = OcGetMemoryMap;
  gBS->AllocatePool         = OcAllocatePool;
  gBS->FreePool             = OcFreePool;
  gBS->ExitBootServices     = OcExitBootServices;
  gBS->StartImage           = OcStartImage;

  gBS->Hdr.CRC32 = 0;
  gBS->Hdr.CRC32 = CalculateCrc32 (gBS, gBS->Hdr.HeaderSize);

  gRT->Hdr.CRC32 = 0;
  gRT->Hdr.CRC32 = CalculateCrc32 (gRT, gRT->Hdr.HeaderSize);

  gBS->RestoreTPL (OriginalTpl);

  //
  // Update GetVariable handle with the help of external runtime services.
  //
  SetGetVariableHookHandler (NULL, BootCompat);

  if (BootCompat->ServicePtrs.GetVariable != NULL) {
    return;
  }

  Status = gBS->CreateEvent (
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    SetGetVariableHookHandler,
    BootCompat,
    &BootCompat->ServiceState.GetVariableEvent
    );

  if (!EFI_ERROR (Status)) {
    Status = gBS->RegisterProtocolNotify (
      &gOcFirmwareRuntimeProtocolGuid,
      BootCompat->ServiceState.GetVariableEvent,
      &Registration
      );

    if (EFI_ERROR (Status)) {
      gBS->CloseEvent (BootCompat->ServiceState.GetVariableEvent);
    }
  }
}
