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

#include <Base.h>

#include <Library/DebugLib.h>
#include <Library/OcAppleKernelLib.h>

STATIC
UINT8
mAppleIntelCPUPowerManagementPatchFind[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00,     // mov ecx, 0xe2
  0x0F, 0x30                        // wrmsr
};

STATIC
UINT8
mAppleIntelCPUPowerManagementPatchReplace[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00,     // mov ecx, 0xe2
  0x90, 0x90                        // nop nop
};

STATIC
PATCHER_GENERIC_PATCH
mAppleIntelCPUPowerManagementPatch = {
  .Base        = NULL,
  .Find        = mAppleIntelCPUPowerManagementPatchFind,
  .Mask        = NULL,
  .Replace     = mAppleIntelCPUPowerManagementPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mAppleIntelCPUPowerManagementPatchFind),
  .Count       = 0,
  .Skip        = 0
};

STATIC
UINT8
mAppleIntelCPUPowerManagementPatch2Find[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00,       // mov ecx, 0xe2
  0x48, 0x89, 0xF0,                   // mov rax, <some register>
  0x0F, 0x30                          // wrmsr
};

STATIC
UINT8
mAppleIntelCPUPowerManagementPatch2FindMask[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xF0,
  0xFF, 0xFF
};

STATIC
UINT8
mAppleIntelCPUPowerManagementPatch2Replace[] = {
  0x00, 0x00, 0x00, 0x00, 0x00,       // leave as is
  0x00, 0x00, 0x00,                   // leave as is
  0x90, 0x90                          // nop nop
};

STATIC
UINT8
mAppleIntelCPUPowerManagementPatch2ReplaceMask[] = {
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00,
  0xFF, 0xFF
};

STATIC
PATCHER_GENERIC_PATCH
mAppleIntelCPUPowerManagementPatch2 = {
  .Base        = NULL,
  .Find        = mAppleIntelCPUPowerManagementPatch2Find,
  .Mask        = mAppleIntelCPUPowerManagementPatch2FindMask,
  .Replace     = mAppleIntelCPUPowerManagementPatch2Replace,
  .ReplaceMask = mAppleIntelCPUPowerManagementPatch2ReplaceMask,
  .Size        = sizeof (mAppleIntelCPUPowerManagementPatch2Find),
  .Count       = 0,
  .Skip        = 0
};

