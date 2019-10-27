/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef IMG4_OIDS_H
#define IMG4_OIDS_H

#include "libDER/libDER.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const DERItem
  oidAppleImg4ManifestCertSpec,
  oidSha384Rsa,
  oidSha512Rsa;

#ifdef __cplusplus
}
#endif

#endif // IMG4_OIDS_H
