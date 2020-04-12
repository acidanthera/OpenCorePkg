/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "OcApfsInternal.h"
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcApfsLib.h>
#include <Protocol/BlockIo.h>

VOID
InternalApfsInitFusionData (
  IN  APFS_NX_SUPERBLOCK   *SuperBlock,
  OUT APFS_PRIVATE_DATA    *PrivateData
  )
{
  LIST_ENTRY         *Entry;
  APFS_PRIVATE_DATA  *Sibling;
  UINT32             BlockSize;

  //
  // All-zero Fusion UUID means this is a normal disk.
  //
  if (IsZeroGuid (&SuperBlock->FusionUuid)) {
    PrivateData->CanLoadDriver = TRUE;
    return;
  }

  CopyGuid (&PrivateData->FusionUuid, &SuperBlock->FusionUuid);
  PrivateData->IsFusion = TRUE;

  //
  // The highest bit is one for the Fusion set's main device and
  // zero for the second-tier device.
  //
  PrivateData->IsFusionMaster       = SuperBlock->FusionUuid.Data4[7] & BIT0;
  //
  // Drop master type from the stored value for easier comparison.
  //
  PrivateData->FusionUuid.Data4[7] &= ~BIT0;

  for (
    Entry = GetFirstNode (&mApfsPrivateDataList);
    !IsNull (&mApfsPrivateDataList, Entry);
    Entry = GetNextNode (&mApfsPrivateDataList, Entry)) {
    Sibling = CR (Entry, APFS_PRIVATE_DATA, Link, APFS_PRIVATE_DATA_SIGNATURE);

    //
    // Ignore the following potential siblings:
    // - Non-fusion.
    // - Ready to go fusion (aka FusionSibling != NULL).
    // - Same master/slave type.
    //
    if (!Sibling->IsFusion
      || Sibling->CanLoadDriver
      || Sibling->IsFusionMaster == PrivateData->IsFusionMaster
      || !CompareGuid (&Sibling->FusionUuid, &PrivateData->FusionUuid)) {
      continue;
    }

    //
    // We have a matching fusion sibling, mark this partition as ready to go.
    //
    PrivateData->FusionSibling = Sibling;
    PrivateData->CanLoadDriver = TRUE;
    //
    // Calculate FusionMask. This is essentially ctz, but we do not have it in EDK II.
    // We cannot use division either, since ApfsBlockSize is not guaranteed to be POT.
    //
    PrivateData->FusionMask    = APFS_FUSION_TIER2_DEVICE_BYTE_ADDR;
    BlockSize                  = PrivateData->ApfsBlockSize;
    while ((BlockSize & BIT0) == 0) {
      PrivateData->FusionMask >>= 1U;
      BlockSize               >>= 1U;
    }
    //
    // Update sibling fields as well.
    //
    PrivateData->FusionSibling->FusionSibling = PrivateData;
    PrivateData->FusionSibling->CanLoadDriver = TRUE;
    PrivateData->FusionSibling->FusionMask    = PrivateData->FusionMask;
    break;
  }
}

EFI_BLOCK_IO_PROTOCOL *
InternalApfsTranslateBlock (
  IN  APFS_PRIVATE_DATA    *PrivateData,
  IN  UINT64               Block,
  OUT EFI_LBA              *Lba
  )
{
  BOOLEAN  IsFusionMaster;

  ASSERT (PrivateData->CanLoadDriver);

  //
  // Note, LBA arithmetics may wrap around, but in this case we will
  // just read the wrong block, and the signature is checked anyway.
  //
  
  //
  // For normal disks we just return as is.
  //
  if (!PrivateData->IsFusion) {
    *Lba = Block * PrivateData->LbaMultiplier;
    return PrivateData->BlockIo;
  }

  ASSERT (PrivateData->FusionSibling != NULL);

  //
  // For Fusion disks it can be either volume.
  //
  if ((Block & PrivateData->FusionMask) == 0) {
    IsFusionMaster = TRUE;
  } else {
    Block         &= ~PrivateData->FusionMask;
    IsFusionMaster = FALSE;
  }

  *Lba = Block * PrivateData->LbaMultiplier;

  if (IsFusionMaster == PrivateData->IsFusionMaster) {
    return PrivateData->BlockIo;
  }

  return PrivateData->FusionSibling->BlockIo;
}
