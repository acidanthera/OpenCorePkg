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

STATIC OC_STORAGE_CONTEXT  *mOcStorage;
STATIC OC_GLOBAL_CONFIG    *mOcConfiguration;
STATIC OC_CPU_INFO         *mOcCpuInfo;
STATIC UINT8               mKernelDigest[SHA384_DIGEST_SIZE];

STATIC UINT32              mOcDarwinVersion;
STATIC BOOLEAN             mUse32BitKernel;

STATIC CACHELESS_CONTEXT   mOcCachelessContext;
STATIC BOOLEAN             mOcCachelessInProgress;

//
// Kernel cache types.
//
typedef enum KERNEL_CACHE_TYPE_ {
  CacheTypeCacheless,
  CacheTypeMkext,
  CacheTypePrelinked
} KERNEL_CACHE_TYPE;

STATIC
EFI_STATUS
OcKernelLoadKextsAndReserve (
  IN  OC_STORAGE_CONTEXT  *Storage,
  IN  OC_GLOBAL_CONFIG    *Config,
  IN  KERNEL_CACHE_TYPE   CacheType,
  OUT UINT32              *ReservedExeSize,
  OUT UINT32              *ReservedInfoSize,
  OUT UINT32              *NumReservedKexts
  )
{
  EFI_STATUS           Status;
  UINT32               Index;
  CHAR8                *BundlePath;
  CHAR8                *Comment;
  CHAR8                *PlistPath;
  CHAR8                *ExecutablePath;
  CHAR16               FullPath[OC_STORAGE_SAFE_PATH_MAX];
  OC_KERNEL_ADD_ENTRY  *Kext;

  *ReservedInfoSize = PRELINK_INFO_RESERVE_SIZE;
  *ReservedExeSize  = 0;
  *NumReservedKexts = 0;

  for (Index = 0; Index < Config->Kernel.Add.Count; ++Index) {
    Kext = Config->Kernel.Add.Values[Index];

    if (!Kext->Enabled) {
      continue;
    }

    if (Kext->PlistDataSize == 0) {
      BundlePath     = OC_BLOB_GET (&Kext->BundlePath);
      Comment        = OC_BLOB_GET (&Kext->Comment);
      PlistPath      = OC_BLOB_GET (&Kext->PlistPath);
      if (BundlePath[0] == '\0' || PlistPath[0] == '\0') {
        DEBUG ((DEBUG_ERROR, "OC: Your config has improper for kext info\n"));
        Kext->Enabled = FALSE;
        continue;
      }

      Status = OcUnicodeSafeSPrint (
        FullPath,
        sizeof (FullPath),
        OPEN_CORE_KEXT_PATH "%a\\%a",
        BundlePath,
        PlistPath
        );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_WARN,
          "OC: Failed to fit kext path %s%a\\%a",
          OPEN_CORE_KEXT_PATH,
          BundlePath,
          PlistPath
          ));
        Kext->Enabled = FALSE;
        continue;
      }

      UnicodeUefiSlashes (FullPath);

      Kext->PlistData = OcStorageReadFileUnicode (
        Storage,
        FullPath,
        &Kext->PlistDataSize
        );

      if (Kext->PlistData == NULL) {
        DEBUG ((
          DEBUG_ERROR,
          "OC: Plist %s is missing for kext %a (%a)\n",
          FullPath,
          BundlePath,
          Comment
          ));
        Kext->Enabled = FALSE;
        continue;
      }

      ExecutablePath = OC_BLOB_GET (&Kext->ExecutablePath);
      if (ExecutablePath[0] != '\0') {
        Status = OcUnicodeSafeSPrint (
          FullPath,
          sizeof (FullPath),
          OPEN_CORE_KEXT_PATH "%a\\%a",
          BundlePath,
          ExecutablePath
          );
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_WARN,
            "OC: Failed to fit kext path %s%a\\%a",
            OPEN_CORE_KEXT_PATH,
            BundlePath,
            ExecutablePath
            ));
          Kext->Enabled = FALSE;
          FreePool (Kext->PlistData);
          Kext->PlistData = NULL;
          continue;
        }

        UnicodeUefiSlashes (FullPath);

        Kext->ImageData = OcStorageReadFileUnicode (
          Storage,
          FullPath,
          &Kext->ImageDataSize
          );

        if (Kext->ImageData == NULL) {
          DEBUG ((
            DEBUG_ERROR,
            "OC: Image %s is missing for kext %a (%a)\n",
            FullPath,
            BundlePath,
            Comment
            ));
          Kext->Enabled = FALSE;
          FreePool (Kext->PlistData);
          Kext->PlistData = NULL;
          continue;
        }
      }
    }

    if (CacheType == CacheTypeCacheless || CacheType == CacheTypeMkext) {
      Status = MkextReserveKextSize (
        ReservedInfoSize,
        ReservedExeSize,
        Kext->PlistDataSize,
        Kext->ImageData,
        Kext->ImageDataSize
        );
    } else if (CacheType == CacheTypePrelinked) {
      Status = PrelinkedReserveKextSize (
        ReservedInfoSize,
        ReservedExeSize,
        Kext->PlistDataSize,
        Kext->ImageData,
        Kext->ImageDataSize
        );
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "OC: Failed to fit kext %a (%a)\n",
        BundlePath,
        Comment
        ));
      Kext->Enabled = FALSE;
      FreePool (Kext->PlistData);
      Kext->PlistData = NULL;
      continue;
    }

    (*NumReservedKexts)++;
  }

  if (CacheType == CacheTypePrelinked) {
    if (*ReservedExeSize > PRELINKED_KEXTS_MAX_SIZE
      || *ReservedInfoSize + *ReservedExeSize < *ReservedExeSize) {
      return EFI_UNSUPPORTED;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "OC: Kext reservation size info %X exe %X\n",
    *ReservedInfoSize, *ReservedExeSize
    ));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
