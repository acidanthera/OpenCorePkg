/** @file
  Apple Disk Image protocol.

Copyright (C) 2019, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DISK_IMAGE_PROTOCOL_H
#define APPLE_DISK_IMAGE_PROTOCOL_H

/**
  Apple Disk Image protocol GUID.
  004B07E8-0B9C-427E-B0D4-A466E6E57A62
**/
#define APPLE_DISK_IMAGE_PROTOCOL_GUID \
  { 0x004B07E8, 0x0B9C, 0x427E,        \
    { 0xB0, 0xD4, 0xA4, 0x66, 0xE6, 0xE5, 0x7A, 0x62 } }

/**
  Apple Disk Image protocol revision.
**/
#define APPLE_DISK_IMAGE_PROTOCOL_REVISION 2

/**
  Checks whether dmg file at DevicePath is valid.
  Essentially this is done by verifying last 512 bytes
  of the file.

  @param[in] DevicePath  Path to dmg file.

  @retval EFI_SUCCESS           Dmg looks valid and can be loaded.
  @retval EFI_UNSUPPORTED       Dmg is unsupported.
  @retval EFI_NOT_FOUND         Dmg was not found at this device path.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation error happened.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_DISK_IMAGE_SUPPORTED) (
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  );

/**
  Mounts dmg file at DevicePath providing relevant protocols:
  - gEfiDevicePathProtocolGuid
  - gEfiBlockIoProtocolGuid
  - gTDMApprovedGuid
  - gAppleDiskImageProtocolGuid (as NULL)
  Mounted dmg handle is to be connnected recursively on all protocols
  with connection status unchecked.

  Note, that DiskImage protocol does not protect DMG memory from the kernel.
  It only works in UEFI scope. For the kernel to boot the parent protocol,
  namely RamDisk, should have us covered by allocating DMG extent memory
  as wired (EfiACPIMemoryNVS).

  @param[in]  DevicePath  Path to dmg file.
  @param[out] Handle      Dmg handle.

  @retval EFI_SUCCESS           Dmg was mounted with relevant protocols.
  @retval EFI_INVALID_PARAMETER Dmg is not valid.
  @retval EFI_UNSUPPORTED       Dmg is less than 512 bytes.
  @retval EFI_NOT_FOUND         Dmg was not found at this device path.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation error happened.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_DISK_IMAGE_MOUNT_IMAGE) (
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  OUT EFI_HANDLE                 *Handle OPTIONAL
  );

/**
  Unmounts dmg file at handle and uninstalls the following protocols:
  - gEfiDevicePathProtocolGuid
  - gEfiBlockIoProtocolGuid
  - gAppleDiskImageProtocolGuid
  All the resources consumed by the dmg will be freed.

  @param[in] Handle      Dmg handle.

  @retval EFI_SUCCESS           Dmg was unmounted with relevant protocols.
  @retval EFI_INVALID_PARAMETER Dmg handle is not valid.
  @retval EFI_NOT_FOUND         Relevant protocols were not found.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_DISK_IMAGE_UNMOUNT_IMAGE) (
  IN  EFI_HANDLE                 Handle
  );

/**
  Apple disk imge protocol.
**/
typedef struct {
  UINT32                         Revision;
  APPLE_DISK_IMAGE_SUPPORTED     Supported;
  APPLE_DISK_IMAGE_MOUNT_IMAGE   MountImage;
  APPLE_DISK_IMAGE_UNMOUNT_IMAGE UnmountImage;
} APPLE_DISK_IMAGE_PROTOCOL;

extern EFI_GUID gAppleDiskImageProtocolGuid;

#endif // APPLE_DISK_IMAGE_PROTOCOL_H
