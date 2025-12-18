/** @file
  Console Splitter Driver. Any Handle that attached console I/O protocols
  (Console In device, Console Out device, Console Error device, Simple Pointer
  protocol, Absolute Pointer protocol) can be bound by this driver.

  So far it works like any other driver by opening a SimpleTextIn and/or
  SimpleTextOut protocol with EFI_OPEN_PROTOCOL_BY_DRIVER attributes. The big
  difference is this driver does not layer a protocol on the passed in
  handle, or construct a child handle like a standard device or bus driver.
  This driver produces three virtual handles as children, one for console input
  splitter, one for console output splitter and one for error output splitter.
  These 3 virtual handles would be installed on gST.

  Each virtual handle, that supports the Console I/O protocol, will be produced
  in the driver entry point. The virtual handle are added on driver entry and
  never removed. Such design ensures system function well during none console
  device situation.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ConSplitter.h"

//
// Identify if ConIn is connected in PcdConInConnectOnDemand enabled mode.
// default not connect
//
BOOLEAN  mConInIsConnect = FALSE;

//
// Text In Splitter Private Data template
//
GLOBAL_REMOVE_IF_UNREFERENCED TEXT_IN_SPLITTER_PRIVATE_DATA  mConIn = {
  TEXT_IN_SPLITTER_PRIVATE_DATA_SIGNATURE,
  (EFI_HANDLE)NULL,

  {
    ConSplitterTextInReset,
    ConSplitterTextInReadKeyStroke,
    (EFI_EVENT)NULL
  },
  0,
  (EFI_SIMPLE_TEXT_INPUT_PROTOCOL **)NULL,
  0,

  {
    ConSplitterTextInResetEx,
    ConSplitterTextInReadKeyStrokeEx,
    (EFI_EVENT)NULL,
    ConSplitterTextInSetState,
    ConSplitterTextInRegisterKeyNotify,
    ConSplitterTextInUnregisterKeyNotify
  },
  0,
  (EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL **)NULL,
  0,
  {
    (LIST_ENTRY *)NULL,
    (LIST_ENTRY *)NULL
  },
  (EFI_KEY_DATA *)NULL,
  0,
  0,
  FALSE,

  {
    ConSplitterSimplePointerReset,
    ConSplitterSimplePointerGetState,
    (EFI_EVENT)NULL,
    (EFI_SIMPLE_POINTER_MODE *)NULL
  },
  {
    0x10000,
    0x10000,
    0x10000,
    TRUE,
    TRUE
  },
  0,
  (EFI_SIMPLE_POINTER_PROTOCOL **)NULL,
  0,

  {
    ConSplitterAbsolutePointerReset,
    ConSplitterAbsolutePointerGetState,
    (EFI_EVENT)NULL,
    (EFI_ABSOLUTE_POINTER_MODE *)NULL
  },
  {
    0,       // AbsoluteMinX
    0,       // AbsoluteMinY
    0,       // AbsoluteMinZ
    0x10000, // AbsoluteMaxX
    0x10000, // AbsoluteMaxY
    0x10000, // AbsoluteMaxZ
    0        // Attributes
  },
  0,
  (EFI_ABSOLUTE_POINTER_PROTOCOL **)NULL,
  0,
  FALSE,

  FALSE,
  FALSE
};

//
// Uga Draw Protocol Private Data template
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UGA_DRAW_PROTOCOL  mUgaDrawProtocolTemplate = {
  ConSplitterUgaDrawGetMode,
  ConSplitterUgaDrawSetMode,
  ConSplitterUgaDrawBlt
};

//
// Graphics Output Protocol Private Data template
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_GRAPHICS_OUTPUT_PROTOCOL  mGraphicsOutputProtocolTemplate = {
  ConSplitterGraphicsOutputQueryMode,
  ConSplitterGraphicsOutputSetMode,
  ConSplitterGraphicsOutputBlt,
  NULL
};

//
// Text Out Splitter Private Data template
//
GLOBAL_REMOVE_IF_UNREFERENCED TEXT_OUT_SPLITTER_PRIVATE_DATA  mConOut = {
  TEXT_OUT_SPLITTER_PRIVATE_DATA_SIGNATURE,
  (EFI_HANDLE)NULL,
  {
    ConSplitterTextOutReset,
    ConSplitterTextOutOutputString,
    ConSplitterTextOutTestString,
    ConSplitterTextOutQueryMode,
    ConSplitterTextOutSetMode,
    ConSplitterTextOutSetAttribute,
    ConSplitterTextOutClearScreen,
    ConSplitterTextOutSetCursorPosition,
    ConSplitterTextOutEnableCursor,
    (EFI_SIMPLE_TEXT_OUTPUT_MODE *)NULL
  },
  {
    1,
    0,
    0,
    0,
    0,
    FALSE,
  },

  {
    NULL,
    NULL,
    NULL
  },
  0,
  0,
  0,
  0,

  {
    NULL,
    NULL,
    NULL,
    NULL
  },
  (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *)NULL,
  0,
  0,

  0,
  (TEXT_OUT_AND_GOP_DATA *)NULL,
  0,
  (TEXT_OUT_SPLITTER_QUERY_DATA *)NULL,
  0,
  (INT32 *)NULL,
  FALSE
};

//
// Standard Error Text Out Splitter Data Template
//
GLOBAL_REMOVE_IF_UNREFERENCED TEXT_OUT_SPLITTER_PRIVATE_DATA  mStdErr = {
  TEXT_OUT_SPLITTER_PRIVATE_DATA_SIGNATURE,
  (EFI_HANDLE)NULL,
  {
    ConSplitterTextOutReset,
    ConSplitterTextOutOutputString,
    ConSplitterTextOutTestString,
    ConSplitterTextOutQueryMode,
    ConSplitterTextOutSetMode,
    ConSplitterTextOutSetAttribute,
    ConSplitterTextOutClearScreen,
    ConSplitterTextOutSetCursorPosition,
    ConSplitterTextOutEnableCursor,
    (EFI_SIMPLE_TEXT_OUTPUT_MODE *)NULL
  },
  {
    1,
    0,
    0,
    0,
    0,
    FALSE,
  },

  {
    NULL,
    NULL,
    NULL
  },
  0,
  0,
  0,
  0,

  {
    NULL,
    NULL,
    NULL,
    NULL
  },
  (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *)NULL,
  0,
  0,

  0,
  (TEXT_OUT_AND_GOP_DATA *)NULL,
  0,
  (TEXT_OUT_SPLITTER_QUERY_DATA *)NULL,
  0,
  (INT32 *)NULL,
  FALSE
};

//
// Driver binding instance for Console Input Device
//
EFI_DRIVER_BINDING_PROTOCOL  gConSplitterConInDriverBinding = {
  ConSplitterConInDriverBindingSupported,
  ConSplitterConInDriverBindingStart,
  ConSplitterConInDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Driver binding instance for Console Out device
//
EFI_DRIVER_BINDING_PROTOCOL  gConSplitterConOutDriverBinding = {
  ConSplitterConOutDriverBindingSupported,
  ConSplitterConOutDriverBindingStart,
  ConSplitterConOutDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Driver binding instance for Standard Error device
//
EFI_DRIVER_BINDING_PROTOCOL  gConSplitterStdErrDriverBinding = {
  ConSplitterStdErrDriverBindingSupported,
  ConSplitterStdErrDriverBindingStart,
  ConSplitterStdErrDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Driver binding instance for Simple Pointer protocol
//
EFI_DRIVER_BINDING_PROTOCOL  gConSplitterSimplePointerDriverBinding = {
  ConSplitterSimplePointerDriverBindingSupported,
  ConSplitterSimplePointerDriverBindingStart,
  ConSplitterSimplePointerDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Driver binding instance for Absolute Pointer protocol
//
EFI_DRIVER_BINDING_PROTOCOL  gConSplitterAbsolutePointerDriverBinding = {
  ConSplitterAbsolutePointerDriverBindingSupported,
  ConSplitterAbsolutePointerDriverBindingStart,
  ConSplitterAbsolutePointerDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  Key notify for toggle state sync.

  @param KeyData        A pointer to a buffer that is filled in with
                        the keystroke information for the key that was
                        pressed.

  @retval EFI_SUCCESS   Toggle state sync successfully.

**/
EFI_STATUS
EFIAPI
ToggleStateSyncKeyNotify (
  IN EFI_KEY_DATA  *KeyData
  )
{
  UINTN  Index;

  if (((KeyData->KeyState.KeyToggleState & KEY_STATE_VALID_EXPOSED) == KEY_STATE_VALID_EXPOSED) &&
      (KeyData->KeyState.KeyToggleState != mConIn.PhysicalKeyToggleState))
  {
    //
    // There is toggle state change, sync to other console input devices.
    //
    for (Index = 0; Index < mConIn.CurrentNumberOfExConsoles; Index++) {
      mConIn.TextInExList[Index]->SetState (
                                    mConIn.TextInExList[Index],
                                    &KeyData->KeyState.KeyToggleState
                                    );
    }

    mConIn.PhysicalKeyToggleState = KeyData->KeyState.KeyToggleState;
    DEBUG ((DEBUG_INFO, "Current toggle state is 0x%02x\n", mConIn.PhysicalKeyToggleState));
  }

  return EFI_SUCCESS;
}

/**
  Initialization for toggle state sync.

  @param Private                    Text In Splitter pointer.

**/
VOID
ToggleStateSyncInitialization (
  IN TEXT_IN_SPLITTER_PRIVATE_DATA  *Private
  )
{
  EFI_KEY_DATA  KeyData;
  VOID          *NotifyHandle;

  //
  // Initialize PhysicalKeyToggleState that will be synced to new console
  // input device to turn on physical TextInEx partial key report for
  // toggle state sync.
  //
  Private->PhysicalKeyToggleState = KEY_STATE_VALID_EXPOSED;

  //
  // Initialize VirtualKeyStateExported to let the virtual TextInEx not report
  // the partial key even though the physical TextInEx turns on the partial
  // key report. The virtual TextInEx will report the partial key after it is
  // required by calling SetState(X | KEY_STATE_VALID_EXPOSED) explicitly.
  //
  Private->VirtualKeyStateExported = FALSE;

  //
  // Register key notify for toggle state sync.
  //
  KeyData.Key.ScanCode            = SCAN_NULL;
  KeyData.Key.UnicodeChar         = CHAR_NULL;
  KeyData.KeyState.KeyShiftState  = 0;
  KeyData.KeyState.KeyToggleState = 0;
  Private->TextInEx.RegisterKeyNotify (
                      &Private->TextInEx,
                      &KeyData,
                      ToggleStateSyncKeyNotify,
                      &NotifyHandle
                      );
}

/**
  Re-initialization for toggle state sync.

  @param Private                    Text In Splitter pointer.

**/
VOID
ToggleStateSyncReInitialization (
  IN TEXT_IN_SPLITTER_PRIVATE_DATA  *Private
  )
{
  UINTN  Index;

  //
  // Reinitialize PhysicalKeyToggleState that will be synced to new console
  // input device to turn on physical TextInEx partial key report for
  // toggle state sync.
  //
  Private->PhysicalKeyToggleState = KEY_STATE_VALID_EXPOSED;

  //
  // Reinitialize VirtualKeyStateExported to let the virtual TextInEx not report
  // the partial key even though the physical TextInEx turns on the partial
  // key report. The virtual TextInEx will report the partial key after it is
  // required by calling SetState(X | KEY_STATE_VALID_EXPOSED) explicitly.
  //
  Private->VirtualKeyStateExported = FALSE;

  for (Index = 0; Index < Private->CurrentNumberOfExConsoles; Index++) {
    Private->TextInExList[Index]->SetState (
                                    Private->TextInExList[Index],
                                    &Private->PhysicalKeyToggleState
                                    );
  }
}

/**
  The Entry Point for module ConSplitter. The user code starts with this function.

  Installs driver module protocols and. Creates virtual device handles for ConIn,
  ConOut, and StdErr. Installs Simple Text In protocol, Simple Text In Ex protocol,
  Simple Pointer protocol, Absolute Pointer protocol on those virtual handlers.
  Installs Graphics Output protocol and/or UGA Draw protocol if needed.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
ConSplitterDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gConSplitterConInDriverBinding,
             ImageHandle,
             &gConSplitterConInComponentName,
             &gConSplitterConInComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gConSplitterSimplePointerDriverBinding,
             NULL,
             &gConSplitterSimplePointerComponentName,
             &gConSplitterSimplePointerComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gConSplitterAbsolutePointerDriverBinding,
             NULL,
             &gConSplitterAbsolutePointerComponentName,
             &gConSplitterAbsolutePointerComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gConSplitterConOutDriverBinding,
             NULL,
             &gConSplitterConOutComponentName,
             &gConSplitterConOutComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gConSplitterStdErrDriverBinding,
             NULL,
             &gConSplitterStdErrComponentName,
             &gConSplitterStdErrComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Either Graphics Output protocol or UGA Draw protocol must be supported.
  //
  ASSERT (
    FeaturePcdGet (PcdConOutGopSupport) ||
    FeaturePcdGet (PcdConOutUgaSupport)
    );

  //
  // The driver creates virtual handles for ConIn, ConOut, StdErr.
  // The virtual handles will always exist even if no console exist in the
  // system. This is need to support hotplug devices like USB.
  //
  //
  // Create virtual device handle for ConIn Splitter
  //
  Status = ConSplitterTextInConstructor (&mConIn);
  if (!EFI_ERROR (Status)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mConIn.VirtualHandle,
                    &gEfiSimpleTextInProtocolGuid,
                    &mConIn.TextIn,
                    &gEfiSimpleTextInputExProtocolGuid,
                    &mConIn.TextInEx,
                    &gEfiSimplePointerProtocolGuid,
                    &mConIn.SimplePointer,
                    &gEfiAbsolutePointerProtocolGuid,
                    &mConIn.AbsolutePointer,
                    NULL
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Update the EFI System Table with new virtual console
      // and update the pointer to Simple Text Input protocol.
      //
      gST->ConsoleInHandle = mConIn.VirtualHandle;
      gST->ConIn           = &mConIn.TextIn;
    }
  }

  //
  // Create virtual device handle for ConOut Splitter
  //
  Status = ConSplitterTextOutConstructor (&mConOut);
  if (!EFI_ERROR (Status)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mConOut.VirtualHandle,
                    &gEfiSimpleTextOutProtocolGuid,
                    &mConOut.TextOut,
                    NULL
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Update the EFI System Table with new virtual console
      // and Update the pointer to Text Output protocol.
      //
      gST->ConsoleOutHandle = mConOut.VirtualHandle;
      gST->ConOut           = &mConOut.TextOut;
    }
  }

  //
  // Create virtual device handle for StdErr Splitter
  //
  Status = ConSplitterTextOutConstructor (&mStdErr);
  if (!EFI_ERROR (Status)) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mStdErr.VirtualHandle,
                    &gEfiSimpleTextOutProtocolGuid,
                    &mStdErr.TextOut,
                    NULL
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Update the EFI System Table with new virtual console
      // and update the pointer to Text Output protocol.
      //
      gST->StandardErrorHandle = mStdErr.VirtualHandle;
      gST->StdErr              = &mStdErr.TextOut;
    }
  }

  //
  // Update the CRC32 in the EFI System Table header
  //
  gST->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 (
         (UINT8 *)&gST->Hdr,
         gST->Hdr.HeaderSize,
         &gST->Hdr.CRC32
         );

  return EFI_SUCCESS;
}

/**
  Construct console input devices' private data.

  @param  ConInPrivate             A pointer to the TEXT_IN_SPLITTER_PRIVATE_DATA
                                   structure.

  @retval EFI_OUT_OF_RESOURCES     Out of resources.
  @retval EFI_SUCCESS              Text Input Device's private data has been constructed.
  @retval other                    Failed to construct private data.

**/
EFI_STATUS
ConSplitterTextInConstructor (
  TEXT_IN_SPLITTER_PRIVATE_DATA  *ConInPrivate
  )
{
  EFI_STATUS  Status;
  UINTN       TextInExListCount;

  //
  // Allocate buffer for Simple Text Input device
  //
  Status = ConSplitterGrowBuffer (
             sizeof (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *),
             &ConInPrivate->TextInListCount,
             (VOID **)&ConInPrivate->TextInList
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create Event to wait for a key
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  ConSplitterTextInWaitForKey,
                  ConInPrivate,
                  &ConInPrivate->TextIn.WaitForKey
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate buffer for KeyQueue
  //
  TextInExListCount = ConInPrivate->TextInExListCount;
  Status            = ConSplitterGrowBuffer (
                        sizeof (EFI_KEY_DATA),
                        &TextInExListCount,
                        (VOID **)&ConInPrivate->KeyQueue
                        );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Allocate buffer for Simple Text Input Ex device
  //
  Status = ConSplitterGrowBuffer (
             sizeof (EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *),
             &ConInPrivate->TextInExListCount,
             (VOID **)&ConInPrivate->TextInExList
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create Event to wait for a key Ex
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  ConSplitterTextInWaitForKey,
                  ConInPrivate,
                  &ConInPrivate->TextInEx.WaitForKeyEx
                  );
  ASSERT_EFI_ERROR (Status);

  InitializeListHead (&ConInPrivate->NotifyList);

  ToggleStateSyncInitialization (ConInPrivate);

  ConInPrivate->AbsolutePointer.Mode = &ConInPrivate->AbsolutePointerMode;
  //
  // Allocate buffer for Absolute Pointer device
  //
  Status = ConSplitterGrowBuffer (
             sizeof (EFI_ABSOLUTE_POINTER_PROTOCOL *),
             &ConInPrivate->AbsolutePointerListCount,
             (VOID **)&ConInPrivate->AbsolutePointerList
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create Event to wait for device input for Absolute pointer device
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  ConSplitterAbsolutePointerWaitForInput,
                  ConInPrivate,
                  &ConInPrivate->AbsolutePointer.WaitForInput
                  );
  ASSERT_EFI_ERROR (Status);

  ConInPrivate->SimplePointer.Mode = &ConInPrivate->SimplePointerMode;
  //
  // Allocate buffer for Simple Pointer device
  //
  Status = ConSplitterGrowBuffer (
             sizeof (EFI_SIMPLE_POINTER_PROTOCOL *),
             &ConInPrivate->PointerListCount,
             (VOID **)&ConInPrivate->PointerList
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create Event to wait for device input for Simple pointer device
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  ConSplitterSimplePointerWaitForInput,
                  ConInPrivate,
                  &ConInPrivate->SimplePointer.WaitForInput
                  );
  ASSERT_EFI_ERROR (Status);
  //
  // Create Event to signal ConIn connection request
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  EfiEventEmptyFunction,
                  NULL,
                  &gConnectConInEventGuid,
                  &ConInPrivate->ConnectConInEvent
                  );

  return Status;
}

