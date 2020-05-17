/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PlatformData.c

Abstract:

  Defined the platform specific device path which will be used by
  platform Bbd to perform the platform policy connect.

--*/

#include "BdsPlatform.h"

ACPI_HID_DEVICE_PATH       gPnpPs2KeyboardDeviceNode  = gPnpPs2Keyboard;

//
// Predefined platform root bridge
//
PLATFORM_ROOT_BRIDGE_DEVICE_PATH  gPlatformRootBridge0 = {
  gPciRootBridge,
  gEndEntire
};

EFI_DEVICE_PATH_PROTOCOL          *gPlatformRootBridges[] = {
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformRootBridge0,
  NULL
};

USB_CLASS_FORMAT_DEVICE_PATH gUsbClassKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      {
        (UINT8) (sizeof (USB_CLASS_DEVICE_PATH)),
        (UINT8) ((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
      }
    },
    0xffff,           // VendorId
    0xffff,           // ProductId
    CLASS_HID,        // DeviceClass
    SUBCLASS_BOOT,    // DeviceSubClass
    PROTOCOL_KEYBOARD // DeviceProtocol
  },

  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};


//
// Platform specific Dummy ISA keyboard device path
//

PLATFORM_DUMMY_ISA_KEYBOARD_DEVICE_PATH gDummyIsaKeyboardDevicePath = {
  gPciRootBridge,
  gPciIsaBridge,
  gPnpPs2Keyboard,
  gEndEntire
};

//
// Predefined platform default console device path
//
BDS_CONSOLE_CONNECT_ENTRY         gPlatformConsole[] = {
  //
  // need update dynamically
  //
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gDummyIsaKeyboardDevicePath,
    (CONSOLE_IN | STD_ERROR)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL*) &gUsbClassKeyboardDevicePath,
    CONSOLE_IN
  },
  {
    NULL,
    0
  }
};

//
// Predefined platform specific driver option
//
EFI_DEVICE_PATH_PROTOCOL    *gPlatformDriverOption[] = { NULL };
