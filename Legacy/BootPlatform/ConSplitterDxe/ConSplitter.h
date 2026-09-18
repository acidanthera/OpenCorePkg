/** @file
  Private data structures for the Console Splitter driver

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CON_SPLITTER_H_
#define _CON_SPLITTER_H_

#include <Uefi.h>
#include <PiDxe.h>

#include <Protocol/DevicePath.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/AbsolutePointer.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>

#include <Guid/ConsoleInDevice.h>
#include <Guid/StandardErrorDevice.h>
#include <Guid/ConsoleOutDevice.h>
#include <Guid/ConnectConInEvent.h>

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// Driver Binding Externs
//
extern EFI_DRIVER_BINDING_PROTOCOL   gConSplitterConInDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gConSplitterConInComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gConSplitterConInComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL   gConSplitterSimplePointerDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gConSplitterSimplePointerComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gConSplitterSimplePointerComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL   gConSplitterAbsolutePointerDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gConSplitterAbsolutePointerComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gConSplitterAbsolutePointerComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL   gConSplitterConOutDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gConSplitterConOutComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gConSplitterConOutComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL   gConSplitterStdErrDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gConSplitterStdErrComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gConSplitterStdErrComponentName2;

//
// These definitions were in the old Hii protocol, but are not in the new UEFI
// version. So they are defined locally.
//
#define UNICODE_NARROW_CHAR  0xFFF0
#define UNICODE_WIDE_CHAR    0xFFF1

//
// Private Data Structures
//
#define CONSOLE_SPLITTER_ALLOC_UNIT  32

typedef struct {
  UINTN    Column;
  UINTN    Row;
} CONSOLE_OUT_MODE;

typedef struct {
  UINTN    Columns;
  UINTN    Rows;
} TEXT_OUT_SPLITTER_QUERY_DATA;

#define KEY_STATE_VALID_EXPOSED  (EFI_TOGGLE_STATE_VALID | EFI_KEY_STATE_EXPOSED)

#define TEXT_IN_EX_SPLITTER_NOTIFY_SIGNATURE  SIGNATURE_32 ('T', 'i', 'S', 'n')

//
// Private data for Text In Ex Splitter Notify
//
typedef struct _TEXT_IN_EX_SPLITTER_NOTIFY {
  UINTN                      Signature;
  VOID                       **NotifyHandleList;
  EFI_KEY_DATA               KeyData;
  EFI_KEY_NOTIFY_FUNCTION    KeyNotificationFn;
  LIST_ENTRY                 NotifyEntry;
} TEXT_IN_EX_SPLITTER_NOTIFY;

#define TEXT_IN_EX_SPLITTER_NOTIFY_FROM_THIS(a)  \
  CR ((a),                                       \
      TEXT_IN_EX_SPLITTER_NOTIFY,                \
      NotifyEntry,                               \
      TEXT_IN_EX_SPLITTER_NOTIFY_SIGNATURE       \
      )

#define TEXT_IN_SPLITTER_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('T', 'i', 'S', 'p')

//
// Private data for the Console In splitter
//
typedef struct {
  UINT64                               Signature;
  EFI_HANDLE                           VirtualHandle;

  EFI_SIMPLE_TEXT_INPUT_PROTOCOL       TextIn;
  UINTN                                CurrentNumberOfConsoles;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL       **TextInList;
  UINTN                                TextInListCount;

  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    TextInEx;
  UINTN                                CurrentNumberOfExConsoles;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    **TextInExList;
  UINTN                                TextInExListCount;
  LIST_ENTRY                           NotifyList;
  EFI_KEY_DATA                         *KeyQueue;
  UINTN                                CurrentNumberOfKeys;
  //
  // It will be initialized and synced between console input devices
  // for toggle state sync.
  //
  EFI_KEY_TOGGLE_STATE                 PhysicalKeyToggleState;
  //
  // It will be initialized and used to record if virtual KeyState
  // has been required to be exposed.
  //
  BOOLEAN                              VirtualKeyStateExported;

  EFI_SIMPLE_POINTER_PROTOCOL          SimplePointer;
  EFI_SIMPLE_POINTER_MODE              SimplePointerMode;
  UINTN                                CurrentNumberOfPointers;
  EFI_SIMPLE_POINTER_PROTOCOL          **PointerList;
  UINTN                                PointerListCount;

  EFI_ABSOLUTE_POINTER_PROTOCOL        AbsolutePointer;
  EFI_ABSOLUTE_POINTER_MODE            AbsolutePointerMode;
  UINTN                                CurrentNumberOfAbsolutePointers;
  EFI_ABSOLUTE_POINTER_PROTOCOL        **AbsolutePointerList;
  UINTN                                AbsolutePointerListCount;
  BOOLEAN                              AbsoluteInputEventSignalState;

  BOOLEAN                              KeyEventSignalState;
  BOOLEAN                              InputEventSignalState;
  EFI_EVENT                            ConnectConInEvent;
} TEXT_IN_SPLITTER_PRIVATE_DATA;

#define TEXT_IN_SPLITTER_PRIVATE_DATA_FROM_THIS(a)  \
  CR ((a),                                          \
      TEXT_IN_SPLITTER_PRIVATE_DATA,                \
      TextIn,                                       \
      TEXT_IN_SPLITTER_PRIVATE_DATA_SIGNATURE       \
      )

#define TEXT_IN_SPLITTER_PRIVATE_DATA_FROM_SIMPLE_POINTER_THIS(a) \
  CR ((a),                                                        \
      TEXT_IN_SPLITTER_PRIVATE_DATA,                              \
      SimplePointer,                                              \
      TEXT_IN_SPLITTER_PRIVATE_DATA_SIGNATURE                     \
      )
#define TEXT_IN_EX_SPLITTER_PRIVATE_DATA_FROM_THIS(a) \
  CR (a,                                              \
      TEXT_IN_SPLITTER_PRIVATE_DATA,                  \
      TextInEx,                                       \
      TEXT_IN_SPLITTER_PRIVATE_DATA_SIGNATURE         \
      )

#define TEXT_IN_SPLITTER_PRIVATE_DATA_FROM_ABSOLUTE_POINTER_THIS(a) \
  CR (a,                                                            \
      TEXT_IN_SPLITTER_PRIVATE_DATA,                                \
      AbsolutePointer,                                              \
      TEXT_IN_SPLITTER_PRIVATE_DATA_SIGNATURE                       \
      )

#define TEXT_OUT_SPLITTER_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('T', 'o', 'S', 'p')

typedef struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL              *UgaDraw;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *TextOut;
} TEXT_OUT_AND_GOP_DATA;

//
// Private data for the Console Out splitter
//
typedef struct {
  UINT64                                  Signature;
  EFI_HANDLE                              VirtualHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL         TextOut;
  EFI_SIMPLE_TEXT_OUTPUT_MODE             TextOutMode;

  EFI_UGA_DRAW_PROTOCOL                   UgaDraw;
  UINT32                                  UgaHorizontalResolution;
  UINT32                                  UgaVerticalResolution;
  UINT32                                  UgaColorDepth;
  UINT32                                  UgaRefreshRate;

  EFI_GRAPHICS_OUTPUT_PROTOCOL            GraphicsOutput;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION    *GraphicsOutputModeBuffer;
  UINTN                                   CurrentNumberOfGraphicsOutput;
  UINTN                                   CurrentNumberOfUgaDraw;

  UINTN                                   CurrentNumberOfConsoles;
  TEXT_OUT_AND_GOP_DATA                   *TextOutList;
  UINTN                                   TextOutListCount;
  TEXT_OUT_SPLITTER_QUERY_DATA            *TextOutQueryData;
  UINTN                                   TextOutQueryDataCount;
  INT32                                   *TextOutModeMap;

  BOOLEAN                                 AddingConOutDevice;
} TEXT_OUT_SPLITTER_PRIVATE_DATA;

#define TEXT_OUT_SPLITTER_PRIVATE_DATA_FROM_THIS(a) \
  CR ((a),                                          \
      TEXT_OUT_SPLITTER_PRIVATE_DATA,               \
      TextOut,                                      \
      TEXT_OUT_SPLITTER_PRIVATE_DATA_SIGNATURE      \
      )

#define GRAPHICS_OUTPUT_SPLITTER_PRIVATE_DATA_FROM_THIS(a)  \
  CR ((a),                                                  \
      TEXT_OUT_SPLITTER_PRIVATE_DATA,                       \
      GraphicsOutput,                                       \
      TEXT_OUT_SPLITTER_PRIVATE_DATA_SIGNATURE              \
      )

#define UGA_DRAW_SPLITTER_PRIVATE_DATA_FROM_THIS(a) \
  CR ((a),                                          \
      TEXT_OUT_SPLITTER_PRIVATE_DATA,               \
      UgaDraw,                                      \
      TEXT_OUT_SPLITTER_PRIVATE_DATA_SIGNATURE      \
      )

#define CONSOLE_CONTROL_SPLITTER_PRIVATE_DATA_FROM_THIS(a)  \
  CR ((a),                                                  \
      TEXT_OUT_SPLITTER_PRIVATE_DATA,                       \
      ConsoleControl,                                       \
      TEXT_OUT_SPLITTER_PRIVATE_DATA_SIGNATURE              \
      )

//
// Function Prototypes
//

/**
  The user Entry Point for module ConSplitter. The user code starts with this function.

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
  );

/**
  Construct console input devices' private data.

  @param  ConInPrivate             A pointer to the TEXT_IN_SPLITTER_PRIVATE_DATA
                                   structure.

  @retval EFI_OUT_OF_RESOURCES     Out of resources.
  @retval EFI_SUCCESS              Text Input Devcie's private data has been constructed.
  @retval other                    Failed to construct private data.

**/
EFI_STATUS
ConSplitterTextInConstructor (
  TEXT_IN_SPLITTER_PRIVATE_DATA  *ConInPrivate
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Stop Console In ConSplitter on ControllerHandle by closing Console In Devcice GUID.

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
  );

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
  );

