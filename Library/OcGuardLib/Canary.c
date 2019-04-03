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

//
// TODO: For the cookie to work for security needs, the value is to be runtime
// generated, e.g. with rdrand. For now this code is only written to help debugging
// stack corruptions.
//
UINT64 __security_cookie = 0x9C7D6B4580C0BC9ULL;

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
