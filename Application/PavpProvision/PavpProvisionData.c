/** @file
  Provision EPID data.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// This needs to be dumped from some firmware.
//

UINT8 gDefaultAppleEpidCertificate[] = {
  0x00
};

UINTN gDefaultAppleEpidCertificateSize = sizeof (gDefaultAppleEpidCertificate);

UINT8 gDefaultAppleGroupPublicKeys[] = {
  0x00
};

UINTN gDefaultAppleGroupPublicKeysSize = sizeof (gDefaultAppleGroupPublicKeys);