/**
  Stop Console Out ConSplitter on device handle by closing Console Out Devcice GUID.

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

//
// Absolute Pointer protocol interfaces
//

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
  );

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
  );

/**
  This event agregates all the events of the pointer devices in the splitter.

  If any events of physical pointer devices are signaled, signal the pointer
  splitter event. This will cause the calling code to call
  ConSplitterAbsolutePointerGetState ().

  @param  Event                    The Event assoicated with callback.
  @param  Context                  Context registered when Event was created.

**/
VOID
EFIAPI
ConSplitterAbsolutePointerWaitForInput (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
ConSplitterComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
ConSplitterConInComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
ConSplitterSimplePointerComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by an EFI Driver.

  @param  This                   A pointer to the EFI_COMPONENT_NAME_PROTOCOL
                                 instance.
  @param  ControllerHandle       The handle of a controller that the driver
                                 specified by This is managing.  This handle
                                 specifies the controller whose name is to be
                                 returned.
  @param  ChildHandle            The handle of the child controller to retrieve the
                                 name of.  This is an optional parameter that may
                                 be NULL.  It will be NULL for device drivers.  It
                                 will also be NULL for a bus drivers that wish to
                                 retrieve the name of the bus controller.  It will
                                 not be NULL for a bus driver that wishes to
                                 retrieve the name of a child controller.
  @param  Language               A pointer to RFC4646 language identifier. This is
                                 the language of the controller name that that the
                                 caller is requesting, and it must match one of the
                                 languages specified in SupportedLanguages.  The
                                 number of languages supported by a driver is up to
                                 the driver writer.
  @param  ControllerName         A pointer to the Unicode string to return.  This
                                 Unicode string is the name of the controller
                                 specified by ControllerHandle and ChildHandle in
                                 the language specified by Language from the point
                                 of view of the driver specified by This.

  @retval EFI_SUCCESS            The Unicode string for the user readable name in
                                 the language specified by Language for the driver
                                 specified by This was returned in DriverName.
  @retval EFI_INVALID_PARAMETER  ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER  ChildHandle is not NULL and it is not a valid
                                 EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER  Language is NULL.
  @retval EFI_INVALID_PARAMETER  ControllerName is NULL.
  @retval EFI_UNSUPPORTED        The driver specified by This is not currently
                                 managing the controller specified by
                                 ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED        The driver specified by This does not support the
                                 language specified by Language.

**/
EFI_STATUS
EFIAPI
ConSplitterAbsolutePointerComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
ConSplitterConOutComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
ConSplitterStdErrComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

//
// TextIn Constructor/Destructor functions
//

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
  );

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
  );

