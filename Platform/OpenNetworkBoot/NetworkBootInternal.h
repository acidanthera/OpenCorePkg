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
#include <Library/NetLib.h>
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
#include <Protocol/Ip4Config2.h>
#include <Protocol/LoadFile.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/OcBootEntry.h>
#include <Protocol/RamDisk.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/TlsAuthentication.h>

/////
// Ip4Config2Impl.h
//
typedef struct {
  UINT16                       Offset;
  UINT32                       DataSize;
  EFI_IP4_CONFIG2_DATA_TYPE    DataType;
} IP4_CONFIG2_DATA_RECORD;

//
// Modified from original Ip4Config2Impl.h version to use flexible array member.
//
typedef struct {
  UINT16                     Checksum;
  UINT16                     DataRecordCount;
  IP4_CONFIG2_DATA_RECORD    DataRecord[];
} IP4_CONFIG2_VARIABLE;

#define IP4_CONFIG2_VARIABLE_ATTRIBUTE  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
//
// Ip4Config2Impl.h
/////

/////
// Ip4Common.h
//
#define IP4_ALLZERO_ADDRESS  0x00000000u
//
// Ip4Common.h
/////

typedef struct {
  CHAR16    *StationAddress;
  CHAR16    *SubnetMask;
  CHAR16    *GatewayAddress;
  CHAR16    *DnsAddress;
} IP4_CONFIG2_OC_CONFIG_DATA;

EFI_STATUS
Ip4Config2ConvertOcConfigDataToNvData (
  IN CHAR16                      *VarName,
  IN IP4_CONFIG2_OC_CONFIG_DATA  *ConfigData
  );

EFI_STATUS
Ip4Config2DeleteStaticIpNvData (
  IN     CHAR16  *VarName
  );

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

///
/// StaticIp4.c
///

EFI_STATUS
AddRemoveStaticIPs (
  OC_FLEX_ARRAY  *ParsedLoadOptions
  );

///
/// Ip4Utils.c
///

BOOLEAN
Ip4StationAddressValid (
  IN IP4_ADDR  Ip,
  IN IP4_ADDR  Netmask
  );

UINT8
GetSubnetMaskPrefixLength (
  IN EFI_IPv4_ADDRESS  *SubnetMask
  );

EFI_STATUS
Ip4Config2StrToIp (
  IN  CHAR16            *Str,
  OUT EFI_IPv4_ADDRESS  *Ip
  );

EFI_STATUS
Ip4Config2StrToIpList (
  IN  CHAR16            *Str,
  OUT EFI_IPv4_ADDRESS  **PtrIpList,
  OUT UINTN             *IpCount
  );

#endif // LOAD_FILE_INTERNAL_H