/**
  Construct console output devices' private data.

  @param  ConOutPrivate            A pointer to the TEXT_OUT_SPLITTER_PRIVATE_DATA
                                   structure.

  @retval EFI_OUT_OF_RESOURCES     Out of resources.
  @retval EFI_SUCCESS              Text Input Devcie's private data has been constructed.

**/
EFI_STATUS
ConSplitterTextOutConstructor (
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *ConOutPrivate
  )
{
  EFI_STATUS                            Status;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;

  //
  // Copy protocols template
  //
  if (FeaturePcdGet (PcdConOutUgaSupport)) {
    CopyMem (&ConOutPrivate->UgaDraw, &mUgaDrawProtocolTemplate, sizeof (EFI_UGA_DRAW_PROTOCOL));
  }

  if (FeaturePcdGet (PcdConOutGopSupport)) {
    CopyMem (&ConOutPrivate->GraphicsOutput, &mGraphicsOutputProtocolTemplate, sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL));
  }

  //
  // Initialize console output splitter's private data.
  //
  ConOutPrivate->TextOut.Mode = &ConOutPrivate->TextOutMode;

  //
  // When new console device is added, the new mode will be set later,
  // so put current mode back to init state.
  //
  ConOutPrivate->TextOutMode.Mode = 0xFF;
  //
  // Allocate buffer for Console Out device
  //
  Status = ConSplitterGrowBuffer (
             sizeof (TEXT_OUT_AND_GOP_DATA),
             &ConOutPrivate->TextOutListCount,
             (VOID **)&ConOutPrivate->TextOutList
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Allocate buffer for Text Out query data
  //
  Status = ConSplitterGrowBuffer (
             sizeof (TEXT_OUT_SPLITTER_QUERY_DATA),
             &ConOutPrivate->TextOutQueryDataCount,
             (VOID **)&ConOutPrivate->TextOutQueryData
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Setup the default console to 80 x 25 and mode to 0
  //
  ConOutPrivate->TextOutQueryData[0].Columns = 80;
  ConOutPrivate->TextOutQueryData[0].Rows    = 25;
  TextOutSetMode (ConOutPrivate, 0);

  if (FeaturePcdGet (PcdConOutUgaSupport)) {
    //
    // Setup the UgaDraw to 800 x 600 x 32 bits per pixel, 60Hz.
    //
    ConSplitterUgaDrawSetMode (&ConOutPrivate->UgaDraw, 800, 600, 32, 60);
  }

  if (FeaturePcdGet (PcdConOutGopSupport)) {
    //
    // Setup resource for mode information in Graphics Output Protocol interface
    //
    if ((ConOutPrivate->GraphicsOutput.Mode = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE))) == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if ((ConOutPrivate->GraphicsOutput.Mode->Info = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION))) == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Setup the DevNullGraphicsOutput to 800 x 600 x 32 bits per pixel
    // DevNull will be updated to user-defined mode after driver has started.
    //
    if ((ConOutPrivate->GraphicsOutputModeBuffer = AllocateZeroPool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION))) == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Info                       = &ConOutPrivate->GraphicsOutputModeBuffer[0];
    Info->Version              = 0;
    Info->HorizontalResolution = 800;
    Info->VerticalResolution   = 600;
    Info->PixelFormat          = PixelBltOnly;
    Info->PixelsPerScanLine    = 800;
    CopyMem (ConOutPrivate->GraphicsOutput.Mode->Info, Info, sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
    ConOutPrivate->GraphicsOutput.Mode->SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

    //
    // Initialize the following items, theset items remain unchanged in GraphicsOutput->SetMode()
    // GraphicsOutputMode->FrameBufferBase, GraphicsOutputMode->FrameBufferSize
    //
    ConOutPrivate->GraphicsOutput.Mode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS)(UINTN)NULL;
    ConOutPrivate->GraphicsOutput.Mode->FrameBufferSize = 0;

    ConOutPrivate->GraphicsOutput.Mode->MaxMode = 1;
    //
    // Initial current mode to unknown state, and then set to mode 0
    //
    ConOutPrivate->GraphicsOutput.Mode->Mode = 0xffff;
    ConOutPrivate->GraphicsOutput.SetMode (&ConOutPrivate->GraphicsOutput, 0);
  }

  return EFI_SUCCESS;
}

/**
  Test to see if the specified protocol could be supported on the specified device.

  @param  This                Driver Binding protocol pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  Guid                The specified protocol.

  @retval EFI_SUCCESS         The specified protocol is supported on this device.
  @retval EFI_UNSUPPORTED     The specified protocol attempts to be installed on virtual handle.
  @retval other               Failed to open specified protocol on this device.

**/
EFI_STATUS
ConSplitterSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_GUID                     *Guid
  )
{
  EFI_STATUS  Status;
  VOID        *Instance;

  //
  // Make sure the Console Splitter does not attempt to attach to itself
  //
  if ((ControllerHandle == mConIn.VirtualHandle) ||
      (ControllerHandle == mConOut.VirtualHandle) ||
      (ControllerHandle == mStdErr.VirtualHandle)
      )
  {
    return EFI_UNSUPPORTED;
  }

  //
  // Check to see whether the specific protocol could be opened BY_DRIVER
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  Guid,
                  &Instance,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         ControllerHandle,
         Guid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return EFI_SUCCESS;
}

/**
  Test to see if Console In Device could be supported on the Controller.

  @param  This                Driver Binding protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterConInDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  return ConSplitterSupported (
           This,
           ControllerHandle,
           &gEfiConsoleInDeviceGuid
           );
}

/**
  Test to see if Simple Pointer protocol could be supported on the Controller.

  @param  This                Driver Binding protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterSimplePointerDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  return ConSplitterSupported (
           This,
           ControllerHandle,
           &gEfiSimplePointerProtocolGuid
           );
}

/**
  Test to see if Absolute Pointer protocol could be supported on the Controller.

  @param  This                Driver Binding protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterAbsolutePointerDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  return ConSplitterSupported (
           This,
           ControllerHandle,
           &gEfiAbsolutePointerProtocolGuid
           );
}

/**
  Test to see if Console Out Device could be supported on the Controller.

  @param  This                Driver Binding protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterConOutDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  return ConSplitterSupported (
           This,
           ControllerHandle,
           &gEfiConsoleOutDeviceGuid
           );
}

/**
  Test to see if Standard Error Device could be supported on the Controller.

  @param  This                Driver Binding protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterStdErrDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  return ConSplitterSupported (
           This,
           ControllerHandle,
           &gEfiStandardErrorDeviceGuid
           );
}

/**
  Start ConSplitter on devcie handle by opening Console Device Guid on device handle
  and the console virtual handle. And Get the console interface on controller handle.

  @param  This                      Driver Binding protocol instance pointer.
  @param  ControllerHandle          Handle of device.
  @param  ConSplitterVirtualHandle  Console virtual Handle.
  @param  DeviceGuid                The specified Console Device, such as ConInDev,
                                    ConOutDev.
  @param  InterfaceGuid             The specified protocol to be opened.
  @param  Interface                 Protocol interface returned.

  @retval EFI_SUCCESS               This driver supports this device.
  @retval other                     Failed to open the specified Console Device Guid
                                    or specified protocol.

**/
EFI_STATUS
ConSplitterStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ConSplitterVirtualHandle,
  IN  EFI_GUID                     *DeviceGuid,
  IN  EFI_GUID                     *InterfaceGuid,
  OUT VOID                         **Interface
  )
{
  EFI_STATUS  Status;
  VOID        *Instance;

  //
  // Check to see whether the ControllerHandle has the DeviceGuid on it.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  DeviceGuid,
                  &Instance,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the Parent Handle for the child.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  DeviceGuid,
                  &Instance,
                  This->DriverBindingHandle,
                  ConSplitterVirtualHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto Err;
  }

  //
  // Open InterfaceGuid on the virtual handle.
  //
  Status =  gBS->OpenProtocol (
                   ControllerHandle,
                   InterfaceGuid,
                   Interface,
                   This->DriverBindingHandle,
                   ConSplitterVirtualHandle,
                   EFI_OPEN_PROTOCOL_GET_PROTOCOL
                   );

  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  //
  // close the DeviceGuid on ConSplitter VirtualHandle.
  //
  gBS->CloseProtocol (
         ControllerHandle,
         DeviceGuid,
         This->DriverBindingHandle,
         ConSplitterVirtualHandle
         );

Err:
  //
  // close the DeviceGuid on ControllerHandle.
  //
  gBS->CloseProtocol (
         ControllerHandle,
         DeviceGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return Status;
}

/**
  Start Console In Consplitter on device handle.

  @param  This                 Driver Binding protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          Console In Consplitter is added to ControllerHandle.
  @retval other                Console In Consplitter does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterConInDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL     *TextIn;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TextInEx;

  //
  // Start ConSplitter on ControllerHandle, and create the virtual
  // aggregated console device on first call Start for a SimpleTextIn handle.
  //
  Status = ConSplitterStart (
             This,
             ControllerHandle,
             mConIn.VirtualHandle,
             &gEfiConsoleInDeviceGuid,
             &gEfiSimpleTextInProtocolGuid,
             (VOID **)&TextIn
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Add this device into Text In devices list.
  //
  Status = ConSplitterTextInAddDevice (&mConIn, TextIn);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&TextInEx,
                  This->DriverBindingHandle,
                  mConIn.VirtualHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // If Simple Text Input Ex protocol exists,
    // add this device into Text In Ex devices list.
    //
    Status = ConSplitterTextInExAddDevice (&mConIn, TextInEx);
  }

  return Status;
}

/**
  Start Simple Pointer Consplitter on device handle.

  @param  This                 Driver Binding protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          Simple Pointer Consplitter is added to ControllerHandle.
  @retval other                Simple Pointer Consplitter does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterSimplePointerDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_POINTER_PROTOCOL  *SimplePointer;

  //
  // Start ConSplitter on ControllerHandle, and create the virtual
  // aggregated console device on first call Start for a SimplePointer handle.
  //
  Status = ConSplitterStart (
             This,
             ControllerHandle,
             mConIn.VirtualHandle,
             &gEfiSimplePointerProtocolGuid,
             &gEfiSimplePointerProtocolGuid,
             (VOID **)&SimplePointer
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Add this devcie into Simple Pointer devices list.
  //
  return ConSplitterSimplePointerAddDevice (&mConIn, SimplePointer);
}

/**
  Start Absolute Pointer Consplitter on device handle.

  @param  This                 Driver Binding protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          Absolute Pointer Consplitter is added to ControllerHandle.
  @retval other                Absolute Pointer Consplitter does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterAbsolutePointerDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                     Status;
  EFI_ABSOLUTE_POINTER_PROTOCOL  *AbsolutePointer;

  //
  // Start ConSplitter on ControllerHandle, and create the virtual
  // aggregated console device on first call Start for a AbsolutePointer handle.
  //
  Status = ConSplitterStart (
             This,
             ControllerHandle,
             mConIn.VirtualHandle,
             &gEfiAbsolutePointerProtocolGuid,
             &gEfiAbsolutePointerProtocolGuid,
             (VOID **)&AbsolutePointer
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Add this devcie into Absolute Pointer devices list.
  //
  return ConSplitterAbsolutePointerAddDevice (&mConIn, AbsolutePointer);
}

/**
  Start Console Out Consplitter on device handle.

  @param  This                 Driver Binding protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          Console Out Consplitter is added to ControllerHandle.
  @retval other                Console Out Consplitter does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterConOutDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                            Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL       *TextOut;
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL                 *UgaDraw;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;

  //
  // Start ConSplitter on ControllerHandle, and create the virtual
  // aggregated console device on first call Start for a ConsoleOut handle.
  //
  Status = ConSplitterStart (
             This,
             ControllerHandle,
             mConOut.VirtualHandle,
             &gEfiConsoleOutDeviceGuid,
             &gEfiSimpleTextOutProtocolGuid,
             (VOID **)&TextOut
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GraphicsOutput = NULL;
  UgaDraw        = NULL;
  //
  // Try to Open Graphics Output protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput,
                  This->DriverBindingHandle,
                  mConOut.VirtualHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    //
    // Open UGA DRAW protocol
    //
    gBS->OpenProtocol (
           ControllerHandle,
           &gEfiUgaDrawProtocolGuid,
           (VOID **)&UgaDraw,
           This->DriverBindingHandle,
           mConOut.VirtualHandle,
           EFI_OPEN_PROTOCOL_GET_PROTOCOL
           );
  }

  //
  // When new console device is added, the new mode will be set later,
  // so put current mode back to init state.
  //
  mConOut.TextOutMode.Mode = 0xFF;

  //
  // If both ConOut and StdErr incorporate the same Text Out device,
  // their MaxMode and QueryData should be the intersection of both.
  //
  Status = ConSplitterTextOutAddDevice (&mConOut, TextOut, GraphicsOutput, UgaDraw);
  ConSplitterTextOutSetAttribute (&mConOut.TextOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));

  if (FeaturePcdGet (PcdConOutUgaSupport)) {
    //
    // Get the UGA mode data of ConOut from the current mode
    //
    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->QueryMode (GraphicsOutput, GraphicsOutput->Mode->Mode, &SizeOfInfo, &Info);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      ASSERT (SizeOfInfo <= sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

      mConOut.UgaHorizontalResolution = Info->HorizontalResolution;
      mConOut.UgaVerticalResolution   = Info->VerticalResolution;
      mConOut.UgaColorDepth           = 32;
      mConOut.UgaRefreshRate          = 60;

      FreePool (Info);
    } else if (UgaDraw != NULL) {
      Status = UgaDraw->GetMode (
                          UgaDraw,
                          &mConOut.UgaHorizontalResolution,
                          &mConOut.UgaVerticalResolution,
                          &mConOut.UgaColorDepth,
                          &mConOut.UgaRefreshRate
                          );
    }
  }

  return Status;
}

/**
  Start Standard Error Consplitter on device handle.

  @param  This                 Driver Binding protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          Standard Error Consplitter is added to ControllerHandle.
  @retval other                Standard Error Consplitter does not support this device.

**/
EFI_STATUS
EFIAPI
ConSplitterStdErrDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut;

  //
  // Start ConSplitter on ControllerHandle, and create the virtual
  // aggregated console device on first call Start for a StandardError handle.
  //
  Status = ConSplitterStart (
             This,
             ControllerHandle,
             mStdErr.VirtualHandle,
             &gEfiStandardErrorDeviceGuid,
             &gEfiSimpleTextOutProtocolGuid,
             (VOID **)&TextOut
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // When new console device is added, the new mode will be set later,
  // so put current mode back to init state.
  //
  mStdErr.TextOutMode.Mode = 0xFF;

  //
  // If both ConOut and StdErr incorporate the same Text Out device,
  // their MaxMode and QueryData should be the intersection of both.
  //
  Status = ConSplitterTextOutAddDevice (&mStdErr, TextOut, NULL, NULL);
  ConSplitterTextOutSetAttribute (&mStdErr.TextOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));

  return Status;
}

