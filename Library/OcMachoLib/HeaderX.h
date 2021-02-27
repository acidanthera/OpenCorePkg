/**
  Provides services for Mach-O headers.

Copyright (C) 2016 - 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MachoX.h"

/**
  Returns whether Section is sane.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Section  Section to verify.
  @param[in]     Segment  Segment the section is part of.

**/
STATIC
BOOLEAN
InternalSectionIsSane (
  IN OUT OC_MACHO_CONTEXT               *Context,
  IN     CONST MACH_SECTION_X           *Section,
  IN     CONST MACH_SEGMENT_COMMAND_X   *Segment
  )
{
  MACH_UINT_X   TopOffsetX;
  UINT32        TopOffset32;
  MACH_UINT_X   TopOfSegment;
  BOOLEAN       Result;
  MACH_UINT_X   TopOfSection;

  ASSERT (Context != NULL);
  ASSERT (Section != NULL);
  ASSERT (Segment != NULL);
  MACH_ASSERT_X (Context);

  //
  // Section->Alignment is stored as a power of 2.
  //
  if ((Section->Alignment > 31)
   || ((Section->Offset != 0) && (Section->Offset < Segment->FileOffset))) {
    return FALSE;
  }

  TopOfSegment = (Segment->VirtualAddress + Segment->Size);
  Result       = MACH_X (OcOverflowAddU) (
    Section->Address,
    Section->Size,
    &TopOfSection
    );
  if (Result || (TopOfSection > TopOfSegment)) {
    return FALSE;
  }

  Result   = MACH_X (OcOverflowAddU) (
    Section->Offset,
    Section->Size,
    &TopOffsetX
    );
  if (Result || (TopOffsetX > (Segment->FileOffset + Segment->FileSize))) {
    return FALSE;
  }

  if (Section->NumRelocations != 0) {
    Result = OcOverflowSubU32 (
      Section->RelocationsOffset,
      Context->ContainerOffset,
      &TopOffset32
      );
    Result |= OcOverflowMulAddU32 (
      Section->NumRelocations,
      sizeof (MACH_RELOCATION_INFO),
      TopOffset32,
      &TopOffset32
      );
    if (Result || (TopOffset32 > Context->FileSize)) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Strip superfluous Load Commands from the Mach-O header.  This includes the
  Code Signature Load Command which must be removed for the binary has been
  modified by the prelinking routines.

  @param[in,out] MachHeader  Mach-O header to strip the Load Commands from.

**/
STATIC
VOID
InternalStripLoadCommands (
  IN OUT MACH_HEADER_X    *MachHeader
  )
{
  STATIC CONST MACH_LOAD_COMMAND_TYPE LoadCommandsToStrip[] = {
    MACH_LOAD_COMMAND_CODE_SIGNATURE,
    MACH_LOAD_COMMAND_DYLD_INFO,
    MACH_LOAD_COMMAND_DYLD_INFO_ONLY,
    MACH_LOAD_COMMAND_FUNCTION_STARTS,
    MACH_LOAD_COMMAND_DATA_IN_CODE,
    MACH_LOAD_COMMAND_DYLIB_CODE_SIGN_DRS,
    MACH_LOAD_COMMAND_UNIX_THREAD
  };

  UINT32            Index;
  UINT32            Index2;
  MACH_LOAD_COMMAND *LoadCommand;
  UINT32            SizeOfLeftCommands;
  UINT32            OriginalCommandSize;
  //
  // Delete the Code Signature Load Command if existent as we modified the
  // binary, as well as linker metadata not needed for runtime operation.
  //
  LoadCommand         = MachHeader->Commands;
  SizeOfLeftCommands  = MachHeader->CommandsSize;
  OriginalCommandSize = SizeOfLeftCommands;

  for (Index = 0; Index < MachHeader->NumCommands; ++Index) {
    SizeOfLeftCommands -= LoadCommand->CommandSize;

    for (Index2 = 0; Index2 < ARRAY_SIZE (LoadCommandsToStrip); ++Index2) {
      if (LoadCommand->CommandType == LoadCommandsToStrip[Index2]) {
        if (Index != (MachHeader->NumCommands - 1)) {
          //
          // If the current Load Command is not the last one, relocate the
          // subsequent ones.
          //
          CopyMem (
            LoadCommand,
            NEXT_MACH_LOAD_COMMAND (LoadCommand),
            SizeOfLeftCommands
            );
        }

        --MachHeader->NumCommands;
        MachHeader->CommandsSize -= LoadCommand->CommandSize;

        break;
      }
    }

    LoadCommand = NEXT_MACH_LOAD_COMMAND (LoadCommand);
  }

  ZeroMem (LoadCommand, OriginalCommandSize - MachHeader->CommandsSize);
}

UINT32
MACH_X (InternalMachoGetVmSize) (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  MACH_UINT_X               VmSize;
  MACH_SEGMENT_COMMAND_X    *Segment;

  ASSERT (Context != NULL);
  ASSERT (Context->FileSize != 0);
  MACH_ASSERT_X (Context);

  VmSize = 0;

  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    if (MACH_X (OcOverflowAddU) (VmSize, Segment->Size, &VmSize)) {
      return 0;
    }
    VmSize = MACHO_ALIGN (VmSize);
  }

#ifndef MACHO_LIB_32
  if (VmSize > MAX_UINT32) {
    return 0;
  }
#endif

  return MACH_X_TO_UINT32 (VmSize);
}

MACH_LOAD_COMMAND *
MACH_X (InternalMachoGetNextCommand) (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_LOAD_COMMAND_TYPE   LoadCommandType,
  IN     CONST MACH_LOAD_COMMAND  *LoadCommand  OPTIONAL
  )
{
  MACH_LOAD_COMMAND       *Command;
  MACH_HEADER_X           *MachHeader;
  UINTN                   TopOfCommands;

  ASSERT (Context != NULL);
  ASSERT (Context->MachHeader != NULL);
  MACH_ASSERT_X (Context);

  MachHeader = MACH_X (&Context->MachHeader->Header);

  TopOfCommands = ((UINTN)MachHeader->Commands + MachHeader->CommandsSize);

  if (LoadCommand != NULL) {
    ASSERT (
      (LoadCommand >= &MachHeader->Commands[0])
        && ((UINTN)LoadCommand <= TopOfCommands)
      );
    Command = NEXT_MACH_LOAD_COMMAND (LoadCommand);
  } else {
    Command = &MachHeader->Commands[0];
  }
  
  for (
    ;
    (UINTN)Command < TopOfCommands;
    Command = NEXT_MACH_LOAD_COMMAND (Command)
    ) {
    if (Command->CommandType == LoadCommandType) {
      return Command;
    }
  }

  return NULL;
}

VOID *
MACH_X (InternalMachoGetFilePointerByAddress) (
  IN OUT OC_MACHO_CONTEXT   *Context,
  IN     MACH_UINT_X        Address,
  OUT    UINT32             *MaxSize OPTIONAL
  )
{
  CONST MACH_SEGMENT_COMMAND_X  *Segment;
  MACH_UINT_X                   Offset;

  ASSERT (Context != NULL);
  MACH_ASSERT_X (Context);

  Segment = NULL;
  while ((Segment = MACH_X (MachoGetNextSegment) (Context, Segment)) != NULL) {
    if ((Address >= Segment->VirtualAddress)
     && (Address < Segment->VirtualAddress + Segment->Size)) {
      Offset = (Address - Segment->VirtualAddress);

      if (MaxSize != NULL) {
        *MaxSize = MACH_X_TO_UINT32 (Segment->Size - Offset);
      }

      Offset += Segment->FileOffset - Context->ContainerOffset;
      return (VOID *) ((UINTN) Context->MachHeader + (UINTN) Offset);
    }
  }

  return NULL;
}

UINT32
MACH_X (InternalMachoExpandImage) (
  IN  OC_MACHO_CONTEXT   *Context,
  IN  BOOLEAN            CalculateSizeOnly,
  OUT UINT8              *Destination,
  IN  UINT32             DestinationSize,
  IN  BOOLEAN            Strip,
  OUT UINT64             *FileOffset OPTIONAL
  )
{
  MACH_HEADER_X             *Header;
  UINT8                     *Source;
  UINT32                    HeaderSize;
  UINT32                    HeaderSizeAligned;
  BOOLEAN                   IsObject;

  MACH_UINT_X               CopyFileOffset;
  MACH_UINT_X               CopyFileSize;
  MACH_UINT_X               CopyVmSize;
  MACH_UINT_X               SegmentOffset;
  UINT32                    SectionOffset;
  UINT32                    SymbolsOffset;
  UINT32                    StringsOffset;
  UINT32                    AlignedOffset;
  UINT32                    RelocationsSize;
  UINT32                    SymtabSize;
  UINT32                    CurrentDelta;
  UINT32                    OriginalDelta;
  MACH_UINT_X               CurrentSize;
  UINT32                    FileSize;
  MACH_SEGMENT_COMMAND_X    *Segment;
  MACH_SEGMENT_COMMAND_X    *FirstSegment;
  MACH_SEGMENT_COMMAND_X    *DstSegment;
  MACH_SYMTAB_COMMAND       *Symtab;
  MACH_DYSYMTAB_COMMAND     *DySymtab;
  UINT32                    Index;
  BOOLEAN                   FoundLinkedit;

  ASSERT (Context != NULL);
  ASSERT (Context->FileSize > 0);
  MACH_ASSERT_X (Context);

  if (!CalculateSizeOnly) {
    ASSERT (Destination != NULL);
    ASSERT (DestinationSize > 0);
  }

  //
  // Header is valid, copy it first.
  //
  Header     = MACH_X (MachoGetMachHeader) (Context);
  IsObject   = Header->FileType == MachHeaderFileTypeObject;
  Source     = (UINT8 *) Header;
  HeaderSize = sizeof (*Header) + Header->CommandsSize;
  if (!CalculateSizeOnly) {
    if (HeaderSize > DestinationSize) {
      return 0;
    }
    CopyMem (Destination, Header, HeaderSize);
  }

  //
  // Align header size to page size if this is an MH_OBJECT.
  // The header does not exist in a segment in MH_OBJECT files.
  //
  HeaderSizeAligned = 0;
  if (IsObject) {
    HeaderSizeAligned = MACHO_ALIGN (HeaderSize);
    if (!CalculateSizeOnly) {
      if (HeaderSizeAligned > DestinationSize) {
        return 0;
      }
      ZeroMem (&Destination[HeaderSize], HeaderSizeAligned - HeaderSize);
    }
  }

  if (FileOffset != NULL) {
    *FileOffset = 0;
  }

  CurrentDelta  = 0;
  FirstSegment  = NULL;
  CurrentSize   = 0;
  FoundLinkedit = FALSE;
  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    if (Segment->FileSize > Segment->Size) {
      return 0;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "OCMCO: Src segment offset 0x%X size 0x%X delta 0x%X\n",
      Segment->FileOffset,
      Segment->FileSize,
      CurrentDelta
      ));

    //
    // Align delta by x86 page size, this is what our lib expects.
    //
    OriginalDelta = CurrentDelta;
    CurrentDelta  = MACHO_ALIGN (CurrentDelta);

    //
    // Do not overwrite header. Header must be in the first segment, but not if we are MH_OBJECT.
    // For objects, the header size will be aligned so we'll need to shift segments to account for this.
    //
    CopyFileOffset = Segment->FileOffset - Context->ContainerOffset;
    CopyFileSize   = Segment->FileSize;
    CopyVmSize     = Segment->Size;

    if (IsObject && CopyFileOffset <= HeaderSizeAligned) {
      CurrentDelta    = HeaderSizeAligned - HeaderSize;
      //
      // Some kexts seem to have empty space after header and before segment.
      //
      if (CopyFileOffset > HeaderSize) {
        CurrentDelta -= (UINT32) (CopyFileOffset - HeaderSize);
      }
      if (FileOffset != NULL) {
        *FileOffset = HeaderSizeAligned;
      }
    } else if (!IsObject && CopyFileOffset <= HeaderSize) {
      CopyFileOffset = HeaderSize;
      CopyFileSize   = Segment->FileSize - CopyFileOffset;
      CopyVmSize     = Segment->Size - CopyFileOffset;
      if (CopyFileSize > Segment->FileSize || CopyVmSize > Segment->Size) {
        //
        // Header must fit in one segment.
        //
        return 0;
      }
    }

    //
    // Store first segment.
    //
    if (FirstSegment == NULL) {
      FirstSegment = Segment;
    }

    //
    // Ensure that it still fits. In legit files segments are ordered.
    // We do not care for other (the file will be truncated).
    //
    if (MACH_X (OcOverflowTriAddU) (CopyFileOffset, CurrentDelta, CopyVmSize, &CurrentSize)
      || (!CalculateSizeOnly && CurrentSize > DestinationSize)) {
      return 0;
    }

    //
    // Copy and zero fill file data. We can do this because only last sections can have 0 file size.
    //
#ifndef MACHO_LIB_32
    ASSERT (CopyFileSize <= MAX_UINTN && CopyVmSize <= MAX_UINTN);
#endif
    if (!CalculateSizeOnly) {
      ZeroMem (&Destination[CopyFileOffset + OriginalDelta], CurrentDelta - OriginalDelta);
      CopyMem (&Destination[CopyFileOffset + CurrentDelta], &Source[CopyFileOffset], (UINTN) CopyFileSize);
      ZeroMem (&Destination[CopyFileOffset + CurrentDelta + CopyFileSize], (UINTN) (CopyVmSize - CopyFileSize));
    }
  
    //
    // Grab pointer to destination segment and updated offsets.
    // If we are only calculating a size, we'll just use the source segment
    // here for a cleaner function.
    //
    if (!CalculateSizeOnly) {
      DstSegment = (MACH_SEGMENT_COMMAND_X *) ((UINT8 *) Segment - Source + Destination);
    } else {
      DstSegment = Segment;
    }
    SegmentOffset = DstSegment->FileOffset + CurrentDelta;

    DEBUG ((
      DEBUG_VERBOSE,
      "OCMCO: Dst segment offset 0x%X size 0x%X delta 0x%X\n",
      SegmentOffset,
      DstSegment->Size,
      CurrentDelta
      ));

    if (!IsObject && DstSegment->VirtualAddress - (SegmentOffset - Context->ContainerOffset) != FirstSegment->VirtualAddress) {
      return 0;
    }
    if (!CalculateSizeOnly) {
      DstSegment->FileOffset = SegmentOffset;
      DstSegment->FileSize   = DstSegment->Size;
    }

    //
    // We need to update fields in SYMTAB and DYSYMTAB. Tables have to be present before 0 FileSize
    // sections as they have data, so we update them before parsing sections. 
    // Note: There is an assumption they are in __LINKEDIT segment, another option is to check addresses.
    //
    if (AsciiStrnCmp (DstSegment->SegmentName, "__LINKEDIT", ARRAY_SIZE (DstSegment->SegmentName)) == 0) {
      FoundLinkedit = TRUE;

      if (!CalculateSizeOnly) {
        Symtab = (MACH_SYMTAB_COMMAND *) (
          MachoGetNextCommand (
            Context,
            MACH_LOAD_COMMAND_SYMTAB,
            NULL
            )
          );

        if (Symtab != NULL) {
          Symtab = (MACH_SYMTAB_COMMAND *) ((UINT8 *) Symtab - Source + Destination);
          if (Symtab->SymbolsOffset != 0) {
            Symtab->SymbolsOffset += CurrentDelta;
          }
          if (Symtab->StringsOffset != 0) {
            Symtab->StringsOffset += CurrentDelta;
          }
        }

        DySymtab = (MACH_DYSYMTAB_COMMAND *) (
          MachoGetNextCommand (
            Context,
            MACH_LOAD_COMMAND_DYSYMTAB,
            NULL
            )
          );

        if (DySymtab != NULL) {
          DySymtab = (MACH_DYSYMTAB_COMMAND *) ((UINT8 *) DySymtab - Source + Destination);
          if (DySymtab->TableOfContentsNumEntries != 0) {
            DySymtab->TableOfContentsNumEntries += CurrentDelta;
          }
          if (DySymtab->ModuleTableFileOffset != 0) {
            DySymtab->ModuleTableFileOffset += CurrentDelta;
          }
          if (DySymtab->ReferencedSymbolTableFileOffset != 0) {
            DySymtab->ReferencedSymbolTableFileOffset += CurrentDelta;
          }
          if (DySymtab->IndirectSymbolsOffset != 0) {
            DySymtab->IndirectSymbolsOffset += CurrentDelta;
          }
          if (DySymtab->ExternalRelocationsOffset != 0) {
            DySymtab->ExternalRelocationsOffset += CurrentDelta;
          }
          if (DySymtab->LocalRelocationsOffset != 0) {
            DySymtab->LocalRelocationsOffset += CurrentDelta;
          }
        }
      }
    }

    //
    // These may well wrap around with invalid data.
    // But we do not care, as we do not access these fields ourselves,
    // and later on the section values are checked by MachoLib.
    // Note: There is an assumption that 'CopyFileOffset + CurrentDelta' is aligned.
    //
    OriginalDelta  = CurrentDelta;
    CopyFileOffset = DstSegment->FileOffset;
    for (Index = 0; Index < DstSegment->NumSections; ++Index) {
      SectionOffset = DstSegment->Sections[Index].Offset;

      DEBUG ((
        DEBUG_VERBOSE,
        "OCMCO: Src section %u offset 0x%X size 0x%X delta 0x%X\n",
        Index,
        SectionOffset,
        DstSegment->Sections[Index].Size,
        CurrentDelta
        ));

      //
      // Allocate space for zero offset sections.
      // For all other sections, copy as-is.
      //
      if (DstSegment->Sections[Index].Offset == 0) {
        SectionOffset   = MACH_X_TO_UINT32 (CopyFileOffset);
        CopyFileOffset += MACH_X_TO_UINT32 (DstSegment->Sections[Index].Size);
        CurrentDelta   += MACH_X_TO_UINT32 (DstSegment->Sections[Index].Size);
      } else {
        SectionOffset  += CurrentDelta;
        CopyFileOffset  = SectionOffset + DstSegment->Sections[Index].Size;
      }

      DEBUG ((
        DEBUG_VERBOSE,
        "OCMCO: Dst section %u offset 0x%X size 0x%X delta 0x%X\n",
        Index,
        SectionOffset,
        DstSegment->Sections[Index].Size,
        CurrentDelta
        ));

      if (!CalculateSizeOnly) {
        DstSegment->Sections[Index].Offset = SectionOffset;
      }
    }

    CurrentDelta = OriginalDelta + MACH_X_TO_UINT32 (Segment->Size - Segment->FileSize);
  }

  //
  // If we did not find __LINKEDIT, we'll need to manually update SYMTAB and section relocations.
  // This assumes that if there is __LINKEDIT, all relocations are in __LINKEDIT and not in sections.
  //
  if (!FoundLinkedit) {
    DEBUG ((DEBUG_VERBOSE, "OCMCO: __LINKEDIT not found\n"));

    //
    // Copy section relocations.
    //
    for (
      Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
      Segment != NULL;
      Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
      ) {

      if (!CalculateSizeOnly) {
        DstSegment = (MACH_SEGMENT_COMMAND_X *) ((UINT8 *) Segment - Source + Destination);
      } else {
        DstSegment = Segment;
      }

      for (Index = 0; Index < DstSegment->NumSections; ++Index) {
        SectionOffset = DstSegment->Sections[Index].RelocationsOffset;

        if (SectionOffset != 0) {
          DEBUG ((
            DEBUG_VERBOSE,
            "OCMCO: Src section %u relocs offset 0x%X count %u delta 0x%X\n",
            Index,
            SectionOffset,
            DstSegment->Sections[Index].NumRelocations,
            CurrentDelta
            ));

          CopyFileOffset  = SectionOffset;
          RelocationsSize = DstSegment->Sections[Index].NumRelocations * sizeof (MACH_RELOCATION_INFO);
          AlignedOffset   = ALIGN_VALUE (SectionOffset + CurrentDelta, sizeof (MACH_RELOCATION_INFO));
          CurrentDelta    = AlignedOffset - SectionOffset;

          if (MACH_X (OcOverflowTriAddU) (CopyFileOffset, CurrentDelta, RelocationsSize, &CurrentSize)
            || (!CalculateSizeOnly && CurrentSize > DestinationSize)) {
            return 0;
          }
          SectionOffset += CurrentDelta;

          DEBUG ((
            DEBUG_VERBOSE,
            "OCMCO: Dst section %u relocs offset 0x%X count %u delta 0x%X\n",
            Index,
            SectionOffset,
            DstSegment->Sections[Index].NumRelocations,
            CurrentDelta
            ));

          if (!CalculateSizeOnly) {
            DstSegment->Sections[Index].RelocationsOffset = SectionOffset;
            CopyMem (&Destination[CopyFileOffset + CurrentDelta], &Source[CopyFileOffset], RelocationsSize);
          }
        }
      }
    }

    //
    // Copy symbols and string tables.
    //
    Symtab = (MACH_SYMTAB_COMMAND *) (
      MachoGetNextCommand (
        Context,
        MACH_LOAD_COMMAND_SYMTAB,
        NULL
        )
      );
    if (Symtab != NULL) {
      SymbolsOffset = Symtab->SymbolsOffset;
      StringsOffset = Symtab->StringsOffset;

      DEBUG ((
        DEBUG_VERBOSE,
        "OCMCO: Src symtab 0x%X (%u symbols), strings 0x%X (size 0x%X) delta 0x%X\n",
        SymbolsOffset,
        Symtab->NumSymbols,
        StringsOffset,
        Symtab->StringsSize,
        CurrentDelta
        ));

      if (!CalculateSizeOnly) {
        Symtab = (MACH_SYMTAB_COMMAND *) ((UINT8 *) Symtab - Source + Destination);
      }

      //
      // Copy symbol table.
      //
      if (SymbolsOffset != 0) {
        CopyFileOffset = SymbolsOffset;
        SymtabSize     = Symtab->NumSymbols * sizeof (MACH_NLIST_X);
        AlignedOffset  = ALIGN_VALUE (SymbolsOffset + CurrentDelta, sizeof (MACH_NLIST_X));
        CurrentDelta   = AlignedOffset - SymbolsOffset;

        if (MACH_X (OcOverflowTriAddU) (CopyFileOffset, CurrentDelta, SymtabSize, &CurrentSize)
          || (!CalculateSizeOnly && CurrentSize > DestinationSize)) {
          return 0;
        }
        SymbolsOffset += CurrentDelta;

        if (!CalculateSizeOnly) {
          Symtab->SymbolsOffset = SymbolsOffset;
          CopyMem (&Destination[CopyFileOffset + CurrentDelta], &Source[CopyFileOffset], SymtabSize);
        }
      }

      //
      // Copy string table.
      //
      if (StringsOffset != 0) {
        CopyFileOffset = StringsOffset;
        if (MACH_X (OcOverflowTriAddU) (CopyFileOffset, CurrentDelta, Symtab->StringsSize, &CurrentSize)
          || (!CalculateSizeOnly && CurrentSize > DestinationSize)) {
          return 0;
        }
        StringsOffset += CurrentDelta;

        if (!CalculateSizeOnly) {
          Symtab->StringsOffset = StringsOffset;
          CopyMem (&Destination[CopyFileOffset + CurrentDelta], &Source[CopyFileOffset], Symtab->StringsSize);
        }
      }

      DEBUG ((
        DEBUG_VERBOSE,
        "OCMCO: Dst symtab 0x%X (%u symbols), strings 0x%X (size 0x%X) delta 0x%X\n",
        SymbolsOffset,
        Symtab->NumSymbols,
        StringsOffset,
        Symtab->StringsSize,
        CurrentDelta
        ));
    }
  }

  //
  // CurrentSize will only be 0 if there are no valid segments, which is the
  // case for Kernel Resource KEXTs.  In this case, try to use the raw file.
  //
  if (CurrentSize == 0) {
    FileSize = MachoGetFileSize (Context);
    //
    // HeaderSize must be at most as big as the file size by OcMachoLib
    // guarantees. It's sanity-checked to ensure the safety of the subtraction.
    //
    ASSERT (FileSize >= HeaderSize);

    if ((!CalculateSizeOnly && FileSize > DestinationSize)) {
      return 0;
    }

    if (!CalculateSizeOnly) {
      CopyMem (
        Destination + HeaderSize,
        (UINT8 *) Header + HeaderSize,
        FileSize - HeaderSize
        );
    }

    CurrentSize = FileSize;
  }

  if (!CalculateSizeOnly && Strip) {
    InternalStripLoadCommands ((MACH_HEADER_X *) Destination);
  }

  //
  // This cast is safe because CurrentSize is verified against DestinationSize.
  //
  return MACH_X_TO_UINT32 (CurrentSize);
}

