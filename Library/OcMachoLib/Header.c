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

/**
  Returns the last virtual address of a MACH-O.

  @param[in] MachHeader  Header of the MACH-O.

**/
UINT64
MachoGetLastAddress64 (
  IN CONST MACH_HEADER_64  *MachHeader
  )
{
  UINT64                LastAddress;

  MACH_LOAD_COMMAND_PTR Command;
  UINTN                 Index;
  UINT64                Address;

  ASSERT (MachHeader != NULL);
 
  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
    return 0;
  }

  LastAddress = 0;

  Command.Hdr = &MachHeader->Commands[0];

  for (Index = 0; Index < MachHeader->NumberOfCommands; ++Index) {
    if (Command.Hdr->Type == MACH_LOAD_COMMAND_SEGMENT_64) {
      Address = (Command.Segment64->VirtualAddress + Command.Hdr->Size);

      if (Address > LastAddress) {
        LastAddress = Address;
      }
    }

    Command.Hdr = NEXT_MACH_LOAD_COMMAND (Command.Hdr);
  }

  return LastAddress;
}

/**
  Retrieves the first Load Command of type LoadCommandType.

  @param[in] MachHeader       Header of the MACH-O.
  @param[in] LoadCommandType  Type of the Load Command to retrieve.
  @param[in] LoadCommand      Previous Load Command.

  @retval NULL  NULL is returned on failure.

**/
MACH_LOAD_COMMAND *
MachoGetNextCommand64 (
  IN CONST MACH_HEADER_64     *MachHeader,
  IN MACH_LOAD_COMMAND_TYPE   LoadCommandType,
  IN CONST MACH_LOAD_COMMAND  *LoadCommand
  )
{
  CONST MACH_LOAD_COMMAND *CommandsWalker;
  UINTN                   Index;

  ASSERT (MachHeader != NULL);
  //
  // LoadCommand being past the MachHeader Load Commands is implicitly caught
  // by the for-loop.
  //
  if ((MachHeader->Signature != MACH_HEADER_64_SIGNATURE)
   || (LoadCommand < MachHeader->Commands)) {
    return NULL;
  }

  CommandsWalker = LoadCommand;

  for (
    Index = ((UINTN)(LoadCommand - MachHeader->Commands) + 1);
    Index < MachHeader->NumberOfCommands;
    ++Index
    ) {
    if (CommandsWalker->Type == LoadCommandType) {
      return (MACH_LOAD_COMMAND *)CommandsWalker;
    }

    CommandsWalker = NEXT_MACH_LOAD_COMMAND (CommandsWalker);
  }

  return NULL;
}

/**
  Retrieves the first Load Command of type LoadCommandType.

  @param[in] MachHeader       Header of the MACH-O.
  @param[in] LoadCommandType  Type of the Load Command to retrieve.

  @retval NULL  NULL is returned on failure.

**/
MACH_LOAD_COMMAND *
MachoGetFirstCommand64 (
  IN CONST MACH_HEADER_64    *MachHeader,
  IN MACH_LOAD_COMMAND_TYPE  LoadCommandType
  )
{
  return MachoGetNextCommand64 (
           MachHeader,
           LoadCommandType,
           &MachHeader->Commands[0]
           );
}

