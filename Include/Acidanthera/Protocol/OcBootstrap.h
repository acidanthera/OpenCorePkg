/** @file
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_BOOTSTRAP_PROTOCOL_H
#define OC_BOOTSTRAP_PROTOCOL_H

#include <Library/OcCryptoLib.h>

#include <Protocol/SimpleFileSystem.h>

///
/// BA1EB455-B182-4F14-8521-E422C325DEF6
///
#define OC_BOOTSTRAP_PROTOCOL_GUID                                                  \
  {                                                                                 \
    0xBA1EB455, 0xB182, 0x4F14, { 0x85, 0x21, 0xE4, 0x22, 0xC3, 0x25, 0xDE, 0xF6 }  \
  }

///
/// OC_BOOTSTRAP_PROTOCOL revision
///
#define OC_BOOTSTRAP_PROTOCOL_REVISION 7

///
/// Forward declaration of OC_BOOTSTRAP_PROTOCOL structure.
///
typedef struct OC_BOOTSTRAP_PROTOCOL_ OC_BOOTSTRAP_PROTOCOL;

/**
  Obtain OpenCore load handle.

  @param[in] This           This protocol.

  @retval load handle on success.
  @retval NULL on failure.
**/
typedef
EFI_HANDLE
(EFIAPI *OC_GET_LOAD_HANDLE) (
  IN OC_BOOTSTRAP_PROTOCOL            *This
  );

///
/// The structure exposed by the OC_BOOTSTRAP_PROTOCOL.
///
struct OC_BOOTSTRAP_PROTOCOL_ {
  UINTN               Revision;
  OC_GET_LOAD_HANDLE  GetLoadHandle;
};

extern EFI_GUID gOcBootstrapProtocolGuid;

#endif // OC_BOOTSTRAP_PROTOCOL_H
