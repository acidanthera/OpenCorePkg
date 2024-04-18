/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  Debug.h

Abstract:

Revision History:

--*/

#ifndef _EFILDR_DEBUG_H_
#define _EFILDR_DEBUG_H_

#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

VOID
PrintHeader (
  CHAR8  Char
  );

VOID
EFIAPI
PrintString (
  IN CONST CHAR8  *FormatString,
  ...
  );

VOID
ClearScreen (
  VOID
  );

#endif
