/** @file
  Library handling KEXT prelinking.
  Currently limited to Intel 64 architectures.
  
Copyright (c) 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef PRELINKED_INTERNAL_H
#define PRELINKED_INTERNAL_H

#include <IndustryStandard/AppleMachoImage.h>

#include <Library/OcAppleKernelLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcXmlLib.h>

//
// Some sane maximum value.
//
#define MAX_KEXT_DEPEDENCIES 12

typedef struct PRELINKED_KEXT_ PRELINKED_KEXT;

typedef struct {
  UINT32 StringIndex;  ///< index into the string table
  UINT64 Value;        ///< value of this symbol (or stab offset)
} PRELINKED_KEXT_SYMBOL;

typedef struct {
  CONST CHAR8 *Name;    ///< The symbol's name.
  UINT64      Address;  ///< The symbol's address.
} PRELINKED_VTABLE_ENTRY;

#define GET_NEXT_PRELINKED_VTABLE(This)                                    \
  (PRELINKED_VTABLE *)(                                                    \
    (UINTN)((This) + 1) + ((This)->NumEntries * sizeof (*(This)->Entries)) \
    )

typedef struct {
  CONST CHAR8            *Name;       ///< The VTable's name.
  UINT32                 NumEntries;  ///< The number of VTable entries.
  PRELINKED_VTABLE_ENTRY Entries[];   ///< The VTable entries.
} PRELINKED_VTABLE;

struct PRELINKED_KEXT_ {
  //
  // These data are used to construct linked lists of dependency information
  // for each KEXT.  It is declared hear for every dependency will
  // eventually be part of a list and to save separate allocations per KEXT.
  //
  UINT32                   Signature;
  LIST_ENTRY               Link;
  //
  // Kext CFBundleIdentifier.
  //
  CONST CHAR8              *Identifier;
  //
  // Patcher context containing useful data.
  //
  PATCHER_CONTEXT          Context;
  //
  // Dependencies dictionary (OSBundleLibraries).
  // May be NULL for KPI kexts or after Dependencies are set.
  //
  XML_NODE                 *BundleLibraries;
  //
  // Compatible version, may be NULL.
  //
  CONST CHAR8              *CompatibleVersion;
  //
  // Scanned dependencies (PRELINKED_KEXT) from BundleLibraries.
  // Not resolved by default. See InternalScanPrelinkedKext for fields below.
  //
  PRELINKED_KEXT           *Dependencies[MAX_KEXT_DEPEDENCIES];
  //
  // Linkedit segment reference.
  //
  MACH_SEGMENT_COMMAND_64  *LinkEditSegment;
  //
  // The String Table associated with this symbol table.
  //
  CONST CHAR8              *StringTable;
  //
  // Symbol table.
  //
  CONST MACH_NLIST_64      *SymbolTable;
  //
  // Symbol table size.
  //
  UINT32                   NumberOfSymbols;
  //
  // Number of C++ symbols. They are put at the end of LinkedSymbolTable.
  // Calculated at LinkedSymbolTable construction.
  //
  UINT32                   NumberOfCxxSymbols;
  //
  // Sorted symbol table used only for dependencies.
  //
  PRELINKED_KEXT_SYMBOL    *LinkedSymbolTable;
  //
  // A flag set during dependency walk BFS to avoid going through the same path.
  //
  BOOLEAN                  Processed;
  //
  // Number of vtables in this kext.
  //
  UINT32                   NumberOfVtables;
  //
  // Scanned vtable buffer. Iterated with GET_NEXT_PRELINKED_VTABLE.
  //
  PRELINKED_VTABLE         *LinkedVtables;
};

//
// PRELINKED_KEXT signature for list identification.
//
#define PRELINKED_KEXT_SIGNATURE  SIGNATURE_32 ('P', 'K', 'X', 'T')

/**
  Gets the next element in a linked list of PRELINKED_KEXT.

  @param[in] This  The current ListEntry.
**/
#define GET_PRELINKED_KEXT_FROM_LINK(This)  \
  (CR (                                     \
    (This),                                 \
    PRELINKED_KEXT,                         \
    Link,                                   \
    PRELINKED_KEXT_SIGNATURE                \
    ))

