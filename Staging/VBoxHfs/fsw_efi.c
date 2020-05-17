/* $Id: fsw_efi.c $ */

/** @file
 * fsw_efi.c - EFI host environment code.
 */

/*
 * Copyright (C) 2010-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*-
 * This code is based on:
 *
 * Copyright (c) 2006 Christoph Pfisterer
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsw_efi.h"

#include <Guid/AppleBless.h>

#ifndef FSTYPE
#error FSTYPE must be defined!
#endif

/** Helper macro for stringification. */

#define FSW_EFI_STRINGIFY(x) L#x

/** Expands to the EFI driver name given the file system type name. */

#define FSW_EFI_DRIVER_NAME(t) L"Fsw " FSW_EFI_STRINGIFY(t) L" File System Driver"

// ----------------------------------------

// function prototypes

EFI_STATUS EFIAPI fsw_efi_DriverBinding_Supported (
  IN EFI_DRIVER_BINDING_PROTOCOL * This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL * RemainingDevicePath
);

EFI_STATUS EFIAPI fsw_efi_DriverBinding_Start (
  IN EFI_DRIVER_BINDING_PROTOCOL * This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL * RemainingDevicePath
);

EFI_STATUS EFIAPI fsw_efi_DriverBinding_Stop (
  IN EFI_DRIVER_BINDING_PROTOCOL * This,
  IN EFI_HANDLE ControllerHandle,
  IN UINTN NumberOfChildren,
  IN EFI_HANDLE * ChildHandleBuffer
);

EFI_STATUS EFIAPI fsw_efi_ComponentName_GetDriverName (
  IN EFI_COMPONENT_NAME_PROTOCOL * This,
  IN CHAR8 *Language,
  OUT CHAR16 **DriverName
);

EFI_STATUS EFIAPI fsw_efi_ComponentName_GetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL * This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_HANDLE ChildHandle OPTIONAL,
  IN CHAR8 *Language,
  OUT CHAR16 **ControllerName
);

void fsw_efi_change_blocksize (
  struct fsw_volume *vol,
  fsw_u32 old_phys_blocksize,
  fsw_u32 old_log_blocksize,
  fsw_u32 new_phys_blocksize,
  fsw_u32 new_log_blocksize
);

fsw_status_t fsw_efi_read_block (
  struct fsw_volume *vol,
  fsw_u32 phys_bno,
  void *buffer
);

EFI_STATUS fsw_efi_map_status (
  fsw_status_t fsw_status,
  FSW_VOLUME_DATA * Volume
);

EFI_STATUS EFIAPI fsw_efi_FileSystem_OpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL * This,
  OUT EFI_FILE ** Root
);

EFI_STATUS fsw_efi_dnode_to_FileHandle (
  IN struct fsw_dnode *dno,
  OUT EFI_FILE ** NewFileHandle
);

EFI_STATUS fsw_efi_file_read (
  IN FSW_FILE_DATA * File,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
);

EFI_STATUS fsw_efi_file_getpos (
  IN FSW_FILE_DATA * File,
  OUT UINT64 *Position
);

EFI_STATUS fsw_efi_file_setpos (
  IN FSW_FILE_DATA * File,
  IN UINT64 Position
);

EFI_STATUS fsw_efi_dir_open (
  IN FSW_FILE_DATA * File,
  OUT EFI_FILE ** NewHandle,
  IN CHAR16 *FileName,
  IN UINT64 OpenMode,
  IN UINT64 Attributes
);

EFI_STATUS fsw_efi_dir_read (
  IN FSW_FILE_DATA * File,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
);

EFI_STATUS fsw_efi_dir_setpos (
  IN FSW_FILE_DATA * File,
  IN UINT64 Position
);

EFI_STATUS fsw_efi_dnode_getinfo (
  IN FSW_FILE_DATA * File,
  IN EFI_GUID *InformationType,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
);

EFI_STATUS fsw_efi_dnode_fill_FileInfo (
  IN FSW_VOLUME_DATA * Volume,
  IN struct fsw_dnode *dno,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
);

/**
 * Interface structure for the EFI Driver Binding protocol.
 */

EFI_DRIVER_BINDING_PROTOCOL fsw_efi_DriverBinding_table = {
  fsw_efi_DriverBinding_Supported,
  fsw_efi_DriverBinding_Start,
  fsw_efi_DriverBinding_Stop,
  0x10, /* Driver version */
  NULL,
  NULL
};

/**
 * Interface structure for the EFI Component Name protocol.
 */

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL
  fsw_efi_ComponentName_table = {
  fsw_efi_ComponentName_GetDriverName,
  fsw_efi_ComponentName_GetControllerName,
  "eng"
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL
  fsw_efi_ComponentName2_table = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME) fsw_efi_ComponentName_GetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)
    fsw_efi_ComponentName_GetControllerName,
  "en"
};

EFI_LOCK fsw_efi_Lock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_CALLBACK);

/**
 * Dispatch table for our FSW host driver.
 */