//
// SimplePointer Constuctor/Destructor functions
//

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
  );

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
  );

//
// TextOut Constuctor/Destructor functions
//

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
  );

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
  );

//
// TextIn I/O Functions
//

/**
  Reset the input device and optionaly run diagnostics

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
  );

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existance of a keystroke via WaitForEvent () call.

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
  );

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
  );

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
  );

//
// Simple Text Input Ex protocol function prototypes
//

/**
  Reset the input device and optionaly run diagnostics

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
  );

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existance of a keystroke via WaitForEvent () call.

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
  );

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
  );

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
  @retval EFI_OUT_OF_RESOURCES     Unable to allocate resources for necesssary data
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
  );

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                     Protocol instance pointer.
  @param  NotificationHandle       The handle of the notification function being
                                   unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered
                                   successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid.
  @retval EFI_NOT_FOUND            Can not find the matching entry in database.

**/
EFI_STATUS
EFIAPI
ConSplitterTextInUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  );

/**
  This event aggregates all the events of the ConIn devices in the spliter.

  If any events of physical ConIn devices are signaled, signal the ConIn
  spliter event. This will cause the calling code to call
  ConSplitterTextInReadKeyStroke ().

  @param  Event                    The Event assoicated with callback.
  @param  Context                  Context registered when Event was created.

**/
VOID
EFIAPI
ConSplitterTextInWaitForKey (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/**
  Reads the next keystroke from the input device. The WaitForKey Event can
  be used to test for existance of a keystroke via WaitForEvent () call.

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
  );

/**
  Reset the input device and optionaly run diagnostics

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
  );

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
  );

/**
  This event agregates all the events of the ConIn devices in the spliter.
  If any events of physical ConIn devices are signaled, signal the ConIn
  spliter event. This will cause the calling code to call
  ConSplitterTextInReadKeyStroke ().

  @param  Event                    The Event assoicated with callback.
  @param  Context                  Context registered when Event was created.

**/
VOID
EFIAPI
ConSplitterSimplePointerWaitForInput (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

//
// TextOut I/O Functions
//

/**
  Reset the text output device hardware and optionaly run diagnostics

  @param  This                     Protocol instance pointer.
  @param  ExtendedVerification     Driver may perform more exhaustive verfication
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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Returns information for an available graphics mode that the graphics device
  and the set of active video output devices supports.

  @param  This                  The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber            The mode number to return information on.
  @param  SizeOfInfo            A pointer to the size, in bytes, of the Info buffer.
  @param  Info                  A pointer to callee allocated buffer that returns information about ModeNumber.

  @retval EFI_SUCCESS           Mode information returned.
  @retval EFI_BUFFER_TOO_SMALL  The Info buffer was too small.
  @retval EFI_DEVICE_ERROR      A hardware error occurred trying to retrieve the video mode.
  @retval EFI_INVALID_PARAMETER One of the input args was NULL.
  @retval EFI_OUT_OF_RESOURCES  No resource available.

**/
EFI_STATUS
EFIAPI
ConSplitterGraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  );

/**
  Set the video device into the specified mode and clears the visible portions of
  the output display to black.

  @param  This                  The EFI_GRAPHICS_OUTPUT_PROTOCOL instance.
  @param  ModeNumber            Abstraction that defines the current video mode.

  @retval EFI_SUCCESS           The graphics mode specified by ModeNumber was selected.
  @retval EFI_DEVICE_ERROR      The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED       ModeNumber is not supported by this device.
  @retval EFI_OUT_OF_RESOURCES  No resource available.

**/
EFI_STATUS
EFIAPI
ConSplitterGraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  );

