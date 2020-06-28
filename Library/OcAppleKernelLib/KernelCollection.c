/** @file
  Kernel collection support.

  Copyright (c) 2020, vit9696. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <IndustryStandard/AppleCompressedBinaryImage.h>
#include <IndustryStandard/AppleFatBinaryImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcCompressionLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>

#include "PrelinkedInternal.h"

STATIC
UINTN
InternalKcGetKextFilesetSize (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  PRELINKED_KEXT  *PrelinkedKext;
  LIST_ENTRY      *Kext;
  UINTN           Size;
  UINTN           CommandSize;

  Size = 0;

  Kext = GetFirstNode (&Context->InjectedKexts);
  while (!IsNull (&Context->InjectedKexts, Kext)) {
    PrelinkedKext = GET_INJECTED_KEXT_FROM_LINK (Kext);

    //
    // Each command must be NUL-terminated and 8-byte aligned.
    //
    CommandSize = sizeof (MACH_FILESET_ENTRY_COMMAND) + AsciiStrSize (PrelinkedKext->Identifier);
    Size       += ALIGN_VALUE (CommandSize, 8);

    Kext = GetNextNode (&Context->InjectedKexts, Kext);
  }

  return Size;
}

STATIC
VOID
InternalKcWriteCommandHeaders (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN OUT MACH_HEADER_64     *MachHeader
  )
{
  PRELINKED_KEXT           *PrelinkedKext;
  PRELINKED_KEXT           *LowestKext;
  LIST_ENTRY               *Kext;
  MACH_LOAD_COMMAND_PTR    Command;
  MACH_SEGMENT_COMMAND_64  *Segment;
  UINTN                    StringSize;

  Command.Address = (UINTN) MachHeader->Commands + MachHeader->CommandsSize;

  Kext       = GetFirstNode (&Context->InjectedKexts);
  LowestKext = GET_INJECTED_KEXT_FROM_LINK (Kext);

  while (!IsNull (&Context->InjectedKexts, Kext)) {
    PrelinkedKext = GET_INJECTED_KEXT_FROM_LINK (Kext);

    StringSize = AsciiStrSize (PrelinkedKext->Identifier);

    //
    // Write 8-byte aligned fileset command.
    //
    Command.FilesetEntry->CommandType = MACH_LOAD_COMMAND_FILESET_ENTRY;
    Command.FilesetEntry->CommandSize = (UINT32) ALIGN_VALUE (sizeof (MACH_FILESET_ENTRY_COMMAND) + StringSize, 8);
    Command.FilesetEntry->VirtualAddress = PrelinkedKext->Context.VirtualBase;
    Segment = MachoGetNextSegment64 (&PrelinkedKext->Context.MachContext, NULL);
    ASSERT (Segment != NULL);
    Command.FilesetEntry->FileOffset     = Segment->FileOffset;
    Command.FilesetEntry->EntryId.Offset = OFFSET_OF (MACH_FILESET_ENTRY_COMMAND, Payload);
    Command.FilesetEntry->Reserved       = 0;
    CopyMem (Command.FilesetEntry->Payload, PrelinkedKext->Identifier, StringSize);
    ZeroMem (
      &Command.FilesetEntry->Payload[StringSize],
      Command.FilesetEntry->CommandSize - Command.FilesetEntry->EntryId.Offset - StringSize
      );
    Command.Address += Command.FilesetEntry->CommandSize;

    //
    // Refresh Mach-O header constants to include the new command.
    //
    MachHeader->NumCommands++;
    MachHeader->CommandsSize += Command.FilesetEntry->CommandSize;

    Kext = GetNextNode (&Context->InjectedKexts, Kext);
  }

  //
  // Get last kext.
  //
  Kext = GetPreviousNode (&Context->InjectedKexts, &Context->InjectedKexts);
  PrelinkedKext = GET_INJECTED_KEXT_FROM_LINK (Kext);

  //
  // Write a segment covering all the kexts.
  //
  Command.Segment64->CommandType       = MACH_LOAD_COMMAND_SEGMENT_64;
  Command.Segment64->CommandSize       = sizeof (MACH_SEGMENT_COMMAND_64);
  CopyMem (Command.Segment64->SegmentName, KC_MOSCOW_SEGMENT, sizeof (KC_MOSCOW_SEGMENT));
  ZeroMem (
    &Command.Segment64->SegmentName[sizeof (KC_MOSCOW_SEGMENT)],
    sizeof (Command.Segment64->SegmentName) - sizeof (KC_MOSCOW_SEGMENT)
    );
  Segment = MachoGetNextSegment64 (&LowestKext->Context.MachContext, NULL);
  Command.Segment64->VirtualAddress    = Segment->VirtualAddress;
  Command.Segment64->FileOffset        = Segment->FileOffset;
  Segment = MachoGetNextSegment64 (&PrelinkedKext->Context.MachContext, NULL);
  Command.Segment64->Size              = Segment->VirtualAddress - Command.Segment64->VirtualAddress
    + MachoGetVmSize64 (&PrelinkedKext->Context.MachContext);
  Command.Segment64->FileSize          = Segment->FileOffset - Command.Segment64->FileOffset
    + MachoGetFileSize (&PrelinkedKext->Context.MachContext);
  Command.Segment64->MaximumProtection = MACH_SEGMENT_VM_PROT_READ
    | MACH_SEGMENT_VM_PROT_WRITE | MACH_SEGMENT_VM_PROT_EXECUTE;
  Command.Segment64->InitialProtection = MACH_SEGMENT_VM_PROT_READ
    | MACH_SEGMENT_VM_PROT_WRITE | MACH_SEGMENT_VM_PROT_EXECUTE;
  Command.Segment64->NumSections       = 0;
  Command.Segment64->Flags             = 0;

  //
  // Refresh Mach-O header constants to include the new segment command.
  //
  MachHeader->NumCommands++;
  MachHeader->CommandsSize += sizeof (MACH_SEGMENT_COMMAND_64);
}

EFI_STATUS
InternalKcRebuildMachHeader (
  IN OUT PRELINKED_CONTEXT  *Context
  )
{
  MACH_HEADER_64           *MachHeader;
  MACH_SEGMENT_COMMAND_64  *TextSegment;
  UINTN                    CurrentSize;
  UINTN                    FilesetSize;
  UINTN                    RequiredSize;

  MachHeader = MachoGetMachHeader64 (
    &Context->PrelinkedMachContext
    );

  CurrentSize  = MachHeader->CommandsSize + sizeof (*MachHeader);
  FilesetSize  = InternalKcGetKextFilesetSize (Context);
  RequiredSize = FilesetSize + sizeof (MACH_LOAD_COMMAND_SEGMENT_64);

  TextSegment = MachoGetSegmentByName64 (
    &Context->PrelinkedMachContext,
    KC_TEXT_SEGMENT
    );

  DEBUG ((
    DEBUG_INFO,
    "OCAK: KC TEXT is %u bytes with %u Mach-O headers need %u\n",
    (UINT32) (TextSegment != NULL ? TextSegment->FileSize : 0),
    (UINT32) CurrentSize,
    (UINT32) RequiredSize
    ));

  if (TextSegment == NULL
    || TextSegment->FileOffset != 0
    || TextSegment->FileSize != TextSegment->Size
    || TextSegment->FileSize < CurrentSize) {
    return EFI_INVALID_PARAMETER;
  }

  if (FilesetSize == 0) {
    return EFI_SUCCESS; ///< Just in case.
  }

  if (CurrentSize + RequiredSize > TextSegment->FileSize) {
    //
    // We do not have enough memory in the header, free some memory by merging
    // kext segments (__REGION###) into a single RWX segment.
    // - This is not bad for security as it is actually how it is done before
    //   11.0, where OSKext::setVMAttributes sets the proper memory permissions
    //   on every kext soon after the kernel loads.
    // - This is not bad for compatibility as the only thing referencing these
    //   segments is dyld fixups command, and it does not matter whether it
    //   references the base segment or points to the middle of it.
    // The actual reason this was added might actually be because of the ARM
    // transition, where you can restrict the memory permissions till the next
    // hardware reset (like on iOS) and they now really try to require W^X:
    // https://developer.apple.com/videos/play/wwdc2020/10686/
    //
    if (!MachoMergeSegments64 (&Context->PrelinkedMachContext, KC_REGION_SEGMENT_PREFIX)) {
      DEBUG ((DEBUG_INFO, "OCAK: Segment expansion failure\n"));
      return EFI_UNSUPPORTED;
    }

    CurrentSize = MachHeader->CommandsSize + sizeof (*MachHeader);
  }

  //
  // If we do not fit even after the expansion, we are really doomed.
  // This should never happen.
  //
  if (CurrentSize + RequiredSize > TextSegment->FileSize) {
    DEBUG ((DEBUG_INFO, "OCAK: Used header %u is still too large\n", (UINT32) CurrentSize));
    return EFI_UNSUPPORTED;
  }

  //
  // At this step we have memory for all the new commands.
  // Just write the here.
  //
  InternalKcWriteCommandHeaders (Context, MachHeader);

  return EFI_SUCCESS;
}

/*
  Returns the size required to store a segment's fixup chains information.

  @param[in] SegmentSize  The size, in bytes, of the segment to index.

  @retval 0      The segment is too large to index with a single structure.
  @retval other  The size, in bytes, required to store a segment's fixup chain
                 information.
*/
UINT32
InternalKcGetSegmentFixupChainsSize (
  IN UINT32  SegmentSize
  )
{
  CONST MACH_DYLD_CHAINED_STARTS_IN_SEGMENT *Dummy;
  //
  // Theis assertion is propagated from OcMachoLib.
  //
  ASSERT (SegmentSize % MACHO_PAGE_SIZE == 0);
  //
  // As PageCount is UINT16, we can only index 2^16 * 4096 Bytes with one chain.
  //
  if (SegmentSize > BIT16 * MACHO_PAGE_SIZE) {
    return 0;
  }

  return (UINT32) (sizeof (*Dummy)
    + (SegmentSize / MACHO_PAGE_SIZE) * sizeof (Dummy->PageStart[0]));
}

