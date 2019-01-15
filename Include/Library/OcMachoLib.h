/** @file
  Provides Mach-O parsing helper functions.

Copyright (c) 2016 - 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_MACHO_LIB_H_
#define OC_MACHO_LIB_H_

#include <IndustryStandard/AppleMachoImage.h>

///
/// Context used to refer to a Mach-O.  This struct is exposed for reference
/// only.  Members are not guaranteed to be sane.
///
typedef struct {
  MACH_HEADER_64        *MachHeader;
  UINT32                FileSize;
  MACH_SYMTAB_COMMAND   *Symtab;
  MACH_NLIST_64         *SymbolTable;
  CHAR8                 *StringTable;
  MACH_DYSYMTAB_COMMAND *DySymtab;
  MACH_NLIST_64         *IndirectSymbolTable;
  MACH_RELOCATION_INFO  *LocalRelocations;
  MACH_RELOCATION_INFO  *ExternRelocations;
} OC_MACHO_CONTEXT;

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
  );

/**
  Returns the Mach-O Header structure.

  @param[in,out] Context  Context of the Mach-O.

**/
MACH_HEADER_64 *
MachoGetMachHeader64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

/**
  Returns the Mach-O's file size.

  @param[in,out] Context  Context of the Mach-O.

**/
UINT32
MachoGetFileSize (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

/**
  Returns the last virtual address of a Mach-O.

  @param[in] Context  Context of the Mach-O.

  @retval 0  The binary is malformed.

**/
UINT64
MachoGetLastAddress64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Returns whether Symbol describes a section.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to evaluate.

**/
BOOLEAN
MachoSymbolIsSection (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  );

/**
  Returns whether Symbol is defined.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to evaluate.

**/
BOOLEAN
MachoSymbolIsDefined (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  );

/**
  Returns whether Symbol is defined locally.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to evaluate.

**/
BOOLEAN
MachoSymbolIsLocalDefined (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  );

/**
  Retrieves a locally defined symbol by its name.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Name     Name of the symbol to locate.
  @param[out]    Symbol   Pointer to return the symbol into.
                          If FALSE is returned, the output is undefined.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetLocalDefinedSymbolByName (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *Name,
  OUT    MACH_NLIST_64     **Symbol
  );

/**
  Retrieves a symbol by its index.

  @param[in]  Context  Context of the Mach-O.
  @param[in]  Index    Index of the symbol to locate.
  @param[out] Symbol   Pointer to return the symbol into.
                       If FALSE is returned, the output is undefined.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetSymbolByIndex64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Index,
  OUT    MACH_NLIST_64     **Symbol
  );

/**
  Retrieves Symbol's name.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to retrieve the name of.

  @retval NULL  NULL is returned on failure.

**/
CONST CHAR8 *
MachoGetSymbolName64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  );

/**
  Retrieves Symbol's name.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Indirect symbol to retrieve the name of.

  @retval NULL  NULL is returned on failure.

**/
CONST CHAR8 *
MachoGetIndirectSymbolName64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  );

/**
  Returns whether the symbol's value is a valid address within the Mach-O
  referenced to by Context.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol to verify the value of.

**/
BOOLEAN
MachoIsSymbolValueSane64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  );

/**
  Retrieves the symbol referenced by the Relocation targeting Address.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  Address to search for.
  @param[out]    Symbol   Pointer the symbol is returned into.
                          If FALSE is returned, the output data is undefined.
                          If TRUE is returned and Symbol is NULL, no
                          Relocation exists that references Address.                     

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
MachoGetSymbolByExternRelocationOffset64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address,
  OUT    MACH_NLIST_64     **Symbol
  );

/**
  Relocate Symbol to be against LinkAddress.

  @param[in,out] Context      Context of the Mach-O.
  @param[in]     LinkAddress  The address to be linked against.
  @param[in,out] Symbol       The symbol to be relocated.

  @returns  Whether the operation has been completed successfully.

**/
BOOLEAN
MachoRelocateSymbol64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            LinkAddress,
  IN OUT MACH_NLIST_64     *Symbol
  );

/**
  Retrieves the Mach-O file offset of the address pointed to by Symbol.

  @param[in,ouz] Context     Context of the Mach-O.
  @param[in]     Symbol      Symbol to retrieve the offset of.
  @param[out]    FileOffset  Pointer the file offset is returned into.
                             If FALSE is returned, the output is undefined.

  @retval 0  0 is returned on failure.

**/
BOOLEAN
MachoSymbolGetFileOffset64 (
  IN OUT OC_MACHO_CONTEXT      *Context,
  IN     CONST  MACH_NLIST_64  *Symbol,
  OUT    UINT32                *FileOffset
  );

/**
  Returns whether Name is pure virtual.

  @param[in] Name  The name to evaluate.

**/
BOOLEAN
MachoSymbolNameIsPureVirtual (
  IN CONST CHAR8  *Name
  );

/**
  Returns whether Name is a Padslot.

  @param[in] Name  The name to evaluate.

**/
BOOLEAN
MachoSymbolNameIsPadslot (
  IN CONST CHAR8  *Name
  );

/**
  Returns whether SymbolName defines a Super Metaclass Pointer.

  @param[in,out] Context     Context of the Mach-O.
  @param[in]     SymbolName  The symbol name to check.

**/
BOOLEAN
MachoSymbolNameIsSmcp64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SymbolName
  );

