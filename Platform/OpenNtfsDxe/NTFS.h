/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#ifndef NTFS_H
#define NTFS_H

#include "Protocol/PciIo.h"
#include "Protocol/DevicePath.h"
#include "Protocol/DriverBinding.h"
#include "Protocol/BlockIo.h"
#include "Protocol/DiskIo.h"
#include "Protocol/SimpleFileSystem.h"
#include "Protocol/UnicodeCollation.h"
#include "Protocol/LoadedImage.h"
#include <Protocol/ComponentName.h>

#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/OcStringLib.h>

EFI_STATUS
EFIAPI
NTFSSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
NTFSStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
NTFSStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  );

EFI_STATUS
EFIAPI
UnloadNTFSDriver (
  IN EFI_HANDLE ImageHandle
  );

EFI_STATUS
EFIAPI
NTFSCtlDriverName (
  IN EFI_COMPONENT_NAME_PROTOCOL *This,
  IN CHAR8                       *Language,
  OUT CHAR16                     **DriverName
  );

EFI_STATUS
EFIAPI
NTFSCtlGetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL *This,
  IN EFI_HANDLE                  Controller,
  IN EFI_HANDLE                  ChildHandle      OPTIONAL,
  IN CHAR8                       *Language,
  OUT CHAR16                     **ControllerName
  );

EFI_STATUS
EFIAPI
FileOpen (
  IN EFI_FILE_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL **NewHandle,
  IN CHAR16             *FileName,
  IN UINT64             OpenMode,
  IN UINT64             Attributes
  );

EFI_STATUS
EFIAPI
FileClose (
  IN EFI_FILE_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
FileDelete (
  IN EFI_FILE_PROTOCOL *This
  );

EFI_STATUS
EFIAPI
FileRead (
  IN EFI_FILE_PROTOCOL *This,
  IN OUT UINTN         *BufferSize,
  OUT VOID             *Buffer
  );

EFI_STATUS
EFIAPI
FileWrite (
  IN EFI_FILE_PROTOCOL *This,
  IN OUT UINTN         *BufferSize,
  IN VOID              *Buffer
  );

EFI_STATUS
EFIAPI
FileGetPosition (
  IN  EFI_FILE_PROTOCOL *This,
  OUT UINT64            *Position
  );

EFI_STATUS
EFIAPI
FileSetPosition (
  IN EFI_FILE_PROTOCOL *This,
  IN UINT64           Position
  );

EFI_STATUS
EFIAPI
FileGetInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN EFI_GUID          *Type,
  IN OUT UINTN         *Len,
  OUT VOID             *Data
  );

EFI_STATUS
EFIAPI
FileSetInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN EFI_GUID          *InformationType,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  );

EFI_STATUS
EFIAPI
FileFlush (
  IN EFI_FILE_PROTOCOL *This
  );

#endif // NTFS_H
