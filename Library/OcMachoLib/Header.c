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

BOOLEAN
MachoInitializeContext (
  OUT OC_MACHO_CONTEXT  *Context,
  IN  VOID              *FileData,
  IN  UINT32            FileSize,
  IN  UINT32            ContainerOffset,
  IN  BOOLEAN           Is32Bit
  )
{
  return Is32Bit ?
    MachoInitializeContext32 (Context, FileData, FileSize, ContainerOffset) :
    MachoInitializeContext64 (Context, FileData, FileSize, ContainerOffset);
}

MACH_HEADER_ANY *
MachoGetMachHeader (
  IN OUT OC_MACHO_CONTEXT   *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->MachHeader != NULL);

  return Context->MachHeader;
}

UINT32
MachoGetFileSize (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->FileSize != 0);

  return Context->FileSize;
}

UINT32
MachoGetVmSize (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    InternalMachoGetVmSize32 (Context) : InternalMachoGetVmSize64 (Context);
}

UINT64
MachoGetLastAddress (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    InternalMachoGetLastAddress32 (Context) : InternalMachoGetLastAddress64 (Context);
}

MACH_LOAD_COMMAND *
MachoGetNextCommand (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_LOAD_COMMAND_TYPE   LoadCommandType,
  IN     CONST MACH_LOAD_COMMAND  *LoadCommand  OPTIONAL
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    InternalMachoGetNextCommand32 (Context, LoadCommandType, LoadCommand) :
    InternalMachoGetNextCommand64 (Context, LoadCommandType, LoadCommand);
}

MACH_UUID_COMMAND *
MachoGetUuid (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  MACH_UUID_COMMAND *UuidCommand;

  ASSERT (Context != NULL);

  //
  // Context initialisation guarantees the command size is a multiple of 4 or 8.
  //
  if (Context->Is32Bit) {
    STATIC_ASSERT (
      OC_ALIGNOF (MACH_UUID_COMMAND) <= sizeof (UINT32),
      "Alignment is not guaranteed."
      );
  } else {
    STATIC_ASSERT (
      OC_ALIGNOF (MACH_UUID_COMMAND) <= sizeof (UINT64),
      "Alignment is not guaranteed."
      );
  }

  UuidCommand = (MACH_UUID_COMMAND *) (VOID *) MachoGetNextCommand (
    Context,
    MACH_LOAD_COMMAND_UUID,
    NULL
    );
  if (UuidCommand == NULL || UuidCommand->CommandSize != sizeof (*UuidCommand)) {
    return NULL;
  }
  return UuidCommand;
}

MACH_SEGMENT_COMMAND_ANY *
MachoGetNextSegment (
  IN OUT OC_MACHO_CONTEXT               *Context,
  IN     CONST MACH_SEGMENT_COMMAND_ANY *Segment  OPTIONAL
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    (MACH_SEGMENT_COMMAND_ANY *) MachoGetNextSegment32 (Context, Segment != NULL ? &Segment->Segment32 : NULL) :
    (MACH_SEGMENT_COMMAND_ANY *) MachoGetNextSegment64 (Context, Segment != NULL ? &Segment->Segment64 : NULL);
}

MACH_SECTION_ANY *
MachoGetNextSection (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_ANY *Segment,
  IN     MACH_SECTION_ANY         *Section  OPTIONAL
  )
{
  ASSERT (Context != NULL);
  ASSERT (Segment != NULL);

  return Context->Is32Bit ?
    (MACH_SECTION_ANY *) MachoGetNextSection32 (Context, &Segment->Segment32, Section != NULL ? &Section->Section32 : NULL) :
    (MACH_SECTION_ANY *) MachoGetNextSection64 (Context, &Segment->Segment64, Section != NULL ? &Section->Section64 : NULL);
}

MACH_SECTION_ANY *
MachoGetSectionByIndex (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Index
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    (MACH_SECTION_ANY *) MachoGetSectionByIndex32 (Context, Index) :
    (MACH_SECTION_ANY *) MachoGetSectionByIndex64 (Context, Index);
}

MACH_SEGMENT_COMMAND_ANY *
MachoGetSegmentByName (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SegmentName
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    (MACH_SEGMENT_COMMAND_ANY *) MachoGetSegmentByName32 (Context, SegmentName) :
    (MACH_SEGMENT_COMMAND_ANY *) MachoGetSegmentByName64 (Context, SegmentName);
}

