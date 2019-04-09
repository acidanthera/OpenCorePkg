/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_LOG_VARIABLE_GUID_H
#define OC_LOG_VARIABLE_GUID_H

#define OC_LOG_VARIABLE_NAME  L"boot-log"

/// Definition for OcLog Variable
/// 4D1FDA02-38C7-4A6A-9CC6-4BCCA8B30102
#define OC_LOG_VARIABLE_GUID                                                        \
  {                                                                                 \
    0x4D1FDA02, 0x38C7, 0x4A6A, { 0x9C, 0xC6, 0x4B, 0xCC, 0xA8, 0xB3, 0x01, 0x02 }  \
  }

extern EFI_GUID gOcLogVariableGuid;

#endif // OC_LOG_VARIABLE_GUID_H