/*
  Initialises a structure that stores Segment's fixup chains information.

  @param[out] SegChain      The information structure to initialise.
  @param[in]  SegChainSize  The size, in bytes, available to SegChain.
  @param[in]  Segment       The segment to index the fixup chains of.
*/
VOID
InternalKcInitSegmentFixupChains (
  OUT MACH_DYLD_CHAINED_STARTS_IN_SEGMENT  *SegChain,
  IN  UINT32                               SegChainSize, 
  IN  CONST MACH_SEGMENT_COMMAND_64        *Segment
  )
{
  UINT16 PageIndex;

  ASSERT (SegChain != NULL);
  ASSERT (Segment != NULL);
  //
  // These assertions are propagated from OcMachoLib.
  //
  ASSERT (Segment->Size <= MAX_UINT32);
  ASSERT (Segment->Size % MACHO_PAGE_SIZE == 0);

  ASSERT (SegChainSize != 0);
  ASSERT (
    SegChainSize == InternalKcGetSegmentFixupChainsSize ((UINT32) Segment->Size)
    );

  SegChain->Size            = SegChainSize;
  SegChain->PageSize        = MACHO_PAGE_SIZE;
  SegChain->PointerFormat   = MACH_DYLD_CHAINED_PTR_X86_64_KERNEL_CACHE;
  SegChain->SegmentOffset   = Segment->VirtualAddress;
  SegChain->MaxValidPointer = 0;
  SegChain->PageCount       = (UINT16) (Segment->Size / MACHO_PAGE_SIZE);
  //
  // Initialise all pages with no associated fixups.
  //
  for (PageIndex = 0; PageIndex < SegChain->PageCount; ++PageIndex) {
    SegChain->PageStart[PageIndex] = MACH_DYLD_CHAINED_PTR_START_NONE;
  }
}

