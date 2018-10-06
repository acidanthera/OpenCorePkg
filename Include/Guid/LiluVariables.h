/** @file
  Lilu specific GUIDs for UEFI Variable Storage, version 1.0.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// 2660DD78-81D2-419D-8138-7B1F363F79A6
// This GUID is specifically used for normal variable access by Lilu kernel extension and its plugins.
//
#define LILU_NORMAL_VARIABLE_GUID \
  { 0x2660DD78, 0x81D2, 0x419D, { 0x81, 0x38, 0x7B, 0x1F, 0x36, 0x3F, 0x79, 0xA6 } }

//
// E09B9297-7928-4440-9AAB-D1F8536FBF0A
// This GUID is specifically used for reading variables by Lilu kernel extension and its plugins.
// Any writes to this GUID should be prohibited via EFI_RUNTIME_SERVICES after EXIT_BOOT_SERVICES.
// The expected return code on variable write is EFI_SECURITY_VIOLATION.
//
#define LILU_READ_ONLY_VARIABLE_GUID \
  { 0xE09B9297, 0x7928, 0x4440, { 0x9A, 0xAB, 0xD1, 0xF8, 0x53, 0x6F, 0xBF, 0x0A } }

//
// F0B9AF8F-2222-4840-8A37-ECF7CC8C12E1
// This GUID is specifically used for reading variables by Lilu and plugins.
// Any reads from this GUID should be prohibited via EFI_RUNTIME_SERVICES after EXIT_BOOT_SERVICES.
// The expected return code on variable read is EFI_SECURITY_VIOLATION.
//
#define LILU_WRITE_ONLY_VARIABLE_GUID \
  { 0xF0B9AF8F, 0x2222, 0x4840, { 0x8A, 0x37, 0xEC, 0xF7, 0xCC, 0x8C, 0x12, 0xE1 } }