/**
  Stop ConSplitter on device handle by closing Console Device Guid on device handle
  and the console virtual handle.

  @param  This                      Protocol instance pointer.
  @param  ControllerHandle          Handle of device.
  @param  ConSplitterVirtualHandle  Console virtual Handle.
  @param  DeviceGuid                The specified Console Device, such as ConInDev,
                                    ConOutDev.
  @param  InterfaceGuid             The specified protocol to be opened.
  @param  Interface                 Protocol interface returned.

  @retval EFI_SUCCESS               Stop ConSplitter on ControllerHandle successfully.
  @retval other                     Failed to Stop ConSplitter on ControllerHandle.

**/
EFI_STATUS
ConSplitterStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ConSplitterVirtualHandle,
  IN  EFI_GUID                     *DeviceGuid,
  IN  EFI_GUID                     *InterfaceGuid,
  IN  VOID                         **Interface
  )
{
  EFI_STATUS  Status;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  InterfaceGuid,
                  Interface,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // close the protocol referred.
  //
  gBS->CloseProtocol (
         ControllerHandle,
         DeviceGuid,
         This->DriverBindingHandle,
         ConSplitterVirtualHandle
         );

  gBS->CloseProtocol (
         ControllerHandle,
         DeviceGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return EFI_SUCCESS;
}

/**
  Stop Console In ConSplitter on ControllerHandle by closing Console In Device GUID.

  @param  This              Driver Binding protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ConSplitterConInDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL     *TextIn;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TextInEx;

  if (NumberOfChildren == 0) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&TextInEx,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // If Simple Text Input Ex protocol exists,
    // remove device from Text Input Ex devices list.
    //
    Status = ConSplitterTextInExDeleteDevice (&mConIn, TextInEx);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Close Simple Text In protocol on controller handle and virtual handle.
  //
  Status = ConSplitterStop (
             This,
             ControllerHandle,
             mConIn.VirtualHandle,
             &gEfiConsoleInDeviceGuid,
             &gEfiSimpleTextInProtocolGuid,
             (VOID **)&TextIn
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Remove device from Text Input devices list.
  //
  return ConSplitterTextInDeleteDevice (&mConIn, TextIn);
}

/**
  Stop Simple Pointer protocol ConSplitter on ControllerHandle by closing
  Simple Pointer protocol.

  @param  This              Driver Binding protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ConSplitterSimplePointerDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_POINTER_PROTOCOL  *SimplePointer;

  if (NumberOfChildren == 0) {
    return EFI_SUCCESS;
  }

  //
  // Close Simple Pointer protocol on controller handle and virtual handle.
  //
  Status = ConSplitterStop (
             This,
             ControllerHandle,
             mConIn.VirtualHandle,
             &gEfiSimplePointerProtocolGuid,
             &gEfiSimplePointerProtocolGuid,
             (VOID **)&SimplePointer
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Remove this device from Simple Pointer device list.
  //
  return ConSplitterSimplePointerDeleteDevice (&mConIn, SimplePointer);
}

/**
  Stop Absolute Pointer protocol ConSplitter on ControllerHandle by closing
  Absolute Pointer protocol.

  @param  This              Driver Binding protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ConSplitterAbsolutePointerDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                     Status;
  EFI_ABSOLUTE_POINTER_PROTOCOL  *AbsolutePointer;

  if (NumberOfChildren == 0) {
    return EFI_SUCCESS;
  }

  //
  // Close Absolute Pointer protocol on controller handle and virtual handle.
  //
  Status = ConSplitterStop (
             This,
             ControllerHandle,
             mConIn.VirtualHandle,
             &gEfiAbsolutePointerProtocolGuid,
             &gEfiAbsolutePointerProtocolGuid,
             (VOID **)&AbsolutePointer
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Remove this device from Absolute Pointer device list.
  //
  return ConSplitterAbsolutePointerDeleteDevice (&mConIn, AbsolutePointer);
}

/**
  Stop Console Out ConSplitter on device handle by closing Console Out Devcie GUID.

  @param  This              Driver Binding protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ConSplitterConOutDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut;

  if (NumberOfChildren == 0) {
    return EFI_SUCCESS;
  }

  //
  // Close Absolute Pointer protocol on controller handle and virtual handle.
  //
  Status = ConSplitterStop (
             This,
             ControllerHandle,
             mConOut.VirtualHandle,
             &gEfiConsoleOutDeviceGuid,
             &gEfiSimpleTextOutProtocolGuid,
             (VOID **)&TextOut
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Remove this device from Text Out device list.
  //
  return ConSplitterTextOutDeleteDevice (&mConOut, TextOut);
}

/**
  Stop Standard Error ConSplitter on ControllerHandle by closing Standard Error GUID.

  @param  This              Driver Binding protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ConSplitterStdErrDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut;

  if (NumberOfChildren == 0) {
    return EFI_SUCCESS;
  }

  //
  // Close Standard Error Device on controller handle and virtual handle.
  //
  Status = ConSplitterStop (
             This,
             ControllerHandle,
             mStdErr.VirtualHandle,
             &gEfiStandardErrorDeviceGuid,
             &gEfiSimpleTextOutProtocolGuid,
             (VOID **)&TextOut
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Delete this console error out device's data structures.
  //
  return ConSplitterTextOutDeleteDevice (&mStdErr, TextOut);
}

/**
  Take the passed in Buffer of size ElementSize and grow the buffer
  by CONSOLE_SPLITTER_ALLOC_UNIT * ElementSize bytes.
  Copy the current data in Buffer to the new version of Buffer and
  free the old version of buffer.

  @param  ElementSize              Size of element in array.
  @param  Count                    Current number of elements in array.
  @param  Buffer                   Bigger version of passed in Buffer with all the
                                   data.

  @retval EFI_SUCCESS              Buffer size has grown.
  @retval EFI_OUT_OF_RESOURCES     Could not grow the buffer size.

**/
EFI_STATUS
ConSplitterGrowBuffer (
  IN      UINTN  ElementSize,
  IN OUT  UINTN  *Count,
  IN OUT  VOID   **Buffer
  )
{
  VOID  *Ptr;

  //
  // grow the buffer to new buffer size,
  // copy the old buffer's content to the new-size buffer,
  // then free the old buffer.
  //
  Ptr = ReallocatePool (
          ElementSize * (*Count),
          ElementSize * ((*Count) + CONSOLE_SPLITTER_ALLOC_UNIT),
          *Buffer
          );
  if (Ptr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Count += CONSOLE_SPLITTER_ALLOC_UNIT;
  *Buffer = Ptr;
  return EFI_SUCCESS;
}

/**
  Add Text Input Device in Consplitter Text Input list.

  @param  Private                  Text In Splitter pointer.
  @param  TextIn                   Simple Text Input protocol pointer.

  @retval EFI_SUCCESS              Text Input Device added successfully.
  @retval EFI_OUT_OF_RESOURCES     Could not grow the buffer size.

**/
EFI_STATUS
ConSplitterTextInAddDevice (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA   *Private,
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *TextIn
  )
{
  EFI_STATUS  Status;

  //
  // If the Text In List is full, enlarge it by calling ConSplitterGrowBuffer().
  //
  if (Private->CurrentNumberOfConsoles >= Private->TextInListCount) {
    Status = ConSplitterGrowBuffer (
               sizeof (EFI_SIMPLE_TEXT_INPUT_PROTOCOL *),
               &Private->TextInListCount,
               (VOID **)&Private->TextInList
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Add the new text-in device data structure into the Text In List.
  //
  Private->TextInList[Private->CurrentNumberOfConsoles] = TextIn;
  Private->CurrentNumberOfConsoles++;

  //
  // Extra CheckEvent added to reduce the double CheckEvent().
  //
  gBS->CheckEvent (TextIn->WaitForKey);

  return EFI_SUCCESS;
}

/**
  Remove Text Input Device from Consplitter Text Input list.

  @param  Private                  Text In Splitter pointer.
  @param  TextIn                   Simple Text protocol pointer.

  @retval EFI_SUCCESS              Simple Text Device removed successfully.
  @retval EFI_NOT_FOUND            No Simple Text Device found.

**/
EFI_STATUS
ConSplitterTextInDeleteDevice (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA   *Private,
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *TextIn
  )
{
  UINTN  Index;

  //
  // Remove the specified text-in device data structure from the Text In List,
  // and rearrange the remaining data structures in the Text In List.
  //
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
    if (Private->TextInList[Index] == TextIn) {
      for ( ; Index < Private->CurrentNumberOfConsoles - 1; Index++) {
        Private->TextInList[Index] = Private->TextInList[Index + 1];
      }

      Private->CurrentNumberOfConsoles--;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Add Text Input Ex Device in Consplitter Text Input Ex list.

  @param  Private                  Text In Splitter pointer.
  @param  TextInEx                 Simple Text Input Ex Input protocol pointer.

  @retval EFI_SUCCESS              Text Input Ex Device added successfully.
  @retval EFI_OUT_OF_RESOURCES     Could not grow the buffer size.

**/
EFI_STATUS
ConSplitterTextInExAddDevice (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA      *Private,
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TextInEx
  )
{
  EFI_STATUS                  Status;
  LIST_ENTRY                  *Link;
  TEXT_IN_EX_SPLITTER_NOTIFY  *CurrentNotify;
  UINTN                       TextInExListCount;

  //
  // Enlarge the NotifyHandleList and the TextInExList
  //
  if (Private->CurrentNumberOfExConsoles >= Private->TextInExListCount) {
    for (Link = Private->NotifyList.ForwardLink; Link != &Private->NotifyList; Link = Link->ForwardLink) {
      CurrentNotify     = TEXT_IN_EX_SPLITTER_NOTIFY_FROM_THIS (Link);
      TextInExListCount = Private->TextInExListCount;

      Status = ConSplitterGrowBuffer (
                 sizeof (EFI_HANDLE),
                 &TextInExListCount,
                 (VOID **)&CurrentNotify->NotifyHandleList
                 );
      if (EFI_ERROR (Status)) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    TextInExListCount = Private->TextInExListCount;
    Status            = ConSplitterGrowBuffer (
                          sizeof (EFI_KEY_DATA),
                          &TextInExListCount,
                          (VOID **)&Private->KeyQueue
                          );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = ConSplitterGrowBuffer (
               sizeof (EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *),
               &Private->TextInExListCount,
               (VOID **)&Private->TextInExList
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Register the key notify in the new text-in device
  //
  for (Link = Private->NotifyList.ForwardLink; Link != &Private->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = TEXT_IN_EX_SPLITTER_NOTIFY_FROM_THIS (Link);
    Status        = TextInEx->RegisterKeyNotify (
                                TextInEx,
                                &CurrentNotify->KeyData,
                                CurrentNotify->KeyNotificationFn,
                                &CurrentNotify->NotifyHandleList[Private->CurrentNumberOfExConsoles]
                                );
    if (EFI_ERROR (Status)) {
      for (Link = Link->BackLink; Link != &Private->NotifyList; Link = Link->BackLink) {
        CurrentNotify = TEXT_IN_EX_SPLITTER_NOTIFY_FROM_THIS (Link);
        TextInEx->UnregisterKeyNotify (
                    TextInEx,
                    CurrentNotify->NotifyHandleList[Private->CurrentNumberOfExConsoles]
                    );
      }

      return Status;
    }
  }

  //
  // Add the new text-in device data structure into the Text Input Ex List.
  //
  Private->TextInExList[Private->CurrentNumberOfExConsoles] = TextInEx;
  Private->CurrentNumberOfExConsoles++;

  //
  // Sync current toggle state to this new console input device.
  //
  TextInEx->SetState (TextInEx, &Private->PhysicalKeyToggleState);

  //
  // Extra CheckEvent added to reduce the double CheckEvent().
  //
  gBS->CheckEvent (TextInEx->WaitForKeyEx);

  return EFI_SUCCESS;
}

/**
  Remove Text Ex Device from Consplitter Text Input Ex list.

  @param  Private                  Text In Splitter pointer.
  @param  TextInEx                 Simple Text Ex protocol pointer.

  @retval EFI_SUCCESS              Simple Text Input Ex Device removed successfully.
  @retval EFI_NOT_FOUND            No Simple Text Input Ex Device found.

**/
EFI_STATUS
ConSplitterTextInExDeleteDevice (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA      *Private,
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *TextInEx
  )
{
  UINTN  Index;

  //
  // Remove the specified text-in device data structure from the Text Input Ex List,
  // and rearrange the remaining data structures in the Text In List.
  //
  for (Index = 0; Index < Private->CurrentNumberOfExConsoles; Index++) {
    if (Private->TextInExList[Index] == TextInEx) {
      for ( ; Index < Private->CurrentNumberOfExConsoles - 1; Index++) {
        Private->TextInExList[Index] = Private->TextInExList[Index + 1];
      }

      Private->CurrentNumberOfExConsoles--;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Add Simple Pointer Device in Consplitter Simple Pointer list.

  @param  Private                  Text In Splitter pointer.
  @param  SimplePointer            Simple Pointer protocol pointer.

  @retval EFI_SUCCESS              Simple Pointer Device added successfully.
  @retval EFI_OUT_OF_RESOURCES     Could not grow the buffer size.

**/
EFI_STATUS
ConSplitterSimplePointerAddDevice (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private,
  IN  EFI_SIMPLE_POINTER_PROTOCOL    *SimplePointer
  )
{
  EFI_STATUS  Status;

  //
  // If the Simple Pointer List is full, enlarge it by calling ConSplitterGrowBuffer().
  //
  if (Private->CurrentNumberOfPointers >= Private->PointerListCount) {
    Status = ConSplitterGrowBuffer (
               sizeof (EFI_SIMPLE_POINTER_PROTOCOL *),
               &Private->PointerListCount,
               (VOID **)&Private->PointerList
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Add the new text-in device data structure into the Simple Pointer List.
  //
  Private->PointerList[Private->CurrentNumberOfPointers] = SimplePointer;
  Private->CurrentNumberOfPointers++;

  return EFI_SUCCESS;
}

/**
  Remove Simple Pointer Device from Consplitter Simple Pointer list.

  @param  Private                  Text In Splitter pointer.
  @param  SimplePointer            Simple Pointer protocol pointer.

  @retval EFI_SUCCESS              Simple Pointer Device removed successfully.
  @retval EFI_NOT_FOUND            No Simple Pointer Device found.

**/
EFI_STATUS
ConSplitterSimplePointerDeleteDevice (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private,
  IN  EFI_SIMPLE_POINTER_PROTOCOL    *SimplePointer
  )
{
  UINTN  Index;

  //
  // Remove the specified text-in device data structure from the Simple Pointer List,
  // and rearrange the remaining data structures in the Text In List.
  //
  for (Index = 0; Index < Private->CurrentNumberOfPointers; Index++) {
    if (Private->PointerList[Index] == SimplePointer) {
      for ( ; Index < Private->CurrentNumberOfPointers - 1; Index++) {
        Private->PointerList[Index] = Private->PointerList[Index + 1];
      }

      Private->CurrentNumberOfPointers--;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Add Absolute Pointer Device in Consplitter Absolute Pointer list.

  @param  Private                  Text In Splitter pointer.
  @param  AbsolutePointer          Absolute Pointer protocol pointer.

  @retval EFI_SUCCESS              Absolute Pointer Device added successfully.
  @retval EFI_OUT_OF_RESOURCES     Could not grow the buffer size.

**/
EFI_STATUS
ConSplitterAbsolutePointerAddDevice (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private,
  IN  EFI_ABSOLUTE_POINTER_PROTOCOL  *AbsolutePointer
  )
{
  EFI_STATUS  Status;

  //
  // If the Absolute Pointer List is full, enlarge it by calling ConSplitterGrowBuffer().
  //
  if (Private->CurrentNumberOfAbsolutePointers >= Private->AbsolutePointerListCount) {
    Status = ConSplitterGrowBuffer (
               sizeof (EFI_ABSOLUTE_POINTER_PROTOCOL *),
               &Private->AbsolutePointerListCount,
               (VOID **)&Private->AbsolutePointerList
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Add the new text-in device data structure into the Absolute Pointer List.
  //
  Private->AbsolutePointerList[Private->CurrentNumberOfAbsolutePointers] = AbsolutePointer;
  Private->CurrentNumberOfAbsolutePointers++;

  return EFI_SUCCESS;
}

/**
  Remove Absolute Pointer Device from Consplitter Absolute Pointer list.

  @param  Private                  Text In Splitter pointer.
  @param  AbsolutePointer          Absolute Pointer protocol pointer.

  @retval EFI_SUCCESS              Absolute Pointer Device removed successfully.
  @retval EFI_NOT_FOUND            No Absolute Pointer Device found.

**/
EFI_STATUS
ConSplitterAbsolutePointerDeleteDevice (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private,
  IN  EFI_ABSOLUTE_POINTER_PROTOCOL  *AbsolutePointer
  )
{
  UINTN  Index;

  //
  // Remove the specified text-in device data structure from the Absolute Pointer List,
  // and rearrange the remaining data structures from the Absolute Pointer List.
  //
  for (Index = 0; Index < Private->CurrentNumberOfAbsolutePointers; Index++) {
    if (Private->AbsolutePointerList[Index] == AbsolutePointer) {
      for ( ; Index < Private->CurrentNumberOfAbsolutePointers - 1; Index++) {
        Private->AbsolutePointerList[Index] = Private->AbsolutePointerList[Index + 1];
      }

      Private->CurrentNumberOfAbsolutePointers--;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Reallocate Text Out mode map.

  Allocate new buffer and copy original buffer into the new buffer.

  @param  Private                  Consplitter Text Out pointer.

  @retval EFI_SUCCESS              Buffer size has grown
  @retval EFI_OUT_OF_RESOURCES     Could not grow the buffer size.

**/
EFI_STATUS
ConSplitterGrowMapTable (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private
  )
{
  UINTN  Size;
  UINTN  NewSize;
  UINTN  TotalSize;
  INT32  *TextOutModeMap;
  INT32  *OldTextOutModeMap;
  INT32  *SrcAddress;
  INT32  Index;
  UINTN  OldStepSize;
  UINTN  NewStepSize;

  NewSize           = Private->TextOutListCount * sizeof (INT32);
  OldTextOutModeMap = Private->TextOutModeMap;
  TotalSize         = NewSize * (Private->TextOutQueryDataCount);

  //
  // Allocate new buffer for Text Out List.
  //
  TextOutModeMap = AllocatePool (TotalSize);
  if (TextOutModeMap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (TextOutModeMap, TotalSize, 0xFF);
  Private->TextOutModeMap = TextOutModeMap;

  //
  // If TextOutList has been enlarged, need to realloc the mode map table
  // The mode map table is regarded as a two dimension array.
  //
  //                         Old                    New
  //  0   ---------> TextOutListCount ----> TextOutListCount
  //  |   -------------------------------------------
  //  |  |                    |                      |
  //  |  |                    |                      |
  //  |  |                    |                      |
  //  |  |                    |                      |
  //  |  |                    |                      |
  // \/  |                    |                      |
  //      -------------------------------------------
  // QueryDataCount
  //
  if (OldTextOutModeMap != NULL) {
    Size        = Private->CurrentNumberOfConsoles * sizeof (INT32);
    Index       = 0;
    SrcAddress  = OldTextOutModeMap;
    NewStepSize = NewSize / sizeof (INT32);
    // If Private->CurrentNumberOfConsoles is not zero and OldTextOutModeMap
    // is not NULL, it indicates that the original TextOutModeMap is not enough
    // for the new console devices and has been enlarged by CONSOLE_SPLITTER_ALLOC_UNIT columns.
    //
    OldStepSize = NewStepSize - CONSOLE_SPLITTER_ALLOC_UNIT;

    //
    // Copy the old data to the new one
    //
    while (Index < Private->TextOutMode.MaxMode) {
      CopyMem (TextOutModeMap, SrcAddress, Size);
      //
      // Go to next row of new TextOutModeMap.
      //
      TextOutModeMap += NewStepSize;
      //
      // Go to next row of old TextOutModeMap.
      //
      SrcAddress += OldStepSize;
      Index++;
    }

    //
    // Free the old buffer
    //
    FreePool (OldTextOutModeMap);
  }

  return EFI_SUCCESS;
}

/**
  Add new device's output mode to console splitter's mode list.

  @param  Private               Text Out Splitter pointer
  @param  TextOut               Simple Text Output protocol pointer.

  @retval EFI_SUCCESS           Device added successfully.
  @retval EFI_OUT_OF_RESOURCES  Could not grow the buffer size.

**/
EFI_STATUS
ConSplitterAddOutputMode (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA   *Private,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut
  )
{
  EFI_STATUS  Status;
  INT32       MaxMode;
  INT32       Mode;
  UINTN       Index;

  MaxMode                      = TextOut->Mode->MaxMode;
  Private->TextOutMode.MaxMode = MaxMode;

  //
  // Grow the buffer if query data buffer is not large enough to
  // hold all the mode supported by the first console.
  //
  while (MaxMode > (INT32)Private->TextOutQueryDataCount) {
    Status = ConSplitterGrowBuffer (
               sizeof (TEXT_OUT_SPLITTER_QUERY_DATA),
               &Private->TextOutQueryDataCount,
               (VOID **)&Private->TextOutQueryData
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Allocate buffer for the output mode map
  //
  Status = ConSplitterGrowMapTable (Private);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // As the first textout device, directly add the mode in to QueryData
  // and at the same time record the mapping between QueryData and TextOut.
  //
  Mode  = 0;
  Index = 0;
  while (Mode < MaxMode) {
    Status = TextOut->QueryMode (
                        TextOut,
                        Mode,
                        &Private->TextOutQueryData[Mode].Columns,
                        &Private->TextOutQueryData[Mode].Rows
                        );
    //
    // If mode 1 (80x50) is not supported, make sure mode 1 in TextOutQueryData
    // is clear to 0x0.
    //
    if ((EFI_ERROR (Status)) && (Mode == 1)) {
      Private->TextOutQueryData[Mode].Columns = 0;
      Private->TextOutQueryData[Mode].Rows    = 0;
    }

    Private->TextOutModeMap[Index] = Mode;
    Mode++;
    Index += Private->TextOutListCount;
  }

  return EFI_SUCCESS;
}

/**
  Reconstruct TextOutModeMap to get intersection of modes.

  This routine reconstruct TextOutModeMap to get the intersection
  of modes for all console out devices. Because EFI/UEFI spec require
  mode 0 is 80x25, mode 1 is 80x50, this routine will not check the
  intersection for mode 0 and mode 1.

  @param TextOutModeMap  Current text out mode map, begin with the mode 80x25
  @param NewlyAddedMap   New text out mode map, begin with the mode 80x25
  @param MapStepSize     Mode step size for one console device
  @param NewMapStepSize  New Mode step size for one console device
  @param MaxMode         IN: Current max text mode, OUT: Updated max text mode.
  @param CurrentMode     IN: Current text mode,     OUT: Updated current text mode.

**/
VOID
ConSplitterGetIntersection (
  IN     INT32  *TextOutModeMap,
  IN     INT32  *NewlyAddedMap,
  IN     UINTN  MapStepSize,
  IN     UINTN  NewMapStepSize,
  IN OUT INT32  *MaxMode,
  IN OUT INT32  *CurrentMode
  )
{
  INT32  Index;
  INT32  *CurrentMapEntry;
  INT32  *NextMapEntry;
  INT32  *NewMapEntry;
  INT32  CurrentMaxMode;
  INT32  Mode;

  //
  // According to EFI/UEFI spec, mode 0 and mode 1 have been reserved
  // for 80x25 and 80x50 in Simple Text Out protocol, so don't make intersection
  // for mode 0 and mode 1, mode number starts from 2.
  //
  Index           = 2;
  CurrentMapEntry = &TextOutModeMap[MapStepSize * 2];
  NextMapEntry    = CurrentMapEntry;
  NewMapEntry     = &NewlyAddedMap[NewMapStepSize * 2];

  CurrentMaxMode = *MaxMode;
  Mode           = *CurrentMode;

  while (Index < CurrentMaxMode) {
    if (*NewMapEntry == -1) {
      //
      // This mode is not supported any more. Remove it. Special care
      // must be taken as this remove will also affect current mode;
      //
      if (Index == *CurrentMode) {
        Mode = -1;
      } else if (Index < *CurrentMode) {
        Mode--;
      }

      (*MaxMode)--;
    } else {
      if (CurrentMapEntry != NextMapEntry) {
        CopyMem (NextMapEntry, CurrentMapEntry, MapStepSize * sizeof (INT32));
      }

      NextMapEntry += MapStepSize;
    }

    CurrentMapEntry += MapStepSize;
    NewMapEntry     += NewMapStepSize;
    Index++;
  }

  *CurrentMode = Mode;

  return;
}

/**
  Sync the device's output mode to console splitter's mode list.

  @param  Private               Text Out Splitter pointer.
  @param  TextOut               Simple Text Output protocol pointer.

**/
VOID
ConSplitterSyncOutputMode (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA   *Private,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut
  )
{
  INT32                         CurrentMaxMode;
  INT32                         Mode;
  INT32                         Index;
  INT32                         *TextOutModeMap;
  INT32                         *MapTable;
  INT32                         QueryMode;
  TEXT_OUT_SPLITTER_QUERY_DATA  *TextOutQueryData;
  UINTN                         Rows;
  UINTN                         Columns;
  UINTN                         StepSize;
  EFI_STATUS                    Status;

  //
  // Must make sure that current mode won't change even if mode number changes
  //
  CurrentMaxMode   = Private->TextOutMode.MaxMode;
  TextOutModeMap   = Private->TextOutModeMap;
  StepSize         = Private->TextOutListCount;
  TextOutQueryData = Private->TextOutQueryData;

  //
  // Query all the mode that the newly added TextOut supports
  //
  Mode     = 0;
  MapTable = TextOutModeMap + Private->CurrentNumberOfConsoles;
  while (Mode < TextOut->Mode->MaxMode) {
    Status = TextOut->QueryMode (TextOut, Mode, &Columns, &Rows);

    if (EFI_ERROR (Status)) {
      if (Mode == 1) {
        //
        // If mode 1 (80x50) is not supported, make sure mode 1 in TextOutQueryData
        // is clear to 0x0.
        //
        MapTable[StepSize]             = Mode;
        TextOutQueryData[Mode].Columns = 0;
        TextOutQueryData[Mode].Rows    = 0;
      }

      Mode++;
      continue;
    }

    //
    // Search the intersection map and QueryData database to see if they intersects
    //
    Index = 0;
    while (Index < CurrentMaxMode) {
      QueryMode = *(TextOutModeMap + Index * StepSize);
      if ((TextOutQueryData[QueryMode].Rows == Rows) && (TextOutQueryData[QueryMode].Columns == Columns)) {
        MapTable[Index * StepSize] = Mode;
        break;
      }

      Index++;
    }

    Mode++;
  }

  //
  // Now search the TextOutModeMap table to find the intersection of supported
  // mode between ConSplitter and the newly added device.
  //
  ConSplitterGetIntersection (
    TextOutModeMap,
    MapTable,
    StepSize,
    StepSize,
    &Private->TextOutMode.MaxMode,
    &Private->TextOutMode.Mode
    );

  return;
}

/**
  Sync output device between ConOut and StdErr output.

  @retval EFI_SUCCESS              Sync implemented successfully.
  @retval EFI_OUT_OF_RESOURCES     Could not grow the buffer size.

**/
EFI_STATUS
ConSplitterGetIntersectionBetweenConOutAndStrErr (
  VOID
  )
{
  UINTN                         ConOutNumOfConsoles;
  UINTN                         StdErrNumOfConsoles;
  TEXT_OUT_AND_GOP_DATA         *ConOutTextOutList;
  TEXT_OUT_AND_GOP_DATA         *StdErrTextOutList;
  UINTN                         Indexi;
  UINTN                         Indexj;
  UINTN                         ConOutRows;
  UINTN                         ConOutColumns;
  UINTN                         StdErrRows;
  UINTN                         StdErrColumns;
  INT32                         ConOutMaxMode;
  INT32                         StdErrMaxMode;
  INT32                         ConOutMode;
  INT32                         StdErrMode;
  INT32                         Mode;
  INT32                         Index;
  INT32                         *ConOutModeMap;
  INT32                         *StdErrModeMap;
  INT32                         *ConOutMapTable;
  INT32                         *StdErrMapTable;
  TEXT_OUT_SPLITTER_QUERY_DATA  *ConOutQueryData;
  TEXT_OUT_SPLITTER_QUERY_DATA  *StdErrQueryData;
  UINTN                         ConOutStepSize;
  UINTN                         StdErrStepSize;
  BOOLEAN                       FoundTheSameTextOut;
  UINTN                         ConOutMapTableSize;
  UINTN                         StdErrMapTableSize;

  ConOutNumOfConsoles = mConOut.CurrentNumberOfConsoles;
  StdErrNumOfConsoles = mStdErr.CurrentNumberOfConsoles;
  ConOutTextOutList   = mConOut.TextOutList;
  StdErrTextOutList   = mStdErr.TextOutList;

  Indexi              = 0;
  FoundTheSameTextOut = FALSE;
  while ((Indexi < ConOutNumOfConsoles) && (!FoundTheSameTextOut)) {
    Indexj = 0;
    while (Indexj < StdErrNumOfConsoles) {
      if (ConOutTextOutList->TextOut == StdErrTextOutList->TextOut) {
        FoundTheSameTextOut = TRUE;
        break;
      }

      Indexj++;
      StdErrTextOutList++;
    }

    Indexi++;
    ConOutTextOutList++;
  }

  if (!FoundTheSameTextOut) {
    return EFI_SUCCESS;
  }

  //
  // Must make sure that current mode won't change even if mode number changes
  //
  ConOutMaxMode   = mConOut.TextOutMode.MaxMode;
  ConOutModeMap   = mConOut.TextOutModeMap;
  ConOutStepSize  = mConOut.TextOutListCount;
  ConOutQueryData = mConOut.TextOutQueryData;

  StdErrMaxMode   = mStdErr.TextOutMode.MaxMode;
  StdErrModeMap   = mStdErr.TextOutModeMap;
  StdErrStepSize  = mStdErr.TextOutListCount;
  StdErrQueryData = mStdErr.TextOutQueryData;

  //
  // Allocate the map table and set the map table's index to -1.
  //
  ConOutMapTableSize = ConOutMaxMode * sizeof (INT32);
  ConOutMapTable     = AllocateZeroPool (ConOutMapTableSize);
  if (ConOutMapTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (ConOutMapTable, ConOutMapTableSize, 0xFF);

  StdErrMapTableSize = StdErrMaxMode * sizeof (INT32);
  StdErrMapTable     = AllocateZeroPool (StdErrMapTableSize);
  if (StdErrMapTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (StdErrMapTable, StdErrMapTableSize, 0xFF);

  //
  // Find the intersection of the two set of modes. If they actually intersect, the
  // corresponding entry in the map table is set to 1.
  //
  Mode = 0;
  while (Mode < ConOutMaxMode) {
    //
    // Search the intersection map and QueryData database to see if they intersect
    //
    Index         = 0;
    ConOutMode    = *(ConOutModeMap + Mode * ConOutStepSize);
    ConOutRows    = ConOutQueryData[ConOutMode].Rows;
    ConOutColumns = ConOutQueryData[ConOutMode].Columns;
    while (Index < StdErrMaxMode) {
      StdErrMode    = *(StdErrModeMap + Index * StdErrStepSize);
      StdErrRows    = StdErrQueryData[StdErrMode].Rows;
      StdErrColumns = StdErrQueryData[StdErrMode].Columns;
      if ((StdErrRows == ConOutRows) && (StdErrColumns == ConOutColumns)) {
        ConOutMapTable[Mode]  = 1;
        StdErrMapTable[Index] = 1;
        break;
      }

      Index++;
    }

    Mode++;
  }

  //
  // Now search the TextOutModeMap table to find the intersection of supported
  // mode between ConSplitter and the newly added device.
  //
  ConSplitterGetIntersection (
    ConOutModeMap,
    ConOutMapTable,
    mConOut.TextOutListCount,
    1,
    &(mConOut.TextOutMode.MaxMode),
    &(mConOut.TextOutMode.Mode)
    );

  if (mConOut.TextOutMode.Mode < 0) {
    mConOut.TextOut.SetMode (&(mConOut.TextOut), 0);
  }

  ConSplitterGetIntersection (
    StdErrModeMap,
    StdErrMapTable,
    mStdErr.TextOutListCount,
    1,
    &(mStdErr.TextOutMode.MaxMode),
    &(mStdErr.TextOutMode.Mode)
    );

  if (mStdErr.TextOutMode.Mode < 0) {
    mStdErr.TextOut.SetMode (&(mStdErr.TextOut), 0);
  }

  FreePool (ConOutMapTable);
  FreePool (StdErrMapTable);

  return EFI_SUCCESS;
}

/**
  Add Graphics Output modes into Consplitter Text Out list.

  @param  Private               Text Out Splitter pointer.
  @param  GraphicsOutput        Graphics Output protocol pointer.
  @param  UgaDraw               UGA Draw protocol pointer.

  @retval EFI_SUCCESS           Output mode added successfully.
  @retval other                 Failed to add output mode.

**/
EFI_STATUS
ConSplitterAddGraphicsOutputMode (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private,
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL    *GraphicsOutput,
  IN  EFI_UGA_DRAW_PROTOCOL           *UgaDraw
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 CurrentIndex;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Mode;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE     *CurrentGraphicsOutputMode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *ModeBuffer;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *MatchedMode;
  UINTN                                 NumberIndex;
  BOOLEAN                               Match;
  BOOLEAN                               AlreadyExist;
  UINT32                                UgaHorizontalResolution;
  UINT32                                UgaVerticalResolution;
  UINT32                                UgaColorDepth;
  UINT32                                UgaRefreshRate;

  ASSERT (GraphicsOutput != NULL || UgaDraw != NULL);

  CurrentGraphicsOutputMode = Private->GraphicsOutput.Mode;

  Index        = 0;
  CurrentIndex = 0;
  Status       = EFI_SUCCESS;

  if (Private->CurrentNumberOfUgaDraw != 0) {
    //
    // If any UGA device has already been added, then there is no need to
    // calculate intersection of display mode of different GOP/UGA device,
    // since only one display mode will be exported (i.e. user-defined mode)
    //
    goto Done;
  }

  if (GraphicsOutput != NULL) {
    if (Private->CurrentNumberOfGraphicsOutput == 0) {
      //
      // This is the first Graphics Output device added
      //
      CurrentGraphicsOutputMode->MaxMode = GraphicsOutput->Mode->MaxMode;
      CurrentGraphicsOutputMode->Mode    = GraphicsOutput->Mode->Mode;
      CopyMem (CurrentGraphicsOutputMode->Info, GraphicsOutput->Mode->Info, GraphicsOutput->Mode->SizeOfInfo);
      CurrentGraphicsOutputMode->SizeOfInfo      = GraphicsOutput->Mode->SizeOfInfo;
      CurrentGraphicsOutputMode->FrameBufferBase = GraphicsOutput->Mode->FrameBufferBase;
      CurrentGraphicsOutputMode->FrameBufferSize = GraphicsOutput->Mode->FrameBufferSize;

      //
      // Allocate resource for the private mode buffer
      //
      ModeBuffer = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION) * GraphicsOutput->Mode->MaxMode);
      if (ModeBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      FreePool (Private->GraphicsOutputModeBuffer);
      Private->GraphicsOutputModeBuffer = ModeBuffer;

      //
      // Store all supported display modes to the private mode buffer
      //
      Mode = ModeBuffer;
      for (Index = 0; Index < GraphicsOutput->Mode->MaxMode; Index++) {
        //
        // The Info buffer would be allocated by callee
        //
        Status = GraphicsOutput->QueryMode (GraphicsOutput, (UINT32)Index, &SizeOfInfo, &Info);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        ASSERT (SizeOfInfo <= sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
        CopyMem (Mode, Info, SizeOfInfo);
        Mode++;
        FreePool (Info);
      }
    } else {
      //
      // Check intersection of display mode
      //
      ModeBuffer = AllocatePool (sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION) * CurrentGraphicsOutputMode->MaxMode);
      if (ModeBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      MatchedMode = ModeBuffer;
      Mode        = &Private->GraphicsOutputModeBuffer[0];
      for (Index = 0; Index < CurrentGraphicsOutputMode->MaxMode; Index++) {
        Match = FALSE;

        for (NumberIndex = 0; NumberIndex < GraphicsOutput->Mode->MaxMode; NumberIndex++) {
          //
          // The Info buffer would be allocated by callee
          //
          Status = GraphicsOutput->QueryMode (GraphicsOutput, (UINT32)NumberIndex, &SizeOfInfo, &Info);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          if ((Info->HorizontalResolution == Mode->HorizontalResolution) &&
              (Info->VerticalResolution == Mode->VerticalResolution))
          {
            //
            // If GOP device supports one mode in current mode buffer,
            // it will be added into matched mode buffer
            //
            Match = TRUE;
            FreePool (Info);
            break;
          }

          FreePool (Info);
        }

        if (Match) {
          AlreadyExist = FALSE;

          //
          // Check if GOP mode has been in the mode buffer, ModeBuffer = MatchedMode at begin.
          //
          for (Info = ModeBuffer; Info < MatchedMode; Info++) {
            if ((Info->HorizontalResolution == Mode->HorizontalResolution) &&
                (Info->VerticalResolution == Mode->VerticalResolution))
            {
              AlreadyExist = TRUE;
              break;
            }
          }

          if (!AlreadyExist) {
            CopyMem (MatchedMode, Mode, sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

            //
            // Physical frame buffer is no longer available, change PixelFormat to PixelBltOnly
            //
            MatchedMode->Version     = 0;
            MatchedMode->PixelFormat = PixelBltOnly;
            ZeroMem (&MatchedMode->PixelInformation, sizeof (EFI_PIXEL_BITMASK));

            MatchedMode++;
          }
        }

        Mode++;
      }

      //
      // Drop the old mode buffer, assign it to a new one
      //
      FreePool (Private->GraphicsOutputModeBuffer);
      Private->GraphicsOutputModeBuffer = ModeBuffer;

      //
      // Physical frame buffer is no longer available when there are more than one physical GOP devices
      //
      CurrentGraphicsOutputMode->MaxMode           = (UINT32)(((UINTN)MatchedMode - (UINTN)ModeBuffer) / sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
      CurrentGraphicsOutputMode->Info->PixelFormat = PixelBltOnly;
      ZeroMem (&CurrentGraphicsOutputMode->Info->PixelInformation, sizeof (EFI_PIXEL_BITMASK));
      CurrentGraphicsOutputMode->SizeOfInfo      = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
      CurrentGraphicsOutputMode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS)(UINTN)NULL;
      CurrentGraphicsOutputMode->FrameBufferSize = 0;
    }

    //
    // Graphics console driver can ensure the same mode for all GOP devices
    //
    for (Index = 0; Index < CurrentGraphicsOutputMode->MaxMode; Index++) {
      Mode = &Private->GraphicsOutputModeBuffer[Index];
      if ((Mode->HorizontalResolution == GraphicsOutput->Mode->Info->HorizontalResolution) &&
          (Mode->VerticalResolution == GraphicsOutput->Mode->Info->VerticalResolution))
      {
        CurrentIndex = Index;
        break;
      }
    }

    if (Index >= CurrentGraphicsOutputMode->MaxMode) {
      //
      // if user defined mode is not found, set to default mode 800x600
      //
      for (Index = 0; Index < CurrentGraphicsOutputMode->MaxMode; Index++) {
        Mode = &Private->GraphicsOutputModeBuffer[Index];
        if ((Mode->HorizontalResolution == 800) && (Mode->VerticalResolution == 600)) {
          CurrentIndex = Index;
          break;
        }
      }
    }
  } else if (UgaDraw != NULL) {
    //
    // Graphics console driver can ensure the same mode for all GOP devices
    // so we can get the current mode from this video device
    //
    UgaDraw->GetMode (
               UgaDraw,
               &UgaHorizontalResolution,
               &UgaVerticalResolution,
               &UgaColorDepth,
               &UgaRefreshRate
               );

    CurrentGraphicsOutputMode->MaxMode         = 1;
    Info                                       = CurrentGraphicsOutputMode->Info;
    Info->Version                              = 0;
    Info->HorizontalResolution                 = UgaHorizontalResolution;
    Info->VerticalResolution                   = UgaVerticalResolution;
    Info->PixelFormat                          = PixelBltOnly;
    Info->PixelsPerScanLine                    = UgaHorizontalResolution;
    CurrentGraphicsOutputMode->SizeOfInfo      = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    CurrentGraphicsOutputMode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS)(UINTN)NULL;
    CurrentGraphicsOutputMode->FrameBufferSize = 0;

    //
    // Update the private mode buffer
    //
    CopyMem (&Private->GraphicsOutputModeBuffer[0], Info, sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

    //
    // Only mode 0 is available to be set
    //
    CurrentIndex = 0;
  }

Done:

  if (GraphicsOutput != NULL) {
    Private->CurrentNumberOfGraphicsOutput++;
  }

  if (UgaDraw != NULL) {
    Private->CurrentNumberOfUgaDraw++;
  }

  //
  // Force GraphicsOutput mode to be set,
  //

  Mode = &Private->GraphicsOutputModeBuffer[CurrentIndex];
  if ((GraphicsOutput != NULL) &&
      (Mode->HorizontalResolution == CurrentGraphicsOutputMode->Info->HorizontalResolution) &&
      (Mode->VerticalResolution == CurrentGraphicsOutputMode->Info->VerticalResolution))
  {
    CurrentGraphicsOutputMode->Mode = (UINT32)CurrentIndex;
    if ((Mode->HorizontalResolution != GraphicsOutput->Mode->Info->HorizontalResolution) ||
        (Mode->VerticalResolution != GraphicsOutput->Mode->Info->VerticalResolution))
    {
      //
      // If all existing video device has been set to common mode, only set new GOP device to
      // the common mode
      //
      for (NumberIndex = 0; NumberIndex < GraphicsOutput->Mode->MaxMode; NumberIndex++) {
        Status = GraphicsOutput->QueryMode (GraphicsOutput, (UINT32)NumberIndex, &SizeOfInfo, &Info);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        if ((Info->HorizontalResolution == Mode->HorizontalResolution) && (Info->VerticalResolution == Mode->VerticalResolution)) {
          FreePool (Info);
          break;
        }

        FreePool (Info);
      }

      Status = GraphicsOutput->SetMode (GraphicsOutput, (UINT32)NumberIndex);
    }
  } else {
    //
    // Current mode number may need update now, so set it to an invalid mode number
    //
    CurrentGraphicsOutputMode->Mode = 0xffff;
    //
    // Graphics console can ensure all GOP devices have the same mode which can be taken as current mode.
    //
    Status = Private->GraphicsOutput.SetMode (&Private->GraphicsOutput, (UINT32)CurrentIndex);
    if (EFI_ERROR (Status)) {
      //
      // If user defined mode is not valid for display device, set to the default mode 800x600.
      //
      (Private->GraphicsOutputModeBuffer[0]).HorizontalResolution = 800;
      (Private->GraphicsOutputModeBuffer[0]).VerticalResolution   = 600;
      Status                                                      = Private->GraphicsOutput.SetMode (&Private->GraphicsOutput, 0);
    }
  }

  return Status;
}

/**
  Set the current console out mode.

  This routine will get the current console mode information (column, row)
  from ConsoleOutMode variable and set it; if the variable does not exist,
  set to user defined console mode.

  @param  Private            Consplitter Text Out pointer.

**/
VOID
ConsplitterSetConsoleOutMode (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private
  )
{
  UINTN                            Col;
  UINTN                            Row;
  UINTN                            Mode;
  UINTN                            PreferMode;
  UINTN                            BaseMode;
  UINTN                            MaxMode;
  EFI_STATUS                       Status;
  CONSOLE_OUT_MODE                 ModeInfo;
  CONSOLE_OUT_MODE                 MaxModeInfo;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut;

  PreferMode = 0xFF;
  BaseMode   = 0xFF;
  TextOut    = &Private->TextOut;
  MaxMode    = (UINTN)(TextOut->Mode->MaxMode);

  MaxModeInfo.Column = 0;
  MaxModeInfo.Row    = 0;
  ModeInfo.Column    = PcdGet32 (PcdConOutColumn);
  ModeInfo.Row       = PcdGet32 (PcdConOutRow);

  //
  // To find the prefer mode and basic mode from Text Out mode list
  //
  for (Mode = 0; Mode < MaxMode; Mode++) {
    Status = TextOut->QueryMode (TextOut, Mode, &Col, &Row);
    if (!EFI_ERROR (Status)) {
      if ((ModeInfo.Column != 0) && (ModeInfo.Row != 0)) {
        //
        // Use user defined column and row
        //
        if ((Col == ModeInfo.Column) && (Row == ModeInfo.Row)) {
          PreferMode = Mode;
        }
      } else {
        //
        // If user sets PcdConOutColumn or PcdConOutRow to 0,
        // find and set the highest text mode.
        //
        if ((Col >= MaxModeInfo.Column) && (Row >= MaxModeInfo.Row)) {
          MaxModeInfo.Column = Col;
          MaxModeInfo.Row    = Row;
          PreferMode         = Mode;
        }
      }

      if ((Col == 80) && (Row == 25)) {
        BaseMode = Mode;
      }
    }
  }

  //
  // Set prefer mode to Text Out devices.
  //
  Status = TextOut->SetMode (TextOut, PreferMode);
  if (EFI_ERROR (Status)) {
    //
    // if current mode setting is failed, default 80x25 mode will be set.
    //
    Status = TextOut->SetMode (TextOut, BaseMode);
    ASSERT (!EFI_ERROR (Status));

    Status = PcdSet32S (PcdConOutColumn, 80);
    ASSERT (!EFI_ERROR (Status));
    Status = PcdSet32S (PcdConOutRow, 25);
    ASSERT (!EFI_ERROR (Status));
  }

  return;
}

/**
  Add Text Output Device in Consplitter Text Output list.

  @param  Private                  Text Out Splitter pointer.
  @param  TextOut                  Simple Text Output protocol pointer.
  @param  GraphicsOutput           Graphics Output protocol pointer.
  @param  UgaDraw                  UGA Draw protocol pointer.

  @retval EFI_SUCCESS              Text Output Device added successfully.
  @retval EFI_OUT_OF_RESOURCES     Could not grow the buffer size.

**/
EFI_STATUS
ConSplitterTextOutAddDevice (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA   *Private,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut,
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput,
  IN  EFI_UGA_DRAW_PROTOCOL            *UgaDraw
  )
{
  EFI_STATUS                            Status;
  UINTN                                 CurrentNumOfConsoles;
  INT32                                 MaxMode;
  UINT32                                UgaHorizontalResolution;
  UINT32                                UgaVerticalResolution;
  UINT32                                UgaColorDepth;
  UINT32                                UgaRefreshRate;
  TEXT_OUT_AND_GOP_DATA                 *TextAndGop;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  EFI_STATUS                            DeviceStatus;

  Status                      = EFI_SUCCESS;
  CurrentNumOfConsoles        = Private->CurrentNumberOfConsoles;
  Private->AddingConOutDevice = TRUE;

  //
  // If the Text Out List is full, enlarge it by calling ConSplitterGrowBuffer().
  //
  while (CurrentNumOfConsoles >= Private->TextOutListCount) {
    Status = ConSplitterGrowBuffer (
               sizeof (TEXT_OUT_AND_GOP_DATA),
               &Private->TextOutListCount,
               (VOID **)&Private->TextOutList
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Also need to reallocate the TextOutModeMap table
    //
    Status = ConSplitterGrowMapTable (Private);
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  TextAndGop = &Private->TextOutList[CurrentNumOfConsoles];

  TextAndGop->TextOut        = TextOut;
  TextAndGop->GraphicsOutput = GraphicsOutput;
  TextAndGop->UgaDraw        = UgaDraw;

  if (CurrentNumOfConsoles == 0) {
    //
    // Add the first device's output mode to console splitter's mode list
    //
    Status = ConSplitterAddOutputMode (Private, TextOut);
  } else {
    ConSplitterSyncOutputMode (Private, TextOut);
  }

  Private->CurrentNumberOfConsoles++;

  //
  // Scan both TextOutList, for the intersection TextOut device
  // maybe both ConOut and StdErr incorporate the same Text Out
  // device in them, thus the output of both should be synced.
  //
  ConSplitterGetIntersectionBetweenConOutAndStrErr ();

  MaxMode = Private->TextOutMode.MaxMode;
  ASSERT (MaxMode >= 1);

  DeviceStatus = EFI_DEVICE_ERROR;
  Status       = EFI_DEVICE_ERROR;

  //
  // This device display mode will be added into Graphics Ouput modes.
  //
  if ((GraphicsOutput != NULL) || (UgaDraw != NULL)) {
    DeviceStatus = ConSplitterAddGraphicsOutputMode (Private, GraphicsOutput, UgaDraw);
  }

  if (FeaturePcdGet (PcdConOutUgaSupport)) {
    //
    // If UGA is produced by Consplitter
    //
    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->QueryMode (GraphicsOutput, GraphicsOutput->Mode->Mode, &SizeOfInfo, &Info);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      ASSERT (SizeOfInfo <= sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));

      UgaHorizontalResolution = Info->HorizontalResolution;
      UgaVerticalResolution   = Info->VerticalResolution;

      FreePool (Info);
    } else if (UgaDraw != NULL) {
      Status = UgaDraw->GetMode (
                          UgaDraw,
                          &UgaHorizontalResolution,
                          &UgaVerticalResolution,
                          &UgaColorDepth,
                          &UgaRefreshRate
                          );
      if (!EFI_ERROR (Status) && EFI_ERROR (DeviceStatus)) {
        //
        // if GetMode is successfully and UGA device hasn't been set, set it
        //
        Status = ConSplitterUgaDrawSetMode (
                   &Private->UgaDraw,
                   UgaHorizontalResolution,
                   UgaVerticalResolution,
                   UgaColorDepth,
                   UgaRefreshRate
                   );
      }

      //
      // If GetMode/SetMode is failed, set to 800x600 mode
      //
      if (EFI_ERROR (Status)) {
        Status = ConSplitterUgaDrawSetMode (
                   &Private->UgaDraw,
                   800,
                   600,
                   32,
                   60
                   );
      }
    }
  }

  if (((!EFI_ERROR (DeviceStatus)) || (!EFI_ERROR (Status))) &&
      ((Private->CurrentNumberOfGraphicsOutput + Private->CurrentNumberOfUgaDraw) == 1))
  {
    if (!FeaturePcdGet (PcdConOutGopSupport)) {
      //
      // If Graphics Outpurt protocol not supported, UGA Draw protocol is installed
      // on the virtual handle.
      //
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mConOut.VirtualHandle,
                      &gEfiUgaDrawProtocolGuid,
                      &mConOut.UgaDraw,
                      NULL
                      );
    } else if (!FeaturePcdGet (PcdConOutUgaSupport)) {
      //
      // If UGA Draw protocol not supported, Graphics Output Protocol is installed
      // on virtual handle.
      //
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mConOut.VirtualHandle,
                      &gEfiGraphicsOutputProtocolGuid,
                      &mConOut.GraphicsOutput,
                      NULL
                      );
    } else {
      //
      // Boot Graphics Output protocol and UGA Draw protocol are supported,
      // both they will be installed on virtual handle.
      //
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mConOut.VirtualHandle,
                      &gEfiGraphicsOutputProtocolGuid,
                      &mConOut.GraphicsOutput,
                      &gEfiUgaDrawProtocolGuid,
                      &mConOut.UgaDraw,
                      NULL
                      );
    }
  }

  //
  // After adding new console device, all existing console devices should be
  // synced to the current shared mode.
  //
  ConsplitterSetConsoleOutMode (Private);

  Private->AddingConOutDevice = FALSE;

  return Status;
}

/**
  Remove Text Out Device in Consplitter Text Out list.

  @param  Private                  Text Out Splitter pointer.
  @param  TextOut                  Simple Text Output Pointer protocol pointer.

  @retval EFI_SUCCESS              Text Out Device removed successfully.
  @retval EFI_NOT_FOUND            No Text Out Device found.

**/
EFI_STATUS
ConSplitterTextOutDeleteDevice (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA   *Private,
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut
  )
{
  INT32                  Index;
  UINTN                  CurrentNumOfConsoles;
  TEXT_OUT_AND_GOP_DATA  *TextOutList;
  EFI_STATUS             Status;

  //
  // Remove the specified text-out device data structure from the Text out List,
  // and rearrange the remaining data structures in the Text out List.
  //
  CurrentNumOfConsoles = Private->CurrentNumberOfConsoles;
  Index                = (INT32)CurrentNumOfConsoles - 1;
  TextOutList          = Private->TextOutList;
  while (Index >= 0) {
    if (TextOutList->TextOut == TextOut) {
      if (TextOutList->UgaDraw != NULL) {
        Private->CurrentNumberOfUgaDraw--;
      }

      if (TextOutList->GraphicsOutput != NULL) {
        Private->CurrentNumberOfGraphicsOutput--;
      }

      CopyMem (TextOutList, TextOutList + 1, sizeof (TEXT_OUT_AND_GOP_DATA) * Index);
      CurrentNumOfConsoles--;
      break;
    }

    Index--;
    TextOutList++;
  }

  //
  // The specified TextOut is not managed by the ConSplitter driver
  //
  if (Index < 0) {
    return EFI_NOT_FOUND;
  }

  if ((Private->CurrentNumberOfGraphicsOutput == 0) && (Private->CurrentNumberOfUgaDraw == 0)) {
    //
    // If there is not any physical GOP and UGA device in system,
    // Consplitter GOP or UGA protocol will be uninstalled
    //
    if (!FeaturePcdGet (PcdConOutGopSupport)) {
      Status = gBS->UninstallProtocolInterface (
                      Private->VirtualHandle,
                      &gEfiUgaDrawProtocolGuid,
                      &Private->UgaDraw
                      );
    } else if (!FeaturePcdGet (PcdConOutUgaSupport)) {
      Status = gBS->UninstallProtocolInterface (
                      Private->VirtualHandle,
                      &gEfiGraphicsOutputProtocolGuid,
                      &Private->GraphicsOutput
                      );
    } else {
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      Private->VirtualHandle,
                      &gEfiUgaDrawProtocolGuid,
                      &Private->UgaDraw,
                      &gEfiGraphicsOutputProtocolGuid,
                      &Private->GraphicsOutput,
                      NULL
                      );
    }
  }

  if (CurrentNumOfConsoles == 0) {
    //
    // If the number of consoles is zero, reset all parameters
    //
    Private->CurrentNumberOfConsoles     = 0;
    Private->TextOutMode.MaxMode         = 1;
    Private->TextOutQueryData[0].Columns = 80;
    Private->TextOutQueryData[0].Rows    = 25;
    TextOutSetMode (Private, 0);

    return EFI_SUCCESS;
  }

  //
  // Max Mode is really an intersection of the QueryMode command to all
  // devices. So we must copy the QueryMode of the first device to
  // QueryData.
  //
  ZeroMem (
    Private->TextOutQueryData,
    Private->TextOutQueryDataCount * sizeof (TEXT_OUT_SPLITTER_QUERY_DATA)
    );

  FreePool (Private->TextOutModeMap);
  Private->TextOutModeMap = NULL;
  TextOutList             = Private->TextOutList;

  //
  // Add the first TextOut to the QueryData array and ModeMap table
  //
  Status = ConSplitterAddOutputMode (Private, TextOutList->TextOut);

  //
  // Now add one by one
  //
  Index                            = 1;
  Private->CurrentNumberOfConsoles = 1;
  TextOutList++;
  while ((UINTN)Index < CurrentNumOfConsoles) {
    ConSplitterSyncOutputMode (Private, TextOutList->TextOut);
    Index++;
    Private->CurrentNumberOfConsoles++;
    TextOutList++;
  }

  ConSplitterGetIntersectionBetweenConOutAndStrErr ();

  return Status;
}

/**
  Reset the input device and optionally run diagnostics

  @param  This                     Protocol instance pointer.
  @param  ExtendedVerification     Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS              The device was reset.
  @retval EFI_DEVICE_ERROR         The device is not functioning properly and could
                                   not be reset.

**/
EFI_STATUS
EFIAPI
ConSplitterTextInReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN  BOOLEAN                         ExtendedVerification
  )
{
  EFI_STATUS                     Status;
  EFI_STATUS                     ReturnStatus;
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                          Index;

  Private = TEXT_IN_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  Private->KeyEventSignalState = FALSE;

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfConsoles; Index++) {
    Status = Private->TextInList[Index]->Reset (
                                           Private->TextInList[Index],
                                           ExtendedVerification
                                           );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  if (!EFI_ERROR (ReturnStatus)) {
    ToggleStateSyncReInitialization (Private);
    //
    // Empty the key queue.
    //
    Private->CurrentNumberOfKeys = 0;
  }

  return ReturnStatus;
}

/**
  Dequeue the saved key from internal key queue.

  @param  Private                  Protocol instance pointer.
  @param  KeyData                  A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   pressed.
  @retval EFI_NOT_FOUND            Queue is empty.
  @retval EFI_SUCCESS              First key is dequeued and returned.
**/
EFI_STATUS
ConSplitterTextInExDequeueKey (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private,
  OUT EFI_KEY_DATA                   *KeyData
  )
{
  if (Private->CurrentNumberOfKeys == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Return the first saved key.
  //
  CopyMem (KeyData, &Private->KeyQueue[0], sizeof (EFI_KEY_DATA));
  Private->CurrentNumberOfKeys--;
  CopyMem (
    &Private->KeyQueue[0],
    &Private->KeyQueue[1],
    Private->CurrentNumberOfKeys * sizeof (EFI_KEY_DATA)
    );
  return EFI_SUCCESS;
}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  Private                  Protocol instance pointer.
  @param  Key                      Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS              The keystroke information was returned.
  @retval EFI_NOT_READY            There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR         The keydtroke information was not returned due
                                   to hardware errors.
  @retval EFI_UNSUPPORTED          The device does not support the ability to read
                                   keystroke data.

**/
EFI_STATUS
EFIAPI
ConSplitterTextInPrivateReadKeyStroke (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private,
  OUT EFI_INPUT_KEY                  *Key
  )
{
  EFI_STATUS    Status;
  UINTN         Index;
  EFI_KEY_DATA  KeyData;

  //
  // Return the first saved non-NULL key.
  //
  while (TRUE) {
    Status = ConSplitterTextInExDequeueKey (Private, &KeyData);
    if (EFI_ERROR (Status)) {
      break;
    }

    if ((KeyData.Key.ScanCode != CHAR_NULL) || (KeyData.Key.UnicodeChar != SCAN_NULL)) {
      CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));
      return Status;
    }
  }

  Key->UnicodeChar = 0;
  Key->ScanCode    = SCAN_NULL;

  //
  // if no physical console input device exists, return EFI_NOT_READY;
  // if any physical console input device has key input,
  // return the key and EFI_SUCCESS.
  //
  for (Index = 0; Index < Private->CurrentNumberOfConsoles;) {
    Status = Private->TextInList[Index]->ReadKeyStroke (
                                           Private->TextInList[Index],
                                           &KeyData.Key
                                           );
    if (!EFI_ERROR (Status)) {
      //
      // If it is not partial keystorke, return the key. Otherwise, continue
      // to read key from THIS physical console input device.
      //
      if ((KeyData.Key.ScanCode != CHAR_NULL) || (KeyData.Key.UnicodeChar != SCAN_NULL)) {
        CopyMem (Key, &KeyData.Key, sizeof (EFI_INPUT_KEY));
        return Status;
      }
    } else {
      //
      // Continue to read key from NEXT physical console input device.
      //
      Index++;
    }
  }

  return EFI_NOT_READY;
}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  This                     Protocol instance pointer.
  @param  Key                      Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS              The keystroke information was returned.
  @retval EFI_NOT_READY            There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR         The keydtroke information was not returned due
                                   to hardware errors.
  @retval EFI_UNSUPPORTED          The device does not support the ability to read
                                   keystroke data.

**/
EFI_STATUS
EFIAPI
ConSplitterTextInReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  OUT EFI_INPUT_KEY                   *Key
  )
{
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;

  Private = TEXT_IN_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  Private->KeyEventSignalState = FALSE;

  //
  // Signal ConnectConIn event on first call in Lazy ConIn mode
  //
  if (!mConInIsConnect && PcdGetBool (PcdConInConnectOnDemand)) {
    DEBUG ((DEBUG_INFO, "Connect ConIn in first ReadKeyStoke in Lazy ConIn mode.\n"));
    gBS->SignalEvent (Private->ConnectConInEvent);
    mConInIsConnect = TRUE;
  }

  return ConSplitterTextInPrivateReadKeyStroke (Private, Key);
}

/**
  This event aggregates all the events of the ConIn devices in the spliter.

  If any events of physical ConIn devices are signaled, signal the ConIn
  spliter event. This will cause the calling code to call
  ConSplitterTextInReadKeyStroke ().

  @param  Event                    The Event associated with callback.
  @param  Context                  Context registered when Event was created.

**/
VOID
EFIAPI
ConSplitterTextInWaitForKey (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                     Status;
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                          Index;

  Private = (TEXT_IN_SPLITTER_PRIVATE_DATA *)Context;

  if (Private->KeyEventSignalState) {
    //
    // If KeyEventSignalState is flagged before, and not cleared by Reset() or ReadKeyStroke()
    //
    gBS->SignalEvent (Event);
    return;
  }

  //
  // If any physical console input device has key input, signal the event.
  //
  for (Index = 0; Index < Private->CurrentNumberOfConsoles; Index++) {
    Status = gBS->CheckEvent (Private->TextInList[Index]->WaitForKey);
    if (!EFI_ERROR (Status)) {
      gBS->SignalEvent (Event);
      Private->KeyEventSignalState = TRUE;
    }
  }
}

/**
  Test if the key has been registered on input device.

  @param  RegsiteredData           A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   registered.
  @param  InputData                A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   pressed.

  @retval TRUE                     Key be pressed matches a registered key.
  @retval FALSE                    Match failed.

**/
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegsiteredData,
  IN EFI_KEY_DATA  *InputData
  )
{
  ASSERT (RegsiteredData != NULL && InputData != NULL);

  if ((RegsiteredData->Key.ScanCode    != InputData->Key.ScanCode) ||
      (RegsiteredData->Key.UnicodeChar != InputData->Key.UnicodeChar))
  {
    return FALSE;
  }

  //
  // Assume KeyShiftState/KeyToggleState = 0 in Registered key data means these state could be ignored.
  //
  if ((RegsiteredData->KeyState.KeyShiftState != 0) &&
      (RegsiteredData->KeyState.KeyShiftState != InputData->KeyState.KeyShiftState))
  {
    return FALSE;
  }

  if ((RegsiteredData->KeyState.KeyToggleState != 0) &&
      (RegsiteredData->KeyState.KeyToggleState != InputData->KeyState.KeyToggleState))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Reset the input device and optionally run diagnostics

  @param  This                     Protocol instance pointer.
  @param  ExtendedVerification     Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS              The device was reset.
  @retval EFI_DEVICE_ERROR         The device is not functioning properly and could
                                   not be reset.

**/
EFI_STATUS
EFIAPI
ConSplitterTextInResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  )
{
  EFI_STATUS                     Status;
  EFI_STATUS                     ReturnStatus;
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                          Index;

  Private = TEXT_IN_EX_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  Private->KeyEventSignalState = FALSE;

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfExConsoles; Index++) {
    Status = Private->TextInExList[Index]->Reset (
                                             Private->TextInExList[Index],
                                             ExtendedVerification
                                             );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  if (!EFI_ERROR (ReturnStatus)) {
    ToggleStateSyncReInitialization (Private);
    //
    // Empty the key queue.
    //
    Private->CurrentNumberOfKeys = 0;
  }

  return ReturnStatus;
}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  This                     Protocol instance pointer.
  @param  KeyData                  A pointer to a buffer that is filled in with the
                                   keystroke state data for the key that was
                                   pressed.

  @retval EFI_SUCCESS              The keystroke information was returned.
  @retval EFI_NOT_READY            There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR         The keystroke information was not returned due
                                   to hardware errors.
  @retval EFI_INVALID_PARAMETER    KeyData is NULL.
  @retval EFI_UNSUPPORTED          The device does not support the ability to read
                                   keystroke data.

**/
EFI_STATUS
EFIAPI
ConSplitterTextInReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  OUT EFI_KEY_DATA                       *KeyData
  )
{
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  EFI_STATUS                     Status;
  UINTN                          Index;
  EFI_KEY_STATE                  KeyState;
  EFI_KEY_DATA                   CurrentKeyData;

  if (KeyData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = TEXT_IN_EX_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  Private->KeyEventSignalState = FALSE;

  //
  // Signal ConnectConIn event on first call in Lazy ConIn mode
  //
  if (!mConInIsConnect && PcdGetBool (PcdConInConnectOnDemand)) {
    DEBUG ((DEBUG_INFO, "Connect ConIn in first ReadKeyStoke in Lazy ConIn mode.\n"));
    gBS->SignalEvent (Private->ConnectConInEvent);
    mConInIsConnect = TRUE;
  }

  //
  // Return the first saved key.
  //
  Status = ConSplitterTextInExDequeueKey (Private, KeyData);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Private->CurrentNumberOfKeys == 0);

  ZeroMem (&KeyState, sizeof (KeyState));

  //
  // Iterate through all physical consoles to get key state.
  // Some physical consoles may return valid key.
  // Queue the valid keys.
  //
  for (Index = 0; Index < Private->CurrentNumberOfExConsoles; Index++) {
    ZeroMem (&CurrentKeyData, sizeof (EFI_KEY_DATA));
    Status = Private->TextInExList[Index]->ReadKeyStrokeEx (
                                             Private->TextInExList[Index],
                                             &CurrentKeyData
                                             );
    if (EFI_ERROR (Status) && (Status != EFI_NOT_READY)) {
      continue;
    }

    //
    // Consolidate the key state from all physical consoles.
    //
    if ((CurrentKeyData.KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) != 0) {
      KeyState.KeyShiftState |= CurrentKeyData.KeyState.KeyShiftState;
    }

    if ((CurrentKeyData.KeyState.KeyToggleState & EFI_TOGGLE_STATE_VALID) != 0) {
      KeyState.KeyToggleState |= CurrentKeyData.KeyState.KeyToggleState;
    }

    if (!EFI_ERROR (Status)) {
      //
      // If virtual KeyState has been required to be exposed, or it is not
      // partial keystorke, queue the key.
      // It's possible that user presses at multiple keyboards at the same moment,
      // Private->KeyQueue[] are the storage to save all the keys.
      //
      if ((Private->VirtualKeyStateExported) ||
          (CurrentKeyData.Key.ScanCode != CHAR_NULL) ||
          (CurrentKeyData.Key.UnicodeChar != SCAN_NULL))
      {
        CopyMem (
          &Private->KeyQueue[Private->CurrentNumberOfKeys],
          &CurrentKeyData,
          sizeof (EFI_KEY_DATA)
          );
        Private->CurrentNumberOfKeys++;
      }
    }
  }

  //
  // Consolidate the key state for all keys in Private->KeyQueue[]
  //
  for (Index = 0; Index < Private->CurrentNumberOfKeys; Index++) {
    CopyMem (&Private->KeyQueue[Index].KeyState, &KeyState, sizeof (EFI_KEY_STATE));
  }

  //
  // Return the first saved key.
  //
  Status = ConSplitterTextInExDequeueKey (Private, KeyData);
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Always return the key state even there is no key pressed.
  //
  ZeroMem (&KeyData->Key, sizeof (KeyData->Key));
  CopyMem (&KeyData->KeyState, &KeyState, sizeof (KeyData->KeyState));
  return EFI_NOT_READY;
}

/**
  Set certain state for the input device.

  @param  This                     Protocol instance pointer.
  @param  KeyToggleState           A pointer to the EFI_KEY_TOGGLE_STATE to set the
                                   state for the input device.

  @retval EFI_SUCCESS              The device state was set successfully.
  @retval EFI_DEVICE_ERROR         The device is not functioning correctly and
                                   could not have the setting adjusted.
  @retval EFI_UNSUPPORTED          The device does not have the ability to set its
                                   state.
  @retval EFI_INVALID_PARAMETER    KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
ConSplitterTextInSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  )
{
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  EFI_STATUS                     Status;
  UINTN                          Index;
  EFI_KEY_TOGGLE_STATE           PhysicalKeyToggleState;

  if (KeyToggleState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = TEXT_IN_EX_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Always turn on physical TextInEx partial key report for
  // toggle state sync.
  //
  PhysicalKeyToggleState = *KeyToggleState | EFI_KEY_STATE_EXPOSED;

  //
  // if no physical console input device exists, return EFI_SUCCESS;
  // otherwise return the status of setting state of physical console input device
  //
  for (Index = 0; Index < Private->CurrentNumberOfExConsoles; Index++) {
    Status = Private->TextInExList[Index]->SetState (
                                             Private->TextInExList[Index],
                                             &PhysicalKeyToggleState
                                             );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Record the physical KeyToggleState.
  //
  Private->PhysicalKeyToggleState = PhysicalKeyToggleState;
  //
  // Get if virtual KeyState has been required to be exposed.
  //
  Private->VirtualKeyStateExported = (((*KeyToggleState) & EFI_KEY_STATE_EXPOSED) != 0);

  return EFI_SUCCESS;
}

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                     Protocol instance pointer.
  @param  KeyData                  A pointer to a buffer that is filled in with
                                   the keystroke information for the key that was
                                   pressed. If KeyData.Key, KeyData.KeyState.KeyToggleState
                                   and KeyData.KeyState.KeyShiftState are 0, then any incomplete
                                   keystroke will trigger a notification of the KeyNotificationFunction.
  @param  KeyNotificationFunction  Points to the function to be called when the key
                                   sequence is typed specified by KeyData. This notification function
                                   should be called at <=TPL_CALLBACK.
  @param  NotifyHandle             Points to the unique handle assigned to the
                                   registered notification.

  @retval EFI_SUCCESS              The notification function was registered
                                   successfully.
  @retval EFI_OUT_OF_RESOURCES     Unable to allocate resources for necessary data
                                   structures.
  @retval EFI_INVALID_PARAMETER    KeyData or KeyNotificationFunction or NotifyHandle is NULL.

**/
EFI_STATUS
EFIAPI
ConSplitterTextInRegisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_DATA                       *KeyData,
  IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                              **NotifyHandle
  )
{
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  EFI_STATUS                     Status;
  UINTN                          Index;
  TEXT_IN_EX_SPLITTER_NOTIFY     *NewNotify;
  LIST_ENTRY                     *Link;
  TEXT_IN_EX_SPLITTER_NOTIFY     *CurrentNotify;

  if ((KeyData == NULL) || (NotifyHandle == NULL) || (KeyNotificationFunction == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = TEXT_IN_EX_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Return EFI_SUCCESS if the (KeyData, NotificationFunction) is already registered.
  //
  for (Link = Private->NotifyList.ForwardLink; Link != &Private->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = TEXT_IN_EX_SPLITTER_NOTIFY_FROM_THIS (Link);
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
  NewNotify = (TEXT_IN_EX_SPLITTER_NOTIFY *)AllocateZeroPool (sizeof (TEXT_IN_EX_SPLITTER_NOTIFY));
  if (NewNotify == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewNotify->NotifyHandleList = (VOID **)AllocateZeroPool (sizeof (VOID *) *  Private->TextInExListCount);
  if (NewNotify->NotifyHandleList == NULL) {
    gBS->FreePool (NewNotify);
    return EFI_OUT_OF_RESOURCES;
  }

  NewNotify->Signature         = TEXT_IN_EX_SPLITTER_NOTIFY_SIGNATURE;
  NewNotify->KeyNotificationFn = KeyNotificationFunction;
  CopyMem (&NewNotify->KeyData, KeyData, sizeof (EFI_KEY_DATA));

  //
  // Return the wrong status of registering key notify of
  // physical console input device if meet problems
  //
  for (Index = 0; Index < Private->CurrentNumberOfExConsoles; Index++) {
    Status = Private->TextInExList[Index]->RegisterKeyNotify (
                                             Private->TextInExList[Index],
                                             KeyData,
                                             KeyNotificationFunction,
                                             &NewNotify->NotifyHandleList[Index]
                                             );
    if (EFI_ERROR (Status)) {
      //
      // Un-register the key notify on all physical console input devices
      //
      while (Index-- != 0) {
        Private->TextInExList[Index]->UnregisterKeyNotify (
                                        Private->TextInExList[Index],
                                        NewNotify->NotifyHandleList[Index]
                                        );
      }

      gBS->FreePool (NewNotify->NotifyHandleList);
      gBS->FreePool (NewNotify);
      return Status;
    }
  }

  InsertTailList (&Private->NotifyList, &NewNotify->NotifyEntry);

  *NotifyHandle = NewNotify;

  return EFI_SUCCESS;
}

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                     Protocol instance pointer.
  @param  NotificationHandle       The handle of the notification function being
                                   unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered
                                   successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid.

**/
EFI_STATUS
EFIAPI
ConSplitterTextInUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  )
{
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                          Index;
  TEXT_IN_EX_SPLITTER_NOTIFY     *CurrentNotify;
  LIST_ENTRY                     *Link;

  if (NotificationHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = TEXT_IN_EX_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  for (Link = Private->NotifyList.ForwardLink; Link != &Private->NotifyList; Link = Link->ForwardLink) {
    CurrentNotify = TEXT_IN_EX_SPLITTER_NOTIFY_FROM_THIS (Link);
    if (CurrentNotify == NotificationHandle) {
      for (Index = 0; Index < Private->CurrentNumberOfExConsoles; Index++) {
        Private->TextInExList[Index]->UnregisterKeyNotify (
                                        Private->TextInExList[Index],
                                        CurrentNotify->NotifyHandleList[Index]
                                        );
      }

      RemoveEntryList (&CurrentNotify->NotifyEntry);

      gBS->FreePool (CurrentNotify->NotifyHandleList);
      gBS->FreePool (CurrentNotify);
      return EFI_SUCCESS;
    }
  }

  //
  // NotificationHandle is not found in database
  //
  return EFI_INVALID_PARAMETER;
}

/**
  Reset the input device and optionally run diagnostics

  @param  This                     Protocol instance pointer.
  @param  ExtendedVerification     Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS              The device was reset.
  @retval EFI_DEVICE_ERROR         The device is not functioning properly and could
                                   not be reset.

**/
EFI_STATUS
EFIAPI
ConSplitterSimplePointerReset (
  IN  EFI_SIMPLE_POINTER_PROTOCOL  *This,
  IN  BOOLEAN                      ExtendedVerification
  )
{
  EFI_STATUS                     Status;
  EFI_STATUS                     ReturnStatus;
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                          Index;

  Private = TEXT_IN_SPLITTER_PRIVATE_DATA_FROM_SIMPLE_POINTER_THIS (This);

  Private->InputEventSignalState = FALSE;

  if (Private->CurrentNumberOfPointers == 0) {
    return EFI_SUCCESS;
  }

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfPointers; Index++) {
    Status = Private->PointerList[Index]->Reset (
                                            Private->PointerList[Index],
                                            ExtendedVerification
                                            );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  return ReturnStatus;
}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existence of a keystroke via WaitForEvent () call.

  @param  Private                  Protocol instance pointer.
  @param  State                    The state information of simple pointer device.

  @retval EFI_SUCCESS              The keystroke information was returned.
  @retval EFI_NOT_READY            There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR         The keydtroke information was not returned due
                                   to hardware errors.

**/
EFI_STATUS
EFIAPI
ConSplitterSimplePointerPrivateGetState (
  IN  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private,
  IN OUT EFI_SIMPLE_POINTER_STATE    *State
  )
{
  EFI_STATUS                Status;
  EFI_STATUS                ReturnStatus;
  UINTN                     Index;
  EFI_SIMPLE_POINTER_STATE  CurrentState;

  State->RelativeMovementX = 0;
  State->RelativeMovementY = 0;
  State->RelativeMovementZ = 0;
  State->LeftButton        = FALSE;
  State->RightButton       = FALSE;

  //
  // if no physical console input device exists, return EFI_NOT_READY;
  // if any physical console input device has key input,
  // return the key and EFI_SUCCESS.
  //
  ReturnStatus = EFI_NOT_READY;
  for (Index = 0; Index < Private->CurrentNumberOfPointers; Index++) {
    Status = Private->PointerList[Index]->GetState (
                                            Private->PointerList[Index],
                                            &CurrentState
                                            );
    if (!EFI_ERROR (Status)) {
      if (ReturnStatus == EFI_NOT_READY) {
        ReturnStatus = EFI_SUCCESS;
      }

      if (CurrentState.LeftButton) {
        State->LeftButton = TRUE;
      }

      if (CurrentState.RightButton) {
        State->RightButton = TRUE;
      }

      if ((CurrentState.RelativeMovementX != 0) && (Private->PointerList[Index]->Mode->ResolutionX != 0)) {
        State->RelativeMovementX += (CurrentState.RelativeMovementX * (INT32)Private->SimplePointerMode.ResolutionX) / (INT32)Private->PointerList[Index]->Mode->ResolutionX;
      }

      if ((CurrentState.RelativeMovementY != 0) && (Private->PointerList[Index]->Mode->ResolutionY != 0)) {
        State->RelativeMovementY += (CurrentState.RelativeMovementY * (INT32)Private->SimplePointerMode.ResolutionY) / (INT32)Private->PointerList[Index]->Mode->ResolutionY;
      }

      if ((CurrentState.RelativeMovementZ != 0) && (Private->PointerList[Index]->Mode->ResolutionZ != 0)) {
        State->RelativeMovementZ += (CurrentState.RelativeMovementZ * (INT32)Private->SimplePointerMode.ResolutionZ) / (INT32)Private->PointerList[Index]->Mode->ResolutionZ;
      }
    } else if (Status == EFI_DEVICE_ERROR) {
      ReturnStatus = EFI_DEVICE_ERROR;
    }
  }

  return ReturnStatus;
}

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existance of a keystroke via WaitForEvent () call.

  @param  This                     A pointer to protocol instance.
  @param  State                    A pointer to state information on the pointer device

  @retval EFI_SUCCESS              The keystroke information was returned in State.
  @retval EFI_NOT_READY            There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR         The keydtroke information was not returned due
                                   to hardware errors.

**/
EFI_STATUS
EFIAPI
ConSplitterSimplePointerGetState (
  IN  EFI_SIMPLE_POINTER_PROTOCOL  *This,
  IN OUT EFI_SIMPLE_POINTER_STATE  *State
  )
{
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;

  Private = TEXT_IN_SPLITTER_PRIVATE_DATA_FROM_SIMPLE_POINTER_THIS (This);

  Private->InputEventSignalState = FALSE;

  return ConSplitterSimplePointerPrivateGetState (Private, State);
}

/**
  This event aggregates all the events of the ConIn devices in the spliter.
  If any events of physical ConIn devices are signaled, signal the ConIn
  spliter event. This will cause the calling code to call
  ConSplitterTextInReadKeyStroke ().

  @param  Event                    The Event associated with callback.
  @param  Context                  Context registered when Event was created.

**/
VOID
EFIAPI
ConSplitterSimplePointerWaitForInput (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                     Status;
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                          Index;

  Private = (TEXT_IN_SPLITTER_PRIVATE_DATA *)Context;

  //
  // if InputEventSignalState is flagged before, and not cleared by Reset() or ReadKeyStroke()
  //
  if (Private->InputEventSignalState) {
    gBS->SignalEvent (Event);
    return;
  }

  //
  // if any physical console input device has key input, signal the event.
  //
  for (Index = 0; Index < Private->CurrentNumberOfPointers; Index++) {
    Status = gBS->CheckEvent (Private->PointerList[Index]->WaitForInput);
    if (!EFI_ERROR (Status)) {
      gBS->SignalEvent (Event);
      Private->InputEventSignalState = TRUE;
    }
  }
}

/**
  Resets the pointer device hardware.

  @param  This                     Protocol instance pointer.
  @param  ExtendedVerification     Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS              The device was reset.
  @retval EFI_DEVICE_ERROR         The device is not functioning correctly and
                                   could not be reset.

**/
EFI_STATUS
EFIAPI
ConSplitterAbsolutePointerReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  EFI_STATUS                     Status;
  EFI_STATUS                     ReturnStatus;
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                          Index;

  Private = TEXT_IN_SPLITTER_PRIVATE_DATA_FROM_ABSOLUTE_POINTER_THIS (This);

  Private->AbsoluteInputEventSignalState = FALSE;

  if (Private->CurrentNumberOfAbsolutePointers == 0) {
    return EFI_SUCCESS;
  }

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfAbsolutePointers; Index++) {
    Status = Private->AbsolutePointerList[Index]->Reset (
                                                    Private->AbsolutePointerList[Index],
                                                    ExtendedVerification
                                                    );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  return ReturnStatus;
}

/**
  Retrieves the current state of a pointer device.

  @param  This                     Protocol instance pointer.
  @param  State                    A pointer to the state information on the
                                   pointer device.

  @retval EFI_SUCCESS              The state of the pointer device was returned in
                                   State..
  @retval EFI_NOT_READY            The state of the pointer device has not changed
                                   since the last call to GetState().
  @retval EFI_DEVICE_ERROR         A device error occurred while attempting to
                                   retrieve the pointer device's current state.

**/
EFI_STATUS
EFIAPI
ConSplitterAbsolutePointerGetState (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL   *This,
  IN OUT EFI_ABSOLUTE_POINTER_STATE  *State
  )
{
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  EFI_STATUS                     Status;
  EFI_STATUS                     ReturnStatus;
  UINTN                          Index;
  EFI_ABSOLUTE_POINTER_STATE     CurrentState;
  UINT64                         MinX;
  UINT64                         MinY;
  UINT64                         MinZ;
  UINT64                         MaxX;
  UINT64                         MaxY;
  UINT64                         MaxZ;
  UINT64                         VirtualMinX;
  UINT64                         VirtualMinY;
  UINT64                         VirtualMinZ;
  UINT64                         VirtualMaxX;
  UINT64                         VirtualMaxY;
  UINT64                         VirtualMaxZ;

  Private = TEXT_IN_SPLITTER_PRIVATE_DATA_FROM_ABSOLUTE_POINTER_THIS (This);

  Private->AbsoluteInputEventSignalState = FALSE;

  State->CurrentX      = 0;
  State->CurrentY      = 0;
  State->CurrentZ      = 0;
  State->ActiveButtons = 0;

  VirtualMinX = Private->AbsolutePointerMode.AbsoluteMinX;
  VirtualMinY = Private->AbsolutePointerMode.AbsoluteMinY;
  VirtualMinZ = Private->AbsolutePointerMode.AbsoluteMinZ;
  VirtualMaxX = Private->AbsolutePointerMode.AbsoluteMaxX;
  VirtualMaxY = Private->AbsolutePointerMode.AbsoluteMaxY;
  VirtualMaxZ = Private->AbsolutePointerMode.AbsoluteMaxZ;

  //
  // if no physical pointer device exists, return EFI_NOT_READY;
  // if any physical pointer device has changed state,
  // return the state and EFI_SUCCESS.
  //
  ReturnStatus = EFI_NOT_READY;
  for (Index = 0; Index < Private->CurrentNumberOfAbsolutePointers; Index++) {
    Status = Private->AbsolutePointerList[Index]->GetState (
                                                    Private->AbsolutePointerList[Index],
                                                    &CurrentState
                                                    );
    if (!EFI_ERROR (Status)) {
      if (ReturnStatus == EFI_NOT_READY) {
        ReturnStatus = EFI_SUCCESS;
      }

      MinX = Private->AbsolutePointerList[Index]->Mode->AbsoluteMinX;
      MinY = Private->AbsolutePointerList[Index]->Mode->AbsoluteMinY;
      MinZ = Private->AbsolutePointerList[Index]->Mode->AbsoluteMinZ;
      MaxX = Private->AbsolutePointerList[Index]->Mode->AbsoluteMaxX;
      MaxY = Private->AbsolutePointerList[Index]->Mode->AbsoluteMaxY;
      MaxZ = Private->AbsolutePointerList[Index]->Mode->AbsoluteMaxZ;

      State->ActiveButtons = CurrentState.ActiveButtons;

      //
      // Rescale to Con Splitter virtual Absolute Pointer's resolution.
      //
      if (!((MinX == 0) && (MaxX == 0))) {
        State->CurrentX = VirtualMinX + DivU64x64Remainder (
                                          MultU64x64 (
                                            CurrentState.CurrentX,
                                            VirtualMaxX - VirtualMinX
                                            ),
                                          MaxX - MinX,
                                          NULL
                                          );
      }

      if (!((MinY == 0) && (MaxY == 0))) {
        State->CurrentY = VirtualMinY + DivU64x64Remainder (
                                          MultU64x64 (
                                            CurrentState.CurrentY,
                                            VirtualMaxY - VirtualMinY
                                            ),
                                          MaxY - MinY,
                                          NULL
                                          );
      }

      if (!((MinZ == 0) && (MaxZ == 0))) {
        State->CurrentZ = VirtualMinZ + DivU64x64Remainder (
                                          MultU64x64 (
                                            CurrentState.CurrentZ,
                                            VirtualMaxZ - VirtualMinZ
                                            ),
                                          MaxZ - MinZ,
                                          NULL
                                          );
      }
    } else if (Status == EFI_DEVICE_ERROR) {
      ReturnStatus = EFI_DEVICE_ERROR;
    }
  }

  return ReturnStatus;
}

/**
  This event aggregates all the events of the pointer devices in the splitter.
  If any events of physical pointer devices are signaled, signal the pointer
  splitter event. This will cause the calling code to call
  ConSplitterAbsolutePointerGetState ().

  @param  Event                    The Event associated with callback.
  @param  Context                  Context registered when Event was created.

**/
VOID
EFIAPI
ConSplitterAbsolutePointerWaitForInput (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                     Status;
  TEXT_IN_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                          Index;

  Private = (TEXT_IN_SPLITTER_PRIVATE_DATA *)Context;

  //
  // if AbsoluteInputEventSignalState is flagged before,
  // and not cleared by Reset() or GetState(), signal it
  //
  if (Private->AbsoluteInputEventSignalState) {
    gBS->SignalEvent (Event);
    return;
  }

  //
  // if any physical console input device has key input, signal the event.
  //
  for (Index = 0; Index < Private->CurrentNumberOfAbsolutePointers; Index++) {
    Status = gBS->CheckEvent (Private->AbsolutePointerList[Index]->WaitForInput);
    if (!EFI_ERROR (Status)) {
      gBS->SignalEvent (Event);
      Private->AbsoluteInputEventSignalState = TRUE;
    }
  }
}

/**
  Reset the text output device hardware and optionally run diagnostics

  @param  This                     Protocol instance pointer.
  @param  ExtendedVerification     Driver may perform more exhaustive verification
                                   operation of the device during reset.

  @retval EFI_SUCCESS              The text output device was reset.
  @retval EFI_DEVICE_ERROR         The text output device is not functioning
                                   correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
ConSplitterTextOutReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          ExtendedVerification
  )
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_STATUS                      ReturnStatus;

  Private = TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfConsoles; Index++) {
    Status = Private->TextOutList[Index].TextOut->Reset (
                                                    Private->TextOutList[Index].TextOut,
                                                    ExtendedVerification
                                                    );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  This->SetAttribute (This, EFI_TEXT_ATTR (This->Mode->Attribute & 0x0F, EFI_BLACK));

  //
  // reset all mode parameters
  //
  TextOutSetMode (Private, 0);

  return ReturnStatus;
}

/**
  Write a Unicode string to the output device.

  @param  This                     Protocol instance pointer.
  @param  WString                  The NULL-terminated Unicode string to be
                                   displayed on the output device(s). All output
                                   devices must also support the Unicode drawing
                                   defined in this file.

  @retval EFI_SUCCESS              The string was output to the device.
  @retval EFI_DEVICE_ERROR         The device reported an error while attempting to
                                   output the text.
  @retval EFI_UNSUPPORTED          The output device's mode is not currently in a
                                   defined text mode.
  @retval EFI_WARN_UNKNOWN_GLYPH   This warning code indicates that some of the
                                   characters in the Unicode string could not be
                                   rendered and were skipped.

**/
EFI_STATUS
EFIAPI
ConSplitterTextOutOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_STATUS                      ReturnStatus;
  UINTN                           MaxColumn;
  UINTN                           MaxRow;

  This->SetAttribute (This, This->Mode->Attribute);

  Private = TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfConsoles; Index++) {
    Status = Private->TextOutList[Index].TextOut->OutputString (
                                                    Private->TextOutList[Index].TextOut,
                                                    WString
                                                    );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  if (Private->CurrentNumberOfConsoles > 0) {
    Private->TextOutMode.CursorColumn = Private->TextOutList[0].TextOut->Mode->CursorColumn;
    Private->TextOutMode.CursorRow    = Private->TextOutList[0].TextOut->Mode->CursorRow;
  } else {
    //
    // When there is no real console devices in system,
    // update cursor position for the virtual device in consplitter.
    //
    Private->TextOut.QueryMode (
                       &Private->TextOut,
                       Private->TextOutMode.Mode,
                       &MaxColumn,
                       &MaxRow
                       );
    for ( ; *WString != CHAR_NULL; WString++) {
      switch (*WString) {
        case CHAR_BACKSPACE:
          if ((Private->TextOutMode.CursorColumn == 0) && (Private->TextOutMode.CursorRow > 0)) {
            Private->TextOutMode.CursorRow--;
            Private->TextOutMode.CursorColumn = (INT32)(MaxColumn - 1);
          } else if (Private->TextOutMode.CursorColumn > 0) {
            Private->TextOutMode.CursorColumn--;
          }

          break;

        case CHAR_LINEFEED:
          if (Private->TextOutMode.CursorRow < (INT32)(MaxRow - 1)) {
            Private->TextOutMode.CursorRow++;
          }

          break;

        case CHAR_CARRIAGE_RETURN:
          Private->TextOutMode.CursorColumn = 0;
          break;

        default:
          if (Private->TextOutMode.CursorColumn < (INT32)(MaxColumn - 1)) {
            Private->TextOutMode.CursorColumn++;
          } else {
            Private->TextOutMode.CursorColumn = 0;
            if (Private->TextOutMode.CursorRow < (INT32)(MaxRow - 1)) {
              Private->TextOutMode.CursorRow++;
            }
          }

          break;
      }
    }
  }

  return ReturnStatus;
}

/**
  Verifies that all characters in a Unicode string can be output to the
  target device.

  @param  This                     Protocol instance pointer.
  @param  WString                  The NULL-terminated Unicode string to be
                                   examined for the output device(s).

  @retval EFI_SUCCESS              The device(s) are capable of rendering the
                                   output string.
  @retval EFI_UNSUPPORTED          Some of the characters in the Unicode string
                                   cannot be rendered by one or more of the output
                                   devices mapped by the EFI handle.

**/
EFI_STATUS
EFIAPI
ConSplitterTextOutTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_STATUS                      ReturnStatus;

  Private = TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfConsoles; Index++) {
    Status = Private->TextOutList[Index].TextOut->TestString (
                                                    Private->TextOutList[Index].TextOut,
                                                    WString
                                                    );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  //
  // There is no DevNullTextOutTestString () since a Unicode buffer would
  // always return EFI_SUCCESS.
  // ReturnStatus will be EFI_SUCCESS if no consoles are present
  //
  return ReturnStatus;
}

/**
  Returns information for an available text mode that the output device(s)
  supports.

  @param  This                     Protocol instance pointer.
  @param  ModeNumber               The mode number to return information on.
  @param  Columns                  Returns the columns of the text output device
                                   for the requested ModeNumber.
  @param  Rows                     Returns the rows of the text output device
                                   for the requested ModeNumber.

  @retval EFI_SUCCESS              The requested mode information was returned.
  @retval EFI_DEVICE_ERROR         The device had an error and could not complete
                                   the request.
  @retval EFI_UNSUPPORTED          The mode number was not valid.

**/
EFI_STATUS
EFIAPI
ConSplitterTextOutQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  )
{
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           CurrentMode;
  INT32                           *TextOutModeMap;

  Private = TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Check whether param ModeNumber is valid.
  // ModeNumber should be within range 0 ~ MaxMode - 1.
  //
  if ((ModeNumber > (UINTN)(((UINT32)-1)>>1))) {
    return EFI_UNSUPPORTED;
  }

  if ((INT32)ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  //
  // We get the available mode from mode intersection map if it's available
  //
  if (Private->TextOutModeMap != NULL) {
    TextOutModeMap = Private->TextOutModeMap + Private->TextOutListCount * ModeNumber;
    CurrentMode    = (UINTN)(*TextOutModeMap);
    *Columns       = Private->TextOutQueryData[CurrentMode].Columns;
    *Rows          = Private->TextOutQueryData[CurrentMode].Rows;
  } else {
    *Columns = Private->TextOutQueryData[ModeNumber].Columns;
    *Rows    = Private->TextOutQueryData[ModeNumber].Rows;
  }

  if ((*Columns <= 0) && (*Rows <= 0)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Sets the output device(s) to a specified mode.

  @param  This                     Protocol instance pointer.
  @param  ModeNumber               The mode number to set.

  @retval EFI_SUCCESS              The requested text mode was set.
  @retval EFI_DEVICE_ERROR         The device had an error and could not complete
                                   the request.
  @retval EFI_UNSUPPORTED          The mode number was not valid.

**/
EFI_STATUS
EFIAPI
ConSplitterTextOutSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  )
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  INT32                           *TextOutModeMap;
  EFI_STATUS                      ReturnStatus;

  Private = TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Check whether param ModeNumber is valid.
  // ModeNumber should be within range 0 ~ MaxMode - 1.
  //
  if ((ModeNumber > (UINTN)(((UINT32)-1)>>1))) {
    return EFI_UNSUPPORTED;
  }

  if ((INT32)ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  //
  // If the mode is being set to the curent mode, then just clear the screen and return.
  //
  if (Private->TextOutMode.Mode == (INT32)ModeNumber) {
    return ConSplitterTextOutClearScreen (This);
  }

  //
  // return the worst status met
  //
  TextOutModeMap = Private->TextOutModeMap + Private->TextOutListCount * ModeNumber;
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfConsoles; Index++) {
    //
    // While adding a console out device do not set same mode again for the same device.
    //
    if ((!Private->AddingConOutDevice) ||
        (TextOutModeMap[Index] != Private->TextOutList[Index].TextOut->Mode->Mode))
    {
      Status = Private->TextOutList[Index].TextOut->SetMode (
                                                      Private->TextOutList[Index].TextOut,
                                                      TextOutModeMap[Index]
                                                      );
      if (EFI_ERROR (Status)) {
        ReturnStatus = Status;
      }
    }
  }

  //
  // Set mode parameter to specified mode number
  //
  TextOutSetMode (Private, ModeNumber);

  return ReturnStatus;
}

/**
  Sets the background and foreground colors for the OutputString () and
  ClearScreen () functions.

  @param  This                     Protocol instance pointer.
  @param  Attribute                The attribute to set. Bits 0..3 are the
                                   foreground color, and bits 4..6 are the
                                   background color. All other bits are undefined
                                   and must be zero. The valid Attributes are
                                   defined in this file.

  @retval EFI_SUCCESS              The attribute was set.
  @retval EFI_DEVICE_ERROR         The device had an error and could not complete
                                   the request.
  @retval EFI_UNSUPPORTED          The attribute requested is not defined.

**/
EFI_STATUS
EFIAPI
ConSplitterTextOutSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
  )
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_STATUS                      ReturnStatus;

  Private = TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // Check whether param Attribute is valid.
  //
  if ((Attribute | 0x7F) != 0x7F) {
    return EFI_UNSUPPORTED;
  }

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfConsoles; Index++) {
    Status = Private->TextOutList[Index].TextOut->SetAttribute (
                                                    Private->TextOutList[Index].TextOut,
                                                    Attribute
                                                    );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  Private->TextOutMode.Attribute = (INT32)Attribute;

  return ReturnStatus;
}

/**
  Clears the output device(s) display to the currently selected background
  color.

  @param  This                     Protocol instance pointer.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_DEVICE_ERROR         The device had an error and could not complete
                                   the request.
  @retval EFI_UNSUPPORTED          The output device is not in a valid text mode.

**/
EFI_STATUS
EFIAPI
ConSplitterTextOutClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_STATUS                      ReturnStatus;

  Private = TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfConsoles; Index++) {
    Status = Private->TextOutList[Index].TextOut->ClearScreen (Private->TextOutList[Index].TextOut);
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  //
  // No need to do extra check here as whether (Column, Row) is valid has
  // been checked in ConSplitterTextOutSetCursorPosition. And (0, 0) should
  // always be supported.
  //
  Private->TextOutMode.CursorColumn  = 0;
  Private->TextOutMode.CursorRow     = 0;
  Private->TextOutMode.CursorVisible = TRUE;

  return ReturnStatus;
}

/**
  Sets the current coordinates of the cursor position

  @param  This                     Protocol instance pointer.
  @param  Column                   The column position to set the cursor to. Must be
                                   greater than or equal to zero and less than the
                                   number of columns by QueryMode ().
  @param  Row                      The row position to set the cursor to. Must be
                                   greater than or equal to zero and less than the
                                   number of rows by QueryMode ().

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_DEVICE_ERROR         The device had an error and could not complete
                                   the request.
  @retval EFI_UNSUPPORTED          The output device is not in a valid text mode,
                                   or the cursor position is invalid for the
                                   current mode.

**/
EFI_STATUS
EFIAPI
ConSplitterTextOutSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Column,
  IN  UINTN                            Row
  )
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_STATUS                      ReturnStatus;
  UINTN                           MaxColumn;
  UINTN                           MaxRow;
  INT32                           *TextOutModeMap;
  INT32                           ModeNumber;
  INT32                           CurrentMode;

  Private        = TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);
  TextOutModeMap = NULL;
  ModeNumber     = Private->TextOutMode.Mode;

  //
  // Get current MaxColumn and MaxRow from intersection map
  //
  if (Private->TextOutModeMap != NULL) {
    TextOutModeMap = Private->TextOutModeMap + Private->TextOutListCount * ModeNumber;
    CurrentMode    = *TextOutModeMap;
  } else {
    CurrentMode = ModeNumber;
  }

  MaxColumn = Private->TextOutQueryData[CurrentMode].Columns;
  MaxRow    = Private->TextOutQueryData[CurrentMode].Rows;

  if ((Column >= MaxColumn) || (Row >= MaxRow)) {
    return EFI_UNSUPPORTED;
  }

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfConsoles; Index++) {
    Status = Private->TextOutList[Index].TextOut->SetCursorPosition (
                                                    Private->TextOutList[Index].TextOut,
                                                    Column,
                                                    Row
                                                    );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  //
  // No need to do extra check here as whether (Column, Row) is valid has
  // been checked in ConSplitterTextOutSetCursorPosition. And (0, 0) should
  // always be supported.
  //
  Private->TextOutMode.CursorColumn = (INT32)Column;
  Private->TextOutMode.CursorRow    = (INT32)Row;

  return ReturnStatus;
}

/**
  Makes the cursor visible or invisible

  @param  This                     Protocol instance pointer.
  @param  Visible                  If TRUE, the cursor is set to be visible. If
                                   FALSE, the cursor is set to be invisible.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_DEVICE_ERROR         The device had an error and could not complete
                                   the request, or the device does not support
                                   changing the cursor mode.
  @retval EFI_UNSUPPORTED          The output device is not in a valid text mode.

**/
EFI_STATUS
EFIAPI
ConSplitterTextOutEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  )
{
  EFI_STATUS                      Status;
  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private;
  UINTN                           Index;
  EFI_STATUS                      ReturnStatus;

  Private = TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS (This);

  //
  // return the worst status met
  //
  for (Index = 0, ReturnStatus = EFI_SUCCESS; Index < Private->CurrentNumberOfConsoles; Index++) {
    Status = Private->TextOutList[Index].TextOut->EnableCursor (
                                                    Private->TextOutList[Index].TextOut,
                                                    Visible
                                                    );
    if (EFI_ERROR (Status)) {
      ReturnStatus = Status;
    }
  }

  Private->TextOutMode.CursorVisible = Visible;

  return ReturnStatus;
}
