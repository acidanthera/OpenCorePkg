/** @file
  Header file for Apple-specific USB Keyboard Driver's Data Structures.

Copyright (c) 2016 - 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef APPLE_USB_KB_H_
#define APPLE_USB_KB_H_

#include "EfiKey.h"

#include <Guid/ApplePlatformInfo.h>

#include <Protocol/AppleKeyMapDatabase.h>
#include <Protocol/ApplePlatformInfoDatabase.h>
#include <Protocol/KeyboardInfo.h>
#include <Protocol/UsbIo.h>

//
// Functions of Keyboard Info Protocol
//
VOID
UsbKbLocateAppleKeyMapDb (
  IN USB_KB_DEV  *UsbKeyboardDevice
  );

VOID
UsbKbFreeAppleKeyMapDb (
  IN USB_KB_DEV  *UsbKeyboardDevice
  );

#endif // APPLE_USB_KB_H_

