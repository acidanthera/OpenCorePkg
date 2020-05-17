/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_OS_LOADED_H
#define APPLE_OS_LOADED_H

#include <Uefi.h>

// APPLE_OS_LOADED_EVENT_NAME
#define APPLE_OS_LOADED_NAMED_EVENT_GUID  \
  { 0xC5C5DA95, 0x7D5C, 0x45E6,           \
    { 0x83, 0x72, 0x89, 0xBD, 0x52, 0x6D, 0xE9, 0x56 } }

// gAppleOSLoadedNamedEventGuid
extern EFI_GUID gAppleOSLoadedNamedEventGuid;

#endif // APPLE_OS_LOADED_H
