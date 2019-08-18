/** @file
  Copyright (C) 2016 Sergey Slice. All rights reserved.<BR>
  Portions copyright (C) 2018 savvas.<BR>
  Portions copyright (C) 2006-2014 Intel Corporation. All rights reserved.<BR>
  Portions copyright (C) 2016-2018 Alex James. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_FIRMWARE_VOLUME_LIB_H
#define OC_FIRMWARE_VOLUME_LIB_H

#include <Pi/PiFirmwareFile.h>
#include <Pi/PiFirmwareVolume.h>
#include <Protocol/FirmwareVolume.h>

/**
  Install and initialise EFI Firmware Volume protocol.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed protocol.
  @retval NULL  There was an error installing the protocol.
**/
EFI_FIRMWARE_VOLUME_PROTOCOL *
OcFirmwareVolumeInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_FIRMWARE_VOLUME_LIB_H
