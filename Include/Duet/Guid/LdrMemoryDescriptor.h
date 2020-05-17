/** @file
  Guid is for GUIDED HOB of LDR memory descriptor.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __LDR_MEMORY_DESCRIPTOR__
#define __LDR_MEMORY_DESCRIPTOR__

#define LDR_MEMORY_DESCRIPTOR_GUID \
  { 0x7701d7e5, 0x7d1d, 0x4432, {0xa4, 0x68, 0x67, 0x3d, 0xab, 0x8a, 0xde, 0x60}}

#pragma pack(1)

typedef struct {
  EFI_HOB_GUID_TYPE             Hob;
  UINTN                         MemDescCount;
  EFI_MEMORY_DESCRIPTOR         *MemDesc;
} MEMORY_DESC_HOB;

#pragma pack()

extern EFI_GUID gLdrMemoryDescriptorGuid;

#endif
