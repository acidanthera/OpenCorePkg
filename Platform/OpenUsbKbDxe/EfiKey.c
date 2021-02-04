/** @file
  USB Keyboard Driver that manages USB keyboard and produces Simple Text Input
  Protocol and Simple Text Input Ex Protocol.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
Portions Copyright (C) 2016 - 2018, Download-Fritz. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EfiKey.h"
#include "AppleKey.h"
#include "KeyBoard.h"

STATIC BOOLEAN mExitingBootServices = FALSE;

//
// USB Keyboard Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gUsbKeyboardDriverBinding = {
  USBKeyboardDriverBindingSupported,
  USBKeyboardDriverBindingStart,
  USBKeyboardDriverBindingStop,
  0x10,
  NULL,
  NULL
};

/**
  Entrypoint of USB Keyboard Driver.

  This function is the entrypoint of USB Keyboard Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUsbKeyboardDriverBinding,
             ImageHandle,
             &gUsbKeyboardComponentName,
             &gUsbKeyboardComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Check whether USB keyboard driver supports this device.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS          Status;
  EFI_USB_IO_PROTOCOL *UsbIo;

  //
  // Check if USB I/O Protocol is attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  PcdGetBool (PcdUsbKbDriverTakePrecedence)
                    ? EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    : EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Use the USB I/O Protocol interface to check whether Controller is
  // a keyboard device that can be managed by this driver.
  //
  Status = EFI_SUCCESS;

  if (!IsUSBKeyboard (UsbIo)) {
    Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Starts the keyboard device with this driver.

  This function produces Simple Text Input Protocol and Simple Text Input Ex Protocol,
  initializes the keyboard device, and submit Asynchronous Interrupt Transfer to manage
  this keyboard device.

  @param  This                   The USB keyboard driver binding instance.
  @param  Controller             Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The controller is controlled by the usb keyboard driver.
  @retval EFI_UNSUPPORTED        No interrupt endpoint can be found.
  @retval Other                  This controller cannot be started.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  USB_KB_DEV                    *UsbKeyboardDevice;
  UINT8                         EndpointNumber;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
  UINT8                         Index;
  UINT8                         EndpointAddr;
  UINT8                         PollingInterval;
  UINT8                         PacketSize;
  BOOLEAN                       Found;
  EFI_TPL                       OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  //
  // Open USB I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (PcdGetBool (PcdUsbKbDriverTakePrecedence)) {
    if (Status == EFI_ACCESS_DENIED) {
      Status = gBS->DisconnectController (
                      Controller,
                      NULL,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        goto ErrorExit1;
      }

      Status = gBS->OpenProtocol (
                      Controller,
                      &gEfiUsbIoProtocolGuid,
                      (VOID **) &UsbIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );
    }
  }
  if (EFI_ERROR (Status)) {
    goto ErrorExit1;
  }

  UsbKeyboardDevice = AllocateZeroPool (sizeof (USB_KB_DEV));
  ASSERT (UsbKeyboardDevice != NULL);

  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbKeyboardDevice->DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }
  //
  // Report that the USB keyboard is being enabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_ENABLE),
    UsbKeyboardDevice->DevicePath
    );

  //
  // This is pretty close to keyboard detection, so log progress
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_PRESENCE_DETECT),
    UsbKeyboardDevice->DevicePath
    );

  UsbKeyboardDevice->UsbIo = UsbIo;

  //
  // Valid signature is required by UsbKbLocateAppleKeyMapDb.
  //
  UsbKeyboardDevice->Signature = USB_KB_DEV_SIGNATURE;
  UsbKbLocateAppleKeyMapDb (UsbKeyboardDevice);

  //
  // Get interface & endpoint descriptor
  //
  UsbIo->UsbGetInterfaceDescriptor (
           UsbIo,
           &UsbKeyboardDevice->InterfaceDescriptor
           );

  EndpointNumber = UsbKeyboardDevice->InterfaceDescriptor.NumEndpoints;

  //
  // Traverse endpoints to find interrupt endpoint IN
  //
  Found = FALSE;
  for (Index = 0; Index < EndpointNumber; Index++) {

    UsbIo->UsbGetEndpointDescriptor (
             UsbIo,
             Index,
             &EndpointDescriptor
             );

    if (((EndpointDescriptor.Attributes & (BIT0 | BIT1)) == USB_ENDPOINT_INTERRUPT) &&
        ((EndpointDescriptor.EndpointAddress & USB_ENDPOINT_DIR_IN) != 0)) {
      //
      // We only care interrupt endpoint here
      //
      CopyMem(&UsbKeyboardDevice->IntEndpointDescriptor, &EndpointDescriptor, sizeof(EndpointDescriptor));
      Found = TRUE;
      break;
    }
  }

  if (!Found) {
    //
    // Report Status Code to indicate that there is no USB keyboard
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_NOT_DETECTED)
      );
    //
    // No interrupt endpoint found, then return unsupported.
    //
    Status = EFI_UNSUPPORTED;
    goto ErrorExit;
  }

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DETECTED),
    UsbKeyboardDevice->DevicePath
    );

  UsbKeyboardDevice->Signature                  = USB_KB_DEV_SIGNATURE;
  UsbKeyboardDevice->SimpleInput.Reset          = USBKeyboardReset;
  UsbKeyboardDevice->SimpleInput.ReadKeyStroke  = USBKeyboardReadKeyStroke;

  UsbKeyboardDevice->SimpleInputEx.Reset               = USBKeyboardResetEx;
  UsbKeyboardDevice->SimpleInputEx.ReadKeyStrokeEx     = USBKeyboardReadKeyStrokeEx;
  UsbKeyboardDevice->SimpleInputEx.SetState            = USBKeyboardSetState;
  UsbKeyboardDevice->SimpleInputEx.RegisterKeyNotify   = USBKeyboardRegisterKeyNotify;
  UsbKeyboardDevice->SimpleInputEx.UnregisterKeyNotify = USBKeyboardUnregisterKeyNotify;

  InitializeListHead (&UsbKeyboardDevice->NotifyList);

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  USBKeyboardTimerHandler,
                  UsbKeyboardDevice,
                  &UsbKeyboardDevice->TimerEvent
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->SetTimer (UsbKeyboardDevice->TimerEvent, TimerPeriodic, KEYBOARD_TIMER_INTERVAL);
  }
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  USBKeyboardWaitForKey,
                  UsbKeyboardDevice,
                  &(UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx)
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  USBKeyboardWaitForKey,
                  UsbKeyboardDevice,
                  &(UsbKeyboardDevice->SimpleInput.WaitForKey)
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  KeyNotifyProcessHandler,
                  UsbKeyboardDevice,
                  &UsbKeyboardDevice->KeyNotifyProcessEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Install Simple Text Input Protocol and Simple Text Input Ex Protocol
  // for the USB keyboard device.
  // USB keyboard is a hot plug device, and expected to work immediately
  // when plugging into system, other conventional console devices could
  // distinguish it by its device path.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  &UsbKeyboardDevice->SimpleInput,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &UsbKeyboardDevice->SimpleInputEx,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  UsbKeyboardDevice->ControllerHandle = Controller;
  Status = InitKeyboardLayout (UsbKeyboardDevice);
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
      Controller,
      &gEfiSimpleTextInProtocolGuid,
      &UsbKeyboardDevice->SimpleInput,
      &gEfiSimpleTextInputExProtocolGuid,
      &UsbKeyboardDevice->SimpleInputEx,
      NULL
      );
    goto ErrorExit;
  }


  //
  // Reset USB Keyboard Device exhaustively.
  //
  Status = UsbKeyboardDevice->SimpleInputEx.Reset (
                                            &UsbKeyboardDevice->SimpleInputEx,
                                            TRUE
                                            );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Controller,
           &gEfiSimpleTextInProtocolGuid,
           &UsbKeyboardDevice->SimpleInput,
           &gEfiSimpleTextInputExProtocolGuid,
           &UsbKeyboardDevice->SimpleInputEx,
           NULL
           );
    goto ErrorExit;
  }

  //
  // Submit Asynchronous Interrupt Transfer to manage this device.
  //
  EndpointAddr    = UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress;
  PollingInterval = UsbKeyboardDevice->IntEndpointDescriptor.Interval;
  PacketSize      = (UINT8) (UsbKeyboardDevice->IntEndpointDescriptor.MaxPacketSize);

  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EndpointAddr,
                    TRUE,
                    PollingInterval,
                    PacketSize,
                    KeyboardHandler,
                    UsbKeyboardDevice
                    );

  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Controller,
           &gEfiSimpleTextInProtocolGuid,
           &UsbKeyboardDevice->SimpleInput,
           &gEfiSimpleTextInputExProtocolGuid,
           &UsbKeyboardDevice->SimpleInputEx,
           NULL
           );
    goto ErrorExit;
  }

  UsbKeyboardDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gUsbKeyboardComponentName.SupportedLanguages,
    &UsbKeyboardDevice->ControllerNameTable,
    L"Generic Usb Keyboard",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gUsbKeyboardComponentName2.SupportedLanguages,
    &UsbKeyboardDevice->ControllerNameTable,
    L"Generic Usb Keyboard",
    FALSE
    );

  gBS->RestoreTPL (OldTpl);

  if (PcdGetBool (PcdEnableDisconnectOnExitBootServicesInUsbKbDriver)) {
    Status = gBS->CreateEvent (
                    EVT_SIGNAL_EXIT_BOOT_SERVICES,
                    TPL_NOTIFY,
                    USBKeyboardExitBootServices,
                    UsbKeyboardDevice,
                    &UsbKeyboardDevice->ExitBootServicesEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;

//
// Error handler
//
ErrorExit:
  if (UsbKeyboardDevice != NULL) {
    if (UsbKeyboardDevice->TimerEvent != NULL) {
      gBS->CloseEvent (UsbKeyboardDevice->TimerEvent);
    }
    if (UsbKeyboardDevice->SimpleInput.WaitForKey != NULL) {
      gBS->CloseEvent (UsbKeyboardDevice->SimpleInput.WaitForKey);
    }
    if (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx != NULL) {
      gBS->CloseEvent (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx);
    }
    if (UsbKeyboardDevice->KeyNotifyProcessEvent != NULL) {
      gBS->CloseEvent (UsbKeyboardDevice->KeyNotifyProcessEvent);
    }
    ReleaseKeyboardLayoutResources (UsbKeyboardDevice);
    UsbKbFreeAppleKeyMapDb (UsbKeyboardDevice);
    FreePool (UsbKeyboardDevice);
    UsbKeyboardDevice = NULL;
  }
  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

ErrorExit1:
  gBS->RestoreTPL (OldTpl);

  return Status;

}


/**
  Stop the USB keyboard device handled by this driver.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_UNSUPPORTED        Simple Text In Protocol or Simple Text In Ex Protocol
                                 is not installed on Controller.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
USBKeyboardDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                     Status;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleInput;
  USB_KB_DEV                     *UsbKeyboardDevice;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID **) &SimpleInput,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSimpleTextInputExProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UsbKeyboardDevice = USB_KB_DEV_FROM_THIS (SimpleInput);

  //
  // The key data input from this device will be disabled.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DISABLE),
    UsbKeyboardDevice->DevicePath
    );

  //
  // Delete the Asynchronous Interrupt Transfer from this device
  //
  UsbKeyboardDevice->UsbIo->UsbAsyncInterruptTransfer (
                              UsbKeyboardDevice->UsbIo,
                              UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
                              FALSE,
                              UsbKeyboardDevice->IntEndpointDescriptor.Interval,
                              0,
                              NULL,
                              NULL
                              );

  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  if (!PcdGetBool (PcdEnableDisconnectOnExitBootServicesInUsbKbDriver)
   || !mExitingBootServices) {
    UsbKbFreeAppleKeyMapDb (UsbKeyboardDevice);

    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Controller,
                    &gEfiSimpleTextInProtocolGuid,
                    &UsbKeyboardDevice->SimpleInput,
                    &gEfiSimpleTextInputExProtocolGuid,
                    &UsbKeyboardDevice->SimpleInputEx,
                    NULL
                    );
    //
    // Free all resources.
    //
    gBS->CloseEvent (UsbKeyboardDevice->TimerEvent);
    gBS->CloseEvent (UsbKeyboardDevice->RepeatTimer);
    gBS->CloseEvent (UsbKeyboardDevice->DelayedRecoveryEvent);
    gBS->CloseEvent (UsbKeyboardDevice->SimpleInput.WaitForKey);
    gBS->CloseEvent (UsbKeyboardDevice->SimpleInputEx.WaitForKeyEx);
    gBS->CloseEvent (UsbKeyboardDevice->KeyNotifyProcessEvent);

    if (PcdGetBool (PcdEnableDisconnectOnExitBootServicesInUsbKbDriver)) {
      gBS->CloseEvent (UsbKeyboardDevice->ExitBootServicesEvent);
    }

    KbdFreeNotifyList (&UsbKeyboardDevice->NotifyList);

    ReleaseKeyboardLayoutResources (UsbKeyboardDevice);

    if (UsbKeyboardDevice->ControllerNameTable != NULL) {
      FreeUnicodeStringTable (UsbKeyboardDevice->ControllerNameTable);
    }

    DestroyQueue (&UsbKeyboardDevice->UsbKeyQueue);
    DestroyQueue (&UsbKeyboardDevice->EfiKeyQueue);
    DestroyQueue (&UsbKeyboardDevice->EfiKeyQueueForNotify);

    FreePool (UsbKeyboardDevice);
  }

  return Status;
}

/**
  Internal function to read the next keystroke from the keyboard buffer.

  @param  UsbKeyboardDevice       USB keyboard's private structure.
  @param  KeyData                 A pointer to buffer to hold the keystroke
                                  data for the key that was pressed.

  @retval EFI_SUCCESS             The keystroke information was returned.
  @retval EFI_NOT_READY           There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR        The keystroke information was not returned due to
                                  hardware errors.
  @retval EFI_INVALID_PARAMETER   KeyData is NULL.
  @retval Others                  Fail to translate keycode into EFI_INPUT_KEY

**/
EFI_STATUS
USBKeyboardReadKeyStrokeWorker (
  IN OUT USB_KB_DEV                 *UsbKeyboardDevice,
     OUT EFI_KEY_DATA               *KeyData
  )
{
  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsQueueEmpty (&UsbKeyboardDevice->EfiKeyQueue)) {
    ZeroMem (&KeyData->Key, sizeof (KeyData->Key));
    InitializeKeyState (UsbKeyboardDevice, &KeyData->KeyState);
    return EFI_NOT_READY;
  }

  Dequeue (&UsbKeyboardDevice->EfiKeyQueue, KeyData, sizeof (*KeyData));

  return EFI_SUCCESS;
}

