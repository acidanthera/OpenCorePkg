/** @file
  Header file for AmiEfiPointer to EfiPointer translator.

Copyright (c) 2016, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef AIM_SELF_H
#define AIM_SELF_H

#include <Library/UefiLib.h>
#include <Protocol/AmiPointer.h>
#include <Protocol/SimplePointer.h>

//
// Taken from APTIO IV Z87.
//
#define AIM_MAX_POINTERS 6

//
// APTIO IV Z87 has 66666 here.
// Slightly lower resolution results in sometimes overflowng mouse.
//
#define AIM_POSITION_POLL_INTERVAL 66666

//
// Position movement boost.
//
#define AIM_SCALE_FACTOR 1

typedef struct {
  EFI_HANDLE                    DeviceHandle;
  AMI_EFIPOINTER_PROTOCOL       *EfiPointer;
  EFI_SIMPLE_POINTER_PROTOCOL   *SimplePointer;
  EFI_SIMPLE_POINTER_GET_STATE  OriginalGetState;
  BOOLEAN                       PositionChanged;
  INT32                         PositionX;
  INT32                         PositionY;
  INT32                         PositionZ;
} AMI_SHIM_POINTER_INSTANCE;

typedef struct {
  BOOLEAN                       TimersInitialised;
  BOOLEAN                       UsageStarted;
  EFI_EVENT                     ProtocolArriveEvent;
  EFI_EVENT                     PositionEvent;
  AMI_SHIM_POINTER_INSTANCE     PointerMap[AIM_MAX_POINTERS];
} AMI_SHIM_POINTER;

#endif
