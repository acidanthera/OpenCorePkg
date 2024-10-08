/** @file
  Copyright (C) 2024, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef LOAD_FILE_INTERNAL_H
#define LOAD_FILE_INTERNAL_H

#include <Uefi.h>
#include <Uefi/UefiSpec.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcFlexArrayLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVirtualFsLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/HttpBootCallback.h>
#include <Protocol/LoadFile.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OcBootEntry.h>
#include <Protocol/RamDisk.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/TlsAuthentication.h>

/**
  Set if we should enforce https only within this driver.
**/
extern BOOLEAN  gRequireHttpsUri;

/**
  Current DmgLoading setting, for HTTP BOOT callback validation.
**/
extern OC_DMG_LOADING_SUPPORT  gDmgLoading;

/**
  Custom validation for network boot device path.

  @param Path         Device path to validate.

  @retval EFI_SUCCESS           Device path should be accepted.
  @retval other                 Device path should be rejected.
**/
typedef
EFI_STATUS
(*VALIDATE_BOOT_DEVICE_PATH)(
  IN VOID                      *Context,
  IN EFI_DEVICE_PATH_PROTOCOL  *Path
  );

/*
  Return pointer to final node in device path, if it as a URI node.
  Used to return the URI node for an HTTP Boot device path.

  @return Required device path node if available, NULL otherwise.
*/
EFI_DEVICE_PATH_PROTOCOL *
GetUriNode (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

///
/// BmBootDescription.c
///

CHAR16 *
BmGetNetworkDescription (
  IN EFI_HANDLE  Handle
  );

///
/// BmBoot.c
///

EFI_DEVICE_PATH_PROTOCOL *
BmExpandLoadFiles (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT VOID                      **Data,
  OUT UINT32                    *DataSize,
  IN BOOLEAN                    ValidateHttp
  );

EFI_DEVICE_PATH_PROTOCOL *
BmGetRamDiskDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

VOID
BmDestroyRamDisk (
  IN EFI_DEVICE_PATH_PROTOCOL  *RamDiskDevicePath
  );

///
/// CustomRead.c
///

EFI_STATUS
EFIAPI
HttpBootCustomFree (
  IN  VOID  *Context
  );

EFI_STATUS
EFIAPI
HttpBootCustomRead (
  IN  OC_STORAGE_CONTEXT                   *Storage,
  IN  OC_BOOT_ENTRY                        *ChosenEntry,
  OUT VOID                                 **Data,
  OUT UINT32                               *DataSize,
  OUT EFI_DEVICE_PATH_PROTOCOL             **DevicePath,
  OUT EFI_HANDLE                           *StorageHandle,
  OUT EFI_DEVICE_PATH_PROTOCOL             **StoragePath,
  IN  OC_DMG_LOADING_SUPPORT               DmgLoading,
  OUT OC_APPLE_DISK_IMAGE_PRELOAD_CONTEXT  *DmgPreloadContext,
  OUT VOID                                 **Context
  );

EFI_STATUS
EFIAPI
PxeBootCustomRead (
  IN  OC_STORAGE_CONTEXT                   *Storage,
  IN  OC_BOOT_ENTRY                        *ChosenEntry,
  OUT VOID                                 **Data,
  OUT UINT32                               *DataSize,
  OUT EFI_DEVICE_PATH_PROTOCOL             **DevicePath,
  OUT EFI_HANDLE                           *StorageHandle,
  OUT EFI_DEVICE_PATH_PROTOCOL             **StoragePath,
  IN  OC_DMG_LOADING_SUPPORT               DmgLoading,
  OUT OC_APPLE_DISK_IMAGE_PRELOAD_CONTEXT  *DmgPreloadContext,
  OUT VOID                                 **Context
  );

///
/// Uri.c
///

BOOLEAN
HasHttpsUri (
  CHAR16  *Uri
  );

EFI_STATUS
ExtractOtherUriFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR8                     *FromExt,
  IN  CHAR8                     *ToExt,
  OUT CHAR8                     **OtherUri,
  IN  BOOLEAN                   OnlySearchForFromExt
  );

BOOLEAN
UriFileHasExtension (
  IN  CHAR8  *Uri,
  IN  CHAR8  *Ext
  );

EFI_STATUS
HttpBootAddUri (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  VOID                      *Uri,
  OC_STRING_FORMAT          StringFormat,
  EFI_DEVICE_PATH_PROTOCOL  **UriDevicePath
  );

EFI_EVENT
MonitorHttpBootCallback (
  EFI_HANDLE  LoadFileHandle
  );

BOOLEAN
UriWasValidated (
  VOID
  );

///
/// TlsAuthConfigImpl.c
///

EFI_STATUS
LogInstalledCerts (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  );

EFI_STATUS
CertIsPresent (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN EFI_GUID  *OwnerGuid,
  IN UINTN     X509DataSize,
  IN VOID      *X509Data
  );

EFI_STATUS
DeleteCertsForOwner (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN EFI_GUID  *OwnerGuid,
  IN UINTN     X509DataSize,
  IN VOID      *X509Data,
  OUT UINTN    *DeletedCount
  );

EFI_STATUS
EnrollX509toVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN EFI_GUID  *OwnerGuid,
  IN UINTN     X509DataSize,
  IN VOID      *X509Data
  );

#endif // LOAD_FILE_INTERNAL_H