/**
  Reset the input device and optionally run diagnostics

  There are 2 types of reset for USB keyboard.
  For non-exhaustive reset, only keyboard buffer is cleared.
  For exhaustive reset, in addition to clearance of keyboard buffer, the hardware status
  is also re-initialized.

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
EFI_STATUS
EFIAPI
USBKeyboardReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  IN  BOOLEAN                          ExtendedVerification
  )
{
  EFI_STATUS          Status;
  USB_KB_DEV          *UsbKeyboardDevice;

  UsbKeyboardDevice = USB_KB_DEV_FROM_THIS (This);

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_RESET),
    UsbKeyboardDevice->DevicePath
    );

  //
  // Non-exhaustive reset:
  // only reset private data structures.
  //
  if (!ExtendedVerification) {
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      (EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_CLEAR_BUFFER),
      UsbKeyboardDevice->DevicePath
      );
    //
    // Clear the key buffer of this USB keyboard
    //
    InitQueue (&UsbKeyboardDevice->UsbKeyQueue, sizeof (USB_KEY));
    InitQueue (&UsbKeyboardDevice->EfiKeyQueue, sizeof (EFI_KEY_DATA));
    InitQueue (&UsbKeyboardDevice->EfiKeyQueueForNotify, sizeof (EFI_KEY_DATA));

    return EFI_SUCCESS;
  }

  //
  // Exhaustive reset
  //
  Status = InitUSBKeyboard (UsbKeyboardDevice);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Reads the next keystroke from the input device.

  @param  This                 The EFI_SIMPLE_TEXT_INPUT_PROTOCOL instance.
  @param  Key                  A pointer to a buffer that is filled in with the keystroke
                               information for the key that was pressed.

  @retval EFI_SUCCESS          The keystroke information was returned.
  @retval EFI_NOT_READY        There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR     The keystroke information was not returned due to
                               hardware errors.

**/
EFI_STATUS
EFIAPI
USBKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  OUT EFI_INPUT_KEY                    *Key
  )
{
  USB_KB_DEV   *UsbKeyboardDevice;
  EFI_STATUS   Status;
  EFI_KEY_DATA KeyData;

  UsbKeyboardDevice = USB_KB_DEV_FROM_THIS (This);

  //
  // Considering if the partial keystroke is enabled, there maybe a partial
  // keystroke in the queue, so here skip the partial keystroke and get the
  // next key from the queue
  //
  while (1) {
    Status = USBKeyboardReadKeyStrokeWorker (UsbKeyboardDevice, &KeyData);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // SimpleTextIn Protocol doesn't support partial keystroke;
    //
    if (KeyData.Key.ScanCode == CHAR_NULL && KeyData.Key.UnicodeChar == SCAN_NULL) {
      continue;
    }
    //
    // Translate the CTRL-Alpha characters to their corresponding control value
    // (ctrl-a = 0x0001 through ctrl-Z = 0x001A)
    //
    if ((KeyData.KeyState.KeyShiftState & (EFI_LEFT_CONTROL_PRESSED | EFI_RIGHT_CONTROL_PRESSED)) != 0) {
      if (KeyData.Key.UnicodeChar >= L'a' && KeyData.Key.UnicodeChar <= L'z') {
        KeyData.Key.UnicodeChar = (CHAR16) (KeyData.Key.UnicodeChar - L'a' + 1);
      } else if (KeyData.Key.UnicodeChar >= L'A' && KeyData.Key.UnicodeChar <= L'Z') {
        KeyData.Key.UnicodeChar = (CHAR16) (KeyData.Key.UnicodeChar - L'A' + 1);
      }
    }

    CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));
    return EFI_SUCCESS;
  }
}