OcKernelApplyQuirk (
  IN     KERNEL_QUIRK_NAME  Quirk,
  IN     KERNEL_CACHE_TYPE  CacheType,
  IN OUT VOID               *Context,
  IN OUT PATCHER_CONTEXT    *KernelPatcher
  )
{
  //
  // Apply kernel quirks to kernel, kext patches to context.
  //
  if (Context == NULL) {
    ASSERT (KernelPatcher != NULL);
    return KernelApplyQuirk (Quirk, KernelPatcher, mOcDarwinVersion);
  }

  if (CacheType == CacheTypeCacheless) {
    return CachelessContextAddQuirk (Context, Quirk);
  }
  
  if (CacheType == CacheTypeMkext) {
    return MkextContextApplyQuirk (Context, Quirk, mOcDarwinVersion);
  }

  if (CacheType == CacheTypePrelinked) {
    return PrelinkedContextApplyQuirk (Context, Quirk, mOcDarwinVersion);
  }

  return EFI_UNSUPPORTED;
}

STATIC
VOID
OcKernelApplyPatches (
  IN     OC_GLOBAL_CONFIG  *Config,
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
        Status = CachelessContextAddPatch ((CACHELESS_CONTEXT *) Context, Target, &Patch);
      } else if (CacheType == CacheTypeMkext) {
        Status = MkextContextApplyPatch ((MKEXT_CONTEXT *) Context, Target, &Patch);
      } else if (CacheType == CacheTypePrelinked) {
        Status = PrelinkedContextApplyPatch ((PRELINKED_CONTEXT *) Context, Target, &Patch);
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
      OcKernelApplyQuirk (KernelQuirkAppleCpuPmCfgLock, CacheType, Context, NULL);
    }

    if (Config->Kernel.Quirks.ExternalDiskIcons) {
      OcKernelApplyQuirk (KernelQuirkExternalDiskIcons, CacheType, Context, NULL);
    }

    if (Config->Kernel.Quirks.ThirdPartyDrives) {
      OcKernelApplyQuirk (KernelQuirkThirdPartyDrives, CacheType, Context, NULL);
    }

    if (Config->Kernel.Quirks.XhciPortLimit) {
      OcKernelApplyQuirk (KernelQuirkXhciPortLimit1, CacheType, Context, NULL);
      OcKernelApplyQuirk (KernelQuirkXhciPortLimit2, CacheType, Context, NULL);
      OcKernelApplyQuirk (KernelQuirkXhciPortLimit3, CacheType, Context, NULL);
    }

    if (Config->Kernel.Quirks.DisableIoMapper) {
      OcKernelApplyQuirk (KernelQuirkDisableIoMapper, CacheType, Context, NULL);
    }

    if (Config->Kernel.Quirks.DisableRtcChecksum) {
      OcKernelApplyQuirk (KernelQuirkDisableRtcChecksum, CacheType, Context, NULL);
    }

    if (Config->Kernel.Quirks.IncreasePciBarSize) {
      OcKernelApplyQuirk (KernelQuirkIncreasePciBarSize, CacheType, Context, NULL);     
    }

    if (Config->Kernel.Quirks.CustomSmbiosGuid) {
      OcKernelApplyQuirk (KernelQuirkCustomSmbiosGuid1, CacheType, Context, NULL);
      OcKernelApplyQuirk (KernelQuirkCustomSmbiosGuid2, CacheType, Context, NULL);
    }

    if (Config->Kernel.Quirks.DummyPowerManagement) {
      OcKernelApplyQuirk (KernelQuirkDummyPowerManagement, CacheType, Context, NULL);
    }
  } else {
    if (Config->Kernel.Quirks.AppleXcpmCfgLock) {
      OcKernelApplyQuirk (KernelQuirkAppleXcpmCfgLock, CacheType, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Quirks.AppleXcpmExtraMsrs) {
      OcKernelApplyQuirk (KernelQuirkAppleXcpmExtraMsrs, CacheType, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Quirks.AppleXcpmForceBoost) {
      OcKernelApplyQuirk (KernelQuirkAppleXcpmForceBoost, CacheType, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Quirks.PanicNoKextDump) {
      OcKernelApplyQuirk (KernelQuirkPanicNoKextDump, CacheType, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Emulate.Cpuid1Data[0] != 0
      || Config->Kernel.Emulate.Cpuid1Data[1] != 0
      || Config->Kernel.Emulate.Cpuid1Data[2] != 0
      || Config->Kernel.Emulate.Cpuid1Data[3] != 0) {
      PatchKernelCpuId (
        &KernelPatcher,
        mOcCpuInfo,
        Config->Kernel.Emulate.Cpuid1Data,
        Config->Kernel.Emulate.Cpuid1Mask
        );
    }

    if (Config->Kernel.Quirks.LapicKernelPanic) {
      OcKernelApplyQuirk (KernelQuirkLapicKernelPanic, CacheType, NULL, &KernelPatcher);
    }

    if (Config->Kernel.Quirks.PowerTimeoutKernelPanic) {
      OcKernelApplyQuirk (KernelQuirkPowerTimeoutKernelPanic, CacheType, NULL, &KernelPatcher);
    }
  }
}

STATIC
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

STATIC
EFI_STATUS
OcKernelProcessPrelinked (
  IN     OC_GLOBAL_CONFIG  *Config,
  IN     UINT32            DarwinVersion,
  IN OUT UINT8             *Kernel,
  IN     UINT32            *KernelSize,
  IN     UINT32            AllocatedSize,
  IN     UINT32            LinkedExpansion,
  IN     UINT32            ReservedExeSize
  )
{
  EFI_STATUS           Status;
  PRELINKED_CONTEXT    Context;
  CHAR8                *BundlePath;
  CHAR8                *ExecutablePath;
  CHAR8                *Comment;
  UINT32               Index;
  CHAR8                FullPath[OC_STORAGE_SAFE_PATH_MAX];
  OC_KERNEL_ADD_ENTRY  *Kext;
  UINT32               MaxKernel;
  UINT32               MinKernel;

  Status = PrelinkedContextInit (&Context, Kernel, *KernelSize, AllocatedSize);

  if (!EFI_ERROR (Status)) {
    OcKernelApplyPatches (Config, DarwinVersion, CacheTypePrelinked, &Context, NULL, 0);

    OcKernelBlockKexts (Config, DarwinVersion, &Context);

    Status = PrelinkedInjectPrepare (
      &Context,
      LinkedExpansion,
      ReservedExeSize
      );
    if (!EFI_ERROR (Status)) {
      for (Index = 0; Index < Config->Kernel.Add.Count; ++Index) {
        Kext = Config->Kernel.Add.Values[Index];

        if (!Kext->Enabled || Kext->PlistDataSize == 0) {
          continue;
        }

        BundlePath  = OC_BLOB_GET (&Kext->BundlePath);
        Comment     = OC_BLOB_GET (&Kext->Comment);
        MaxKernel   = OcParseDarwinVersion (OC_BLOB_GET (&Kext->MaxKernel));
        MinKernel   = OcParseDarwinVersion (OC_BLOB_GET (&Kext->MinKernel));

        if (!OcMatchDarwinVersion (DarwinVersion, MinKernel, MaxKernel)) {
          DEBUG ((
            DEBUG_INFO,
            "OC: Prelink injection skips %a (%a) kext at %u due to version %u <= %u <= %u\n",
            BundlePath,
            Comment,
            Index,
            MinKernel,
            DarwinVersion,
            MaxKernel
            ));
          continue;
        }

        Status = OcAsciiSafeSPrint (FullPath, sizeof (FullPath), "/Library/Extensions/%a", BundlePath);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN, "OC: Failed to fit kext path /Library/Extensions/%a", BundlePath));
          continue;
        }

        if (Kext->ImageData != NULL) {
          ExecutablePath = OC_BLOB_GET (&Kext->ExecutablePath);
        } else {
          ExecutablePath = NULL;
        }

        Status = PrelinkedInjectKext (
          &Context,
          FullPath,
          Kext->PlistData,
          Kext->PlistDataSize,
          ExecutablePath,
          Kext->ImageData,
          Kext->ImageDataSize
          );

        DEBUG ((
          EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
          "OC: Prelink injection %a (%a) - %r\n",
          BundlePath,
          Comment,
          Status
          ));
      }

      DEBUG ((
        DEBUG_INFO,
        "OC: Prelink size %u kext offset %u reserved %u\n",
        Context.PrelinkedSize,
        Context.KextsFileOffset,
        ReservedExeSize
        ));

      ASSERT (Context.PrelinkedSize - Context.KextsFileOffset <= ReservedExeSize);

      Status = PrelinkedInjectComplete (&Context);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OC: Prelink insertion error - %r\n", Status));
      }
    } else {
      DEBUG ((DEBUG_WARN, "OC: Prelink inject prepare error - %r\n", Status));
    }

    *KernelSize = Context.PrelinkedSize;

    PrelinkedContextFree (&Context);
  }

  return Status;
}

