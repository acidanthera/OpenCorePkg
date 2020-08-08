/** @file
  Cacheless boot (S/L/E) support.

  Copyright (c) 2020, Goldfish64. All rights reserved.
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef CACHELESS_INTERNAL_H
#define CACHELESS_INTERNAL_H

#include <Library/OcAppleKernelLib.h>

//
// Names are of format OcXXXXXXXX.kext, where XXXXXXXX is a 32-bit hexadecimal number.
//
#define KEXT_BUNDLE_NAME          L"OcXXXXXXXX.kext"
#define KEXT_BUNDLE_NAME_SIZE     (L_STR_SIZE (KEXT_BUNDLE_NAME))
#define KEXT_BUNDLE_NAME_LEN      (L_STR_LEN (KEXT_BUNDLE_NAME))
#define KEXT_BUNDLE_INFO_SIZE     (SIZE_OF_EFI_FILE_INFO + KEXT_BUNDLE_NAME_SIZE)

enum {
  KEXT_OSBUNDLE_REQUIRED_NONE = 0,
  KEXT_OSBUNDLE_REQUIRED_INVALID,
  KEXT_OSBUNDLE_REQUIRED_VALID
};

//
// Kext dependency.
//
typedef struct {
  //
  // Signature.
  //
  UINT32              Signature;
  //
  // Link for global list.
  //
  LIST_ENTRY          Link;
  //
  // Bundle ID.
  //
  CHAR8               *BundleId;
} DEPEND_KEXT;

//
// Cacheless kext.
//
typedef struct {
  //
  // Signature.
  //
  UINT32              Signature;
  //
  // Link for global list (CACHELESS_CONTEXT -> InjectedKexts).
  //
  LIST_ENTRY          Link;
  //
  // Bundle filename used during S/L/E overlay creation.
  //
  CHAR16              BundleFileName[KEXT_BUNDLE_NAME_LEN + 1];
  //
  // Plist data.
  //
  CHAR8               *PlistData;
  //
  // Plist data size.
  //
  UINT32              PlistDataSize;
  //
  // Binary data.
  //
  UINT8               *BinaryData;
  //
  // Binary data size.
  //
  UINT32              BinaryDataSize;
  //
  // Binary file name.
  //
  CHAR16              *BinaryFileName;
} CACHELESS_KEXT;

//
// Built-in kexts in SLE.
//
typedef struct {
  //
  // Signature.
  //
  UINT32              Signature;
  //
  // Link for global list (CACHELESS_CONTEXT -> BuiltInKexts).
  //
  LIST_ENTRY          Link;
  //
  // Plist path.
  //
  CHAR16              *PlistPath;
  //
  // Bundle ID.
  //
  CHAR8               *BundleId;
  //
  // Binary file name.
  //
  CHAR16              *BinaryFileName;
  //
  // Dependencies.
  //
  LIST_ENTRY          Dependencies;
  //
  // OSBundleRequired is valid?
  //
  UINT8               OSBundleRequiredValue;
  //
  // Needs OSBundleRequired override for dependency injection?
  //
  BOOLEAN             PatchValidOSBundleRequired;
  //
  // Needs patches or blocks?
  //
  BOOLEAN             PatchKext;
} BUILTIN_KEXT;

//
// DEPEND_KEXT signature for list identification.
//
#define DEPEND_KEXT_SIGNATURE  SIGNATURE_32 ('S', 'l', 'e', 'D')

/**
  Gets the next element in list of DEPEND_KEXT.

  @param[in] This  The current ListEntry.
**/
#define GET_DEPEND_KEXT_FROM_LINK(This)  \
  (CR (                                  \
    (This),                              \
    DEPEND_KEXT,                         \
    Link,                                \
    DEPEND_KEXT_SIGNATURE                \
    ))

//
// CACHELESS_KEXT signature for list identification.
//
#define CACHELESS_KEXT_SIGNATURE  SIGNATURE_32 ('S', 'l', 'e', 'X')

/**
  Gets the next element in InjectedKexts list of CACHELESS_KEXT.

  @param[in] This  The current ListEntry.
**/
#define GET_CACHELESS_KEXT_FROM_LINK(This)  \
  (CR (                                     \
    (This),                                 \
    CACHELESS_KEXT,                         \
    Link,                                   \
    CACHELESS_KEXT_SIGNATURE                \
    ))

//
// BUILTIN_KEXT signature for list identification.
//
#define BUILTIN_KEXT_SIGNATURE  SIGNATURE_32 ('S', 'l', 'e', 'B')

/**
  Gets the next element in BuiltInKexts list of CACHELESS_KEXT.

  @param[in] This  The current ListEntry.
**/
#define GET_BUILTIN_KEXT_FROM_LINK(This)  \
  (CR (                                   \
    (This),                               \
    BUILTIN_KEXT,                         \
    Link,                                 \
    BUILTIN_KEXT_SIGNATURE                \
    ))

#endif
