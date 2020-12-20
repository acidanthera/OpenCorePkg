/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "ocvalidate.h"

#include <Library/MemoryAllocationLib.h>

#include <Library/OcAppleKernelLib.h>

UINT32
CheckKernel (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32              ErrorCount;
  UINT32              Index;
  UINT32              LiluIndex;
  OC_KERNEL_CONFIG    *UserKernel;
  OC_PLATFORM_CONFIG  *UserPlatformInfo;
  CONST CHAR8         *Arch;
  CONST CHAR8         *BundlePath;
  CONST CHAR8         *Comment;
  CONST CHAR8         *ExecutablePath;
  CONST CHAR8         *MaxKernel;
  CONST CHAR8         *MinKernel;
  CONST CHAR8         *PlistPath;
  CONST CHAR8         *Identifier;
  BOOLEAN             HasLiluKext;
  BOOLEAN             IsDisableLinkeditJettisonEnabled;
  BOOLEAN             IsCustomSMBIOSGuidEnabled;
  CONST CHAR8         *UpdateSMBIOSMode;
  CONST CHAR8         *Base;
  CONST UINT8         *Find;
  BOOLEAN             FindSizeCanBeZero;
  UINT32              FindSize;
  CONST UINT8         *Replace;
  UINT32              ReplaceSize;
  CONST UINT8         *Mask;
  UINT32              MaskSize;
  CONST UINT8         *ReplaceMask;
  UINT32              ReplaceMaskSize;

  DEBUG ((DEBUG_VERBOSE, "config loaded into Kernel checker!\n"));

  ErrorCount                       = 0;
  LiluIndex                        = 0;
  UserKernel                       = &Config->Kernel;
  UserPlatformInfo                 = &Config->PlatformInfo;
  HasLiluKext                      = FALSE;
  IsDisableLinkeditJettisonEnabled = UserKernel->Quirks.DisableLinkeditJettison;
  IsCustomSMBIOSGuidEnabled        = UserKernel->Quirks.CustomSmbiosGuid;
  UpdateSMBIOSMode                 = OC_BLOB_GET (&UserPlatformInfo->UpdateSmbiosMode);
  FindSizeCanBeZero                = FALSE;

  for (Index = 0; Index < UserKernel->Add.Count; ++Index) {
    Arch            = OC_BLOB_GET (&UserKernel->Add.Values[Index]->Arch);
    BundlePath      = OC_BLOB_GET (&UserKernel->Add.Values[Index]->BundlePath);
    Comment         = OC_BLOB_GET (&UserKernel->Add.Values[Index]->Comment);
    ExecutablePath  = OC_BLOB_GET (&UserKernel->Add.Values[Index]->ExecutablePath);
    MaxKernel       = OC_BLOB_GET (&UserKernel->Add.Values[Index]->MaxKernel);
    MinKernel       = OC_BLOB_GET (&UserKernel->Add.Values[Index]->MinKernel);
    PlistPath       = OC_BLOB_GET (&UserKernel->Add.Values[Index]->PlistPath);

    //
    // Sanitise strings.
    //
    if (!AsciiArchIsLegal (Arch)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->Arch is borked (Can only be Any, i386, and x86_64)!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiFileSystemPathIsLegal (BundlePath)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->BundlePath contains illegal character!\n", Index));
      ++ErrorCount;
    } else {
      if (!AsciiFileNameHasSuffix (BundlePath, "kext")) {
        DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->BundlePath does NOT contain .kext suffix!\n", Index));
        ++ErrorCount;
      }
    }
    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiFileSystemPathIsLegal (ExecutablePath)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->ExecutablePath contains illegal character!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiFileSystemPathIsLegal (PlistPath)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->PlistPath contains illegal character!\n", Index));
      ++ErrorCount;
    } else {
      if (!AsciiFileNameHasSuffix (PlistPath, "plist")) {
        DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->PlistPath does NOT contain .plist suffix!\n", Index));
        ++ErrorCount;
      }
    }

    //
    // FIXME: Handle correct kernel version checking.
    //
    if (MaxKernel[0] != '\0' && OcParseDarwinVersion (MaxKernel) == 0) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->MaxKernel (currently set to %a) is borked!\n", Index, MaxKernel));
      ++ErrorCount;
    }
    if (MinKernel[0] != '\0' && OcParseDarwinVersion (MinKernel) == 0) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->MinKernel (currently set to %a) is borked!\n", Index, MinKernel));
      ++ErrorCount;
    }

    //
    // TODO: Bring more special checks to kexts from Acidanthera.
    //
    if (AsciiStrCmp (BundlePath, "Lilu.kext") == 0
      && AsciiStrCmp (ExecutablePath, "Contents/MacOS/Lilu") == 0) {
      HasLiluKext = TRUE;
      LiluIndex   = Index;
    }
  }

  for (Index = 0; Index < UserKernel->Block.Count; ++Index) {
    Arch            = OC_BLOB_GET (&UserKernel->Block.Values[Index]->Arch);
    Comment         = OC_BLOB_GET (&UserKernel->Block.Values[Index]->Comment);
    Identifier      = OC_BLOB_GET (&UserKernel->Block.Values[Index]->Identifier);
    MaxKernel       = OC_BLOB_GET (&UserKernel->Block.Values[Index]->MaxKernel);
    MinKernel       = OC_BLOB_GET (&UserKernel->Block.Values[Index]->MinKernel);
    
    //
    // Sanitise strings.
    //
    if (!AsciiArchIsLegal (Arch)) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->Arch is borked (Can only be Any, i386, and x86_64)!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiIdentifierIsLegal (Identifier, FALSE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->Identifier contains illegal character!\n", Index));
      ++ErrorCount;
    }

    //
    // FIXME: Handle correct kernel version checking.
    //
    if (MaxKernel[0] != '\0' && OcParseDarwinVersion (MaxKernel) == 0) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->MaxKernel (currently set to %a) is borked!\n", Index, MaxKernel));
      ++ErrorCount;
    }
    if (MinKernel[0] != '\0' && OcParseDarwinVersion (MinKernel) == 0) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->MinKernel (currently set to %a) is borked!\n", Index, MinKernel));
      ++ErrorCount;
    }
  }

  //
  // TODO: Handle Force checks here...
  //

  for (Index = 0; Index < UserKernel->Patch.Count; ++Index) {
    Base            = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->Base);
    Comment         = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->Comment);
    Arch            = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->Arch);
    Identifier      = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->Identifier);
    Find            = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->Find);
    FindSize        = UserKernel->Patch.Values[Index]->Find.Size;
    Replace         = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->Replace);
    ReplaceSize     = UserKernel->Patch.Values[Index]->Replace.Size;
    Mask            = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->Mask);
    MaskSize        = UserKernel->Patch.Values[Index]->Mask.Size;
    ReplaceMask     = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->ReplaceMask);
    ReplaceMaskSize = UserKernel->Patch.Values[Index]->ReplaceMask.Size;

    //
    // Sanitise strings.
    //
    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiArchIsLegal (Arch)) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->Arch is borked (Can only be Any, i386, and x86_64)!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiIdentifierIsLegal (Identifier, TRUE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->Identifier contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (Base[0] != '\0' && FindSize == 0) {
      FindSizeCanBeZero = TRUE;
    }
    if (!FindSizeCanBeZero && FindSize != ReplaceSize) {
      DEBUG ((
        DEBUG_WARN,
        "Kernel->Patch[%u] has different Find and Replace size (%u vs %u) when Base is not in use!\n",
        Index,
        FindSize,
        ReplaceSize
        ));
      ++ErrorCount;
    }
    if (MaskSize > 0) {
      if (MaskSize != FindSize) {
        DEBUG ((
          DEBUG_WARN,
          "Kernel->Patch[%u] has Mask set but its size is different from Find (%u vs %u)!\n",
          Index,
          MaskSize,
          FindSize
          ));
        ++ErrorCount;
      } else {
        if (!DataHasProperMasking (Find, Mask, FindSize)) {
          DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->Find requires Mask to be active for corresponding bits!\n", Index));
          ++ErrorCount;
        }
      }
    }
    if (ReplaceMaskSize > 0) {
      if (ReplaceMaskSize != ReplaceSize) {
        DEBUG ((
          DEBUG_WARN,
          "Kernel->Patch[%u] has ReplaceMask set but its size is different from Replace (%u vs %u)!\n",
          Index,
          ReplaceMaskSize,
          ReplaceSize
          ));
        ++ErrorCount;
      } else {
        if (!DataHasProperMasking (Replace, ReplaceMask, ReplaceSize)) {
          DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->Replace requires ReplaceMask to be active for corresponding bits!\n", Index));
          ++ErrorCount;
        }
      }
    }
  }

  //
  // FIXME: Handle correct kernel version checking.
  //
  MaxKernel = OC_BLOB_GET (&UserKernel->Emulate.MaxKernel);
  MinKernel = OC_BLOB_GET (&UserKernel->Emulate.MinKernel);
  if (MaxKernel[0] != '\0' && OcParseDarwinVersion (MaxKernel) == 0) {
    DEBUG ((DEBUG_WARN, "Kernel->Emulate->MaxKernel (currently set to %a) is borked!\n", MaxKernel));
    ++ErrorCount;
  }
  if (MinKernel[0] != '\0' && OcParseDarwinVersion (MinKernel) == 0) {
    DEBUG ((DEBUG_WARN, "Kernel->Emulate->MinKernel (currently set to %a) is borked!\n", MinKernel));
    ++ErrorCount;
  }

  if (!DataHasProperMasking (UserKernel->Emulate.Cpuid1Data, UserKernel->Emulate.Cpuid1Mask, sizeof (UserKernel->Emulate.Cpuid1Data))) {
    DEBUG ((DEBUG_WARN, "Kernel->Emulate->Cpuid1Data requires Cpuid1Mask to be active for replaced bits!\n"));
    ++ErrorCount;
  }

  //
  // CustomSMBIOSGuid quirk requires UpdateSMBIOSMode at PlatformInfo set to Custom.
  //
  if (IsCustomSMBIOSGuidEnabled && AsciiStrCmp (UpdateSMBIOSMode, "Custom") != 0) {
    DEBUG ((DEBUG_WARN, "Kernel->Quirks->CustomSMBIOSGuid is enabled, but PlatformInfo->UpdateSMBIOSMode is not set to Custom!\n"));
    ++ErrorCount;
  }

  //
  // DisableLinkeditJettison should be enabled when Lilu is in use.
  //
  if (HasLiluKext && !IsDisableLinkeditJettisonEnabled) {
    DEBUG ((DEBUG_WARN, "Lilu.kext is loaded at Kernel->Add[%u], but DisableLinkeditJettison is not enabled at Kernel->Quirks!\n", LiluIndex));
    ++ErrorCount;
  }

  return ReportError (__func__, ErrorCount);
}
