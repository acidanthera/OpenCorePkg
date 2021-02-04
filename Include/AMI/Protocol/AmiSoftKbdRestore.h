/** @file
  Header file for AMI SoftKbd Restore protocol definitions.

Copyright (c) 2020, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef AMI_SOFT_KBD_RESTORE_H
#define AMI_SOFT_KBD_RESTORE_H

#include <Library/OcGuardLib.h>

// 890DF583-0D14-4C82-996D-E5EAE8CA905E
#define AMI_SOFT_KBD_RESTORE_PROTOCOL_GUID \
  { 0x890DF583, 0x0D14, 0x4C82, { 0x99, 0x6D, 0xE5, 0xEA, 0xE8, 0xCA, 0x90, 0x5E }}

/**
  AMI Soft Keyboard Restore protocol GUID.
**/
extern EFI_GUID gAmiSoftKbdRestoreProtocolGuid;

/**
  AMI Soft Keyboard Restore protocol forward declaration.
**/
typedef struct AMI_SOFT_KBD_RESTORE_PROTOCOL_ AMI_SOFT_KBD_RESTORE_PROTOCOL;

/**
  AMI Soft Keyboard Restore protocol action common for different methods.
**/
typedef
EFI_STATUS
(EFIAPI *AMI_SOFT_KBD_RESTORE_ACTION) (
  IN AMI_SOFT_KBD_RESTORE_PROTOCOL  *This
  );

/**
  AMI Soft Keyboard Restore protocol definition.
**/
struct AMI_SOFT_KBD_RESTORE_PROTOCOL_ {
  AMI_SOFT_KBD_RESTORE_ACTION              Activate;
  AMI_SOFT_KBD_RESTORE_ACTION              Deactivate;
  BOOLEAN                                  IsActive;
};

#endif // AMI_SOFT_KBD_RESTORE_H