/**
  Event notification function registered for EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL.WaitForKeyEx
  and EFI_SIMPLE_TEXT_INPUT_PROTOCOL.WaitForKey.

  @param  Event        Event to be signaled when a key is pressed.
  @param  Context      Points to USB_KB_DEV instance.

**/
VOID
EFIAPI
USBKeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  )
{
  USB_KB_DEV  *UsbKeyboardDevice;
  EFI_KEY_DATA KeyData;
  EFI_TPL      OldTpl;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  //
  // Enter critical section
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  
  //
  // WaitforKey doesn't suppor the partial key.
  // Considering if the partial keystroke is enabled, there maybe a partial
  // keystroke in the queue, so here skip the partial keystroke and get the
  // next key from the queue
  //
  while (!IsQueueEmpty (&UsbKeyboardDevice->EfiKeyQueue)) {
    //
    // If there is pending key, signal the event.
    //
    CopyMem (
      &KeyData,
      UsbKeyboardDevice->EfiKeyQueue.Buffer[UsbKeyboardDevice->EfiKeyQueue.Head],
      sizeof (EFI_KEY_DATA)
      );
    if (KeyData.Key.ScanCode == SCAN_NULL && KeyData.Key.UnicodeChar == CHAR_NULL) {
      Dequeue (&UsbKeyboardDevice->EfiKeyQueue, &KeyData, sizeof (EFI_KEY_DATA));
      continue;
    }
    gBS->SignalEvent (Event);
    break;
  }
  //
  // Leave critical section and return
  //
  gBS->RestoreTPL (OldTpl);
}

