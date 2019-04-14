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

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcVirtualFsLib.h>

#include <Guid/FileInfo.h>

#include "VirtualFsInternal.h"

STATIC EFI_HANDLE_PROTOCOL mOriginalHandleProtocol;
STATIC EFI_LOCATE_PROTOCOL mOriginalLocateProtocol;
STATIC EFI_FILE_OPEN       mOpenCallback;
STATIC UINT32              mEntranceCount;

STATIC
VOID
VirtualFsWrapProtocol (
  IN  EFI_GUID          *Protocol,
  OUT VOID              **Interface
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;

  if (!CompareGuid (Protocol, &gEfiSimpleFileSystemProtocolGuid)) {
    return;
  }

  Status = CreateVirtualFs (*Interface, mOpenCallback, &FileSystem);
  if (!EFI_ERROR (Status)) {
    *Interface = FileSystem;
  }
}

STATIC
EFI_STATUS
EFIAPI
VirtualFsHandleProtocol (
  IN  EFI_HANDLE        Handle,
  IN  EFI_GUID          *Protocol,
  OUT VOID              **Interface
  )
{
  EFI_STATUS  Status;

  Status = mOriginalHandleProtocol (Handle, Protocol, Interface);

  if (!EFI_ERROR (Status) && Interface != NULL && mEntranceCount == 0) {
    ++mEntranceCount;
    VirtualFsWrapProtocol (Protocol, Interface);
    --mEntranceCount;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
VirtualFsLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  )
{
  EFI_STATUS  Status;

  Status = mOriginalLocateProtocol (Protocol, Registration, Interface);

  if (!EFI_ERROR (Status) && Interface != NULL && mEntranceCount == 0) {
    ++mEntranceCount;
    VirtualFsWrapProtocol (Protocol, Interface);
    --mEntranceCount;
  }

  return Status;
}

EFI_STATUS
EnableVirtualFs (
  IN OUT EFI_BOOT_SERVICES       *BootServices,
  IN     EFI_FILE_OPEN           OpenCallback
  )
{
  if (mOriginalHandleProtocol != NULL
    || BootServices->HandleProtocol == VirtualFsHandleProtocol) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Note, we should normally also hook OpenProtocol, but it results in freezes on APTIO IV and V.
  //
  mOpenCallback                = OpenCallback;
  mOriginalHandleProtocol      = BootServices->HandleProtocol;
  mOriginalLocateProtocol      = BootServices->LocateProtocol;
  BootServices->HandleProtocol = VirtualFsHandleProtocol;
  BootServices->LocateProtocol = VirtualFsLocateProtocol;
  BootServices->Hdr.CRC32      = 0;
  BootServices->CalculateCrc32 (BootServices, BootServices->Hdr.HeaderSize, &BootServices->Hdr.CRC32);

  return EFI_SUCCESS;
}

EFI_STATUS
DisableVirtualFs (
  IN OUT EFI_BOOT_SERVICES       *BootServices
  )
{
  if (mOriginalHandleProtocol == NULL
    || BootServices->HandleProtocol != VirtualFsHandleProtocol) {
    return EFI_ALREADY_STARTED;
  }

  BootServices->HandleProtocol = mOriginalHandleProtocol;
  BootServices->LocateProtocol = mOriginalLocateProtocol;
  mOpenCallback                = NULL;
  mOriginalHandleProtocol      = NULL;
  mOriginalLocateProtocol      = NULL;
  BootServices->Hdr.CRC32      = 0;
  BootServices->CalculateCrc32 (BootServices, BootServices->Hdr.HeaderSize, &BootServices->Hdr.CRC32);

  return EFI_SUCCESS;
}

