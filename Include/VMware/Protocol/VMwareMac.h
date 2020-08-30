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

#ifndef VMWARE_MAC_H
#define VMWARE_MAC_H

/**
  VMWare Mac OS X server or 10.7+ protocol GUID.
  Installed as NULL on the legit image handle.
**/
#define VMWARE_MAC_PROTOCOL_GUID  \
  { 0x03F38E56, 0x8231, 0x4469,   \
    { 0x94, 0xED, 0x82, 0xAE, 0x53, 0x15, 0x83, 0x4F } }

extern EFI_GUID gVMwareMacProtocolGuid;

#endif // VMWARE_MAC_H
