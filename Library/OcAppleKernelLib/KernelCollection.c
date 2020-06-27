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
    Command.FilesetEntry->CommandSize = ALIGN_VALUE (sizeof (MACH_FILESET_ENTRY_COMMAND) + StringSize, 8);
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
