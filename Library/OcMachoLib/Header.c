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
#include <Library/OcGuardLib.h>
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

  return Context->MachHeader;
}

/**
  Returns the Mach-O's file size.

  @param[in,out] Context  Context of the Mach-O.

**/
UINT32
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
  IN  VOID              *FileData,
  IN  UINT32            FileSize
  )
{
  MACH_HEADER_64          *MachHeader;
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
  //
  // Verify Mach-O Header sanity.
  //
  MachHeader = (MACH_HEADER_64 *)FileData;

  Result = OcOverflowAddUN (
             (UINTN)MachHeader,
             FileSize,
             &TopOfFile
             );
  if (!Result
   || (FileSize < sizeof (*MachHeader))
   || !OC_ALIGNED (MachHeader)
   || (MachHeader->Signature != MACH_HEADER_64_SIGNATURE)
    ) {
    return FALSE;
  }

  Result = OcOverflowAddUN (
             (UINTN)MachHeader->Commands,
             MachHeader->CommandsSize,
             &TopOfCommands
             );
  if (!Result || (TopOfCommands > TopOfFile)) {
    return FALSE;
  }

  CommandsSize = 0;

  for (
    Index = 0, Command = MachHeader->Commands;
    Index < MachHeader->NumCommands;
    ++Index, Command = NEXT_MACH_LOAD_COMMAND (Command)
    ) {
    Result = OcOverflowAddUN (
               (UINTN)Command,
               sizeof (*Command),
               &TopOfCommand
               );
    if (!Result
     || (TopOfCommand > TopOfCommands)
     || (Command->CommandSize < sizeof (*Command))
     || ((Command->CommandSize % sizeof (UINT64)) != 0)  // Assumption: 64-bit, see below.
      ) {
      return FALSE;
    }

    Result = OcOverflowAddU32 (
               CommandsSize,
               Command->CommandSize,
               &CommandsSize
               );
    if (!Result) {
      return FALSE;
    }
  }

  if (MachHeader->CommandsSize != CommandsSize) {
    ASSERT (FALSE);
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
CONST MACH_LOAD_COMMAND *
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
      return Command;
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
   && OC_ALIGNED (UuidCommand)
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
  MACH_SEGMENT_COMMAND_64 *Segment;
  INTN                    Result;

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
      return Segment;
    }
  }

  return NULL;
}

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
  IN     CONST MACH_SECTION_64          *Section,
  IN     CONST MACH_SEGMENT_COMMAND_64  *Segment
  )
{
  UINT32  FileSize;
  UINT32  TopOffset;
  UINT64  TopOfSegment;
  BOOLEAN Result;
  UINT64  TopOfSection;

  ASSERT (Context != NULL);
  ASSERT (Section != NULL);
  ASSERT (Segment != NULL);
  //
  // Section->Alignment is stored as a power of 2.
  //
  if (Section->Alignment > 31) {
    return FALSE;
  }

  TopOfSegment = (Segment->VirtualAddress + Segment->Size);
  Result       = OcOverflowAddU64 (
                   Section->Address,
                   Section->Size,
                   &TopOfSection
                   );
  if (!Result || (TopOfSection > TopOfSegment)) {
    return FALSE;
  }

  FileSize = MachoGetFileSize (Context);
  Result   = OcOverflowAddU32 (
                Section->Offset,
                Section->Size,
                &TopOffset
                );
  if (!Result || (TopOffset > FileSize)) {
    return FALSE;
  }

  if (Section->NumRelocations != 0) {
    Result = OcOverflowMulAddU32 (
               Section->NumRelocations,
               sizeof (MACH_RELOCATION_INFO),
               Section->RelocationsOffset,
               &TopOffset
               );
    if (!Result || (TopOffset > FileSize)) {
      return FALSE;
    }
  }

  return TRUE;
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
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_64  *Segment,
  IN     CONST CHAR8              *SectionName
  )
{
  MACH_SECTION_64 *Section;
  INTN            Result;

  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);
  ASSERT (SectionName != NULL);

  for (
    Section = MachoGetNextSection64 (Context, Segment, NULL);
    Section != NULL;
    Section = MachoGetNextSection64 (Context, Segment, Section)
    ) {
    Result = AsciiStrnCmp (
               Section->SectionName,
               SectionName,
               ARRAY_SIZE (Section->SectionName)
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
                   Section->SegmentName,
                   Segment->SegmentName,
                   MIN (
                     ARRAY_SIZE (Section->SegmentName),
                     ARRAY_SIZE (Segment->SegmentName)
                     )
                   );
        ASSERT (Result == 0);
        );

      return Section;
    }
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
  MACH_SEGMENT_COMMAND_64 *Segment;

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
  BOOLEAN                 Result;
  UINT64                  TopOfSegment;
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
                    (MACH_LOAD_COMMAND *)Segment
                    )
                  );
  if ((NextSegment == NULL)
   || !OC_ALIGNED (NextSegment)
   || (NextSegment->CommandSize < sizeof (*NextSegment))) {
    return NULL;
  }

  Result = OcOverflowMulAddUN (
             NextSegment->NumSections,
             sizeof (*NextSegment->Sections),
             (UINTN)NextSegment->Sections,
             &TopOfSections
             );
  if (!Result
   || (((UINTN)NextSegment + NextSegment->CommandSize) < TopOfSections)) {
    return NULL;
  }

  Result = OcOverflowAddU32 (
             NextSegment->FileOffset,
             NextSegment->FileSize,
             &TopOfSegment
             );
  if (!Result || (TopOfSegment > Context->FileSize)) {
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
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_64  *Segment,
  IN     MACH_SECTION_64          *Section  OPTIONAL
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
    Section = &Segment->Sections[0];
  }

  if (!InternalSectionIsSane (Context, Section, Segment)) {
    return NULL;
  }

  return Section;
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
  IN     UINT32            Index
  )
{
  MACH_SECTION_64         *Section;

  MACH_SEGMENT_COMMAND_64 *Segment;
  UINT32                  SectionIndex;
  UINT32                  NextSectionIndex;
  BOOLEAN                 Result;

  ASSERT (Context != NULL);

  SectionIndex = 0;

  for (
    Segment = MachoGetNextSegment64 (Context, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    Result = OcOverflowAddU32 (
               SectionIndex,
               Segment->NumSections,
               &NextSectionIndex
               );
    //
    // If NextSectionIndex is overflowing, Index must be contained.
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
  MACH_SEGMENT_COMMAND_64 *Segment;
  MACH_SECTION_64         *Section;
  UINT64                  TopOfSegment;
  UINT64                  TopOfSection;

  ASSERT (Context != NULL);

  for (
    Segment = MachoGetNextSegment64 (Context, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    TopOfSegment = (Segment->VirtualAddress + Segment->Size);
    if ((Address >= Segment->VirtualAddress) && (Address < TopOfSegment)) {
      for (
        Section = MachoGetNextSection64 (Context, Segment, NULL);
        Section != NULL;
        Section = MachoGetNextSection64 (Context, Segment, Section)
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
  UINTN                 MachoAddress;
  MACH_SYMTAB_COMMAND   *Symtab;
  MACH_DYSYMTAB_COMMAND *DySymtab;
  CHAR8                 *StringTable;
  UINT32                FileSize;
  UINT32                OffsetTop;
  BOOLEAN               Result;

  MACH_NLIST_64         *SymbolTable;
  MACH_NLIST_64         *IndirectSymtab;
  MACH_RELOCATION_INFO  *LocalRelocations;
  MACH_RELOCATION_INFO  *ExternRelocations;

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
               Context,
               MACH_LOAD_COMMAND_SYMTAB,
               NULL
               )
             );
  if ((Symtab == NULL)
   || !OC_ALIGNED (Symtab)
   || (Symtab->CommandSize != sizeof (*Symtab))) {
    return FALSE;
  }

  FileSize = Context->FileSize;

  Result = OcOverflowMulAddU32 (
             Symtab->NumSymbols,
             sizeof (MACH_NLIST_64),
             Symtab->SymbolsOffset,
             &OffsetTop
             );
  if (!Result || (OffsetTop > FileSize)) {
    return FALSE;
  }

  Result = OcOverflowAddU32 (
             Symtab->StringsOffset,
             Symtab->StringsSize,
             &OffsetTop
             );
  if (!Result || (OffsetTop > FileSize)) {
    return FALSE;
  }

  MachoAddress = (UINTN)Context->MachHeader;
  StringTable  = (CHAR8 *)(MachoAddress + Symtab->StringsOffset);

  if (StringTable[(Symtab->StringsSize / sizeof (*StringTable)) - 1] != '\0') {
    return FALSE;
  }

  SymbolTable = (MACH_NLIST_64 *)(MachoAddress + Symtab->SymbolsOffset);
  if (!OC_ALIGNED (SymbolTable)) {
    return FALSE;
  }
  //
  // Retrieve DYSYMTAB.
  //
  DySymtab = (MACH_DYSYMTAB_COMMAND *)(
               InternalGetNextCommand64 (
                 Context,
                 MACH_LOAD_COMMAND_DYSYMTAB,
                 NULL
                 )
               );
  if ((DySymtab == NULL)
   || !OC_ALIGNED (DySymtab) 
   || (DySymtab->CommandSize != sizeof (*DySymtab))) {
    return FALSE;
  }

  Result = OcOverflowAddU32 (
             DySymtab->LocalSymbolsIndex,
             DySymtab->NumLocalSymbols,
             &OffsetTop
             );
  if (!Result || (OffsetTop > Symtab->NumSymbols)) {
    return FALSE;
  }

  Result = OcOverflowAddU32 (
             DySymtab->ExternalSymbolsIndex,
             DySymtab->NumExternalSymbols,
             &OffsetTop
             );
  if (!Result || (OffsetTop > Symtab->NumSymbols)) {
    return FALSE;
  }

  Result = OcOverflowAddU32 (
             DySymtab->UndefinedSymbolsIndex,
             DySymtab->NumUndefinedSymbols,
             &OffsetTop
             );
  if (!Result || (OffsetTop > Symtab->NumSymbols)) {
    return FALSE;
  }

  Result = OcOverflowMulAddU32 (
             DySymtab->NumIndirectSymbols,
             sizeof (MACH_NLIST_64),
             DySymtab->IndirectSymbolsOffset,
             &OffsetTop
             );
  if (!Result || (OffsetTop > FileSize)) {
    return FALSE;
  }

  Result = OcOverflowMulAddU32 (
             DySymtab->NumOfLocalRelocations,
             sizeof (MACH_RELOCATION_INFO),
             DySymtab->LocalRelocationsOffset,
             &OffsetTop
             );
  if (!Result || (OffsetTop > FileSize)) {
    return FALSE;
  }

  Result = OcOverflowMulAddU32 (
             DySymtab->NumExternalRelocations,
             sizeof (MACH_RELOCATION_INFO),
             DySymtab->ExternalRelocationsOffset,
             &OffsetTop
             );
  if (!Result || (OffsetTop > FileSize)) {
    return FALSE;
  }

  IndirectSymtab = (MACH_NLIST_64 *)(
                     MachoAddress + DySymtab->IndirectSymbolsOffset
                     );
  LocalRelocations = (MACH_RELOCATION_INFO *)(
                       MachoAddress + DySymtab->LocalRelocationsOffset
                       );
  ExternRelocations = (MACH_RELOCATION_INFO *)(
                        MachoAddress + DySymtab->ExternalRelocationsOffset
                        );
  if (!OC_ALIGNED (IndirectSymtab)
   || !OC_ALIGNED (LocalRelocations)
   || !OC_ALIGNED (ExternRelocations)) {
    return FALSE;
  }
  //
  // Store the symbol information.
  //
  Context->Symtab      = Symtab;
  Context->SymbolTable = SymbolTable;
  Context->StringTable = StringTable;

  Context->IndirectSymbolTable = IndirectSymtab;
  Context->LocalRelocations    = LocalRelocations;
  Context->ExternRelocations   = ExternRelocations;

  return TRUE;
}
