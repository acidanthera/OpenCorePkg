/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  Support.h

Abstract:

Revision History:

--*/

#ifndef _EFILDR_SUPPORT_H_
#define _EFILDR_SUPPORT_H_

EFI_STATUS
EfiAddMemoryDescriptor(
  UINTN                 *NoDesc,
  EFI_MEMORY_DESCRIPTOR *Desc,
  EFI_MEMORY_TYPE       Type,
  EFI_PHYSICAL_ADDRESS  BaseAddress,
  UINT64                NoPages,
  UINT64                Attribute
  );

UINTN
FindSpace(
  UINTN                       NoPages,
  IN UINTN                    *NumberOfMemoryMapEntries,
  IN EFI_MEMORY_DESCRIPTOR    *EfiMemoryDescriptor,
  EFI_MEMORY_TYPE             Type,
  UINT64                      Attribute
  );

VOID
GenMemoryMap (
  UINTN                 *NumberOfMemoryMapEntries,
  EFI_MEMORY_DESCRIPTOR *EfiMemoryDescriptor,
  BIOS_MEMORY_MAP       *BiosMemoryMap
  );

#endif
