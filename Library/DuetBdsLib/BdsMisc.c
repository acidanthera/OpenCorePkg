/** @file
  Misc BDS library function

Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DuetBdsLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>


/**
  Read the EFI variable (VendorGuid/Name) and return a dynamically allocated
  buffer, and the size of the buffer. If failure return NULL.

  @param  Name                  String part of EFI variable name
  @param  VendorGuid            GUID part of EFI variable name
  @param  VariableSize          Returns the size of the EFI variable that was read

  @return                       Dynamically allocated memory that contains a copy of the EFI variable
                                Caller is responsible freeing the buffer.
  @retval NULL                  Variable was not read

**/
VOID *
EFIAPI
BdsLibGetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  VOID        *Buffer;

  Buffer = NULL;

  //
  // Pass in a zero size buffer to find the required buffer size.
  //
  BufferSize  = 0;
  Status      = gRT->GetVariable (Name, VendorGuid, NULL, &BufferSize, Buffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate the buffer to return
    //
    Buffer = AllocateZeroPool (BufferSize);
    if (Buffer == NULL) {
      *VariableSize = 0;
      return NULL;
    }
    //
    // Read variable into the allocated buffer.
    //
    Status = gRT->GetVariable (Name, VendorGuid, NULL, &BufferSize, Buffer);
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      BufferSize = 0;
      Buffer     = NULL;
    }
  }

  *VariableSize = BufferSize;
  return Buffer;
}

/**
  Delete the instance in Multi which matches partly with Single instance

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @return This function will remove the device path instances in Multi which partly
          match with the Single, and return the result device path. If there is no
          remaining device path as a result, this function will return NULL.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
BdsLibDelPartMatchInstance (
  IN     EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN     EFI_DEVICE_PATH_PROTOCOL  *Single
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  UINTN                     InstanceSize;
  UINTN                     SingleDpSize;
  UINTN                     Size;

  NewDevicePath     = NULL;
  TempNewDevicePath = NULL;

  if (Multi == NULL || Single == NULL) {
    return Multi;
  }

  Instance        =  GetNextDevicePathInstance (&Multi, &InstanceSize);
  SingleDpSize    =  GetDevicePathSize (Single) - END_DEVICE_PATH_LENGTH;
  InstanceSize    -= END_DEVICE_PATH_LENGTH;

  while (Instance != NULL) {

    Size = (SingleDpSize < InstanceSize) ? SingleDpSize : InstanceSize;

    if ((CompareMem (Instance, Single, Size) != 0)) {
      //
      // Append the device path instance which does not match with Single
      //
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, Instance);
      if (TempNewDevicePath != NULL) {
        FreePool(TempNewDevicePath);
      }
    }
    FreePool(Instance);
    Instance = GetNextDevicePathInstance (&Multi, &InstanceSize);
    InstanceSize  -= END_DEVICE_PATH_LENGTH;
  }

  return NewDevicePath;
}

/**
  Function compares a device path data structure to that of all the nodes of a
  second device path instance.

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @retval TRUE                  If the Single device path is contained within Multi device path.
  @retval FALSE                 The Single device path is not match within Multi device path.

**/
BOOLEAN
EFIAPI
BdsLibMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInst;
  UINTN                     Size;

  if (Multi == NULL || Single  == NULL) {
    return FALSE;
  }

  DevicePath      = Multi;
  DevicePathInst  = GetNextDevicePathInstance (&DevicePath, &Size);

  //
  // Search for the match of 'Single' in 'Multi'
  //
  while (DevicePathInst != NULL) {
    //
    // If the single device path is found in multiple device paths,
    // return success
    //
    if (CompareMem (Single, DevicePathInst, Size) == 0) {
      FreePool (DevicePathInst);
      return TRUE;
    }

    FreePool (DevicePathInst);
    DevicePathInst = GetNextDevicePathInstance (&DevicePath, &Size);
  }

  return FALSE;
}
