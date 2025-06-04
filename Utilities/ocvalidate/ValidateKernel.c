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
#include "OcValidateLib.h"
#include "KextInfo.h"

#include <Library/OcAppleKernelLib.h>

/**
  Callback function to verify whether BundlePath is duplicated in Kernel->Add.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
KernelAddHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  CONST OC_KERNEL_ADD_ENTRY  *KernelAddPrimaryEntry;
  CONST OC_KERNEL_ADD_ENTRY  *KernelAddSecondaryEntry;
  CONST CHAR8                *KernelAddPrimaryBundlePathString;
  CONST CHAR8                *KernelAddSecondaryBundlePathString;

  KernelAddPrimaryEntry              = *(CONST OC_KERNEL_ADD_ENTRY **)PrimaryEntry;
  KernelAddSecondaryEntry            = *(CONST OC_KERNEL_ADD_ENTRY **)SecondaryEntry;
  KernelAddPrimaryBundlePathString   = OC_BLOB_GET (&KernelAddPrimaryEntry->BundlePath);
  KernelAddSecondaryBundlePathString = OC_BLOB_GET (&KernelAddSecondaryEntry->BundlePath);

  if (!KernelAddPrimaryEntry->Enabled || !KernelAddSecondaryEntry->Enabled) {
    return FALSE;
  }

  return StringIsDuplicated ("Kernel->Add", KernelAddPrimaryBundlePathString, KernelAddSecondaryBundlePathString);
}

/**
  Callback function to verify whether Identifier is duplicated in Kernel->Block.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
KernelBlockHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  CONST OC_KERNEL_BLOCK_ENTRY  *KernelBlockPrimaryEntry;
  CONST OC_KERNEL_BLOCK_ENTRY  *KernelBlockSecondaryEntry;
  CONST CHAR8                  *KernelBlockPrimaryIdentifierString;
  CONST CHAR8                  *KernelBlockSecondaryIdentifierString;

  KernelBlockPrimaryEntry              = *(CONST OC_KERNEL_BLOCK_ENTRY **)PrimaryEntry;
  KernelBlockSecondaryEntry            = *(CONST OC_KERNEL_BLOCK_ENTRY **)SecondaryEntry;
  KernelBlockPrimaryIdentifierString   = OC_BLOB_GET (&KernelBlockPrimaryEntry->Identifier);
  KernelBlockSecondaryIdentifierString = OC_BLOB_GET (&KernelBlockSecondaryEntry->Identifier);

  if (!KernelBlockPrimaryEntry->Enabled || !KernelBlockSecondaryEntry->Enabled) {
    return FALSE;
  }

  return StringIsDuplicated ("Kernel->Block", KernelBlockPrimaryIdentifierString, KernelBlockSecondaryIdentifierString);
}

/**
  Callback function to verify whether BundlePath is duplicated in Kernel->Force.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
KernelForceHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  //
  // NOTE: Add and Force share the same constructor.
  //
  CONST OC_KERNEL_ADD_ENTRY  *KernelForcePrimaryEntry;
  CONST OC_KERNEL_ADD_ENTRY  *KernelForceSecondaryEntry;
  CONST CHAR8                *KernelForcePrimaryBundlePathString;
  CONST CHAR8                *KernelForceSecondaryBundlePathString;

  KernelForcePrimaryEntry              = *(CONST OC_KERNEL_ADD_ENTRY **)PrimaryEntry;
  KernelForceSecondaryEntry            = *(CONST OC_KERNEL_ADD_ENTRY **)SecondaryEntry;
  KernelForcePrimaryBundlePathString   = OC_BLOB_GET (&KernelForcePrimaryEntry->BundlePath);
  KernelForceSecondaryBundlePathString = OC_BLOB_GET (&KernelForceSecondaryEntry->BundlePath);

  if (!KernelForcePrimaryEntry->Enabled || !KernelForceSecondaryEntry->Enabled) {
    return FALSE;
  }

  return StringIsDuplicated ("Kernel->Force", KernelForcePrimaryBundlePathString, KernelForceSecondaryBundlePathString);
}

STATIC
UINT32
CheckKernelAdd (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  UINT32       Index;
  CONST CHAR8  *Arch;
  CONST CHAR8  *BundlePath;
  UINTN        BundlePathSumSize;
  CONST CHAR8  *Comment;
  CONST CHAR8  *ExecutablePath;
  UINTN        ExecutableUserLength;
  UINTN        ExecutablePathSumSize;
  CONST CHAR8  *MaxKernel;
  CONST CHAR8  *MinKernel;
  CONST CHAR8  *PlistPath;
  UINTN        PlistUserLength;
  UINTN        PlistPathSumSize;
  BOOLEAN      IsLiluUsed;
  BOOLEAN      IsDisableLinkeditJettisonEnabled;
  UINTN        IndexKextInfo;
  UINTN        IndexKextPrecedence;
  BOOLEAN      HasParent;
  CONST CHAR8  *CurrentKext;
  CONST CHAR8  *ParentKext;
  CONST CHAR8  *ChildKext;

  ErrorCount = 0;

  for (Index = 0; Index < Config->Kernel.Add.Count; ++Index) {
    Arch           = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->Arch);
    BundlePath     = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->BundlePath);
    Comment        = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->Comment);
    ExecutablePath = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->ExecutablePath);
    MaxKernel      = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->MaxKernel);
    MinKernel      = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->MinKernel);
    PlistPath      = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->PlistPath);

    //
    // Sanitise strings.
    //
    if (!AsciiArchIsLegal (Arch, FALSE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->Arch is borked (Can only be Any, i386, and x86_64)!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiFileSystemPathIsLegal (BundlePath)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->BundlePath contains illegal character!\n", Index));
      ++ErrorCount;
      continue;
    }

    //
    // Valid BundlePath must contain .kext suffix.
    //
    if (!OcAsciiEndsWith (BundlePath, ".kext", TRUE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->BundlePath does NOT contain .kext suffix!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiFileSystemPathIsLegal (ExecutablePath)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->ExecutablePath contains illegal character!\n", Index));
      ++ErrorCount;
      continue;
    }

    if (!AsciiFileSystemPathIsLegal (PlistPath)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->PlistPath contains illegal character!\n", Index));
      ++ErrorCount;
      continue;
    }

    //
    // Valid PlistPath must contain .plist suffix.
    //
    if (!OcAsciiEndsWith (PlistPath, ".plist", TRUE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->PlistPath does NOT contain .plist suffix!\n", Index));
      ++ErrorCount;
    }

    //
    // Check the length of path relative to OC directory.
    //
    BundlePathSumSize = L_STR_LEN (OPEN_CORE_KEXT_PATH) + AsciiStrSize (BundlePath);
    if (BundlePathSumSize > OC_STORAGE_SAFE_PATH_MAX) {
      DEBUG ((
        DEBUG_WARN,
        "Kernel->Add[%u]->BundlePath (length %u) is too long (should not exceed %u)!\n",
        Index,
        AsciiStrLen (BundlePath),
        OC_STORAGE_SAFE_PATH_MAX - L_STR_LEN (OPEN_CORE_KEXT_PATH) - 1
        ));
      ++ErrorCount;
    }

    //
    // There is one missing '\\' after the concatenation of BundlePath and ExecutablePath. Append one.
    //
    ExecutableUserLength  = AsciiStrLen (BundlePath) + 1 + AsciiStrLen (ExecutablePath);
    ExecutablePathSumSize = L_STR_LEN (OPEN_CORE_KEXT_PATH) + ExecutableUserLength + 1;
    if (ExecutablePathSumSize > OC_STORAGE_SAFE_PATH_MAX) {
      DEBUG ((
        DEBUG_WARN,
        "Kernel->Add[%u]->BundlePath + '\\' + ExecutablePath (length %u) is too long (should not exceed %u)!\n",
        Index,
        ExecutableUserLength,
        OC_STORAGE_SAFE_PATH_MAX - L_STR_LEN (OPEN_CORE_KEXT_PATH) - 1
        ));
      ++ErrorCount;
    }

    //
    // There is one missing '\\' after the concatenation of BundlePath and PlistPath. Append one.
    //
    PlistUserLength  = AsciiStrLen (BundlePath) + 1 + AsciiStrLen (PlistPath);
    PlistPathSumSize = L_STR_LEN (OPEN_CORE_KEXT_PATH) + PlistUserLength + 1;
    if (PlistPathSumSize > OC_STORAGE_SAFE_PATH_MAX) {
      DEBUG ((
        DEBUG_WARN,
        "Kernel->Add[%u]->BundlePath + '\\' + PlistPath (length %u) is too long (should not exceed %u)!\n",
        Index,
        PlistUserLength,
        OC_STORAGE_SAFE_PATH_MAX - L_STR_LEN (OPEN_CORE_KEXT_PATH) - 1
        ));
      ++ErrorCount;
    }

    //
    // MinKernel must not be below macOS 10.4 (Darwin version 8).
    //
    if (!OcMatchDarwinVersion (OcParseDarwinVersion (MinKernel), KERNEL_VERSION_TIGER_MIN, 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->MinKernel has a Darwin version %a, which is below 8 (macOS 10.4)!\n", Index, MinKernel));
      ++ErrorCount;
    }

    //
    // FIXME: Handle correct kernel version checking.
    //
    if ((MaxKernel[0] != '\0') && (OcParseDarwinVersion (MaxKernel) == 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->MaxKernel (currently set to %a) is borked!\n", Index, MaxKernel));
      ++ErrorCount;
    }

    if ((MinKernel[0] != '\0') && (OcParseDarwinVersion (MinKernel) == 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Add[%u]->MinKernel (currently set to %a) is borked!\n", Index, MinKernel));
      ++ErrorCount;
    }

    for (IndexKextInfo = 0; IndexKextInfo < mKextInfoSize; ++IndexKextInfo) {
      if (AsciiStrCmp (BundlePath, mKextInfo[IndexKextInfo].KextBundlePath) == 0) {
        //
        // BundlePath matched. Continue checking ExecutablePath and PlistPath.
        //
        if (  (AsciiStrCmp (ExecutablePath, mKextInfo[IndexKextInfo].KextExecutablePath) == 0)
           && (AsciiStrCmp (PlistPath, mKextInfo[IndexKextInfo].KextPlistPath) == 0))
        {
          //
          // Special check for Lilu and Quirks->DisableLinkeditJettison.
          //
          if (IndexKextInfo == INDEX_KEXT_LILU) {
            IsLiluUsed                       = Config->Kernel.Add.Values[Index]->Enabled;
            IsDisableLinkeditJettisonEnabled = Config->Kernel.Quirks.DisableLinkeditJettison;
            if (IsLiluUsed && !IsDisableLinkeditJettisonEnabled) {
              DEBUG ((DEBUG_WARN, "Lilu.kext is loaded at Kernel->Add[%u], but DisableLinkeditJettison is not enabled at Kernel->Quirks!\n", Index));
              ++ErrorCount;
            }
          }

          //
          // Special check for BrcmFirmwareRepo, which cannot be injected by OC.
          //
          if (AsciiStrCmp (BundlePath, "BrcmFirmwareRepo.kext") == 0) {
            DEBUG ((DEBUG_WARN, "BrcmFirmwareRepo.kext at Kernel->Add[%u] cannot be injected by OpenCore, please remove it!\n", Index));
            ++ErrorCount;
          }
        } else {
          DEBUG ((
            DEBUG_WARN,
            "Kernel->Add[%u] discovers %a, but its ExecutablePath (%a) or PlistPath (%a) is borked!\n",
            IndexKextInfo,
            BundlePath,
            ExecutablePath,
            PlistPath
            ));
          ++ErrorCount;
        }
      }
    }
  }

  //
  // Special checks for kext precedence from Acidanthera.
  //
  for (IndexKextPrecedence = 0; IndexKextPrecedence < mKextPrecedenceSize; ++IndexKextPrecedence) {
    HasParent = FALSE;

    for (Index = 0; Index < Config->Kernel.Add.Count; ++Index) {
      CurrentKext = OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->BundlePath);
      ParentKext  = mKextPrecedence[IndexKextPrecedence].Parent;
      ChildKext   = mKextPrecedence[IndexKextPrecedence].Child;

      if (AsciiStrCmp (CurrentKext, ParentKext) == 0) {
        HasParent = TRUE;
      } else if (AsciiStrCmp (CurrentKext, ChildKext) == 0) {
        if (!HasParent) {
          DEBUG ((DEBUG_WARN, "Kernel->Add[%u] discovers %a, but its Parent (%a) is either placed after it or is missing!\n", Index, CurrentKext, ParentKext));
          ++ErrorCount;
        }

        //
        // Parent is already found before Child as guaranteed by the first if. Done.
        //
        break;
      }
    }
  }

  //
  // Check duplicated entries in Kernel->Add.
  //
  ErrorCount += FindArrayDuplication (
                  Config->Kernel.Add.Values,
                  Config->Kernel.Add.Count,
                  sizeof (Config->Kernel.Add.Values[0]),
                  KernelAddHasDuplication
                  );

  return ErrorCount;
}

STATIC
UINT32
CheckKernelBlock (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  UINT32       Index;
  CONST CHAR8  *Arch;
  CONST CHAR8  *Comment;
  CONST CHAR8  *MaxKernel;
  CONST CHAR8  *MinKernel;
  CONST CHAR8  *Identifier;
  CONST CHAR8  *Strategy;

  ErrorCount = 0;

  for (Index = 0; Index < Config->Kernel.Block.Count; ++Index) {
    Arch       = OC_BLOB_GET (&Config->Kernel.Block.Values[Index]->Arch);
    Comment    = OC_BLOB_GET (&Config->Kernel.Block.Values[Index]->Comment);
    Identifier = OC_BLOB_GET (&Config->Kernel.Block.Values[Index]->Identifier);
    MaxKernel  = OC_BLOB_GET (&Config->Kernel.Block.Values[Index]->MaxKernel);
    MinKernel  = OC_BLOB_GET (&Config->Kernel.Block.Values[Index]->MinKernel);
    Strategy   = OC_BLOB_GET (&Config->Kernel.Block.Values[Index]->Strategy);

    //
    // Sanitise strings.
    //
    if (!AsciiArchIsLegal (Arch, FALSE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->Arch is borked (Can only be Any, i386, and x86_64)!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiIdentifierIsLegal (Identifier, TRUE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->Identifier contains illegal character!\n", Index));
      ++ErrorCount;
    }

    //
    // MinKernel must not be below macOS 10.4 (Darwin version 8).
    //
    if (!OcMatchDarwinVersion (OcParseDarwinVersion (MinKernel), KERNEL_VERSION_TIGER_MIN, 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->MinKernel has a Darwin version %a, which is below 8 (macOS 10.4)!\n", Index, MinKernel));
      ++ErrorCount;
    }

    //
    // FIXME: Handle correct kernel version checking.
    //
    if ((MaxKernel[0] != '\0') && (OcParseDarwinVersion (MaxKernel) == 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->MaxKernel (currently set to %a) is borked!\n", Index, MaxKernel));
      ++ErrorCount;
    }

    if ((MinKernel[0] != '\0') && (OcParseDarwinVersion (MinKernel) == 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->MinKernel (currently set to %a) is borked!\n", Index, MinKernel));
      ++ErrorCount;
    }

    if (  (AsciiStrCmp (Strategy, "Disable") != 0)
       && (AsciiStrCmp (Strategy, "Exclude") != 0))
    {
      DEBUG ((DEBUG_WARN, "Kernel->Block[%u]->Strategy is borked (Can only be Disable or Exclude)!\n", Index));
      ++ErrorCount;
    }
  }

  //
  // Check duplicated entries in Kernel->Block.
  //
  ErrorCount += FindArrayDuplication (
                  Config->Kernel.Block.Values,
                  Config->Kernel.Block.Count,
                  sizeof (Config->Kernel.Block.Values[0]),
                  KernelBlockHasDuplication
                  );

  return ErrorCount;
}

STATIC
UINT32
CheckKernelEmulate (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  CONST CHAR8  *MaxKernel;
  CONST CHAR8  *MinKernel;
  BOOLEAN      Result;

  ErrorCount = 0;

  MaxKernel = OC_BLOB_GET (&Config->Kernel.Emulate.MaxKernel);
  MinKernel = OC_BLOB_GET (&Config->Kernel.Emulate.MinKernel);

  //
  // MinKernel must not be below macOS 10.4 (Darwin version 8).
  //
  if (!OcMatchDarwinVersion (OcParseDarwinVersion (MinKernel), KERNEL_VERSION_TIGER_MIN, 0)) {
    DEBUG ((DEBUG_WARN, "Kernel->Emulate->MinKernel has a Darwin version %a, which is below 8 (macOS 10.4)!\n", MinKernel));
    ++ErrorCount;
  }

  //
  // FIXME: Handle correct kernel version checking.
  //
  if ((MaxKernel[0] != '\0') && (OcParseDarwinVersion (MaxKernel) == 0)) {
    DEBUG ((DEBUG_WARN, "Kernel->Emulate->MaxKernel (currently set to %a) is borked!\n", MaxKernel));
    ++ErrorCount;
  }

  if ((MinKernel[0] != '\0') && (OcParseDarwinVersion (MinKernel) == 0)) {
    DEBUG ((DEBUG_WARN, "Kernel->Emulate->MinKernel (currently set to %a) is borked!\n", MinKernel));
    ++ErrorCount;
  }

  Result = DataHasProperMasking (
             Config->Kernel.Emulate.Cpuid1Data,
             Config->Kernel.Emulate.Cpuid1Mask,
             sizeof (Config->Kernel.Emulate.Cpuid1Data),
             sizeof (Config->Kernel.Emulate.Cpuid1Mask)
             );

  if (!Result) {
    DEBUG ((DEBUG_WARN, "Kernel->Emulate->Cpuid1Data requires Cpuid1Mask to be active for replaced bits!\n"));
    ++ErrorCount;
  }

  return ErrorCount;
}

STATIC
UINT32
CheckKernelForce (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  UINT32       Index;
  CONST CHAR8  *Arch;
  CONST CHAR8  *BundlePath;
  CONST CHAR8  *Comment;
  CONST CHAR8  *ExecutablePath;
  CONST CHAR8  *Identifier;
  CONST CHAR8  *MaxKernel;
  CONST CHAR8  *MinKernel;
  CONST CHAR8  *PlistPath;

  ErrorCount = 0;

  for (Index = 0; Index < Config->Kernel.Force.Count; ++Index) {
    Arch           = OC_BLOB_GET (&Config->Kernel.Force.Values[Index]->Arch);
    BundlePath     = OC_BLOB_GET (&Config->Kernel.Force.Values[Index]->BundlePath);
    Comment        = OC_BLOB_GET (&Config->Kernel.Force.Values[Index]->Comment);
    ExecutablePath = OC_BLOB_GET (&Config->Kernel.Force.Values[Index]->ExecutablePath);
    Identifier     = OC_BLOB_GET (&Config->Kernel.Force.Values[Index]->Identifier);
    MaxKernel      = OC_BLOB_GET (&Config->Kernel.Force.Values[Index]->MaxKernel);
    MinKernel      = OC_BLOB_GET (&Config->Kernel.Force.Values[Index]->MinKernel);
    PlistPath      = OC_BLOB_GET (&Config->Kernel.Force.Values[Index]->PlistPath);

    //
    // Sanitise strings.
    //
    if (!AsciiArchIsLegal (Arch, FALSE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->Arch is borked (Can only be Any, i386, and x86_64)!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiIdentifierIsLegal (Identifier, TRUE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->Identifier contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiFileSystemPathIsLegal (BundlePath)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->BundlePath contains illegal character!\n", Index));
      ++ErrorCount;
      continue;
    }

    //
    // Valid BundlePath must contain .kext suffix.
    //
    if (!OcAsciiEndsWith (BundlePath, ".kext", TRUE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->BundlePath does NOT contain .kext suffix!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiFileSystemPathIsLegal (ExecutablePath)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->ExecutablePath contains illegal character!\n", Index));
      ++ErrorCount;
      continue;
    }

    if (!AsciiFileSystemPathIsLegal (PlistPath)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->PlistPath contains illegal character!\n", Index));
      ++ErrorCount;
      continue;
    }

    //
    // Valid PlistPath must contain .plist suffix.
    //
    if (!OcAsciiEndsWith (PlistPath, ".plist", TRUE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->PlistPath does NOT contain .plist suffix!\n", Index));
      ++ErrorCount;
    }

    //
    // MinKernel must not be below macOS 10.4 (Darwin version 8).
    //
    if (!OcMatchDarwinVersion (OcParseDarwinVersion (MinKernel), KERNEL_VERSION_TIGER_MIN, 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->MinKernel has a Darwin version %a, which is below 8 (macOS 10.4)!\n", Index, MinKernel));
      ++ErrorCount;
    }

    //
    // FIXME: Handle correct kernel version checking.
    //
    if ((MaxKernel[0] != '\0') && (OcParseDarwinVersion (MaxKernel) == 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->MaxKernel (currently set to %a) is borked!\n", Index, MaxKernel));
      ++ErrorCount;
    }

    if ((MinKernel[0] != '\0') && (OcParseDarwinVersion (MinKernel) == 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->MinKernel (currently set to %a) is borked!\n", Index, MinKernel));
      ++ErrorCount;
    }
  }

  //
  // Check duplicated entries in Kernel->Force.
  //
  ErrorCount += FindArrayDuplication (
                  Config->Kernel.Force.Values,
                  Config->Kernel.Force.Count,
                  sizeof (Config->Kernel.Force.Values[0]),
                  KernelForceHasDuplication
                  );

  return ErrorCount;
}

STATIC
UINT32
CheckKernelPatch (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  UINT32       Index;
  CONST CHAR8  *Arch;
  CONST CHAR8  *Comment;
  CONST CHAR8  *MaxKernel;
  CONST CHAR8  *MinKernel;
  CONST CHAR8  *Identifier;
  CONST CHAR8  *Base;
  CONST UINT8  *Find;
  UINT32       FindSize;
  CONST UINT8  *Replace;
  UINT32       ReplaceSize;
  CONST UINT8  *Mask;
  UINT32       MaskSize;
  CONST UINT8  *ReplaceMask;
  UINT32       ReplaceMaskSize;

  ErrorCount = 0;

  for (Index = 0; Index < Config->Kernel.Patch.Count; ++Index) {
    Base            = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->Base);
    Comment         = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->Comment);
    Arch            = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->Arch);
    Identifier      = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->Identifier);
    Find            = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->Find);
    FindSize        = Config->Kernel.Patch.Values[Index]->Find.Size;
    Replace         = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->Replace);
    ReplaceSize     = Config->Kernel.Patch.Values[Index]->Replace.Size;
    Mask            = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->Mask);
    MaskSize        = Config->Kernel.Patch.Values[Index]->Mask.Size;
    ReplaceMask     = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->ReplaceMask);
    ReplaceMaskSize = Config->Kernel.Patch.Values[Index]->ReplaceMask.Size;
    MaxKernel       = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->MaxKernel);
    MinKernel       = OC_BLOB_GET (&Config->Kernel.Patch.Values[Index]->MinKernel);

    //
    // Sanitise strings.
    //
    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiArchIsLegal (Arch, FALSE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->Arch is borked (Can only be Any, i386, and x86_64)!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiIdentifierIsLegal (Identifier, TRUE)) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->Identifier contains illegal character!\n", Index));
      ++ErrorCount;
    }

    //
    // MinKernel must not be below macOS 10.4 (Darwin version 8).
    //
    if (!OcMatchDarwinVersion (OcParseDarwinVersion (MinKernel), KERNEL_VERSION_TIGER_MIN, 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->MinKernel has a Darwin version %a, which is below 8 (macOS 10.4)!\n", Index, MinKernel));
      ++ErrorCount;
    }

    //
    // FIXME: Handle correct kernel version checking.
    //
    if ((MaxKernel[0] != '\0') && (OcParseDarwinVersion (MaxKernel) == 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->MaxKernel (currently set to %a) is borked!\n", Index, MaxKernel));
      ++ErrorCount;
    }

    if ((MinKernel[0] != '\0') && (OcParseDarwinVersion (MinKernel) == 0)) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->MinKernel (currently set to %a) is borked!\n", Index, MinKernel));
      ++ErrorCount;
    }

    //
    // Checks for size.
    //
    ErrorCount += ValidatePatch (
                    "Kernel->Patch",
                    Index,
                    Base[0] != '\0' && FindSize == 0,
                    Find,
                    FindSize,
                    Replace,
                    ReplaceSize,
                    Mask,
                    MaskSize,
                    ReplaceMask,
                    ReplaceMaskSize
                    );
  }

  return ErrorCount;
}

STATIC
UINT32
CheckKernelQuirks (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  BOOLEAN      IsCustomSMBIOSGuidEnabled;
  CONST CHAR8  *UpdateSMBIOSMode;
  INT64        SetApfsTrimTimeout;

  ErrorCount = 0;

  //
  // CustomSMBIOSGuid quirk requires UpdateSMBIOSMode at PlatformInfo set to Custom.
  //
  IsCustomSMBIOSGuidEnabled = Config->Kernel.Quirks.CustomSmbiosGuid;
  UpdateSMBIOSMode          = OC_BLOB_GET (&Config->PlatformInfo.UpdateSmbiosMode);
  if (IsCustomSMBIOSGuidEnabled && (AsciiStrCmp (UpdateSMBIOSMode, "Custom") != 0)) {
    DEBUG ((DEBUG_WARN, "Kernel->Quirks->CustomSMBIOSGuid is enabled, but PlatformInfo->UpdateSMBIOSMode is not set to Custom!\n"));
    ++ErrorCount;
  }

  SetApfsTrimTimeout = Config->Kernel.Quirks.SetApfsTrimTimeout;
  if (  (SetApfsTrimTimeout  > MAX_UINT32)
     || (SetApfsTrimTimeout < -1))
  {
    DEBUG ((DEBUG_WARN, "Kernel->Quirks->SetApfsTrimTimeout is invalid value %d!\n", SetApfsTrimTimeout));
    ++ErrorCount;
  }

  return ErrorCount;
}

STATIC
UINT32
CheckKernelScheme (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  CONST CHAR8  *Arch;
  CONST CHAR8  *KernelCache;

  ErrorCount = 0;

  //
  // Sanitise Kernel->Scheme keys.
  //
  Arch = OC_BLOB_GET (&Config->Kernel.Scheme.KernelArch);
  if (!AsciiArchIsLegal (Arch, TRUE)) {
    DEBUG ((DEBUG_WARN, "Kernel->Scheme->KernelArch is borked (Can only be Auto, i386, i386-user32, or x86_64)!\n"));
    ++ErrorCount;
  }

  KernelCache = OC_BLOB_GET (&Config->Kernel.Scheme.KernelCache);
  if (  (AsciiStrCmp (KernelCache, "Auto") != 0)
     && (AsciiStrCmp (KernelCache, "Cacheless") != 0)
     && (AsciiStrCmp (KernelCache, "Mkext") != 0)
     && (AsciiStrCmp (KernelCache, "Prelinked") != 0))
  {
    DEBUG ((DEBUG_WARN, "Kernel->Scheme->KernelCache is borked (Can only be Auto, Cacheless, Mkext, or Prelinked)!\n"));
    ++ErrorCount;
  }

  return ErrorCount;
}

UINT32
CheckKernel (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32               ErrorCount;
  UINTN                Index;
  STATIC CONFIG_CHECK  KernelCheckers[] = {
    &CheckKernelAdd,
    &CheckKernelBlock,
    &CheckKernelEmulate,
    &CheckKernelForce,
    &CheckKernelPatch,
    &CheckKernelQuirks,
    &CheckKernelScheme
  };

  DEBUG ((DEBUG_VERBOSE, "config loaded into %a!\n", __func__));

  ErrorCount = 0;

  //
  // Ensure correct kext info prior to verification.
  //
  ValidateKextInfo ();

  for (Index = 0; Index < ARRAY_SIZE (KernelCheckers); ++Index) {
    ErrorCount += KernelCheckers[Index](Config);
  }

  return ReportError (__func__, ErrorCount);
}