STATIC
EFI_STATUS
OcKernelProcessMkext (
  IN     OC_GLOBAL_CONFIG  *Config,
  IN     UINT32            DarwinVersion,
  IN OUT UINT8             *Mkext,
  IN OUT UINT32            *MkextSize,
  IN     UINT32            AllocatedSize
  )
{
  EFI_STATUS            Status;
  MKEXT_CONTEXT         Context;
  CHAR8                 *BundlePath;
  CHAR8                 *Comment;
  UINT32                Index;
  CHAR8                 FullPath[OC_STORAGE_SAFE_PATH_MAX];
  OC_KERNEL_ADD_ENTRY   *Kext;
  UINT32                MaxKernel;
  UINT32                MinKernel;

  Status = MkextContextInit (&Context, Mkext, *MkextSize, AllocatedSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OcKernelApplyPatches (Config, DarwinVersion, CacheTypeMkext, &Context, NULL, 0);

  for (Index = 0; Index < Config->Kernel.Add.Count; ++Index) {
    Kext = Config->Kernel.Add.Values[Index];

    if (!Kext->Enabled || Kext->PlistDataSize == 0) {
      continue;
    }

    BundlePath  = OC_BLOB_GET (&Kext->BundlePath);
    Comment     = OC_BLOB_GET (&Kext->Comment);
    MaxKernel   = OcParseDarwinVersion (OC_BLOB_GET (&Kext->MaxKernel));
    MinKernel   = OcParseDarwinVersion (OC_BLOB_GET (&Kext->MinKernel));

    if (!OcMatchDarwinVersion (DarwinVersion, MinKernel, MaxKernel)) {
      DEBUG ((
        DEBUG_INFO,
        "OC: Mkext injection skips %a (%a) kext at %u due to version %u <= %u <= %u\n",
        BundlePath,
        Comment,
        Index,
        MinKernel,
        DarwinVersion,
        MaxKernel
        ));
      continue;
    }

    Status = OcAsciiSafeSPrint (FullPath, sizeof (FullPath), "/Library/Extensions/%a", BundlePath);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OC: Failed to fit kext path /Library/Extensions/%a", BundlePath));
      continue;
    }

    Status = MkextInjectKext (
      &Context,
      FullPath,
      Kext->PlistData,
      Kext->PlistDataSize,
      Kext->ImageData,
      Kext->ImageDataSize
      );

    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
      "OC: Mkext injection %a (%a) - %r\n",
      BundlePath,
      Comment,
      Status
      ));
  }

  Status = MkextInjectPatchComplete (&Context);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OC: Mkext insertion error - %r\n", Status));
  }

  *MkextSize = Context.MkextSize;

  MkextContextFree (&Context);
  return Status;
}

