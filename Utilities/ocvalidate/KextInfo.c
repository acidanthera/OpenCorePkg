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

#include "KextInfo.h"

#include <Library/DebugLib.h>

KEXT_PRECEDENCE mKextPrecedence[] = {
  { "VirtualSMC.kext",                                                  "Lilu.kext" },
  { "WhateverGreen.kext",                                               "Lilu.kext" },
  { "SMCBatteryManager.kext",                                           "VirtualSMC.kext" },
  { "SMCDellSensors.kext",                                              "VirtualSMC.kext" },
  { "SMCLightSensor.kext",                                              "VirtualSMC.kext" },
  { "SMCProcessor.kext",                                                "VirtualSMC.kext" },
  { "SMCSuperIO.kext",                                                  "VirtualSMC.kext" },
  { "AppleALC.kext",                                                    "Lilu.kext" },
  { "AppleALCU.kext",                                                   "Lilu.kext" },
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
  { "SidecarFixup.kext",                                                "Lilu.kext" },
  { "MacHyperVSupport.kext",                                            "Lilu.kext" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext", "VoodooPS2Controller.kext" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Mouse.kext",    "VoodooPS2Controller.kext" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Trackpad.kext", "VoodooPS2Controller.kext" },
};
UINTN mKextPrecedenceSize = ARRAY_SIZE (mKextPrecedence);

KEXT_INFO mKextInfo[] = {
  //
  // NOTE: Index of Lilu should always be 0. Please add entries after this if necessary.
  //
  { "Lilu.kext",                                                            "Contents/MacOS/Lilu",                "Contents/Info.plist" },
  //
  // NOTE: Index of VirtualSMC should always be 1. Please add entries after this if necessary.
  //
  { "VirtualSMC.kext",                                                      "Contents/MacOS/VirtualSMC",          "Contents/Info.plist" },
  { "WhateverGreen.kext",                                                   "Contents/MacOS/WhateverGreen",       "Contents/Info.plist" },
  { "SMCBatteryManager.kext",                                               "Contents/MacOS/SMCBatteryManager",   "Contents/Info.plist" },
  { "SMCDellSensors.kext",                                                  "Contents/MacOS/SMCDellSensors",      "Contents/Info.plist" },
  { "SMCLightSensor.kext",                                                  "Contents/MacOS/SMCLightSensor",      "Contents/Info.plist" },
  { "SMCProcessor.kext",                                                    "Contents/MacOS/SMCProcessor",        "Contents/Info.plist" },
  { "SMCSuperIO.kext",                                                      "Contents/MacOS/SMCSuperIO",          "Contents/Info.plist" },
  { "AppleALC.kext",                                                        "Contents/MacOS/AppleALC",            "Contents/Info.plist" },
  { "AppleALCU.kext",                                                       "Contents/MacOS/AppleALCU",           "Contents/Info.plist" },
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
  { "SidecarFixup.kext",                                                    "Contents/MacOS/SidecarFixup",        "Contents/Info.plist" },
  { "MacHyperVSupport.kext",                                                "Contents/MacOS/MacHyperVSupport",    "Contents/Info.plist" },
  { "IntelMausi.kext",                                                      "Contents/MacOS/IntelMausi",          "Contents/Info.plist" },
  { "IntelSnowMausi.kext",                                                  "Contents/MacOS/IntelSnowMausi",      "Contents/Info.plist" },
  { "BrcmBluetoothInjector.kext",                                           "",                                   "Contents/Info.plist" },
  { "BrcmBluetoothInjectorLegacy.kext",                                     "",                                   "Contents/Info.plist" },
  { "BrcmFirmwareData.kext",                                                "Contents/MacOS/BrcmFirmwareData",    "Contents/Info.plist" },
  { "BrcmFirmwareRepo.kext",                                                "Contents/MacOS/BrcmFirmwareRepo",    "Contents/Info.plist" },
  { "BrcmNonPatchRAM.kext",                                                 "Contents/MacOS/BrcmNonPatchRAM",     "Contents/Info.plist" },
  { "BrcmNonPatchRAM2.kext",                                                "Contents/MacOS/BrcmNonPatchRAM2",    "Contents/Info.plist" },
  { "BrcmPatchRAM.kext",                                                    "Contents/MacOS/BrcmPatchRAM",        "Contents/Info.plist" },
  { "BrcmPatchRAM2.kext",                                                   "Contents/MacOS/BrcmPatchRAM2",       "Contents/Info.plist" },
  { "BrcmPatchRAM3.kext",                                                   "Contents/MacOS/BrcmPatchRAM3",       "Contents/Info.plist" },
  { "Legacy_USB3.kext",                                                     "",                                   "Contents/Info.plist" },
  { "Legacy_InternalHub-EHCx.kext",                                         "",                                   "Contents/Info.plist" },
  { "VoodooPS2Controller.kext",                                             "Contents/MacOS/VoodooPS2Controller", "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext",     "Contents/MacOS/VoodooPS2Keyboard",   "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Mouse.kext",        "Contents/MacOS/VoodooPS2Mouse",      "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Trackpad.kext",     "Contents/MacOS/VoodooPS2Trackpad",   "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooInput.kext",           "Contents/MacOS/VoodooInput",         "Contents/Info.plist" },
};
UINTN mKextInfoSize = ARRAY_SIZE (mKextInfo);

VOID
ValidateKextInfo (
  VOID
  )
{
  //
  // Ensure Lilu and VirtualSMC to be always placed where it is supposed to be.
  // 
  ASSERT (AsciiStrCmp (mKextInfo[INDEX_KEXT_LILU].KextBundlePath, "Lilu.kext") == 0);
  ASSERT (AsciiStrCmp (mKextInfo[INDEX_KEXT_LILU].KextExecutablePath, "Contents/MacOS/Lilu") == 0);
  ASSERT (AsciiStrCmp (mKextInfo[INDEX_KEXT_LILU].KextPlistPath, "Contents/Info.plist") == 0);

  ASSERT (AsciiStrCmp (mKextInfo[INDEX_KEXT_VSMC].KextBundlePath, "VirtualSMC.kext") == 0);
  ASSERT (AsciiStrCmp (mKextInfo[INDEX_KEXT_VSMC].KextExecutablePath, "Contents/MacOS/VirtualSMC") == 0);
  ASSERT (AsciiStrCmp (mKextInfo[INDEX_KEXT_VSMC].KextPlistPath, "Contents/Info.plist") == 0);
}
