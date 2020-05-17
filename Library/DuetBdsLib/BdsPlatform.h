/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BdsPlatform.h

Abstract:

  Head file for BDS Platform specific code

--*/

#ifndef _PLATFORM_SPECIFIC_BDS_PLATFORM_H_
#define _PLATFORM_SPECIFIC_BDS_PLATFORM_H_

#include <Library/DuetBdsLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/DevicePath.h>

extern BDS_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];
extern EFI_DEVICE_PATH_PROTOCOL   *gPlatformDriverOption[];
extern EFI_DEVICE_PATH_PROTOCOL   *gPlatformRootBridges[];
extern ACPI_HID_DEVICE_PATH       gPnpPs2KeyboardDeviceNode;

//
// attributes for reserved memory before it is promoted to system memory
//
#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL

#define VarConsoleInpDev        L"ConInDev"
#define VarConsoleInp           L"ConIn"
#define VarConsoleOutDev        L"ConOutDev"
#define VarConsoleOut           L"ConOut"
#define VarErrorOutDev          L"ErrOutDev"
#define VarErrorOut             L"ErrOut"

#define IS_PCI_ISA_PDECODE(_p)        IS_CLASS3 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA_PDECODE, 0)

#define PCI_DEVICE_PATH_NODE(Func, Dev) \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_PCI_DP, \
      { \
        (UINT8) (sizeof (PCI_DEVICE_PATH)), \
        (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8) \
      } \
    }, \
    (Func), \
    (Dev) \
  }

#define PNPID_DEVICE_PATH_NODE(PnpId) \
  { \
    { \
      ACPI_DEVICE_PATH, \
      ACPI_DP, \
      { \
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), \
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) \
      } \
    }, \
    EISA_PNP_ID((PnpId)), \
    0 \
  }

#define gPciRootBridge \
  PNPID_DEVICE_PATH_NODE(0x0A03)

#define gPciIsaBridge \
  PCI_DEVICE_PATH_NODE(0, 0x1f)

#define gPnpPs2Keyboard \
  PNPID_DEVICE_PATH_NODE(0x0303)

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, \
    END_ENTIRE_DEVICE_PATH_SUBTYPE, \
    { \
      END_DEVICE_PATH_LENGTH, \
      0 \
    } \
  }

#define EFI_SYSTEM_TABLE_MAX_ADDRESS 0xFFFFFFFF
#define SYS_TABLE_PAD(ptr) (((~ptr) +1) & 0x07 )

//
// Platform Root Bridge
//
typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ROOT_BRIDGE_DEVICE_PATH;

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           IsaBridge;
  ACPI_HID_DEVICE_PATH      Keyboard;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_DUMMY_ISA_KEYBOARD_DEVICE_PATH;

typedef struct {
  USB_CLASS_DEVICE_PATH           UsbClass;
  EFI_DEVICE_PATH_PROTOCOL        End;
} USB_CLASS_FORMAT_DEVICE_PATH;

//
// the short form device path for Usb keyboard
//
#define CLASS_HID           3
#define SUBCLASS_BOOT       1
#define PROTOCOL_KEYBOARD   1

extern USB_CLASS_FORMAT_DEVICE_PATH      gUsbClassKeyboardDevicePath;
extern PLATFORM_ROOT_BRIDGE_DEVICE_PATH  gPlatformRootBridge0;

#endif // _PLATFORM_SPECIFIC_BDS_PLATFORM_H_
