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

#ifndef VMWARE_HDA_H
#define VMWARE_HDA_H

/**
  VMWare Intel HDA protocol GUID.
**/
#define VMWARE_INTEL_HDA_PROTOCOL_GUID  \
  { 0x94E46BC2, 0x9127, 0x11DF,         \
    { 0xBF, 0xCE, 0xE7, 0x83, 0xCA, 0x2A, 0x34, 0xBE } }

extern EFI_GUID gVMwareHdaProtocolGuid;

#endif // VMWARE_HDA_H
