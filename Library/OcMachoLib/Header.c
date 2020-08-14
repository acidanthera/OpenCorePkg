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

#include <Uefi.h>

#include <IndustryStandard/AppleMachoImage.h>
#include <IndustryStandard/AppleFatBinaryImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>

#include "OcMachoLibInternal.h"

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
  Returns the Mach-O's virtual address space size.

  @param[out] Context   Context of the Mach-O.

**/
UINT32
MachoGetVmSize64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  UINT64                   VmSize;
  MACH_SEGMENT_COMMAND_64  *Segment;

  ASSERT (Context != NULL);
  ASSERT (Context->FileSize != 0);

  VmSize = 0;

  for (
    Segment = MachoGetNextSegment64 (Context, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    if (OcOverflowAddU64 (VmSize, Segment->Size, &VmSize)) {
      return 0;
    }
    VmSize = MACHO_ALIGN (VmSize);
  }

  if (VmSize > MAX_UINT32) {
    return 0;
  }

  return (UINT32) VmSize;
}

/**
  Initializes a Mach-O Context.

  @param[out] Context   Mach-O Context to initialize.
  @param[in]  FileData  Pointer to the file's data.
  @param[in]  FileSize  File size of FileData.

  @return  Whether Context has been initialized successfully.
**/
BOOLEAN
MachoInitializeContext (
  OUT OC_MACHO_CONTEXT  *Context,
  IN  VOID              *FileData,
  IN  UINT32            FileSize,
  IN  UINT32            ContainerOffset
  )
{
  EFI_STATUS              Status;
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

  TopOfFile = ((UINTN)FileData + FileSize);
  ASSERT (TopOfFile > (UINTN)FileData);

  Status = FatFilterArchitecture64 ((UINT8 **) &FileData, &FileSize);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (FileSize < sizeof (*MachHeader)
    || !OC_TYPE_ALIGNED (MACH_HEADER_64, FileData)) {
    return FALSE;
  }
  MachHeader = (MACH_HEADER_64 *)FileData;
  if (MachHeader->Signature != MACH_HEADER_64_SIGNATURE) {
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
               (UINTN)Command,
               sizeof (*Command),
               &TopOfCommand
               );
    if (Result
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
  if ((MachHeader->CpuType != MachCpuTypeX8664)
   || ((MachHeader->FileType != MachHeaderFileTypeKextBundle)
    && (MachHeader->FileType != MachHeaderFileTypeExecute)
    && (MachHeader->FileType != MachHeaderFileTypeFileSet))) {
    return FALSE;
  }

  ZeroMem (Context, sizeof (*Context));

  Context->MachHeader      = MachHeader;
  Context->FileSize        = FileSize;
  Context->ContainerOffset = ContainerOffset;

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

MACH_LOAD_COMMAND *
MachoGetNextCommand64 (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_LOAD_COMMAND_TYPE   LoadCommandType,
  IN     CONST MACH_LOAD_COMMAND  *LoadCommand  OPTIONAL
  )
{
  MACH_LOAD_COMMAND       *Command;
  MACH_HEADER_64          *MachHeader;
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
  //
  // Context initialisation guarantees the command size is a multiple of 8.
  //
  STATIC_ASSERT (
    OC_ALIGNOF (MACH_UUID_COMMAND) <= sizeof (UINT64),
    "Alignment is not guaranteed."
    );

  UuidCommand = (MACH_UUID_COMMAND *) (VOID *) MachoGetNextCommand64 (
    Context,
    MACH_LOAD_COMMAND_UUID,
    NULL
    );
  if (UuidCommand == NULL || UuidCommand->CommandSize != sizeof (*UuidCommand)) {
    return NULL;
  }
  return UuidCommand;
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

  Result = 0;

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
  UINT64  TopOffset64;
  UINT32  TopOffset32;
  UINT64  TopOfSegment;
  BOOLEAN Result;
  UINT64  TopOfSection;

  ASSERT (Context != NULL);
  ASSERT (Section != NULL);
  ASSERT (Segment != NULL);
  //
  // Section->Alignment is stored as a power of 2.
  //
  if ((Section->Alignment > 31)
   || ((Section->Offset != 0) && (Section->Offset < Segment->FileOffset))) {
    return FALSE;
  }

  TopOfSegment = (Segment->VirtualAddress + Segment->Size);
  Result       = OcOverflowAddU64 (
                   Section->Address,
                   Section->Size,
                   &TopOfSection
                   );
  if (Result || (TopOfSection > TopOfSegment)) {
    return FALSE;
  }

  Result   = OcOverflowAddU64 (
                Section->Offset,
                Section->Size,
                &TopOffset64
                );
  if (Result || (TopOffset64 > (Segment->FileOffset + Segment->FileSize))) {
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
    OC_ALIGNOF (MACH_SEGMENT_COMMAND_64) <= sizeof (UINT64),
    "Alignment is not guaranteed."
    );
  NextSegment = (MACH_SEGMENT_COMMAND_64 *) (VOID *) MachoGetNextCommand64 (
    Context,
    MACH_LOAD_COMMAND_SEGMENT_64,
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

  Result = OcOverflowSubU64 (
             NextSegment->FileOffset,
             Context->ContainerOffset,
             &TopOfSegment
             );
  Result |= OcOverflowAddU64 (
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

  Segment = NULL;
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
  Initialises the symbol information of Context.

  @param[in,out] Context   Context of the Mach-O.
  @param[in]     Symtab    The SYMTAB command to initialise with.
  @param[in]     DySymtab  The DYSYMTAB command to initialise with.

  @returns  Whether the operation was successful.

**/
STATIC
BOOLEAN
InternalInitialiseSymtabs64 (
  IN OUT OC_MACHO_CONTEXT       *Context,
  IN     MACH_SYMTAB_COMMAND    *Symtab,
  IN     MACH_DYSYMTAB_COMMAND  *DySymtab
  )
{
  UINTN                 MachoAddress;
  CHAR8                 *StringTable;
  UINT32                FileSize;
  UINT32                SymbolsOffset;
  UINT32                StringsOffset;
  UINT32                OffsetTop;
  BOOLEAN               Result;

  UINT32                IndirectSymbolsOffset;
  UINT32                LocalRelocationsOffset;
  UINT32                ExternalRelocationsOffset;
  MACH_NLIST_64         *SymbolTable;
  MACH_NLIST_64         *IndirectSymtab;
  MACH_RELOCATION_INFO  *LocalRelocations;
  MACH_RELOCATION_INFO  *ExternRelocations;

  VOID                  *Tmp;

  ASSERT (Context != NULL);
  ASSERT (Context->MachHeader != NULL);
  ASSERT (Context->FileSize > 0);
  ASSERT (Context->SymbolTable == NULL);

  FileSize = Context->FileSize;

  Result = OcOverflowSubU32 (
             Symtab->SymbolsOffset,
             Context->ContainerOffset,
             &SymbolsOffset
             );
  Result |= OcOverflowMulAddU32 (
              Symtab->NumSymbols,
              sizeof (MACH_NLIST_64),
              SymbolsOffset,
              &OffsetTop
              );
  if (Result || (OffsetTop > FileSize)) {
    return FALSE;
  }

  Result = OcOverflowSubU32 (
             Symtab->StringsOffset,
             Context->ContainerOffset,
             &StringsOffset
             );
  Result |= OcOverflowAddU32 (
              StringsOffset,
              Symtab->StringsSize,
              &OffsetTop
              );
  if (Result || (OffsetTop > FileSize)) {
    return FALSE;
  }

  MachoAddress = (UINTN)Context->MachHeader;
  StringTable  = (CHAR8 *)(MachoAddress + StringsOffset);

  if (Symtab->StringsSize == 0 || StringTable[Symtab->StringsSize - 1] != '\0') {
    return FALSE;
  }

  Tmp = (VOID *)(MachoAddress + SymbolsOffset);
  if (!OC_TYPE_ALIGNED (MACH_NLIST_64, Tmp)) {
    return FALSE;
  }
  SymbolTable = (MACH_NLIST_64 *)Tmp;

  IndirectSymtab    = NULL;
  LocalRelocations  = NULL;
  ExternRelocations = NULL;

  if (DySymtab != NULL) {
    Result = OcOverflowAddU32 (
               DySymtab->LocalSymbolsIndex,
               DySymtab->NumLocalSymbols,
               &OffsetTop
               );
    if (Result || (OffsetTop > Symtab->NumSymbols)) {
      return FALSE;
    }

    Result = OcOverflowAddU32 (
               DySymtab->ExternalSymbolsIndex,
               DySymtab->NumExternalSymbols,
               &OffsetTop
               );
    if (Result || (OffsetTop > Symtab->NumSymbols)) {
      return FALSE;
    }

    Result = OcOverflowAddU32 (
               DySymtab->UndefinedSymbolsIndex,
               DySymtab->NumUndefinedSymbols,
               &OffsetTop
               );
    if (Result || (OffsetTop > Symtab->NumSymbols)) {
      return FALSE;
    }

    //
    // We additionally check for offset validity here, as KC kexts have some garbage
    // in their DySymtab, but it is "valid" for symbols.
    //
    if (DySymtab->NumIndirectSymbols > 0 && DySymtab->IndirectSymbolsOffset != 0) {
      Result = OcOverflowSubU32 (
                 DySymtab->IndirectSymbolsOffset,
                 Context->ContainerOffset,
                 &IndirectSymbolsOffset
                 );
      Result |= OcOverflowMulAddU32 (
                  DySymtab->NumIndirectSymbols,
                  sizeof (MACH_NLIST_64),
                  IndirectSymbolsOffset,
                  &OffsetTop
                  );
      if (Result || (OffsetTop > FileSize)) {
        return FALSE;
      }

      Tmp = (VOID *)(MachoAddress + IndirectSymbolsOffset);
      if (!OC_TYPE_ALIGNED (MACH_NLIST_64, Tmp)) {
        return FALSE;
      }
      IndirectSymtab = (MACH_NLIST_64 *)Tmp;
    }

    if (DySymtab->NumOfLocalRelocations > 0 && DySymtab->LocalRelocationsOffset != 0) {
      Result = OcOverflowSubU32 (
                 DySymtab->LocalRelocationsOffset,
                 Context->ContainerOffset,
                 &LocalRelocationsOffset
                 );
      Result |= OcOverflowMulAddU32 (
                  DySymtab->NumOfLocalRelocations,
                  sizeof (MACH_RELOCATION_INFO),
                  LocalRelocationsOffset,
                  &OffsetTop
                  );
      if (Result || (OffsetTop > FileSize)) {
        return FALSE;
      }

      Tmp = (VOID *)(MachoAddress + LocalRelocationsOffset);
      if (!OC_TYPE_ALIGNED (MACH_RELOCATION_INFO, Tmp)) {
        return FALSE;
      }
      LocalRelocations = (MACH_RELOCATION_INFO *)Tmp;
    }

    if (DySymtab->NumExternalRelocations > 0 && DySymtab->ExternalRelocationsOffset != 0) {
      Result = OcOverflowSubU32 (
                 DySymtab->ExternalRelocationsOffset,
                 Context->ContainerOffset,
                 &ExternalRelocationsOffset
                 );
      Result |= OcOverflowMulAddU32 (
                  DySymtab->NumExternalRelocations,
                  sizeof (MACH_RELOCATION_INFO),
                  ExternalRelocationsOffset,
                  &OffsetTop
                  );
      if (Result || (OffsetTop > FileSize)) {
        return FALSE;
      }

      Tmp = (VOID *)(MachoAddress + ExternalRelocationsOffset);
      if (!OC_TYPE_ALIGNED (MACH_RELOCATION_INFO, Tmp)) {
        return FALSE;
      }
      ExternRelocations = (MACH_RELOCATION_INFO *)Tmp;
    }
  }

  //
  // Store the symbol information.
  //
  Context->Symtab      = Symtab;
  Context->SymbolTable = SymbolTable;
  Context->StringTable = StringTable;
  Context->DySymtab    = DySymtab;

  Context->IndirectSymbolTable = IndirectSymtab;
  Context->LocalRelocations    = LocalRelocations;
  Context->ExternRelocations   = ExternRelocations;

  return TRUE;
}

BOOLEAN
MachoInitialiseSymtabsExternal64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     OC_MACHO_CONTEXT  *SymsContext
  )
{
  MACH_SYMTAB_COMMAND   *Symtab;
  MACH_DYSYMTAB_COMMAND *DySymtab;
  BOOLEAN               IsDyld;

  if (Context->SymbolTable != NULL) {
    return TRUE;
  }
  //
  // We cannot use SymsContext's symbol tables if Context is flagged for DYLD
  // and SymsContext is not.
  //
  IsDyld = (Context->MachHeader->Flags & MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK) != 0;
  if (IsDyld
   && (SymsContext->MachHeader->Flags & MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK) == 0) {
    return FALSE;
  }

  //
  // Context initialisation guarantees the command size is a multiple of 8.
  //
  STATIC_ASSERT (
    OC_ALIGNOF (MACH_SYMTAB_COMMAND) <= sizeof (UINT64),
    "Alignment is not guaranteed."
    );
  //
  // Retrieve SYMTAB.
  //
  Symtab = (MACH_SYMTAB_COMMAND *) (VOID *) MachoGetNextCommand64 (
    SymsContext,
    MACH_LOAD_COMMAND_SYMTAB,
    NULL
    );
  if (Symtab == NULL || Symtab->CommandSize != sizeof (*Symtab)) {
    return FALSE;
  }

  DySymtab = NULL;

  if (IsDyld) {
    //
    // Context initialisation guarantees the command size is a multiple of 8.
    //
    STATIC_ASSERT (
      OC_ALIGNOF (MACH_DYSYMTAB_COMMAND) <= sizeof (UINT64),
      "Alignment is not guaranteed."
      );
    //
    // Retrieve DYSYMTAB.
    //
    DySymtab = (MACH_DYSYMTAB_COMMAND *) (VOID *) MachoGetNextCommand64 (
      SymsContext,
      MACH_LOAD_COMMAND_DYSYMTAB,
      NULL
      );
    if (DySymtab == NULL || DySymtab->CommandSize != sizeof (*DySymtab)) {
      return FALSE;
    }
  }

  return InternalInitialiseSymtabs64 (Context, Symtab, DySymtab);
}

BOOLEAN
InternalRetrieveSymtabs64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  //
  // Retrieve the symbol information for Context from itself.
  //
  return MachoInitialiseSymtabsExternal64 (Context, Context);
}

UINT32
MachoGetSymbolTable (
  IN OUT OC_MACHO_CONTEXT     *Context,
     OUT CONST MACH_NLIST_64  **SymbolTable,
     OUT CONST CHAR8          **StringTable OPTIONAL,
     OUT CONST MACH_NLIST_64  **LocalSymbols, OPTIONAL
     OUT UINT32               *NumLocalSymbols, OPTIONAL
     OUT CONST MACH_NLIST_64  **ExternalSymbols, OPTIONAL
     OUT UINT32               *NumExternalSymbols, OPTIONAL
     OUT CONST MACH_NLIST_64  **UndefinedSymbols, OPTIONAL
     OUT UINT32               *NumUndefinedSymbols OPTIONAL
  )
{
  UINT32              Index;
  CONST MACH_NLIST_64 *SymTab;
  UINT32              NoLocalSymbols;
  UINT32              NoExternalSymbols;
  UINT32              NoUndefinedSymbols;

  ASSERT (Context != NULL);

  if (!InternalRetrieveSymtabs64 (Context)) {
    return 0;
  }

  if (Context->Symtab->NumSymbols == 0) {
    return 0;
  }

  SymTab = Context->SymbolTable;

  for (Index = 0; Index < Context->Symtab->NumSymbols; ++Index) {
    if (!InternalSymbolIsSane (Context, &SymTab[Index])) {
      return 0;
    }
  }

  *SymbolTable = Context->SymbolTable;

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
MachoGetIndirectSymbolTable (
  IN OUT OC_MACHO_CONTEXT     *Context,
  OUT    CONST MACH_NLIST_64  **SymbolTable
  )
{
  UINT32 Index;

  if (!InternalRetrieveSymtabs64 (Context)) {
    return 0;
  }

  for (Index = 0; Index < Context->DySymtab->NumIndirectSymbols; ++Index) {
    if (
      !InternalSymbolIsSane (Context, &Context->IndirectSymbolTable[Index])
      ) {
      return 0;
    }
  }

  *SymbolTable = Context->IndirectSymbolTable;

  return Context->DySymtab->NumIndirectSymbols;
}

/**
  Returns a pointer to the Mach-O file at the specified virtual address.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  Virtual address to look up.    
  @param[out]    MaxSize  Maximum data safely available from FileOffset.
                          If NULL is returned, the output is undefined.

**/
VOID *
MachoGetFilePointerByAddress64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address,
  OUT    UINT32            *MaxSize OPTIONAL
  )
{
  CONST MACH_SEGMENT_COMMAND_64 *Segment;
  UINT64                        Offset;

  Segment = NULL;
  while ((Segment = MachoGetNextSegment64 (Context, Segment)) != NULL) {
    if ((Address >= Segment->VirtualAddress)
     && (Address < Segment->VirtualAddress + Segment->Size)) {
      Offset = (Address - Segment->VirtualAddress);

      if (MaxSize != NULL) {
        *MaxSize = (UINT32)(Segment->Size - Offset);
      }

      Offset += Segment->FileOffset - Context->ContainerOffset;
      return (VOID *)((UINTN)Context->MachHeader + (UINTN)Offset);
    }
  }

  return NULL;
}

/**
  Strip superfluous Load Commands from the Mach-O header.  This includes the
  Code Signature Load Command which must be removed for the binary has been
  modified by the prelinking routines.

  @param[in,out] MachHeader  Mach-O header to strip the Load Commands from.

**/
STATIC
VOID
InternalStripLoadCommands64 (
  IN OUT MACH_HEADER_64  *MachHeader
  )
{
  STATIC CONST MACH_LOAD_COMMAND_TYPE LoadCommandsToStrip[] = {
    MACH_LOAD_COMMAND_CODE_SIGNATURE,
    MACH_LOAD_COMMAND_DYLD_INFO,
    MACH_LOAD_COMMAND_DYLD_INFO_ONLY,
    MACH_LOAD_COMMAND_FUNCTION_STARTS,
    MACH_LOAD_COMMAND_DATA_IN_CODE,
    MACH_LOAD_COMMAND_DYLIB_CODE_SIGN_DRS
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
    //
    // Assertion: LC_UNIXTHREAD and LC_MAIN are technically stripped in KXLD,
    //            but they are not supposed to be present in the first place.
    //
    if ((LoadCommand->CommandType == MACH_LOAD_COMMAND_UNIX_THREAD)
     || (LoadCommand->CommandType == MACH_LOAD_COMMAND_MAIN)) {
      DEBUG ((DEBUG_WARN, "OCMCO: UNIX Thread and Main LCs are unsupported\n"));
    }

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
MachoExpandImage64 (
  IN  OC_MACHO_CONTEXT   *Context,
  OUT UINT8              *Destination,
  IN  UINT32             DestinationSize,
  IN  BOOLEAN            Strip
  )
{
  MACH_HEADER_64           *Header;
  UINT8                    *Source;
  UINT32                   HeaderSize;
  UINT64                   CopyFileOffset;
  UINT64                   CopyFileSize;
  UINT64                   CopyVmSize;
  UINT32                   CurrentDelta;
  UINT32                   OriginalDelta;
  UINT64                   CurrentSize;
  UINT32                   FileSize;
  MACH_SEGMENT_COMMAND_64  *Segment;
  MACH_SEGMENT_COMMAND_64  *FirstSegment;
  MACH_SEGMENT_COMMAND_64  *DstSegment;
  MACH_SYMTAB_COMMAND      *Symtab;
  MACH_DYSYMTAB_COMMAND    *DySymtab;
  UINT32                   Index;

  ASSERT (Context != NULL);
  ASSERT (Context->FileSize != 0);

  //
  // Header is valid, copy it first.
  //
  Header     = MachoGetMachHeader64 (Context);
  Source     = (UINT8 *) Header;
  HeaderSize = sizeof (*Header) + Header->CommandsSize;
  if (HeaderSize > DestinationSize) {
    return 0;
  }
  CopyMem (Destination, Header, HeaderSize);

  CurrentDelta = 0;
  FirstSegment = NULL;
  CurrentSize  = 0;
  for (
    Segment = MachoGetNextSegment64 (Context, NULL);
    Segment != NULL;
    Segment = MachoGetNextSegment64 (Context, Segment)
    ) {
    //
    // Align delta by x86 page size, this is what our lib expects.
    //
    OriginalDelta = CurrentDelta;
    CurrentDelta  = MACHO_ALIGN (CurrentDelta);
    if (Segment->FileSize > Segment->Size) {
      return 0;
    }

    if (FirstSegment == NULL) {
      FirstSegment = Segment;
    }

    //
    // Do not overwrite header.
    //
    CopyFileOffset = Segment->FileOffset - Context->ContainerOffset;
    CopyFileSize   = Segment->FileSize;
    CopyVmSize     = Segment->Size;
    if (CopyFileOffset <= HeaderSize) {
      CopyFileOffset = HeaderSize;
      CopyFileSize   = Segment->FileSize - CopyFileOffset;
      CopyVmSize     = Segment->Size - CopyFileOffset;
      if (CopyFileSize > Segment->FileSize || CopyVmSize > Segment->Size) {
        //
        // Header must fit in 1 segment.
        //
        return 0;
      }
    }
    //
    // Ensure that it still fits. In legit files segments are ordered.
    // We do not care for other (the file will be truncated).
    //
    if (OcOverflowTriAddU64 (CopyFileOffset, CurrentDelta, CopyVmSize, &CurrentSize)
      || CurrentSize > DestinationSize) {
      return 0;
    }

    //
    // Copy and zero fill file data. We can do this because only last sections can have 0 file size.
    //
    ASSERT (CopyFileSize <= MAX_UINTN && CopyVmSize <= MAX_UINTN);
    ZeroMem (&Destination[CopyFileOffset + OriginalDelta], CurrentDelta - OriginalDelta);
    CopyMem (&Destination[CopyFileOffset + CurrentDelta], &Source[CopyFileOffset], (UINTN)CopyFileSize);
    ZeroMem (&Destination[CopyFileOffset + CurrentDelta + CopyFileSize], (UINTN)(CopyVmSize - CopyFileSize));
    //
    // Refresh destination segment size and offsets.
    //
    DstSegment = (MACH_SEGMENT_COMMAND_64 *) ((UINT8 *) Segment - Source + Destination);
    DstSegment->FileOffset += CurrentDelta;
    DstSegment->FileSize    = DstSegment->Size;

    if (DstSegment->VirtualAddress - (DstSegment->FileOffset - Context->ContainerOffset) != FirstSegment->VirtualAddress) {
      return 0;
    }

    //
    // We need to update fields in SYMTAB and DYSYMTAB. Tables have to be present before 0 FileSize
    // sections as they have data, so we update them before parsing sections. 
    // Note: There is an assumption they are in __LINKEDIT segment, another option is to check addresses.
    //
    if (AsciiStrnCmp (DstSegment->SegmentName, "__LINKEDIT", ARRAY_SIZE (DstSegment->SegmentName)) == 0) {
      Symtab = (MACH_SYMTAB_COMMAND *)(
                 MachoGetNextCommand64 (
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

      DySymtab = (MACH_DYSYMTAB_COMMAND *)(
                     MachoGetNextCommand64 (
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
    //
    // These may well wrap around with invalid data.
    // But we do not care, as we do not access these fields ourselves,
    // and later on the section values are checked by MachoLib.
    // Note: There is an assumption that 'CopyFileOffset + CurrentDelta' is aligned.
    //
    OriginalDelta  = CurrentDelta;
    CopyFileOffset = Segment->FileOffset;
    for (Index = 0; Index < DstSegment->NumSections; ++Index) {
      if (DstSegment->Sections[Index].Offset == 0) {
        DstSegment->Sections[Index].Offset = (UINT32) CopyFileOffset + CurrentDelta;
        CurrentDelta += (UINT32) DstSegment->Sections[Index].Size;
      } else {
        DstSegment->Sections[Index].Offset += CurrentDelta;
        CopyFileOffset = DstSegment->Sections[Index].Offset + DstSegment->Sections[Index].Size;
      }
    }

    CurrentDelta = OriginalDelta + (UINT32)(Segment->Size - Segment->FileSize);
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

    if (FileSize > DestinationSize) {
      return 0;
    }

    CopyMem (
      Destination + HeaderSize,
      (UINT8 *)Header + HeaderSize,
      FileSize - HeaderSize
      );

    CurrentSize = FileSize;
  }

  if (Strip) {
    InternalStripLoadCommands64 ((MACH_HEADER_64 *) Destination);
  }
  //
  // This cast is safe because CurrentSize is verified against DestinationSize.
  //
  return (UINT32) CurrentSize;
}

UINT64
MachoRuntimeGetEntryAddress (
  IN VOID  *Image
  )
{
  MACH_HEADER_ANY         *Header;
  BOOLEAN                 Is64Bit;
  UINT32                  NumCmds;
  MACH_LOAD_COMMAND       *Cmd;
  UINTN                   Index;
  MACH_THREAD_COMMAND     *ThreadCmd;
  MACH_X86_THREAD_STATE   *ThreadState;
  UINT64                  Address;

  Address = 0;
  Header  = (MACH_HEADER_ANY *) Image;

  if (Header->Signature == MACH_HEADER_SIGNATURE) {
    //
    // 32-bit header.
    //
    Is64Bit = FALSE;
    NumCmds = Header->Header32.NumCommands;
    Cmd     = &Header->Header32.Commands[0];
  } else if (Header->Signature == MACH_HEADER_64_SIGNATURE) {
    //
    // 64-bit header.
    //
    Is64Bit = TRUE;
    NumCmds = Header->Header64.NumCommands;
    Cmd     = &Header->Header64.Commands[0];
  } else {
    //
    // Invalid Mach-O image.
    //
    return Address;
  }

  //
  // Iterate over load commands.
  //
  for (Index = 0; Index < NumCmds; ++Index) {
    if (Cmd->CommandType == MACH_LOAD_COMMAND_UNIX_THREAD) {
      ThreadCmd     = (MACH_THREAD_COMMAND *) Cmd;
      ThreadState   = (MACH_X86_THREAD_STATE *) &ThreadCmd->ThreadState[0];
      Address       = Is64Bit ? ThreadState->State64.rip : ThreadState->State32.eip;
      break;
    }

    Cmd = NEXT_MACH_LOAD_COMMAND (Cmd);
  }

  return Address;
}

BOOLEAN
MachoMergeSegments64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *Prefix
  )
{
  UINT32                  LcIndex;
  MACH_LOAD_COMMAND       *LoadCommand;
  MACH_SEGMENT_COMMAND_64 *Segment;
  MACH_SEGMENT_COMMAND_64 *FirstSegment;
  MACH_HEADER_64          *Header;
  UINTN                   PrefixLength;
  UINTN                   RemainingArea;
  UINT32                  SkipCount;

  ASSERT (Context != NULL);
  ASSERT (Context->FileSize != 0);
  ASSERT (Prefix != NULL);

  Header       = MachoGetMachHeader64 (Context);
  PrefixLength = AsciiStrLen (Prefix);
  FirstSegment = NULL;

  SkipCount   = 0;

  LoadCommand = &Header->Commands[0];

  for (LcIndex = 0; LcIndex < Header->NumCommands; ++LcIndex) {
    //
    // Either skip or stop at unrelated commands.
    //
    Segment = (MACH_SEGMENT_COMMAND_64 *) (VOID *) LoadCommand;

    if (LoadCommand->CommandType != MACH_LOAD_COMMAND_SEGMENT_64
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
  Header->CommandsSize -= (UINT32) (sizeof (MACH_SEGMENT_COMMAND_64) * SkipCount);

  return TRUE;
}
