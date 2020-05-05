/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_INTERFACE_PROTOCOL_H
#define OC_INTERFACE_PROTOCOL_H

#include <Library/OcBootManagementLib.h>
#include <Library/OcStorageLib.h>

/**
  Current supported interface protocol revision.
  It is changed every time the contract changes.

  WARNING: This protocol currently undergoes design process.
**/
#define OC_INTERFACE_REVISION  4

/**
  The GUID of the OC_INTERFACE_PROTOCOL.
**/
#define OC_INTERFACE_PROTOCOL_GUID      \
  { 0x53027CDF, 0x3A89, 0x4255,         \
    { 0xAE, 0x29, 0xD6, 0x66, 0x6E, 0xFE, 0x99, 0xEF } }

/**
  The forward declaration for the protocol for the OC_INTERFACE_PROTOCOL_H.
**/
typedef struct OC_INTERFACE_PROTOCOL_ OC_INTERFACE_PROTOCOL;

/**
  Add an entry to the log buffer

  @param[in] This          This protocol.
  @param[in] Storage       File system access storage.
  @param[in] Picker        User interface configuration.

  @retval does not return unless a fatal error happened.
**/
typedef
EFI_STATUS
(EFIAPI *OC_INTERFACE_RUN) (
  IN OC_INTERFACE_PROTOCOL  *This,
  IN OC_STORAGE_CONTEXT     *Storage,
  IN OC_PICKER_CONTEXT      *Picker
  );

/**
  The structure exposed by the OC_INTERFACE_PROTOCOL.
**/
struct OC_INTERFACE_PROTOCOL_ {
  UINT32                  Revision;     ///< The revision of the installed protocol.
  OC_INTERFACE_RUN        ShowInteface; ///< A pointer to the ShowInterface function.
};

/**
  A global variable storing the GUID of the OC_INTERFACE_PROTOCOL.
**/
extern EFI_GUID gOcInterfaceProtocolGuid;

#endif // OC_INTERFACE_PROTOCOL_H
