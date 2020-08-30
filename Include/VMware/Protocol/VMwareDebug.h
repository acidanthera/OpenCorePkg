/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef VMWARE_DEBUG_H
#define VMWARE_DEBUG_H

/**
  VMWare debugging protocol GUID.
**/
#define VMWARE_DEBUG_PROTOCOL_GUID  \
  { 0x5127A9FE, 0x2274, 0x451D,     \
    { 0x90, 0xAA, 0xCB, 0xE8, 0x44, 0xCF, 0x55, 0x71 } }

extern EFI_GUID gVMwareDebugProtocolGuid;

/**
  Function to message on VMware virtual machines.
**/
typedef
VOID
(EFIAPI *VMWARE_DEBUG_PROTOCOL_MESSAGE) (
  IN CONST CHAR8  *FormatString,
  IN VA_LIST      Marker
  );

/**
  VMware debug protocol definition.
**/
typedef struct VMWARE_DEBUG_PROTOCOL_ {
  VMWARE_DEBUG_PROTOCOL_MESSAGE  Message;
} VMWARE_DEBUG_PROTOCOL;

#endif // VMWARE_DEBUG_H