struct fsw_host_table fsw_efi_host_table = {
  FSW_STRING_KIND_UTF16,

  fsw_efi_change_blocksize,
  fsw_efi_read_block
};

extern struct fsw_fstype_table FSW_FSTYPE_TABLE_NAME (
  FSTYPE
);

EFI_STATUS
fsw_efi_AcquireLockOrFail (
  VOID
)
{
  return EfiAcquireLockOrFail (&fsw_efi_Lock);
}

VOID
fsw_efi_ReleaseLock (
  VOID
)
{
  EfiReleaseLock (&fsw_efi_Lock);
}

/**
 * Image entry point. Installs the Driver Binding and Component Name protocols
 * on the image's handle. Actually mounting a file system is initiated through
 * the Driver Binding protocol at the firmware's request.
 */

EFI_STATUS EFIAPI
fsw_efi_main (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE * SystemTable
)
{
  EFI_STATUS Status;

  Status = EfiLibInstallDriverBindingComponentName2 (ImageHandle, SystemTable,
                                              &fsw_efi_DriverBinding_table,
                                              ImageHandle,
                                              &fsw_efi_ComponentName_table,
                                              &fsw_efi_ComponentName2_table);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
 * Driver Binding EFI protocol, Supported function. This function is called by EFI
 * to test if this driver can handle a certain device. Our implementation only checks
 * if the device is a disk (i.e. that it supports the Block I/O and Disk I/O protocols)
 * and implicitly checks if the disk is already in use by another driver.
 */

EFI_STATUS EFIAPI
fsw_efi_DriverBinding_Supported (
  IN EFI_DRIVER_BINDING_PROTOCOL * This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL * RemainingDevicePath
)
{
  EFI_STATUS Status;
  EFI_DISK_IO_PROTOCOL *DiskIo;

  Status =
    gBS->OpenProtocol (ControllerHandle, &gEfiDiskIoProtocolGuid,
                      (VOID **) &DiskIo, This->DriverBindingHandle,
                      ControllerHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  gBS->CloseProtocol (ControllerHandle, &gEfiDiskIoProtocolGuid,
                     This->DriverBindingHandle, ControllerHandle);

  Status =
    gBS->OpenProtocol (ControllerHandle, &gEfiBlockIoProtocolGuid, NULL,
                      This->DriverBindingHandle, ControllerHandle,
                      EFI_OPEN_PROTOCOL_TEST_PROTOCOL);

Done:
  return Status;
}

VOID
fsw_efi_DetachVolume (
  IN FSW_VOLUME_DATA * pVolume
)
{
  // uninstall Simple File System protocol
  gBS->UninstallMultipleProtocolInterfaces (pVolume->Handle,
                                           &gEfiSimpleFileSystemProtocolGuid,
                                           &pVolume->FileSystem, NULL);

  // release private data structure

  if (pVolume->vol != NULL)
    fsw_unmount (pVolume->vol);

  FreePool (pVolume);
}

EFI_STATUS
fsw_efi_AttachVolume (
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DISK_IO_PROTOCOL * pDiskIo,
  IN EFI_BLOCK_IO_PROTOCOL * pBlockIo
)
{
  EFI_STATUS Status;
  FSW_VOLUME_DATA *pVolume;

  pVolume = AllocateZeroPool (sizeof (FSW_VOLUME_DATA));

  if (pVolume == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  pVolume->Signature = FSW_VOLUME_DATA_SIGNATURE;
  pVolume->Handle = ControllerHandle;
  pVolume->DiskIo = pDiskIo;
  pVolume->MediaId = pBlockIo->Media->MediaId;
  pVolume->FileSystem.Revision = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  pVolume->FileSystem.OpenVolume = fsw_efi_FileSystem_OpenVolume;
  pVolume->LastIOStatus = EFI_SUCCESS;

  Status = fsw_efi_map_status (fsw_mount
                        (pVolume, &fsw_efi_host_table,
                         &FSW_FSTYPE_TABLE_NAME (FSTYPE), &pVolume->vol),
                        pVolume);

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (&pVolume->Handle,
                                           &gEfiSimpleFileSystemProtocolGuid,
                                           &pVolume->FileSystem, NULL);

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  DEBUG ((EFI_D_INIT, "%a: Installed volume on %p\n", __FUNCTION__, ControllerHandle));

Done:

  if (EFI_ERROR (Status)) {
    fsw_efi_DetachVolume (pVolume);
  }

  return Status;
}

/**
 * Driver Binding EFI protocol, Start function. This function is called by EFI
 * to start driving the given device. It is still possible at this point to
 * return EFI_UNSUPPORTED, and in fact we will do so if the file system driver
 * cannot find the superblock signature (or equivalent) that it expects.
 *
 * This function allocates memory for a per-volume structure, opens the
 * required protocols (just Disk I/O in our case, Block I/O is only looked
 * at to get the MediaId field), and lets the FSW core mount the file system.
 * If successful, an EFI Simple File System protocol is exported on the
 * device handle.
 */

EFI_STATUS EFIAPI
fsw_efi_DriverBinding_Start (
  IN EFI_DRIVER_BINDING_PROTOCOL * This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL * RemainingDevicePath
)
{
  EFI_STATUS Status;
  EFI_STATUS Status2;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  EFI_DISK_IO_PROTOCOL *DiskIo;
  BOOLEAN LockedByMe;

  LockedByMe = FALSE;

  Status = fsw_efi_AcquireLockOrFail ();

  if (!EFI_ERROR (Status)) {
    LockedByMe = TRUE;
  }

  // open consumed protocols
 
  // NOTE: we only want to look at the MediaId

  Status = gBS->OpenProtocol (ControllerHandle, &gEfiBlockIoProtocolGuid,
                             (VOID **) &BlockIo, This->DriverBindingHandle,
                             ControllerHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (ControllerHandle, &gEfiDiskIoProtocolGuid,
                      (VOID **) &DiskIo, This->DriverBindingHandle,
                      ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = fsw_efi_AttachVolume (ControllerHandle, DiskIo, BlockIo);

  // on errors, close the opened protocols

  if (EFI_ERROR (Status)) {
    Status2 = gBS->OpenProtocol (ControllerHandle,
                         &gEfiSimpleFileSystemProtocolGuid, NULL,
                         This->DriverBindingHandle, ControllerHandle,
                         EFI_OPEN_PROTOCOL_TEST_PROTOCOL);

    if (EFI_ERROR (Status2)) {
      gBS->CloseProtocol (ControllerHandle, &gEfiDiskIoProtocolGuid,
                         This->DriverBindingHandle, ControllerHandle);
    }
  }

Exit:

  if (LockedByMe) {
    fsw_efi_ReleaseLock ();
  }

  return Status;
}

/**
 * Driver Binding EFI protocol, Stop function. This function is called by EFI
 * to stop the driver on the given device. This translates to an unmount
 * call for the FSW core.
 *
 * We assume that all file handles on the volume have been closed before
 * the driver is stopped. At least with the EFI shell, that is actually the
 * case; it closes all file handles between commands.
 */

EFI_STATUS EFIAPI
fsw_efi_DriverBinding_Stop (
  IN EFI_DRIVER_BINDING_PROTOCOL * This,
  IN EFI_HANDLE ControllerHandle,
  IN UINTN NumberOfChildren,
  IN EFI_HANDLE * ChildHandleBuffer
)
{
  EFI_STATUS Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  FSW_VOLUME_DATA *Volume;

  Status = gBS->OpenProtocol (ControllerHandle, &gEfiSimpleFileSystemProtocolGuid,
                      (VOID **) &FileSystem, This->DriverBindingHandle,
                      ControllerHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (!EFI_ERROR (Status)) {
    Volume = FSW_VOLUME_FROM_VOL_INTERFACE (FileSystem);
    fsw_efi_DetachVolume (Volume);
  }

  Status = gBS->CloseProtocol (ControllerHandle, &gEfiDiskIoProtocolGuid,
                       This->DriverBindingHandle, ControllerHandle);

  return Status;
}

/**
 * Component Name EFI protocol, GetDriverName function. Used by the EFI
 * environment to inquire the name of this driver. The name returned is
 * based on the file system type actually used in compilation.
 */

EFI_STATUS EFIAPI
fsw_efi_ComponentName_GetDriverName (
  IN EFI_COMPONENT_NAME_PROTOCOL * This,
  IN CHAR8 *Language,
  OUT CHAR16 **DriverName
)
{
  if (Language == NULL || DriverName == NULL)
    return EFI_INVALID_PARAMETER;

  return EFI_UNSUPPORTED;
}

/**
 * Component Name EFI protocol, GetControllerName function. Not implemented
 * because this is not a "bus" driver in the sense of the EFI Driver Model.
 */

EFI_STATUS EFIAPI
fsw_efi_ComponentName_GetControllerName (
  IN EFI_COMPONENT_NAME_PROTOCOL * This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_HANDLE ChildHandle OPTIONAL,
  IN CHAR8 *Language,
  OUT CHAR16 **ControllerName
)
{
  return EFI_UNSUPPORTED;
}

/**
 * FSW interface function for block size changes. This function is called by the FSW core
 * when the file system driver changes the block sizes for the volume.
 */

void
fsw_efi_change_blocksize (
  struct fsw_volume *vol,
  fsw_u32 old_phys_blocksize,
  fsw_u32 old_log_blocksize,
  fsw_u32 new_phys_blocksize,
  fsw_u32 new_log_blocksize
)
{
  // nothing to do
}

/**
 * FSW interface function to read data blocks. This function is called by the FSW core
 * to read a block of data from the device. The buffer is allocated by the core code.
 */

fsw_status_t
fsw_efi_read_block (
  struct fsw_volume *vol,
  fsw_u32 phys_bno,
  void *buffer
)
{
  EFI_STATUS Status;
  FSW_VOLUME_DATA *Volume = (FSW_VOLUME_DATA *) vol->host_data;

  // read from disk
  Status =
    Volume->DiskIo->ReadDisk (Volume->DiskIo, Volume->MediaId,
                              (UINT64) phys_bno * vol->phys_blocksize,
                              vol->phys_blocksize, buffer);
  Volume->LastIOStatus = Status;

  if (EFI_ERROR (Status)) {
    return FSW_IO_ERROR;
  }

  return FSW_SUCCESS;
}

/**
 * Map FSW status codes to EFI status codes. The FSW_IO_ERROR code is only produced
 * by fsw_efi_read_block, so we map it back to the EFI status code remembered from
 * the last I/O operation.
 */

EFI_STATUS
fsw_efi_map_status (
  fsw_status_t fsw_status,
  FSW_VOLUME_DATA * Volume
)
{
  switch (fsw_status) {
  case FSW_SUCCESS:
    return EFI_SUCCESS;
  case FSW_OUT_OF_MEMORY:
    return EFI_VOLUME_CORRUPTED;
  case FSW_IO_ERROR:
    return Volume->LastIOStatus;
  case FSW_UNSUPPORTED:
    return EFI_UNSUPPORTED;
  case FSW_NOT_FOUND:
    return EFI_NOT_FOUND;
  case FSW_VOLUME_CORRUPTED:
    return EFI_VOLUME_CORRUPTED;
  default:
    return EFI_DEVICE_ERROR;
  }
}

/**
 * File System EFI protocol, OpenVolume function. Creates a file handle for
 * the root directory and returns it. Note that this function may be called
 * multiple times and returns a new file handle each time. Each returned
 * handle is closed by the client using it.
 */

EFI_STATUS EFIAPI
fsw_efi_FileSystem_OpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL * This,
  OUT EFI_FILE ** Root
)
{
  EFI_STATUS Status;
  FSW_VOLUME_DATA *Volume = FSW_VOLUME_FROM_VOL_INTERFACE (This);

  Status = fsw_efi_dnode_to_FileHandle (Volume->vol->root, Root);

  return Status;
}

/**
 * File Handle EFI protocol, Open function. Dispatches the call
 * based on the kind of file handle.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_Open (
  IN EFI_FILE * This,
  OUT EFI_FILE ** NewHandle,
  IN CHAR16 *FileName,
  IN UINT64 OpenMode,
  IN UINT64 Attributes
)
{
  EFI_STATUS Status;
  FSW_FILE_DATA *File;

  Status = EFI_UNSUPPORTED;
  File = FSW_FILE_FROM_FILE_HANDLE (This);

  switch (File->Type) {
    case FSW_EFI_FILE_KIND_FILE:
      /* Already opened */

      Status = EFI_SUCCESS;
      break;

    case FSW_EFI_FILE_KIND_DIR:
      Status = fsw_efi_dir_open (File, NewHandle, FileName, OpenMode, Attributes);
      break;

    default:
      // Not supported for irregular files.
      break;
  }

  return Status;
}

/**
 * File Handle EFI protocol, Close function. Closes the FSW shandle
 * and frees the memory used for the structure.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_Close (
  IN EFI_FILE * This
)
{
  FSW_FILE_DATA *File;

  File = FSW_FILE_FROM_FILE_HANDLE (This);
  fsw_shandle_close (&File->shand);
  FreePool (File);

  return EFI_SUCCESS;
}

/**
 * File Handle EFI protocol, Delete function. Calls through to Close
 * and returns a warning because this driver is read-only.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_Delete (
  IN EFI_FILE * This
)
{
  EFI_STATUS Status;

  Status = This->Close (This);

  if (Status == EFI_SUCCESS) {
    // this driver is read-only

    Status = EFI_WARN_DELETE_FAILURE;
  }

  return Status;
}

/**
 * File Handle EFI protocol, Read function. Dispatches the call
 * based on the kind of file handle.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_Read (
  IN EFI_FILE * This,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
)
{
  EFI_STATUS Status;
  FSW_FILE_DATA *File;

  Status = EFI_UNSUPPORTED;
  File = FSW_FILE_FROM_FILE_HANDLE (This);

  if (File->Type == FSW_EFI_FILE_KIND_FILE)
    Status = fsw_efi_file_read (File, BufferSize, Buffer);
  else if (File->Type == FSW_EFI_FILE_KIND_DIR)
    Status = fsw_efi_dir_read (File, BufferSize, Buffer);

  return Status;
}

/**
 * File Handle EFI protocol, Write function. Returns unsupported status
 * because this driver is read-only.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_Write (
  IN EFI_FILE * This,
  IN OUT UINTN *BufferSize,
  IN VOID *Buffer
)
{
  // this driver is read-only

  return EFI_WRITE_PROTECTED;
}

/**
 * File Handle EFI protocol, GetPosition function. Dispatches the call
 * based on the kind of file handle.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_GetPosition (
  IN EFI_FILE * This,
  OUT UINT64 *Position
)
{
  EFI_STATUS Status;
  FSW_FILE_DATA *File;

  Status = EFI_UNSUPPORTED;
  File = FSW_FILE_FROM_FILE_HANDLE (This);

  if (File->Type == FSW_EFI_FILE_KIND_FILE)
    Status = fsw_efi_file_getpos (File, Position);

  // not defined for directories

  return Status;
}

/**
 * File Handle EFI protocol, SetPosition function. Dispatches the call
 * based on the kind of file handle.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_SetPosition (
  IN EFI_FILE * This,
  IN UINT64 Position
)
{
  EFI_STATUS Status;
  FSW_FILE_DATA *File;

  Status = EFI_UNSUPPORTED;
  File = FSW_FILE_FROM_FILE_HANDLE (This);

  if (File->Type == FSW_EFI_FILE_KIND_FILE)
    Status = fsw_efi_file_setpos (File, Position);
  else if (File->Type == FSW_EFI_FILE_KIND_DIR)
    Status = fsw_efi_dir_setpos (File, Position);

  return Status;
}

/**
 * File Handle EFI protocol, GetInfo function. Dispatches to the common
 * function implementing this.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_GetInfo (
  IN EFI_FILE * This,
  IN EFI_GUID *InformationType,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
)
{
  EFI_STATUS Status;
  FSW_FILE_DATA *File;

  File = FSW_FILE_FROM_FILE_HANDLE (This);

  Status = fsw_efi_dnode_getinfo (File, InformationType, BufferSize, Buffer);

  return Status;
}

/**
 * File Handle EFI protocol, SetInfo function. Returns unsupported status
 * because this driver is read-only.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_SetInfo (
  IN EFI_FILE * This,
  IN EFI_GUID *InformationType,
  IN UINTN BufferSize,
  IN VOID *Buffer
)
{
  // this driver is read-only

  return EFI_WRITE_PROTECTED;
}

/**
 * File Handle EFI protocol, Flush function. Returns unsupported status
 * because this driver is read-only.
 */

EFI_STATUS EFIAPI
fsw_efi_FileHandle_Flush (
  IN EFI_FILE * This
)
{
  // this driver is read-only

  return EFI_WRITE_PROTECTED;
}

/**
 * Set up a file handle for a dnode. This function allocates a data structure
 * for a file handle, opens a FSW shandle and populates the EFI_FILE structure
 * with the interface functions.
 */

EFI_STATUS
fsw_efi_dnode_to_FileHandle (
  IN struct fsw_dnode *dno,
  OUT EFI_FILE ** NewFileHandle
)
{
  EFI_STATUS Status;
  FSW_FILE_DATA *File;

  // make sure the dnode has complete info

  Status = fsw_efi_map_status (fsw_dnode_fill (dno), (FSW_VOLUME_DATA *) dno->vol->host_data);

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  // allocate file structure

  File = AllocateZeroPool (sizeof (FSW_FILE_DATA));

  if (File == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  File->Signature = FSW_FILE_DATA_SIGNATURE;

  // check type

  switch (dno->dkind) {
    case FSW_DNODE_KIND_FILE:
      File->Type = FSW_EFI_FILE_KIND_FILE;
      break;
    case FSW_DNODE_KIND_DIR:
      File->Type = FSW_EFI_FILE_KIND_DIR;
      break;
    default:
      FreePool(File);
      Status = EFI_UNSUPPORTED;
      goto Done;
  }

  // open shandle

  Status = fsw_efi_map_status (fsw_shandle_open (dno, &File->shand),
                        (FSW_VOLUME_DATA *) dno->vol->host_data);

  if (EFI_ERROR (Status)) {
    FreePool (File);
    goto Done;
  }

  // populate the file handle

  File->FileHandle.Revision = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  File->FileHandle.Open = fsw_efi_FileHandle_Open;
  File->FileHandle.Close = fsw_efi_FileHandle_Close;
  File->FileHandle.Delete = fsw_efi_FileHandle_Delete;
  File->FileHandle.Read = fsw_efi_FileHandle_Read;
  File->FileHandle.Write = fsw_efi_FileHandle_Write;
  File->FileHandle.GetPosition = fsw_efi_FileHandle_GetPosition;
  File->FileHandle.SetPosition = fsw_efi_FileHandle_SetPosition;
  File->FileHandle.GetInfo = fsw_efi_FileHandle_GetInfo;
  File->FileHandle.SetInfo = fsw_efi_FileHandle_SetInfo;
  File->FileHandle.Flush = fsw_efi_FileHandle_Flush;

  *NewFileHandle = &File->FileHandle;
  Status = EFI_SUCCESS;

Done:
  return Status;
}

/**
 * Data read function for regular files. Calls through to fsw_shandle_read.
 */

EFI_STATUS
fsw_efi_file_read (
  IN FSW_FILE_DATA * File,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
)
{
  EFI_STATUS Status;
  fsw_u32 buffer_size;

  buffer_size = (fsw_u32) (*BufferSize);
  Status = fsw_efi_map_status (fsw_shandle_read (&File->shand, &buffer_size, Buffer),
                        (FSW_VOLUME_DATA *) File->shand.dnode->vol->host_data);
  *BufferSize = buffer_size;

  return Status;
}

/**
 * Get file position for regular files.
 */

EFI_STATUS
fsw_efi_file_getpos (
  IN FSW_FILE_DATA * File,
  OUT UINT64 *Position
)
{
  *Position = File->shand.pos;

  return EFI_SUCCESS;
}

/**
 * Set file position for regular files. EFI specifies the all-ones value
 * to be a special value for the end of the file.
 */

EFI_STATUS
fsw_efi_file_setpos (
  IN FSW_FILE_DATA * File,
  IN UINT64 Position
)
{
  if (Position == 0xFFFFFFFFFFFFFFFFULL)
    File->shand.pos = File->shand.dnode->size;
  else
    File->shand.pos = Position;

  return EFI_SUCCESS;
}

/**
 * Open function used to open new file handles relative to a directory.
 * In EFI, the "open file" function is implemented by directory file handles
 * and is passed a relative or volume-absolute path to the file or directory
 * to open. We use fsw_dnode_lookup_path to find the node plus an additional
 * call to fsw_dnode_resolve because EFI has no concept of symbolic links.
 */

EFI_STATUS
fsw_efi_dir_open (
  IN FSW_FILE_DATA * File,
  OUT EFI_FILE ** NewHandle,
  IN CHAR16 *FileName,
  IN UINT64 OpenMode,
  IN UINT64 Attributes
)
{
  EFI_STATUS Status;
  FSW_VOLUME_DATA *Volume =
    (FSW_VOLUME_DATA *) File->shand.dnode->vol->host_data;
  struct fsw_dnode *dno;
  struct fsw_dnode *target_dno;
  struct fsw_string lookup_path;
  int lplen;

  if (OpenMode != EFI_FILE_MODE_READ)
    return EFI_WRITE_PROTECTED;

  lplen = (int) StrLen (FileName);
  fsw_string_setter(&lookup_path, FSW_STRING_KIND_UTF16, lplen, lplen * sizeof(fsw_u16), FileName);

  // resolve the path (symlinks along the way are automatically resolved)

  Status = fsw_efi_map_status (fsw_dnode_lookup_path
                        (File->shand.dnode, &lookup_path, '\\', &dno), Volume);

  if (EFI_ERROR (Status))
    return Status;

  // if the final node is a symlink, also resolve it

  Status = fsw_efi_map_status (fsw_dnode_resolve (dno, &target_dno), Volume);
  fsw_dnode_release (dno);

  if (EFI_ERROR (Status))
    return Status;

  dno = target_dno;

  // make a new EFI handle for the target dnode

  Status = fsw_efi_dnode_to_FileHandle (dno, NewHandle);
  fsw_dnode_release (dno);

  return Status;
}

/**
 * Read function for directories. A file handle read on a directory retrieves
 * the next directory entry.
 */

EFI_STATUS
fsw_efi_dir_read (
  IN FSW_FILE_DATA * File,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
)
{
  EFI_STATUS Status;
  FSW_VOLUME_DATA *Volume =
    (FSW_VOLUME_DATA *) File->shand.dnode->vol->host_data;
  struct fsw_dnode *dno;

  // read the next entry

  Status = fsw_efi_map_status (fsw_dnode_dir_read (&File->shand, &dno), Volume);

  if (Status == EFI_NOT_FOUND) {
    // end of directory

    *BufferSize = 0;
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status))
    return Status;

  // get info into buffer

  Status = fsw_efi_dnode_fill_FileInfo (Volume, dno, BufferSize, Buffer);
  fsw_dnode_release (dno);

  return Status;
}

/**
 * Set file position for directories. The only allowed set position operation
 * for directories is to rewind the directory completely by setting the
 * position to zero.
 */

EFI_STATUS
fsw_efi_dir_setpos (
  IN FSW_FILE_DATA * File,
  IN UINT64 Position
)
{
  if (Position == 0) {
    File->shand.pos = 0;
    return EFI_SUCCESS;
  } else {
    // directories can only rewind to the start
    return EFI_UNSUPPORTED;
  }
}

/* XXX: Actually fsw_hfs_vol_bless_info_id() expects fsw_hfs_volume *, but by design it would work */

fsw_u32 fsw_hfs_vol_bless_id (
  struct fsw_volume *vol,
  fsw_hfs_bless_kind_t bkind
);

EFI_STATUS
fsw_efi_bless_info (
  IN FSW_VOLUME_DATA *Volume,
  IN fsw_hfs_bless_kind_t bkind,
  OUT VOID *Buffer,
  IN OUT UINTN *BufferSize
)
{
  fsw_status_t fstatus;
  EFI_STATUS Status = EFI_UNSUPPORTED;
  fsw_u32 bnid;
  UINTN RequiredSize;
  UINT16 *tmpStr;
  UINT16 *tmpPtr;
  struct fsw_string_list *path = NULL;
  struct fsw_string_list *p2;
  fsw_u32 names;
  fsw_u32 chars;
  EFI_DEVICE_PATH_PROTOCOL *dpp;

  bnid = fsw_hfs_vol_bless_id (Volume->vol, bkind);

  if (bnid == 0)
    return EFI_NOT_FOUND;

  fstatus = fsw_dnode_id_fullpath (Volume->vol, bnid, FSW_STRING_KIND_UTF16, &path);

  if (fstatus != FSW_SUCCESS)
    return EFI_NOT_FOUND;

  fsw_string_list_lengths (path, &names, &chars);

  RequiredSize = (names + chars + 1 + 1) * sizeof (UINT16);
  tmpStr = AllocatePool(RequiredSize);

  if (tmpStr == NULL) {
    fsw_string_list_free(path);
    return EFI_OUT_OF_RESOURCES;
  }

  tmpPtr = tmpStr;

  for(p2 = path; p2 != NULL; p2 = p2->flink) {
    *tmpPtr++ = L'\\';
    fsw_efi_strcpy (tmpPtr, p2->str);
    tmpPtr += fsw_strlen(p2->str);
  }

  fsw_string_list_free(path);

  if (tmpPtr == tmpStr)	/* Root CNID returns 0 names and 0 chars */
    *tmpPtr++ = L'\\';

  *tmpPtr = 0;

  dpp = FileDevicePath (Volume->Handle, tmpStr);
  FreePool(tmpStr);

  if (dpp == NULL)
    return EFI_OUT_OF_RESOURCES;

  RequiredSize = GetDevicePathSize (dpp);

  if (*BufferSize < RequiredSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    CopyMem (Buffer, dpp, RequiredSize);
    Status = EFI_SUCCESS;
    DEBUG_CODE_BEGIN();
    tmpStr = ConvertDevicePathToText(dpp, TRUE, TRUE);
    if (tmpStr != NULL) {
      FSW_MSG_DEBUGV ((FSW_MSGSTR ("%a: handle %p, {devpath %s}\n"), __FUNCTION__, Volume, tmpStr));
      FreePool(tmpStr);
    }
    DEBUG_CODE_END();
  }

  *BufferSize = RequiredSize;
  FreePool (dpp);

  return Status;
}

/**
 * Get file or volume information. This function implements the GetInfo call
 * for all file handles. Control is dispatched according to the type of information
 * requested by the caller.
 */

EFI_STATUS
fsw_efi_dnode_getinfo (
  IN FSW_FILE_DATA * File,
  IN EFI_GUID *InformationType,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
)
{
  EFI_STATUS Status;
  FSW_VOLUME_DATA *Volume;
  EFI_FILE_SYSTEM_INFO *FSInfo;
  UINTN RequiredSize;
  struct fsw_volume_stat vsb;

  Volume = (FSW_VOLUME_DATA *) File->shand.dnode->vol->host_data;

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Status = fsw_efi_dnode_fill_FileInfo (Volume, File->shand.dnode, BufferSize, Buffer);
  } else if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {

    // check buffer size

    RequiredSize = fsw_efi_strsize (&Volume->vol->label) + SIZE_OF_EFI_FILE_SYSTEM_INFO;

    if (*BufferSize < RequiredSize) {
      *BufferSize = RequiredSize;
      Status = EFI_BUFFER_TOO_SMALL;
      goto Done;
    }

    // fill structure

    FSInfo = (EFI_FILE_SYSTEM_INFO *) Buffer;
    FSInfo->Size = RequiredSize;
    FSInfo->ReadOnly = TRUE;
    FSInfo->BlockSize = Volume->vol->log_blocksize;
    fsw_efi_strcpy (FSInfo->VolumeLabel, &Volume->vol->label);

    // get the missing info from the fs driver

    ZeroMem (&vsb, sizeof (struct fsw_volume_stat));
    Status = fsw_efi_map_status (fsw_volume_stat (Volume->vol, &vsb), Volume);

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    FSInfo->VolumeSize = vsb.total_bytes;
    FSInfo->FreeSpace = vsb.free_bytes;

    // prepare for return

    *BufferSize = RequiredSize;
    Status = EFI_SUCCESS;
  } else if (CompareGuid (InformationType, &gEfiFileSystemVolumeLabelInfoIdGuid)) {

    // check buffer size

    RequiredSize = SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL + fsw_efi_strsize (&Volume->vol->label);

    if (*BufferSize < RequiredSize) {
      *BufferSize = RequiredSize;
      Status = EFI_BUFFER_TOO_SMALL;
      goto Done;
    }

    // copy volume label

    fsw_efi_strcpy (((EFI_FILE_SYSTEM_VOLUME_LABEL *) Buffer)->VolumeLabel, &Volume->vol->label);

    // prepare for return

    *BufferSize = RequiredSize;
    Status = EFI_SUCCESS;
  } else if (CompareGuid (InformationType, &gAppleBlessedSystemFileInfoGuid)) {
    Status = fsw_efi_bless_info (Volume, HFS_BLESS_SYSFILE, Buffer, BufferSize);
  } else if (CompareGuid (InformationType, &gAppleBlessedSystemFolderInfoGuid)) {
    Status = fsw_efi_bless_info (Volume, HFS_BLESS_SYSFLDR, Buffer, BufferSize);
  } else if (CompareGuid (InformationType, &gAppleBlessedOsxFolderInfoGuid)) {
    Status = fsw_efi_bless_info (Volume, HFS_BLESS_OSXFLDR, Buffer, BufferSize);
  } else {
    FSW_MSG_DEBUGV ((FSW_MSGSTR ("%a: Unsupported request (guid %g)\n"), __FUNCTION__, InformationType));
    Status = EFI_UNSUPPORTED;
  }

Done:

  return Status;
}

/**
 * Time mapping callback for the fsw_dnode_stat call. This function converts
 * a Posix style timestamp into an EFI_TIME structure and writes it to the
 * appropriate member of the EFI_FILE_INFO structure that we're filling.
 */

static void
fsw_efi_store_time_posix (
  struct fsw_dnode_stat *sb,
  int which,
  fsw_u32 posix_time
)
{
  EFI_FILE_INFO *FileInfo = (EFI_FILE_INFO *) sb->host_data;

  if (which == FSW_DNODE_STAT_CTIME)
    fsw_efi_decode_time (&FileInfo->CreateTime, posix_time);
  else if (which == FSW_DNODE_STAT_MTIME)
    fsw_efi_decode_time (&FileInfo->ModificationTime, posix_time);
  else if (which == FSW_DNODE_STAT_ATIME)
    fsw_efi_decode_time (&FileInfo->LastAccessTime, posix_time);
}

/**
 * Mode mapping callback for the fsw_dnode_stat call. This function looks at
 * the Posix mode passed by the file system driver and makes appropriate
 * adjustments to the EFI_FILE_INFO structure that we're filling.
 */

static void
fsw_efi_store_attr_posix (
  struct fsw_dnode_stat *sb,
  fsw_u16 posix_mode
)
{
  EFI_FILE_INFO *FileInfo = (EFI_FILE_INFO *) sb->host_data;

  if ((posix_mode & S_IWUSR) == 0)
    FileInfo->Attribute |= EFI_FILE_READ_ONLY;
}

/**
 * Common function to fill an EFI_FILE_INFO with information about a dnode.
 */

EFI_STATUS
fsw_efi_dnode_fill_FileInfo (
  IN FSW_VOLUME_DATA * Volume,
  IN struct fsw_dnode *dno,
  IN OUT UINTN *BufferSize,
  OUT VOID *Buffer
)
{
  EFI_STATUS Status;
  EFI_FILE_INFO *FileInfo;
  UINTN RequiredSize;
  struct fsw_dnode_stat sb;
  struct fsw_dnode *target_dno;

  // make sure the dnode has complete info

  Status = fsw_efi_map_status (fsw_dnode_fill (dno), Volume);

  if (EFI_ERROR (Status))
    return Status;

  // TODO: check/assert that the dno's name is in UTF16

  // check buffer size

  RequiredSize = SIZE_OF_EFI_FILE_INFO + fsw_efi_strsize (&dno->name);

  if (*BufferSize < RequiredSize) {
    // TODO: wind back the directory in this case

    *BufferSize = RequiredSize;

    return EFI_BUFFER_TOO_SMALL;
  }

  // fill structure

  ZeroMem (Buffer, RequiredSize);
  FileInfo = (EFI_FILE_INFO *) Buffer;

  // Use original name (name of symlink if any)

  fsw_efi_strcpy (FileInfo->FileName, &dno->name);

  // if the node is a symlink, resolve it

  Status = fsw_efi_map_status (fsw_dnode_resolve (dno, &target_dno), Volume);

  if (EFI_ERROR (Status))
    return Status;

  // make sure the dnode has complete info

  Status = fsw_efi_map_status (fsw_dnode_fill (target_dno), Volume);

  if (EFI_ERROR (Status)) {
    fsw_dnode_release (target_dno);
    return Status;
  }

  fsw_dnode_release (dno);
  dno = target_dno;

  FileInfo->Size = RequiredSize;
  FileInfo->FileSize = dno->size;
  FileInfo->Attribute = EFI_FILE_READ_ONLY;

  if (dno->dkind == FSW_DNODE_KIND_DIR)
    FileInfo->Attribute |= EFI_FILE_DIRECTORY;

  // get the missing info from the fs driver

  ZeroMem (&sb, sizeof (struct fsw_dnode_stat));
  sb.store_time_posix = fsw_efi_store_time_posix;
  sb.store_attr_posix = fsw_efi_store_attr_posix;
  sb.host_data = FileInfo;
  Status = fsw_efi_map_status (fsw_dnode_stat (dno, &sb), Volume);

  if (EFI_ERROR (Status))
    return Status;

  FileInfo->PhysicalSize = sb.used_bytes;

  // prepare for return

  *BufferSize = RequiredSize;
  return EFI_SUCCESS;
}

// EOF