/**
  Returns whether SymbolName defines a Super Metaclass Pointer.

  @param[in,out] Context     Context of the Mach-O.
  @param[in]     SymbolName  The symbol name to check.

**/
BOOLEAN
MachoSymbolNameIsMetaclassPointer64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST CHAR8       *SymbolName
  );

/**
  Retrieves the class name of a Super Meta Class Pointer.

  @param[in,out] Context        Context of the Mach-O.
  @param[in]     SmcpName       SMCP Symbol name to get the class name of.
  @param[in]     ClassNameSize  The size of ClassName.
  @param[out]    ClassName      The output buffer for the class name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetClassNameFromSuperMetaClassPointer (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *SmcpName,
  IN     UINTN                ClassNameSize,
  OUT    CHAR8                *ClassName
  );

/**
  Retrieves the class name of a VTable.

  @param[out] VtableName  The VTable's name.

**/
CONST CHAR8 *
MachoGetClassNameFromVtableName (
  IN  CONST CHAR8  *VtableName
  );

/**
  Retrieves the function prefix of a class name.

  @param[in]  ClassName           The class name to evaluate.
  @param[in]  FunctionPrefixSize  The size of FunctionPrefix.
  @param[out] FunctionPrefix      The output buffer for the function prefix.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetFunctionPrefixFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        FunctionPrefixSize,
  OUT CHAR8        *FunctionPrefix
  );

/**
  Retrieves the class name of a Meta Class Pointer.

  @param[in,out] Context             Context of the Mach-O.
  @param[in]     MetaClassName       MCP Symbol name to get the class name of.
  @param[in]     ClassNameSize       The size of ClassName.
  @param[out]    ClassName           The output buffer for the class name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetClassNameFromMetaClassPointer (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *MetaClassName,
  IN     UINTN                ClassNameSize,
  OUT    CHAR8                *ClassName
  );

/**
  Retrieves the VTable name of a class name.

  @param[in]  ClassName       Class name to get the VTable name of.
  @param[in]  VtableNameSize  The size of VtableName.
  @param[out] VtableName      The output buffer for the VTable name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetVtableNameFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        VtableNameSize,
  OUT CHAR8        *VtableName
  );

/**
  Retrieves the Meta VTable name of a class name.

  @param[in]  ClassName       Class name to get the Meta VTable name of.
  @param[in]  VtableNameSize  The size of VtableName.
  @param[out] VtableName      The output buffer for the VTable name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetMetaVtableNameFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        VtableNameSize,
  OUT CHAR8        *VtableName
  );

/**
  Retrieves the final symbol name of a class name.

  @param[in]  ClassName            Class name to get the final symbol name of.
  @param[in]  FinalSymbolNameSize  The size of FinalSymbolName.
  @param[out] FinalSymbolName      The output buffer for the final symbol name.

  @returns  Whether the name has been retrieved successfully.

**/
BOOLEAN
MachoGetFinalSymbolNameFromClassName (
  IN  CONST CHAR8  *ClassName,
  IN  UINTN        FinalSymbolNameSize,
  OUT CHAR8        *FinalSymbolName
  );

/**
  Returns whether SymbolName defines a VTable.

  @param[in]     SymbolName  The symbol name to check.

**/
BOOLEAN
MachoSymbolNameIsVtable64 (
  IN     CONST CHAR8       *SymbolName
  );

/**
  Returns whether the symbol name describes a C++ symbol.

  @param[in] Name  The name to evaluate.

**/
BOOLEAN
MachoSymbolNameIsCxx (
  IN CONST CHAR8  *Name
  );

/**
  Returns the number of VTable entires in VtableData.

  @param[in,out] Context     Context of the Mach-O.
  @param[in]     VtableData  The VTable's data.

**/
UINTN
MachoVtableGetNumberOfEntries64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     CONST UINT64      *VtableData
  );

/**
  Retrieves Metaclass symbol of a SMCP.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Smcp     The SMCP to evaluate.

  @retval NULL  NULL is returned on failure.

**/
MACH_NLIST_64 *
MachoGetMetaclassSymbolFromSmcpSymbol64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Smcp
  );

/**
  Retrieves VTable and Meta VTable of a SMCP.
  Logically matches XNU's get_vtable_syms_from_smcp.

  @param[in,out] Context      Context of the Mach-O.
  @param[in]     SmcpName     SMCP Symbol mame to retrieve the VTables from.
  @param[out]    Vtable       Output buffer for the VTable symbol pointer.
  @param[out]    MetaVtable   Output buffer for the Meta VTable symbol pointer.

**/
BOOLEAN
MachoGetVtableSymbolsFromSmcp64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *SmcpName,
  OUT    CONST MACH_NLIST_64  **Vtable,
  OUT    CONST MACH_NLIST_64  **MetaVtable
  );

/**
  Returns whether the Relocation's type indicates a Pair for the Intel 64
  platform.
  
  @param[in] Type  The Relocation's type to verify.

**/
BOOLEAN
MachoRelocationIsPairIntel64 (
  IN UINT8  Type
  );

/**
  Returns whether the Relocation's type matches a Pair's for the Intel 64
  platform.
  
  @param[in] Type  The Relocation's type to verify.

**/
BOOLEAN
MachoIsRelocationPairTypeIntel64 (
  IN UINT8  Type
  );

/**
  Returns whether the Relocation shall be preserved for the Intel 64 platform.
  
  @param[in] Type  The Relocation's type to verify.

**/
BOOLEAN
MachoPreserveRelocationIntel64 (
  IN UINT8  Type
  );

#endif // OC_MACHO_LIB_H_