/**
  The following table defines actions for BltOperations.

  EfiBltVideoFill - Write data from the  BltBuffer pixel (SourceX, SourceY)
  directly to every pixel of the video display rectangle
  (DestinationX, DestinationY)
  (DestinationX + Width, DestinationY + Height).
  Only one pixel will be used from the BltBuffer. Delta is NOT used.
  EfiBltVideoToBltBuffer - Read data from the video display rectangle
  (SourceX, SourceY) (SourceX + Width, SourceY + Height) and place it in
  the BltBuffer rectangle (DestinationX, DestinationY )
  (DestinationX + Width, DestinationY + Height). If DestinationX or
  DestinationY is not zero then Delta must be set to the length in bytes
  of a row in the BltBuffer.
  EfiBltBufferToVideo - Write data from the  BltBuffer rectangle
  (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the
  video display rectangle (DestinationX, DestinationY)
  (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is
  not zero then Delta must be set to the length in bytes of a row in the
  BltBuffer.
  EfiBltVideoToVideo - Copy from the video display rectangle
  (SourceX, SourceY) (SourceX + Width, SourceY + Height) .
  to the video display rectangle (DestinationX, DestinationY)
  (DestinationX + Width, DestinationY + Height).
  The BltBuffer and Delta  are not used in this mode.

  @param  This                    Protocol instance pointer.
  @param  BltBuffer               Buffer containing data to blit into video buffer.
                                  This buffer has a size of
                                  Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
  @param  BltOperation            Operation to perform on BlitBuffer and video
                                  memory
  @param  SourceX                 X coordinate of source for the BltBuffer.
  @param  SourceY                 Y coordinate of source for the BltBuffer.
  @param  DestinationX            X coordinate of destination for the BltBuffer.
  @param  DestinationY            Y coordinate of destination for the BltBuffer.
  @param  Width                   Width of rectangle in BltBuffer in pixels.
  @param  Height                  Hight of rectangle in BltBuffer in pixels.
  @param  Delta                   OPTIONAL.

  @retval EFI_SUCCESS             The Blt operation completed.
  @retval EFI_INVALID_PARAMETER   BltOperation is not valid.
  @retval EFI_DEVICE_ERROR        A hardware error occurred writting to the video
                                  buffer.

**/
EFI_STATUS
EFIAPI
ConSplitterGraphicsOutputBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta         OPTIONAL
  );

