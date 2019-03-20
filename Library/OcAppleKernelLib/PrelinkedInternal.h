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
  // Dependencies dictionary (OSBundleLibraries), may be NULL for KPI kexts.
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
  // Sorted symbol table used only for dependencies.
  //
  PRELINKED_KEXT_SYMBOL    *LinkedSymbolTable;
  //
  // The number of symbols in the entire LinkedSymbolTable.
  //
  UINT32                   LinkedNumberOfSymbols;
  //
  // The number of C++ symbols at the end of LinkedSymbolTable.
  //
  UINT32                   LinkedNumberOfCxxSymbols;
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
  Scan PRELINKED_KEXT for dependencies.
**/
EFI_STATUS
InternalScanPrelinkedKext (
  IN OUT PRELINKED_KEXT     *Kext,
  IN OUT PRELINKED_CONTEXT  *Context
  );


#define KXLD_WEAK_TEST_SYMBOL  "_gOSKextUnresolved"

#define OS_METACLASS_VTABLE_NAME "__ZTV11OSMetaClass"

#define X86_64_RIP_RELATIVE_LIMIT  0x80000000ULL

#define SYM_MAX_NAME_LEN  256U

#define VTABLE_ENTRY_SIZE_64   8U
#define VTABLE_HEADER_LEN_64   2U
#define VTABLE_HEADER_SIZE_64  (VTABLE_HEADER_LEN_64 * VTABLE_ENTRY_SIZE_64)

typedef struct {
  UINT32 StringIndex;  ///< index into the string table
  UINT64 Value;        ///< value of this symbol (or stab offset)
} OC_SYMBOL_64;

#define OC_SYMBOL_TABLE_64_SIGNATURE  SIGNATURE_32 ('O', 'S', '6', '4')

/**
  Gets the next element in a linked list of OC_SYMBOL_TABLE_64.

  @param[in] This  The current ListEntry.

**/
#define GET_OC_SYMBOL_TABLE_64_FROM_LINK(This)  \
  CR (                                          \
    (This),                                     \
    OC_SYMBOL_TABLE_64,                         \
    Link,                                       \
    OC_SYMBOL_TABLE_64_SIGNATURE                \
    )

typedef struct {
  ///
  /// These data are used to construct linked lists of dependency information
  /// for each KEXT.  It is declared hear for every dependency will
  /// eventually be part of a list and to save separate allocations per KEXT.
  ///
  UINT32       Signature;
  LIST_ENTRY   Link;
  BOOLEAN      IsIndirect;
  ///
  /// The String Table associated with this symbol table.
  ///
  CONST CHAR8  *StringTable;
  ///
  /// The number of symbols in the entire symbols buffer.
  ///
  UINT32       NumSymbols;
  ///
  /// The number of C++ symbols at the end of the symbols buffer.
  ///
  UINT32       NumCxxSymbols;
  OC_SYMBOL_64 Symbols[];  ///< The symbol buffer.
} OC_SYMBOL_TABLE_64;

typedef struct {
  CONST CHAR8 *Name;    ///< The symbol's name.
  UINT64      Address;  ///< The symbol's address.
} OC_VTABLE_ENTRY;

#define GET_NEXT_OC_VTABLE(This)  \
  ((OC_VTABLE *)(&(This)->Entries[(This)->NumEntries]))

typedef struct {
  CONST CHAR8     *Name;       ///< The VTable's name.
  UINT32          NumEntries;  ///< The number of VTable entries.
  OC_VTABLE_ENTRY Entries[];   ///< The VTable entries.
} OC_VTABLE;

#define OC_VTABLE_ARRAY_SIGNATURE  SIGNATURE_32 ('O', 'V', 'T', 'A')

/**
  Gets the next element in a linked list of OC_VTABLE_ARRAY.

  @param[in] This  The current ListEntry.

**/
#define GET_OC_VTABLE_ARRAY_FROM_LINK(This)  \
  CR (                                       \
    (This),                                  \
    OC_VTABLE_ARRAY,                         \
    Link,                                    \
    OC_VTABLE_ARRAY_SIGNATURE                \
    )

#define GET_FIRST_OC_VTABLE(This)  \
  ((OC_VTABLE *)((This) + 1))

