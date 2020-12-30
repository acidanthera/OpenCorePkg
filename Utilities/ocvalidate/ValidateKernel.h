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

#ifndef OC_USER_UTILITIES_OCVALIDATE_VALIDATE_KERNEL_H
#define OC_USER_UTILITIES_OCVALIDATE_VALIDATE_KERNEL_H

#define INDEX_KEXT_LILU  0U

//
// Child kext must be put after Parent kext in OpenCore config->Kernel->Add.
// This means that the index of Child kext must be greater than that of Parent kext.
//
typedef struct KEXT_PRECEDENCE_ {
  CONST CHAR8  *Child;
  CONST CHAR8  *Parent;
} KEXT_PRECEDENCE;

//
// Sanitiser for known kext info. Mainly those from Acidanthera.
//
typedef struct KEXT_INFO_ {
  CONST CHAR8  *KextBundlePath;
  CONST CHAR8  *KextExecutablePath;
  CONST CHAR8  *KextPlistPath;
} KEXT_INFO;

KEXT_PRECEDENCE mKextPrecedence[] = {
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
UINTN mKextPrecedenceSize = ARRAY_SIZE (mKextPrecedence);

KEXT_INFO mKextInfo[] = {
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
  { "Legacy_USB3.kext",                                                     "",                                   "Contents/Info.plist" },
  { "Legacy_InternalHub-EHCx.kext",                                         "",                                   "Contents/Info.plist" },
  { "VoodooPS2Controller.kext",                                             "Contents/MacOS/VoodooPS2Controller", "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Keyboard.kext",     "Contents/MacOS/VoodooPS2Keyboard",   "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Mouse.kext",        "Contents/MacOS/VoodooPS2Mouse",      "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooPS2Trackpad.kext",     "Contents/MacOS/VoodooPS2Trackpad",   "Contents/Info.plist" },
  { "VoodooPS2Controller.kext/Contents/PlugIns/VoodooInput.kext",           "Contents/MacOS/VoodooInput",         "Contents/Info.plist" },
};
UINTN mKextInfoSize = ARRAY_SIZE (mKextInfo);

#endif // OC_USER_UTILITIES_OCVALIDATE_VALIDATE_KERNEL_H
