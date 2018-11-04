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

#include <Base.h>

#include <IndustryStandard/AppleMachoImage.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/OcMachoLib.h>

#include "OcMachoLibInternal.h"

/**
  Returns the size of a Mach-O Context.

**/
UINTN
MachoGetContextSize (
  VOID
  )
{
  return sizeof (OC_MACHO_CONTEXT);
}

/**
  Returns the Mach-O Header structure.

  @param[in,out] Context  Context of the Mach-O.

**/
MACH_HEADER_64 *
MachoGetMachHeader64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->MachHeader != NULL);

  return (MACH_HEADER_64 *)Context->MachHeader;
}

/**
  Returns the Mach-O's file size.

  @param[in,out] Context  Context of the Mach-O.

**/
UINTN
MachoGetFileSize (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->FileSize != 0);

  return Context->FileSize;
}

/**
  Initializes a Mach-O Context.

  @param[out] Context   Mach-O Context to initialize.
  @param[in]  FileData  Pointer to the file's data.
  @param[in]  FileSize  File size of FIleData.

  @return  Whether Context has been initialized successfully.

**/
BOOLEAN
MachoInitializeContext (
  OUT OC_MACHO_CONTEXT  *Context,
  IN  CONST VOID        *FileData,
  IN  UINTN             FileSize
  )
{
  CONST MACH_HEADER_64    *MachHeader;
  UINTN                   TopOfCommands;
  UINTN                   Index;
  CONST MACH_LOAD_COMMAND *Command;
  UINTN                   CommandsSize;

  ASSERT (FileData != NULL);
  ASSERT (FileSize > 0);
  ASSERT (Context != NULL);

  MachHeader = (MACH_HEADER_64 *)FileData;
  //
  // Verify Mach-O Header sanity.
  //
  TopOfCommands = ((UINTN)MachHeader->Commands + MachHeader->CommandsSize);
  if ((FileSize < sizeof (*MachHeader))
   || (MachHeader->Signature != MACH_HEADER_64_SIGNATURE)
   || (TopOfCommands > ((UINTN)MachHeader + FileSize))) {
    return FALSE;
  }

  CommandsSize = 0;

  for (
    Index = 0, Command = MachHeader->Commands;
    Index < MachHeader->NumCommands;
    ++Index, Command = NEXT_MACH_LOAD_COMMAND (Command)
    ) {
    if ((((UINTN)Command + sizeof (MACH_LOAD_COMMAND)) > ((UINTN)MachHeader + MachHeader->CommandsSize))
     || (Command->CommandSize < sizeof (*Command))
     || ((Command->CommandSize % 8) != 0)  // Assumption: 64-bit, see below.
      ) {
      return FALSE;
    }

    CommandsSize += Command->CommandSize;
  }

  if (MachHeader->CommandsSize < CommandsSize) {
    return FALSE;
  }
  //
  // The Load Command retrieval code uses size instead of indices to speed up
  // finding LCs after a provided one.  Verify padding does not include a full
  // LC for the loop might incorrectly reference it as a valid LC.
  //
  ASSERT (
    (MachHeader->CommandsSize - CommandsSize) < sizeof (MACH_LOAD_COMMAND)
    );
  //
  // Verify assumptions made by this library.
  // Carefully audit all "Assumption:" remarks before modifying these checks.
  //
  if ((MachHeader->CpuType != MachCpuTypeX8664)
   || ((MachHeader->FileType != MachHeaderFileTypeKextBundle)
    && (MachHeader->FileType != MachHeaderFileTypeExecute))) {
    ASSERT (FALSE);
    return FALSE;
  }

  Context->MachHeader = MachHeader;
  Context->FileSize   = FileSize;

  return TRUE;
}

/**
  Returns the last virtual address of a Mach-O.

  @param[in,out] Context  Context of the Mach-O.

**/
UINT64
MachoGetLastAddress64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  UINT64                        LastAddress;

  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  UINT64                        Address;

  ASSERT (Context != NULL);

  LastAddress = 0;

  for (
    Segment = MachoGetNextSegment64 (Context, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    Address = (Segment->VirtualAddress + Segment->Size);

    if (Address > LastAddress) {
      LastAddress = Address;
    }
  }

  return LastAddress;
}

