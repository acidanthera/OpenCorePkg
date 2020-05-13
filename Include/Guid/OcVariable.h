/** @file
  Lilu & OpenCore specific GUIDs for UEFI Variable Storage, version 1.0.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_VARIABLE_H
#define OC_VARIABLE_H

//
// Variable used for OpenCore log storage (if enabled).
//
#define OC_LOG_VARIABLE_NAME                 L"boot-log"

//
// Variable used for OpenCore boot path (if enabled).
//
#define OC_LOG_VARIABLE_PATH                 L"boot-path"

//
// Variable used for OpenCore request to redirect NVRAM Boot variable write.
// Boot Services only.
// See: https://github.com/acidanthera/bugtracker/issues/308.
//
#define OC_BOOT_REDIRECT_VARIABLE_NAME       L"boot-redirect"

//
// Variable used for exposing OpenCore Security -> LoadPolicy.
// Boot Services only.
//
#define OC_LOAD_POLICY_VARIABLE_NAME         L"load-policy"

//
// Variable used for exposing OpenCore Security -> ScanPolicy.
// Boot Services only.
//
#define OC_SCAN_POLICY_VARIABLE_NAME         L"scan-policy"

//
// Variable used to report boot protection.
//
#define OC_BOOT_PROTECT_VARIABLE_NAME        L"boot-protect"

//
// Boot protection guids.
//
#define OC_BOOT_PROTECT_VARIABLE_BOOTSTRAP BIT0
#define OC_BOOT_PROTECT_VARIABLE_NAMESPACE BIT1

//
// Variable used to report OpenCore version in the following format:
// REL-001-2019-01-01. This follows versioning style of Lilu and plugins.
//
#define OC_VERSION_VARIABLE_NAME             L"opencore-version"

//
// Variable used to report OEM product from SMBIOS Type1 ProductName.
//
#define OC_OEM_PRODUCT_VARIABLE_NAME         L"oem-product"

//
// Variable used to report OEM board vendor from SMBIOS Type2 Manufacturer.
//
#define OC_OEM_VENDOR_VARIABLE_NAME          L"oem-vendor"

//
// Variable used to report OEM board vendor from SMBIOS Type2 ProductName.
//
#define OC_OEM_BOARD_VARIABLE_NAME           L"oem-board"

//
// Variable used to share CPU frequency calculated from ACPI between the drivers.
//
#define OC_ACPI_CPU_FREQUENCY_VARIABLE_NAME  L"acpi-cpu-frequency"

//
// Variable used to mark blacklisted RTC values.
//
#define OC_RTC_BLACKLIST_VARIABLE_NAME       L"rtc-blacklist"

//
// 4D1FDA02-38C7-4A6A-9CC6-4BCCA8B30102
// This GUID is specifically used for normal variable access by Lilu kernel extension and its plugins.
//
#define OC_VENDOR_VARIABLE_GUID \
  { 0x4D1FDA02, 0x38C7, 0x4A6A, { 0x9C, 0xC6, 0x4B, 0xCC, 0xA8, 0xB3, 0x01, 0x02 } }

//
// E09B9297-7928-4440-9AAB-D1F8536FBF0A
// This GUID is specifically used for reading variables by Lilu kernel extension and its plugins.
// Any writes to this GUID should be prohibited via EFI_RUNTIME_SERVICES after EXIT_BOOT_SERVICES.
// The expected return code on variable write is EFI_SECURITY_VIOLATION.
//
#define OC_READ_ONLY_VARIABLE_GUID \
  { 0xE09B9297, 0x7928, 0x4440, { 0x9A, 0xAB, 0xD1, 0xF8, 0x53, 0x6F, 0xBF, 0x0A } }

//
// F0B9AF8F-2222-4840-8A37-ECF7CC8C12E1
// This GUID is specifically used for reading variables by Lilu and plugins.
// Any reads from this GUID should be prohibited via EFI_RUNTIME_SERVICES after EXIT_BOOT_SERVICES.
// The expected return code on variable read is EFI_SECURITY_VIOLATION.
//
#define OC_WRITE_ONLY_VARIABLE_GUID \
  { 0xF0B9AF8F, 0x2222, 0x4840, { 0x8A, 0x37, 0xEC, 0xF7, 0xCC, 0x8C, 0x12, 0xE1 } }

//
// External global variables with GUID values.
//
extern EFI_GUID gOcVendorVariableGuid;
extern EFI_GUID gOcReadOnlyVariableGuid;
extern EFI_GUID gOcWriteOnlyVariableGuid;

#endif // OC_VARIABLE_H
