/** @file
  Copyright (C) 2018 savvas. All rights reserved.<BR>
  Portions copyright (C) 2016 slice. All rights reserved.<BR>
  Portions copyright (C) 2018 vit9696. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_APPLE_IMAGE_CONVERSION_LIB_H
#define OC_APPLE_IMAGE_CONVERSION_LIB_H

#include <Protocol/UgaDraw.h>
#include <Protocol/AppleImageConversion.h>

/**
  Install and initialise the Apple Image Conversion protocol.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed or located protocol.
  @retval NULL  There was an error locating or installing the protocol.
**/
APPLE_IMAGE_CONVERSION_PROTOCOL *
OcAppleImageConversionInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_APPLE_IMAGE_CONVERSION_LIB_H