BOOLEAN
MACH_X (InternalMachoMergeSegments) (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *Prefix
  )
{
  UINT32                  LcIndex;
  MACH_LOAD_COMMAND       *LoadCommand;
  MACH_SEGMENT_COMMAND_X  *Segment;
  MACH_SEGMENT_COMMAND_X  *FirstSegment;
  MACH_HEADER_X           *Header;
  UINTN                   PrefixLength;
  UINTN                   RemainingArea;
  UINT32                  SkipCount;

  ASSERT (Context != NULL);
  ASSERT (Context->FileSize != 0);
  ASSERT (Prefix != NULL);
  MACH_ASSERT_X (Context);

  Header       = MACH_X (MachoGetMachHeader) (Context);
  PrefixLength = AsciiStrLen (Prefix);
  FirstSegment = NULL;

  SkipCount   = 0;

  LoadCommand = &Header->Commands[0];

  for (LcIndex = 0; LcIndex < Header->NumCommands; ++LcIndex) {
    //
    // Either skip or stop at unrelated commands.
    //
    Segment = (MACH_SEGMENT_COMMAND_X *) (VOID *) LoadCommand;

    if (LoadCommand->CommandType != MACH_LOAD_COMMAND_SEGMENT_X
      || AsciiStrnCmp (Segment->SegmentName, Prefix, PrefixLength) != 0) {
      if (FirstSegment != NULL) {
        break;
      }

      LoadCommand = NEXT_MACH_LOAD_COMMAND (LoadCommand);
      continue;
    }

    //
    // We have a segment starting with the prefix.
    //

    //
    // Do not support this for now as it will require changes in the file.
    //
    if (Segment->Size != Segment->FileSize) {
      return FALSE;
    }

    //
    // Remember the first segment or assume it is a skip.
    //
    if (FirstSegment == NULL) {
      FirstSegment = Segment;
    } else {
      ++SkipCount;

      //
      // Expand the first segment.
      // TODO: Do we need to check these for overflow for our own purposes?
      //
      FirstSegment->Size              = Segment->VirtualAddress - FirstSegment->VirtualAddress + Segment->Size;
      FirstSegment->FileSize          = Segment->FileOffset - FirstSegment->FileOffset + Segment->FileSize;

      //
      // Add new segment protection to the first segment.
      //
      FirstSegment->InitialProtection  |= Segment->InitialProtection;
      FirstSegment->MaximumProtection  |= Segment->MaximumProtection;
    }

    LoadCommand = NEXT_MACH_LOAD_COMMAND (LoadCommand);
  }

  //
  // The segment does not exist.
  //
  if (FirstSegment == NULL) {
    return FALSE;
  }

  //
  // The segment is only one.
  //
  if (SkipCount == 0) {
    return FALSE;
  }

  //
  // Move back remaining commands ontop of the skipped ones and zero this area.
  //
  RemainingArea = Header->CommandsSize - ((UINTN) LoadCommand - (UINTN) &Header->Commands[0]);
  CopyMem (
    (UINT8 *) FirstSegment + FirstSegment->CommandSize,
    LoadCommand,
    RemainingArea
    );
  ZeroMem (LoadCommand, RemainingArea);

  //
  // Account for dropped commands in the header.
  //
  Header->NumCommands  -= SkipCount;
  Header->CommandsSize -= (UINT32) (sizeof (MACH_SEGMENT_COMMAND_X) * SkipCount);

  return TRUE;
}

