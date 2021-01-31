/** @file
  Header file for AMI SoftKbd protocol definitions.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef AMI_SOFT_KBD_H
#define AMI_SOFT_KBD_H

#include <Library/OcGuardLib.h>

// 96FD60F3-0BC8-4A11-84F1-2EB1CB5BA5A3
#define AMI_SOFT_KBD_PROTOCOL_GUID \
  { 0x96FD60F3, 0x0BC8, 0x4A11, { 0x84, 0xF1, 0x2E, 0xB1, 0xCB, 0x5B, 0xA5, 0xA3 }}

/**
  AMI Soft Keyboard protocol GUID.
**/
extern EFI_GUID gAmiSoftKbdProtocolGuid;

/**
  AMI Soft Keyboard protocol forward declaration.
**/
typedef struct AMI_SOFT_KBD_PROTOCOL_ AMI_SOFT_KBD_PROTOCOL;

/**
  AMI Soft Keyboard protocol action common for different methods.
**/
typedef
EFI_STATUS
(EFIAPI *AMI_SOFT_KBD_ACTION) (
  IN AMI_SOFT_KBD_PROTOCOL    *This
  );

/**
  AMI Soft Keyboard protocol definition.
**/
struct AMI_SOFT_KBD_PROTOCOL_ {
  AMI_SOFT_KBD_ACTION                      Initialize;
  UINTN                                    Private1;
  UINTN                                    Private2;
  AMI_SOFT_KBD_ACTION                      Activate;
  AMI_SOFT_KBD_ACTION                      Deactivate;
  UINTN                                    Private3;
  UINTN                                    Private4;
  UINTN                                    Private5;
  UINTN                                    Private6;
  UINTN                                    Private7;
  UINTN                                    Private8;
  UINTN                                    Private9;
  BOOLEAN                                  IsActive;
  AMI_SOFT_KBD_ACTION                      Stop;
  UINTN                                    Private10;
  UINTN                                    Private11;
  UINTN                                    Private12;
};

#endif // AMI_SOFT_KBD_H
