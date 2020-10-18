/**
  Private data of OcMachoLib.

Copyright (C) 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_MACHO_LIB_INTERNAL_H_
#define OC_MACHO_LIB_INTERNAL_H_

#include <IndustryStandard/AppleMachoImage.h>

#include <Library/OcMachoLib.h>

#define SYM_MAX_NAME_LEN  256U

/**
  Retrieves the SYMTAB command.

  @param[in] Context  Context of the Mach-O.

  @retval NULL  NULL is returned on failure.
**/
BOOLEAN
InternalRetrieveSymtabs (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

/**
  Retrieves an extern Relocation by the address it targets.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  The address to search for.

  @retval NULL  NULL is returned on failure.
**/
MACH_RELOCATION_INFO *
InternalGetExternRelocationByOffset (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address
  );

/**
  Retrieves a Relocation by the address it targets.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  The address to search for.

  @retval NULL  NULL is returned on failure.

**/
MACH_RELOCATION_INFO *
InternalGetLocalRelocationByOffset (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address
  );

/**
  Check 32-bit symbol validity.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol from some table.

  @retval TRUE on success.
**/
BOOLEAN
InternalSymbolIsSane32 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST     *Symbol
  );

/**
  Check 64-bit symbol validity.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Symbol   Symbol from some table.

  @retval TRUE on success.
**/
BOOLEAN
InternalSymbolIsSane64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST MACH_NLIST_64  *Symbol
  );

/**
  Retrieves the Mach-O file offset of the address pointed to by a 32-bit Symbol.

  @param[in,out] Context     Context of the Mach-O.
  @param[in]     Address     Virtual address to retrieve the offset of.
  @param[out]    FileOffset  Pointer the file offset is returned into.
                             If FALSE is returned, the output is undefined.
  @param[out]    MaxSize     Maximum data safely available from FileOffset.

  @retval 0  0 is returned on failure.

**/
BOOLEAN
InternalMachoSymbolGetDirectFileOffset32 (
  IN OUT OC_MACHO_CONTEXT       *Context,
  IN     UINT32                 Address,
  OUT    UINT32                 *FileOffset,
  OUT    UINT32                 *MaxSize OPTIONAL
  );

/**
  Retrieves the Mach-O file offset of the address pointed to by a 64-bit Symbol.

  @param[in,out] Context     Context of the Mach-O.
  @param[in]     Address     Virtual address to retrieve the offset of.
  @param[out]    FileOffset  Pointer the file offset is returned into.
                             If FALSE is returned, the output is undefined.
  @param[out]    MaxSize     Maximum data safely available from FileOffset.

  @retval 0  0 is returned on failure.

**/
BOOLEAN
InternalMachoSymbolGetDirectFileOffset64 (
  IN OUT OC_MACHO_CONTEXT       *Context,
  IN     UINT64                 Address,
  OUT    UINT32                 *FileOffset,
  OUT    UINT32                 *MaxSize OPTIONAL
  );