BOOLEAN
MACH_X (MachoInitializeContext) (
  OUT OC_MACHO_CONTEXT  *Context,
  IN  VOID              *FileData,
  IN  UINT32            FileSize,
  IN  UINT32            ContainerOffset
  )
{
  EFI_STATUS              Status;
  MACH_HEADER_X           *MachHeader;
  UINTN                   TopOfFile;
  UINTN                   TopOfCommands;
  UINT32                  Index;
  CONST MACH_LOAD_COMMAND *Command;
  UINTN                   TopOfCommand;
  UINT32                  CommandsSize;
  BOOLEAN                 Result;

  ASSERT (FileData != NULL);
  ASSERT (FileSize > 0);
  ASSERT (Context != NULL);

  TopOfFile = ((UINTN)FileData + FileSize);
  ASSERT (TopOfFile > (UINTN)FileData);

#ifdef MACHO_LIB_32
  Status = FatFilterArchitecture32 ((UINT8 **) &FileData, &FileSize);
#else
  Status = FatFilterArchitecture64 ((UINT8 **) &FileData, &FileSize);
#endif
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (FileSize < sizeof (*MachHeader)
    || !OC_TYPE_ALIGNED (MACH_HEADER_X, FileData)) {
    return FALSE;
  }
  MachHeader = (MACH_HEADER_X *)FileData;
#ifdef MACHO_LIB_32
  if (MachHeader->Signature != MACH_HEADER_SIGNATURE) {
#else
  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
#endif
    return FALSE;
  }

  Result = OcOverflowAddUN (
    (UINTN)MachHeader->Commands,
    MachHeader->CommandsSize,
    &TopOfCommands
    );
  if (Result || (TopOfCommands > TopOfFile)) {
    return FALSE;
  }

  CommandsSize = 0;

  for (
    Index = 0, Command = MachHeader->Commands;
    Index < MachHeader->NumCommands;
    ++Index, Command = NEXT_MACH_LOAD_COMMAND (Command)
    ) {
    Result = OcOverflowAddUN (
      (UINTN) Command,
      sizeof (*Command),
      &TopOfCommand
      );
    if (Result
     || (TopOfCommand > TopOfCommands)
     || (Command->CommandSize < sizeof (*Command))
     || ((Command->CommandSize % sizeof (MACH_UINT_X)) != 0)
      ) {
      return FALSE;
    }

    Result = OcOverflowAddU32 (
      CommandsSize,
      Command->CommandSize,
      &CommandsSize
      );
    if (Result) {
      return FALSE;
    }
  }

  if (MachHeader->CommandsSize != CommandsSize) {
    return FALSE;
  }
  //
  // Verify assumptions made by this library.
  // Carefully audit all "Assumption:" remarks before modifying these checks.
  //
#ifdef MACHO_LIB_32
  if ((MachHeader->CpuType != MachCpuTypeI386)
#else
  if ((MachHeader->CpuType != MachCpuTypeX8664)
#endif
   || ((MachHeader->FileType != MachHeaderFileTypeKextBundle)
    && (MachHeader->FileType != MachHeaderFileTypeExecute)
    && (MachHeader->FileType != MachHeaderFileTypeFileSet)
#ifdef MACHO_LIB_32
    && (MachHeader->FileType != MachHeaderFileTypeObject))) {
#else
    )) {
#endif
    return FALSE;
  }

  ZeroMem (Context, sizeof (*Context));

  Context->MachHeader      = (MACH_HEADER_ANY*)MachHeader;
  Context->Is32Bit         = MachHeader->CpuType == MachCpuTypeI386;
  Context->FileSize        = FileSize;
  Context->ContainerOffset = ContainerOffset;

  return TRUE;
}

