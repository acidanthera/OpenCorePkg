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
#include "ProcessorBind.h"
#include "Uefi/UefiInternalFormRepresentation.h"

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
  LIST_ENTRY               *Kext;
  MACH_LOAD_COMMAND_PTR    Command;
  MACH_HEADER_64           *KextHeader;
  UINTN                    StringSize;

  Command.Address = (UINTN) MachHeader->Commands + MachHeader->CommandsSize;

  Kext       = GetFirstNode (&Context->InjectedKexts);

  while (!IsNull (&Context->InjectedKexts, Kext)) {
    PrelinkedKext = GET_INJECTED_KEXT_FROM_LINK (Kext);

    StringSize = AsciiStrSize (PrelinkedKext->Identifier);

    //
    // Write 8-byte aligned fileset command.
    //
    Command.FilesetEntry->CommandType = MACH_LOAD_COMMAND_FILESET_ENTRY;
    Command.FilesetEntry->CommandSize = (UINT32) ALIGN_VALUE (sizeof (MACH_FILESET_ENTRY_COMMAND) + StringSize, 8);
    Command.FilesetEntry->VirtualAddress = PrelinkedKext->Context.VirtualBase;
    KextHeader = MachoGetMachHeader64(&PrelinkedKext->Context.MachContext);
    ASSERT (KextHeader != NULL);
    ASSERT ((UINTN) KextHeader > (UINTN) Context->Prelinked
      && (UINTN) KextHeader < (UINTN) Context->Prelinked + Context->PrelinkedSize);
    Command.FilesetEntry->FileOffset     = (UINTN) KextHeader - (UINTN) Context->Prelinked;
    Command.FilesetEntry->EntryId.Offset = OFFSET_OF (MACH_FILESET_ENTRY_COMMAND, Payload);
    Command.FilesetEntry->Reserved       = 0;
    CopyMem (Command.FilesetEntry->Payload, PrelinkedKext->Identifier, StringSize);
    ZeroMem (
      &Command.FilesetEntry->Payload[StringSize],
      Command.FilesetEntry->CommandSize - Command.FilesetEntry->EntryId.Offset - StringSize
      );

    //
    // Refresh Mach-O header constants to include the new command.
    //
    MachHeader->NumCommands++;
    MachHeader->CommandsSize += Command.FilesetEntry->CommandSize;

    //
    // Proceed to the next command.
    //
    Command.Address += Command.FilesetEntry->CommandSize;

    Kext = GetNextNode (&Context->InjectedKexts, Kext);
  }

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
  Command.Segment64->VirtualAddress    = Context->KextsVmAddress;
  Command.Segment64->FileOffset        = Context->KextsFileOffset;
  //
  // In KC mode, PrelinkedLastAddress and PrelinkedLastLoadAddress are equal
  // before KEXTs are injected.
  //
  Command.Segment64->Size              = Context->PrelinkedLastAddress - Context->KextsVmAddress;
  Command.Segment64->FileSize          = Context->PrelinkedSize - Context->KextsFileOffset;
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

  //
  // Proceed to the next command.
  //
  Command.Address += Command.FilesetEntry->CommandSize;

  //
  // Write new __PRELINKED_INFO segment, as the first writeable segment is used by EfiBoot
  // as a relocation base. Relocation in KC mode is very simplified, and this segment PADDR
  // is simply used for all the relocations in DYSYMTAB. Unless we leave it untouched, sliding
  // will "relocate" invalid memory, as new plist is normally put at the end of KC memory.
  //
  // Note, that we might need to resolve Context->PrelinkedInfoSegment if regions move upper
  // and current __PRELINKED_INFO gets shifted.
  //
  CopyMem (
    (VOID *) Command.Address,
    Context->PrelinkedInfoSegment,
    Context->PrelinkedInfoSegment->CommandSize
    );

  //
  // Must choose a different name to avoid collisions.
  //
  CopyMem (Context->PrelinkedInfoSegment->SegmentName, "__KREMLIN_START", sizeof ("__KREMLIN_START"));
  CopyMem (Context->PrelinkedInfoSection->SectionName, "__kremlin_start", sizeof ("__kremlin_start"));

  //
  // Refresh Mach-O header constants to include the new segment command.
  //
  MachHeader->NumCommands++;
  MachHeader->CommandsSize += Context->PrelinkedInfoSegment->CommandSize;

  //
  // Update __PRELINKED_INFO pointers to get them updated at a later stage.
  //
  Context->PrelinkedInfoSegment = (MACH_SEGMENT_COMMAND_64 *) Command.Address;
  Context->PrelinkedInfoSection = &Context->PrelinkedInfoSegment->Sections[0];
}

