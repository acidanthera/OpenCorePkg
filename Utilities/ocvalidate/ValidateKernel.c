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

#include <Library/OcAppleKernelLib.h>

#define INDEX_KEXT_LILU  0U

typedef struct KEXT_PRECEDENCE_ {
  CONST CHAR8  *Child;
  CONST CHAR8  *Parent;
} KEXT_PRECEDENCE;

typedef struct KEXT_INFO_ {
  CONST CHAR8  *KextBundlePath;
  CONST CHAR8  *KextExecutablePath;
  CONST CHAR8  *KextPlistPath;
} KEXT_INFO;

STATIC KEXT_PRECEDENCE mKextPrecedence[] = {
  { "VirtualSMC.kext",                                                  "Lilu.kext" },
  { "WhateverGreen.kext",                                               "Lilu.kext" },
  { "SMCBatteryManager.kext",                                           "VirtualSMC.kext" },
  { "SMCDellSensors.kext",                                              "VirtualSMC.kext" },
  { "SMCLightSensor.kext",                                              "VirtualSMC.kext" },
  { "SMCProcessor.kext",                                                "VirtualSMC.kext" },
  { "SMCSuperIO.kext",                                                  "VirtualSMC.kext" },
  { "AppleALC.kext",                                                    "Lilu.kext" },
  { "AirportBrcmFixup.kext",                                            "Lilu.kext" },
  { "BrightnessKeys.kext",                                              "Lilu.kext" },
  { "CpuTscSync.kext",                                                  "Lilu.kext" },
  { "CPUFriend.kext",                                                   "Lilu.kext" },
  { "CPUFriendDataProvider.kext",                                       "CPUFriend.kext" },
  { "DebugEnhancer.kext",                                               "Lilu.kext" },
  { "HibernationFixup.kext",                                            "Lilu.kext" },
  { "NVMeFix.kext",                                                     "Lilu.kext" },
  { "RestrictEvents.kext",                                              "Lilu.kext" },
  { "RTCMemoryFixup.kext",                                              "Lilu.kext" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext", "VoodooPS2Controller.kext" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Mouse.kext",    "VoodooPS2Controller.kext" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Trackpad.kext", "VoodooPS2Controller.kext" },
};
STATIC UINTN mKextPrecedenceSize = ARRAY_SIZE (mKextPrecedence);

STATIC KEXT_INFO mKextInfo[] = {
  //
  // NOTE: Index of Lilu should always be 0. Please add entries after this if necessary.
  //
  { "Lilu.kext",                                                            "Contents/MacOS/Lilu",                "Contents/Info.plist" },
  { "VirtualSMC.kext",                                                      "Contents/MacOS/VirtualSMC",          "Contents/Info.plist" },
  { "WhateverGreen.kext",                                                   "Contents/MacOS/WhateverGreen",       "Contents/Info.plist" },
  { "SMCBatteryManager.kext",                                               "Contents/MacOS/SMCBatteryManager",   "Contents/Info.plist" },
  { "SMCDellSensors.kext",                                                  "Contents/MacOS/SMCDellSensors",      "Contents/Info.plist" },
  { "SMCLightSensor.kext",                                                  "Contents/MacOS/SMCLightSensor",      "Contents/Info.plist" },
  { "SMCProcessor.kext",                                                    "Contents/MacOS/SMCProcessor",        "Contents/Info.plist" },
  { "SMCSuperIO.kext",                                                      "Contents/MacOS/SMCSuperIO",          "Contents/Info.plist" },
  { "AppleALC.kext",                                                        "Contents/MacOS/AppleALC",            "Contents/Info.plist" },
  { "AirportBrcmFixup.kext",                                                "Contents/MacOS/AirportBrcmFixup",    "Contents/Info.plist" },
  { "AirportBrcmFixup.kext/Contents/PlugIns/AirPortBrcm4360_Injector.kext", "",                                   "Contents/Info.plist" },
  { "AirportBrcmFixup.kext/Contents/PlugIns/AirPortBrcmNIC_Injector.kext",  "",                                   "Contents/Info.plist" },
  { "BrightnessKeys.kext",                                                  "Contents/MacOS/BrightnessKeys",      "Contents/Info.plist" },
  { "CpuTscSync.kext",                                                      "Contents/MacOS/CpuTscSync",          "Contents/Info.plist" },
  { "CPUFriend.kext",                                                       "Contents/MacOS/CPUFriend",           "Contents/Info.plist" },
  { "CPUFriendDataProvider.kext",                                           "",                                   "Contents/Info.plist" },
  { "DebugEnhancer.kext",                                                   "Contents/MacOS/DebugEnhancer",       "Contents/Info.plist" },
  { "HibernationFixup.kext",                                                "Contents/MacOS/HibernationFixup",    "Contents/Info.plist" },
  { "NVMeFix.kext",                                                         "Contents/MacOS/NVMeFix",             "Contents/Info.plist" },
  { "RestrictEvents.kext",                                                  "Contents/MacOS/RestrictEvents",      "Contents/Info.plist" },
  { "RTCMemoryFixup.kext",                                                  "Contents/MacOS/RTCMemoryFixup",      "Contents/Info.plist" },
  { "IntelMausi.kext",                                                      "Contents/MacOS/IntelMausi",          "Contents/Info.plist" },
  { "IntelSnowMausi.kext",                                                  "Contents/MacOS/IntelSnowMausi",      "Contents/Info.plist" },
  { "BrcmBluetoothInjector.kext",                                           "",                                   "Contents/Info.plist" },
  { "BrcmFirmwareData.kext",                                                "Contents/MacOS/BrcmFirmwareData",    "Contents/Info.plist" },
  { "BrcmFirmwareRepo.kext",                                                "Contents/MacOS/BrcmFirmwareRepo",    "Contents/Info.plist" },
  { "BrcmNonPatchRAM.kext",                                                 "Contents/MacOS/BrcmNonPatchRAM",     "Contents/Info.plist" },
  { "BrcmNonPatchRAM2.kext",                                                "Contents/MacOS/BrcmNonPatchRAM2",    "Contents/Info.plist" },
  { "BrcmPatchRAM.kext",                                                    "Contents/MacOS/BrcmPatchRAM",        "Contents/Info.plist" },
  { "BrcmPatchRAM2.kext",                                                   "Contents/MacOS/BrcmPatchRAM2",       "Contents/Info.plist" },
  { "BrcmPatchRAM3.kext",                                                   "Contents/MacOS/BrcmPatchRAM3",       "Contents/Info.plist" },
  { "BrcmPatchRAM3.kext",                                                   "Contents/MacOS/BrcmPatchRAM3",       "Contents/Info.plist" },
  { "Legacy_USB3.kext",                                                     "",                                   "Contents/Info.plist" },
  { "Legacy_InternalHub-EHCx.kext",                                         "",                                   "Contents/Info.plist" },
  { "VoodooPS2Controller.kext",                                             "Contents/MacOS/VoodooPS2Controller", "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext",     "Contents/MacOS/VoodooPS2Keyboard",   "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Mouse.kext",        "Contents/MacOS/VoodooPS2Mouse",      "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Trackpad.kext",     "Contents/MacOS/VoodooPS2Trackpad",   "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooInput.kext",           "Contents/MacOS/VoodooInput",         "Contents/Info.plist" },
};
STATIC UINTN mKextInfoSize = ARRAY_SIZE (mKextInfo);

UINT32
CheckKernel (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32              ErrorCount;
  UINT32              Index;
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
  BOOLEAN             IsDisableLinkeditJettisonEnabled;
  BOOLEAN             IsCustomSMBIOSGuidEnabled;
  CONST CHAR8         *UpdateSMBIOSMode;
  CONST CHAR8         *Base;
  CONST UINT8         *Find;
  UINT32              FindSize;
  CONST UINT8         *Replace;
  UINT32              ReplaceSize;
  CONST UINT8         *Mask;
  UINT32              MaskSize;
  CONST UINT8         *ReplaceMask;
  UINT32              ReplaceMaskSize;
  CONST CHAR8         *KernelCache;
  UINTN               IndexKextInfo;
  UINTN               IndexKextPrecedence;
  BOOLEAN             HasParent;
  CONST CHAR8         *CurrentKext;
  CONST CHAR8         *ParentKext;
  CONST CHAR8         *ChildKext;

  DEBUG ((DEBUG_VERBOSE, "config loaded into Kernel checker!\n"));
  //
  // Ensure Lilu to be always placed where it is supposed to be.
  // 
  ASSERT (AsciiStrCmp (mKextInfo[INDEX_KEXT_LILU].KextBundlePath, "Lilu.kext") == 0);
  ASSERT (AsciiStrCmp (mKextInfo[INDEX_KEXT_LILU].KextExecutablePath, "Contents/MacOS/Lilu") == 0);
  ASSERT (AsciiStrCmp (mKextInfo[INDEX_KEXT_LILU].KextPlistPath, "Contents/Info.plist") == 0);

  ErrorCount                       = 0;
  UserKernel                       = &Config->Kernel;
  UserPlatformInfo                 = &Config->PlatformInfo;
  IsDisableLinkeditJettisonEnabled = UserKernel->Quirks.DisableLinkeditJettison;
  IsCustomSMBIOSGuidEnabled        = UserKernel->Quirks.CustomSmbiosGuid;
  UpdateSMBIOSMode                 = OC_BLOB_GET (&UserPlatformInfo->UpdateSmbiosMode);
  KernelCache                      = OC_BLOB_GET (&UserKernel->Scheme.KernelCache);

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

    for (IndexKextInfo = 0; IndexKextInfo < mKextInfoSize; ++IndexKextInfo) {
      if (AsciiStrCmp (BundlePath, mKextInfo[IndexKextInfo].KextBundlePath) == 0) {
        //
        // BundlePath matched. Continue checking ExecutablePath and PlistPath.
        //
        if (AsciiStrCmp (ExecutablePath, mKextInfo[IndexKextInfo].KextExecutablePath) == 0
          && AsciiStrCmp (PlistPath, mKextInfo[IndexKextInfo].KextPlistPath) == 0) {
          //
          // Special check for Lilu and Quirks->DisableLinkeditJettison.
          //
          if (IndexKextInfo == INDEX_KEXT_LILU) {
            if (!IsDisableLinkeditJettisonEnabled) {
              DEBUG ((DEBUG_WARN, "Lilu.kext is loaded at Kernel->Add[%u], but DisableLinkeditJettison is not enabled at Kernel->Quirks!\n", Index));
              ++ErrorCount;
            }
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

    for (Index = 0; Index < UserKernel->Add.Count; ++Index) {
      CurrentKext = OC_BLOB_GET (&UserKernel->Add.Values[Index]->BundlePath);
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
        // Parent is already found before Child. Done.
        //
        break;
      }
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

  for (Index = 0; Index < UserKernel->Force.Count; ++Index) {
    Arch            = OC_BLOB_GET (&UserKernel->Force.Values[Index]->Arch);
    BundlePath      = OC_BLOB_GET (&UserKernel->Force.Values[Index]->BundlePath);
    Comment         = OC_BLOB_GET (&UserKernel->Force.Values[Index]->Comment);
    ExecutablePath  = OC_BLOB_GET (&UserKernel->Force.Values[Index]->ExecutablePath);
    Identifier      = OC_BLOB_GET (&UserKernel->Force.Values[Index]->Identifier);
    MaxKernel       = OC_BLOB_GET (&UserKernel->Force.Values[Index]->MaxKernel);
    MinKernel       = OC_BLOB_GET (&UserKernel->Force.Values[Index]->MinKernel);
    PlistPath       = OC_BLOB_GET (&UserKernel->Force.Values[Index]->PlistPath);

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
    // FIXME: Handle correct kernel version checking.
    //
    if (MaxKernel[0] != '\0' && OcParseDarwinVersion (MaxKernel) == 0) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->MaxKernel (currently set to %a) is borked!\n", Index, MaxKernel));
      ++ErrorCount;
    }
    if (MinKernel[0] != '\0' && OcParseDarwinVersion (MinKernel) == 0) {
      DEBUG ((DEBUG_WARN, "Kernel->Force[%u]->MinKernel (currently set to %a) is borked!\n", Index, MinKernel));
      ++ErrorCount;
    }

  }

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
    MaxKernel       = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->MaxKernel);
    MinKernel       = OC_BLOB_GET (&UserKernel->Patch.Values[Index]->MinKernel);

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
    // FIXME: Handle correct kernel version checking.
    //
    if (MaxKernel[0] != '\0' && OcParseDarwinVersion (MaxKernel) == 0) {
      DEBUG ((DEBUG_WARN, "Kernel->Patch[%u]->MaxKernel (currently set to %a) is borked!\n", Index, MaxKernel));
      ++ErrorCount;
    }
    if (MinKernel[0] != '\0' && OcParseDarwinVersion (MinKernel) == 0) {
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

  //
  // Sanitise Kernel->Scheme keys.
  //
  Arch = OC_BLOB_GET (&UserKernel->Scheme.KernelArch);
  if (!AsciiArchIsLegal (Arch, TRUE)) {
    DEBUG ((DEBUG_WARN, "Kernel->Scheme->KernelArch is borked (Can only be Auto, i386, i386-user32, or x86_64)!\n"));
    ++ErrorCount;
  }

  if (AsciiStrCmp (KernelCache, "Auto") != 0
    && AsciiStrCmp (KernelCache, "Cacheless") != 0
    && AsciiStrCmp (KernelCache, "Mkext") != 0
    && AsciiStrCmp (KernelCache, "Prelinked") != 0) {
    DEBUG ((DEBUG_WARN, "Kernel->Scheme->KernelCache is borked (Can only be Auto, Cacheless, Mkext, or Prelinked)!\n"));
    ++ErrorCount;
  }

  //
  // CustomSMBIOSGuid quirk requires UpdateSMBIOSMode at PlatformInfo set to Custom.
  //
  if (IsCustomSMBIOSGuidEnabled && AsciiStrCmp (UpdateSMBIOSMode, "Custom") != 0) {
    DEBUG ((DEBUG_WARN, "Kernel->Quirks->CustomSMBIOSGuid is enabled, but PlatformInfo->UpdateSMBIOSMode is not set to Custom!\n"));
    ++ErrorCount;
  }

  return ReportError (__func__, ErrorCount);
}
