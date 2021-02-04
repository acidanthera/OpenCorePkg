/** @file
  Produces Keyboard Info Protocol.

Copyright (c) 2016 - 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EfiKey.h"
#include "AppleKey.h"

#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define ASSERT_USB_KB_DEV_VALID(UsbKbDev)                    \
  do {                                                       \
    ASSERT ((UsbKbDev) != NULL);                             \
    ASSERT ((UsbKbDev)->Signature == USB_KB_DEV_SIGNATURE);  \
  } while (FALSE)

STATIC
VOID
UsbKbSetAppleKeyMapDb (
  IN USB_KB_DEV                       *UsbKeyboardDevice,
  IN APPLE_KEY_MAP_DATABASE_PROTOCOL  *KeyMapDb
  )
{
  EFI_STATUS Status;

  ASSERT_USB_KB_DEV_VALID (UsbKeyboardDevice);
  ASSERT (KeyMapDb != NULL);

  Status = KeyMapDb->CreateKeyStrokesBuffer (
                       KeyMapDb,
                       6,
                       &UsbKeyboardDevice->KeyMapDbIndex
                       );
  if (!EFI_ERROR (Status)) {
    UsbKeyboardDevice->KeyMapDb = KeyMapDb;
  }
}

/**
  Protocol installation notify for Apple KeyMap Database.

  @param[in] Event    Indicates the event that invoke this function.
  @param[in] Context  Indicates the calling context.

**/
STATIC
VOID
EFIAPI
UsbKbAppleKeyMapDbInstallNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                      Status;
  APPLE_KEY_MAP_DATABASE_PROTOCOL *KeyMapDb;
  USB_KB_DEV                      *UsbKeyboardDevice;

  ASSERT (Event != NULL);
  ASSERT_USB_KB_DEV_VALID ((USB_KB_DEV *)Context);
  ASSERT (((USB_KB_DEV *)Context)->KeyMapInstallNotifyEvent == Event);

  UsbKeyboardDevice = (USB_KB_DEV *)Context;
  Status = gBS->LocateProtocol (
                  &gAppleKeyMapDatabaseProtocolGuid,
                  UsbKeyboardDevice->KeyMapInstallRegistration,
                  (VOID **)&KeyMapDb
                  );
  ASSERT (Status != EFI_NOT_FOUND);

  UsbKbSetAppleKeyMapDb (UsbKeyboardDevice, KeyMapDb);

  gBS->CloseEvent (UsbKeyboardDevice->KeyMapInstallNotifyEvent);
  UsbKeyboardDevice->KeyMapInstallNotifyEvent = NULL;
  UsbKeyboardDevice->KeyMapInstallRegistration = NULL;
}

VOID
UsbKbLocateAppleKeyMapDb (
  IN USB_KB_DEV  *UsbKeyboardDevice
  )
{
  EFI_STATUS                      Status;
  APPLE_KEY_MAP_DATABASE_PROTOCOL *KeyMapDb;

  ASSERT_USB_KB_DEV_VALID (UsbKeyboardDevice);

  Status = gBS->LocateProtocol (
                  &gAppleKeyMapDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&KeyMapDb
                  );
  if (!EFI_ERROR (Status)) {
    UsbKbSetAppleKeyMapDb (UsbKeyboardDevice, KeyMapDb);
  } else if (PcdGetBool (PcdNotifyAppleKeyMapDbInUsbKbDriver)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    UsbKbAppleKeyMapDbInstallNotify,
                    (VOID *)UsbKeyboardDevice,
                    &UsbKeyboardDevice->KeyMapInstallNotifyEvent
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->RegisterProtocolNotify (
                    &gAppleKeyMapDatabaseProtocolGuid,
                    UsbKeyboardDevice->KeyMapInstallNotifyEvent,
                    &UsbKeyboardDevice->KeyMapInstallRegistration
                    );
    ASSERT_EFI_ERROR (Status);
  }
}

VOID
UsbKbFreeAppleKeyMapDb (
  IN USB_KB_DEV  *UsbKeyboardDevice
  )
{
  EFI_STATUS Status;

  ASSERT_USB_KB_DEV_VALID (UsbKeyboardDevice);

  if (UsbKeyboardDevice->KeyMapDb != NULL) {
    Status = UsbKeyboardDevice->KeyMapDb->RemoveKeyStrokesBuffer (
                                            UsbKeyboardDevice->KeyMapDb,
                                            UsbKeyboardDevice->KeyMapDbIndex
                                            );
    ASSERT_EFI_ERROR (Status);
  } else if (UsbKeyboardDevice->KeyMapInstallNotifyEvent != NULL) {
    Status = gBS->CloseEvent (UsbKeyboardDevice->KeyMapInstallNotifyEvent);
    ASSERT_EFI_ERROR (Status);

    UsbKeyboardDevice->KeyMapInstallNotifyEvent = NULL;
  }
}
