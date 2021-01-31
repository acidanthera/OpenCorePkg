/** @file
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/DebugLib.h>
#include <Library/OcTemplateLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcAppleKernelLib.h>

#include <string.h>
#include <sys/time.h>

#include <UserFile.h>

STATIC BOOLEAN FailedToProcess = FALSE;
STATIC UINT32  KernelVersion   = 0;

STATIC CHAR8 KextInfoPlistData[] = {
  0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20, 0x76, 0x65,
  0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x31,
  0x2E, 0x30, 0x22, 0x20, 0x65, 0x6E, 0x63, 0x6F,
  0x64, 0x69, 0x6E, 0x67, 0x3D, 0x22, 0x55, 0x54,
  0x46, 0x2D, 0x38, 0x22, 0x3F, 0x3E, 0x0D, 0x3C,
  0x21, 0x44, 0x4F, 0x43, 0x54, 0x59, 0x50, 0x45,
  0x20, 0x70, 0x6C, 0x69, 0x73, 0x74, 0x20, 0x50,
  0x55, 0x42, 0x4C, 0x49, 0x43, 0x20, 0x22, 0x2D,
  0x2F, 0x2F, 0x41, 0x70, 0x70, 0x6C, 0x65, 0x2F,
  0x2F, 0x44, 0x54, 0x44, 0x20, 0x50, 0x4C, 0x49,
  0x53, 0x54, 0x20, 0x31, 0x2E, 0x30, 0x2F, 0x2F,
  0x45, 0x4E, 0x22, 0x20, 0x22, 0x68, 0x74, 0x74,
  0x70, 0x3A, 0x2F, 0x2F, 0x77, 0x77, 0x77, 0x2E,
  0x61, 0x70, 0x70, 0x6C, 0x65, 0x2E, 0x63, 0x6F,
  0x6D, 0x2F, 0x44, 0x54, 0x44, 0x73, 0x2F, 0x50,
  0x72, 0x6F, 0x70, 0x65, 0x72, 0x74, 0x79, 0x4C,
  0x69, 0x73, 0x74, 0x2D, 0x31, 0x2E, 0x30, 0x2E,
  0x64, 0x74, 0x64, 0x22, 0x3E, 0x0D, 0x3C, 0x70,
  0x6C, 0x69, 0x73, 0x74, 0x20, 0x76, 0x65, 0x72,
  0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x31, 0x2E,
  0x30, 0x22, 0x3E, 0x0D, 0x3C, 0x64, 0x69, 0x63,
  0x74, 0x3E, 0x0D, 0x09, 0x3C, 0x6B, 0x65, 0x79,
  0x3E, 0x43, 0x46, 0x42, 0x75, 0x6E, 0x64, 0x6C,
  0x65, 0x49, 0x64, 0x65, 0x6E, 0x74, 0x69, 0x66,
  0x69, 0x65, 0x72, 0x3C, 0x2F, 0x6B, 0x65, 0x79,
  0x3E, 0x0D, 0x09, 0x3C, 0x73, 0x74, 0x72, 0x69,
  0x6E, 0x67, 0x3E, 0x61, 0x73, 0x2E, 0x76, 0x69,
  0x74, 0x39, 0x36, 0x39, 0x36, 0x2E, 0x54, 0x65,
  0x73, 0x74, 0x44, 0x72, 0x69, 0x76, 0x65, 0x72,
  0x3C, 0x2F, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,
  0x3E, 0x0D, 0x09, 0x3C, 0x6B, 0x65, 0x79, 0x3E,
  0x43, 0x46, 0x42, 0x75, 0x6E, 0x64, 0x6C, 0x65,
  0x49, 0x6E, 0x66, 0x6F, 0x44, 0x69, 0x63, 0x74,
  0x69, 0x6F, 0x6E, 0x61, 0x72, 0x79, 0x56, 0x65,
  0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3C, 0x2F, 0x6B,
  0x65, 0x79, 0x3E, 0x0D, 0x09, 0x3C, 0x73, 0x74,
  0x72, 0x69, 0x6E, 0x67, 0x3E, 0x36, 0x2E, 0x30,
  0x3C, 0x2F, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,
  0x3E, 0x0D, 0x09, 0x3C, 0x6B, 0x65, 0x79, 0x3E,
  0x43, 0x46, 0x42, 0x75, 0x6E, 0x64, 0x6C, 0x65,
  0x4E, 0x61, 0x6D, 0x65, 0x3C, 0x2F, 0x6B, 0x65,
  0x79, 0x3E, 0x0D, 0x09, 0x3C, 0x73, 0x74, 0x72,
  0x69, 0x6E, 0x67, 0x3E, 0x43, 0x50, 0x55, 0x46,
  0x72, 0x69, 0x65, 0x6E, 0x64, 0x44, 0x61, 0x74,
  0x61, 0x50, 0x72, 0x6F, 0x76, 0x69, 0x64, 0x65,
  0x72, 0x3C, 0x2F, 0x73, 0x74, 0x72, 0x69, 0x6E,
  0x67, 0x3E, 0x0D, 0x09, 0x3C, 0x6B, 0x65, 0x79,
  0x3E, 0x43, 0x46, 0x42, 0x75, 0x6E, 0x64, 0x6C,
  0x65, 0x50, 0x61, 0x63, 0x6B, 0x61, 0x67, 0x65,
  0x54, 0x79, 0x70, 0x65, 0x3C, 0x2F, 0x6B, 0x65,
  0x79, 0x3E, 0x0D, 0x09, 0x3C, 0x73, 0x74, 0x72,
  0x69, 0x6E, 0x67, 0x3E, 0x4B, 0x45, 0x58, 0x54,
  0x3C, 0x2F, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,
  0x3E, 0x0D, 0x09, 0x3C, 0x6B, 0x65, 0x79, 0x3E,
  0x43, 0x46, 0x42, 0x75, 0x6E, 0x64, 0x6C, 0x65,
  0x53, 0x68, 0x6F, 0x72, 0x74, 0x56, 0x65, 0x72,
  0x73, 0x69, 0x6F, 0x6E, 0x53, 0x74, 0x72, 0x69,
  0x6E, 0x67, 0x3C, 0x2F, 0x6B, 0x65, 0x79, 0x3E,
  0x0D, 0x09, 0x3C, 0x73, 0x74, 0x72, 0x69, 0x6E,
  0x67, 0x3E, 0x31, 0x2E, 0x30, 0x2E, 0x30, 0x3C,
  0x2F, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3E,
  0x0D, 0x09, 0x3C, 0x6B, 0x65, 0x79, 0x3E, 0x43,
  0x46, 0x42, 0x75, 0x6E, 0x64, 0x6C, 0x65, 0x56,
  0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3C, 0x2F,
  0x6B, 0x65, 0x79, 0x3E, 0x0D, 0x09, 0x3C, 0x73,
  0x74, 0x72, 0x69, 0x6E, 0x67, 0x3E, 0x31, 0x2E,
  0x30, 0x2E, 0x30, 0x3C, 0x2F, 0x73, 0x74, 0x72,
  0x69, 0x6E, 0x67, 0x3E, 0x0D, 0x09, 0x3C, 0x6B,
  0x65, 0x79, 0x3E, 0x49, 0x4F, 0x4B, 0x69, 0x74,
  0x50, 0x65, 0x72, 0x73, 0x6F, 0x6E, 0x61, 0x6C,
  0x69, 0x74, 0x69, 0x65, 0x73, 0x3C, 0x2F, 0x6B,
  0x65, 0x79, 0x3E, 0x0D, 0x09, 0x3C, 0x64, 0x69,
  0x63, 0x74, 0x3E, 0x0D, 0x09, 0x09, 0x3C, 0x6B,
  0x65, 0x79, 0x3E, 0x54, 0x65, 0x73, 0x74, 0x44,
  0x61, 0x74, 0x61, 0x50, 0x72, 0x6F, 0x76, 0x69,
  0x64, 0x65, 0x72, 0x3C, 0x2F, 0x6B, 0x65, 0x79,
  0x3E, 0x0D, 0x09, 0x09, 0x3C, 0x64, 0x69, 0x63,
  0x74, 0x3E, 0x0D, 0x09, 0x09, 0x09, 0x3C, 0x6B,
  0x65, 0x79, 0x3E, 0x43, 0x46, 0x42, 0x75, 0x6E,
  0x64, 0x6C, 0x65, 0x49, 0x64, 0x65, 0x6E, 0x74,
  0x69, 0x66, 0x69, 0x65, 0x72, 0x3C, 0x2F, 0x6B,
  0x65, 0x79, 0x3E, 0x0D, 0x09, 0x09, 0x09, 0x3C,
  0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3E, 0x63,
  0x6F, 0x6D, 0x2E, 0x61, 0x70, 0x70, 0x6C, 0x65,
  0x2E, 0x64, 0x72, 0x69, 0x76, 0x65, 0x72, 0x2E,
  0x41, 0x70, 0x70, 0x6C, 0x65, 0x41, 0x43, 0x50,
  0x49, 0x50, 0x6C, 0x61, 0x74, 0x66, 0x6F, 0x72,
  0x6D, 0x3C, 0x2F, 0x73, 0x74, 0x72, 0x69, 0x6E,
  0x67, 0x3E, 0x0D, 0x09, 0x09, 0x09, 0x3C, 0x6B,
  0x65, 0x79, 0x3E, 0x49, 0x4F, 0x43, 0x6C, 0x61,
  0x73, 0x73, 0x3C, 0x2F, 0x6B, 0x65, 0x79, 0x3E,
  0x0D, 0x09, 0x09, 0x09, 0x3C, 0x73, 0x74, 0x72,
  0x69, 0x6E, 0x67, 0x3E, 0x41, 0x70, 0x70, 0x6C,
  0x65, 0x41, 0x43, 0x50, 0x49, 0x43, 0x50, 0x55,
  0x3C, 0x2F, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,
  0x3E, 0x0D, 0x09, 0x09, 0x09, 0x3C, 0x6B, 0x65,
  0x79, 0x3E, 0x49, 0x4F, 0x4E, 0x61, 0x6D, 0x65,
  0x4D, 0x61, 0x74, 0x63, 0x68, 0x3C, 0x2F, 0x6B,
  0x65, 0x79, 0x3E, 0x0D, 0x09, 0x09, 0x09, 0x3C,
  0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3E, 0x70,
  0x72, 0x6F, 0x63, 0x65, 0x73, 0x73, 0x6F, 0x72,
  0x3C, 0x2F, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,
  0x3E, 0x0D, 0x09, 0x09, 0x09, 0x3C, 0x6B, 0x65,
  0x79, 0x3E, 0x49, 0x4F, 0x50, 0x72, 0x6F, 0x62,
  0x65, 0x53, 0x63, 0x6F, 0x72, 0x65, 0x3C, 0x2F,
  0x6B, 0x65, 0x79, 0x3E, 0x0D, 0x09, 0x09, 0x09,
  0x3C, 0x69, 0x6E, 0x74, 0x65, 0x67, 0x65, 0x72,
  0x3E, 0x31, 0x31, 0x30, 0x30, 0x3C, 0x2F, 0x69,
  0x6E, 0x74, 0x65, 0x67, 0x65, 0x72, 0x3E, 0x0D,
  0x09, 0x09, 0x09, 0x3C, 0x6B, 0x65, 0x79, 0x3E,
  0x49, 0x4F, 0x50, 0x72, 0x6F, 0x76, 0x69, 0x64,
  0x65, 0x72, 0x43, 0x6C, 0x61, 0x73, 0x73, 0x3C,
  0x2F, 0x6B, 0x65, 0x79, 0x3E, 0x0D, 0x09, 0x09,
  0x09, 0x3C, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,
  0x3E, 0x49, 0x4F, 0x41, 0x43, 0x50, 0x49, 0x50,
  0x6C, 0x61, 0x74, 0x66, 0x6F, 0x72, 0x6D, 0x44,
  0x65, 0x76, 0x69, 0x63, 0x65, 0x3C, 0x2F, 0x73,
  0x74, 0x72, 0x69, 0x6E, 0x67, 0x3E, 0x0D, 0x09,
  0x09, 0x09, 0x3C, 0x6B, 0x65, 0x79, 0x3E, 0x4F,
  0x70, 0x65, 0x6E, 0x43, 0x6F, 0x72, 0x65, 0x3C,
  0x2F, 0x6B, 0x65, 0x79, 0x3E, 0x0D, 0x09, 0x09,
  0x09, 0x3C, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,
  0x3E, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57,
  0x6F, 0x72, 0x6C, 0x64, 0x21, 0x3C, 0x2F, 0x73,
  0x74, 0x72, 0x69, 0x6E, 0x67, 0x3E, 0x0D, 0x09,
  0x09, 0x3C, 0x2F, 0x64, 0x69, 0x63, 0x74, 0x3E,
  0x0D, 0x09, 0x3C, 0x2F, 0x64, 0x69, 0x63, 0x74,
  0x3E, 0x0D, 0x09, 0x3C, 0x6B, 0x65, 0x79, 0x3E,
  0x4E, 0x53, 0x48, 0x75, 0x6D, 0x61, 0x6E, 0x52,
  0x65, 0x61, 0x64, 0x61, 0x62, 0x6C, 0x65, 0x43,
  0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74,
  0x3C, 0x2F, 0x6B, 0x65, 0x79, 0x3E, 0x0D, 0x09,
  0x3C, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3E,
  0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68,
  0x74, 0x20, 0xC2, 0xA9, 0x20, 0x32, 0x30, 0x31,
  0x39, 0x20, 0x76, 0x69, 0x74, 0x39, 0x36, 0x39,
  0x36, 0x2E, 0x20, 0x41, 0x6C, 0x6C, 0x20, 0x72,
  0x69, 0x67, 0x68, 0x74, 0x73, 0x20, 0x72, 0x65,
  0x73, 0x65, 0x72, 0x76, 0x65, 0x64, 0x2E, 0x3C,
  0x2F, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3E,
  0x0D, 0x09, 0x3C, 0x6B, 0x65, 0x79, 0x3E, 0x4F,
  0x53, 0x42, 0x75, 0x6E, 0x64, 0x6C, 0x65, 0x52,
  0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x64, 0x3C,
  0x2F, 0x6B, 0x65, 0x79, 0x3E, 0x0D, 0x09, 0x3C,
  0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x3E, 0x52,
  0x6F, 0x6F, 0x74, 0x3C, 0x2F, 0x73, 0x74, 0x72,
  0x69, 0x6E, 0x67, 0x3E, 0x0D, 0x3C, 0x2F, 0x64,
  0x69, 0x63, 0x74, 0x3E, 0x0D, 0x3C, 0x2F, 0x70,
  0x6C, 0x69, 0x73, 0x74, 0x3E
};

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

STATIC
UINT8
DisableIOAHCIPatchReplace[] = {
  0x31, 0xC0, 0xC3 // xor eax, eax ; ret
};

STATIC
PATCHER_GENERIC_PATCH
DisableIOAHCIPatch = {
  .Base    = "__ZN10IOAHCIPort5startEP9IOService",
  .Find    = NULL,
  .Mask    = NULL,
  .Replace = DisableIOAHCIPatchReplace,
  .ReplaceMask = NULL,
  .Size    = sizeof (DisableIOAHCIPatchReplace),
  .Count   = 1,
  .Skip    = 0
};

STATIC
UINT8
DisableKernelLog[] = {
  0xC3
};

STATIC
PATCHER_GENERIC_PATCH
DisableIoLogPatch = {
  .Base    = "_IOLog",
  .Find    = NULL,
  .Mask    = NULL,
  .Replace = DisableKernelLog,
  .Size    = sizeof (DisableKernelLog),
  .Count   = 1,
  .Skip    = 0
};

STATIC
VOID
ApplyKextPatches (
  PRELINKED_CONTEXT  *Context
  )
{
  EFI_STATUS       Status;
  PATCHER_CONTEXT  Patcher;

  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.iokit.IOAHCIFamily"
    );

  if (!EFI_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &DisableIOAHCIPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply patch com.apple.iokit.IOAHCIFamily - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] Patch success com.apple.iokit.IOAHCIFamily\n"));
    }
  } else {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to find com.apple.iokit.IOAHCIFamily - %r\n", Status));
    FailedToProcess = TRUE;
  }

  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.iokit.IOHIDFamily"
    );

  if (!EFI_ERROR (Status)) {
    Status = PatcherBlockKext (&Patcher);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] Failed to block com.apple.iokit.IOHIDFamily - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] Block success com.apple.iokit.IOHIDFamily\n"));
    }
  } else {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to find com.apple.iokit.IOHIDFamily - %r\n", Status));
    FailedToProcess = TRUE;
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkAppleCpuPmCfgLock, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkAppleCpuPmCfgLock - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkAppleCpuPmCfgLock\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkExternalDiskIcons, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkExternalDiskIcons - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkExternalDiskIcons\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkThirdPartyDrives, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkThirdPartyDrives - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkThirdPartyDrives\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkDisableIoMapper, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkDisableIoMapper - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkDisableIoMapper\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkDisableRtcChecksum, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkDisableRtcChecksum - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkDisableRtcChecksum\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkIncreasePciBarSize, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkIncreasePciBarSize - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkIncreasePciBarSize\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkDummyPowerManagement, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkDummyPowerManagement - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkDummyPowerManagement\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkXhciPortLimit1, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkXhciPortLimit1 - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkXhciPortLimit1\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkXhciPortLimit2, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkXhciPortLimit2 - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkXhciPortLimit2\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkXhciPortLimit3, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkXhciPortLimit3 - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkXhciPortLimit3\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkCustomSmbiosGuid1, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkCustomSmbiosGuid1 - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkCustomSmbiosGuid1\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkCustomSmbiosGuid2, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkCustomSmbiosGuid2 - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkCustomSmbiosGuid2\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkExtendBTFeatureFlags, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkExtendBTFeatureFlags - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkExtendBTFeatureFlags\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkForceSecureBootScheme, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkForceSecureBootScheme - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkForceSecureBootScheme\n"));
  }

  Status = PrelinkedContextApplyQuirk (Context, KernelQuirkSetApfsTrimTimeout, KernelVersion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to apply KernelQuirkSetApfsTrimTimeout - %r\n", Status));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Success KernelQuirkSetApfsTrimTimeout\n"));
  }
}

VOID
ApplyKernelPatches (
  IN OUT UINT8   *Kernel,
  IN     UINT32  Size
  )
{
  EFI_STATUS       Status;
  PATCHER_CONTEXT  Patcher;

  Status = PatcherInitContextFromBuffer (
    &Patcher,
    Kernel,
    Size,
    FALSE
    );

  if (!EFI_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &DisableIoLogPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] DisableIoLogPatch kernel patch - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] DisableIoLogPatch kernel patch\n"));
    }

    UINT32  VirtualCpuid[4]     = {0, 0, 0, 0};
    UINT32  VirtualCpuidMask[4] = {0xFFFFFFFF, 0, 0, 0};
    OC_CPU_INFO CpuInfo;
    memset(&CpuInfo, 0, sizeof(CpuInfo));

    Status = PatchKernelCpuId (
      &Patcher,
      &CpuInfo,
      VirtualCpuid,
      VirtualCpuidMask,
      KernelVersion
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] CPUID kernel patch - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] CPUID kernel patch\n"));
    }

    Status = KernelApplyQuirk (KernelQuirkAppleXcpmCfgLock, &Patcher, KernelVersion);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] KernelQuirkAppleXcpmCfgLock - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] KernelQuirkAppleXcpmCfgLock patch\n"));
    }

    Status = KernelApplyQuirk (KernelQuirkAppleXcpmExtraMsrs, &Patcher, KernelVersion);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] KernelQuirkAppleXcpmExtraMsrs - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] KernelQuirkAppleXcpmExtraMsrs patch\n"));
    }

    Status = KernelApplyQuirk (KernelQuirkAppleXcpmForceBoost, &Patcher, KernelVersion);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] KernelQuirkAppleXcpmForceBoost - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] KernelQuirkAppleXcpmForceBoost patch\n"));
    }

    Status = KernelApplyQuirk (KernelQuirkPanicNoKextDump, &Patcher, KernelVersion);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] KernelQuirkPanicNoKextDump - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] KernelQuirkPanicNoKextDump patch\n"));
    }

    Status = KernelApplyQuirk (KernelQuirkLapicKernelPanic, &Patcher, KernelVersion);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] KernelQuirkLapicKernelPanic - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] KernelQuirkLapicKernelPanic patch\n"));
    }


    Status = KernelApplyQuirk (KernelQuirkPowerTimeoutKernelPanic, &Patcher, KernelVersion);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] KernelQuirkPowerTimeoutKernelPanic - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] KernelQuirkPowerTimeoutKernelPanic patch\n"));
    }

    Status = KernelApplyQuirk (KernelQuirkSegmentJettison, &Patcher, KernelVersion);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] KernelQuirkSegmentJettison - %r\n", Status));
      FailedToProcess = TRUE;
    } else {
      DEBUG ((DEBUG_WARN, "[OK] KernelQuirkSegmentJettison patch\n"));
    }
  } else {
    DEBUG ((DEBUG_WARN, "Failed to find kernel - %r\n", Status));
    FailedToProcess = TRUE;
  }
}

static EFI_FILE_PROTOCOL nilFilProtocol;

UINT8  *Prelinked;
UINT32 PrelinkedSize;

EFI_STATUS
GetFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINT32             Position,
  IN  UINT32             Size,
  OUT UINT8              *Buffer
  )
{
  ASSERT (File == &nilFilProtocol);

  if ((UINT64) Position + Size > PrelinkedSize) {
    return EFI_INVALID_PARAMETER;
  }

  memcpy (&Buffer[0], &Prelinked[Position], Size);
  return EFI_SUCCESS;
}

EFI_STATUS
GetFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  )
{
  ASSERT (File == &nilFilProtocol);
  *Size = PrelinkedSize;
  return EFI_SUCCESS;
}

int wrap_main(int argc, char** argv) {
  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

  UINT32 AllocSize;
  PRELINKED_CONTEXT Context;
  const char *name = argc > 1 ? argv[1] : "/System/Library/PrelinkedKernels/prelinkedkernel";
  if ((Prelinked = UserReadFile(name, &PrelinkedSize)) == NULL) {
    printf("Read fail %s\n", name);
    return -1;
  }

  UINT32 ReservedInfoSize = PRELINK_INFO_RESERVE_SIZE;
  UINT32 ReservedExeSize  = 0;

  for (int argi = 0; argc - argi > 2; argi += 2) {
    UINT8  *TestData = NULL;
    UINT32 TestDataSize = 0;
    CHAR8  *TestPlist = NULL;
    UINT32 TestPlistSize = 0;

    if (argc - argi > 2) {
      if (argv[argi + 2][0] == 'n' && argv[argi + 2][1] == 0) {
        TestData = NULL;
        TestDataSize = 0;
      } else {
        TestData = UserReadFile(argv[argi + 2], &TestDataSize);
        if (TestData == NULL) {
          printf("Read data fail %s\n", argv[argi + 2]);
          abort();
          return -1;
        }
      }
    }

    if (argc - argi > 3) {
      TestPlist = (CHAR8*) UserReadFile(argv[argi + 3], &TestPlistSize);
      if (TestPlist == NULL) {
        printf("Read plist fail\n");
        free(TestData);
        abort();
        return -1;
      }

      free(TestPlist);
    }

    EFI_STATUS Status = PrelinkedReserveKextSize (
      &ReservedInfoSize,
      &ReservedExeSize,
      TestPlistSize,
      TestData,
      TestDataSize,
      FALSE
      );

    free(TestData);
    
    if (EFI_ERROR (Status)) {
      printf (
        "OC: Failed to fit kext %s\n",
        argv[argi + 2]
        );
      FailedToProcess = TRUE;
    }
  }

  UINT32 LinkedExpansion = KcGetSegmentFixupChainsSize (ReservedExeSize);
  if (LinkedExpansion == 0) {
    FailedToProcess = TRUE;
    return -1;
  }

  UINT8 *NewPrelinked;
  UINT32 NewPrelinkedSize;
  UINT8 Sha384[48];
  BOOLEAN Is32Bit;
  EFI_STATUS Status = ReadAppleKernel (
    &nilFilProtocol,
    FALSE,
    &Is32Bit,
    &NewPrelinked,
    &NewPrelinkedSize,
    &AllocSize,
    ReservedInfoSize + ReservedExeSize + LinkedExpansion,
    Sha384
    );

  if (!EFI_ERROR (Status)) {
    free(Prelinked);
    Prelinked = NewPrelinked;
    PrelinkedSize = NewPrelinkedSize;
    DEBUG ((DEBUG_WARN, "[OK] Sha384 is %02X%02X%02X%02X\n", Sha384[0], Sha384[1], Sha384[2], Sha384[3]));
  } else {
    DEBUG ((DEBUG_WARN, "[FAIL] Kernel unpack failure - %r\n", Status));
    FailedToProcess = TRUE;
    return -1;
  }

  KernelVersion = OcKernelReadDarwinVersion (Prelinked, PrelinkedSize);
  if (KernelVersion != 0) {
    DEBUG ((DEBUG_WARN, "[OK] Got version %u\n", KernelVersion));
  } else {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to detect version\n"));
    FailedToProcess = TRUE;
  }


  ApplyKernelPatches (Prelinked, PrelinkedSize);

  PATCHER_CONTEXT        Patcher;
  Status = PatcherInitContextFromBuffer (
    &Patcher,
    Prelinked,
    PrelinkedSize,
    FALSE
    );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[OK] Patcher init success\n"));
  } else {
    DEBUG ((DEBUG_WARN, "[FAIL] Patcher init failure - %r\n", Status));
    FailedToProcess = TRUE;
  }

  Status = PrelinkedContextInit (&Context, Prelinked, PrelinkedSize, AllocSize, FALSE);

  if (!EFI_ERROR (Status)) {
    Status = PrelinkedInjectPrepare (&Context, LinkedExpansion, ReservedExeSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[FAIL] Prelink inject prepare error %r\n", Status));
      FailedToProcess = TRUE;
      return -1;
    }

    Status = PrelinkedInjectKext (
      &Context,
      NULL,
      "/Library/Extensions/PlistKext.kext",
      KextInfoPlistData,
      sizeof (KextInfoPlistData),
      NULL,
      NULL,
      0
      );

    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[OK] PlistKext.kext injected - %r\n", Status));
    } else {
      DEBUG ((DEBUG_WARN, "[FAIL] PlistKext.kext injected - %r\n", Status));
      FailedToProcess = TRUE;
    }

    int c = 0;

    while (argc > 2) {
      UINT8  *TestData = NULL;
      UINT32 TestDataSize = 0;
      CHAR8  *TestPlist = NULL;
      UINT32 TestPlistSize = 0;

      if (argc > 2) {
        if (argv[2][0] == 'n' && argv[2][1] == 0) {
          TestData = NULL;
          TestDataSize = 0;
        } else {
          TestData = UserReadFile(argv[2], &TestDataSize);
          if (TestData == NULL) {
            printf("Read data fail %s\n", argv[2]);
            abort();
            return -1;
          }
        }
      }

      if (argc > 3) {
        TestPlist = (CHAR8*) UserReadFile(argv[3], &TestPlistSize);
        if (TestPlist == NULL) {
          printf("Read plist fail\n");
          abort();
          return -1;
        }
      }

      char KextPath[64];
      snprintf(KextPath, sizeof(KextPath), "/Library/Extensions/Kex%d.kext", c);

      Status = PrelinkedInjectKext (
        &Context,
        NULL,
        KextPath,
        TestPlist,
        TestPlistSize,
        "Contents/MacOS/Kext",
        TestData,
        TestDataSize
        );

      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "[OK] %a injected - %r\n", argv[2], Status));
      } else {
        DEBUG ((DEBUG_WARN, "[FAIL] %a injected - %r\n", argv[2], Status));
        FailedToProcess = TRUE;
      }

      if (argc > 2) free(TestData);
      if (argc > 3) free(TestPlist);

      argc -= 2;
      argv += 2;
      c++;
    }

    ASSERT (Context.PrelinkedSize - Context.KextsFileOffset <= ReservedExeSize);

    Status = PrelinkedInjectComplete (&Context);

    ApplyKextPatches (&Context);

    UserWriteFile("out.bin", Prelinked, Context.PrelinkedSize);

    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "[OK] Prelink inject complete success\n"));
    } else {
      DEBUG ((DEBUG_WARN, "[FAIL] Prelink inject complete error %r\n", Status));
      FailedToProcess = TRUE;
    }

    PrelinkedContextFree (&Context);
  } else {
    DEBUG ((DEBUG_WARN, "[FAIL] Context creation error %r\n", Status));
    FailedToProcess = TRUE;
  }

  free(Prelinked);

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  UINT32 PrelinkedSize;
  UINT32 AllocSize;
  UINT8  *Prelinked;
  PRELINKED_CONTEXT Context;

  if (Size == 0) {
    return 0;
  }

  if ((Prelinked = UserReadFile("prelinkedkernel.unpack", &PrelinkedSize)) == NULL) {
    printf("Read fail\n");
    return 0;
  }

  AllocSize = MACHO_ALIGN (PrelinkedSize + 64*1024*1024);

  Prelinked = realloc (Prelinked, AllocSize);
  if (Prelinked == NULL) {
    return 0;
  }

  EFI_STATUS Status = PrelinkedContextInit (&Context, Prelinked, PrelinkedSize, AllocSize, FALSE);

  if (EFI_ERROR (Status)) {
    free (Prelinked);
    return 0;
  }

  Status = PrelinkedInjectPrepare (&Context, BASE_2MB, BASE_2MB);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "Prelink inject prepare error %r\n", Status));
    PrelinkedContextFree (&Context);
    free (Prelinked);
    return 0;
  }

  Status = PrelinkedInjectKext (
      &Context,
      NULL,
      "/Library/Extensions/Lilu.kext",
      KextInfoPlistData, ///< FIXME: has no executable
      sizeof (KextInfoPlistData),
      "Contents/MacOS/Lilu",
      Data,
      Size
      );

  PrelinkedInjectComplete (&Context);
  PrelinkedContextFree (&Context);
  free(Prelinked);

  return 0;
}

int ENTRY_POINT(int argc, char *argv[]) {
  int code = wrap_main(argc, argv);
  if (FailedToProcess) {
    code = -1;
  }
  return code;
}
