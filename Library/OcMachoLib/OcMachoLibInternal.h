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

/**
  Retrieves the SYMTAB command.

  @param[in] Context  Context of the Mach-O.

  @retval NULL  NULL is returned on failure.

**/
BOOLEAN
InternalRetrieveSymtabs64 (
  IN OUT OC_MACHO_CONTEXT  *Context
  );

/**
  Retrieves an extern Relocation by the address it targets.

  @param[in,out] Context         Context of the Mach-O.
  @param[in]     Address         The address to search for.
  @param[out]    RelocationInfo  The pointer to return the Relocation info
                                 into.
                                 If FALSE is returned, the output is undefined.

  @return  Whether the inspected binary elements are sane.

**/
BOOLEAN
InternalGetExternalRelocationByOffset (
  IN OUT OC_MACHO_CONTEXT      *Context,
  IN     UINT64                Address,
  OUT    MACH_RELOCATION_INFO  **RelocationInfo
  );

#endif // OC_MACHO_LIB_INTERNAL_H_