/**
  Retrieves the first Load Command of type LoadCommandType.

  @param[in,out] Context          Context of the Mach-O.
  @param[in]     LoadCommandType  Type of the Load Command to retrieve.
  @param[in]     LoadCommand      Previous Load Command.
                                  If NULL, the first match is returned.

  @retval NULL  NULL is returned on failure.

**/
STATIC
MACH_LOAD_COMMAND *
InternalGetNextCommand64 (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_LOAD_COMMAND_TYPE   LoadCommandType,
  IN     CONST MACH_LOAD_COMMAND  *LoadCommand  OPTIONAL
  )
{
  CONST MACH_LOAD_COMMAND *Command;
  CONST MACH_HEADER_64    *MachHeader;
  UINTN                   TopOfCommands;

  ASSERT (Context != NULL);

  MachHeader = Context->MachHeader;
  ASSERT (MachHeader != NULL);

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
      return (MACH_LOAD_COMMAND *)Command;
    }
  }

  return NULL;
}

/**
  Retrieves the first UUID Load Command.

  @param[in,out] Context  Context of the Mach-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_UUID_COMMAND *
MachoGetUuid64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  MACH_UUID_COMMAND *UuidCommand;

  ASSERT (Context != NULL);

  UuidCommand = (MACH_UUID_COMMAND *)(
                  InternalGetNextCommand64 (
                    Context,
                    MACH_LOAD_COMMAND_UUID,
                    NULL
                    )
                  );
  if ((UuidCommand != NULL)
   && (UuidCommand->CommandSize == sizeof (*UuidCommand))) {
    return UuidCommand;
  }

  return NULL;
}

/**
  Retrieves the first segment by the name of SegmentName.

  @param[in,out] Context      Context of the Mach-O.
  @param[in]     SegmentName  Segment name to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetSegmentByName64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SegmentName
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  INTN                          Result;

  ASSERT (Context != NULL);
  ASSERT (SegmentName != NULL);

  for (
    Segment = MachoGetNextSegment64 (Context, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    Result = AsciiStrnCmp (
                Segment->SegmentName,
                SegmentName,
                ARRAY_SIZE (Segment->SegmentName)
                );
    if (Result == 0) {
      return (MACH_SEGMENT_COMMAND_64 *)Segment;
    }
  }

  return NULL;
}

/**
  Retrieves the first section by the name of SectionName.

  @param[in,out] Context      Context of the Mach-O.
  @param[in]     Segment      Segment to search in.
  @param[in]     SectionName  Section name to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSectionByName64 (
  IN OUT OC_MACHO_CONTEXT               *Context,
  IN     CONST MACH_SEGMENT_COMMAND_64  *Segment,
  IN     CONST CHAR8                    *SectionName
  )
{
  CONST MACH_SECTION_64 *SectionWalker;
  UINTN                 Index;
  INTN                  Result;

  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);
  ASSERT (SectionName != NULL);

  SectionWalker = &Segment->Sections[0];

  for (Index = 0; Index < Segment->NumSections; ++Index) {
    Result = AsciiStrnCmp (
               SectionWalker->SectionName,
               SectionName,
               ARRAY_SIZE (SectionWalker->SectionName)
               );
    if (Result == 0) {
      //
      // Assumption: Mach-O is not of type MH_OBJECT.
      // MH_OBJECT might have sections in segments they do not belong in for
      // performance reasons.  This library does not support intermediate
      // objects.
      //
      DEBUG_CODE (
        Result = AsciiStrnCmp (
                   SectionWalker->SegmentName,
                   Segment->SegmentName,
                   MIN (
                     ARRAY_SIZE (SectionWalker->SegmentName),
                     ARRAY_SIZE (Segment->SegmentName)
                     )
                   );
        ASSERT (Result == 0);
        );

      return (MACH_SECTION_64 *)SectionWalker;
    }

    ++SectionWalker;
  }

  return NULL;
}

/**
  Retrieves a section within a segment by the name of SegmentName.

  @param[in,out] Context      Context of the Mach-O.
  @param[in]     SegmentName  The name of the segment to search in.
  @param[in]     SectionName  The name of the section to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSegmentSectionByName64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SegmentName,
  IN     CONST CHAR8       *SectionName
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;

  ASSERT (Context != NULL);
  ASSERT (SegmentName != NULL);
  ASSERT (SectionName != NULL);

  Segment = MachoGetSegmentByName64 (Context, SegmentName);

  if (Segment != NULL) {
    return MachoGetSectionByName64 (Context, Segment, SectionName);
  }

  return NULL;
}

/**
  Retrieves the next segment.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Segment  Segment to retrieve the successor of.
                          if NULL, the first segment is returned.

  @retal NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetNextSegment64 (
  IN OUT OC_MACHO_CONTEXT               *Context,
  IN     CONST MACH_SEGMENT_COMMAND_64  *Segment  OPTIONAL
  )
{
  MACH_SEGMENT_COMMAND_64 *NextSegment;

  CONST MACH_HEADER_64    *MachHeader;
  UINTN                   TopOfCommands;
  UINTN                   TopOfSections;

  ASSERT (Context != NULL);

  ASSERT (Context->MachHeader != NULL);
  ASSERT (Context->FileSize > 0);

  if (Segment != NULL) {
    MachHeader    = Context->MachHeader;
    TopOfCommands = ((UINTN)MachHeader->Commands + MachHeader->CommandsSize);
    ASSERT (
      ((UINTN)Segment >= (UINTN)&MachHeader->Commands[0])
        && ((UINTN)Segment < TopOfCommands)
      );
  }

  NextSegment = (MACH_SEGMENT_COMMAND_64 *)(
                  InternalGetNextCommand64 (
                    Context,
                    MACH_LOAD_COMMAND_SEGMENT_64,
                    (CONST MACH_LOAD_COMMAND *)Segment
                    )
                  );
  if ((NextSegment == NULL)
   || (NextSegment->CommandSize < sizeof (*NextSegment))) {
    return NULL;
  }

  TopOfSections = (UINTN)&NextSegment[NextSegment->NumSections];
  if (((NextSegment->FileOffset + NextSegment->FileSize) > Context->FileSize)
    || (((UINTN)NextSegment + NextSegment->CommandSize) < TopOfSections)) {
    return NULL;
  }

  return NextSegment;
}

/**
  Retrieves the next section of a segment.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Segment  The segment to get the section of.
  @param[in]     Section  The section to get the successor of.
                          If NULL, the first section is returned.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetNextSection64 (
  IN OUT OC_MACHO_CONTEXT               *Context,
  IN     CONST MACH_SEGMENT_COMMAND_64  *Segment,
  IN     CONST MACH_SECTION_64          *Section  OPTIONAL
  )
{
  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);

  if (Section != NULL) {
    ASSERT ((Section >= Segment->Sections)
         && (Section < &Segment->Sections[Segment->NumSections])
      );
    ++Section;
  } else {
    Section = (MACH_SECTION_64 *)&Segment->Sections[0];
  }

  ASSERT (Context->FileSize > 0);

  if (((UINTN)(Section - Segment->Sections) < Segment->NumSections)
   && ((Section->Offset + Section->Size) <= Context->FileSize)) {
    return (MACH_SECTION_64 *)Section;
  }

  return NULL;
}

/**
  Retrieves a section by its index.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Index    Index of the section to retrieve.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSectionByIndex64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINTN             Index
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  UINTN                         SectionIndex;

  ASSERT (Context != NULL);

  SectionIndex = 0;

  for (
    Segment = MachoGetNextSegment64 (Context, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    if (Index <= (SectionIndex + (Segment->NumSections - 1))) {
      return (MACH_SECTION_64 *)&Segment->Sections[Index - SectionIndex];
    }

    SectionIndex += Segment->NumSections;
  }

  return NULL;
}

/**
  Retrieves a section by its address.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  Address of the section to retrieve.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSectionByAddress64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  CONST MACH_SECTION_64         *Section;
  UINTN                         Index;

  ASSERT (Context != NULL);

  for (
    Segment = MachoGetNextSegment64 (Context, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    if ((Address >= Segment->VirtualAddress)
     && (Address < (Segment->VirtualAddress + Segment->Size))) {
      Section = &Segment->Sections[0];

      for (Index = 0; Index < Segment->NumSections; ++Index) {
        if ((Address >= Section->Address)
         && (Address < Section->Address + Section->Size)) {
          return (MACH_SECTION_64 *)Section;
        }

        ++Section;
      }
    }
  }

  return NULL;
}

/**
  Retrieves the SYMTAB command.

  @param[in,out] Context  Context of the Mach-O.

  @retval NULL  NULL is returned on failure.

**/
BOOLEAN
InternalRetrieveSymtabs64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  UINTN                       MachoAddress;
  CONST MACH_SYMTAB_COMMAND   *Symtab;
  CONST MACH_DYSYMTAB_COMMAND *DySymtab;
  CONST CHAR8                 *StringTable;
  UINTN                       FileSize;
  UINTN                       OffsetTop;
  UINTN                       TopOfSymbols;

  ASSERT (Context != NULL);
  ASSERT (Context->MachHeader != NULL);
  ASSERT (Context->FileSize > 0);
  ASSERT (Context->SymbolTable == NULL);

  if (Context->SymbolTable != NULL) {
    return TRUE;
  }
  //
  // Retrieve SYMTAB.
  //
  Symtab = (MACH_SYMTAB_COMMAND *)(
             InternalGetNextCommand64 (
               (VOID *)Context,
               MACH_LOAD_COMMAND_SYMTAB,
               NULL
               )
             );
  if ((Symtab == NULL) || (Symtab->CommandSize < sizeof (*Symtab))) {
    return FALSE;
  }

  FileSize = Context->FileSize;

  TopOfSymbols  = Symtab->SymbolsOffset;
  TopOfSymbols += (Symtab->NumSymbols * sizeof (MACH_NLIST_64));

  if ((TopOfSymbols > FileSize)
   || ((Symtab->StringsOffset + Symtab->StringsSize) > FileSize)) {
    return FALSE;
  }

  MachoAddress = (UINTN)Context->MachHeader;
  StringTable  = (CONST CHAR8 *)(MachoAddress + Symtab->StringsOffset);

  if (StringTable[(Symtab->StringsSize / sizeof (*StringTable)) - 1] != '\0') {
    return FALSE;
  }
  //
  // Retrieve DYSYMTAB.
  //
  DySymtab = (MACH_DYSYMTAB_COMMAND *)(
               InternalGetNextCommand64 (
                 (VOID *)Context,
                 MACH_LOAD_COMMAND_DYSYMTAB,
                 NULL
                 )
               );
  if ((DySymtab == NULL) || (DySymtab->CommandSize < sizeof (*DySymtab))) {
    return FALSE;
  }

  OffsetTop  = DySymtab->IndirectSymbolsOffset;
  OffsetTop += (DySymtab->NumIndirectSymbols * sizeof (MACH_NLIST_64));
  if (OffsetTop > FileSize) {
    return FALSE;
  }

  OffsetTop  = DySymtab->LocalRelocationsOffset;
  OffsetTop += (DySymtab->NumOfLocalRelocations * sizeof (MACH_NLIST_64));
  if (OffsetTop > FileSize) {
    return FALSE;
  }

  OffsetTop  = DySymtab->ExternalRelocationsOffset;
  OffsetTop += (DySymtab->NumExternalRelocations * sizeof (MACH_NLIST_64));
  if (OffsetTop > FileSize) {
    return FALSE;
  }
  //
  // Store the symbol information.
  //
  Context->NumSymbols  = Symtab->NumSymbols;
  Context->SymbolTable = (CONST MACH_NLIST_64 *)(
                                MachoAddress + Symtab->SymbolsOffset
                                );
  Context->StringsSize = Symtab->StringsSize;
  Context->StringTable = StringTable;

  Context->IndirectSymbolTable = (CONST MACH_NLIST_64 *)(
                                        MachoAddress
                                          + DySymtab->IndirectSymbolsOffset
                                        );
  Context->LocalRelocations = (CONST MACH_RELOCATION_INFO *)(
                                     MachoAddress
                                       + DySymtab->LocalRelocationsOffset
                                     );
  Context->ExternRelocations = (CONST MACH_RELOCATION_INFO *)(
                                      MachoAddress
                                        + DySymtab->ExternalRelocationsOffset
                                      );

  return TRUE;
}
