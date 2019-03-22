/** @file
  Commonly used kext patches.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcProtocolLib.h>
#include <Library/OcAppleBootPolicyLib.h>
#include <Library/OcSmbiosLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVirtualFsLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcXmlLib.h>

#include <Protocol/AppleBootPolicy.h>
#include <Protocol/DevicePathPropertyDatabase.h>


#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleFileSystem.h>


STATIC
UINT8
AppleIntelCPUPowerManagementPatchFind[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00,     // mov ecx, 0xe2
  0x0F, 0x30                        // wrmsr
};

STATIC
UINT8
AppleIntelCPUPowerManagementPatchReplace[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00,     // mov ecx, 0xe2
  0x90, 0x90                        // nop nop
};

STATIC
PATCHER_GENERIC_PATCH
AppleIntelCPUPowerManagementPatch = {
  .Base    = NULL, // Symbolic patch
  .Find    = AppleIntelCPUPowerManagementPatchFind,
  .Mask    = NULL,
  .Replace = AppleIntelCPUPowerManagementPatchReplace,
  .ReplaceMask = NULL,
  .Size    = sizeof (AppleIntelCPUPowerManagementPatchFind),
  .Count   = 0,
  .Skip    = 0
};

STATIC
UINT8
AppleIntelCPUPowerManagementPatch2Find[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00,       // mov ecx, 0xe2
  0x48, 0x89, 0xF0,                   // mov rax, <some register>
  0x0F, 0x30                          // wrmsr
};

STATIC
UINT8
AppleIntelCPUPowerManagementPatch2FindMask[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xF0,
  0xFF, 0xFF
};

STATIC
UINT8
AppleIntelCPUPowerManagementPatch2Replace[] = {
  0x00, 0x00, 0x00, 0x00, 0x00,       // leave as is
  0x00, 0x00, 0x00,                   // leave as is
  0x90, 0x90                          // nop nop
};

STATIC
UINT8
AppleIntelCPUPowerManagementPatch2ReplaceMask[] = {
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00,
  0xFF, 0xFF
};

STATIC
PATCHER_GENERIC_PATCH
AppleIntelCPUPowerManagementPatch2 = {
  .Base    = NULL,
  .Find    = AppleIntelCPUPowerManagementPatch2Find,
  .Mask    = AppleIntelCPUPowerManagementPatch2FindMask,
  .Replace = AppleIntelCPUPowerManagementPatch2Replace,
  .ReplaceMask = AppleIntelCPUPowerManagementPatch2ReplaceMask,
  .Size    = sizeof (AppleIntelCPUPowerManagementPatch2Find),
  .Count   = 0,
  .Skip    = 0
};

/**
 Apply MSR E2 patches to AppleIntelCPUPowerManagement kext

 @param Context Prelinked kernel context
 */
EFI_STATUS
PatchAppleIntelCPUPowerManagement (
  PRELINKED_CONTEXT  *Context
  )
{
  EFI_STATUS       Status;
  PATCHER_CONTEXT  Patcher;

  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.driver.AppleIntelCPUPowerManagement"
    );
  
  if (!EFI_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &AppleIntelCPUPowerManagementPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to apply patch com.apple.driver.AppleIntelCPUPowerManagement - %r\n", Status));
    } else {
      DEBUG ((DEBUG_WARN, "Patch success com.apple.driver.AppleIntelCPUPowerManagement\n"));
    }
    Status = PatcherApplyGenericPatch (&Patcher, &AppleIntelCPUPowerManagementPatch2);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to apply patch com.apple.driver.AppleIntelCPUPowerManagement - %r\n", Status));
    } else {
      DEBUG ((DEBUG_WARN, "Patch success com.apple.driver.AppleIntelCPUPowerManagement\n"));
    }
  } else {
    DEBUG ((DEBUG_WARN, "Failed to find com.apple.driver.AppleIntelCPUPowerManagement - %r\n", Status));
  }
  
  return Status;
}

STATIC
UINT8
RemoveUsbLimitV1Find[] = {
  0xff, 0xff, 0x10
};

STATIC
UINT8
RemoveUsbLimitV1Replace[] = {
  0xff, 0xff, 0x40
};

STATIC
PATCHER_GENERIC_PATCH
RemoveUsbLimitV1Patch = {
  .Base    = "__ZN15AppleUSBXHCIPCI11createPortsEv",
  .Find    = RemoveUsbLimitV1Find,
  .Mask    = NULL,
  .Replace = RemoveUsbLimitV1Replace,
  .ReplaceMask = NULL,
  .Size    = sizeof (RemoveUsbLimitV1Replace),
  .Count   = 1,
  .Skip    = 0
};

STATIC
UINT8
RemoveUsbLimitV2Find[] = {
  0x0f, 0x0f, 0x83
};

STATIC
UINT8
RemoveUsbLimitV2Replace[] = {
  0x40, 0x0f, 0x83
};

STATIC
PATCHER_GENERIC_PATCH
RemoveUsbLimitV2Patch = {
  .Base    = "__ZN12AppleUSBXHCI11createPortsEv",
  .Find    = RemoveUsbLimitV2Find,
  .Mask    = NULL,
  .Replace = RemoveUsbLimitV2Replace,
  .ReplaceMask = NULL,
  .Size    = sizeof (RemoveUsbLimitV2Replace),
  .Count   = 1,
  .Skip    = 0
};

/**
 Apply port limit patches to AppleUSBXHCI and AppleUSBXHCIPCI kexts

 @param Context Prelinked kernel context
 */
EFI_STATUS
RemoveUSBXHCIPortLimit (
  PRELINKED_CONTEXT  *Context
  )
{
  EFI_STATUS       Status;
  PATCHER_CONTEXT  Patcher;
  
  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.driver.usb.AppleUSBXHCI"
    );
  
  if (!EFI_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &RemoveUsbLimitV2Patch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to apply patch com.apple.driver.usb.AppleUSBXHCI - %r\n", Status));
    } else {
      DEBUG ((DEBUG_WARN, "Patch success com.apple.driver.usb.AppleUSBXHCI\n"));
    }
  } else {
    DEBUG ((DEBUG_WARN, "Failed to find com.apple.driver.usb.AppleUSBXHCI - %r\n", Status));
  }
  
  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.driver.usb.AppleUSBXHCIPCI"
    );
  
  if (!EFI_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &RemoveUsbLimitV1Patch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to apply patch com.apple.driver.usb.AppleUSBXHCIPCI - %r\n", Status));
    } else {
      DEBUG ((DEBUG_WARN, "Patch success com.apple.driver.usb.AppleUSBXHCIPCI\n"));
    }
  } else {
    DEBUG ((DEBUG_WARN, "Failed to find com.apple.driver.usb.AppleUSBXHCIPCI - %r\n", Status));
  }
  
  return Status;
}