typedef struct {
  ///
  /// These data are used to construct linked lists of dependency information
  /// for each KEXT.  It is declared hear for every dependency will
  /// eventually be part of a list and to save separate allocations per KEXT.
  ///
  UINT32     Signature;
  LIST_ENTRY Link;
  ///
  /// The number of VTables in the array.
  ///
  UINT32     NumVtables;
  //
  // NOTE: This is an array that cannot be declared as such as OC_VTABLE
  //       contains a flexible array itself.  As the size is dynamic, do not
  //       try to use pointer arithmetics.
  //
  ///
  /// VTable array.
  ///
  OC_VTABLE  Vtables;
} OC_VTABLE_ARRAY;

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
  OC_SYMBOL_TABLE_64 *SymbolTable;
  OC_VTABLE_ARRAY    *Vtables;
} OC_DEPENDENCY_DATA;

#define OC_DEPENDENCY_INFO_ENTRY_SIGNATURE  \
  SIGNATURE_32 ('O', 'D', 'I', 'E')

#define OC_DEP_INFO_FROM_LINK(This)  \
  (CR (  \
     (This),  \
     OC_DEPENDENCY_INFO_ENTRY,  \
     Link,  \
     OC_DEPENDENCY_INFO_ENTRY_SIGNATURE  \
     ))

typedef struct OC_DEPENDENCY_INFO_ENTRY_ OC_DEPENDENCY_INFO_ENTRY;

typedef struct {
  MACH_HEADER_64           *MachHeader;
  UINT32                   MachoSize;
  CONST CHAR8              *Name;
  OC_KEXT_VERSION          Version;
  OC_KEXT_VERSION          CompatibleVersion;
  UINTN                    NumDependencies;
  OC_DEPENDENCY_INFO_ENTRY *Dependencies[];
} OC_DEPENDENCY_INFO;

struct OC_DEPENDENCY_INFO_ENTRY_ {
  UINT32             Signature;
  LIST_ENTRY         Link;
  BOOLEAN            Prelinked;
  OC_DEPENDENCY_DATA Data;
  OC_DEPENDENCY_INFO Info;
};

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
// Dependencies
//

OC_DEPENDENCY_INFO_ENTRY *
InternalKextCollectInformation (
  IN     XML_NODE          *Plist,
  IN OUT OC_MACHO_CONTEXT  *MachoContext OPTIONAL,
  IN     UINT64            KextsVirtual OPTIONAL,
  IN     UINTN             KextsPhysical OPTIONAL,
  IN     UINT64            RequestedVersion OPTIONAL
  );

VOID
InternalFreeDependencyEntry (
  IN OC_DEPENDENCY_INFO_ENTRY  *Entry
  );

UINT64
InternalGetNewPrelinkedKextLoadAddress (
  IN OUT OC_MACHO_CONTEXT               *KernelContext,
  IN     CONST MACH_SEGMENT_COMMAND_64  *PrelinkedKextsSegment,
  IN     CONST CHAR8                    *PrelinkedPlist
  );

/**
  Fills SymbolTable with the symbols provided in Symbols.  For performance
  reasons, the C++ symbols are continuously added to the top of the buffer.
  Their order is not preserved.  SymbolTable->SymbolTable is expected to be
  a valid buffer that can store at least NumSymbols symbols.

  @param[in]     MachoContext  Context of the Mach-O.
  @param[in]     NumSymbols    The number of symbols to copy.
  @param[in]     Symbols       The source symbol array.
  @param[in,out] SymbolTable   The desination Symbol List.  Must be able to
                               hold at least NumSymbols symbols.

**/
VOID
InternalFillSymbolTable64 (
  IN OUT OC_MACHO_CONTEXT     *MachoContext,
  IN     UINT32               NumSymbols,
  IN     CONST MACH_NLIST_64  *Symbols,
  IN OUT OC_SYMBOL_TABLE_64   *SymbolTable
  );

//
// PLIST
//

CONST CHAR8 *
InternalPlistStrStrSameLevelUp (
  IN CONST CHAR8  *AnchorString,
  IN CONST CHAR8  *FindString,
  IN UINTN        FindStringLen
  );