RETURN_STATUS
PatchAppleCpuPmCfgLock (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  RETURN_STATUS       Status;
  RETURN_STATUS       Status2;
  PATCHER_CONTEXT     Patcher;

  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.driver.AppleIntelCPUPowerManagement"
    );

  if (!RETURN_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &mAppleIntelCPUPowerManagementPatch);
    if (RETURN_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Failed to apply patch com.apple.driver.AppleIntelCPUPowerManagement - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "Patch success com.apple.driver.AppleIntelCPUPowerManagement\n"));
    }
    Status2 = PatcherApplyGenericPatch (&Patcher, &mAppleIntelCPUPowerManagementPatch2);
    if (RETURN_ERROR (Status2)) {
      DEBUG ((DEBUG_INFO, "Failed to apply patch com.apple.driver.AppleIntelCPUPowerManagement - %r\n", Status2));
    } else {
      DEBUG ((DEBUG_INFO, "Patch success com.apple.driver.AppleIntelCPUPowerManagement\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "Failed to find com.apple.driver.AppleIntelCPUPowerManagement - %r\n", Status));
  }

  return RETURN_ERROR (Status) ? Status : Status2;
}

#pragma pack(push, 1)

//
// XCPM record definition, extracted from XNU debug kernel.
//
typedef struct XCPM_MSR_RECORD_ {
  UINT32  xcpm_msr_num;
  UINT32  xcpm_msr_applicable_cpus;
  UINT32  *xcpm_msr_flag_p;
  UINT64  xcpm_msr_bits_clear;
  UINT64  xcpm_msr_bits_set;
  UINT64  xcpm_msr_initial_value;
  UINT64  xcpm_msr_rb_value;
} XCPM_MSR_RECORD;

#pragma pack(pop)

RETURN_STATUS
PatchAppleXcpmCfgLock (
  IN OUT PATCHER_CONTEXT  *Patcher
  )
{
  RETURN_STATUS       Status;
  XCPM_MSR_RECORD     *Record;
  XCPM_MSR_RECORD     *Last;

  UINT32              Replacements;

  Last = (XCPM_MSR_RECORD *) ((UINT8 *) MachoGetMachHeader64 (&Patcher->MachContext)
    + MachoGetFileSize (&Patcher->MachContext) - sizeof (XCPM_MSR_RECORD));

  Replacements = 0;

  Status = PatcherGetSymbolAddress (Patcher, "_xcpm_core_scope_msrs", (UINT8 **) &Record);
  if (!RETURN_ERROR (Status)) {
    while (Record < Last) {
      if (Record->xcpm_msr_num == 0xE2) {
        DEBUG ((
          DEBUG_INFO,
          "Replacing _xcpm_core_scope_msrs data %u %u\n",
          Record->xcpm_msr_num,
          Record->xcpm_msr_applicable_cpus
          ));
        Record->xcpm_msr_applicable_cpus = 0;
        ++Replacements;
      } else {
        DEBUG ((
          DEBUG_INFO,
          "Not matching _xcpm_core_scope_msrs data %u %u\n",
          Record->xcpm_msr_num,
          Record->xcpm_msr_applicable_cpus
          ));
        break;
      }
      ++Record;
    }
  } else {
    DEBUG ((DEBUG_WARN, "Failed to locate _xcpm_core_scope_msrs - %r\n", Status));
  }

  return Replacements > 0 ? EFI_SUCCESS : EFI_NOT_FOUND;
}

STATIC
UINT8
mRemoveUsbLimitV1Find[] = {
  0xff, 0xff, 0x10
};

STATIC
UINT8
mRemoveUsbLimitV1Replace[] = {
  0xff, 0xff, 0x40
};

STATIC
PATCHER_GENERIC_PATCH
mRemoveUsbLimitV1Patch = {
  .Base        = "__ZN15AppleUSBXHCIPCI11createPortsEv",
  .Find        = mRemoveUsbLimitV1Find,
  .Mask        = NULL,
  .Replace     = mRemoveUsbLimitV1Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mRemoveUsbLimitV1Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
UINT8
mRemoveUsbLimitV2Find[] = {
  0x0f, 0x0f, 0x83
};

STATIC
UINT8
mRemoveUsbLimitV2Replace[] = {
  0x40, 0x0f, 0x83
};

STATIC
PATCHER_GENERIC_PATCH
mRemoveUsbLimitV2Patch = {
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
UINT8
mRemoveUsbLimitIoP1Find[] = {
  0x0f, 0x0f, 0x87
};

STATIC
UINT8
mRemoveUsbLimitIoP1Replace[] = {
  0x40, 0x0f, 0x87
};

STATIC
PATCHER_GENERIC_PATCH
mRemoveUsbLimitIoP1Patch = {
  .Base        = "__ZN16AppleUSBHostPort15setPortLocationEj",
  .Find        = mRemoveUsbLimitIoP1Find,
  .Mask        = NULL,
  .Replace     = mRemoveUsbLimitIoP1Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mRemoveUsbLimitIoP1Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

RETURN_STATUS
PatchUsbXhciPortLimit (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  RETURN_STATUS       Status;
  PATCHER_CONTEXT  Patcher;

  //
  // On 10.14.4 and newer IOUSBHostFamily also needs limit removal.
  // Thanks to ydeng discovering this.
  //
  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.iokit.IOUSBHostFamily"
    );

  if (!RETURN_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &mRemoveUsbLimitIoP1Patch);
    if (RETURN_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Failed to apply P1 patch com.apple.iokit.IOUSBHostFamily - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "Patch success com.apple.iokit.IOUSBHostFamily\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "Failed to find com.apple.iokit.IOUSBHostFamily - %r\n", Status));
  }

  //
  // TODO: Implement some locationID hack in IOUSBHFamily.
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

  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.driver.usb.AppleUSBXHCI"
    );

  if (!RETURN_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &mRemoveUsbLimitV2Patch);
    if (!RETURN_ERROR (Status)) {
      //
      // We do not need to patch com.apple.driver.usb.AppleUSBXHCI if this patch was successful.
      // Only legacy systems require com.apple.driver.usb.AppleUSBXHCI to be patched.
      //
      DEBUG ((DEBUG_INFO, "Patch success com.apple.driver.usb.AppleUSBXHCI\n"));
      return RETURN_SUCCESS;
    }

    DEBUG ((DEBUG_INFO, "Failed to apply patch com.apple.driver.usb.AppleUSBXHCI - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "Failed to find com.apple.driver.usb.AppleUSBXHCI - %r\n", Status));
  }

  //
  // If we are here, we are on legacy 10.13 or below, try the oldest patch.
  //
  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.driver.usb.AppleUSBXHCIPCI"
    );

  if (!RETURN_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &mRemoveUsbLimitV1Patch);
    if (RETURN_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Failed to apply patch com.apple.driver.usb.AppleUSBXHCIPCI - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "Patch success com.apple.driver.usb.AppleUSBXHCIPCI\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "Failed to find com.apple.driver.usb.AppleUSBXHCIPCI - %r\n", Status));
  }

  return Status;
}

STATIC
UINT8
mIOAHCIBlockStoragePatchFind[] = {
  0x41, 0x50, 0x50, 0x4C, 0x45, 0x20, 0x53, 0x53, 0x44, 0x00
};

STATIC
UINT8
mIOAHCIBlockStoragePatchReplace[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
mIOAHCIBlockStoragePatch = {
  .Base        = NULL,
  .Find        = mIOAHCIBlockStoragePatchFind,
  .Mask        = NULL,
  .Replace     = mIOAHCIBlockStoragePatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIOAHCIBlockStoragePatchFind),
  .Count       = 1,
  .Skip        = 0
};

RETURN_STATUS
PatchThirdPartySsdTrim (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  RETURN_STATUS       Status;
  PATCHER_CONTEXT  Patcher;

  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.iokit.IOAHCIBlockStorage"
    );

  if (!RETURN_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &mIOAHCIBlockStoragePatch);
    if (RETURN_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Failed to apply patch com.apple.iokit.IOAHCIBlockStorage - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "Patch success com.apple.iokit.IOAHCIBlockStorage\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "Failed to find com.apple.iokit.IOAHCIBlockStorage - %r\n", Status));
  }

  return Status;
}

STATIC
UINT8
mIOAHCIPortPatchFind[] = {
  0x45, 0x78, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C
};

STATIC
UINT8
mIOAHCIPortPatchReplace[] = {
  0x49, 0x6E, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C
};

STATIC
PATCHER_GENERIC_PATCH
mIOAHCIPortPatch = {
  .Base    = NULL,
  .Find    = mIOAHCIPortPatchFind,
  .Mask    = NULL,
  .Replace = mIOAHCIPortPatchReplace,
  .ReplaceMask = NULL,
  .Size    = sizeof (mIOAHCIPortPatchFind),
  .Count   = 1,
  .Skip    = 0
};

RETURN_STATUS
PatchForceInternalDiskIcons (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  RETURN_STATUS       Status;
  PATCHER_CONTEXT  Patcher;

  Status = PatcherInitContextFromPrelinked (
    &Patcher,
    Context,
    "com.apple.driver.AppleAHCIPort"
    );

  if (!RETURN_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (&Patcher, &mIOAHCIPortPatch);
    if (RETURN_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Failed to apply patch com.apple.driver.AppleAHCIPort - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "Patch success com.apple.driver.AppleAHCIPort\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "Failed to find com.apple.driver.AppleAHCIPort - %r\n", Status));
  }

  return Status;
}