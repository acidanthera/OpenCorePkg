/** @file

OcGuardLib

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/OcRngLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>

//
// TODO: For the cookie to work for security needs, the value is to be runtime
// generated, e.g. with rdrand. For now this code is only written to help debugging
// stack corruptions.
//
VOID
__security_check_cookie (
  IN UINTN  Value
  )
{
  volatile UINTN  Index;

  if (Value != (UINTN) __security_cookie) {
    Index = 0;
    while (Index == 0)
    {
    }
  }
}

VOID
__stack_chk_fail (
  VOID
  )
{
  volatile UINTN  Index;

  DEBUG ((DEBUG_ERROR, "Error: Stack overflow detected!\n"));

  Index = 0;
  while (Index == 0)
  {
  }
}

VOID
InitializeSecurityCookie (
  VOID
  )
{
    UINT64 RandomValue = 0;
    OcRngLibConstructor();
    GetRandomNumber64(&RandomValue);
    __security_cookie = RandomValue;
}