CONST CHAR8 *
InternalPlistStrStrSameLevelDown (
  IN CONST CHAR8  *AnchorString,
  IN CONST CHAR8  *FindString,
  IN UINTN        FindStringLen
  );

CONST CHAR8 *
InternalPlistStrStrSameLevel (
  IN CONST CHAR8  *AnchorString,
  IN CONST CHAR8  *FindString,
  IN UINTN        FindStringLen,
  IN UINTN        DownwardsOffset
  );

UINT64
InternalPlistGetIntValue (
  IN CONST CHAR8  **Tag
  );

//
// VTables
//

UINT32
InternalGetVtableSize64 (
  IN CONST UINT64  *VtableData
  );

BOOLEAN
InternalGetVtableSizeWithRelocs64 (
  IN OUT OC_MACHO_CONTEXT  *MachoContext,
  IN     CONST UINT64      *VtableData,
  OUT    UINT32            *VtableSize
  );

BOOLEAN
InternalPrepareVtableCreationNonPrelinked64 (
  IN OUT OC_MACHO_CONTEXT       *MachoContext,
  IN     UINT32                 NumSymbols,
  IN     CONST MACH_NLIST_64    *SymbolTable,
  OUT    OC_VTABLE_PATCH_ARRAY  *PatchData
  );

BOOLEAN
InternalCreateVtablesNonPrelinked64 (
  IN OUT OC_MACHO_CONTEXT          *MachoContext,
  IN     CONST OC_DEPENDENCY_DATA  *DependencyData,
  IN     OC_VTABLE_PATCH_ARRAY     *PatchData,
  OUT    OC_VTABLE_ARRAY           *VtableArray
  );

BOOLEAN
InternalPrepareCreateVtablesPrelinked64 (
  IN  OC_MACHO_CONTEXT          *MachoContext,
  OUT OC_VTABLE_EXPORT_ARRAY    *VtableExport,
  IN  UINT32                    VtableExportSize
  );

BOOLEAN
InternalCreateVtablesPrelinked64 (
  IN  OC_MACHO_CONTEXT          *MachoContext,
  IN  CONST OC_SYMBOL_TABLE_64  *DefinedSymbols,
  IN  OC_VTABLE_EXPORT_ARRAY    *VtableExport,
  OUT OC_VTABLE                 *VtableBuffer
  );

//
// Prelink
//

// TODO: Move?
CONST OC_SYMBOL_64 *
InternalOcGetSymbolByName (
  IN CONST OC_SYMBOL_TABLE_64  *DefinedSymbols,
  IN CONST CHAR8               *Name,
  IN BOOLEAN                   CheckIndirect
  );

VOID
InternalSolveSymbolValue64 (
  IN  UINT64         Value,
  OUT MACH_NLIST_64  *Symbol
  );

/**
  Prelinks the specified KEXT against the specified LinkAddress and the data
  of its dependencies.

  @param[in,out] MachoContext     Mach-O context of the KEXT to prelink.
  @param[in]     LinkEditSegment  __LINKEDIT segment of the KEXT to prelink.
  @param[in]     LinkAddress      The address this KEXT shall be linked
                                  against.
  @param[in]     DependencyData   List of data of all dependencies.
  @param[in]     ExposeSymbols    Whether the symbol table shall be exposed.
  @param[out]    OutputData       Buffer to output data into.
  @param[out]    ScratchMemory    Scratch memory buffer that is at least as big
                                  as the KEXT's __LINKEDIT segment.

  @retval  Returned is whether the prelinking process has been successful.
           The state of the KEXT is undefined in case this routine fails.

**/
BOOLEAN
InternalPrelinkKext64 (
  IN OUT OC_MACHO_CONTEXT         *MachoContext,
  IN     MACH_SEGMENT_COMMAND_64  *LinkEditSegment,
  IN     UINT64                   LinkAddress,
  IN     OC_DEPENDENCY_DATA       *DependencyData,
  IN     BOOLEAN                  ExposeSymbols,
  IN OUT OC_DEPENDENCY_DATA       *OutputData,
  OUT    VOID                     *ScratchMemory
  );

#endif // PRELINKED_INTERNAL_H