MACH_SECTION_ANY *
MachoGetSectionByName (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_SEGMENT_COMMAND_ANY *Segment,
  IN     CONST CHAR8              *SectionName
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    (MACH_SECTION_ANY *) MachoGetSectionByName32 (Context, &Segment->Segment32, SectionName) :
    (MACH_SECTION_ANY *) MachoGetSectionByName64 (Context, &Segment->Segment64, SectionName);
}

MACH_SECTION_ANY *
MachoGetSegmentSectionByName (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SegmentName,
  IN     CONST CHAR8       *SectionName
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    (MACH_SECTION_ANY *) MachoGetSegmentSectionByName32 (Context, SegmentName, SectionName) :
    (MACH_SECTION_ANY *) MachoGetSegmentSectionByName64 (Context, SegmentName, SectionName);
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
InternalInitialiseSymtabs (
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
  MACH_NLIST_ANY        *SymbolTable;
  MACH_NLIST_ANY        *IndirectSymtab;
  MACH_RELOCATION_INFO  *LocalRelocations;
  MACH_RELOCATION_INFO  *ExternRelocations;

  VOID                  *Tmp;

  ASSERT (Context != NULL);
  ASSERT (Context->MachHeader != NULL);
  ASSERT (Context->FileSize > 0);
  ASSERT (Context->SymbolTable == NULL);

  FileSize  = Context->FileSize;

  Result = OcOverflowSubU32 (
             Symtab->SymbolsOffset,
             Context->ContainerOffset,
             &SymbolsOffset
             );
  Result |= OcOverflowMulAddU32 (
              Symtab->NumSymbols,
              Context->Is32Bit ? sizeof (MACH_NLIST) : sizeof (MACH_NLIST_64),
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

  SymbolTable = NULL;

  Tmp = (VOID *)(MachoAddress + SymbolsOffset);
  if (!(Context->Is32Bit ? OC_TYPE_ALIGNED (MACH_NLIST, Tmp) : OC_TYPE_ALIGNED (MACH_NLIST_64, Tmp))) {
    return FALSE;
  }

  SymbolTable       = (MACH_NLIST_ANY *)Tmp;
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
                  Context->Is32Bit ? sizeof (MACH_NLIST) : sizeof (MACH_NLIST_64),
                  IndirectSymbolsOffset,
                  &OffsetTop
                  );
      if (Result || (OffsetTop > FileSize)) {
        return FALSE;
      }

      Tmp = (VOID *)(MachoAddress + IndirectSymbolsOffset);
      if (!(Context->Is32Bit ? OC_TYPE_ALIGNED (MACH_NLIST, Tmp) : OC_TYPE_ALIGNED (MACH_NLIST_64, Tmp))) {
        return FALSE;
      }
      IndirectSymtab = (MACH_NLIST_ANY *)Tmp;
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
  Context->StringTable = StringTable;
  Context->DySymtab    = DySymtab;

  Context->LocalRelocations     = LocalRelocations;
  Context->ExternRelocations    = ExternRelocations;
  Context->SymbolTable          = SymbolTable;
  Context->IndirectSymbolTable  = IndirectSymtab;

  return TRUE;
}

BOOLEAN
MachoInitialiseSymtabsExternal (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     OC_MACHO_CONTEXT  *SymsContext
  )
{
  MACH_SYMTAB_COMMAND   *Symtab;
  MACH_DYSYMTAB_COMMAND *DySymtab;
  BOOLEAN               IsDyld;
  MACH_HEADER_FLAGS     MachFlags;
  MACH_HEADER_FLAGS     SymsMachFlags;

  ASSERT (Context != NULL);
  ASSERT (Context->MachHeader != NULL);
  ASSERT (SymsContext != NULL);

  if (Context->SymbolTable != NULL) {
    return TRUE;
  }

  MachFlags = Context->Is32Bit ?
    Context->MachHeader->Header32.Flags :
    Context->MachHeader->Header64.Flags;
  SymsMachFlags = SymsContext->Is32Bit ?
    SymsContext->MachHeader->Header32.Flags :
    SymsContext->MachHeader->Header64.Flags;

  //
  // We cannot use SymsContext's symbol tables if Context is flagged for DYLD
  // and SymsContext is not.
  //
  IsDyld = (MachFlags & MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK) != 0;
  if (IsDyld
   && (SymsMachFlags & MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK) == 0) {
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
  Symtab = (MACH_SYMTAB_COMMAND *) (VOID *) MachoGetNextCommand (
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
    DySymtab = (MACH_DYSYMTAB_COMMAND *) (VOID *) MachoGetNextCommand (
      SymsContext,
      MACH_LOAD_COMMAND_DYSYMTAB,
      NULL
      );
    if (DySymtab == NULL || DySymtab->CommandSize != sizeof (*DySymtab)) {
      return FALSE;
    }
  }

  return InternalInitialiseSymtabs (Context, Symtab, DySymtab);
}

BOOLEAN
InternalRetrieveSymtabs (
  IN OUT OC_MACHO_CONTEXT  *Context
  )
{
  //
  // Retrieve the symbol information for Context from itself.
  //
  return MachoInitialiseSymtabsExternal (Context, Context);
}

UINT32
MachoGetSymbolTable (
  IN OUT OC_MACHO_CONTEXT     *Context,
     OUT CONST MACH_NLIST_ANY **SymbolTable,
     OUT CONST CHAR8          **StringTable OPTIONAL,
     OUT CONST MACH_NLIST_ANY **LocalSymbols OPTIONAL,
     OUT UINT32               *NumLocalSymbols OPTIONAL,
     OUT CONST MACH_NLIST_ANY **ExternalSymbols OPTIONAL,
     OUT UINT32               *NumExternalSymbols OPTIONAL,
     OUT CONST MACH_NLIST_ANY **UndefinedSymbols OPTIONAL,
     OUT UINT32               *NumUndefinedSymbols OPTIONAL
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    MachoGetSymbolTable32 (
      Context,
      (CONST MACH_NLIST **) SymbolTable,
      StringTable,
      (CONST MACH_NLIST **) LocalSymbols,
      NumLocalSymbols,
      (CONST MACH_NLIST **) ExternalSymbols,
      NumExternalSymbols, 
      (CONST MACH_NLIST **) UndefinedSymbols,
      NumUndefinedSymbols
      ) :
    MachoGetSymbolTable64 (
      Context,
      (CONST MACH_NLIST_64 **) SymbolTable,
      StringTable,
      (CONST MACH_NLIST_64 **) LocalSymbols,
      NumLocalSymbols,
      (CONST MACH_NLIST_64 **) ExternalSymbols,
      NumExternalSymbols, 
      (CONST MACH_NLIST_64 **) UndefinedSymbols,
      NumUndefinedSymbols
      );
}

UINT32
MachoGetIndirectSymbolTable (
  IN OUT OC_MACHO_CONTEXT     *Context,
     OUT CONST MACH_NLIST_ANY **SymbolTable
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    MachoGetIndirectSymbolTable32 (Context, (CONST MACH_NLIST **) SymbolTable) :
    MachoGetIndirectSymbolTable64 (Context, (CONST MACH_NLIST_64 **) SymbolTable);
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

VOID *
MachoGetFilePointerByAddress (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address,
  OUT    UINT32            *MaxSize OPTIONAL
  )
{
  ASSERT (Context != NULL);

  if (Context->Is32Bit) {
    ASSERT (Address < MAX_UINT32);
  }

  return Context->Is32Bit ?
    InternalMachoGetFilePointerByAddress32 (Context, (UINT32) Address, MaxSize) :
    InternalMachoGetFilePointerByAddress64 (Context, Address, MaxSize);
}

UINT32
MachoExpandImage (
  IN  OC_MACHO_CONTEXT   *Context,
  OUT UINT8              *Destination,
  IN  UINT32             DestinationSize,
  IN  BOOLEAN            Strip,
  OUT UINT64             *FileOffset OPTIONAL
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    InternalMachoExpandImage32 (Context, FALSE, Destination, DestinationSize, Strip, FileOffset) :
    InternalMachoExpandImage64 (Context, FALSE, Destination, DestinationSize, Strip, FileOffset);
}

UINT32
MachoGetExpandedImageSize (
  IN  OC_MACHO_CONTEXT   *Context
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    MACHO_ALIGN (InternalMachoExpandImage32 (Context, TRUE, NULL, 0, FALSE, NULL)) :
    MACHO_ALIGN (InternalMachoExpandImage64 (Context, TRUE, NULL, 0, FALSE, NULL));
}

BOOLEAN
MachoMergeSegments (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *Prefix
  )
{
  ASSERT (Context != NULL);

  return Context->Is32Bit ?
    InternalMachoMergeSegments32 (Context, Prefix) :
    InternalMachoMergeSegments64 (Context, Prefix);
}
