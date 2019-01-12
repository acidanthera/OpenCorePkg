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
  // The initial checks on the input data are ASSERTs because the caller is
  // expected to submit a valid buffer.
  //
  MachHeader = (MACH_HEADER_64 *)FileData;
  TopOfFile  = ((UINTN)MachHeader + FileSize);

  ASSERT (FileSize >= sizeof (*MachHeader));
  ASSERT (OC_ALIGNED (MachHeader));
  ASSERT (TopOfFile > (UINTN)MachHeader);

  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
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

  @retval 0  The binary is malformed.

**/
UINT64
MachoGetLastAddress64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  UINT64                  LastAddress;

  MACH_SEGMENT_COMMAND_64 *Segment;
  UINT64                  Address;

  ASSERT (Context != NULL);

  LastAddress = 0;

  Segment = NULL;
  while (MachoGetNextSegment64 (Context, &Segment)) {
    if (Segment == NULL) {
      return LastAddress;
    }

    Address = (Segment->VirtualAddress + Segment->Size);

    if (Address > LastAddress) {
      LastAddress = Address;
    }
  }

  return 0;
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
  @param[out]    Uuid     The pointer the UUID command is returned into.
                          NULL if unavailable.  Undefined if FALSE is returned.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetUuid64 (
  IN OUT OC_MACHO_CONTEXT   *Context,
  OUT    MACH_UUID_COMMAND  **Uuid
  )
{
  MACH_UUID_COMMAND *UuidCommand;

  ASSERT (Context != NULL);
  ASSERT (Uuid != NULL);

  UuidCommand = (MACH_UUID_COMMAND *)(
                  InternalGetNextCommand64 (
                    Context,
                    MACH_LOAD_COMMAND_UUID,
                    NULL
                    )
                  );
  if ((UuidCommand != NULL)
   && (!OC_ALIGNED (UuidCommand)
    || (UuidCommand->CommandSize != sizeof (*UuidCommand)))) {
    return FALSE;
  }

  *Uuid = UuidCommand;
  return TRUE;
}

/**
  Retrieves the first segment by the name of SegmentName.

  @param[in,out] Context      Context of the Mach-O.
  @param[in]     SegmentName  Segment name to search for.
  @param[out]    Segment      Pointer the segment is returned in.
                              If FALSE is returned, the output is undefined.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetSegmentByName64 (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     CONST CHAR8              *SegmentName,
  OUT    MACH_SEGMENT_COMMAND_64  **Segment
  )
{
  MACH_SEGMENT_COMMAND_64 *SegmentTemp;
  INTN                    Result;

  ASSERT (Context != NULL);
  ASSERT (SegmentName != NULL);
  ASSERT (Segment != NULL);

  Result = 0;

  SegmentTemp = NULL;
  while (MachoGetNextSegment64 (Context, &SegmentTemp)) {
    if (SegmentTemp != NULL) {
      Result = AsciiStrnCmp (
                 SegmentTemp->SegmentName,
                 SegmentName,
                 ARRAY_SIZE (SegmentTemp->SegmentName)
                 );
      }

    if (Result == 0) {
      *Segment = SegmentTemp;
      return TRUE;
    }
  }

  return FALSE;
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
  @param[out]    Section      Pointer the section is returned in.
                              If FALSE is returned, the output is undefined.

  @return  Whether all inspected sections are sane.

**/
BOOLEAN
MachoGetSectionByName64 (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_64  *Segment,
  IN     CONST CHAR8              *SectionName,
  OUT    MACH_SECTION_64          **Section
  )
{
  INTN            Result;
  MACH_SECTION_64 *SectionTemp;

  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);
  ASSERT (SectionName != NULL);
  ASSERT (Section != NULL);

  SectionTemp = NULL;
  while (MachoGetNextSection64 (Context, Segment, &SectionTemp)) {
    if (SectionTemp != NULL) {
      Result = AsciiStrnCmp (
                 SectionTemp->SectionName,
                 SectionName,
                 ARRAY_SIZE (SectionTemp->SectionName)
                 );
      if (Result != 0) {
        continue;
      }
      //
      // Assumption: Mach-O is not of type MH_OBJECT.
      // MH_OBJECT might have sections in segments they do not belong in for
      // performance reasons.  This library does not support intermediate
      // objects.
      //
      DEBUG_CODE (
        Result = AsciiStrnCmp (
                    SectionTemp->SegmentName,
                    Segment->SegmentName,
                    MIN (
                      ARRAY_SIZE (SectionTemp->SegmentName),
                      ARRAY_SIZE (Segment->SegmentName)
                      )
                    );
        ASSERT (Result == 0);
        );
    }

    *Section = SectionTemp;
    return TRUE;
  }

  return FALSE;
}

