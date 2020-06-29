/** @file
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/DebugLib.h>

#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <File.h>

/*
 for fuzzing (TODO):
 clang-mp-7.0 -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../MdePkg/Include/ -include ../Include/Base.h -I../../../EfiPkg/Include/ Macho.c  ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c  ../../Library/OcMachoLib/CxxSymbols.c ../../Library/OcMachoLib/Header.c ../../Library/OcMachoLib/Relocations.c ../../Library/OcMachoLib/Symbols.c -o Macho
 rm -rf DICT fuzz*.log ; mkdir DICT ; cp /System/Library/Kernels/kernel DICT ; ./Macho -rss_limit_mb=4096M -jobs=4 DICT

 rm -rf fuzz*.log ; mkdir -p DICT ; cp /System/Library/Kernels/kernel DICT/kernel ; ./Macho -jobs=4 -rss_limit_mb=4096M DICT

 rm -rf Macho.dSYM DICT fuzz*.log Macho
*/

extern UINT8 LiluKextData[];
extern UINT32 LiluKextDataSize;
extern CHAR8 LiluKextInfoPlistData[];
extern UINT32 LiluKextInfoPlistDataSize;
extern UINT8 VsmcKextData[];
extern UINT32 VsmcKextDataSize;
extern CHAR8 VsmcKextInfoPlistData[];
extern UINT32 VsmcKextInfoPlistDataSize;

static int FeedMacho(void *file, uint32_t size) {
  OC_MACHO_CONTEXT Context;
  if (!MachoInitializeContext (&Context, file, size)) {
    DEBUG ((DEBUG_WARN, "MachoInitializeContext failure\n"));
    return -1;
  }

/*
  For MH_FILELIST Mach-O as determined by checking Mach-O header:

  1. Save original Mach-O header.
  2. Find __TEXT segment:
  - make sure its offset is 0.
  - make sure its memory size and file size equal.
  3. Calculate DIFF = NEW_SIZE - file size.
  4. Write NEW_SIZE to memory size and file size of __TEXT.
  5. For each segment, that is not text:
  - Increase file_offset by DIFF.
  - Increase vm_offset by DIFF.
  6. For each dyld fixup:
  - Increase target by DIFF, becuase the offset will not change for anything.
  7. Insert DIFF bytes between __TEXT and other segments.

VRAM            FILE           FILE NEW                    VRAM
                       
__HIB          __TEXT         __TEXT         (bigger)
__TEXT         __TEXT_EXEC    __TEXT_EXEC    (lower)
__TEXT_EXEC    __DATA_CONST   __DATA_CONST   (lower)
__DATA_CONST   __PRELINK_INFO <empty space>
__PRELINK_INFO __DATA         __DATA         (same off)
__DATA         __HIB          __HIB          (same off)
__REGION0      __REGION0      __REGION0      (same off)
...            ...            ...
__LINKEDIT     __LINKEDIT     __REGIONX      (new kext)
                              __PRELINK_INFO (bigger)
                              __LINKEDIT     (bigger?)                  

*/


  BOOLEAN shr = MachoMergeSegments64 (&Context, "__REGION");
  DEBUG ((DEBUG_WARN, "Shrinking __REGION is %d\n", shr));

  writeFile("out.bin", file, size);

  return 0;
}

EFI_STATUS
GetFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINT32             Position,
  IN  UINT32             Size,
  OUT UINT8              *Buffer
  )
{
  abort();
  return EFI_SUCCESS;
}

EFI_STATUS
GetFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  )
{
  abort();
  return EFI_SUCCESS;
}


int main(int argc, char** argv) {
  uint32_t PrelinkedSize;
  uint8_t *Prelinked;
  UINT32 AllocSize;
  PRELINKED_CONTEXT Context;

  if ((Prelinked = readFile(argc > 1 ? argv[1] : "/System/Library/KernelCollections/BootKernelExtensions.kc", &PrelinkedSize)) == NULL) {
    printf("Read fail\n");
    return -1;
  }


  AllocSize = MACHO_ALIGN (PrelinkedSize + 1*1024*1024);

  Prelinked = realloc (Prelinked, AllocSize);
  if (Prelinked == NULL) {
    printf("Realloc fail\n");
    abort();
    return -1;
  }

#if 0
  ApplyKernelPatches (Prelinked, PrelinkedSize);
#endif

  EFI_STATUS Status = PrelinkedContextInit (&Context, Prelinked, PrelinkedSize, AllocSize);

  if (!EFI_ERROR (Status)) {

    Status = PrelinkedInjectPrepare (&Context, EFI_PAGE_SIZE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Prelink inject prepare error %r\n", Status));
      return -1;
    }

    Status = PrelinkedInjectKext (
      &Context,
      "/Library/Extensions/Lilu.kext",
      LiluKextInfoPlistData,
      LiluKextInfoPlistDataSize,
      "Contents/MacOS/Lilu",
      LiluKextData,
      LiluKextDataSize
      );

    DEBUG ((DEBUG_WARN, "%a injected - %r\n", "Lilu.kext", Status));


    Status = PrelinkedInjectKext (
      &Context,
      "/Library/Extensions/VirtualSMC.kext",
      VsmcKextInfoPlistData,
      VsmcKextInfoPlistDataSize,
      "Contents/MacOS/VirtualSMC",
      VsmcKextData,
      VsmcKextDataSize
      );

    DEBUG ((DEBUG_WARN, "VirtualSMC.kext injected - %r\n", Status));

    Status = PrelinkedInjectComplete (&Context);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Prelink inject complete error %r\n", Status));
    }

    writeFile("out.bin", Prelinked, Context.PrelinkedSize);
    if (!EFI_ERROR (Status)) {
      printf("All good\n");
    } else {
      printf("Inject error\n");
    }

    PrelinkedContextFree (&Context);
  } else {
    DEBUG ((DEBUG_WARN, "Context creation error %r\n", Status));
  }

  free(Prelinked);
 
  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  if (Size > 0) {
    VOID *NewData = AllocatePool (Size);
    if (NewData) {
      CopyMem (NewData, Data, Size);
      FeedMacho (NewData, (UINT32) Size);
      FreePool (NewData);
    }
  }
  return 0;
}