MACH_HEADER_X *
MACH_X (MachoGetMachHeader) (
  IN OUT OC_MACHO_CONTEXT   *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->MachHeader != NULL);
  MACH_ASSERT_X (Context);

  return MACH_X (&Context->MachHeader->Header);
}

MACH_UINT_X
MACH_X (InternalMachoGetLastAddress) (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  MACH_UINT_X                   LastAddress;

  CONST MACH_SEGMENT_COMMAND_X  *Segment;
  MACH_UINT_X                   Address;

  ASSERT (Context != NULL);
  MACH_ASSERT_X (Context);

  LastAddress = 0;

  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    Address = (Segment->VirtualAddress + Segment->Size);

    if (Address > LastAddress) {
      LastAddress = Address;
    }
  }

  return LastAddress;
}

MACH_SEGMENT_COMMAND_X *
MACH_X (MachoGetSegmentByName) (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SegmentName
  )
{
  MACH_SEGMENT_COMMAND_X  *Segment;
  INTN                    Result;

  ASSERT (Context != NULL);
  ASSERT (SegmentName != NULL);
  MACH_ASSERT_X (Context);

  Result = 0;

  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    Result = AsciiStrnCmp (
      Segment->SegmentName,
      SegmentName,
      ARRAY_SIZE (Segment->SegmentName)
      );
    if (Result == 0) {
      return Segment;
    }
  }

  return NULL;
}

