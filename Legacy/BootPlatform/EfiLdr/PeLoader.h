/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PeLoader.h

Abstract:

Revision History:

--*/

#ifndef _EFILDR_PELOADER_H_
#define _EFILDR_PELOADER_H_

#include "EfiLdr.h"

EFI_STATUS
EfiLdrGetPeImageInfo (
  IN VOID                     *FHand,
  OUT UINT64                  *ImageBase,
  OUT UINT32                  *ImageSize
  );

EFI_STATUS
EfiLdrPeCoffLoadPeImage (
  IN VOID                     *FHand,
  IN EFILDR_LOADED_IMAGE      *Image,
  IN UINTN                    *NumberOfMemoryMapEntries,
  IN EFI_MEMORY_DESCRIPTOR    *EfiMemoryDescriptor
  );


#endif