/**
  Creates new PRELINKED_KEXT from OC_MACHO_CONTEXT.
**/
PRELINKED_KEXT *
InternalNewPrelinkedKext (
  IN OC_MACHO_CONTEXT       *Context,
  IN XML_NODE               *KextPlist
  );

/**
  Frees PRELINKED_KEXT.
**/
VOID
InternalFreePrelinkedKext (
  IN PRELINKED_KEXT  *Kext
  );

/**
  Gets cached PRELINKED_KEXT from PRELINKED_CONTEXT.
**/
PRELINKED_KEXT *
InternalCachedPrelinkedKext (
  IN OUT PRELINKED_CONTEXT  *Prelinked,
  IN     CONST CHAR8        *Identifier
  );

/**
  Gets cached kernel PRELINKED_KEXT from PRELINKED_CONTEXT.
**/
PRELINKED_KEXT *
InternalCachedPrelinkedKernel (
  IN OUT PRELINKED_CONTEXT  *Prelinked
  );

/**
  Scan PRELINKED_KEXT for dependencies.
**/
EFI_STATUS
InternalScanPrelinkedKext (
  IN OUT PRELINKED_KEXT     *Kext,
  IN OUT PRELINKED_CONTEXT  *Context
  );

/**
  Unlock all context dependency kexts by unsetting Processed flag.

  @param[in] Context      Prelinked context.
**/
VOID
InternalUnlockContextKexts (
  IN PRELINKED_CONTEXT  *Context
  );

/**
  Link executable within current prelink context.

  @param[in,out] Context         Prelinked context.
  @param[in,out] Executable      Kext executable copied to prelinked.
  @param[in]     ExecutableSize  Kext executable size.
  @param[in]     PlistRoot       Current kext info.plist.
  @param[in]     LoadAddress     Kext load address.
  @param[in]     KmodAddress     Kext kmod address.
  @param[in,out] AlignedLoadSize Resulting virtual space size.

  @return  prelinked kext to be inserted into PRELINKED_CONTEXT.
**/
PRELINKED_KEXT *
InternalLinkPrelinkedKext (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN OUT OC_MACHO_CONTEXT   *Executable,
  IN     XML_NODE           *PlistRoot,
  IN     UINT64             LoadAddress,
  IN     UINT64             KmodAddress,
  IN OUT UINT32             *AlignedLoadSize
  );

#define KXLD_WEAK_TEST_SYMBOL  "_gOSKextUnresolved"

#define OS_METACLASS_VTABLE_NAME "__ZTV11OSMetaClass"

#define X86_64_RIP_RELATIVE_LIMIT  0x80000000ULL

#define SYM_MAX_NAME_LEN  256U

#define VTABLE_ENTRY_SIZE_64   8U
#define VTABLE_HEADER_LEN_64   2U
#define VTABLE_HEADER_SIZE_64  (VTABLE_HEADER_LEN_64 * VTABLE_ENTRY_SIZE_64)

typedef union {
  struct {
    UINT32 Major      : 14;
    UINT32 Minor      : 7;
    UINT32 Revision   : 7;
    UINT32 Stage      : 4;
    UINT32 StageLevel : 8;
  }      Bits;
  UINT64 Value;
} OC_KEXT_VERSION;

typedef enum {
  OcKextVersionStageDevelopment = 1,
  OcKextVersionStageAlpha       = 3,
  OcKextVersionStageBeta        = 5,
  OcKextVersionStageCandidate   = 7,
  OcKextVersionStageRelease     = 9
} OC_KEXT_VERSION_STAGE;

typedef struct {
  CONST MACH_NLIST_64 *Smcp;
  CONST MACH_NLIST_64 *Vtable;
  CONST MACH_NLIST_64 *MetaVtable;
} OC_VTABLE_PATCH_ENTRY;

typedef struct {
  UINTN                 NumEntries;
  OC_VTABLE_PATCH_ENTRY Entries[];
} OC_VTABLE_PATCH_ARRAY;