/*
  Indexes RelocInfo into the fixup chains SegChain of Segment.

  @param[in,out] SegChain     The segment fixup chains information structure to
                              add RelocInfo into.
  @param[in]     Segment      The segment SegChain is describing.
  @param[in]     MachContext  The context of the Mach-O RelocInfo belongs to. It
                              must have been prelinked by OcAppleKernelLib. The
                              image must reside in Segment.
  @param[in]     RelocInfo    The relocation to add a fixup of.
  @param[in]     RelocBase    The relocation base address.
*/
VOID
InternalKcConvertRelocToFixup (
  IN OUT MACH_DYLD_CHAINED_STARTS_IN_SEGMENT  *SegChain,
  IN     CONST MACH_SEGMENT_COMMAND_64        *Segment,
  IN     OC_MACHO_CONTEXT                     *MachContext,
  IN     CONST MACH_RELOCATION_INFO           *RelocInfo,
  IN     UINT64                               RelocBase
  )
{
  UINT8                                        *SegmentData;
  UINT8                                        *SegmentPageData;

  UINT64                                       RelocAddress;
  UINT32                                       RelocOffsetInSeg;
  VOID                                         *RelocDest;

  UINT16                                       NewFixupPage;
  UINT16                                       NewFixupPageOffset;
  MACH_DYKD_CHAINED_PTR_64_KERNEL_CACHE_REBASE NewFixup;

  UINT16                                       IterFixupPageOffset;
  VOID                                         *IterFixupData;
  MACH_DYKD_CHAINED_PTR_64_KERNEL_CACHE_REBASE IterFixup;
  UINT16                                       NextIterFixupPageOffset;

  UINT16                                       FixupDelta;

  ASSERT (SegChain != NULL);
  ASSERT (MachContext != NULL);
  ASSERT (Segment != NULL);
  ASSERT (RelocInfo != NULL);
  //
  // SegChain must belong to Segment.
  //
  ASSERT (SegChain->SegmentOffset == Segment->VirtualAddress);
  //
  // This assertion is propagated from OcMachoLib.
  //
  ASSERT (SegChain->PageSize == MACHO_PAGE_SIZE);
  //
  // The entire KEXT and thus its relocations must be in Segment.
  // Mach-O images are limited to 4 GB size by OcMachoLib, so the cast is safe.
  //
  RelocAddress     = RelocBase + (UINT32) RelocInfo->Address;
  RelocOffsetInSeg = (UINT32) (RelocAddress - Segment->VirtualAddress);
  //
  // For now we assume we prelinked already and the relocations are sane.
  //
  ASSERT (RelocInfo->Extern == 0);
  ASSERT (RelocInfo->Type == MachX8664RelocUnsigned);
  ASSERT (RelocAddress >= Segment->VirtualAddress
    && RelocOffsetInSeg <= Segment->FileSize
    && Segment->FileSize - RelocOffsetInSeg <= 8);
  //
  // Create a new fixup based on the data of RelocInfo.
  //
  SegmentData = (UINT8 *) MachContext->MachHeader + Segment->FileOffset;
  RelocDest   = SegmentData + RelocOffsetInSeg;
  //
  // It has been observed all fields but target and next are 0 for the kernel
  // KC. For isAuth, this is because x86 does not support Pointer
  // Authentication.
  //
  ZeroMem (&NewFixup, sizeof (NewFixup));
  NewFixup.Target = ReadUnaligned64 (RelocDest);

  NewFixupPage       = (UINT16) (RelocOffsetInSeg / MACHO_PAGE_SIZE);
  NewFixupPageOffset = (UINT16) (RelocOffsetInSeg % MACHO_PAGE_SIZE);

  IterFixupPageOffset = SegChain->PageStart[NewFixupPage];

  if (IterFixupPageOffset == MACH_DYLD_CHAINED_PTR_START_NONE) {
    //
    // The current page has no fixups, just assign and terminate this one.
    //
    SegChain->PageStart[NewFixupPage] = NewFixupPageOffset;
    //
    // Fixup.next is 0 (i.e. the chain is terminated) already.
    //
  } else if (NewFixupPageOffset < IterFixupPageOffset) {
    //
    // RelocInfo preceeds the first fixup of the page - prepend the new fixup.
    //
    NewFixup.Next = IterFixupPageOffset - NewFixupPageOffset;
    SegChain->PageStart[NewFixupPage] = NewFixupPageOffset;
  } else {
    SegmentPageData = SegmentData + NewFixupPage * MACHO_PAGE_SIZE;
    //
    // Find the last fixup of this page that preceeds RelocInfo.
    // FIXME: Consider sorting the relocations in descending order first to
    //        then always prepend the new fixup.
    //
    NextIterFixupPageOffset = IterFixupPageOffset;
    do {
      IterFixupPageOffset = NextIterFixupPageOffset;
      IterFixupData       = SegmentPageData + IterFixupPageOffset;

      CopyMem (&IterFixup, IterFixupData, sizeof (IterFixup));
      NextIterFixupPageOffset = (UINT16) (IterFixupPageOffset + IterFixup.Next);
    } while (NextIterFixupPageOffset < NewFixupPageOffset && IterFixup.Next != 0);

    FixupDelta = NewFixupPageOffset - IterFixupPageOffset;
    //
    // Our new fixup needs to point to the first fixup following it or terminate
    // if the previous one was the last.
    //
    if (IterFixup.Next != 0) {
      ASSERT (IterFixup.Next > FixupDelta);
      NewFixup.Next = IterFixup.Next - FixupDelta;
    } else {
      NewFixup.Next = 0;
    }
    //
    // The last fixup preceeding RelocInfo must point to our new fixup.
    //
    IterFixup.Next = FixupDelta;
    CopyMem (IterFixupData, &IterFixup, sizeof (IterFixup));
  }

  CopyMem (RelocDest, &NewFixup, sizeof (NewFixup));
}

