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

#include <Uefi.h>

#include <IndustryStandard/Pci.h>

#include <Guid/ApplePlatformInfo.h>

#include <Protocol/LegacyRegion.h>
#include <Protocol/LegacyRegion2.h>
#include <Protocol/PciIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/** Unlock the legacy region specified to enable modification.

  @param[in] LegacyAddress  The address of the region to unlock.
  @param[in] LegacyLength   The size of the region to unlock.

  @retval EFI_SUCCESS  The region was unlocked successfully.
**/
EFI_STATUS
LegacyRegionUnlock (
  IN UINT32  LegacyAddress,
  IN UINT32  LegacyLength
  )
{
  EFI_STATUS                    Status;

  EFI_LEGACY_REGION_PROTOCOL    *LegacyRegionProtocol;
  EFI_LEGACY_REGION2_PROTOCOL   *LegacyRegion2Protocol;
  UINT32                        Granularity;

  //
  // Locate v2 protocol and fallback to v1 if unavailable.
  //
  LegacyRegionProtocol  = NULL;
  LegacyRegion2Protocol = NULL;
  Status                = gBS->LocateProtocol (
                            &gEfiLegacyRegion2ProtocolGuid,
                            NULL,
                            (VOID **) &LegacyRegion2Protocol
                            );

  if (Status == EFI_NOT_FOUND) {
    Status = gBS->LocateProtocol (
      &gEfiLegacyRegionProtocolGuid,
      NULL,
      (VOID **) &LegacyRegionProtocol
      );
  }

  if (!EFI_ERROR (Status)) {
    //
    // Unlock region using LegacyRegionProtocol or LegacyRegion2Protocol.
    //
    Granularity = 0;
    if (LegacyRegionProtocol != NULL) {
      Status = LegacyRegionProtocol->UnLock (
                                        LegacyRegionProtocol,
                                        LegacyAddress,
                                        LegacyLength,
                                        &Granularity
                                        );
    } else {
      Status = LegacyRegion2Protocol->UnLock (
                                        LegacyRegion2Protocol,
                                        LegacyAddress,
                                        LegacyLength,
                                        &Granularity
                                        );
    }

    DEBUG ((
      DEBUG_INFO,
      "Unlock LegacyRegion %0X-%0X - %r\n",
      LegacyAddress,
      (LegacyAddress + (LegacyLength - 1)),
      Status
      ));
  }

  return Status;
}

