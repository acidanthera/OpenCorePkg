/** @file

OcCryptoLib

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_SHA2_INTERNAL_H
#define OC_SHA2_INTERNAL_H

#include "CryptoInternal.h"

extern BOOLEAN mIsAccelEnabled;

VOID
EFIAPI
Sha512TransformAccel (
  IN OUT UINT64      *State,
  IN     CONST UINT8 *Data,
  IN     UINTN       BlockNb
  );

#endif // OC_SHA2_INTERNAL_H