/**
  Return the current video mode information.

  @param  This                  The EFI_UGA_DRAW_PROTOCOL instance.
  @param  HorizontalResolution  The size of video screen in pixels in the X dimension.
  @param  VerticalResolution    The size of video screen in pixels in the Y dimension.
  @param  ColorDepth            Number of bits per pixel, currently defined to be 32.
  @param  RefreshRate           The refresh rate of the monitor in Hertz.

  @retval EFI_SUCCESS           Mode information returned.
  @retval EFI_NOT_STARTED       Video display is not initialized. Call SetMode ()
  @retval EFI_INVALID_PARAMETER One of the input args was NULL.

**/
EFI_STATUS
EFIAPI
ConSplitterUgaDrawGetMode (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  OUT UINT32                 *HorizontalResolution,
  OUT UINT32                 *VerticalResolution,
  OUT UINT32                 *ColorDepth,
  OUT UINT32                 *RefreshRate
  );

/**
  Set the current video mode information.

  @param  This                 The EFI_UGA_DRAW_PROTOCOL instance.
  @param  HorizontalResolution The size of video screen in pixels in the X dimension.
  @param  VerticalResolution   The size of video screen in pixels in the Y dimension.
  @param  ColorDepth           Number of bits per pixel, currently defined to be 32.
  @param  RefreshRate          The refresh rate of the monitor in Hertz.

  @retval EFI_SUCCESS          Mode information returned.
  @retval EFI_NOT_STARTED      Video display is not initialized. Call SetMode ()
  @retval EFI_OUT_OF_RESOURCES Out of resources.

**/
EFI_STATUS
EFIAPI
ConSplitterUgaDrawSetMode (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN UINT32                  HorizontalResolution,
  IN UINT32                  VerticalResolution,
  IN UINT32                  ColorDepth,
  IN UINT32                  RefreshRate
  );

