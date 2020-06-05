/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_USER_BOOT_SERVICES_H
#define OC_USER_BOOT_SERVICES_H

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include <stdio.h>

extern EFI_BOOT_SERVICES mBootServices;
extern EFI_SYSTEM_TABLE mSystemTable;
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL mConOut;

EFI_TPL
EFIAPI
DummyRaiseTPL (
  IN EFI_TPL      NewTpl
  );

EFI_STATUS
DummyLocateProtocol (
  EFI_GUID        *ProtocolGuid,
  VOID            *Registration,
  VOID            **Interface
  );

EFI_STATUS
DummyAllocatePages (
  IN     EFI_ALLOCATE_TYPE            Type,
  IN     EFI_MEMORY_TYPE              MemoryType,
  IN     UINTN                        Pages,
  IN OUT EFI_PHYSICAL_ADDRESS         *Memory
  );

EFI_STATUS
EFIAPI
DummyInstallConfigurationTable (
  IN EFI_GUID *Guid,
  IN VOID     *Table
  );

EFI_STATUS
EFIAPI
NullTextOutputString (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN CHAR16                          *String
  );

#endif // OC_USER_BOOT_SERVICES_H
