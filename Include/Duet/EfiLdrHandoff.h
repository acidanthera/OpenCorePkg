/** @file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  EfiLdrHandoff.h

Abstract:

Revision History:

**/

#ifndef _EFILDR_HANDOFF_H_
#define _EFILDR_HANDOFF_H_

#include <Base.h>
#include <Uefi.h>

#define EFILDR_BASE_SEGMENT 0x2000
#define EFILDR_LOAD_ADDRESS        (EFILDR_BASE_SEGMENT << 4)
#define EFILDR_HEADER_ADDRESS      (EFILDR_LOAD_ADDRESS+0x2000)

#define EFILDR_CB_VA        0x00

typedef struct _EFILDRHANDOFF {
    UINTN                    MemDescCount;
    EFI_MEMORY_DESCRIPTOR   *MemDesc;
    VOID                    *BfvBase;
    UINTN                   BfvSize;
    VOID                    *DxeIplImageBase;
    UINTN                   DxeIplImageSize;
    VOID                    *DxeCoreImageBase;
    UINTN                   DxeCoreImageSize;
    VOID                    *DxeCoreEntryPoint;
} EFILDRHANDOFF;

typedef struct {
    UINT32       CheckSum;
    UINT32       Offset;
    UINT32       Length;
    UINT8        FileName[52];
} EFILDR_IMAGE;

typedef struct {          
    UINT32       Signature;     
    UINT32       HeaderCheckSum;
    UINT32       FileLength;
    UINT32       NumberOfImages;
} EFILDR_HEADER;

#endif
