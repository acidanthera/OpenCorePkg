/** @file
  Commonly used kext and kernel patches.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <IndustryStandard/AppleIntelCpuInfo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/PrintLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiLib.h>

STATIC
CONST UINT8
  mMovEcxE2[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00  ///< mov ecx, 0xE2
};
STATIC_ASSERT (sizeof (mMovEcxE2) == 5, "Unsupported mMovEcxE2");

STATIC
CONST UINT8
  mMovCxE2[] = {
  0x66, 0xB9, 0xE2, 0x00        ///< mov cx, 0xE2
};
STATIC_ASSERT (sizeof (mMovCxE2) == 4, "Unsupported mMovCxE2");

STATIC
CONST UINT8
  mWrmsr[] = {
  0x0F, 0x30                    ///< wrmsr
};
STATIC_ASSERT (sizeof (mWrmsr) == 2, "Unsupported mWrmsr");

STATIC
CONST UINTN
  mWrmsrMaxDistance = 32;

STATIC
EFI_STATUS
PatchAppleCpuPmCfgLock (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  UINTN  Count;
  UINT8  *Walker;
  UINT8  *WalkerEnd;
  UINT8  *WalkerTmp;

  //
  // NOTE: As of macOS 13.0 AICPUPM kext is removed.
  // However, legacy version of this kext may be injected and patched,
  // thus no need to perform system version check here.
  //

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on kernel version %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  Count     = 0;
  Walker    = (UINT8 *)MachoGetMachHeader (&Patcher->MachContext);
  WalkerEnd = Walker + MachoGetInnerSize (&Patcher->MachContext) - mWrmsrMaxDistance;

  //
  // Thanks to Clover developers for the approach.
  //
  while (Walker < WalkerEnd) {
    //
    // Match (e)cx E2h assignment.
    //
    if (  (Walker[0] == mMovEcxE2[0])
       && (Walker[1] == mMovEcxE2[1])
       && (Walker[2] == mMovEcxE2[2])
       && (Walker[3] == mMovEcxE2[3])
       && (Walker[4] == mMovEcxE2[4]))
    {
      Walker += sizeof (mMovEcxE2);
    } else if (  (Walker[0] == mMovCxE2[0])
              && (Walker[1] == mMovCxE2[1])
              && (Walker[2] == mMovCxE2[2])
              && (Walker[3] == mMovCxE2[3]))
    {
      Walker += sizeof (mMovCxE2);
    } else {
      ++Walker;
      continue;
    }

    WalkerTmp = Walker + mWrmsrMaxDistance;

    while (Walker < WalkerTmp) {
      if (  (Walker[0] == mWrmsr[0])
         && (Walker[1] == mWrmsr[1]))
      {
        ++Count;
        //
        // Patch matched wrmsr with nop.
        //
        *Walker++ = 0x90;
        *Walker++ = 0x90;
        break;
      }

      if (  ((Walker[0] == 0xC9) && (Walker[1] == 0xC3))  ///< leave; ret
         || ((Walker[0] == 0x5D) && (Walker[1] == 0xC3))) ///< pop rbp; ret
      //
      // Stop searching upon matching return sequences.
      //
      {
        Walker += 2;
        break;
      }

      if (  ((Walker[0] == 0xB9) && (Walker[3] == 0) && (Walker[4] == 0))     ///< mov ecx, 00000xxxxh
         || ((Walker[0] == 0x66) && (Walker[1] == 0xB9) && (Walker[3] == 0))) ///< mov cx, 00xxh
      //
      // Stop searching upon matching reassign sequences.
      //
      {
        break;
      }

      //
      // Continue searching.
      //
      ++Walker;
    }
  }

  //
  // At least one patch must be successful for this to work.
  //
  if (Count > 0) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Success AppleCpuPmCfgLock patch\n"));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply AppleCpuPmCfgLock patch\n"));
  return EFI_NOT_FOUND;
}

#pragma pack(push, 1)

//
// XCPM record definition, extracted from XNU debug kernel.
//
typedef struct XCPM_MSR_RECORD_ {
  UINT32    xcpm_msr_num;
  UINT32    xcpm_msr_applicable_cpus;
  UINT32    *xcpm_msr_flag_p;
  UINT64    xcpm_msr_bits_clear;
  UINT64    xcpm_msr_bits_set;
  UINT64    xcpm_msr_initial_value;
  UINT64    xcpm_msr_rb_value;
} XCPM_MSR_RECORD;

#pragma pack(pop)

STATIC
CONST UINT8
  mXcpmCfgLockRelFind[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00,  ///< mov ecx, 0xE2
  0x0F, 0x30                     ///< wrmsr
};

STATIC
CONST UINT8
  mXcpmCfgLockRelReplace[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00,  ///< mov ecx, 0xE2
  0x90, 0x90                     ///< nop
};

STATIC
PATCHER_GENERIC_PATCH
  mXcpmCfgLockRelPatch = {
  .Comment     = DEBUG_POINTER ("XcpmCfgLockRel"),
  .Base        = "_xcpm_idle",
  .Find        = mXcpmCfgLockRelFind,
  .Mask        = NULL,
  .Replace     = mXcpmCfgLockRelReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mXcpmCfgLockRelFind),
  .Count       = 2,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
CONST UINT8
  mXcpmCfgLockDbgFind[] = {
  0xBF, 0xE2, 0x00, 0x00, 0x00,  ///< mov edi, 0xE2
  0xE8                           ///< call (wrmsr64)
};

STATIC
CONST UINT8
  mXcpmCfgLockDbgReplace[] = {
  0xEB, 0x08,                    ///< jmp LBL        ; Skip the 3x nops and 5x call
  0x90, 0x90, 0x90,              ///< nop
  0xE8                           ///< call (wrmsr64) ; Skipped by jmp LBL
};

STATIC
PATCHER_GENERIC_PATCH
  mXcpmCfgLockDbgPatch = {
  .Comment     = DEBUG_POINTER ("XcpmCfgLockDbg"),
  .Base        = "_xcpm_cst_control_evaluate",
  .Find        = mXcpmCfgLockDbgFind,
  .Mask        = NULL,
  .Replace     = mXcpmCfgLockDbgReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mXcpmCfgLockDbgFind),
  .Count       = 2,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
EFI_STATUS
PatchAppleXcpmCfgLock (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS       Status;
  XCPM_MSR_RECORD  *Record;
  XCPM_MSR_RECORD  *Last;

  UINT32  Replacements;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  //
  // XCPM is not available before macOS 10.8.5.
  //
  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION (KERNEL_VERSION_MOUNTAIN_LION, 5, 0), 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping XcpmCfgLock on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Last = (XCPM_MSR_RECORD *)((UINT8 *)MachoGetMachHeader (&Patcher->MachContext)
                             + MachoGetInnerSize (&Patcher->MachContext) - sizeof (XCPM_MSR_RECORD));

  Replacements = 0;

  Status = PatcherGetSymbolAddress (Patcher, "_xcpm_core_scope_msrs", (UINT8 **)&Record);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate _xcpm_core_scope_msrs for XcpmCfgLock patch - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  while (Record < Last) {
    if (Record->xcpm_msr_num != 0xE2) {
      break;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCAK: Zeroing _xcpm_core_scope_msrs 0x%X applicable CPUs (%u)\n",
      Record->xcpm_msr_num,
      Record->xcpm_msr_applicable_cpus
      ));
    Record->xcpm_msr_applicable_cpus = 0;
    ++Replacements;

    ++Record;
  }

  //
  // Now the HWP patch at _xcpm_idle() for Release XNU.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mXcpmCfgLockRelPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply _xcpm_idle release patch - %r, trying dbg\n", Status));
    Status = PatcherApplyGenericPatch (Patcher, &mXcpmCfgLockDbgPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCAK: Failed to apply dbg _xcpm_cst_control_evaluate patches - %r\n", Status));
    }
  }

  if ((Replacements > 0) && !EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Success XcpmCfgLock patch\n"));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply XcpmCfgLock patch\n"));
  return EFI_NOT_FOUND;
}

STATIC
CONST UINT8
  mMiscPwrMgmtRelFind[] = {
  0xB9, 0xAA, 0x01, 0x00, 0x00,  ///< mov ecx, 0x1AA
  0x0F, 0x30                     ///< wrmsr
};

STATIC
CONST UINT8
  mMiscPwrMgmtRelReplace[] = {
  0xB9, 0xAA, 0x01, 0x00, 0x00,  ///< mov ecx, 0x1AA
  0x90, 0x90                     ///< nop
};

STATIC
PATCHER_GENERIC_PATCH
  mMiscPwrMgmtRelPatch = {
  .Comment     = DEBUG_POINTER ("MiscPwrMgmtRel"),
  .Base        = NULL,
  .Find        = mMiscPwrMgmtRelFind,
  .Mask        = NULL,
  .Replace     = mMiscPwrMgmtRelReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mMiscPwrMgmtRelFind),
  .Count       = 0,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
CONST UINT8
  mMiscPwrMgmtDbgFind[] = {
  0xBF, 0xAA, 0x01, 0x00, 0x00,  ///< mov edi, 0x1AA
  0xE8                           ///< call (wrmsr64)
};

STATIC
CONST UINT8
  mMiscPwrMgmtDbgReplace[] = {
  0xEB, 0x08,                    ///< jmp LBL        ; Skip the 3x nops and 5x call
  0x90, 0x90, 0x90,              ///< nop
  0xE8                           ///< call (wrmsr64) ; Skipped by jmp LBL
};

STATIC
PATCHER_GENERIC_PATCH
  mMiscPwrMgmtDbgPatch = {
  .Comment     = DEBUG_POINTER ("MiscPwrMgmtDbg"),
  .Base        = NULL,
  .Find        = mMiscPwrMgmtDbgFind,
  .Mask        = NULL,
  .Replace     = mMiscPwrMgmtDbgReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mMiscPwrMgmtDbgFind),
  .Count       = 0,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
EFI_STATUS
PatchAppleXcpmExtraMsrs (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS       Status;
  XCPM_MSR_RECORD  *Record;
  XCPM_MSR_RECORD  *Last;
  UINT32           Replacements;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  //
  // XCPM is not available before macOS 10.8.5.
  //
  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION (KERNEL_VERSION_MOUNTAIN_LION, 5, 0), 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping XcpmExtraMsrs on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Last = (XCPM_MSR_RECORD *)((UINT8 *)MachoGetMachHeader (&Patcher->MachContext)
                             + MachoGetInnerSize (&Patcher->MachContext) - sizeof (XCPM_MSR_RECORD));

  Replacements = 0;

  Status = PatcherGetSymbolAddress (Patcher, "_xcpm_pkg_scope_msrs", (UINT8 **)&Record);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate _xcpm_pkg_scope_msrs for XcpmExtraMsrs patch - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  while (Record < Last) {
    //
    // Most Record->xcpm_msr_applicable_cpus has
    // 0xDC or 0xDE in its lower 16-bit and thus here we
    // AND 0xFF0000FDU in order to match both. (The result will be 0xDC)
    //
    if ((Record->xcpm_msr_applicable_cpus & 0xFF0000FDU) != 0xDC) {
      break;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCAK: Zeroing _xcpm_pkg_scope_msrs 0x%X applicable CPUs (%u)\n",
      Record->xcpm_msr_num,
      Record->xcpm_msr_applicable_cpus
      ));
    Record->xcpm_msr_applicable_cpus = 0;
    ++Replacements;

    ++Record;
  }

  Status = PatcherGetSymbolAddress (Patcher, "_xcpm_SMT_scope_msrs", (UINT8 **)&Record);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate _xcpm_SMT_scope_msrs for XcpmExtraMsrs patch - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  while (Record < Last) {
    if (Record->xcpm_msr_flag_p != NULL) {
      break;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCAK: Zeroing _xcpm_SMT_scope_msrs data 0x%X applicable CPUs (%u)\n",
      Record->xcpm_msr_num,
      Record->xcpm_msr_applicable_cpus
      ));
    Record->xcpm_msr_applicable_cpus = 0;
    ++Replacements;

    ++Record;
  }

  //
  // Now patch writes to MSR_MISC_PWR_MGMT.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mMiscPwrMgmtRelPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to patch writes to MSR_MISC_PWR_MGMT - %r, trying dbg\n", Status));
    Status = PatcherApplyGenericPatch (Patcher, &mMiscPwrMgmtDbgPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCAK: Failed to patch writes to MSR_MISC_PWR_MGMT - %r\n", Status));
    }
  }

  if ((Replacements > 0) && !EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Success XcpmExtraMsrs patch\n"));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply XcpmExtraMsrs patch\n"));
  return EFI_NOT_FOUND;
}

STATIC
CONST UINT8
  mPerfCtrlFind1[] = {
  0xB9, 0x99, 0x01, 0x00, 0x00,  ///< mov ecx, 0x199
  0x0F, 0x30                     ///< wrmsr
};

STATIC
CONST UINT8
  mPerfCtrlFind2[] = {
  0xB9, 0x99, 0x01, 0x00, 0x00, ///< mov ecx, 0x199
  0x31, 0xD2,                   ///< xor edx, edx
  0x0F, 0x30                    ///< wrmsr
};

STATIC
CONST UINT8
  mPerfCtrlFind3[] = {
  0xB9, 0x99, 0x01, 0x00, 0x00, ///< mov ecx, 0x199
  0x4C, 0x89, 0xF0,             ///< mov rax, r14
  0x0F, 0x30                    ///< wrmsr
};

STATIC
CONST UINT8
  mPerfCtrlFind4[] = {
  0xB9, 0x99, 0x01, 0x00, 0x00, ///< mov ecx, 0x199
  0x48, 0x89, 0xD8,             ///< mov rax, rbx
  0x0F, 0x30                    ///< wrmsr
};

STATIC
CONST UINT8
  mPerfCtrlMax[] = {
  0xB9, 0x99, 0x01, 0x00, 0x00, ///< mov ecx, 0x199
  0x31, 0xD2,                   ///< xor edx, edx
  0xB8, 0x00, 0xFF, 0x00, 0x00, ///< mov eax, 0xFF00
  0x0F, 0x30,                   ///< wrmsr
  0xC3                          ///< ret
};

STATIC
EFI_STATUS
PatchAppleXcpmForceBoost (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion
  )
{
  UINT8  *Start;
  UINT8  *Last;
  UINT8  *Current;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  //
  // XCPM is not available before macOS 10.8.5.
  //
  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION (KERNEL_VERSION_MOUNTAIN_LION, 5, 0), 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping XcpmForceBoost on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Start   = (UINT8 *)MachoGetMachHeader (&Patcher->MachContext);
  Last    = Start + MachoGetInnerSize (&Patcher->MachContext) - EFI_PAGE_SIZE * 2;
  Start  += EFI_PAGE_SIZE;
  Current = Start;

  while (Current < Last) {
    //
    // Compare <mov ecx, 0x199> in common.
    //
    if (  (Current[0] == mPerfCtrlFind1[0])
       && (Current[1] == mPerfCtrlFind1[1])
       && (Current[2] == mPerfCtrlFind1[2])
       && (Current[3] == mPerfCtrlFind1[3]))
    {
      if (  (CompareMem (&Current[4], &mPerfCtrlFind1[4], sizeof (mPerfCtrlFind1) - 4) == 0)
         || (CompareMem (&Current[4], &mPerfCtrlFind2[4], sizeof (mPerfCtrlFind2) - 4) == 0)
         || (CompareMem (&Current[4], &mPerfCtrlFind3[4], sizeof (mPerfCtrlFind3) - 4) == 0)
         || (CompareMem (&Current[4], &mPerfCtrlFind4[4], sizeof (mPerfCtrlFind4) - 4) == 0))
      {
        break;
      }
    }

    ++Current;
  }

  if (Current == Last) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate MSR_IA32_PERF_CONTROL write for XcpmForceBoost patch\n"));
    return EFI_NOT_FOUND;
  }

  Start    = Current - EFI_PAGE_SIZE;
  Current -= 4;

  while (Current >= Start) {
    //
    // Locate the beginning.
    //
    if (  (Current[0] == 0x55)
       && (Current[1] == 0x48)
       && (Current[2] == 0x89)
       && (Current[3] == 0xE5))
    {
      break;
    }

    --Current;
  }

  if (Current < Start) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate MSR_IA32_PERF_CONTROL prologue for XcpmForceBoost patch\n"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch write max to MSR_IA32_PERF_CONTROL for XcpmForceBoost patch\n"));
  CopyMem (Current, mPerfCtrlMax, sizeof (mPerfCtrlMax));
  return EFI_SUCCESS;
}

STATIC
CONST UINT8
  mRemoveUsbLimitV1Find[] = {
  0xFF, 0xFF, 0x10
};

STATIC
CONST UINT8
  mRemoveUsbLimitV1Replace[] = {
  0xFF, 0xFF, 0x40
};

STATIC
PATCHER_GENERIC_PATCH
  mRemoveUsbLimitV1Patch = {
  .Comment     = DEBUG_POINTER ("RemoveUsbLimitV1"),
  .Base        = "__ZN15AppleUSBXHCIPCI11createPortsEv",
  .Find        = mRemoveUsbLimitV1Find,
  .Mask        = NULL,
  .Replace     = mRemoveUsbLimitV1Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mRemoveUsbLimitV1Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 8192
};

STATIC
CONST UINT8
  mRemoveUsbLimitV2Find[] = {
  0x0F, 0x0F, 0x83
};

STATIC
CONST UINT8
  mRemoveUsbLimitV2Replace[] = {
  0x40, 0x0F, 0x83
};

STATIC
PATCHER_GENERIC_PATCH
  mRemoveUsbLimitV2Patch = {
  .Comment     = DEBUG_POINTER ("RemoveUsbLimitV2"),
  .Base        = "__ZN12AppleUSBXHCI11createPortsEv",
  .Find        = mRemoveUsbLimitV2Find,
  .Mask        = NULL,
  .Replace     = mRemoveUsbLimitV2Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mRemoveUsbLimitV2Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
CONST UINT8
  mRemoveUsbLimitIoP1Find1[] = {
  0x0F, 0x0F, 0x87
};

STATIC
CONST UINT8
  mRemoveUsbLimitIoP1Replace1[] = {
  0x40, 0x0F, 0x87
};

STATIC
PATCHER_GENERIC_PATCH
  mRemoveUsbLimitIoP1Patch1 = {
  .Comment     = DEBUG_POINTER ("RemoveUsbLimitIoP1 part 1"),
  .Base        = "__ZN16AppleUSBHostPort15setPortLocationEj",
  .Find        = mRemoveUsbLimitIoP1Find1,
  .Mask        = NULL,
  .Replace     = mRemoveUsbLimitIoP1Replace1,
  .ReplaceMask = NULL,
  .Size        = sizeof (mRemoveUsbLimitIoP1Replace1),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
CONST UINT8
  mRemoveUsbLimitIoP1Find2[] = {
  0x41, 0x83, 0x00, 0x0F,  ///< and whatever, 0x0Fh
  0x41, 0xD3, 0x00,        ///< shl whatever, cl
  0x00, 0x09, 0x00         ///< or ebx, whatever
};

STATIC
CONST UINT8
  mRemoveUsbLimitIoP1Mask2[] = {
  0xFF, 0xFF, 0x00, 0xFF,
  0xFF, 0xFF, 0x00,
  0x00, 0xFF, 0x00
};

STATIC
CONST UINT8
  mRemoveUsbLimitIoP1Replace2[] = {
  0x00, 0x00, 0x00, 0x3F,
  0x00, 0x00, 0x00,
  0x00, 0x00, 0x00
};

STATIC
CONST UINT8
  mRemoveUsbLimitIoP1ReplaceMask2[] = {
  0x00, 0x00, 0x00, 0xFF,
  0x00, 0x00, 0x00,
  0x00, 0x00, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
  mRemoveUsbLimitIoP1Patch2 = {
  .Comment     = DEBUG_POINTER ("RemoveUsbLimitIoP1 part 2"),
  .Base        = "__ZN16AppleUSBHostPort15setPortLocationEj",
  .Find        = mRemoveUsbLimitIoP1Find2,
  .Mask        = mRemoveUsbLimitIoP1Mask2,
  .Replace     = mRemoveUsbLimitIoP1Replace2,
  .ReplaceMask = mRemoveUsbLimitIoP1ReplaceMask2,
  .Size        = sizeof (mRemoveUsbLimitIoP1Replace2),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
EFI_STATUS
PatchUsbXhciPortLimit1 (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  //
  // On 10.14.4 and newer IOUSBHostFamily also needs limit removal.
  // Thanks to ydeng discovering this.
  //
  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION (KERNEL_VERSION_MOJAVE, 5, 0), 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping port patch IOUSBHostFamily on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mRemoveUsbLimitIoP1Patch1);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply port patch com.apple.iokit.IOUSBHostFamily part 1 - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success port com.apple.iokit.IOUSBHostFamily part 1\n"));
  }

  //
  // The following patch is only needed on macOS 11.1 (Darwin 20.2.0) and above; skip it otherwise.
  //
  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION (KERNEL_VERSION_BIG_SUR, 2, 0), 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping port patch com.apple.iokit.IOUSBHostFamily part 2 on %u\n", KernelVersion));
    return Status;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mRemoveUsbLimitIoP1Patch2);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply port patch com.apple.iokit.IOUSBHostFamily part 2 - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success port com.apple.iokit.IOUSBHostFamily part 2\n"));
  }

  return Status;
}

STATIC
EFI_STATUS
PatchUsbXhciPortLimit2 (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping modern port patch AppleUSBXHCI on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  //
  // TODO: Implement some locationID hack in IOUSBHostFamily.
  // The location ID is a 32 bit number which is unique among all USB devices in the system,
  // and which will not change on a system reboot unless the topology of the bus itself changes.
  // See AppleUSBHostPort::setPortLocation():
  // locationId = getLocationId();
  // if (!(locationId & 0xF)) {
  //   int32_t shift = 20;
  //   while (locationId & (0xF << shift)) {
  //     shift -= 4;
  //     if (Shift < 0) { setLocationId(locationId); return; }
  //   }
  //   setLocationId(locationId | ((portNumber & 0xF) << shift));
  // }
  // The value (e.g. 0x14320000) is represented as follows: 0xAABCDEFG
  // AA  — Ctrl number 8 bits (e.g. 0x14, aka XHCI)
  // B   - Port number 4 bits (e.g. 0x3, aka SS03)
  // C~F - Bus number  4 bits (e.g. 0x2, aka IOUSBHostHIDDevice)
  //
  // C~F are filled as many times as many USB Hubs are there on the port.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mRemoveUsbLimitV2Patch);
  if (!EFI_ERROR (Status)) {
    //
    // We do not need to patch com.apple.driver.usb.AppleUSBXHCI if this patch was successful.
    // Only legacy systems require com.apple.driver.usb.AppleUSBXHCI to be patched.
    //
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success com.apple.driver.usb.AppleUSBXHCI\n"));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch com.apple.driver.usb.AppleUSBXHCI - %r\n", Status));
  }

  //
  // TODO: Check when the patch changed actually.
  //
  if (  EFI_ERROR (Status)
     && OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, KERNEL_VERSION_HIGH_SIERRA_MAX))
  {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Assuming success for AppleUSBXHCI on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  return Status;
}

STATIC
EFI_STATUS
PatchUsbXhciPortLimit3 (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_EL_CAPITAN_MIN, KERNEL_VERSION_HIGH_SIERRA_MAX)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping legacy port patch AppleUSBXHCIPCI on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  //
  // If we are here, we are on legacy 10.13 or below, try the oldest patch.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mRemoveUsbLimitV1Patch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply legacy port patch AppleUSBXHCIPCI - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success apply legacy port AppleUSBXHCIPCI\n"));
  }

  //
  // TODO: Check when the patch changed actually.
  //
  if (  EFI_ERROR (Status)
     && OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, KERNEL_VERSION_HIGH_SIERRA_MAX))
  {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Assuming success for legacy port AppleUSBXHCIPCI on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  return Status;
}

STATIC
CONST UINT8
  mIOAHCIBlockStoragePatchV1Find[] = {
  0x41, 0x50, 0x50, 0x4C, 0x45, 0x20, 0x53, 0x53, 0x44, 0x00  ///< "APPLE SSD\0"
};

STATIC
CONST UINT8
  mIOAHCIBlockStoragePatchV1Replace[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
  mIOAHCIBlockStoragePatchV1 = {
  .Comment     = DEBUG_POINTER ("IOAHCIBlockStorageV1"),
  .Base        = NULL,
  .Find        = mIOAHCIBlockStoragePatchV1Find,
  .Mask        = NULL,
  .Replace     = mIOAHCIBlockStoragePatchV1Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIOAHCIBlockStoragePatchV1Find),
  .Count       = 1,
  .Skip        = 0
};

STATIC
CONST UINT8
  mIOAHCIBlockStoragePatchV2Find[] = {
  0x41, 0x50, 0x50, 0x4C, 0x45, 0x00  ///< "APPLE\0"
};

STATIC
CONST UINT8
  mIOAHCIBlockStoragePatchV2Replace[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
  mIOAHCIBlockStoragePatchV2 = {
  .Comment     = DEBUG_POINTER ("IOAHCIBlockStorageV2"),
  .Base        = NULL,
  .Find        = mIOAHCIBlockStoragePatchV2Find,
  .Mask        = NULL,
  .Replace     = mIOAHCIBlockStoragePatchV2Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIOAHCIBlockStoragePatchV2Find),
  .Count       = 1,
  .Skip        = 0
};

STATIC
CONST UINT8
  mIOAHCIBlockStoragePatch133Find1[] = {
  0x48, 0x8D, 0x3D, 0x00, 0x00, 0x00, 0x00,
  0xBA, 0x09, 0x00, 0x00, 0x00
};

STATIC
CONST UINT8
  mIOAHCIBlockStoragePatch133Find2[] = {
  0x48, 0x8D, 0x3D, 0x00, 0x00, 0x00, 0x00,
  0xBA, 0x05, 0x00, 0x00, 0x00
};

STATIC
CONST UINT8
  mIOAHCIBlockStoragePatch133FindMask[] = {
  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

STATIC
CONST UINT8
  mIOAHCIBlockStoragePatch133Replace[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xBA, 0x00, 0x00, 0x00, 0x00
};

STATIC
CONST UINT8
  mIOAHCIBlockStoragePatch133ReplaceMask[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

STATIC
PATCHER_GENERIC_PATCH
  mIOAHCIBlockStoragePatch133Part1 = {
  .Comment     = DEBUG_POINTER ("IOAHCIBlockStorage trim 13.3+ part 1"),
  .Base        = "__ZN24IOAHCIBlockStorageDriver23DetermineDeviceFeaturesEPt",
  .Find        = mIOAHCIBlockStoragePatch133Find1,
  .Mask        = mIOAHCIBlockStoragePatch133FindMask,
  .Replace     = mIOAHCIBlockStoragePatch133Replace,
  .ReplaceMask = mIOAHCIBlockStoragePatch133ReplaceMask,
  .Size        = sizeof (mIOAHCIBlockStoragePatch133Find1),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
PATCHER_GENERIC_PATCH
  mIOAHCIBlockStoragePatch133Part2 = {
  .Comment     = DEBUG_POINTER ("IOAHCIBlockStorage trim 13.3+ part 2"),
  .Base        = "__ZN24IOAHCIBlockStorageDriver23DetermineDeviceFeaturesEPt",
  .Find        = mIOAHCIBlockStoragePatch133Find2,
  .Mask        = mIOAHCIBlockStoragePatch133FindMask,
  .Replace     = mIOAHCIBlockStoragePatch133Replace,
  .ReplaceMask = mIOAHCIBlockStoragePatch133ReplaceMask,
  .Size        = sizeof (mIOAHCIBlockStoragePatch133Find2),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
EFI_STATUS
PatchThirdPartyDriveSupport (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  //
  // Starting with macOS 13.3 (Darwin 22.4.0), a new set of patches are required, discovered by @vit9696.
  //
  if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION (KERNEL_VERSION_VENTURA, 4, 0), 0)) {
    Status = PatcherApplyGenericPatch (Patcher, &mIOAHCIBlockStoragePatch133Part1);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch 13.3+ com.apple.iokit.IOAHCIBlockStorage part 1 - %r\n", Status));
      return Status;
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success 13.3+ com.apple.iokit.IOAHCIBlockStorage part 1\n"));
    }

    Status = PatcherApplyGenericPatch (Patcher, &mIOAHCIBlockStoragePatch133Part2);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch 13.3+ com.apple.iokit.IOAHCIBlockStorage part 2 - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success 13.3+ com.apple.iokit.IOAHCIBlockStorage part 2\n"));
    }

    return Status;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mIOAHCIBlockStoragePatchV1);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch legacy com.apple.iokit.IOAHCIBlockStorage V1 - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success legacy com.apple.iokit.IOAHCIBlockStorage V1\n"));
  }

  if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_CATALINA_MIN, 0)) {
    Status = PatcherApplyGenericPatch (Patcher, &mIOAHCIBlockStoragePatchV2);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch legacy com.apple.iokit.IOAHCIBlockStorage V2 - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success legacy com.apple.iokit.IOAHCIBlockStorage V2\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping IOAHCIBlockStorage legacy V2 on %u\n", KernelVersion));
  }

  //
  // This started to be required on 10.6.7 or so.
  // We cannot trust which minor SnowLeo version is this, just let it pass.
  //
  if (  EFI_ERROR (Status)
     && OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_SNOW_LEOPARD_MIN, KERNEL_VERSION_SNOW_LEOPARD_MAX))
  {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Assuming success for legacy IOAHCIBlockStorage on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  return Status;
}

STATIC
CONST UINT8
  mIOAHCIPortPatchFind[] = {
  0x45, 0x78, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C  ///< "External"
};

STATIC
CONST UINT8
  mIOAHCIPortPatchReplace[] = {
  0x49, 0x6E, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C  ///< "Internal"
};

STATIC
PATCHER_GENERIC_PATCH
  mIOAHCIPortPatch = {
  .Comment     = DEBUG_POINTER ("IOAHCIPort"),
  .Base        = NULL,
  .Find        = mIOAHCIPortPatchFind,
  .Mask        = NULL,
  .Replace     = mIOAHCIPortPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIOAHCIPortPatchFind),
  .Count       = 1,  ///< 2 for macOS 13.3+
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchForceInternalDiskIcons (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  //
  // Override patch count to 2 on macOS 13.3+ (Darwin 22.4.0).
  //
  if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION (KERNEL_VERSION_VENTURA, 4, 0), 0)) {
    mIOAHCIPortPatch.Count = 2;
  } else {
    mIOAHCIPortPatch.Count = 1;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mIOAHCIPortPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch com.apple.driver.AppleAHCIPort - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success com.apple.driver.AppleAHCIPort\n"));
  }

  return Status;
}

STATIC
CONST UINT8
  mAppleIoMapperPatchFind[] = {
  0x44, 0x4D, 0x41, 0x52, 0x00  ///< "DMAR\0"
};

STATIC
CONST UINT8
  mAppleIoMapperPatchReplace[] = {
  0x52, 0x41, 0x4D, 0x44, 0x00  ///< "RAMD\0"
};

STATIC
PATCHER_GENERIC_PATCH
  mAppleIoMapperPatch = {
  .Comment     = DEBUG_POINTER ("AppleIoMapper"),
  .Base        = NULL,
  .Find        = mAppleIoMapperPatchFind,
  .Mask        = NULL,
  .Replace     = mAppleIoMapperPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mAppleIoMapperPatchFind),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchAppleIoMapperSupport (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_MOUNTAIN_LION_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping AppleIoMapper patch on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mAppleIoMapperPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch com.apple.iokit.IOPCIFamily AppleIoMapper - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success com.apple.iokit.IOPCIFamily AppleIoMapper\n"));
  }

  return Status;
}

STATIC
CONST UINT8
  mAppleIoMapperMappingPatchReplace[] = {
  0xC3  ///< ret
};

STATIC
PATCHER_GENERIC_PATCH
  mAppleIoMapperMappingPatch = {
  .Comment     = DEBUG_POINTER ("AppleIoMapperMapping"),
  .Base        = "__ZN8AppleVTD14addMemoryRangeEyy",
  .Find        = NULL,
  .Mask        = NULL,
  .Replace     = mAppleIoMapperMappingPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mAppleIoMapperMappingPatchReplace),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchAppleIoMapperMapping (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  //
  // This patch is not required before macOS 13.3 (kernel 22.4.0)
  //
  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION (KERNEL_VERSION_VENTURA, 4, 0), 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping AppleIoMapperMapping patch on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mAppleIoMapperMappingPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch com.apple.iokit.IOPCIFamily AppleIoMapperMapping - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success com.apple.iokit.IOPCIFamily AppleIoMapperMapping\n"));
  }

  return Status;
}

STATIC
CONST UINT8
  mAppleDummyCpuPmPatchReplace[] = {
  0xB8, 0x01, 0x00, 0x00, 0x00,  ///< mov eax, 1
  0xC3                           ///< ret
};

STATIC
PATCHER_GENERIC_PATCH
  mAppleDummyCpuPmPatch = {
  .Comment     = DEBUG_POINTER ("DummyCpuPm"),
  .Base        = "__ZN28AppleIntelCPUPowerManagement5startEP9IOService",
  .Find        = NULL,
  .Mask        = NULL,
  .Replace     = mAppleDummyCpuPmPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mAppleDummyCpuPmPatchReplace),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchDummyPowerManagement (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_VENTURA_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping dummy AppleIntelCPUPowerManagement patch on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mAppleDummyCpuPmPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch dummy AppleIntelCPUPowerManagement - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success dummy AppleIntelCPUPowerManagement\n"));
  }

  return Status;
}

STATIC
CONST UINT8
  mIncreasePciBarSizePatchFind[] = {
  0x00, 0x00, 0x00, 0x40
};

STATIC
CONST UINT8
  mIncreasePciBarSizePatchReplace[] = {
  0x00, 0x00, 0x00, 0x80
};

STATIC
PATCHER_GENERIC_PATCH
  mIncreasePciBarSizePatch = {
  .Comment     = DEBUG_POINTER ("IncreasePciBarSize"),
  .Base        = "__ZN17IOPCIConfigurator24probeBaseAddressRegisterEP16IOPCIConfigEntryjj",
  .Find        = mIncreasePciBarSizePatchFind,
  .Mask        = NULL,
  .Replace     = mIncreasePciBarSizePatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIncreasePciBarSizePatchFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
CONST UINT8
  mIncreasePciBarSizePatchLegacyFind[] = {
  0x01, 0x00, 0x00, 0x40
};

STATIC
CONST UINT8
  mIncreasePciBarSizePatchLegacyReplace[] = {
  0x01, 0x00, 0x00, 0x80
};

STATIC
PATCHER_GENERIC_PATCH
  mIncreasePciBarSizeLegacyPatch = {
  .Comment     = DEBUG_POINTER ("IncreasePciBarSizeLegacy"),
  .Base        = "__ZN17IOPCIConfigurator24probeBaseAddressRegisterEP16IOPCIConfigEntryjj",
  .Find        = mIncreasePciBarSizePatchLegacyFind,
  .Mask        = NULL,
  .Replace     = mIncreasePciBarSizePatchLegacyReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIncreasePciBarSizePatchLegacyFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
EFI_STATUS
PatchIncreasePciBarSize (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_YOSEMITE_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping com.apple.iokit.IOPCIFamily IncreasePciBarSize on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mIncreasePciBarSizePatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch com.apple.iokit.IOPCIFamily IncreasePciBarSize - %r, trying legacy patch\n", Status));
    Status = PatcherApplyGenericPatch (Patcher, &mIncreasePciBarSizeLegacyPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply legacy patch com.apple.iokit.IOPCIFamily IncreasePciBarSize - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success legacy com.apple.iokit.IOPCIFamily IncreasePciBarSize\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success com.apple.iokit.IOPCIFamily IncreasePciBarSize\n"));
  }

  return Status;
}

STATIC
CONST UINT8
  mSerialDevicePmioFind[] = {
  0x66, 0xBA, 0xF8, 0x03  ///< mov dx, 0x03F[8-9A-F]
};

STATIC
UINTN
  mPmioRegisterBase = 0;  ///< To be set by PatchSetPciSerialDevice()

STATIC
UINT32
  mPmioRegisterStride = 1; ///< To be set by PatchSetPciSerialDevice()

STATIC
CONST UINTN
  mInOutMaxDistance = 64;

VOID
PatchSetPciSerialDevice (
  IN  UINTN   RegisterBase,
  IN  UINT32  RegisterStride
  )
{
  //
  // FIXME: This is really ugly, make quirks take a context param.
  //
  if (RegisterBase <= MAX_UINT16) {
    DEBUG ((DEBUG_INFO, "OCAK: Registering PCI serial device PMIO port 0x%04X\n", RegisterBase));
    CopyMem (&mPmioRegisterBase, &RegisterBase, sizeof (RegisterBase));

    DEBUG ((DEBUG_INFO, "OCAK: Registering PCI serial device register stride %u\n", RegisterStride));
    CopyMem (&mPmioRegisterStride, &RegisterStride, sizeof (RegisterStride));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: NOT registering unsupported PCI serial device register base 0x%X", RegisterBase));
  }

  //
  // TODO: Add proper MMIO patch.
  //
}

STATIC
EFI_STATUS
PatchCustomPciSerialPmio (
  IN OUT PATCHER_CONTEXT  *Patcher
  )
{
  UINTN  Count;
  UINT8  *Walker;
  UINT8  *WalkerPmio;
  UINTN  Pmio;
  UINT8  *WalkerEnd;
  UINT8  *WalkerTmp;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  Count     = 0;
  Walker    = (UINT8 *)MachoGetMachHeader (&Patcher->MachContext);
  WalkerEnd = Walker + MachoGetInnerSize (&Patcher->MachContext) - mInOutMaxDistance;

  while (Walker < WalkerEnd) {
    if (  (Walker[0] == mSerialDevicePmioFind[0])
       && (Walker[1] == mSerialDevicePmioFind[1])
       && ((Walker[2] & 0xF8U) == mSerialDevicePmioFind[2])
       && (Walker[3] == mSerialDevicePmioFind[3]))
    {
      DEBUG ((
        DEBUG_VERBOSE,
        "OCAK: Matched PMIO serial register base <%02X %02X %02X %02X>\n",
        Walker[0],
        Walker[1],
        Walker[2],
        Walker[3]
        ));
      WalkerPmio = &Walker[2];

      WalkerTmp = Walker + mInOutMaxDistance;
      while (Walker < WalkerTmp) {
        //
        // Locate instruction in (0xEC) or out (0xEE).
        //
        if ((*Walker == 0xEC) || (*Walker == 0xEE)) {
          DEBUG ((
            DEBUG_VERBOSE,
            "OCAK: Matched PMIO serial register base context %a <%02X>, patching register base\n",
            *Walker == 0xEC ? "in" : "out",
            *Walker
            ));

          //
          // Patch PMIO.
          //
          DEBUG ((DEBUG_VERBOSE, "OCAK: Before register base patch <%02X %02X>\n", WalkerPmio[0], WalkerPmio[1]));
          Pmio          = mPmioRegisterBase + (*WalkerPmio & 7U) * mPmioRegisterStride;
          WalkerPmio[0] = Pmio & 0xFFU;
          WalkerPmio[1] = (Pmio >> 8U) & 0xFFU;
          DEBUG ((DEBUG_VERBOSE, "OCAK: After register base patch <%02X %02X>\n", WalkerPmio[0], WalkerPmio[1]));

          ++Count;
          break;
        }

        ++Walker;
      }
    }

    //
    // Continue searching.
    //
    ++Walker;
  }

  if (Count > 0) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patched CustomPciSerialDevice PMIO port %u times\n", Count));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to patch CustomPciSerialDevice PMIO port!\n"));
  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
PatchCustomPciSerialDevice (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  Status = EFI_INVALID_PARAMETER;
  if (  ((mPmioRegisterBase != 0) && (mPmioRegisterStride != 0))
     && ((mPmioRegisterBase + 7U * mPmioRegisterStride) <= MAX_UINT16))
  {
    Status = PatchCustomPciSerialPmio (Patcher);
  }

  //
  // TODO: Check MMIO patch again.
  //

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch CustomPciSerialDevice - %r\n"));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success CustomPciSerialDevice\n"));
  }

  return Status;
}

STATIC
CONST UINT8
  mCustomSmbiosGuidPatchFind[] = {
  0x45, 0x42, 0x39, 0x44, 0x32, 0x44, 0x33, 0x31   ///< "EB9D2D31"
};

STATIC
CONST UINT8
  mCustomSmbiosGuidPatchReplace[] = {
  0x45, 0x42, 0x39, 0x44, 0x32, 0x44, 0x33, 0x35   ///< "EB9D2D35"
};

STATIC
PATCHER_GENERIC_PATCH
  mCustomSmbiosGuidPatch = {
  .Comment     = DEBUG_POINTER ("CustomSmbiosGuid"),
  .Base        = NULL,
  .Find        = mCustomSmbiosGuidPatchFind,
  .Mask        = NULL,
  .Replace     = mCustomSmbiosGuidPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mCustomSmbiosGuidPatchFind),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchCustomSmbiosGuid (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mCustomSmbiosGuidPatch);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] SMBIOS Patch success\n"));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply SMBIOS patch - %r\n", Status));
  }

  return Status;
}

STATIC
CONST UINT8
  mPanicKextDumpPatchFind[] = {
  0x00, 0x25, 0x2E, 0x2A, 0x73, 0x00  ///< "\0%.*s\0"
};

STATIC
CONST UINT8
  mPanicKextDumpPatchReplace[] = {
  0x00, 0x00, 0x2E, 0x2A, 0x73, 0x00  ///< "\0\0.*s\0"
};

STATIC
PATCHER_GENERIC_PATCH
  mPanicKextDumpPatch = {
  .Comment     = DEBUG_POINTER ("PanicKextDump"),
  .Base        = NULL,
  .Find        = mPanicKextDumpPatchFind,
  .Mask        = NULL,
  .Replace     = mPanicKextDumpPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mPanicKextDumpPatchFind),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchPanicKextDump (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;
  UINT8       *Record;
  UINT8       *Last;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping PanicKextDump on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Last = ((UINT8 *)MachoGetMachHeader (&Patcher->MachContext)
          + MachoGetInnerSize (&Patcher->MachContext) - EFI_PAGE_SIZE);

  //
  // This should work on 10.15 and all debug kernels.
  //
  Status = PatcherGetSymbolAddress (
             Patcher,
             "__ZN6OSKext19printKextPanicListsEPFiPKczE",
             (UINT8 **)&Record
             );
  if (EFI_ERROR (Status) || (Record >= Last)) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate printKextPanicLists (%p) - %r\n", Record, Status));
    return EFI_NOT_FOUND;
  }

  *Record = 0xC3;  ///< ret

  //
  // This one is for 10.13~10.14 release kernels, which do dumping inline.
  // A bit risky, but let's hope it works well.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mPanicKextDumpPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply kext dump patch - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success kext dump\n"));
  }

  return Status;
}

STATIC
CONST UINT8
  mLapicKernelPanicPatchFind[] = {
  0x65, 0x8B, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00,  ///< mov eax, gs:1Ch or gs:18h on 10.15.4+ or gs:20h on 11.0.
  0x3B, 0x00, 0x00, 0x00, 0x00, 0x00               ///< cmp eax, cs:_master_cpu <- address masked out
};

STATIC
CONST UINT8
  mLapicKernelPanicPatchMask[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0xFF, 0xFF, 0xFF,
  0xFF, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC
CONST UINT8
  mLapicKernelPanicPatchReplace[] = {
  0x31, 0xC0,                                                            ///< xor eax, eax
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 ///< nop
};

STATIC
PATCHER_GENERIC_PATCH
  mLapicKernelPanicPatch = {
  .Comment     = DEBUG_POINTER ("LapicKernelPanic"),
  .Base        = "_lapic_interrupt",
  .Find        = mLapicKernelPanicPatchFind,
  .Mask        = mLapicKernelPanicPatchMask,
  .Replace     = mLapicKernelPanicPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mLapicKernelPanicPatchReplace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 1024
};

STATIC
CONST UINT8
  mLapicKernelPanicPatchLegacyFind[] = {
  0x65, 0x8B, 0x04, 0x25, 0x10, 0x00, 0x00, 0x00,  ///< mov eax, gs:1Ch on 10.9.5 and 14h on 10.8.5.
  0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,        ///< lea rcx, _master_cpu
  0x00, 0x00                                       ///< cmp eax, [rcx]
};

STATIC
CONST UINT8
  mLapicKernelPanicPatchLegacyMask[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xFF,
  0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00
};

STATIC
CONST UINT8
  mLapicKernelPanicPatchLegacyReplace[] = {
  0x31, 0xC0,                                                                              ///< xor eax, eax
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 ///< nop
};

STATIC
PATCHER_GENERIC_PATCH
  mLapicKernelPanicLegacyPatch = {
  .Comment     = DEBUG_POINTER ("LapicKernelPanicLegacy"),
  .Base        = "_lapic_interrupt",
  .Find        = mLapicKernelPanicPatchLegacyFind,
  .Mask        = mLapicKernelPanicPatchLegacyMask,
  .Replace     = mLapicKernelPanicPatchLegacyReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mLapicKernelPanicPatchLegacyReplace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 1024
};

STATIC
CONST UINT8
  mLapicKernelPanicMasterPatchFind[] = {
  0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ///< cmp cs:_debug_boot_arg, 0 <- address masked out
};

STATIC
CONST UINT8
  mLapicKernelPanicMasterPatchMask[] = {
  0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF
};

STATIC
CONST UINT8
  mLapicKernelPanicMasterPatchReplace[] = {
  0x31, 0xC0,                  ///< xor eax, eax
  0x90, 0x90, 0x90, 0x90, 0x90 ///< nop
};

STATIC
PATCHER_GENERIC_PATCH
  mLapicKernelPanicMasterPatch = {
  .Comment     = DEBUG_POINTER ("LapicKernelPanicMaster"),
  .Base        = "_lapic_interrupt",
  .Find        = mLapicKernelPanicMasterPatchFind,
  .Mask        = mLapicKernelPanicMasterPatchMask,
  .Replace     = mLapicKernelPanicMasterPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mLapicKernelPanicMasterPatchFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
EFI_STATUS
PatchLapicKernelPanic (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  //
  // This one is for <= 10.15 release kernels.
  // TODO: Fix debug kernels and check whether we want more patches.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mLapicKernelPanicPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply modern lapic patch - %r\n", Status));

    Status = PatcherApplyGenericPatch (Patcher, &mLapicKernelPanicLegacyPatch);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success legacy lapic\n"));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply modern lapic patch - %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success lapic\n"));

    //
    // Also patch away the master core check to never require lapic_dont_panic=1.
    // This one is optional, and seems to never be required in real world.
    //
    Status = PatcherApplyGenericPatch (Patcher, &mLapicKernelPanicMasterPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply extended lapic patch - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success extended lapic\n"));
    }

    Status = EFI_SUCCESS;
  }

  return Status;
}

STATIC
CONST UINT8
  mPowerStateTimeoutPanicFind[] = {
  0x63, 0x6F, 0x6D, 0x2E, 0x61, 0x70, 0x70, 0x6C, 0x65, 0x00  ///< "com.apple\0"
};

STATIC
CONST UINT8
  mPowerStateTimeoutPanicReplace[] = {
  // not.apple\0
  0x6E, 0x6F, 0x74, 0x2E, 0x61, 0x70, 0x70, 0x6C, 0x65, 0x00  ///< "not.apple\0"
};

STATIC
PATCHER_GENERIC_PATCH
  mPowerStateTimeoutPanicMasterPatch = {
  .Comment     = DEBUG_POINTER ("PowerStateTimeout"),
  .Base        = NULL,
  .Find        = mPowerStateTimeoutPanicFind,
  .Mask        = NULL,
  .Replace     = mPowerStateTimeoutPanicReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mPowerStateTimeoutPanicFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
CONST UINT8
  mPowerStateTimeoutPanicInlineFind[] = {
  0x80, 0x00, 0x01, 0x6F, ///< cmp byte ptr [rax+1], 6Fh ; 'o'
  0x75, 0x00,             ///< jnz short fail
  0x80, 0x00, 0x02, 0x6D, ///< cmp byte ptr [rax+2], 6Dh ; 'm'
  0x75, 0x00,             ///< jnz short fail
};

STATIC
CONST UINT8
  mPowerStateTimeoutPanicInlineMask[] = {
  0xFF, 0x00, 0xFF, 0xFF, ///< cmp byte ptr [rax+1], 6Fh ; 'o'
  0xFF, 0x00,             ///< jnz short fail
  0xFF, 0x00, 0xFF, 0xFF, ///< cmp byte ptr [rax+2], 6Dh ; 'm'
  0xFF, 0x00,             ///< jnz short fail
};

STATIC
CONST UINT8
  mPowerStateTimeoutPanicInlineReplace[] = {
  0x80, 0x00, 0x01, 0x6E, ///< cmp byte ptr [rax+1], 6Eh ; 'n'
  0x75, 0x00,             ///< jnz short fail
  0x80, 0x00, 0x02, 0x6D, ///< cmp byte ptr [rax+2], 6Dh ; 'm'
  0x75, 0x00,             ///< jnz short fail
};

STATIC
PATCHER_GENERIC_PATCH
  mPowerStateTimeoutPanicInlinePatch = {
  .Comment     = DEBUG_POINTER ("PowerStateTimeout"),
  .Base        = "__ZN9IOService12ackTimerTickEv",
  .Find        = mPowerStateTimeoutPanicInlineFind,
  .Mask        = mPowerStateTimeoutPanicInlineMask,
  .Replace     = mPowerStateTimeoutPanicInlineReplace,
  .ReplaceMask = mPowerStateTimeoutPanicInlineMask,
  .Size        = sizeof (mPowerStateTimeoutPanicInlineFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
EFI_STATUS
PatchPowerStateTimeout (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_CATALINA_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping power state patch on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mPowerStateTimeoutPanicInlinePatch);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success inline power state\n"));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "OCAK: No inline power state patch - %r, trying fallback\n", Status));

  Status = PatcherApplyGenericPatch (Patcher, &mPowerStateTimeoutPanicMasterPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply power state patch - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success power state\n"));
  }

  //
  // TODO: Implement a patch to not require setpowerstate_panic=0 on debug kernels.
  //

  return Status;
}

//
// There currently are 2 places main RTC checksum is calculated in AppleRTC.kext
// __ZN8AppleRTC14updateChecksumEv and __ZN8AppleRTC19rtcRecordTracePointEjjj.
// Since we do not want to completely break RTC and/or saving tracepoints to RTC
// we patch-out __ZN8AppleRTC8rtcWriteEjh call arguments (0x58 and 0x59) with
// invalid (out of range) value 0xFFFF in 4 places.
//
// 10.5 and below do not have __ZN8AppleRTC19rtcRecordTracePointEjjj.
//

STATIC
CONST UINT8
  mAppleRtcChecksumPatchFind32[] = {
  0xC7, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00
};

STATIC
CONST UINT8
  mAppleRtcChecksumPatchMask32[] = {
  0xFF, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFF, 0xFF
};

STATIC
CONST UINT8
  mAppleRtcChecksumPatchReplace32[] = {
  0xC7, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00
};

STATIC
CONST UINT8
  mAppleRtcChecksumPatchFind64[] = {
  0xBE, 0x58, 0x00, 0x00, 0x00
};

STATIC
CONST UINT8
  mAppleRtcChecksumPatchMask64[] = {
  0xFF, 0xFE, 0xFF, 0xFF, 0xFF
};

STATIC
CONST UINT8
  mAppleRtcChecksumPatchReplace64[] = {
  0xBE, 0xFF, 0xFF, 0x00, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
  mAppleRtcChecksumPatch32 = {
  .Comment     = DEBUG_POINTER ("DisableRtcChecksum32"),
  .Base        = NULL,
  .Find        = mAppleRtcChecksumPatchFind32,
  .Mask        = mAppleRtcChecksumPatchMask32,
  .Replace     = mAppleRtcChecksumPatchReplace32,
  .ReplaceMask = mAppleRtcChecksumPatchMask32,
  .Size        = sizeof (mAppleRtcChecksumPatchFind32),
  .Count       = 4,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
PATCHER_GENERIC_PATCH
  mAppleRtcChecksumPatch64 = {
  .Comment     = DEBUG_POINTER ("DisableRtcChecksum64"),
  .Base        = NULL,
  .Find        = mAppleRtcChecksumPatchFind64,
  .Mask        = mAppleRtcChecksumPatchMask64,
  .Replace     = mAppleRtcChecksumPatchReplace64,
  .ReplaceMask = NULL,
  .Size        = sizeof (mAppleRtcChecksumPatchFind64),
  .Count       = 4,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
EFI_STATUS
PatchAppleRtcChecksum (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, Patcher->Is32Bit ? &mAppleRtcChecksumPatch32 : &mAppleRtcChecksumPatch64);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch com.apple.driver.AppleRTC DisableRtcChecksum - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success com.apple.driver.AppleRTC DisableRtcChecksum\n"));
  }

  return Status;
}

STATIC
EFI_STATUS
PatchSegmentJettison (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;
  UINT8       *RemoveBs;
  UINT8       *StaticMfree;
  UINT8       *CurrFreeCall;
  CHAR8       *Jettisoning;
  UINT8       *Last;
  UINTN       Index;
  UINT32      Diff;
  UINT32      Diff2;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_BIG_SUR_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping SegmentJettison on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Last = (UINT8 *)MachoGetMachHeader (&Patcher->MachContext)
         + MachoGetInnerSize (&Patcher->MachContext) - sizeof (EFI_PAGE_SIZE) * 2;

  Status = PatcherGetSymbolAddress (Patcher, "__ZN6OSKext19removeKextBootstrapEv", (UINT8 **)&RemoveBs);
  if (EFI_ERROR (Status) || (RemoveBs > Last)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Missing removeKextBootstrap - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  Status = PatcherGetSymbolAddress (Patcher, "_ml_static_mfree", (UINT8 **)&StaticMfree);
  if (EFI_ERROR (Status) || (StaticMfree > Last)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Missing ml_static_mfree - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  if (RemoveBs - StaticMfree > MAX_INT32) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] ml_static_mfree %p removeKextBootstrap %p are too far\n", StaticMfree, RemoveBs));
    return EFI_UNSUPPORTED;
  }

  //
  // Find the call to _ml_static_mfree.
  //
  // NOTE: One call instruction takes 5 bytes.
  //
  Diff = (UINT32)((UINTN)StaticMfree - (UINTN)RemoveBs - 5);

  CurrFreeCall = NULL;
  for (Index = 0; Index < EFI_PAGE_SIZE; ++Index) {
    if (  (RemoveBs[0] == 0xE8) ///< call
       && (CompareMem (&RemoveBs[1], &Diff, sizeof (Diff)) == 0))
    {
      CurrFreeCall = RemoveBs;
      DEBUG ((
        DEBUG_VERBOSE,
        "OCAK: CurrFreeCall %02X %02X %02X %02X %02X %X\n",
        RemoveBs[0],
        RemoveBs[1],
        RemoveBs[2],
        RemoveBs[3],
        RemoveBs[4],
        Diff
        ));
    } else if (  (CurrFreeCall != NULL)
              && (RemoveBs[0] == 0x48) && (RemoveBs[1] == 0x8D) && (RemoveBs[2] == 0x15))
    {
      //
      // Check if this lea rdx, address is pointing to "Jettisoning fileset Linkedit segments from..."
      //
      CopyMem (&Diff2, &RemoveBs[3], sizeof (Diff2));
      Jettisoning = (CHAR8 *)RemoveBs + Diff2 + 7;
      if (  ((UINT8 *)Jettisoning <= Last)
         && (AsciiStrnCmp (Jettisoning, "Jettisoning fileset", L_STR_LEN ("Jettisoning fileset")) == 0))
      {
        DEBUG ((DEBUG_INFO, "OCAK: [OK] Found jettisoning fileset\n"));
        SetMem (CurrFreeCall, 5, 0x90);
        return EFI_SUCCESS;
      }
    }

    ++RemoveBs;
    --Diff;
  }

  DEBUG ((DEBUG_INFO, "OCAK: Failed to find jettisoning fileset - %p\n", CurrFreeCall));

  return EFI_NOT_FOUND;
}

STATIC
CONST UINT8
  mBTFeatureFlagsReplace[] = {
  0x55,            ///< push rbp
  0x83, 0xCE, 0x0F ///< or esi, 0x0F
};

STATIC
PATCHER_GENERIC_PATCH
  mBTFeatureFlagsPatchV1 = {
  .Comment     = DEBUG_POINTER ("BTFeatureFlagsV1"),
  .Base        = "__ZN25IOBluetoothHostController25SetControllerFeatureFlagsEj",
  .Find        = NULL,
  .Mask        = NULL,
  .Replace     = mBTFeatureFlagsReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mBTFeatureFlagsReplace),
  .Count       = 1,
  .Skip        = 0
};

STATIC
PATCHER_GENERIC_PATCH
  mBTFeatureFlagsPatchV2 = {
  .Comment     = DEBUG_POINTER ("BTFeatureFlagsV2"),
  .Base        = "__ZN24IOBluetoothHCIController25SetControllerFeatureFlagsEj",
  .Find        = NULL,
  .Mask        = NULL,
  .Replace     = mBTFeatureFlagsReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mBTFeatureFlagsReplace),
  .Count       = 1,
  .Skip        = 0
};

STATIC
PATCHER_GENERIC_PATCH
  mBTFeatureFlagsPatchV3 = {
  .Comment     = DEBUG_POINTER ("BTFeatureFlagsV3"),
  .Base        = "__ZN17IOBluetoothDevice25setDeviceSupportedFeatureEj",
  .Find        = NULL,
  .Mask        = NULL,
  .Replace     = mBTFeatureFlagsReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mBTFeatureFlagsReplace),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchBTFeatureFlags (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_MOUNTAIN_LION_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping BTFeatureFlags on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mBTFeatureFlagsPatchV1);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to find BT FeatureFlags symbol v1 - %r, trying v2\n", Status));
    Status = PatcherApplyGenericPatch (Patcher, &mBTFeatureFlagsPatchV2);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to find BT FeatureFlags symbol v2 - %r, trying v3\n", Status));
      Status = PatcherApplyGenericPatch (Patcher, &mBTFeatureFlagsPatchV3);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to find BT FeatureFlags symbol v3 - %r\n", Status));
      } else {
        DEBUG ((DEBUG_INFO, "OCAK: [OK] Success BT FeatureFlags patch v3\n"));
      }
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: [OK] Success BT FeatureFlags patch v2\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Success BT FeatureFlags patch v1\n"));
  }

  return Status;
}

//
// 32-bit commpage_descriptor structure from XNU.
//
typedef struct {
  //
  // Address of code.
  //
  UINT32    CodeAddress;
  //
  // Length of code in bytes.
  //
  UINT32    CodeLength;
  //
  // Address to place this function at.
  //
  UINT32    CommpageAddress;
  //
  // CPU capability bits we must have.
  //
  UINT32    MustHave;
  //
  // CPU capability bits we can't have.
  //
  UINT32    CantHave;
} COMMPAGE_DESCRIPTOR;

//
// 64-bit commpage_descriptor structure from XNU.
//
typedef struct {
  //
  // Address of code.
  //
  UINT64    CodeAddress;
  //
  // Length of code in bytes.
  //
  UINT32    CodeLength;
  //
  // Address to place this function at.
  //
  UINT32    CommpageAddress;
  //
  // CPU capability bits we must have.
  //
  UINT32    MustHave;
  //
  // CPU capability bits we can't have.
  //
  UINT32    CantHave;
} COMMPAGE_DESCRIPTOR_64;

typedef union {
  COMMPAGE_DESCRIPTOR       Desc32;
  COMMPAGE_DESCRIPTOR_64    Desc64;
} COMMPAGE_DESCRIPTOR_ANY;

#define COMM_PAGE_BCOPY       0xFFFF0780
#define kHasSupplementalSSE3  0x00000100

STATIC
CONST UINT8
  mAsmLegacyBcopy64[] = {
  #include "LegacyBcopy.h"
};

STATIC
EFI_STATUS
PatchLegacyCommpage (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;
  UINT8       *Start;
  UINT8       *Last;
  UINT8       *CommpageRoutines;
  UINT8       *Target;
  UINT64      Address;
  UINT32      MaxSize;

  COMMPAGE_DESCRIPTOR_ANY  *Commpage;
  UINT32                   CommpageCodeLength;
  UINT32                   CommpageAddress;
  UINT32                   CommpageMustHave;

  //
  // This is a kernel patch, so Patcher cannot be NULL.
  //
  ASSERT (Patcher != NULL);

  Start = ((UINT8 *)MachoGetMachHeader (&Patcher->MachContext));
  Last  = Start + MachoGetInnerSize (&Patcher->MachContext) - EFI_PAGE_SIZE * 2 - (Patcher->Is32Bit ? sizeof (COMMPAGE_DESCRIPTOR) : sizeof (COMMPAGE_DESCRIPTOR_64));

  //
  // This is a table of pointers to commpage entries.
  //
  Status = PatcherGetSymbolAddress (Patcher, "_commpage_64_routines", (UINT8 **)&CommpageRoutines);
  if (EFI_ERROR (Status) || (CommpageRoutines >= Last)) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate _commpage_64_routines (%p) - %r\n", CommpageRoutines, Status));
    return EFI_NOT_FOUND;
  }

  //
  // Iterate through table looking for bcopy_sse4_64 (on 10.4) or bcopy_sse3x_64 (10.5+) entry.
  //
  Address = Patcher->Is32Bit ? *((UINT32 *)CommpageRoutines) : *((UINT64 *)CommpageRoutines);
  while (Address > 0) {
    Commpage = MachoGetFilePointerByAddress (&Patcher->MachContext, Address, &MaxSize);
    if (  (Commpage == NULL)
       || (MaxSize < (Patcher->Is32Bit ? sizeof (COMMPAGE_DESCRIPTOR) : sizeof (COMMPAGE_DESCRIPTOR_64))))
    {
      break;
    }

    //
    // Locate the bcopy commpage entry that requires SSSE3 and replace it with our own implementation.
    //
    CommpageAddress  = Patcher->Is32Bit ? Commpage->Desc32.CommpageAddress : Commpage->Desc64.CommpageAddress;
    CommpageMustHave = Patcher->Is32Bit ? Commpage->Desc32.MustHave : Commpage->Desc64.MustHave;
    if (  (CommpageAddress == COMM_PAGE_BCOPY)
       && ((CommpageMustHave & kHasSupplementalSSE3) == kHasSupplementalSSE3))
    {
      Address            = Patcher->Is32Bit ? Commpage->Desc32.CodeAddress : Commpage->Desc64.CodeAddress;
      CommpageCodeLength = Patcher->Is32Bit ? Commpage->Desc32.CodeLength : Commpage->Desc64.CodeLength;
      DEBUG ((
        DEBUG_VERBOSE,
        "OCAK: Found 64-bit _COMM_PAGE_BCOPY function @ 0x%llx (0x%X bytes)\n",
        Address,
        CommpageCodeLength
        ));

      Target = MachoGetFilePointerByAddress (&Patcher->MachContext, Address, &MaxSize);
      if (  (Target == NULL)
         || (MaxSize < sizeof (mAsmLegacyBcopy64))
         || (CommpageCodeLength < sizeof (mAsmLegacyBcopy64)))
      {
        break;
      }

      CopyMem (Target, mAsmLegacyBcopy64, sizeof (mAsmLegacyBcopy64));
      if (Patcher->Is32Bit) {
        Commpage->Desc32.CodeLength = sizeof (mAsmLegacyBcopy64);
        Commpage->Desc32.MustHave  &= ~kHasSupplementalSSE3;
      } else {
        Commpage->Desc64.CodeLength = sizeof (mAsmLegacyBcopy64);
        Commpage->Desc64.MustHave  &= ~kHasSupplementalSSE3;
      }

      return EFI_SUCCESS;
    }

    CommpageRoutines += Patcher->Is32Bit ? sizeof (UINT32) : sizeof (UINT64);
    if (CommpageRoutines >= Last) {
      break;
    }

    Address = Patcher->Is32Bit ? *((UINT32 *)CommpageRoutines) : *((UINT64 *)CommpageRoutines);
  }

  DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to find 64-bit _COMM_PAGE_BCOPY function\n"));

  return EFI_NOT_FOUND;
}

STATIC
CONST UINT8
  mAquantiaEthernetPatchFindShikumo[] = {
  0x31, 0xC0,                        ///< xor eax, eax
  0xE8, 0x00, 0x00, 0x00, 0x00,      ///< call _IOLog
  0x83, 0x7D, 0x00, 0x00,            ///< cmp dword [rbp+whatever], whatever
  0x0F, 0x84, 0x00, 0x00, 0x00, 0x00 ///< je unsupported
};

STATIC
CONST UINT8
  mAquantiaEthernetPatchFindMaskShikumo[] = {
  0xFF, 0xFF,
  0xFF, 0x00,0x00,  0x00, 0x00,
  0xFF, 0xFF,0x00,  0x00,
  0xFF, 0xFF,0x00,  0x00, 0x00, 0x00
};

STATIC
CONST UINT8
  mAquantiaEthernetPatchReplaceShikumo[] = {
  0x00, 0x00,
  0x00, 0x00,0x00,  0x00, 0x00,
  0x00, 0x00,0x00,  0x00,
  0x90, 0x90,0x90,  0x90, 0x90, 0x90, ///< nop (je unsupported)
};

STATIC
CONST UINT8
  mAquantiaEthernetPatchReplaceMaskShikumo[] = {
  0x00, 0x00,
  0x00, 0x00,0x00,  0x00, 0x00,
  0x00, 0x00,0x00,  0x00,
  0xFF, 0xFF,0xFF,  0xFF, 0xFF, 0xFF
};

STATIC
PATCHER_GENERIC_PATCH
  mAquantiaEthernetPatchShikumo = {
  .Comment     = DEBUG_POINTER ("ForceAquantiaEthernetShikumo"),
  .Base        = "__ZN27AppleEthernetAquantiaAqtion5startEP9IOService",
  .Find        = mAquantiaEthernetPatchFindShikumo,
  .Mask        = mAquantiaEthernetPatchFindMaskShikumo,
  .Replace     = mAquantiaEthernetPatchReplaceShikumo,
  .ReplaceMask = mAquantiaEthernetPatchReplaceMaskShikumo,
  .Size        = sizeof (mAquantiaEthernetPatchFindShikumo),
  .Count       = 1,
  .Skip        = 0
};

STATIC
CONST UINT8
  mAquantiaEthernetPatchFindMieze[] = {
  0x41, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  ///< mov dword ptr [whatever], 0
  0xE9                                             ///< jmp
};

STATIC
CONST UINT8
  mAquantiaEthernetPatchReplaceMieze[] = {
  0x41, 0xC7, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,  ///< mov dword ptr [whatever], 1
  0xE9                                             ///< jmp
};

STATIC
CONST UINT8
  mAquantiaEthernetPatchMaskMieze[] = {
  0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
  0xFF
};

STATIC
PATCHER_GENERIC_PATCH
  mAquantiaEthernetPatchMieze = {
  .Comment     = DEBUG_POINTER ("ForceAquantiaEthernetMieze"),
  .Base        = NULL,
  .Find        = mAquantiaEthernetPatchFindMieze,
  .Mask        = mAquantiaEthernetPatchMaskMieze,
  .Replace     = mAquantiaEthernetPatchReplaceMieze,
  .ReplaceMask = mAquantiaEthernetPatchMaskMieze,
  .Size        = sizeof (mAquantiaEthernetPatchFindMieze),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchAquantiaEthernet (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  //
  // This patch is not required before macOS 10.15.4.
  //
  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION (KERNEL_VERSION_CATALINA, 4, 0), 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping patching AquantiaEthernet on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  //
  // Shikumo's patch can be applied to a wider range, not limited to AQC 107 series,
  // thus preferred.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mAquantiaEthernetPatchShikumo);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success Aquantia Ethernet Shikumo\n"));
    return Status;
  }

  //
  // In case Shikumo's patch failed, try Mieze's so at least AQC 107 will work.
  //
  DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply Aquantia Ethernet patch Shikumo - %r, trying Mieze\n", Status));
  Status = PatcherApplyGenericPatch (Patcher, &mAquantiaEthernetPatchMieze);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply Aquantia Ethernet patch Mieze - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success Aquantia Ethernet Mieze\n"));
  }

  return Status;
}

STATIC
EFI_STATUS
PatchForceSecureBootScheme (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;
  UINT8       *Last;
  UINT8       *SelectAp;
  UINT8       *HybridAp;
  UINT32      Diff;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_BIG_SUR_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping sb scheme on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  //
  // This code is for debugging APFS snapshot verification for Big Sur.
  // macOS chooses verification scheme based on the hardware:
  // - __img4_chip_x86_software_8012 (software x86 8012)
  //   for CPUs with VMM flag enabled via cpuid_features.
  // - __img4_chip_x86 (x86)
  //   for platforms with no or v1 (0x10000) coprocessor (apple-coprocessor-version).
  // - __img4_chip_ap_hybrid_medium (medium-security hybrid arm/x86 ap)
  //   for platforms with v2 (0x20000) coprocessor and medium (1) policy (AppleSecureBootPolicy).
  // - __img4_chip_ap_hybrid_relaxed (relaxed hybrid arm/x86 ap)
  //   for platforms with v2 coprocessor and relaxed (0) policy.
  // - __img4_chip_ap_hybrid (hybrid arm/x86 ap)
  //   for platfirms with v2 or newer coprocessor and personalised policy (2).
  //

  Last = ((UINT8 *)MachoGetMachHeader (&Patcher->MachContext)
          + MachoGetInnerSize (&Patcher->MachContext) - 64);

  Status = PatcherGetSymbolAddress (Patcher, "_img4_chip_select_effective_ap", &SelectAp);
  if (EFI_ERROR (Status) || (SelectAp > Last)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Missing _img4_chip_select_effective_ap - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  Status = PatcherGetSymbolAddress (Patcher, "__img4_chip_x86", &HybridAp);
  if (EFI_ERROR (Status) || (HybridAp > Last)) {
    DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Missing __img4_chip_x86 - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "OCAK: Forcing sb scheme on %u\n", KernelVersion));

  SelectAp[0] = 0x48;
  SelectAp[1] = 0x8D;
  SelectAp[2] = 0x05;
  Diff        = (UINT32)(HybridAp - SelectAp - 7);
  CopyMem (&SelectAp[3], &Diff, sizeof (Diff));
  SelectAp[7] = 0xC3;
  return EFI_SUCCESS;
}

STATIC
UINT8
  mApfsTimeoutFind[] = {
  0x48, 0x3D, 0x7F, 0x96, 0x98, 0x00  ///< cmp rax, 9999999d
};
STATIC_ASSERT (sizeof (mApfsTimeoutFind) == 6, "Unsupported mApfsTimeoutFind");

STATIC
UINT8
  mApfsTimeoutReplace[] = {
  0x48, 0x3D, 0x00, 0x00, 0x00, 0x00  ///< cmp rax, <new_timeout> ; To be set by PatchSetApfsTimeout()
};
STATIC_ASSERT (sizeof (mApfsTimeoutReplace) == 6, "Unsupported mApfsTimeoutReplace");

STATIC
PATCHER_GENERIC_PATCH
  mApfsTimeoutPatch = {
  .Comment     = DEBUG_POINTER ("ApfsTimeout"),
  .Base        = "_nx_mount_trim_thread",
  .Find        = mApfsTimeoutFind,
  .Mask        = NULL,
  .Replace     = mApfsTimeoutReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mApfsTimeoutFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

VOID
PatchSetApfsTimeout (
  IN UINT32  Timeout
  )
{
  //
  // FIXME: This is really ugly, make quirks take a context param.
  //
  DEBUG ((DEBUG_INFO, "OCAK: Registering %u APFS timeout\n", Timeout));
  CopyMem (&mApfsTimeoutReplace[2], &Timeout, sizeof (Timeout));
}

STATIC
UINT8
  mApfsDisableTrimReplace[] = {
  0x31, 0xC0,  ///< xor eax, eax
  0xC3         ///< ret
};

STATIC
PATCHER_GENERIC_PATCH
  mApfsDisableTrimPatch = {
  .Comment     = DEBUG_POINTER ("ApfsTimeout disable trim"),
  .Base        = "_spaceman_iterate_free_extents_internal",
  .Find        = NULL,
  .Mask        = NULL,
  .Replace     = mApfsDisableTrimReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mApfsDisableTrimReplace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
EFI_STATUS
PatchSetApfsTrimTimeout (
  IN OUT PATCHER_CONTEXT  *Patcher OPTIONAL,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_MOJAVE_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping apfs timeout on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping %a on NULL Patcher on %u\n", __func__, KernelVersion));
    return EFI_NOT_FOUND;
  }

  //
  // Disable trim using another patch when timeout is 0.
  //
  if (IsZeroBuffer (&mApfsTimeoutReplace[2], sizeof (UINT32))) {
    Status = PatcherApplyGenericPatch (Patcher, &mApfsDisableTrimPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch ApfsDisableTrim - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success ApfsDisableTrim\n"));
    }

    return Status;
  }

  //
  // It is only possible to specify trim timeout value from 10.14 to 11.x.
  // Starting at 12.0 this is no longer possible.
  //
  if (KernelVersion < KERNEL_VERSION_MONTEREY_MIN) {
    Status = PatcherApplyGenericPatch (Patcher, &mApfsTimeoutPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to apply patch SetApfsTrimTimeout - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success SetApfsTrimTimeout\n"));
    }

    return Status;
  }

  DEBUG ((DEBUG_INFO, "OCAK: Skipping SetApfsTrimTimeout on macOS 12.0+\n"));
  return EFI_SUCCESS;
}

//
// Quirks table.
//
KERNEL_QUIRK  gKernelQuirks[] = {
  [KernelQuirkAppleCpuPmCfgLock]       = { "com.apple.driver.AppleIntelCPUPowerManagement", PatchAppleCpuPmCfgLock      },
  [KernelQuirkAppleXcpmCfgLock]        = { NULL,                                            PatchAppleXcpmCfgLock       },
  [KernelQuirkAppleXcpmExtraMsrs]      = { NULL,                                            PatchAppleXcpmExtraMsrs     },
  [KernelQuirkAppleXcpmForceBoost]     = { NULL,                                            PatchAppleXcpmForceBoost    },
  [KernelQuirkCustomPciSerialDevice]   = { NULL,                                            PatchCustomPciSerialDevice  },
  [KernelQuirkCustomSmbiosGuid1]       = { "com.apple.driver.AppleSMBIOS",                  PatchCustomSmbiosGuid       },
  [KernelQuirkCustomSmbiosGuid2]       = { "com.apple.driver.AppleACPIPlatform",            PatchCustomSmbiosGuid       },
  [KernelQuirkDisableIoMapper]         = { "com.apple.iokit.IOPCIFamily",                   PatchAppleIoMapperSupport   },
  [KernelQuirkDisableIoMapperMapping]  = { "com.apple.iokit.IOPCIFamily",                   PatchAppleIoMapperMapping   },
  [KernelQuirkDisableRtcChecksum]      = { "com.apple.driver.AppleRTC",                     PatchAppleRtcChecksum       },
  [KernelQuirkDummyPowerManagement]    = { "com.apple.driver.AppleIntelCPUPowerManagement", PatchDummyPowerManagement   },
  [KernelQuirkExtendBTFeatureFlags]    = { "com.apple.iokit.IOBluetoothFamily",             PatchBTFeatureFlags         },
  [KernelQuirkExternalDiskIcons]       = { "com.apple.driver.AppleAHCIPort",                PatchForceInternalDiskIcons },
  [KernelQuirkForceAquantiaEthernet]   = { "com.apple.driver.AppleEthernetAquantiaAqtion",  PatchAquantiaEthernet       },
  [KernelQuirkForceSecureBootScheme]   = { "com.apple.security.AppleImage4",                PatchForceSecureBootScheme  },
  [KernelQuirkIncreasePciBarSize]      = { "com.apple.iokit.IOPCIFamily",                   PatchIncreasePciBarSize     },
  [KernelQuirkLapicKernelPanic]        = { NULL,                                            PatchLapicKernelPanic       },
  [KernelQuirkLegacyCommpage]          = { NULL,                                            PatchLegacyCommpage         },
  [KernelQuirkPanicNoKextDump]         = { NULL,                                            PatchPanicKextDump          },
  [KernelQuirkPowerTimeoutKernelPanic] = { NULL,                                            PatchPowerStateTimeout      },
  [KernelQuirkSegmentJettison]         = { NULL,                                            PatchSegmentJettison        },
  [KernelQuirkSetApfsTrimTimeout]      = { "com.apple.filesystems.apfs",                    PatchSetApfsTrimTimeout     },
  [KernelQuirkThirdPartyDrives]        = { "com.apple.iokit.IOAHCIBlockStorage",            PatchThirdPartyDriveSupport },
  [KernelQuirkXhciPortLimit1]          = { "com.apple.iokit.IOUSBHostFamily",               PatchUsbXhciPortLimit1      },
  [KernelQuirkXhciPortLimit2]          = { "com.apple.driver.usb.AppleUSBXHCI",             PatchUsbXhciPortLimit2      },
  [KernelQuirkXhciPortLimit3]          = { "com.apple.driver.usb.AppleUSBXHCIPCI",          PatchUsbXhciPortLimit3      },
};

EFI_STATUS
KernelApplyQuirk (
  IN     KERNEL_QUIRK_NAME  Name,
  IN OUT PATCHER_CONTEXT    *Patcher OPTIONAL,
  IN     UINT32             KernelVersion
  )
{
  //
  // Patcher cannot be NULL for kernel patches, whose Identifier are NULL.
  //
  if (gKernelQuirks[Name].Identifier == NULL) {
    ASSERT (Patcher != NULL);
  }

  return gKernelQuirks[Name].PatchFunction (Patcher, KernelVersion);
}
