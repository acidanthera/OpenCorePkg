/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FlashLayout.h
   
Abstract:

  Platform specific flash layout

**/

#ifndef _EFI_FLASH_LAYOUT
#define _EFI_FLASH_LAYOUT

#include <EfiFlashMap.h>

//
// Firmware Volume Information for DUET
//
#define FV_BLOCK_SIZE               0x10000
#define FV_BLOCK_MASK               0x0FFFF
#define EFI_RUNTIME_UPDATABLE_FV_HEADER_LENGTH  (sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY))

#define NV_STORAGE_SIZE             0x4000
#define NV_STORAGE_FVB_SIZE         ((NV_STORAGE_SIZE + EFI_RUNTIME_UPDATABLE_FV_HEADER_LENGTH + FV_BLOCK_MASK) & ~FV_BLOCK_MASK)
#define NV_STORAGE_FVB_BLOCK_NUM    (NV_STORAGE_FVB_SIZE / FV_BLOCK_SIZE)

#define NV_FTW_WORKING_SIZE         0x2000
#define NV_FTW_SPARE_SIZE           0x10000
#define NV_FTW_FVB_SIZE             ((NV_FTW_WORKING_SIZE + NV_FTW_SPARE_SIZE + EFI_RUNTIME_UPDATABLE_FV_HEADER_LENGTH + FV_BLOCK_MASK) & ~FV_BLOCK_MASK)
#define NV_FTW_FVB_BLOCK_NUM        (NV_FTW_FVB_SIZE / FV_BLOCK_SIZE)

#define NV_STORAGE_FILE_PATH        L".\\Efivar.bin"

#define BOOT1_BASE                  ((UINTN) 0xE000U)
#define BOOT1_MAGIC                 0xAA55

typedef struct {
  UINT8  LoaderCode[496];
  UINT8  Signature[14];
  UINT16 Magic;
} BOOT1_LOADER;

#endif // _EFI_FLASH_LAYOUT