/**
  Returns the 32-bit Mach-O's virtual address space size.

  @param[out] Context   Context of the Mach-O.

**/
UINT32
InternalMachoGetVmSize32 (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

/**
  Returns the 64-bit Mach-O's virtual address space size.

  @param[out] Context   Context of the Mach-O.

**/
UINT32
InternalMachoGetVmSize64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

/**
  Returns the last virtual address of a 32-bit Mach-O.

  @param[in] Context  Context of the Mach-O.

  @retval 0  The binary is malformed.

**/
UINT32
InternalMachoGetLastAddress32 (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

/**
  Returns the last virtual address of a 64-bit Mach-O.

  @param[in] Context  Context of the Mach-O.

  @retval 0  The binary is malformed.

**/
UINT64
InternalMachoGetLastAddress64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

/**
  Retrieves the next 32-bit Load Command of type LoadCommandType.

  @param[in,out] Context          Context of the Mach-O.
  @param[in]     LoadCommandType  Type of the Load Command to retrieve.
  @param[in]     LoadCommand      Previous Load Command.
                                  If NULL, the first match is returned.

  @retval NULL  NULL is returned on failure.
**/
MACH_LOAD_COMMAND *
InternalMachoGetNextCommand32 (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_LOAD_COMMAND_TYPE   LoadCommandType,
  IN     CONST MACH_LOAD_COMMAND  *LoadCommand  OPTIONAL
  );

/**
  Retrieves the next 64-bit Load Command of type LoadCommandType.

  @param[in,out] Context          Context of the Mach-O.
  @param[in]     LoadCommandType  Type of the Load Command to retrieve.
  @param[in]     LoadCommand      Previous Load Command.
                                  If NULL, the first match is returned.

  @retval NULL  NULL is returned on failure.
**/
MACH_LOAD_COMMAND *
InternalMachoGetNextCommand64 (
  IN OUT OC_MACHO_CONTEXT         *Context,
  IN     MACH_LOAD_COMMAND_TYPE   LoadCommandType,
  IN     CONST MACH_LOAD_COMMAND  *LoadCommand  OPTIONAL
  );

/**
  Returns a pointer to the 32-bit Mach-O file at the specified virtual address.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  Virtual address to look up.    
  @param[out]    MaxSize  Maximum data safely available from FileOffset.
                          If NULL is returned, the output is undefined.

**/
VOID *
InternalMachoGetFilePointerByAddress32 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT32            Address,
  OUT    UINT32            *MaxSize OPTIONAL
  );

/**
  Returns a pointer to the 64-bit Mach-O file at the specified virtual address.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Address  Virtual address to look up.    
  @param[out]    MaxSize  Maximum data safely available from FileOffset.
                          If NULL is returned, the output is undefined.

**/
VOID *
InternalMachoGetFilePointerByAddress64 (
  IN OUT OC_MACHO_CONTEXT  *Context,
  IN     UINT64            Address,
  OUT    UINT32            *MaxSize OPTIONAL
  );

/**
  Expand 32-bit Mach-O image to Destination (make segment file sizes equal to vm sizes).

  @param[in]  Context             Context of the Mach-O.
  @param[in]  CalculateSizeOnly   TRUE to only calcuate a size and not actually expand the image.
  @param[out] Destination         Output buffer.
  @param[in]  DestinationSize     Output buffer maximum size.
  @param[in]  Strip               Output with stripped prelink commands.
  @param[in]  FileOffset          Pointer to the file offset of the first segment.

  @returns  New image size or 0 on failure.

**/
UINT32
InternalMachoExpandImage32 (
  IN  OC_MACHO_CONTEXT   *Context,
  IN  BOOLEAN            CalculateSizeOnly,
  OUT UINT8              *Destination,
  IN  UINT32             DestinationSize,
  IN  BOOLEAN            Strip,
  OUT UINT64             *FileOffset OPTIONAL
  );

/**
  Expand 64-bit Mach-O image to Destination (make segment file sizes equal to vm sizes).

  @param[in]  Context             Context of the Mach-O.
  @param[in]  CalculateSizeOnly   TRUE to only calcuate a size and not actually expand the image.
  @param[out] Destination         Output buffer.
  @param[in]  DestinationSize     Output buffer maximum size.
  @param[in]  Strip               Output with stripped prelink commands.
  @param[in]  FileOffset          Pointer to the file offset of the first segment.

  @returns  New image size or 0 on failure.

**/
UINT32
InternalMachoExpandImage64 (
  IN  OC_MACHO_CONTEXT   *Context,
  IN  BOOLEAN            CalculateSizeOnly,
  OUT UINT8              *Destination,
  IN  UINT32             DestinationSize,
  IN  BOOLEAN            Strip,
  OUT UINT64             *FileOffset OPTIONAL
  );

/**
  Merge 32-bit Mach-O segments into one with lowest protection.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Prefix   Segment prefix to merge.

  @retval TRUE on success

**/
BOOLEAN
InternalMachoMergeSegments32 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *Prefix
  );

/**
  Merge 64-bit Mach-O segments into one with lowest protection.

  @param[in,out] Context  Context of the Mach-O.
  @param[in]     Prefix   Segment prefix to merge.

  @retval TRUE on success

**/
BOOLEAN
InternalMachoMergeSegments64 (
  IN OUT OC_MACHO_CONTEXT     *Context,
  IN     CONST CHAR8          *Prefix
  );

#endif // OC_MACHO_LIB_INTERNAL_H_