/**
  Timer handler to convert the key from USB.

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.
**/
VOID
EFIAPI
USBKeyboardTimerHandler (
  IN  EFI_EVENT                 Event,
  IN  VOID                      *Context
  )
{
  EFI_STATUS                    Status;
  USB_KB_DEV                    *UsbKeyboardDevice;
  UINT8                         KeyCode;
  EFI_KEY_DATA                  KeyData;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  //
  // Fetch raw data from the USB keyboard buffer,
  // and translate it into USB keycode.
  //
  Status = USBParseKey (UsbKeyboardDevice, &KeyCode);
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Translate saved USB keycode into EFI_INPUT_KEY
  //
  Status = UsbKeyCodeToEfiInputKey (UsbKeyboardDevice, KeyCode, &KeyData);
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Insert to the EFI Key queue
  //
  Enqueue (&UsbKeyboardDevice->EfiKeyQueue, &KeyData, sizeof (KeyData));
}

/**
  ExitBootServices handler to disconnect from the device.

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.
**/
VOID
EFIAPI
USBKeyboardExitBootServices (
  IN  EFI_EVENT                 Event,
  IN  VOID                      *Context
  )
{
  USB_KB_DEV                    *UsbKeyboardDevice;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  mExitingBootServices = TRUE;

  gBS->DisconnectController (
         UsbKeyboardDevice->ControllerHandle,
         gImageHandle,
         NULL
         );
}