/*
  Indexes all relocations of MachContext into the fixup chains SegChain of
  Segment.

  @param[in,out] SegChain     The segment fixup chains information structure to
                              add RelocInfo into.
  @param[in]     Segment      The segment SegChain is describing.
  @param[in]     MachContext  The context of the Mach-O to index. It must have
                              been prelinked by OcAppleKernelLib. The image
                              must reside in Segment.
*/
VOID
InternalKcKextIndexFixups (
  IN OUT MACH_DYLD_CHAINED_STARTS_IN_SEGMENT  *SegChain,
  IN     CONST MACH_SEGMENT_COMMAND_64        *Segment,
  IN     OC_MACHO_CONTEXT                     *MachContext
  )
{
  CONST MACH_DYSYMTAB_COMMAND   *DySymtab;
  CONST MACH_SEGMENT_COMMAND_64 *FirstSegment;
  CONST MACH_HEADER_64          *MachHeader;
  CONST MACH_RELOCATION_INFO    *Relocations;
  UINT32                        RelocIndex;

  ASSERT (SegChain != NULL);
  ASSERT (Segment != NULL);
  ASSERT (MachContext != NULL);

  MachHeader = MachoGetMachHeader64 (MachContext);
  //
  // Only perform actions when the kext is flag'd to be dynamically linked.
  // FIXME: Somehow unify this with the prelink function?
  //
  if ((MachHeader->Flags & MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK) == 0) {
    return;
  }
  //
  // FIXME: The current OcMachoLib was not written with post-linking
  // re-initialisation in mind. We really don't want to sanitise everything
  // again, so avoid the dedicated API for now.
  //
  DySymtab = (MACH_DYSYMTAB_COMMAND *) MachoGetNextCommand64 (
    MachContext,
    MACH_LOAD_COMMAND_DYSYMTAB,
    NULL
    );

  FirstSegment = MachoGetNextSegment64 (MachContext, NULL);
  //
  // DYSYMTAB and at least one segment must exist, otherwise prelinking would
  // have failed.
  //
  ASSERT (DySymtab != NULL);
  ASSERT (FirstSegment != NULL);
  //
  // The Mach-O file to index must be included in Segment.
  //
  ASSERT (FirstSegment->VirtualAddress >= Segment->VirtualAddress
    && MachoGetLastAddress64 (MachContext) <= Segment->VirtualAddress + Segment->Size);
  //
  // Prelinking must have eliminated all external relocations.
  //
  ASSERT (DySymtab->NumExternalRelocations == 0);
  //
  // Convert all relocations to fixups.
  //
  Relocations = (MACH_RELOCATION_INFO *) (
    (UINTN) MachHeader + DySymtab->ExternalRelocationsOffset
    );

  for (RelocIndex = 0; RelocIndex < DySymtab->NumOfLocalRelocations; ++RelocIndex) {
    InternalKcConvertRelocToFixup (
      SegChain,
      Segment,
      MachContext,
      &Relocations[RelocIndex],
      FirstSegment->VirtualAddress
      );
  }
}