MACH_SECTION_X *
MACH_X (MachoGetSectionByName) (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_X   *Segment,
  IN     CONST CHAR8              *SectionName
  )
{
  MACH_SECTION_X  *Section;
  INTN            Result;

  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);
  ASSERT (SectionName != NULL);
  MACH_ASSERT_X (Context);

  for (
    Section = MACH_X (MachoGetNextSection) (Context, Segment, NULL);
    Section != NULL;
    Section = MACH_X (MachoGetNextSection) (Context, Segment, Section)
    ) {
    //
    // Assumption: Mach-O is not of type MH_OBJECT.
    // MH_OBJECT might have sections in segments they do not belong in for
    // performance reasons.  This library does not support intermediate
    // objects.
    //
    Result = AsciiStrnCmp (
      Section->SectionName,
      SectionName,
      ARRAY_SIZE (Section->SectionName)
      );
    if (Result == 0) {
      return Section;
    }
  }

  return NULL;
}

MACH_SECTION_X *
MACH_X (MachoGetSegmentSectionByName) (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SegmentName,
  IN     CONST CHAR8       *SectionName
  )
{
  MACH_SEGMENT_COMMAND_X  *Segment;

  ASSERT (Context != NULL);
  ASSERT (SegmentName != NULL);
  ASSERT (SectionName != NULL);
  MACH_ASSERT_X (Context);

  Segment = MACH_X (MachoGetSegmentByName) (Context, SegmentName);

  if (Segment != NULL) {
    return MACH_X (MachoGetSectionByName) (Context, Segment, SectionName);
  }

  return NULL;
}

