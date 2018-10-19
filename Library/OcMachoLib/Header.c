/**
  Provides services for MACH-O headers.

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

/**
  Initializes a MACH-O Context.

  @param[in]  MachHeader  Header of the MACH-O.
  @param[in]  FileSize    File size of the MACH-O.
  @param[out] Context     MACH-O Context to initialize.

  @return  Whether Context has been initialized successfully.

**/
BOOLEAN
MachoInitializeContext (
  IN  CONST MACH_HEADER_64  *MachHeader,
  IN  UINTN                 FileSize,
  OUT OC_MACHO_CONTEXT      *Context
  )
{
  UINTN                   MinCommandsSize;
  UINTN                   TopOfCommands;
  UINTN                   Index;
  CONST MACH_LOAD_COMMAND *Command;
  UINTN                   CommandsSize;

  ASSERT (MachHeader != NULL);
  ASSERT (FileSize > 0);
  ASSERT (Context != NULL);
  //
  // Verify MACH-O Header sanity.
  //
  TopOfCommands   = ((UINTN)MachHeader->Commands + MachHeader->CommandsSize);
  MinCommandsSize = (MachHeader->NumberOfCommands * sizeof (*MachHeader->Commands));
  if ((FileSize < sizeof (*MachHeader))
   || (MachHeader->Signature != MACH_HEADER_64_SIGNATURE)
   || (MachHeader->CommandsSize < MinCommandsSize)
   || (TopOfCommands > ((UINTN)MachHeader + FileSize))) {
    return FALSE;
  }

  CommandsSize = 0;

  for (
    Index = 0, Command = MachHeader->Commands;
    Index < MachHeader->NumberOfCommands;
    ++Index, Command = NEXT_MACH_LOAD_COMMAND (Command)
    ) {
    if (((UINTN)Command >= ((UINTN)MachHeader + MachHeader->CommandsSize))
     || (Command->Size < sizeof (*Command))
     || ((Command->Size % 8) != 0)  // Assumption: 64-bit, see below.
      ) {
      return FALSE;
    }

    CommandsSize += Command->Size;
  }

  if (MachHeader->CommandsSize != CommandsSize) {
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

  Context->MachHeader = MachHeader;
  Context->FileSize   = FileSize;

  return TRUE;
}

/**
  Returns the last virtual address of a MACH-O.

  @param[in] Context  Context of the MACH-O.

**/
UINT64
MachoGetLastAddress64 (
  IN CONST OC_MACHO_CONTEXT  *Context
  )
{
  UINT64                        LastAddress;

  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  UINT64                        Address;

  ASSERT (Context != NULL);

  LastAddress = 0;

  for (
    Segment = MachoGetFirstSegment64 (Context);
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

  @param[in] Context          Context of the MACH-O.
  @param[in] LoadCommandType  Type of the Load Command to retrieve.
  @param[in] LoadCommand      Previous Load Command.

  @retval NULL  NULL is returned on failure.

**/
STATIC
MACH_LOAD_COMMAND *
InternalGetNextCommand64 (
  IN CONST OC_MACHO_CONTEXT   *Context,
  IN MACH_LOAD_COMMAND_TYPE   LoadCommandType,
  IN CONST MACH_LOAD_COMMAND  *LoadCommand
  )
{
  CONST MACH_LOAD_COMMAND *Command;
  CONST MACH_HEADER_64    *MachHeader;
  UINTN                   TopOfCommands;

  ASSERT (Context != NULL);

  MachHeader    = Context->MachHeader;
  TopOfCommands = ((UINTN)MachHeader->Commands + MachHeader->CommandsSize);

  ASSERT (
    (LoadCommand >= &MachHeader->Commands[0])
      && ((UINTN)LoadCommand <= TopOfCommands)
    );
  
  for (
    Command = NEXT_MACH_LOAD_COMMAND (LoadCommand);
    (UINTN)Command < TopOfCommands;
    Command = NEXT_MACH_LOAD_COMMAND (Command)
    ) {
    if (Command->Type == LoadCommandType) {
      return (MACH_LOAD_COMMAND *)Command;
    }
  }

  return NULL;
}

/**
  Retrieves the first Load Command of type LoadCommandType.

  @param[in] Context          Context of the MACH-O.
  @param[in] LoadCommandType  Type of the Load Command to retrieve.

  @retval NULL  NULL is returned on failure.

**/
STATIC
MACH_LOAD_COMMAND *
InternalGetFirstCommand64 (
  IN CONST OC_MACHO_CONTEXT  *Context,
  IN MACH_LOAD_COMMAND_TYPE  LoadCommandType
  )
{
  CONST MACH_HEADER_64 *MachHeader;

  ASSERT (Context != NULL);

  MachHeader = Context->MachHeader;

  if (MachHeader->NumberOfCommands == 0) {
    return NULL;
  }

  if (MachHeader->Commands[0].Type == LoadCommandType) {
    return (MACH_LOAD_COMMAND *)&MachHeader->Commands[0];
  }

  return InternalGetNextCommand64 (
           Context,
           LoadCommandType,
           &MachHeader->Commands[0]
           );
}

/**
  Retrieves the first UUID Load Command.

  @param[in] Context  Context of the MACH-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_UUID_COMMAND *
MachoGetUuid64 (
  IN CONST OC_MACHO_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return (MACH_UUID_COMMAND *)(
           InternalGetFirstCommand64 (Context, MACH_LOAD_COMMAND_UUID)
           );
}

/**
  Retrieves the first segment by the name of SegmentName.

  @param[in] Context      Context of the MACH-O.
  @param[in] SegmentName  Segment name to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetSegmentByName64 (
  IN CONST OC_MACHO_CONTEXT  *Context,
  IN CONST CHAR8             *SegmentName
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  INTN                          Result;

  ASSERT (Context != NULL);
  ASSERT (SegmentName != NULL);

  for (
    Segment = MachoGetFirstSegment64 (Context);
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

  @param[in] Context      Context of the MACH-O.
  @param[in] Segment      Segment to search in.
  @param[in] SectionName  Section name to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSectionByName64 (
  IN CONST OC_MACHO_CONTEXT         *Context,
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

  for (Index = 0; Index < Segment->NumberOfSections; ++Index) {
    Result = AsciiStrnCmp (
               SectionWalker->SectionName,
               SectionName,
               ARRAY_SIZE (SectionWalker->SegmentName)
               );
    if (Result == 0) {
      //
      // Assumption: MACH-O is not of type MH_OBJECT.
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

  @param[in] Context      Context of the MACH-O.
  @param[in] SegmentName  The name of the segment to search in.
  @param[in] SectionName  The name of the section to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSegmentSectionByName64 (
  IN CONST OC_MACHO_CONTEXT  *Context,
  IN CONST CHAR8             *SegmentName,
  IN CONST CHAR8             *SectionName
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
  Retrieves the first segment.

  @param[in] Context  Context of the MACH-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetFirstSegment64 (
  IN CONST OC_MACHO_CONTEXT  *Context
  )
{
  return (MACH_SEGMENT_COMMAND_64 *)(
           InternalGetFirstCommand64 (Context, MACH_LOAD_COMMAND_SEGMENT_64)
           );
}

/**
  Retrieves the next segment.

  @param[in] Context  Context of the MACH-O.
  @param[in] Segment  Segment to retrieve the successor of.

  @retal NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetNextSegment64 (
  IN CONST OC_MACHO_CONTEXT         *Context,
  IN CONST MACH_SEGMENT_COMMAND_64  *Segment
  )
{
  return (MACH_SEGMENT_COMMAND_64 *)(
           InternalGetNextCommand64 (
             Context,
             MACH_LOAD_COMMAND_SEGMENT_64,
             &Segment->Hdr
             )
           );
}

/**
  Retrieves the first section of a segment.

  @param[in] Segment  The segment to get the section of.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetFirstSection64 (
  IN CONST MACH_SEGMENT_COMMAND_64  *Segment
  )
{
  ASSERT (Segment != NULL);

  if (Segment->NumberOfSections > 0) {
    return (MACH_SECTION_64 *)&Segment->Sections[0];
  }

  return NULL;
}

/**
  Retrieves the next section of a segment.

  @param[in] Segment  The segment to get the section of.
  @param[in] Section  The section to get the successor of.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetNextSection64 (
  IN CONST MACH_SEGMENT_COMMAND_64  *Segment,
  IN CONST MACH_SECTION_64          *Section
  )
{
  ASSERT (Segment != NULL);
  ASSERT (Section != NULL);

  ++Section;

  if ((UINTN)(Section - Segment->Sections) < Segment->NumberOfSections) {
    return (MACH_SECTION_64 *)Section;
  }

  return NULL;
}

/**
  Retrieves a section by its index.

  @param[in] Context  Context of the MACH-O.
  @param[in] Index    Index of the section to retrieve.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_SECTION_64 *
MachoGetSectionByIndex64 (
  IN CONST OC_MACHO_CONTEXT  *Context,
  IN UINTN                   Index
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  UINTN                         SectionIndex;

  ASSERT (Context != NULL);

  SectionIndex = 0;

  for (
    Segment = MachoGetFirstSegment64 (Context);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    if (Index <= (SectionIndex + (Segment->NumberOfSections - 1))) {
      return &Segment->Sections[Index - SectionIndex];
    }

    SectionIndex += Segment->NumberOfSections;
  }

  return NULL;
}

/**
  Retrieves a section by its address.

  @param[in] Context  Context of the MACH-O.
  @param[in] Address  Address of the section to retrieve.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_SECTION_64 *
MachoGetSectionByAddress64 (
  IN CONST OC_MACHO_CONTEXT  *Context,
  IN UINT64                  Address
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  CONST MACH_SECTION_64         *Section;
  UINTN                         Index;

  ASSERT (Context != NULL);

  for (
    Segment = MachoGetFirstSegment64 (Context);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    if ((Address >= Segment->VirtualAddress)
     && (Address < (Segment->VirtualAddress + Address >= Segment->FileSize))) {
      Section = &Segment->Sections[0];

      for (Index = 0; Index < Segment->NumberOfSections; ++Index) {
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

  @param[in] Context  Context of the MACH-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_SYMTAB_COMMAND *
MachoGetSymtab (
  IN CONST OC_MACHO_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  return (MACH_SYMTAB_COMMAND *)(
           InternalGetFirstCommand64 (Context, MACH_LOAD_COMMAND_SYMTAB)
           );
}

/**
  Retrieves the DYSYMTAB command.

  @param[in] Context  Context of the MACH-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_DYSYMTAB_COMMAND *
MachoGetDySymtab (
  IN CONST OC_MACHO_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  return (MACH_DYSYMTAB_COMMAND *)(
           InternalGetFirstCommand64 (Context, MACH_LOAD_COMMAND_DYSYMTAB)
           );
}
