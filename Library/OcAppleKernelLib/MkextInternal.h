/** @file
  Mkext support.

  Copyright (c) 2020, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef MKEXT_INTERNAL_H
#define MKEXT_INTERNAL_H

#include <Library/OcAppleKernelLib.h>

//
// Cached mkext kext.
//
typedef struct {
  //
  // Signature.
  //
  UINT32              Signature;
  //
  // Link for global list (MKEXT_CONTEXT -> CachedKexts).
  //
  LIST_ENTRY          Link;
  //
  // Kext bundle identifier.
  //
  CHAR8               *Identifier;
  //
  // Offset of binary in mkext.
  //
  UINT32              BinaryOffset;
  //
  // Size of binary in mkext.
  //
  UINT32              BinarySize;
} MKEXT_KEXT;

//
// MKEXT_KEXT signature for list identification.
//
#define MKEXT_KEXT_SIGNATURE  SIGNATURE_32 ('M', 'k', 'x', 'T')

/**
  Gets the next element in CachedKexts list of MKEXT_KEXT.

  @param[in] This  The current ListEntry.
**/
#define GET_MKEXT_KEXT_FROM_LINK(This)  \
  (CR (                                 \
    (This),                             \
    MKEXT_KEXT,                         \
    Link,                               \
    MKEXT_KEXT_SIGNATURE                \
    ))


MKEXT_KEXT *
InternalCachedMkextKext (
  IN OUT MKEXT_CONTEXT      *Context,
  IN     CONST CHAR8        *BundleId
  );

#endif // MKEXT_INTERNAL_H