MACH_SEGMENT_COMMAND_X *
MACH_X (MachoGetNextSegment) (
  IN OUT OC_MACHO_CONTEXT               *Context,
  IN     CONST MACH_SEGMENT_COMMAND_X   *Segment  OPTIONAL
  )
{
  MACH_SEGMENT_COMMAND_X  *NextSegment;

  CONST MACH_HEADER_X     *MachHeader;
  UINTN                   TopOfCommands;
  BOOLEAN                 Result;
  MACH_UINT_X             TopOfSegment;
  UINTN                   TopOfSections;

  ASSERT (Context != NULL);
  ASSERT (Context->FileSize > 0);
  MACH_ASSERT_X (Context);

  if (Segment != NULL) {
    MachHeader    = MACH_X (MachoGetMachHeader) (Context);
    TopOfCommands = ((UINTN) MachHeader->Commands + MachHeader->CommandsSize);
    ASSERT (
      ((UINTN) Segment >= (UINTN) &MachHeader->Commands[0])
        && ((UINTN) Segment < TopOfCommands)
      );
  }
  //
  // Context initialisation guarantees the command size is a multiple of 8.
  //
  STATIC_ASSERT (
    OC_ALIGNOF (MACH_SEGMENT_COMMAND_X) <= sizeof (UINT64),
    "Alignment is not guaranteed."
    );
  NextSegment = (MACH_SEGMENT_COMMAND_X *) (VOID *) MachoGetNextCommand (
    Context,
    MACH_LOAD_COMMAND_SEGMENT_X,
    (CONST MACH_LOAD_COMMAND *) Segment
    );
  if (NextSegment == NULL || NextSegment->CommandSize < sizeof (*NextSegment)) {
    return NULL;
  }

  Result = OcOverflowMulAddUN (
    NextSegment->NumSections,
    sizeof (*NextSegment->Sections),
    (UINTN) NextSegment->Sections,
    &TopOfSections
    );
  if (Result || (((UINTN) NextSegment + NextSegment->CommandSize) < TopOfSections)) {
    return NULL;
  }

  Result = MACH_X (OcOverflowSubU) (
    NextSegment->FileOffset,
    Context->ContainerOffset,
    &TopOfSegment
    );
  Result |= MACH_X (OcOverflowAddU) (
    TopOfSegment,
    NextSegment->FileSize,
    &TopOfSegment
    );
  if (Result || (TopOfSegment > Context->FileSize)) {
    return NULL;
  }

  if (NextSegment->VirtualAddress + NextSegment->Size < NextSegment->VirtualAddress) {
    return NULL;
  }

  return NextSegment;
}

