/** @file
  Duet BDS library.

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DUET_BDS_LIB_H_
#define _DUET_BDS_LIB_H_

///
/// ConnectType
///
#define CONSOLE_OUT 0x00000001
#define STD_ERROR   0x00000002
#define CONSOLE_IN  0x00000004
#define CONSOLE_ALL (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} BDS_CONSOLE_CONNECT_ENTRY;

/**
  This function reads the EFI variable (VendorGuid/Name) and returns a dynamically allocated
  buffer and the size of the buffer. If it fails, return NULL.

  @param  Name                  The string part of the  EFI variable name.
  @param  VendorGuid            The GUID part of the EFI variable name.
  @param  VariableSize          Returns the size of the EFI variable that was read.

  @return                       Dynamically allocated memory that contains a copy 
                                of the EFI variable. The caller is responsible for 
                                freeing the buffer.
  @retval NULL                  The variable was not read.

**/
VOID *
EFIAPI
BdsLibGetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  );


//
// Bds connect and disconnect driver lib funcions
//
/**
  This function connects all system drivers with the corresponding controllers. 

**/
VOID
EFIAPI
BdsLibConnectAllDriversToAllControllers (
  VOID
  );

/**
  This function connects all system drivers to controllers.

**/
VOID
EFIAPI
BdsLibConnectAll (
  VOID
  );

/**
  This function creates all handles associated with the given device
  path node. If the handle associated with one device path node cannot
  be created, then it tries to execute the dispatch to load the missing drivers.  

  @param  DevicePathToConnect   The device path to be connected. Can be
                                a multi-instance device path.

  @retval EFI_SUCCESS           All handles associates with every device path node
                                were created.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources to create new handles.
  @retval EFI_NOT_FOUND         At least one handle could not be created.

**/
EFI_STATUS
EFIAPI
BdsLibConnectDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect
  );

/**
  This function will connect all current system handles recursively.     
  gBS->ConnectController() service is invoked for each handle exist in system handler buffer.  
  If the handle is bus type handler, all childrens also will be connected recursively  by gBS->ConnectController().
  
  @retval EFI_SUCCESS           All handles and child handles have been
                                connected.  
  @retval EFI_STATUS            Return the status of gBS->LocateHandleBuffer().
**/
EFI_STATUS
EFIAPI
BdsLibConnectAllEfi (
  VOID
  );

/**
  This function will disconnect all current system handles.     
  gBS->DisconnectController() is invoked for each handle exists in system handle buffer.  
  If handle is a bus type handle, all childrens also are disconnected recursively by  gBS->DisconnectController().
  
  @retval EFI_SUCCESS           All handles have been disconnected.
  @retval EFI_STATUS            Error status returned by of gBS->LocateHandleBuffer().

**/
EFI_STATUS
EFIAPI
BdsLibDisconnectAllEfi (
  VOID
  );

//
// Bds console related lib functions
//
/**
  This function will search every simpletxt device in the current system,
  and make every simpletxt device a potential console device.

**/
VOID
EFIAPI
BdsLibConnectAllConsoles (
  VOID
  );


/**
  This function will connect console device based on the console
  device variable ConIn, ConOut and ErrOut.

  @retval EFI_SUCCESS              At least one of the ConIn and ConOut devices have
                                   been connected.
  @retval EFI_STATUS               Return the status of BdsLibConnectConsoleVariable ().

**/
EFI_STATUS
EFIAPI
BdsLibConnectAllDefaultConsoles (
  VOID
  );

/**
  This function updates the console variable based on ConVarName. It can
  add or remove one specific console device path from the variable

  @param  ConVarName               The console-related variable name: ConIn, ConOut,
                                   ErrOut.
  @param  CustomizedConDevicePath  The console device path to be added to
                                   the console variable ConVarName. Cannot be multi-instance.
  @param  ExclusiveDevicePath      The console device path to be removed
                                   from the console variable ConVarName. Cannot be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is the same as a removed one.
  @retval EFI_SUCCESS              Successfully added or removed the device path from the
                                   console variable.

**/
EFI_STATUS
EFIAPI
BdsLibUpdateConsoleVariable (
  IN  CHAR16                    *ConVarName,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  );

/**
  Connect the console device base on the variable ConVarName. If
  ConVarName is a multi-instance device path, and at least one
  instance connects successfully, then this function
  will return success.
  If the handle associate with one device path node can not
  be created successfully, then still give chance to do the dispatch,
  which load the missing drivers if possible.

  @param  ConVarName               The console related variable name: ConIn, ConOut,
                                   ErrOut.

  @retval EFI_NOT_FOUND            No console devices were connected successfully
  @retval EFI_SUCCESS              Connected at least one instance of the console
                                   device path based on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
BdsLibConnectConsoleVariable (
  IN  CHAR16                 *ConVarName
  );

//
// Bds device path related lib functions
//
/**
  Delete the instance in Multi that overlaps with Single. 

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @return This function removes the device path instances in Multi that overlap
          Single, and returns the resulting device path. If there is no
          remaining device path as a result, this function will return NULL.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
BdsLibDelPartMatchInstance (
  IN     EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN     EFI_DEVICE_PATH_PROTOCOL  *Single
  );

/**
  This function compares a device path data structure to that of all the nodes of a
  second device path instance.

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @retval TRUE                  If the Single device path is contained within a 
                                Multi device path.
  @retval FALSE                 The Single device path is not contained within a 
                                Multi device path.

**/
BOOLEAN
EFIAPI
BdsLibMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single
  );

/**
  Connect the specific USB device that matches the RemainingDevicePath,
  and whose bus is determined by Host Controller (Uhci or Ehci).

  @param  HostControllerPI      Uhci (0x00) or Ehci (0x20) or Both uhci and ehci
                                (0xFF).
  @param  RemainingDevicePath   A short-form device path that starts with the first
                                element being a USB WWID or a USB Class device
                                path.

  @retval EFI_SUCCESS           The specific Usb device is connected successfully.
  @retval EFI_INVALID_PARAMETER Invalid HostControllerPi (not 0x00, 0x20 or 0xFF) 
                                or RemainingDevicePath is not the USB class device path.
  @retval EFI_NOT_FOUND         The device specified by device path is not found.

**/
EFI_STATUS
EFIAPI
BdsLibConnectUsbDevByShortFormDP (
  IN UINT8                      HostControllerPI,
  IN EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath
  );

/**
  Platform Bds initialization. Includes the platform firmware vendor, revision
  and so crc check.

**/
VOID
EFIAPI
PlatformBdsInit (
  VOID
  );

/**
  The function will execute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.
**/
VOID
EFIAPI
PlatformBdsPolicyBehavior (
  VOID
  );

#endif
