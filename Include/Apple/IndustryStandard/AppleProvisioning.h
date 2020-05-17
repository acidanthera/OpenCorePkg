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

#ifndef APPLE_PROVISIONING_H
#define APPLE_PROVISIONING_H

#include <Pi/PiHob.h>

//
// D1A26C1F-ABF5-4806-BB24-68D317E071D5
//
#define APPLE_EPID_CERTIFICATE_FILE_GUID \
  { 0xD1A26C1F, 0xABF5, 0x4806, { 0xBB, 0x24, 0x68, 0xD3, 0x17, 0xE0, 0x71, 0xD5 } }

//
// 2906CC1F-09CA-4457-9A4F-C212C545D3D3
//
#define APPLE_EPID_GROUP_PUBLIC_KEYS_FILE_GUID \
  { 0x2906CC1F, 0x09CA, 0x4457, { 0x9A, 0x4F, 0xC2, 0x12, 0xC5, 0x45, 0xD3, 0xD3 } }

//
// E3CC8EC6-81C1-4271-ACBC-DB65086E8DC8
// This HOB is set on all models in 38317FC0-2795-4DE6-B207-680CA768CFB1 PEI module.
// Note, that it just uses some higher address in the firmware for the value.
//
#define APPLE_FPF_CONFIGURATION_HOB_GUID \
  { 0xE3CC8EC6, 0x81C1, 0x4271, { 0xAC, 0xBC, 0xDB, 0x65, 0x08, 0x6E, 0x8D, 0xC8 } }

typedef struct {
  EFI_HOB_GENERIC_HEADER      Header;
  EFI_GUID                    Name;
  BOOLEAN                     ShouldProvision;
} APPLE_FPF_CONFIGURATION_HOB;

extern EFI_GUID gAppleEpidCertificateFileGuid;
extern EFI_GUID gAppleEpidGroupPublicKeysFileGuid;
extern EFI_GUID gAppleFpfConfigurationHobGuid;

#define APPLE_EPID_PROVISIONED_VARIABLE_NAME L"epid_provisioned"

#define APPLE_FPF_PROVISIONED_VARIABLE_NAME  L"fpf_provisioned"

#endif // APPLE_PROVISIONING_H
