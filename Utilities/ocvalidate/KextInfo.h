/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_USER_UTILITIES_OCVALIDATE_VALIDATE_KERNEL_H
#define OC_USER_UTILITIES_OCVALIDATE_VALIDATE_KERNEL_H

#define INDEX_KEXT_LILU  0U
#define INDEX_KEXT_VSMC  1U

/**
  Child kext must be put after Parent kext in OpenCore config->Kernel->Add.
  This means that the index of Child must succeed that of Parent.
**/
typedef struct KEXT_PRECEDENCE_ {
  CONST CHAR8  *Child;
  CONST CHAR8  *Parent;
} KEXT_PRECEDENCE;

/**
  Known information of kexts. Mainly those from Acidanthera.
**/
typedef struct KEXT_INFO_ {
  CONST CHAR8  *KextBundlePath;
  CONST CHAR8  *KextExecutablePath;
  CONST CHAR8  *KextPlistPath;
} KEXT_INFO;

extern KEXT_PRECEDENCE  mKextPrecedence[];
extern UINTN            mKextPrecedenceSize;

extern KEXT_INFO        mKextInfo[];
extern UINTN            mKextInfoSize;

/**
  ASSERT() on incorrect placed kext info, where a set of rules must be followed.
**/
VOID
ValidateKextInfo (
  VOID
  );

#endif // OC_USER_UTILITIES_OCVALIDATE_VALIDATE_KERNEL_H
