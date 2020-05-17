/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_MAC_EFI_H
#define APPLE_MAC_EFI_H

#include <AppleCpuType.h>

#include <Uefi.h>
#include <AppleMacEfi/AppleMacEfiSpec.h>

#ifndef ARRAY_SIZE

  /**
    Return the number of elements in an array.

    @param  Array  An object of array type. Array is only used as an argument to
                   the sizeof operator, therefore Array is never evaluated. The
                   caller is responsible for ensuring that Array's type is not
                   incomplete; that is, Array must have known constant size.

    @return The number of elements in Array. The result has type UINTN.

  **/
  #define ARRAY_SIZE(Array) (sizeof (Array) / sizeof ((Array)[0]))

#endif

#endif // APPLE_MAC_EFI_H
