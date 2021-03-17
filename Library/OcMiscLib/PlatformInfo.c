/** @file
  Copyright (C) 2021, PMheart. All rights reserved.
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/OcMiscLib.h>

#include <Guid/AppleHob.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

EFI_STATUS
OcReadApplePlatformFirstData (
  IN      APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *PlatformInfo,
  IN      EFI_GUID                               *DataGuid,
  IN OUT  UINT32                                 *Size,
     OUT  VOID                                   *Data
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  ASSERT (Size     != NULL);
  ASSERT (Data     != NULL);
  ASSERT (DataGuid != NULL);

  Status = PlatformInfo->GetFirstDataSize (
    PlatformInfo,
    DataGuid,
    &DataSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCCPU: No first platform data size for %g up to %u - %r\n",
      DataGuid,
      *Size,
      Status
      ));
    return Status;
  }
  if (DataSize > *Size) {
    DEBUG ((
      DEBUG_INFO,
      "OCCPU: Invalid first platform data size %u for %g up to %u - %r\n",
      DataSize,
      DataGuid,
      *Size,
      Status
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = PlatformInfo->GetFirstData (
    PlatformInfo,
    DataGuid,
    Data,
    &DataSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCCPU: No first platform data for %g up to %u - %r\n",
      DataGuid,
      *Size,
      Status
      ));
    return Status;
  }

  *Size = DataSize;

  return Status;
}

EFI_STATUS
OcReadApplePlatformFirstDataAlloc (
  IN   APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *PlatformInfo,
  IN   EFI_GUID                               *DataGuid,
  OUT  UINT32                                 *Size,
  OUT  VOID                                   **Data
  )
{
  EFI_STATUS  Status;
  UINT32      DataSize;

  ASSERT (Size     != NULL);
  ASSERT (Data     != NULL);
  ASSERT (DataGuid != NULL);

  Status = PlatformInfo->GetFirstDataSize (
    PlatformInfo,
    DataGuid,
    &DataSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCCPU: No first platform data size for %g - %r\n",
      DataGuid,
      Status
      ));
    return Status;
  }

  *Data = AllocatePool (DataSize);
  if (*Data == NULL) {
    DEBUG ((
      DEBUG_INFO,
      "OCCPU: Cannot alloc %u for first platform data %g\n",
      DataSize,
      DataGuid
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PlatformInfo->GetFirstData (
    PlatformInfo,
    DataGuid,
    *Data,
    &DataSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCCPU: No first platform data for %g with %u - %r\n",
      DataGuid,
      DataSize,
      Status
      ));
    FreePool (*Data);
    return Status;
  }

  *Size = DataSize;

  return Status;
}

EFI_STATUS
OcReadApplePlatformData (
  IN      APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *PlatformInfo,
  IN      EFI_GUID                               *DataGuid,
  IN      EFI_GUID                               *HobGuid,
  IN OUT  UINT32                                 *Size,
     OUT  VOID                                   *Data
  )
{
  EFI_STATUS  Status;
  VOID        *FsbHob;
  UINT32      DataSize;

  ASSERT (Size     != NULL);
  ASSERT (Data     != NULL);
  ASSERT (DataGuid != NULL);
  ASSERT (HobGuid  != NULL);

  FsbHob = GetFirstGuidHob (HobGuid);
  if (FsbHob == NULL) {
    return EFI_UNSUPPORTED;
  }

  Status = PlatformInfo->GetDataSize (
    PlatformInfo,
    DataGuid,
    *(UINT8 *) GET_GUID_HOB_DATA (FsbHob),
    &DataSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCCPU: No platform data size for %g up to %u - %r\n",
      DataGuid,
      *Size,
      Status
      ));
    return Status;
  }
  if (DataSize > *Size) {
    DEBUG ((
      DEBUG_INFO,
      "OCCPU: Invalid platform data size %u for %g up to %u - %r\n",
      DataSize,
      DataGuid,
      *Size,
      Status
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = PlatformInfo->GetData (
    PlatformInfo,
    DataGuid,
    *(UINT8 *) GET_GUID_HOB_DATA (FsbHob),
    Data,
    &DataSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "OCCPU: No platform data for %g up to %u - %r\n",
      DataGuid,
      *Size,
      Status
      ));
    return Status;
  }

  *Size = DataSize;

  return Status;
}
