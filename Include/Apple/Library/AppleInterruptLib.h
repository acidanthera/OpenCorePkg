/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_INTERRUPT_H
#define APPLE_INTERRUPT_H

#define APPLE_INTERRUPT_ASSERT  1

// AppleInterrupt
RETURN_STATUS
EFIAPI
AppleInterrupt (
  IN UINT32  FunctiondId,
  ...
  );

#endif // APPLE_INTERRUPT_H