/**
  Free keyboard notify list.

  @param  NotifyList              The keyboard notify list to free.

  @retval EFI_SUCCESS             Free the notify list successfully.
  @retval EFI_INVALID_PARAMETER   NotifyList is NULL.

**/
EFI_STATUS
KbdFreeNotifyList (
  IN OUT LIST_ENTRY           *NotifyList
  )
{
  KEYBOARD_CONSOLE_IN_EX_NOTIFY *NotifyNode;
  LIST_ENTRY                    *Link;

  if (NotifyList == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  while (!IsListEmpty (NotifyList)) {
    Link = GetFirstNode (NotifyList);
    NotifyNode = CR (Link, KEYBOARD_CONSOLE_IN_EX_NOTIFY, NotifyEntry, USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE);
    RemoveEntryList (Link);
    FreePool (NotifyNode);
  }

  return EFI_SUCCESS;
}

/**
  Check whether the pressed key matches a registered key or not.

  @param  RegsiteredData    A pointer to keystroke data for the key that was registered.
  @param  InputData         A pointer to keystroke data for the key that was pressed.

  @retval TRUE              Key pressed matches a registered key.
  @retval FLASE             Key pressed does not matches a registered key.

**/
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  )
{
  ASSERT (RegsiteredData != NULL && InputData != NULL);

  if ((RegsiteredData->Key.ScanCode    != InputData->Key.ScanCode) ||
      (RegsiteredData->Key.UnicodeChar != InputData->Key.UnicodeChar)) {
    return FALSE;
  }

  //
  // Assume KeyShiftState/KeyToggleState = 0 in Registered key data means these state could be ignored.
  //
  if (RegsiteredData->KeyState.KeyShiftState != 0 &&
      RegsiteredData->KeyState.KeyShiftState != InputData->KeyState.KeyShiftState) {
    return FALSE;
  }
  if (RegsiteredData->KeyState.KeyToggleState != 0 &&
      RegsiteredData->KeyState.KeyToggleState != InputData->KeyState.KeyToggleState) {
    return FALSE;
  }

  return TRUE;
}