/**
  Retrieves the first UUID Load Command.

  @param[in] MachHeader  Header of the MACH-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_UUID_COMMAND *
MachoGetUuid64 (
  IN CONST MACH_HEADER_64  *MachHeader
  )
{
  ASSERT (MachHeader != NULL);
  
  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
    return NULL;
  }

  return (MACH_UUID_COMMAND *)(
           MachoGetFirstCommand64 (MachHeader, MACH_LOAD_COMMAND_UUID)
           );
}

/**
  Retrieves the first segment by the name of SegmentName.

  @param[in] MachHeader   Header of the MACH-O.
  @param[in] SegmentName  Segment name to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetSegmentByName64 (
  IN CONST MACH_HEADER_64  *MachHeader,
  IN CONST CHAR8           *SegmentName
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *SegmentWalker;
  UINTN                         Index;
  INTN                          Result;

  ASSERT (MachHeader != NULL);
  ASSERT (SegmentName != NULL);

  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
    return NULL;
  }

  SegmentWalker = (MACH_SEGMENT_COMMAND_64 *)&MachHeader->Commands[0];

  for (Index = 0; Index < MachHeader->NumberOfCommands; ++Index) {
    if (SegmentWalker->Hdr.Type == MACH_LOAD_COMMAND_SEGMENT_64) {
      Result = AsciiStrnCmp (
                 SegmentWalker->SegmentName,
                 SegmentName,
                 ARRAY_SIZE (SegmentWalker->SegmentName)
                 );
      if (Result == 0) {
        return (MACH_SEGMENT_COMMAND_64 *)SegmentWalker;
      }
    }

    SegmentWalker = NEXT_MACH_SEGMENT_64 (SegmentWalker);
  }

  return NULL;
}

/**
  Retrieves the first section by the name of SectionName.

  @param[in] MachHeader   Header of the MACH-O.
  @param[in] Segment      Segment to search in.
  @param[in] SectionName  Section name to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSectionByName64 (
  IN CONST MACH_HEADER_64           *MachHeader,
  IN CONST MACH_SEGMENT_COMMAND_64  *Segment,
  IN CONST CHAR8                    *SectionName
  )
{
  CONST MACH_SECTION_64 *SectionWalker;
  UINTN                 Index;
  INTN                  Result;

  ASSERT (MachHeader != NULL);
  ASSERT (Segment != NULL);
  ASSERT (SectionName != NULL);

  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
    return NULL;
  }
  //
  // MH_OBJECT might have sections in segments they do not belong in for
  // performance reasons.  We do not support intermediate objects.
  //
  ASSERT (MachHeader->FileType != MachHeaderFileTypeObject);

  SectionWalker = (MACH_SECTION_64 *)&Segment->Sections[0];

  for (Index = 0; Index < Segment->NumberOfSections; ++Index) {
    Result = AsciiStrnCmp (
               SectionWalker->SectionName,
               SectionName,
               ARRAY_SIZE (SectionWalker->SegmentName)
               );

    if (Result == 0) {
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

  @param[in] MachHeader   Header of the MACH-O.
  @param[in] SegmentName  The name of the segment to search in.
  @param[in] SectionName  The name of the section to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_SECTION_64 *
MachoGetSegmentSectionByName64 (
  IN CONST MACH_HEADER_64  *MachHeader,
  IN CONST CHAR8           *SegmentName,
  IN CONST CHAR8           *SectionName
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;

  ASSERT (MachHeader != NULL);
  ASSERT (SegmentName != NULL);
  ASSERT (SectionName != NULL);

  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
    return NULL;
  }

  Segment = MachoGetSegmentByName64 (MachHeader, SegmentName);

  if (Segment != NULL) {
    return MachoGetSectionByName64 (MachHeader, Segment, SectionName);
  }

  return NULL;
}

/**
  Retrieves the first segment.

  @param[in] MachHeader  Header of the MACH-O.

  @retval NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetFirstSegment64 (
  IN CONST MACH_HEADER_64  *MachHeader
  )
{
  return (MACH_SEGMENT_COMMAND_64 *)(
           MachoGetFirstCommand64 (MachHeader, MACH_LOAD_COMMAND_SEGMENT_64)
           );
}

/**
  Retrieves the next segment.

  @param[in] MachHeader  Header of the MACH-O.
  @param[in] Segment     Segment to retrieve the successor of.

  @retal NULL  NULL is returned on failure.

**/
MACH_SEGMENT_COMMAND_64 *
MachoGetNextSegment64 (
  IN CONST MACH_HEADER_64           *MachHeader,
  IN CONST MACH_SEGMENT_COMMAND_64  *Segment
  )
{
  return (MACH_SEGMENT_COMMAND_64 *)(
           MachoGetNextCommand64 (
             MachHeader,
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
  MACH_SECTION_64 *FirstSection;

  ASSERT (Segment != NULL);
  ASSERT (Section != NULL);

  DEBUG_CODE (
    FirstSection = MachoGetFirstSection64 (Segment);
    ASSERT (
      ((UINTN)(Section - FirstSection) + 1) < Segment->NumberOfSections
      );
    );

  return (MACH_SECTION_64 *)(Section + 1);
}

/**
  Retrieves a section by its index.

  @param[in] MachHeader  Header of the MACH-O.
  @param[in] Index       Index of the section to retrieve.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_SECTION_64 *
MachoGetSectionByIndex64 (
  IN CONST MACH_HEADER_64  *MachHeader,
  IN UINTN                 Index
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  UINTN                         SectionIndex;

  ASSERT (MachHeader != NULL);

  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
    return NULL;
  }

  SectionIndex = 0;

  for (
    Segment = MachoGetFirstSegment64 (MachHeader);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (MachHeader, Segment)
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

  @param[in] MachHeader  Header of the MACH-O.
  @param[in] Address     Address of the section to retrieve.

  @retval NULL  NULL is returned on failure.

**/
CONST MACH_SECTION_64 *
MachoGetSectionByAddress64 (
  IN CONST MACH_HEADER_64  *MachHeader,
  IN UINT64                Address
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  CONST MACH_SECTION_64         *Section;
  UINTN                         Index;

  ASSERT (MachHeader != NULL);

  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
    return NULL;
  }

  for (
    Segment = MachoGetFirstSegment64 (MachHeader);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (MachHeader, Segment)
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