STATIC
EFI_STATUS
OcKernelInitCacheless (
  IN     OC_GLOBAL_CONFIG       *Config,
  IN     CACHELESS_CONTEXT      *Context,
  IN     UINT32                 DarwinVersion,
  IN     CHAR16                 *FileName,
  IN     EFI_FILE_PROTOCOL      *ExtensionsDir,
     OUT EFI_FILE_PROTOCOL      **File
  )
{
  EFI_STATUS            Status;
  UINT32                Index;

  OC_KERNEL_ADD_ENTRY   *Kext;
  CHAR8                 *BundlePath;
  CHAR8                 *Comment;
  UINT32                MaxKernel;
  UINT32                MinKernel;

  Status = CachelessContextInit (
    Context,
    FileName,
    ExtensionsDir,
    DarwinVersion
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Add kexts into cacheless context.
  //
  for (Index = 0; Index < Config->Kernel.Add.Count; Index++) {
    Kext = Config->Kernel.Add.Values[Index];

    if (!Kext->Enabled || Kext->PlistDataSize == 0) {
      continue;
    }

    BundlePath  = OC_BLOB_GET (&Kext->BundlePath);
    Comment     = OC_BLOB_GET (&Kext->Comment);
    MaxKernel   = OcParseDarwinVersion (OC_BLOB_GET (&Kext->MaxKernel));
    MinKernel   = OcParseDarwinVersion (OC_BLOB_GET (&Kext->MinKernel));

    if (!OcMatchDarwinVersion (DarwinVersion, MinKernel, MaxKernel)) {
      DEBUG ((
        DEBUG_INFO,
        "OC: Cacheless injection skips %a (%a) kext at %u due to version %u <= %u <= %u\n",
        BundlePath,
        Comment,
        Index,
        MinKernel,
        DarwinVersion,
        MaxKernel
        ));
      continue;
    }

    Status = CachelessContextAddKext (
      Context,
      Kext->PlistData,
      Kext->PlistDataSize,
      Kext->ImageData,
      Kext->ImageDataSize
      );
    if (EFI_ERROR (Status)) {
      CachelessContextFree (Context);
      return Status;
    }
  }

  //
  // Process patches and blocks.
  //
  OcKernelApplyPatches (Config, DarwinVersion, CacheTypeCacheless, Context, NULL, 0);

  return CachelessContextOverlayExtensionsDir (Context, File);
}

STATIC
EFI_STATUS
EFIAPI
OcKernelFileOpen (
  IN  EFI_FILE_PROTOCOL       *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CHAR16                  *FileName,
  IN  UINT64                  OpenMode,
  IN  UINT64                  Attributes
  )
{
  EFI_STATUS         Status;
  EFI_STATUS         Status2;
  CONST CHAR8        *ForceCacheType;
  KERNEL_CACHE_TYPE  MaxCacheTypeAllowed;
  BOOLEAN            Result;
  UINT32             DarwinVersion;
  BOOLEAN            IsKernel32Bit;
  UINT8              *Kernel;
  UINT32             KernelSize;
  UINT32             AllocatedSize;
  EFI_FILE_PROTOCOL  *VirtualFileHandle;
  EFI_STATUS         PrelinkedStatus;
  EFI_TIME           ModificationTime;
  UINT32             ReservedInfoSize;
  UINT32             ReservedExeSize;
  UINT32             NumReservedKexts;
  UINT32             LinkedExpansion;
  UINT32             ReservedFullSize;

  //
  // Prevent access to cache files depending on maximum cache type allowed.
  //
  ForceCacheType = OC_BLOB_GET (&mOcConfiguration->Kernel.Quirks.ForceKernelCache);
  if (AsciiStrCmp (ForceCacheType, "Cacheless") == 0) {
    MaxCacheTypeAllowed = CacheTypeCacheless;
  } else if (AsciiStrCmp (ForceCacheType, "Mkext") == 0) {
    MaxCacheTypeAllowed = CacheTypeMkext;
  } else {
    MaxCacheTypeAllowed = CacheTypePrelinked;
  }

  //
  // Hook injected OcXXXXXXXX.kext reads from /S/L/E.
  //
  if (mOcCachelessInProgress
    && OpenMode == EFI_FILE_MODE_READ
    && StrnCmp (FileName, L"System\\Library\\Extensions\\Oc", L_STR_LEN (L"System\\Library\\Extensions\\Oc")) == 0) {
    Status = CachelessContextPerformInject (&mOcCachelessContext, FileName, NewHandle);
    DEBUG ((
      DEBUG_INFO,
      "OC: Hooking SLE injected file %s with %u mode gave - %r\n",
      FileName,
      (UINT32) OpenMode,
      Status
      ));

    return Status;
  }

  Status = SafeFileOpen (This, NewHandle, FileName, OpenMode, Attributes);

  DEBUG ((
    DEBUG_VERBOSE,
    "OC: Opening file %s with %u mode gave - %r\n",
    FileName,
    (UINT32) OpenMode,
    Status
    ));

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // boot.efi uses /S/L/K/kernel as is to determine valid filesystem.
  // Just skip it to speedup the boot process.
  // On 10.9 mach_kernel is loaded for manual linking aferwards, so we cannot skip it.
  // We also want to skip files named "kernel" that are part of kext bundles, and im4m.
  //
  if (OpenMode == EFI_FILE_MODE_READ
    && OcStriStr (FileName, L"kernel") != NULL
    && StrCmp (FileName, L"System\\Library\\Kernels\\kernel") != 0
    && OcStriStr (FileName, L".kext\\") == NULL
    && OcStriStr (FileName, L".im4m") == NULL) {

    OcKernelLoadKextsAndReserve (
      mOcStorage,
      mOcConfiguration,
      CacheTypePrelinked,
      &ReservedExeSize,
      &ReservedInfoSize,
      &NumReservedKexts
      );

    LinkedExpansion = KcGetSegmentFixupChainsSize (ReservedExeSize);
    if (LinkedExpansion == 0) {
      return EFI_UNSUPPORTED;
    }

    Result = OcOverflowTriAddU32 (
      ReservedInfoSize,
      ReservedExeSize,
      LinkedExpansion,
      &ReservedFullSize
      );
    if (Result) {
      return EFI_UNSUPPORTED;
    }

    //
    // Read last used architecture for kernel.
    //
    DEBUG ((DEBUG_INFO, "OC: Trying %a XNU hook on %s\n", mUse32BitKernel ? "32-bit" : "64-bit", FileName));
    Status = ReadAppleKernel (
      *NewHandle,
      mUse32BitKernel,
      &IsKernel32Bit,
      &Kernel,
      &KernelSize,
      &AllocatedSize,
      ReservedFullSize,
      mKernelDigest
      );
    DEBUG ((
      DEBUG_INFO,
      "OC: Result of %a XNU hook on %s (%02X%02X%02X%02X) is %r\n",
      IsKernel32Bit ? "32-bit" : "64-bit",
      FileName,
      mKernelDigest[0],
      mKernelDigest[1],
      mKernelDigest[2],
      mKernelDigest[3],
      Status
      ));

    if (!EFI_ERROR (Status)) {
      //
      // Recheck kernel version and expected vs actual bitness returned. If either of those differ,
      // re-evaluate whether we can run 64-bit kernels on this platform.
      //
      DarwinVersion = OcKernelReadDarwinVersion (Kernel, KernelSize);
      if (DarwinVersion != mOcDarwinVersion || mUse32BitKernel != IsKernel32Bit) {
        //
        // Query command line arch= argument and fallback to SMBIOS checking.
        // Arch argument will force the desired arch.
        //
        Status2 = OcAbcIs32BitPreferred (&mUse32BitKernel);
        if (EFI_ERROR (Status2)) {
          mUse32BitKernel = !OcPlatformIs64BitSupported (DarwinVersion);
        }

        //
        // If a different kernel arch is required, but we did not originally read it,
        // we'll need to try to get the kernel again.
        //
        if (mUse32BitKernel != IsKernel32Bit) {
          FreePool (Kernel);

          DEBUG ((DEBUG_INFO, "OC: Wrong arch read, retrying %a XNU hook on %s\n", mUse32BitKernel ? "32-bit" : "64-bit", FileName));
          Status = ReadAppleKernel (
            *NewHandle,
            mUse32BitKernel,
            &IsKernel32Bit,
            &Kernel,
            &KernelSize,
            &AllocatedSize,
            ReservedFullSize,
            mKernelDigest
            );
          DEBUG ((
            DEBUG_INFO,
            "OC: Result of %a XNU hook on %s (%02X%02X%02X%02X) is %r\n",
            IsKernel32Bit ? "32-bit" : "64-bit",
            FileName,
            mKernelDigest[0],
            mKernelDigest[1],
            mKernelDigest[2],
            mKernelDigest[3],
            Status
            ));

          if (!EFI_ERROR (Status)) {
            DarwinVersion = OcKernelReadDarwinVersion (Kernel, KernelSize);

            //
            // Ensure we are now matching the requested arch.
            //
            if (mUse32BitKernel != IsKernel32Bit) {
              DEBUG ((DEBUG_WARN, "OC: %a kernel architecture is not available, aborting.\n", mUse32BitKernel ? "32-bit" : "64-bit"));
              FreePool (Kernel);
              return EFI_INVALID_PARAMETER;
            }
          }
        }

        mOcDarwinVersion = DarwinVersion;
      }
    }

    if (!EFI_ERROR (Status)) {
      //
      // Disable prelinked if forcing mkext or cacheless, but only on appropriate versions.
      //
      if ((OcStriStr (FileName, L"kernelcache") != NULL || OcStriStr (FileName, L"prelinkedkernel") != NULL)
        && ((MaxCacheTypeAllowed == CacheTypeMkext && mOcDarwinVersion <= KERNEL_VERSION_SNOW_LEOPARD_MAX)
        || (MaxCacheTypeAllowed == CacheTypeCacheless && mOcDarwinVersion <= KERNEL_VERSION_MAVERICKS_MAX))) {
        DEBUG ((DEBUG_INFO, "OC: Blocking prelinked due to ForceKernelCache=%s: %a\n", FileName, ForceCacheType));

        FreePool (Kernel);
        (*NewHandle)->Close(*NewHandle);
        *NewHandle = NULL;

        return EFI_NOT_FOUND;
      }

      OcKernelApplyPatches (mOcConfiguration, mOcDarwinVersion, 0, NULL, Kernel, KernelSize);
      PrelinkedStatus = OcKernelProcessPrelinked (
        mOcConfiguration,
        mOcDarwinVersion,
        Kernel,
        &KernelSize,
        AllocatedSize,
        LinkedExpansion,
        ReservedExeSize
        );

      DEBUG ((DEBUG_INFO, "OC: Prelinked status - %r\n", PrelinkedStatus));

      Status = GetFileModificationTime (*NewHandle, &ModificationTime);
      if (EFI_ERROR (Status)) {
        ZeroMem (&ModificationTime, sizeof (ModificationTime));
      }

      (*NewHandle)->Close(*NewHandle);

      //
      // Virtualise newly created kernel.
      //
      Status = CreateVirtualFileFileNameCopy (FileName, Kernel, KernelSize, &ModificationTime, &VirtualFileHandle);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OC: Failed to virtualise kernel file (%s) - %r\n", FileName, Status));
        FreePool (Kernel);
        return EFI_OUT_OF_RESOURCES;
      }

      OcAppleImg4RegisterOverride (mKernelDigest, Kernel, KernelSize);

      //
      // Return our handle.
      //
      *NewHandle = VirtualFileHandle;
      return EFI_SUCCESS;
    }
  }

  if (OpenMode == EFI_FILE_MODE_READ
    && OcStriStr (FileName, L"Extensions.mkext") != NULL) {

    //
    // Disable mkext booting if forcing cacheless.
    //
    if (MaxCacheTypeAllowed == CacheTypeCacheless) {
      DEBUG ((DEBUG_INFO, "OC: Blocking mkext due to ForceKernelCache=%s: %a\n", FileName, ForceCacheType));
      (*NewHandle)->Close(*NewHandle);
      *NewHandle = NULL;

      return EFI_NOT_FOUND;
    }
    
    OcKernelLoadKextsAndReserve (
      mOcStorage,
      mOcConfiguration,
      CacheTypeMkext,
      &ReservedExeSize,
      &ReservedInfoSize,
      &NumReservedKexts
      );

    Result = OcOverflowAddU32 (
      ReservedInfoSize,
      ReservedExeSize,
      &ReservedFullSize
      );
    if (Result) {
      return EFI_UNSUPPORTED;
    }

    DEBUG ((DEBUG_INFO, "OC: Trying %a mkext hook on %s\n", mUse32BitKernel ? "32-bit" : "64-bit", FileName));
    Status = ReadAppleMkext (
      *NewHandle,
      mUse32BitKernel,
      &Kernel,
      &KernelSize,
      &AllocatedSize,
      ReservedFullSize,
      NumReservedKexts
      );
    DEBUG ((DEBUG_INFO, "OC: Result of mkext hook on %s is %r\n", FileName, Status));

    if (!EFI_ERROR (Status)) {
      //
      // Process mkext.
      //
      Status = OcKernelProcessMkext (mOcConfiguration, mOcDarwinVersion, Kernel, &KernelSize, AllocatedSize);
      DEBUG ((DEBUG_INFO, "OC: Mkext status - %r\n", Status));
      if (!EFI_ERROR (Status)) {
        Status = GetFileModificationTime (*NewHandle, &ModificationTime);
        if (EFI_ERROR (Status)) {
          ZeroMem (&ModificationTime, sizeof (ModificationTime));
        }

        (*NewHandle)->Close(*NewHandle);

        //
        // Virtualise newly created mkext.
        //
        Status = CreateVirtualFileFileNameCopy (FileName, Kernel, KernelSize, &ModificationTime, &VirtualFileHandle);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_WARN, "OC: Failed to virtualise mkext file (%s) - %r\n", FileName, Status));
          FreePool (Kernel);
          return EFI_OUT_OF_RESOURCES;
        }

        *NewHandle = VirtualFileHandle;
        return EFI_SUCCESS;
      } else {
        FreePool (Kernel);
      }
    }
  }

  //
  // Hook /S/L/E for cacheless boots.
  //
  if (OpenMode == EFI_FILE_MODE_READ
    && StrCmp (FileName, L"System\\Library\\Extensions") == 0) {

    //
    // Free existing context if we are re-opening Extensions directory.
    //
    if (mOcCachelessInProgress) {
      CachelessContextFree (&mOcCachelessContext);
    }
    mOcCachelessInProgress = FALSE;

    OcKernelLoadKextsAndReserve (
      mOcStorage,
      mOcConfiguration,
      CacheTypeCacheless,
      &ReservedExeSize,
      &ReservedInfoSize,
      &NumReservedKexts
      );

    //
    // Initialize Extensions directory overlay for cacheless injection.
    //
    Status = OcKernelInitCacheless (
      mOcConfiguration,
      &mOcCachelessContext,
      mOcDarwinVersion,
      FileName,
      *NewHandle,
      &VirtualFileHandle
      );
    
    DEBUG ((DEBUG_INFO, "OC: Result of SLE hook on %s is %r\n", FileName, Status));

    if (!EFI_ERROR (Status)) {
      mOcCachelessInProgress  = TRUE;
      *NewHandle              = VirtualFileHandle;
      return EFI_SUCCESS;
    }
  }

  //
  // Hook /S/L/E contents for processing during cacheless boots.
  //
  if (mOcCachelessInProgress
    && OpenMode == EFI_FILE_MODE_READ
    && StrnCmp (FileName, L"System\\Library\\Extensions\\", L_STR_LEN (L"System\\Library\\Extensions\\")) == 0) {
      Status = CachelessContextHookBuiltin (
        &mOcCachelessContext,
        FileName,
        *NewHandle,
        &VirtualFileHandle
        );

      if (!EFI_ERROR (Status) && VirtualFileHandle != NULL) {
        *NewHandle = VirtualFileHandle;
        return EFI_SUCCESS;
      }
  }

  //
  // This is not Apple kernel, just return the original file.
  // We recurse the filtering to additionally catch com.apple.boot.[RPS] directories.
  //
  return CreateRealFile (*NewHandle, OcKernelFileOpen, TRUE, NewHandle);
}

VOID
OcLoadKernelSupport (
  IN OC_STORAGE_CONTEXT  *Storage,
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo
  )
{
  EFI_STATUS  Status;

  Status = EnableVirtualFs (gBS, OcKernelFileOpen);

  if (!EFI_ERROR (Status)) {
    mOcStorage              = Storage;
    mOcConfiguration        = Config;
    mOcCpuInfo              = CpuInfo;
    mOcDarwinVersion        = 0;
    mOcCachelessInProgress  = FALSE;

#if defined(MDE_CPU_IA32)
    mUse32BitKernel         = TRUE;
#elif defined(MDE_CPU_X64)
    mUse32BitKernel         = FALSE;
#else
#error "Unsupported architecture"
#endif
  } else {
    DEBUG ((DEBUG_ERROR, "OC: Failed to enable vfs - %r\n", Status));
  }
}

VOID
OcUnloadKernelSupport (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mOcStorage != NULL) {
    Status = DisableVirtualFs (gBS);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OC: Failed to disable vfs - %r\n", Status));
    }
    mOcStorage       = NULL;
    mOcConfiguration = NULL;
  }
}