//
// Simple Text Input Ex protocol functions
//
/**
  Resets the input device hardware.

  The Reset() function resets the input device hardware. As part
  of initialization process, the firmware/device will make a quick
  but reasonable attempt to verify that the device is functioning.
  If the ExtendedVerification flag is TRUE the firmware may take
  an extended amount of time to verify the device is operating on
  reset. Otherwise the reset operation is to occur as quickly as
  possible. The hardware verification process is not defined by
  this specification and is left up to the platform firmware or
  driver to implement.

  @param This                 A pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.

  @param ExtendedVerification Indicates that the driver may perform a more exhaustive
                              verification operation of the device during reset.

  @retval EFI_SUCCESS         The device was reset.
  @retval EFI_DEVICE_ERROR    The device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
USBKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  EFI_STATUS                Status;
  USB_KB_DEV                *UsbKeyboardDevice;

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  Status = UsbKeyboardDevice->SimpleInput.Reset (&UsbKeyboardDevice->SimpleInput, ExtendedVerification);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  UsbKeyboardDevice->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
  UsbKeyboardDevice->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;

  return EFI_SUCCESS;

}

/**
  Reads the next keystroke from the input device.

  @param  This                   Protocol instance pointer.
  @param  KeyData                A pointer to a buffer that is filled in with the keystroke
                                 state data for the key that was pressed.

  @retval EFI_SUCCESS            The keystroke information was returned.
  @retval EFI_NOT_READY          There was no keystroke data available.
  @retval EFI_DEVICE_ERROR       The keystroke information was not returned due to
                                 hardware errors.
  @retval EFI_INVALID_PARAMETER  KeyData is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  return USBKeyboardReadKeyStrokeWorker (UsbKeyboardDevice, KeyData);

}

/**
  Set certain state for the input device.

  @param  This                    Protocol instance pointer.
  @param  KeyToggleState          A pointer to the EFI_KEY_TOGGLE_STATE to set the
                                  state for the input device.

  @retval EFI_SUCCESS             The device state was set appropriately.
  @retval EFI_DEVICE_ERROR        The device is not functioning correctly and could
                                  not have the setting adjusted.
  @retval EFI_UNSUPPORTED         The device does not support the ability to have its state set.
  @retval EFI_INVALID_PARAMETER   KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;

  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  if (((UsbKeyboardDevice->KeyState.KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID) ||
      ((*KeyToggleState & EFI_TOGGLE_STATE_VALID) != EFI_TOGGLE_STATE_VALID)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Update the status light
  //

  UsbKeyboardDevice->ScrollOn   = FALSE;
  UsbKeyboardDevice->NumLockOn  = FALSE;
  UsbKeyboardDevice->CapsOn     = FALSE;
  UsbKeyboardDevice->IsSupportPartialKey = FALSE;

  if ((*KeyToggleState & EFI_SCROLL_LOCK_ACTIVE) == EFI_SCROLL_LOCK_ACTIVE) {
    UsbKeyboardDevice->ScrollOn = TRUE;
  }
  if ((*KeyToggleState & EFI_NUM_LOCK_ACTIVE) == EFI_NUM_LOCK_ACTIVE) {
    UsbKeyboardDevice->NumLockOn = TRUE;
  }
  if ((*KeyToggleState & EFI_CAPS_LOCK_ACTIVE) == EFI_CAPS_LOCK_ACTIVE) {
    UsbKeyboardDevice->CapsOn = TRUE;
  }
  if ((*KeyToggleState & EFI_KEY_STATE_EXPOSED) == EFI_KEY_STATE_EXPOSED) {
    UsbKeyboardDevice->IsSupportPartialKey = TRUE;
  }

  SetKeyLED (UsbKeyboardDevice);

  UsbKeyboardDevice->KeyState.KeyToggleState = *KeyToggleState;

  return EFI_SUCCESS;

}

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                        Protocol instance pointer.
  @param  KeyData                     A pointer to a buffer that is filled in with
                                      the keystroke information for the key that was
                                      pressed. If KeyData.Key, KeyData.KeyState.KeyToggleState
                                      and KeyData.KeyState.KeyShiftState are 0, then any incomplete
                                      keystroke will trigger a notification of the KeyNotificationFunction.
  @param  KeyNotificationFunction     Points to the function to be called when the key
                                      sequence is typed specified by KeyData. This notification function
                                      should be called at <=TPL_CALLBACK.
  @param  NotifyHandle                Points to the unique handle assigned to the registered notification.

  @retval EFI_SUCCESS                 The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES        Unable to allocate resources for necessary data structures.
  @retval EFI_INVALID_PARAMETER       KeyData or NotifyHandle or KeyNotificationFunction is NULL.

**/
EFI_STATUS
EFIAPI
USBKeyboardRegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                               **NotifyHandle
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *NewNotify;
  LIST_ENTRY                        *Link;
  LIST_ENTRY                        *NotifyList;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *CurrentNotify;

  if (KeyData == NULL || NotifyHandle == NULL || KeyNotificationFunction == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  //
  // Return EFI_SUCCESS if the (KeyData, NotificationFunction) is already registered.
  //
  NotifyList = &UsbKeyboardDevice->NotifyList;

  for (Link = GetFirstNode (NotifyList);
       !IsNull (NotifyList, Link);
       Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (
                      Link,
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (IsKeyRegistered (&CurrentNotify->KeyData, KeyData)) {
      if (CurrentNotify->KeyNotificationFn == KeyNotificationFunction) {
        *NotifyHandle = CurrentNotify;
        return EFI_SUCCESS;
      }
    }
  }

  //
  // Allocate resource to save the notification function
  //
  NewNotify = (KEYBOARD_CONSOLE_IN_EX_NOTIFY *) AllocateZeroPool (sizeof (KEYBOARD_CONSOLE_IN_EX_NOTIFY));
  if (NewNotify == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewNotify->Signature         = USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE;
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));
  InsertTailList (&UsbKeyboardDevice->NotifyList, &NewNotify->NotifyEntry);


  *NotifyHandle = NewNotify;

  return EFI_SUCCESS;

}

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                      Protocol instance pointer.
  @param  NotificationHandle        The handle of the notification function being unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid

