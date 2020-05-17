/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_DEVICE_PROPERTY_LIB_H
#define OC_DEVICE_PROPERTY_LIB_H

#include <Protocol/DevicePathPropertyDatabase.h>

/**
  Install and initialise EFI DevicePath property protocol.

  @param[in] Reinstall  Overwrite installed protocol.

  @retval installed or located protocol or NULL.
**/
EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL *
OcDevicePathPropertyInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_DEVICE_PROPERTY_LIB_H
