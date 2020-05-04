/** @file
  Copyright (C) 2017-2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_SMC_LIB_INTERNAL_H
#define OC_SMC_LIB_INTERNAL_H

#include <Guid/OcVariable.h>
#include <Protocol/AppleSmcIo.h>

typedef struct {
	SMC_KEY             Key;
	SMC_KEY_TYPE        Type;
	SMC_DATA_SIZE       Size;
	SMC_KEY_ATTRIBUTES  Attributes;
	SMC_DATA            Data[SMC_MAX_DATA_SIZE];
} VIRTUALSMC_KEY_VALUE;

#define VIRTUALSMC_STATUS_KEY           L"vsmc-status"
#define VIRTUALSMC_ENCRYPTION_KEY       L"vsmc-key"

#endif // OC_SMC_LIB_INTERNAL_H