**/
EFI_STATUS
EFIAPI
USBKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  USB_KB_DEV                        *UsbKeyboardDevice;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY     *CurrentNotify;
  LIST_ENTRY                        *Link;
  LIST_ENTRY                        *NotifyList;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbKeyboardDevice = TEXT_INPUT_EX_USB_KB_DEV_FROM_THIS (This);

  //
  // Traverse notify list of USB keyboard and remove the entry of NotificationHandle.
  //
  NotifyList = &UsbKeyboardDevice->NotifyList;
  for (Link = GetFirstNode (NotifyList);
       !IsNull (NotifyList, Link);
       Link = GetNextNode (NotifyList, Link)) {
    CurrentNotify = CR (
                      Link,
                      KEYBOARD_CONSOLE_IN_EX_NOTIFY,
                      NotifyEntry,
                      USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE
                      );
    if (CurrentNotify == NotificationHandle) {
      //
      // Remove the notification function from NotifyList and free resources
      //
      RemoveEntryList (&CurrentNotify->NotifyEntry);

      FreePool (CurrentNotify);
      return EFI_SUCCESS;
    }
  }

  //
  // Cannot find the matching entry in database.
  //
  return EFI_INVALID_PARAMETER;
}

/**
  Process key notify.
  @param  Event                 Indicates the event that invoke this function.
  @param  Context               Indicates the calling context.
**/
VOID
EFIAPI
KeyNotifyProcessHandler (
  IN  EFI_EVENT                 Event,
  IN  VOID                      *Context
  )
{
  EFI_STATUS                    Status;
  USB_KB_DEV                    *UsbKeyboardDevice;
  EFI_KEY_DATA                  KeyData;
  LIST_ENTRY                    *Link;
  LIST_ENTRY                    *NotifyList;
  KEYBOARD_CONSOLE_IN_EX_NOTIFY *CurrentNotify;
  EFI_TPL                       OldTpl;

  UsbKeyboardDevice = (USB_KB_DEV *) Context;

  //
  // Invoke notification functions.
  //
  NotifyList = &UsbKeyboardDevice->NotifyList;
  while (TRUE) {
    //
    // Enter critical section
    //
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    Status = Dequeue (&UsbKeyboardDevice->EfiKeyQueueForNotify, &KeyData, sizeof (KeyData));
    //
    // Leave critical section
    //
    gBS->RestoreTPL (OldTpl);
    if (EFI_ERROR (Status)) {
      break;
    }
    for (Link = GetFirstNode (NotifyList); !IsNull (NotifyList, Link); Link = GetNextNode (NotifyList, Link)) {
      CurrentNotify = CR (Link, KEYBOARD_CONSOLE_IN_EX_NOTIFY, NotifyEntry, USB_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE);
      if (IsKeyRegistered (&CurrentNotify->KeyData, &KeyData)) {
        CurrentNotify->KeyNotificationFn (&KeyData);
      }
    }
  }
}