typedef struct {
  UINT32              NumSymbols;
  CONST MACH_NLIST_64 *Symbols[];
} OC_VTABLE_EXPORT_ARRAY;

//
// VTables
//

BOOLEAN
InternalGetVtableEntries64 (
  IN  CONST UINT64  *VtableData,
  IN  UINT32        MaxSize,
  OUT UINT32        *NumEntries
  );

BOOLEAN
InternalPrepareVtableCreationNonPrelinked64 (
  IN OUT OC_MACHO_CONTEXT       *MachoContext,
  IN     UINT32                 NumSymbols,
  IN     CONST MACH_NLIST_64    *SymbolTable,
  OUT    OC_VTABLE_PATCH_ARRAY  *PatchData
  );

BOOLEAN
InternalPatchByVtables64 (
  IN     PRELINKED_CONTEXT         *Context,
  IN OUT PRELINKED_KEXT            *Kext,
  IN     OC_VTABLE_PATCH_ARRAY     *PatchData
  );

BOOLEAN
InternalPrepareCreateVtablesPrelinked64 (
  IN  OC_MACHO_CONTEXT          *MachoContext,
  OUT OC_VTABLE_EXPORT_ARRAY    *VtableExport,
  IN  UINT32                    VtableExportSize
  );

BOOLEAN
InternalCreateVtablesPrelinked64 (
  IN     PRELINKED_CONTEXT      *Context,
  IN OUT PRELINKED_KEXT         *Kext,
  IN     OC_VTABLE_EXPORT_ARRAY *VtableExport,
     OUT PRELINKED_VTABLE       *VtableBuffer
  );

CONST PRELINKED_VTABLE *
InternalGetOcVtableByName (
  IN PRELINKED_CONTEXT     *Context,
  IN CONST PRELINKED_KEXT  *Kext,
  IN CONST CHAR8           *Name,
  IN UINT32                RecursionLevel
  );

//
// Prelink
//

typedef enum {
  OcGetSymbolAnyLevel,
  OcGetSymbolFirstLevel,
  OcGetSymbolOnlyCxx
} OC_GET_SYMBOL_LEVEL;

typedef
BOOLEAN
(*PRELINKED_KEXT_SYMBOL_PREDICATE)(
  IN CONST PRELINKED_KEXT         *Kext,
  IN CONST PRELINKED_KEXT_SYMBOL  *Symbol,
  IN       UINT64                 Context
  );

// TODO: Move?
CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolByName (
  IN PRELINKED_CONTEXT    *Context,
  IN PRELINKED_KEXT       *Kext,
  IN CONST CHAR8          *Name,
  IN OC_GET_SYMBOL_LEVEL  SymbolLevel
  );

CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolByValue (
  IN PRELINKED_CONTEXT    *Context,
  IN PRELINKED_KEXT       *Kext,
  IN UINT64               Value,
  IN OC_GET_SYMBOL_LEVEL  SymbolLevel
  );

VOID
InternalSolveSymbolValue64 (
  IN  UINT64         Value,
  OUT MACH_NLIST_64  *Symbol
  );

/**
  Prelinks the specified KEXT against the specified LoadAddress and the data
  of its dependencies.

  @param[in,out] MachoContext     Mach-O context of the KEXT to prelink.
  @param[in]     LinkEditSegment  __LINKEDIT segment of the KEXT to prelink.
  @param[in]     LoadAddress      The address this KEXT shall be linked
                                  against.
  @param[in]     DependencyData   List of data of all dependencies.
  @param[in]     ExposeSymbols    Whether the symbol table shall be exposed.
  @param[out]    OutputData       Buffer to output data into.
  @param[out]    ScratchMemory    Scratch memory buffer that is at least as big
                                  as the KEXT's __LINKEDIT segment.

  @retval  Returned is whether the prelinking process has been successful.
           The state of the KEXT is undefined in case this routine fails.

**/
RETURN_STATUS
InternalPrelinkKext64 (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     PRELINKED_KEXT     *Kext,
  IN     UINT64             LoadAddress
  );

#endif // PRELINKED_INTERNAL_H