/**
  Blt a rectangle of pixels on the graphics screen.

  The following table defines actions for BltOperations.

  EfiUgaVideoFill:
    Write data from the  BltBuffer pixel (SourceX, SourceY)
    directly to every pixel of the video display rectangle
    (DestinationX, DestinationY)
    (DestinationX + Width, DestinationY + Height).
    Only one pixel will be used from the BltBuffer. Delta is NOT used.
  EfiUgaVideoToBltBuffer:
    Read data from the video display rectangle
    (SourceX, SourceY) (SourceX + Width, SourceY + Height) and place it in
    the BltBuffer rectangle (DestinationX, DestinationY )
    (DestinationX + Width, DestinationY + Height). If DestinationX or
    DestinationY is not zero then Delta must be set to the length in bytes
    of a row in the BltBuffer.
  EfiUgaBltBufferToVideo:
    Write data from the  BltBuffer rectangle
    (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the
    video display rectangle (DestinationX, DestinationY)
    (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is
    not zero then Delta must be set to the length in bytes of a row in the
    BltBuffer.
  EfiUgaVideoToVideo:
    Copy from the video display rectangle
    (SourceX, SourceY) (SourceX + Width, SourceY + Height) .
    to the video display rectangle (DestinationX, DestinationY)
    (DestinationX + Width, DestinationY + Height).
    The BltBuffer and Delta  are not used in this mode.

  @param  This           Protocol instance pointer.
  @param  BltBuffer      Buffer containing data to blit into video buffer. This
                         buffer has a size of Width*Height*sizeof(EFI_UGA_PIXEL)
  @param  BltOperation   Operation to perform on BlitBuffer and video memory
  @param  SourceX        X coordinate of source for the BltBuffer.
  @param  SourceY        Y coordinate of source for the BltBuffer.
  @param  DestinationX   X coordinate of destination for the BltBuffer.
  @param  DestinationY   Y coordinate of destination for the BltBuffer.
  @param  Width          Width of rectangle in BltBuffer in pixels.
  @param  Height         Hight of rectangle in BltBuffer in pixels.
  @param  Delta          OPTIONAL

  @retval EFI_SUCCESS            The Blt operation completed.
  @retval EFI_INVALID_PARAMETER  BltOperation is not valid.
  @retval EFI_DEVICE_ERROR       A hardware error occurred writting to the video buffer.

**/
EFI_STATUS
EFIAPI
ConSplitterUgaDrawBlt (
  IN  EFI_UGA_DRAW_PROTOCOL  *This,
  IN  EFI_UGA_PIXEL          *BltBuffer  OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION  BltOperation,
  IN  UINTN                  SourceX,
  IN  UINTN                  SourceY,
  IN  UINTN                  DestinationX,
  IN  UINTN                  DestinationY,
  IN  UINTN                  Width,
  IN  UINTN                  Height,
  IN  UINTN                  Delta         OPTIONAL
  );

/**
  Sets the output device(s) to a specified mode.

  @param  Private                 Text Out Splitter pointer.
  @param  ModeNumber              The mode number to set.

**/
VOID
TextOutSetMode (
  IN  TEXT_OUT_SPLITTER_PRIVATE_DATA  *Private,
  IN  UINTN                           ModeNumber
  );

#endif