EFI_STATUS
KcRebuildMachHeader (
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
  RequiredSize = FilesetSize + sizeof (MACH_SEGMENT_COMMAND_64) + Context->PrelinkedInfoSegment->CommandSize;

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

UINT32
KcGetSegmentFixupChainsSize (
  IN UINT32  SegmentSize
  )
{
  CONST MACH_DYLD_CHAINED_STARTS_IN_SEGMENT *Dummy;

  ASSERT (SegmentSize % MACHO_PAGE_SIZE == 0);

  if (SegmentSize > PRELINKED_KEXTS_MAX_SIZE) {
    return 0;
  }

  return (UINT32) (sizeof (*Dummy)
    + (SegmentSize / MACHO_PAGE_SIZE) * sizeof (Dummy->PageStart[0]));
}

EFI_STATUS
KcInitKextFixupChains (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     UINT32             SegChainSize,
  IN     UINT32             ReservedSize
  )
{
  BOOLEAN                                Result;
  CONST MACH_LINKEDIT_DATA_COMMAND       *DyldChainedFixups;
  CONST MACHO_DYLD_CHAINED_FIXUPS_HEADER *DyldChainedFixupsHdr;
  MACH_DYLD_CHAINED_STARTS_IN_IMAGE      *DyldChainedStarts;
  UINT32                                 DyldChainedStartsSize;
  UINT32                                 SegIndex;

  ASSERT (Context != NULL);
  ASSERT (ReservedSize % MACHO_PAGE_SIZE == 0);

  ASSERT (Context->KextsFixupChains != NULL);

  ASSERT (SegChainSize != 0);
  ASSERT (
    SegChainSize >= KcGetSegmentFixupChainsSize (ReservedSize)
    );
  //
  // Context initialisation guarantees the command size is a multiple of 8.
  //
  STATIC_ASSERT (
    OC_ALIGNOF (MACH_LINKEDIT_DATA_COMMAND) <= sizeof (UINT64),
    "Alignment is not guaranteed."
    );
  
  DyldChainedFixups = (CONST MACH_LINKEDIT_DATA_COMMAND *) MachoGetNextCommand64 (
    &Context->PrelinkedMachContext,
    MACH_LOAD_COMMAND_DYLD_CHAINED_FIXUPS,
    NULL
    );
  //
  // Show that StartsInImage is aligned relative to __LINKEDIT start so we only
  // need to check DataOffset below.
  //
  ASSERT ((Context->LinkEditSegment->FileOffset % MACHO_PAGE_SIZE) == 0);
  STATIC_ASSERT (
    OC_TYPE_ALIGNED (MACHO_DYLD_CHAINED_FIXUPS_HEADER, MACHO_PAGE_SIZE),
    "Alignment is not guaranteed."
    );

  if (DyldChainedFixups == NULL
   || DyldChainedFixups->CommandSize != sizeof (*DyldChainedFixups)
   || DyldChainedFixups->DataSize < sizeof (MACHO_DYLD_CHAINED_FIXUPS_HEADER)
   || DyldChainedFixups->DataOffset < Context->LinkEditSegment->FileOffset
   || (Context->LinkEditSegment->FileOffset + Context->LinkEditSegment->FileSize)
        - DyldChainedFixups->DataOffset < DyldChainedFixups->DataSize
   || !OC_TYPE_ALIGNED (MACHO_DYLD_CHAINED_FIXUPS_HEADER, DyldChainedFixups->DataOffset)) {
    DEBUG ((DEBUG_WARN, "ChainedFixups insane\n"));
    return EFI_UNSUPPORTED;
  }

  DyldChainedFixupsHdr = (CONST MACHO_DYLD_CHAINED_FIXUPS_HEADER *) (VOID *) (
    Context->Prelinked + DyldChainedFixups->DataOffset
    );
  if (DyldChainedFixupsHdr->FixupsVersion != 0) {
    DEBUG ((DEBUG_WARN, "Unrecognised version\n"));
  }

  STATIC_ASSERT (
    OC_ALIGNOF (MACHO_DYLD_CHAINED_FIXUPS_HEADER) >= OC_ALIGNOF (MACH_DYLD_CHAINED_STARTS_IN_IMAGE),
    "Alignment is not guaranteed."
    );

  STATIC_ASSERT (
    sizeof (MACHO_DYLD_CHAINED_FIXUPS_HEADER) >= sizeof (MACH_DYLD_CHAINED_STARTS_IN_IMAGE),
    "The subtraction below is unsafe."
    );

  if (DyldChainedFixupsHdr->StartsOffset < sizeof (MACHO_DYLD_CHAINED_FIXUPS_HEADER)
   || DyldChainedFixupsHdr->StartsOffset > DyldChainedFixups->DataSize - sizeof (MACH_DYLD_CHAINED_STARTS_IN_IMAGE)
   || !OC_TYPE_ALIGNED (MACH_DYLD_CHAINED_STARTS_IN_IMAGE, DyldChainedFixupsHdr->StartsOffset)) {
     DEBUG ((DEBUG_WARN, "ChainedFixupsHdr insane\n"));
     return EFI_UNSUPPORTED;
  }

  DyldChainedStarts = (MACH_DYLD_CHAINED_STARTS_IN_IMAGE *) (VOID *) (
    (UINTN) DyldChainedFixupsHdr + DyldChainedFixupsHdr->StartsOffset
    );

  Result = OcOverflowMulAddU32 (
    DyldChainedStarts->NumSegments,
    sizeof (*DyldChainedStarts->SegInfoOffset),
    sizeof (*DyldChainedStarts),
    &DyldChainedStartsSize
    );
  if (Result || DyldChainedStartsSize > DyldChainedFixups->DataSize - DyldChainedFixupsHdr->StartsOffset) {
    DEBUG ((DEBUG_WARN, "DyldChainedFixups insane\n"));
    return EFI_UNSUPPORTED;
  }

  for (SegIndex = 0; SegIndex < DyldChainedStarts->NumSegments; ++SegIndex) {
    if (DyldChainedStarts->SegInfoOffset[SegIndex] == 0) {
      break;
    }
  }

  if (SegIndex == DyldChainedStarts->NumSegments) {
    DEBUG ((DEBUG_WARN, "No free start\n"));
    return EFI_UNSUPPORTED;
  }

  ASSERT ((UINTN) Context->KextsFixupChains > (UINTN) DyldChainedStarts);
  DyldChainedStarts->SegInfoOffset[SegIndex] = (UINT32) (
    (UINTN) Context->KextsFixupChains - (UINTN) DyldChainedStarts
    );

  Context->KextsFixupChains->Size            = SegChainSize;
  Context->KextsFixupChains->PageSize        = MACHO_PAGE_SIZE;
  Context->KextsFixupChains->PointerFormat   = MACH_DYLD_CHAINED_PTR_X86_64_KERNEL_CACHE;
  Context->KextsFixupChains->SegmentOffset   = Context->KextsVmAddress - Context->VirtualBase;
  Context->KextsFixupChains->MaxValidPointer = 0;
  Context->KextsFixupChains->PageCount       = (UINT16) (ReservedSize / MACHO_PAGE_SIZE);
  //
  // Initialise all pages with no associated fixups.
  // Sets all PageStarts to MACH_DYLD_CHAINED_PTR_START_NONE.
  //
  SetMem (
    Context->KextsFixupChains->PageStart,
    (UINT32) Context->KextsFixupChains->PageCount
      * sizeof (Context->KextsFixupChains->PageStart[0]),
    0xFF
    );
  return EFI_SUCCESS;
}

/*
  Indexes RelocInfo into the fixup chains SegChain of Segment.

  @param[in,out] Context      Prelinked context.
  @param[in]     MachContext  The context of the Mach-O RelocInfo belongs to. It
                              must have been prelinked by OcAppleKernelLib. The
                              image must reside in Segment.
  @param[in]     RelocInfo    The relocation to add a fixup of.
  @param[in]     RelocBase    The relocation base address.
*/
STATIC
VOID
InternalKcConvertRelocToFixup (
  IN OUT PRELINKED_CONTEXT           *Context,
  IN     OC_MACHO_CONTEXT            *MachContext,
  IN     CONST MACH_RELOCATION_INFO  *RelocInfo,
  IN     UINT64                      RelocBase
  )
{
  UINT8                                        *SegmentData;
  UINT8                                        *SegmentPageData;

  UINT64                                       RelocAddress;
  UINT32                                       RelocOffsetInSeg;
  VOID                                         *RelocDest;

  UINT16                                       NewFixupPage;
  UINT16                                       NewFixupPageOffset;
  MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE_REBASE NewFixup;

  UINT16                                       IterFixupPageOffset;
  VOID                                         *IterFixupData;
  MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE_REBASE IterFixup;
  UINT16                                       NextIterFixupPageOffset;

  UINT16                                       FixupDelta;

  ASSERT (Context != NULL);
  ASSERT (MachContext != NULL);
  ASSERT (RelocInfo != NULL);

  ASSERT (Context->KextsFixupChains != NULL);
  ASSERT (Context->KextsFixupChains->PageSize == MACHO_PAGE_SIZE);
  //
  // The entire KEXT and thus its relocations must be in Segment.
  // Mach-O images are limited to 4 GB size by OcMachoLib, so the cast is safe.
  //
  RelocAddress     = RelocBase + (UINT32) RelocInfo->Address;
  RelocOffsetInSeg = (UINT32) (RelocAddress - Context->KextsVmAddress);
  //
  // For now we assume we prelinked already and the relocations are sane.
  //
  ASSERT (RelocInfo->Extern == 0);
  ASSERT (RelocInfo->Type == MachX8664RelocUnsigned);
  ASSERT (RelocAddress >= Context->KextsVmAddress);
  ASSERT (RelocOffsetInSeg <= Context->PrelinkedSize - Context->KextsFileOffset);
  ASSERT (Context->KextsFileOffset - RelocOffsetInSeg >= 8);
  //
  // Create a new fixup based on the data of RelocInfo.
  //
  SegmentData = Context->Prelinked + Context->KextsFileOffset;
  RelocDest   = SegmentData + RelocOffsetInSeg;
  //
  // It has been observed all fields but target and next are 0 for the kernel
  // KC. For isAuth, this is because x86 does not support Pointer
  // Authentication.
  //
  ZeroMem (&NewFixup, sizeof (NewFixup));
  //
  // This 1MB here is a bit of a hack. I think it is just the same thing
  // as KERNEL_BASE_PADDR in OcAfterBootCompatLib.
  //
  NewFixup.Target = ReadUnaligned64 (RelocDest) - KERNEL_FIXUP_OFFSET;

  NewFixupPage       = (UINT16) (RelocOffsetInSeg / MACHO_PAGE_SIZE);
  NewFixupPageOffset = (UINT16) (RelocOffsetInSeg % MACHO_PAGE_SIZE);

  IterFixupPageOffset = Context->KextsFixupChains->PageStart[NewFixupPage];

  if (IterFixupPageOffset == MACH_DYLD_CHAINED_PTR_START_NONE) {
    //
    // The current page has no fixups, just assign and terminate this one.
    //
    Context->KextsFixupChains->PageStart[NewFixupPage] = NewFixupPageOffset;
    //
    // Fixup.next is 0 (i.e. the chain is terminated) already.
    //
  } else if (NewFixupPageOffset < IterFixupPageOffset) {
    //
    // RelocInfo preceeds the first fixup of the page - prepend the new fixup.
    //
    NewFixup.Next = IterFixupPageOffset - NewFixupPageOffset;
    Context->KextsFixupChains->PageStart[NewFixupPage] = NewFixupPageOffset;
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
      ASSERT (IterFixup.Next >= FixupDelta);
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
  Indexes all relocations of MachContext into the kernel described by Context.

  @param[in,out] Context      Prelinked context.
  @param[in]     MachContext  The context of the Mach-O to index. It must have
                              been prelinked by OcAppleKernelLib. The image
                              must reside in Segment.
*/
VOID
KcKextIndexFixups (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     OC_MACHO_CONTEXT   *MachContext
  )
{
  CONST MACH_DYSYMTAB_COMMAND   *DySymtab;
  CONST MACH_SEGMENT_COMMAND_64 *FirstSegment;
  MACH_HEADER_64                *MachHeader;
  CONST MACH_RELOCATION_INFO    *Relocations;
  UINT32                        RelocIndex;

  ASSERT (Context != NULL);
  ASSERT (MachContext != NULL);

  MachHeader = MachoGetMachHeader64 (MachContext);

  //
  // Kexts with fixups are now dylibs in cache.
  // This is required for OSKext_protect to work properly
  // as the kernel map that operates on vm_map no longer has kext addresses.
  //
  MachHeader->Flags |= MACH_HEADER_FLAG_DYLIB_IN_CACHE;

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
  //
  // If DYSYMTAB does not exist, the KEXT must have already not been flagged for
  // DYLD linkage as otherwise prelinking would have failed.
  //
  if (DySymtab == NULL) {
    return;
  }

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
  ASSERT (FirstSegment->VirtualAddress >= Context->KextsVmAddress);
  ASSERT (MachoGetLastAddress64 (MachContext) <= Context->PrelinkedLastAddress);
  //
  // Prelinking must have eliminated all external relocations.
  //
  ASSERT (DySymtab->NumExternalRelocations == 0);
  //
  // Convert all relocations to fixups.
  //
  Relocations = (MACH_RELOCATION_INFO *) (
    (UINTN) MachHeader + DySymtab->LocalRelocationsOffset
    );

  DEBUG ((
    DEBUG_INFO,
    "OCAK: Local relocs %u on %LX\n",
    DySymtab->NumOfLocalRelocations,
    FirstSegment->VirtualAddress
    ));

  for (RelocIndex = 0; RelocIndex < DySymtab->NumOfLocalRelocations; ++RelocIndex) {
    InternalKcConvertRelocToFixup (
      Context,
      MachContext,
      &Relocations[RelocIndex],
      FirstSegment->VirtualAddress
      );
  }
}

UINT32
KcGetKextSize (
  IN PRELINKED_CONTEXT  *Context,
  IN UINT64             SourceAddress
  )
{
  MACH_HEADER_64          *KcHeader;
  MACH_SEGMENT_COMMAND_64 *Segment;

  ASSERT (Context != NULL);
  ASSERT (Context->IsKernelCollection);

  KcHeader = MachoGetMachHeader64 (&Context->PrelinkedMachContext);
  ASSERT (KcHeader != NULL);
  //
  // Find the KC segment that contains the KEXT at SourceAddress.
  //
  for (
    Segment = MachoGetNextSegment64 (&Context->PrelinkedMachContext, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (&Context->PrelinkedMachContext, Segment)
    )
  {
    if (SourceAddress >= Segment->VirtualAddress
     && SourceAddress - Segment->VirtualAddress < Segment->Size) {
      //
      // Ensure SourceAddress lies in file memory.
      //
      if (SourceAddress - Segment->VirtualAddress > Segment->FileSize) {
        return 0;
      }

      //
      // All kexts in KC use joint __LINKEDIT with the kernel.
      //
      return (UINT32) (Context->LinkEditSegment->VirtualAddress
        - Segment->VirtualAddress + Context->LinkEditSegment->Size);
    }
  }

  return 0;
}

EFI_STATUS
KcKextApplyFileDelta (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Delta
  )
{
  MACH_HEADER_64          *KextHeader;
  MACH_LOAD_COMMAND       *Command;
  UINTN                   TopOfCommands;
  MACH_SEGMENT_COMMAND_64 *Segment;
  MACH_SYMTAB_COMMAND     *Symtab;
  MACH_DYSYMTAB_COMMAND   *DySymtab;
  UINT32                  SectIndex;

  ASSERT (Context != NULL);
  ASSERT (Delta > 0);

  KextHeader = MachoGetMachHeader64 (Context);
  ASSERT (KextHeader != NULL);

  TopOfCommands = ((UINTN) KextHeader->Commands + KextHeader->CommandsSize);
  //
  // Iterate over all Load Commands to rebase them based on type.
  //
  for (
    Command = KextHeader->Commands;
    (UINTN) Command < TopOfCommands;
    Command = NEXT_MACH_LOAD_COMMAND (Command)
    ) {
    switch (Command->CommandType) {
      case MACH_LOAD_COMMAND_SEGMENT_64:
        if (Command->CommandSize < sizeof (MACH_SEGMENT_COMMAND_64)) {
          return EFI_UNSUPPORTED;
        }

        Segment = (MACH_SEGMENT_COMMAND_64 *) (VOID *) Command;
        //
        // Rebase the segment's sections.
        //
        for (SectIndex = 0; SectIndex < Segment->NumSections; ++SectIndex) {
          if (Segment->Sections[SectIndex].Offset != 0) {
            Segment->Sections[SectIndex].Offset += Delta;
          }
        }
        //
        // Rebase the segment itself.
        //
        if (Segment->FileOffset != 0 || Segment->FileSize != 0) {
          Segment->FileOffset += Delta;
        }

        break;

      case MACH_LOAD_COMMAND_SYMTAB:
        if (Command->CommandSize != sizeof (MACH_SYMTAB_COMMAND)) {
          return EFI_UNSUPPORTED;
        }

        Symtab = (MACH_SYMTAB_COMMAND *) (VOID *) Command;
        //
        // Rebase all SYMTAB offsets.
        //
        if (Symtab->SymbolsOffset != 0) {
          Symtab->SymbolsOffset += Delta;
        }

        if (Symtab->StringsOffset != 0) {
          Symtab->StringsOffset += Delta;
        }

        break;

      case MACH_LOAD_COMMAND_DYSYMTAB:
        if (Command->CommandSize != sizeof (MACH_DYSYMTAB_COMMAND)) {
          return EFI_UNSUPPORTED;
        }

        DySymtab = (MACH_DYSYMTAB_COMMAND *) (VOID *) Command;
        //
        // Rebase DYSYMTAB fields that make sense in a prelinked context.
        //
        if (DySymtab->IndirectSymbolsOffset != 0) {
          DySymtab->IndirectSymbolsOffset += Delta;
        }
        //
        // KC KEXTs use chained fixups indexed by the KC header.
        //
        DySymtab->NumOfLocalRelocations  = 0;
        DySymtab->LocalRelocationsOffset = 0;

        break;

      default:
        //
        // Other common segments do not contain offsets or they are unused.
        //
        break;
    }
  }

  //
  // Update the container offset to make sure we can link against this
  // kext later as well.
  //
  Context->ContainerOffset = Delta;

  return EFI_SUCCESS;
}
