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
#define MAX_KEXT_DEPEDENCIES 16

typedef struct PRELINKED_KEXT_ PRELINKED_KEXT;

typedef struct {
  //
  // Value is declared first as it has shown to improve comparison performance.
  //
  UINT64       Value;  ///< value of this symbol (or stab offset)
  CONST CHAR8  *Name;  ///< name of this symbol
  UINT32       Length;
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
  //
  // Link for global list (PRELINKED_CONTEXT -> PrelinkedKexts).
  //
  LIST_ENTRY               Link;
  //
  // Link for local list (PRELINKED_CONTEXT -> InjectedKexts).
  //
  LIST_ENTRY               InjectedLink;
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
  Gets the next element in PrelinkedKexts list of PRELINKED_KEXT.

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
  Gets the next element in InjectedKexts list of PRELINKED_KEXT.

  @param[in] This  The current ListEntry.
**/
#define GET_INJECTED_KEXT_FROM_LINK(This)   \
  (CR (                                     \
    (This),                                 \
    PRELINKED_KEXT,                         \
    InjectedLink,                           \
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
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     BOOLEAN            Dependency
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
  @param[in]     PlistRoot       Current kext info.plist.
  @param[in]     LoadAddress     Kext load address.
  @param[in]     KmodAddress     Kext kmod address.

  @return  prelinked kext to be inserted into PRELINKED_CONTEXT.
**/
PRELINKED_KEXT *
InternalLinkPrelinkedKext (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN OUT OC_MACHO_CONTEXT   *Executable,
  IN     XML_NODE           *PlistRoot,
  IN     UINT64             LoadAddress,
  IN     UINT64             KmodAddress
  );

EFI_STATUS
InternalConnectExternalSymtab (
  IN OUT OC_MACHO_CONTEXT  *Context,
     OUT OC_MACHO_CONTEXT  *InnerContext,
  IN     UINT8             *Buffer,
  IN     UINT32            BufferSize,
     OUT BOOLEAN           *KernelCollection  OPTIONAL
  );

#define KXLD_WEAK_TEST_SYMBOL  "_gOSKextUnresolved"

#define OS_METACLASS_VTABLE_NAME "__ZTV11OSMetaClass"

#define X86_64_RIP_RELATIVE_LIMIT  0x80000000ULL

#define SYM_MAX_NAME_LEN  256U

#define VTABLE_ENTRY_SIZE_64   8U
#define VTABLE_HEADER_LEN_64   2U
#define VTABLE_HEADER_SIZE_64  (VTABLE_HEADER_LEN_64 * VTABLE_ENTRY_SIZE_64)

#define KERNEL_ADDRESS_MASK 0xFFFFFFFF00000000ULL
#define KERNEL_ADDRESS_BASE 0xFFFFFF8000000000ULL
#define KERNEL_FIXUP_OFFSET BASE_1MB

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

#define GET_NEXT_OC_VTABLE_PATCH_ENTRY(Entry)                         \
  (OC_VTABLE_PATCH_ENTRY *)(                                          \
    (UINTN)((Entry) + 1)                                              \
      + ((Entry)->NumSolveSymbols * sizeof (*(Entry)->SolveSymbols))  \
    )

typedef struct {
  CONST CHAR8  *Name;
  union {
    UINT64       Value;
    CONST UINT64 *Data;
  } Vtable;
} OC_PRELINKED_VTABLE_LOOKUP_ENTRY;

STATIC_ASSERT (
  (sizeof (OC_PRELINKED_VTABLE_LOOKUP_ENTRY) <= sizeof (MACH_NLIST_64)),
  "Prelinked VTable lookup data might not safely fit LinkBuffer"
  );

typedef struct {
  CONST MACH_NLIST_64 *Smcp;
  CONST MACH_NLIST_64 *Vtable;
  UINT64              *VtableData;
  CONST MACH_NLIST_64 *MetaVtable;
  UINT64              *MetaVtableData;
  UINT32              NumSolveSymbols;
  UINT32              MetaSymsIndex;
  MACH_NLIST_64       *SolveSymbols[];
} OC_VTABLE_PATCH_ENTRY;
//
// This ASSERT is very dirty, but it is unlikely to trigger nevertheless.
// One symbol pointer, for which LinkBuffer is guaranteed to be able to fit one
// NLIST, has either 1 pointer or 2 UINT32s following in the struct.
// Due to the location logic, the three symbols pointers will not be equal.
//
STATIC_ASSERT (
  ((sizeof (MACH_NLIST_64 *) + MAX ((2 * sizeof (UINT32)), sizeof (UINT64 *))) <= sizeof (MACH_NLIST_64)),
  "VTable Patch data might not safely fit LinkBuffer"
  );

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
InternalPatchByVtables64 (
  IN     PRELINKED_CONTEXT         *Context,
  IN OUT PRELINKED_KEXT            *Kext
  );

BOOLEAN
InternalPrepareCreateVtablesPrelinked64 (
  IN  PRELINKED_KEXT                    *Kext,
  IN  UINT32                            MaxSize,
  OUT UINT32                            *NumVtables,
  OUT OC_PRELINKED_VTABLE_LOOKUP_ENTRY  *Vtables
  );

VOID
InternalCreateVtablesPrelinked64 (
  IN     PRELINKED_CONTEXT                       *Context,
  IN OUT PRELINKED_KEXT                          *Kext,
  IN     UINT32                                  NumVtables,
  IN     CONST OC_PRELINKED_VTABLE_LOOKUP_ENTRY  *VtableLookups,
  OUT    PRELINKED_VTABLE                        *VtableBuffer
  );

CONST PRELINKED_VTABLE *
InternalGetOcVtableByName (
  IN PRELINKED_CONTEXT     *Context,
  IN PRELINKED_KEXT        *Kext,
  IN CONST CHAR8           *Name
  );

//
// Prelink
//

typedef enum {
  OcGetSymbolByName,
  OcGetSymbolByValue
} OC_GET_SYMBOL_TYPE;

typedef enum {
  OcGetSymbolAnyLevel,
  OcGetSymbolFirstLevel,
  OcGetSymbolOnlyCxx
} OC_GET_SYMBOL_LEVEL;

CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolName (
  IN PRELINKED_CONTEXT    *Context,
  IN PRELINKED_KEXT       *Kext,
  IN CONST CHAR8          *LookupValue,
  IN OC_GET_SYMBOL_LEVEL  SymbolLevel
  );

CONST PRELINKED_KEXT_SYMBOL *
InternalOcGetSymbolValue (
  IN PRELINKED_CONTEXT    *Context,
  IN PRELINKED_KEXT       *Kext,
  IN UINT64               LookupValue,
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

  @param[in,out] Context      Prelinking context.
  @param[in]     Kext         KEXT prelinking context.
  @param[in]     LoadAddress  The address this KEXT shall be linked against.

  @retval  Returned is whether the prelinking process has been successful.
           The state of the KEXT is undefined in case this routine fails.

**/
EFI_STATUS
InternalPrelinkKext64 (
  IN OUT PRELINKED_CONTEXT  *Context,
  IN     PRELINKED_KEXT     *Kext,
  IN     UINT64             LoadAddress
  );

#endif // PRELINKED_INTERNAL_H