MACH_SECTION_X *
MACH_X (MachoGetNextSection) (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_X   *Segment,
  IN     MACH_SECTION_X           *Section  OPTIONAL
  )
{
  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);
  MACH_ASSERT_X (Context);

  if (Section != NULL) {
    ASSERT (Section >= Segment->Sections);

    ++Section;

    if (Section >= &Segment->Sections[Segment->NumSections]) {
      return NULL;
    }
  } else if (Segment->NumSections > 0) {
    Section = &Segment->Sections[0];
  } else {
    return NULL;
  }

  if (!InternalSectionIsSane (Context, Section, Segment)) {
    return NULL;
  }

  return Section;
}

MACH_SECTION_X *
MACH_X (MachoGetSectionByIndex) (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Index
  )
{
  MACH_SECTION_X          *Section;

  MACH_SEGMENT_COMMAND_X  *Segment;
  UINT32                  SectionIndex;
  UINT32                  NextSectionIndex;
  BOOLEAN                 Result;

  ASSERT (Context != NULL);
  MACH_ASSERT_X (Context);

  SectionIndex = 0;

  Segment = NULL;
  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    Result = OcOverflowAddU32 (
      SectionIndex,
      Segment->NumSections,
      &NextSectionIndex
      );
    //
    // If NextSectionIndex is wrapping around, Index must be contained.
    //
    if (Result || (Index < NextSectionIndex)) {
      Section = &Segment->Sections[Index - SectionIndex];
      if (!InternalSectionIsSane (Context, Section, Segment)) {
        return NULL;
      }

      return Section;
    }

    SectionIndex = NextSectionIndex;
  }

  return NULL;
}

