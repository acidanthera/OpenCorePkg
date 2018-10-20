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
  Initializes a Mach-O Context.

  @param[in]  MachHeader  Header of the Mach-O.
  @param[in]  FileSize    File size of the Mach-O.
  @param[out] Context     Mach-O Context to initialize.

  @return  Whether Context has been initialized successfully.

**/
BOOLEAN
MachoInitializeContext (
  IN  CONST MACH_HEADER_64  *MachHeader,
  IN  UINTN                 FileSize,
  OUT VOID                  *Context
  )
{
  UINTN                   MinCommandsSize;
  UINTN                   TopOfCommands;
  UINTN                   Index;
  CONST MACH_LOAD_COMMAND *Command;
  UINTN                   CommandsSize;
  OC_MACHO_CONTEXT        *MachoContext;

  ASSERT (MachHeader != NULL);
  ASSERT (FileSize > 0);
  ASSERT (Context != NULL);
  //
  // Verify Mach-O Header sanity.
  //
  TopOfCommands   = ((UINTN)MachHeader->Commands + MachHeader->CommandsSize);
  MinCommandsSize = (MachHeader->NumCommands * sizeof (*MachHeader->Commands));
  if ((FileSize < sizeof (*MachHeader))
   || (MachHeader->Signature != MACH_HEADER_64_SIGNATURE)
   || (MachHeader->CommandsSize < MinCommandsSize)
   || (TopOfCommands > ((UINTN)MachHeader + FileSize))) {
    return FALSE;
  }

  CommandsSize = 0;

  for (
    Index = 0, Command = MachHeader->Commands;
    Index < MachHeader->NumCommands;
    ++Index, Command = NEXT_MACH_LOAD_COMMAND (Command)
    ) {
    if (((UINTN)Command >= ((UINTN)MachHeader + MachHeader->CommandsSize))
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
  // Verify assumptions made by this library.
  // Carefully audit all "Assumption:" remarks before modifying these checks.
  //
  if ((MachHeader->CpuType != MachCpuTypeX8664)
   || ((MachHeader->FileType != MachHeaderFileTypeKextBundle)
    && (MachHeader->FileType != MachHeaderFileTypeExecute))) {
    ASSERT (FALSE);
    return FALSE;
  }

  MachoContext = (OC_MACHO_CONTEXT *)Context;
  MachoContext->MachHeader = MachHeader;
  MachoContext->FileSize   = FileSize;

  return TRUE;
}

/**
  Returns the last virtual address of a Mach-O.

  @param[in] Context  Context of the Mach-O.

**/
UINT64
MachoGetLastAddress64 (
  IN CONST VOID  *Context
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

  @param[in] Context          Context of the Mach-O.
  @param[in] LoadCommandType  Type of the Load Command to retrieve.
  @param[in] LoadCommand      Previous Load Command.
                              If NULL, the first match is returned.

  @retval NULL  NULL is returned on failure.

**/
STATIC
MACH_LOAD_COMMAND *
InternalGetNextCommand64 (
  IN CONST VOID               *Context,
  IN MACH_LOAD_COMMAND_TYPE   LoadCommandType,
  IN CONST MACH_LOAD_COMMAND  *LoadCommand  OPTIONAL
  )
{
  CONST MACH_LOAD_COMMAND *Command;
  CONST MACH_HEADER_64    *MachHeader;
  UINTN                   TopOfCommands;

  ASSERT (Context != NULL);

  MachHeader    = ((CONST OC_MACHO_CONTEXT *)Context)->MachHeader;
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

  @param[in] Context  Context of the Mach-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_UUID_COMMAND *
MachoGetUuid64 (
  IN CONST VOID  *Context
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
   && (UuidCommand->CommandSize >= sizeof (*UuidCommand))) {
    return UuidCommand;
  }

  return NULL;
}

/**
  Retrieves the first segment by the name of SegmentName.

  @param[in] Context      Context of the Mach-O.
  @param[in] SegmentName  Segment name to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetSegmentByName64 (
  IN CONST VOID   *Context,
  IN CONST CHAR8  *SegmentName
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

  @param[in] Context      Context of the Mach-O.
  @param[in] Segment      Segment to search in.
  @param[in] SectionName  Section name to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSectionByName64 (
  IN CONST VOID                     *Context,
  IN CONST MACH_SEGMENT_COMMAND_64  *Segment,
  IN CONST CHAR8                    *SectionName
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

  @param[in] Context      Context of the Mach-O.
  @param[in] SegmentName  The name of the segment to search in.
  @param[in] SectionName  The name of the section to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSegmentSectionByName64 (
  IN CONST VOID   *Context,
  IN CONST CHAR8  *SegmentName,
  IN CONST CHAR8  *SectionName
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

  @param[in] Context  Context of the Mach-O.
  @param[in] Segment  Segment to retrieve the successor of.
                      if NULL, the first segment is returned.

  @retal NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetNextSegment64 (
  IN CONST VOID                     *Context,
  IN CONST MACH_SEGMENT_COMMAND_64  *Segment  OPTIONAL
  )
{
  MACH_SEGMENT_COMMAND_64 *NextSegment;

  CONST OC_MACHO_CONTEXT  *MachoContext;
  CONST MACH_HEADER_64    *MachHeader;
  UINTN                   TopOfCommands;
  UINTN                   TopOfSections;

  ASSERT (Context != NULL);

  MachoContext = (CONST OC_MACHO_CONTEXT *)Context;

  if (Segment != NULL) {
    MachHeader    = MachoContext->MachHeader;
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
  if ((NextSegment != NULL)
   && (NextSegment->CommandSize >= sizeof (*NextSegment))) {
    TopOfSections = (UINTN)&NextSegment[NextSegment->NumSections];
    if (((NextSegment->FileOffset + NextSegment->FileSize) > MachoContext->FileSize)
     || (((UINTN)NextSegment + NextSegment->CommandSize) < TopOfSections)) {
      return NULL;
    }
  }

  return NextSegment;
}

/**
  Retrieves the next section of a segment.

  @param[in] Context  Context of the Mach-O.
  @param[in] Segment  The segment to get the section of.
  @param[in] Section  The section to get the successor of.
                      If NULL, the first section is returned.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetNextSection64 (
  IN CONST VOID                     *Context,
  IN CONST MACH_SEGMENT_COMMAND_64  *Segment,
  IN CONST MACH_SECTION_64          *Section  OPTIONAL
  )
{
  CONST OC_MACHO_CONTEXT *MachContext;

  ASSERT (Segment != NULL);

  if (Section != NULL) {
    ASSERT ((Section >= Segment->Sections)
         && (Section < &Segment->Sections[Segment->NumSections])
      );
    ++Section;
  } else {
    Section = (MACH_SECTION_64 *)&Segment->Sections[0];
  }

  MachContext = (CONST OC_MACHO_CONTEXT *)Context;

  if (((UINTN)(Section - Segment->Sections) < Segment->NumSections)
   && ((Section->Offset + Section->Size) <= MachContext->FileSize)) {
    return (MACH_SECTION_64 *)Section;
  }

  return NULL;
}

/**
  Retrieves a section by its index.

  @param[in] Context  Context of the Mach-O.
  @param[in] Index    Index of the section to retrieve.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_SECTION_64 *
MachoGetSectionByIndex64 (
  IN CONST VOID  *Context,
  IN UINTN       Index
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
      return &Segment->Sections[Index - SectionIndex];
    }

    SectionIndex += Segment->NumSections;
  }

  return NULL;
}

/**
  Retrieves a section by its address.

  @param[in] Context  Context of the Mach-O.
  @param[in] Address  Address of the section to retrieve.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_SECTION_64 *
MachoGetSectionByAddress64 (
  IN CONST VOID  *Context,
  IN UINT64      Address
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
          return Section;
        }

        ++Section;
      }
    }
  }

  return NULL;
}

/**
  Retrieves the SYMTAB command.

  @param[in] Context  Context of the Mach-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_SYMTAB_COMMAND *
MachoGetSymtab64 (
  IN CONST VOID  *Context
  )
{
  MACH_SYMTAB_COMMAND *Symtab;
  UINTN               TopOfSymbols;
  UINTN               FileSize;

  ASSERT (Context != NULL);

  Symtab = (MACH_SYMTAB_COMMAND *)(
             InternalGetNextCommand64 (Context, MACH_LOAD_COMMAND_SYMTAB, NULL)
             );
  if ((Symtab != NULL) && (Symtab->CommandSize >= sizeof (*Symtab))) {
    FileSize = ((CONST OC_MACHO_CONTEXT *)Context)->FileSize;

    TopOfSymbols  = Symtab->SymbolsOffset;
    TopOfSymbols += (Symtab->NumSymbols * sizeof (MACH_NLIST_64));

    if ((TopOfSymbols <= FileSize)
     && ((Symtab->StringsOffset + Symtab->StringsSize) <= FileSize)) {
      return Symtab;
    }
  }

  return NULL;
}

/**
  Retrieves the DYSYMTAB command.

  @param[in] Context  Context of the Mach-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_DYSYMTAB_COMMAND *
MachoGetDySymtab64 (
  IN CONST VOID  *Context
  )
{
  MACH_DYSYMTAB_COMMAND *DySymtab;
  UINTN                 FileSize;
  UINTN                 OffsetTop;

  ASSERT (Context != NULL);

  DySymtab = (MACH_DYSYMTAB_COMMAND *)(
               InternalGetNextCommand64 (
                 Context,
                 MACH_LOAD_COMMAND_DYSYMTAB,
                 NULL
                 )
               );
  if (DySymtab != NULL) {
    FileSize = ((CONST OC_MACHO_CONTEXT *)Context)->FileSize;

    OffsetTop  = DySymtab->IndirectSymbolsOffset;
    OffsetTop += (DySymtab->NumIndirectSymbols * sizeof (MACH_NLIST_64));
    if (OffsetTop > FileSize) {
      return NULL;
    }

    OffsetTop  = DySymtab->ExternalRelocationsOffset;
    OffsetTop += (DySymtab->NumExternalRelocations * sizeof (MACH_NLIST_64));
    if (OffsetTop > FileSize) {
      return NULL;
    }

    OffsetTop  = DySymtab->LocalRelocationsOffset;
    OffsetTop += (DySymtab->NumOfLocalRelocations * sizeof (MACH_NLIST_64));
    if (OffsetTop > FileSize) {
      return NULL;
    }
  }

  return DySymtab;
}
