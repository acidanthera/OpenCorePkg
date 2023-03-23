/** @file
  Use PAT to enable write-combining caching (burst mode) on GOP memory,
  when it is suppported but firmware has not set it up.

  Copyright (C) 2023, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "OcConsoleLibInternal.h"

#include <Base.h>
#include <Uefi/UefiBaseType.h>

#include <IndustryStandard/ProcessorInfo.h>
#include <IndustryStandard/VirtualMemory.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MtrrLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define PAT_INDEX_TO_CHANGE  7

STATIC
EFI_STATUS
WriteCombineGop (
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop,
  BOOLEAN                       SetPatWC
  )
{
  EFI_STATUS                      Status;
  MTRR_MEMORY_CACHE_TYPE          MtrrCacheType;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageTable;
  EFI_VIRTUAL_ADDRESS             VirtualAddress;
  EFI_PHYSICAL_ADDRESS            PhysicalAddress;
  UINT64                          Bits;
  UINT8                           Level;
  BOOLEAN                         HasMtrr;
  BOOLEAN                         HasPat;
  UINT64                          PatMsr;
  UINT64                          ModifiedPatMsr;
  PAT_INDEX                       PatIndex;
  BOOLEAN                         HasDefaultPat;
  BOOLEAN                         AlreadySet;

  if (!SetPatWC) {
    return EFI_SUCCESS;
  }

  HasMtrr = IsMtrrSupported ();
  HasPat  = OcIsPatSupported ();

  DEBUG ((
    DEBUG_INFO,
    "OCC: MTRR %asupported, PAT %asupported\n",
    HasMtrr ? "" : "not ",
    HasPat ? "" : "not "
    ));

  if (!HasMtrr || !HasPat) {
    return EFI_UNSUPPORTED;
  }

  AlreadySet = FALSE;
  PageTable  = OcGetCurrentPageTable (NULL);

  do {
    PatMsr        = AsmReadMsr64 (MSR_IA32_CR_PAT);
    HasDefaultPat = (PatMsr == PAT_DEFAULTS);

    DEBUG ((
      DEBUG_INFO,
      "OCC: PAT 0x%016LX (%a)\n",
      PatMsr,
      HasDefaultPat ? "default" : "not default"
      ));

    MtrrCacheType = MtrrGetMemoryAttribute (Gop->Mode->FrameBufferBase);

    DEBUG_CODE_BEGIN ();

    VirtualAddress = Gop->Mode->FrameBufferBase;

    Status = OcGetSetPageTableInfoForAddress (
               PageTable,
               VirtualAddress,
               &PhysicalAddress,
               &Level,
               &Bits,
               &PatIndex,
               FALSE
               );

    DEBUG ((
      DEBUG_INFO,
      "OCC: 0x%LX->0x%LX MTRR %u=%a PTE%u bits 0x%016LX PAT %u->%u=%a - %r\n",
      VirtualAddress,
      PhysicalAddress,
      MtrrCacheType,
      OcGetMtrrTypeString (MtrrCacheType),
      Level,
      Bits,
      PatIndex.Index,
      GET_PAT_N (PatMsr, PatIndex.Index),
      OcGetPatTypeString (GET_PAT_N (PatMsr, PatIndex.Index)),
      Status
      ));

    ASSERT_EFI_ERROR (Status);
    ASSERT (VirtualAddress == PhysicalAddress);

    if (AlreadySet) {
      break;
    }

    DEBUG_CODE_END ();

    //
    // If MTRR is already set to WC, no need to set it via PAT.
    // (Ignore implausible scenario where WC is set via MTRR but overridden via PAT.)
    //
    if (MtrrCacheType == CacheWriteCombining) {
      return EFI_ALREADY_STARTED;
    }

    //
    // We cannot split MTRR ranges (returns out of resources, meaning there are not
    // enough MTRRs to keep the existing mapping and insert a new one), so set up
    // PAT7 as write combining (WC) and modify the page tables instead.
    //
    if (HasDefaultPat) {
      //
      // Modify PAT MSR to add WC entry somewhere reasonable (not first four entries,
      // and should use an entry likely to be unused) in PAT MSR.
      // TODO: Could fully check existing page tables to confirm PAT7 is unused.
      //
      ModifiedPatMsr = MODIFY_PAT_MSR (PatMsr, PAT_INDEX_TO_CHANGE, PatWriteCombining);
      DEBUG ((DEBUG_INFO, "OCC: Writing PAT 0x%016LX\n", ModifiedPatMsr));
      AsmWriteMsr64 (MSR_IA32_CR_PAT, ModifiedPatMsr);

      PatIndex.Index = PAT_INDEX_TO_CHANGE;
    } else {
      //
      // Do not modify non-default PAT MSR, but use WC if it already exists there
      // (including previously set by us, if GOP mode changes more than once).
      //
      for (PatIndex.Index = 0; PatIndex.Index < PAT_INDEX_MAX; PatIndex.Index++) {
        if (GET_PAT_N (PatMsr, PatIndex.Index) == PatWriteCombining) {
          break;
        }
      }

      if (PatIndex.Index == PAT_INDEX_MAX) {
        Status = EFI_UNSUPPORTED;
        DEBUG ((DEBUG_INFO, "OCC: Non-default PAT MSR with no WC entry - %r\n", Status));
        return Status;
      }
    }

    Status = OcSetPatIndexForAddressRange (PageTable, Gop->Mode->FrameBufferBase, Gop->Mode->FrameBufferSize, &PatIndex);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCC: Failed to set PAT index for range - %r\n", Status));
      return Status;
    }

    DEBUG_CODE_BEGIN ();
    AlreadySet = TRUE;
    DEBUG_CODE_END ();
  } while (AlreadySet);

  return EFI_SUCCESS;
}

EFI_STATUS
OcSetGopBurstMode (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: No console GOP found for burst mode - %r\n", Status));
    return Status;
  }

  Status = WriteCombineGop (Gop, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCC: Failed to set burst mode - %r\n", Status));
  }

  return Status;
}