MACH_SECTION_X *
MACH_X (MachoGetSectionByAddress) (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     MACH_UINT_X       Address
  )
{
  MACH_SEGMENT_COMMAND_X  *Segment;
  MACH_SECTION_X          *Section;
  MACH_UINT_X             TopOfSegment;
  MACH_UINT_X             TopOfSection;

  ASSERT (Context != NULL);
  MACH_ASSERT_X (Context);

  for (
    Segment = MACH_X (MachoGetNextSegment) (Context, NULL);
    Segment != NULL;
    Segment = MACH_X (MachoGetNextSegment) (Context, Segment)
    ) {
    TopOfSegment = (Segment->VirtualAddress + Segment->Size);
    if ((Address >= Segment->VirtualAddress) && (Address < TopOfSegment)) {
      for (
        Section = MACH_X (MachoGetNextSection) (Context, Segment, NULL);
        Section != NULL;
        Section = MACH_X (MachoGetNextSection) (Context, Segment, Section)
        ) {
        TopOfSection = (Section->Address + Section->Size);
        if ((Address >= Section->Address) && (Address < TopOfSection)) {
          return Section;
        }
      }
    }
  }

  return NULL;
}

UINT32
MACH_X (MachoGetSymbolTable) (
  IN OUT OC_MACHO_CONTEXT       *Context,
     OUT CONST   MACH_NLIST_X   **SymbolTable,
     OUT CONST   CHAR8          **StringTable        OPTIONAL,
     OUT CONST   MACH_NLIST_X   **LocalSymbols       OPTIONAL,
     OUT UINT32                 *NumLocalSymbols     OPTIONAL,
     OUT CONST   MACH_NLIST_X   **ExternalSymbols    OPTIONAL,
     OUT UINT32                 *NumExternalSymbols  OPTIONAL,
     OUT CONST   MACH_NLIST_X   **UndefinedSymbols   OPTIONAL,
     OUT UINT32                 *NumUndefinedSymbols OPTIONAL
  )
{
  UINT32              Index;
  CONST MACH_NLIST_X  *SymTab;
  UINT32              NoLocalSymbols;
  UINT32              NoExternalSymbols;
  UINT32              NoUndefinedSymbols;

  ASSERT (Context != NULL);
  ASSERT (SymbolTable != NULL);
  MACH_ASSERT_X (Context);

  if (!InternalRetrieveSymtabs (Context)) {
    return 0;
  }

  if (Context->Symtab->NumSymbols == 0) {
    return 0;
  }

  SymTab = MACH_X (&Context->SymbolTable->Symbol);

  for (Index = 0; Index < Context->Symtab->NumSymbols; ++Index) {
    if (!MACH_X (InternalSymbolIsSane) (Context, &SymTab[Index])) {
      return 0;
    }
  }

  *SymbolTable = MACH_X (&Context->SymbolTable->Symbol);

  if (StringTable != NULL) {
    *StringTable = Context->StringTable;
  }

  NoLocalSymbols     = 0;
  NoExternalSymbols  = 0;
  NoUndefinedSymbols = 0;

  if (Context->DySymtab != NULL) {
    NoLocalSymbols     = Context->DySymtab->NumLocalSymbols;
    NoExternalSymbols  = Context->DySymtab->NumExternalSymbols;
    NoUndefinedSymbols = Context->DySymtab->NumUndefinedSymbols;
  }

  if (NumLocalSymbols != NULL) {
    ASSERT (LocalSymbols != NULL);
    *NumLocalSymbols = NoLocalSymbols;
    if (NoLocalSymbols != 0) {
      *LocalSymbols = &SymTab[Context->DySymtab->LocalSymbolsIndex];
    }
  }

  if (NumExternalSymbols != NULL) {
    ASSERT (ExternalSymbols != NULL);
    *NumExternalSymbols = NoExternalSymbols;
    if (NoExternalSymbols != 0) {
      *ExternalSymbols = &SymTab[Context->DySymtab->ExternalSymbolsIndex];
    }
  }

  if (NumUndefinedSymbols != NULL) {
    ASSERT (UndefinedSymbols != NULL);
    *NumUndefinedSymbols = NoUndefinedSymbols;
    if (NoUndefinedSymbols != 0) {
      *UndefinedSymbols = &SymTab[Context->DySymtab->UndefinedSymbolsIndex];
    }
  }

  return Context->Symtab->NumSymbols;
}

UINT32
MACH_X (MachoGetIndirectSymbolTable) (
  IN OUT OC_MACHO_CONTEXT     *Context,
  OUT    CONST MACH_NLIST_X   **SymbolTable
  )
{
  UINT32 Index;

  ASSERT (Context != NULL);
  ASSERT (SymbolTable != NULL);
  MACH_ASSERT_X (Context);

  if (!InternalRetrieveSymtabs (Context) || Context->DySymtab == NULL) {
    return 0;
  }

  for (Index = 0; Index < Context->DySymtab->NumIndirectSymbols; ++Index) {
    if (
      !MACH_X (InternalSymbolIsSane) (Context, &(MACH_X (&Context->IndirectSymbolTable->Symbol))[Index])
      ) {
      return 0;
    }
  }

  *SymbolTable = MACH_X (&Context->IndirectSymbolTable->Symbol);

  return Context->DySymtab->NumIndirectSymbols;
}