/**
  Retrieves a section within a segment by the name of SegmentName.

  @param[in,out] Context      Context of the Mach-O.
  @param[in]     SegmentName  The name of the segment to search in.
  @param[in]     SectionName  The name of the section to search for.
  @param[out]    Section      Pointer the section is returned in.
                              If FALSE is returned, the output is undefined.

  @retval  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetSegmentSectionByName64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SegmentName,
  IN     CONST CHAR8       *SectionName,
  OUT    MACH_SECTION_64   **Section
  )
{
  MACH_SEGMENT_COMMAND_64 *Segment;

  ASSERT (Context != NULL);
  ASSERT (SegmentName != NULL);
  ASSERT (SectionName != NULL);
  ASSERT (Section != NULL);

  if (!MachoGetSegmentByName64 (Context, SegmentName, &Segment)) {
    return FALSE;
  }

  if (Segment == NULL) {
    *Section = NULL;
    return TRUE;
  }

  return MachoGetSectionByName64 (
            Context,
            Segment,
            SectionName,
            Section
            );
}

/**
  Retrieves the next segment.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Segment  On input, the segment to retrieve the successor of.
                          If NULL, the first segment is returned.
                          On output, the following segment is returned.  If no
                          more segment is defined, NULL is returned.
                          If FALSE is returned, the output is undefined.

  @retval  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetNextSegment64 (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_64  **Segment
  )
{
  MACH_SEGMENT_COMMAND_64 *SegmentTemp;
  CONST MACH_HEADER_64    *MachHeader;
  UINTN                   TopOfCommands;
  BOOLEAN                 Result;
  UINT64                  TopOfSegment;
  UINTN                   TopOfSections;

  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);

  ASSERT (Context->MachHeader != NULL);
  ASSERT (Context->FileSize > 0);

  SegmentTemp = *Segment;

  if (SegmentTemp != NULL) {
    MachHeader    = Context->MachHeader;
    TopOfCommands = ((UINTN)MachHeader->Commands + MachHeader->CommandsSize);
    ASSERT (
      ((UINTN)SegmentTemp >= (UINTN)&MachHeader->Commands[0])
        && ((UINTN)SegmentTemp < TopOfCommands)
      );
  }

  SegmentTemp = (MACH_SEGMENT_COMMAND_64 *)(
                  InternalGetNextCommand64 (
                    Context,
                    MACH_LOAD_COMMAND_SEGMENT_64,
                    (MACH_LOAD_COMMAND *)SegmentTemp
                    )
                  );
  if (SegmentTemp != NULL) {
    if (!OC_ALIGNED (SegmentTemp)
     || (SegmentTemp->CommandSize != sizeof (*SegmentTemp))) {
      return FALSE;
    }

    Result = OcOverflowMulAddUN (
               SegmentTemp->NumSections,
               sizeof (*SegmentTemp->Sections),
               (UINTN)SegmentTemp->Sections,
               &TopOfSections
               );
    if (!Result
     || (((UINTN)SegmentTemp + SegmentTemp->CommandSize) != TopOfSections)) {
      return FALSE;
    }

    Result = OcOverflowAddU64 (
               SegmentTemp->FileOffset,
               SegmentTemp->FileSize,
               &TopOfSegment
               );
    if (!Result || (TopOfSegment > Context->FileSize)) {
      return FALSE;
    }
  }

  *Segment = SegmentTemp;
  return TRUE;
}

/**
  Retrieves the next section of a segment.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Segment  The segment to get the section of.
  @param[in]     Section  On input, The section to get the successor of.
                          If NULL, the first section is returned.
                          On output, the successor of the input section.
                          If FALSE is returned, the output is undefined.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetNextSection64 (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_64  *Segment,
  IN     MACH_SECTION_64          **Section
  )
{
  MACH_SECTION_64 *SectionTemp;

  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);
  ASSERT (Section != NULL);

  SectionTemp = *Section;

  if (SectionTemp != NULL) {
    ASSERT (SectionTemp >= Segment->Sections);
    ++SectionTemp;
  } else {
    SectionTemp = &Segment->Sections[0];
  }

  if (SectionTemp >= &Segment->Sections[Segment->NumSections]) {
    *Section = NULL;
    return TRUE;
  }

  if (!InternalSectionIsSane (Context, SectionTemp, Segment)) {
    return FALSE;
  }

  *Section = SectionTemp;
  return TRUE;
}

/**
  Retrieves a section by its index.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Index    Index of the section to retrieve.
  @param[out]    Section  Pointer the section is returned into.
                          If FALSE is returned, the output is undefined.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetSectionByIndex64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Index,
  OUT    MACH_SECTION_64   **Section
  )
{
  MACH_SECTION_64         *SectionTemp;

  MACH_SEGMENT_COMMAND_64 *Segment;
  UINT32                  SectionIndex;
  UINT32                  NextSectionIndex;
  BOOLEAN                 Result;

  ASSERT (Context != NULL);
  ASSERT (Section != NULL);

  SectionIndex = 0;

  Segment = NULL;
  while (MachoGetNextSegment64 (Context, &Segment)) {
    if (Segment == NULL) {
      *Section = NULL;
      return TRUE;
    }

    Result = OcOverflowAddU32 (
               SectionIndex,
               Segment->NumSections,
               &NextSectionIndex
               );
    //
    // If NextSectionIndex is wrapping around, Index must be contained.
    //
    if (!Result || (Index < NextSectionIndex)) {
      SectionTemp = &Segment->Sections[Index - SectionIndex];
      if (!InternalSectionIsSane (Context, SectionTemp, Segment)) {
        break;
      }

      *Section = SectionTemp;
      return TRUE;
    }

    SectionIndex = NextSectionIndex;
  }

  return FALSE;
}

/**
  Retrieves a section by its address.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  Address of the section to retrieve.
  @param[out]    Section  Pointer the section is returned in.
                          If FALSE is returned, the output is undefined.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetSectionByAddress64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address,
  OUT    MACH_SECTION_64   **Section
  )
{
  BOOLEAN                 Result;
  MACH_SEGMENT_COMMAND_64 *Segment;
  MACH_SECTION_64         *SectionWalker;
  UINT64                  TopOfSegment;

  ASSERT (Context != NULL);
  ASSERT (Section != NULL);

  Segment = NULL;
  while (MachoGetNextSegment64 (Context, &Segment)) {
    if (Segment == NULL) {
      *Section = NULL;
      return TRUE;
    }

    TopOfSegment = (Segment->VirtualAddress + Segment->Size);
    if ((Address >= Segment->VirtualAddress) && (Address < TopOfSegment)) {
      Result = FALSE;

      SectionWalker = NULL;
      while (MachoGetNextSection64 (Context, Segment, &SectionWalker)) {
        if (SectionWalker == NULL) {
          Result = TRUE;
          break;
        }

        if ((Address >= SectionWalker->Address)
         && (Address < (SectionWalker->Address + SectionWalker->Size))) {
          *Section = SectionWalker;
          return TRUE;
        }
      }

      if (!Result) {
        break;
      }
    }
  }

  return FALSE;
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
