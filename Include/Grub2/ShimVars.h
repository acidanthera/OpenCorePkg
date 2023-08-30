/** @file
  GRUB2 shim values.

  Copyright (c) 2023, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef __SHIM_H
#define __SHIM_H

#include <Guid/ShimLock.h>

//
// Variable to set to retain shim lock protocol for subsequent image loads.
//
#define SHIM_RETAIN_PROTOCOL  L"ShimRetainProtocol"

#endif // __SHIM_H
