/** @file
  OpenCore driver.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ProcessorBind.h"
#include <OpenCore.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAfterBootCompatLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcAppleImg4Lib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVirtualFsLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
OcKernelApplyQuirk (
  IN     KERNEL_QUIRK_NAME  Quirk,
  IN     KERNEL_CACHE_TYPE  CacheType,
  IN     UINT32             DarwinVersion,
  IN OUT VOID               *Context,
  IN OUT PATCHER_CONTEXT    *KernelPatcher
  )
{
  //
  // Apply kernel quirks to kernel, kext patches to context.
  //
  if (Context == NULL) {
    ASSERT (KernelPatcher != NULL);
    return KernelApplyQuirk (Quirk, KernelPatcher, DarwinVersion);
  }

  if (CacheType == CacheTypeCacheless) {
    return CachelessContextAddQuirk (Context, Quirk);
  }
  
  if (CacheType == CacheTypeMkext) {
    return MkextContextApplyQuirk (Context, Quirk, DarwinVersion);
  }

  if (CacheType == CacheTypePrelinked) {
    return PrelinkedContextApplyQuirk (Context, Quirk, DarwinVersion);
  }

  return EFI_UNSUPPORTED;
}

VOID
OcKernelApplyPatches (
  IN     OC_GLOBAL_CONFIG  *Config,
  IN     OC_CPU_INFO       *CpuInfo,
  IN     UINT32            DarwinVersion,
  IN     KERNEL_CACHE_TYPE CacheType,
  IN     VOID              *Context,
  IN OUT UINT8             *Kernel,
  IN     UINT32            Size
  )
{
  EFI_STATUS             Status;
  PATCHER_CONTEXT        KernelPatcher;
  UINT32                 Index;
  PATCHER_GENERIC_PATCH  Patch;
  OC_KERNEL_PATCH_ENTRY  *UserPatch;
  CONST CHAR8            *Target;
  CONST CHAR8            *Comment;
  UINT32                 MaxKernel;
  UINT32                 MinKernel;
  BOOLEAN                IsKernelPatch;

  IsKernelPatch = Context == NULL;

  if (IsKernelPatch) {
    ASSERT (Kernel != NULL);

    Status = PatcherInitContextFromBuffer (
      &KernelPatcher,
      Kernel,
      Size
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OC: Kernel patcher kernel init failure - %r\n", Status));
      return;
    }
  }

  for (Index = 0; Index < Config->Kernel.Patch.Count; ++Index) {
    UserPatch = Config->Kernel.Patch.Values[Index];
    Target    = OC_BLOB_GET (&UserPatch->Identifier);
    Comment   = OC_BLOB_GET (&UserPatch->Comment);

    if (!UserPatch->Enabled || (AsciiStrCmp (Target, "kernel") == 0) != IsKernelPatch) {
      continue;
    }

    MaxKernel   = OcParseDarwinVersion (OC_BLOB_GET (&UserPatch->MaxKernel));
    MinKernel   = OcParseDarwinVersion (OC_BLOB_GET (&UserPatch->MinKernel));

    if (!OcMatchDarwinVersion (DarwinVersion, MinKernel, MaxKernel)) {
      DEBUG ((
        DEBUG_INFO,
        "OC: Kernel patcher skips %a (%a) patch at %u due to version %u <= %u <= %u\n",
        Target,
        Comment,
        Index,
        MinKernel,
        DarwinVersion,
        MaxKernel
        ));
      continue;
    }

    //
    // Ignore patch if:
    // - There is nothing to replace.
    // - We have neither symbolic base, nor find data.
    // - Find and replace mismatch in size.
    // - Mask and ReplaceMask mismatch in size when are available.
    //
    if (UserPatch->Replace.Size == 0
      || (OC_BLOB_GET (&UserPatch->Base)[0] == '\0' && UserPatch->Find.Size != UserPatch->Replace.Size)
      || (UserPatch->Mask.Size > 0 && UserPatch->Find.Size != UserPatch->Mask.Size)
      || (UserPatch->ReplaceMask.Size > 0 && UserPatch->Find.Size != UserPatch->ReplaceMask.Size)) {
      DEBUG ((DEBUG_ERROR, "OC: Kernel patch %u for %a (%a) is borked\n", Index, Target, Comment));
      continue;
    }

    ZeroMem (&Patch, sizeof (Patch));

    if (OC_BLOB_GET (&UserPatch->Comment)[0] != '\0') {
      Patch.Comment  = OC_BLOB_GET (&UserPatch->Comment);
    }

    if (OC_BLOB_GET (&UserPatch->Base)[0] != '\0') {
      Patch.Base  = OC_BLOB_GET (&UserPatch->Base);
    }

    if (UserPatch->Find.Size > 0) {
      Patch.Find  = OC_BLOB_GET (&UserPatch->Find);
    }

    Patch.Replace = OC_BLOB_GET (&UserPatch->Replace);

    if (UserPatch->Mask.Size > 0) {
      Patch.Mask  = OC_BLOB_GET (&UserPatch->Mask);
    }

    if (UserPatch->ReplaceMask.Size > 0) {
      Patch.ReplaceMask = OC_BLOB_GET (&UserPatch->ReplaceMask);
    }

    Patch.Size    = UserPatch->Replace.Size;
    Patch.Count   = UserPatch->Count;
    Patch.Skip    = UserPatch->Skip;
    Patch.Limit   = UserPatch->Limit;

    if (IsKernelPatch) {
      Status = PatcherApplyGenericPatch (&KernelPatcher, &Patch);
    } else {
      if (CacheType == CacheTypeCacheless) {
        Status = CachelessContextAddPatch (Context, Target, &Patch);
      } else if (CacheType == CacheTypeMkext) {
        Status = MkextContextApplyPatch (Context, Target, &Patch);
      } else if (CacheType == CacheTypePrelinked) {
        Status = PrelinkedContextApplyPatch (Context, Target, &Patch);
      }
    }

    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Kernel patcher result %u for %a (%a) - %r\n",
      Index,
      Target,
      Comment,
      Status
      ));
  }

  if (!IsKernelPatch) {
    if (Config->Kernel.Quirks.AppleCpuPmCfgLock) {
      OcKernelApplyQuirk (KernelQuirkAppleCpuPmCfgLock, CacheType, DarwinVersion, Context, NULL);
    }

    if (Config->Kernel.Quirks.ExternalDiskIcons) {
      OcKernelApplyQuirk (KernelQuirkExternalDiskIcons, CacheType, DarwinVersion, Context, NULL);
    }

    if (Config->Kernel.Quirks.ThirdPartyDrives) {
      OcKernelApplyQuirk (KernelQuirkThirdPartyDrives, CacheType, DarwinVersion, Context, NULL);
    }

    if (Config->Kernel.Quirks.XhciPortLimit) {
      OcKernelApplyQuirk (KernelQuirkXhciPortLimit1, CacheType, DarwinVersion, Context, NULL);
      OcKernelApplyQuirk (KernelQuirkXhciPortLimit2, CacheType, DarwinVersion, Context, NULL);
      OcKernelApplyQuirk (KernelQuirkXhciPortLimit3, CacheType, DarwinVersion, Context, NULL);
    }

    if (Config->Kernel.Quirks.DisableIoMapper) {
      OcKernelApplyQuirk (KernelQuirkDisableIoMapper, CacheType, DarwinVersion, Context, NULL);
    }

    if (Config->Kernel.Quirks.DisableRtcChecksum) {
      OcKernelApplyQuirk (KernelQuirkDisableRtcChecksum, CacheType, DarwinVersion, Context, NULL);
    }

    if (Config->Kernel.Quirks.IncreasePciBarSize) {
      OcKernelApplyQuirk (KernelQuirkIncreasePciBarSize, CacheType, DarwinVersion, Context, NULL);     
    }

    if (Config->Kernel.Quirks.CustomSmbiosGuid) {
      OcKernelApplyQuirk (KernelQuirkCustomSmbiosGuid1, CacheType, DarwinVersion, Context, NULL);
      OcKernelApplyQuirk (KernelQuirkCustomSmbiosGuid2, CacheType, DarwinVersion, Context, NULL);
    }

    if (Config->Kernel.Quirks.DummyPowerManagement) {
      OcKernelApplyQuirk (KernelQuirkDummyPowerManagement, CacheType, DarwinVersion, Context, NULL);
    }
  } else {
    if (Config->Kernel.Quirks.AppleXcpmCfgLock) {
      OcKernelApplyQuirk (KernelQuirkAppleXcpmCfgLock, CacheType, DarwinVersion, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Quirks.AppleXcpmExtraMsrs) {
      OcKernelApplyQuirk (KernelQuirkAppleXcpmExtraMsrs, CacheType, DarwinVersion, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Quirks.AppleXcpmForceBoost) {
      OcKernelApplyQuirk (KernelQuirkAppleXcpmForceBoost, CacheType, DarwinVersion, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Quirks.PanicNoKextDump) {
      OcKernelApplyQuirk (KernelQuirkPanicNoKextDump, CacheType, DarwinVersion, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Emulate.Cpuid1Data[0] != 0
      || Config->Kernel.Emulate.Cpuid1Data[1] != 0
      || Config->Kernel.Emulate.Cpuid1Data[2] != 0
      || Config->Kernel.Emulate.Cpuid1Data[3] != 0) {
      PatchKernelCpuId (
        &KernelPatcher,
        CpuInfo,
        Config->Kernel.Emulate.Cpuid1Data,
        Config->Kernel.Emulate.Cpuid1Mask
        );
    }

    if (Config->Kernel.Quirks.LapicKernelPanic) {
      OcKernelApplyQuirk (KernelQuirkLapicKernelPanic, CacheType, DarwinVersion, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Quirks.PowerTimeoutKernelPanic) {
      OcKernelApplyQuirk (KernelQuirkPowerTimeoutKernelPanic, CacheType, DarwinVersion, NULL, &KernelPatcher);
    }
  }
}

VOID
OcKernelBlockKexts (
  IN     OC_GLOBAL_CONFIG  *Config,
  IN     UINT32            DarwinVersion,
  IN     PRELINKED_CONTEXT *Context
  )
{
  EFI_STATUS             Status;
  PATCHER_CONTEXT        Patcher;
  UINT32                 Index;
  OC_KERNEL_BLOCK_ENTRY  *Kext;
  CONST CHAR8            *Target;
  CONST CHAR8            *Comment;
  UINT32                 MaxKernel;
  UINT32                 MinKernel;

  for (Index = 0; Index < Config->Kernel.Block.Count; ++Index) {
    Kext    = Config->Kernel.Block.Values[Index];
    Target  = OC_BLOB_GET (&Kext->Identifier);
    Comment = OC_BLOB_GET (&Kext->Comment);

    if (!Kext->Enabled) {
      continue;
    }

    MaxKernel = OcParseDarwinVersion (OC_BLOB_GET (&Kext->MaxKernel));
    MinKernel = OcParseDarwinVersion (OC_BLOB_GET (&Kext->MinKernel));

    if (!OcMatchDarwinVersion (DarwinVersion, MinKernel, MaxKernel)) {
      DEBUG ((
        DEBUG_INFO,
        "OC: Prelink blocker skips %a (%a) block at %u due to version %u <= %u <= %u\n",
        Target,
        Comment,
        Index,
        MinKernel,
        DarwinVersion,
        MaxKernel
        ));
      continue;
    }

    Status = PatcherInitContextFromPrelinked (
      &Patcher,
      Context,
      Target
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OC: Prelink blocker %a (%a) init failure - %r\n", Target, Comment, Status));
      continue;
    }

    Status = PatcherBlockKext (&Patcher);

    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Prelink blocker %a (%a) - %r\n",
      Target,
      Comment,
      Status
      ));
  }
}
