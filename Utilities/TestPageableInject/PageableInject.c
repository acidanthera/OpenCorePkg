/** @file
  Copyright (C) 2022, flagers. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcTemplateLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcAppleKernelLib.h>

#include <string.h>
#include <sys/time.h>

#include <UserFile.h>

STATIC BOOLEAN  FailedToProcess = FALSE;
STATIC UINT32   KernelVersion   = 0;

STATIC EFI_FILE_PROTOCOL  BaseFileProtocol;
STATIC EFI_FILE_PROTOCOL  PageableFileProtocol;

STATIC UINT8   *mBase        = NULL;
STATIC UINT32  mBaseSize     = 0;
STATIC UINT8   *mPageable    = NULL;
STATIC UINT32  mPageableSize = 0;

EFI_STATUS
OcGetFileData (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINT32             Position,
  IN  UINT32             Size,
  OUT UINT8              *Buffer
  )
{
  UINT32  FileSize;
  UINT8   *FileBuffer;
  ASSERT ((File == &BaseFileProtocol) || (File == &PageableFileProtocol));

  FileSize = (File == &BaseFileProtocol) ? mBaseSize : mPageableSize;
  FileBuffer = (File == &BaseFileProtocol) ? mBase : mPageable;

  if ((UINT64)Position + Size > (FileSize)) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&Buffer[0], &FileBuffer[Position], Size);
  return EFI_SUCCESS;
}

EFI_STATUS
OcGetFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  )
{
  ASSERT ((File == &BaseFileProtocol) || (File == &PageableFileProtocol));
  *Size = (File == &BaseFileProtocol) ? mBaseSize : mPageableSize;
  return EFI_SUCCESS;
}

int
WrapMain (
  int   argc,
  char  *argv[]
  )
{
  UINT32             BaseAllocSize;
  UINT32             PageableAllocSize;
  PAGEABLE_CONTEXT   Context;

  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO | DEBUG_VERBOSE;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO | DEBUG_VERBOSE;
  PcdGet8 (PcdDebugPropertyMask)          |= DEBUG_PROPERTY_DEBUG_CODE_ENABLED;

  CONST CHAR8  *BaseFileName;
  CONST CHAR8  *PageableFileName;

  BaseFileName = argc > 1 ? argv[1] : "/System/Library/KernelCollections/BootKernelExtensions.kc";
  PageableFileName = argc > 2 ? argv[2] : "/System/Library/KernelCollections/SystemKernelExtensions.kc";

  if ((mBase = UserReadFile (BaseFileName, &mBaseSize)) == NULL) {
    DEBUG ((DEBUG_ERROR, "Read fail %a\n", BaseFileName));
    return -1;
  }
  if ((mPageable = UserReadFile (PageableFileName, &mPageableSize)) == NULL) {
    DEBUG ((DEBUG_ERROR, "Read fail %a\n", PageableFileName));
    return -1;
  }

  // TODO: Determine reserved size for kexts to be injected here

  KernelVersion = OcKernelReadDarwinVersion (mBase, mBaseSize);
  if (KernelVersion == 0) {
    DEBUG ((DEBUG_WARN, "[FAIL] Failed to detect version\n"));
    FailedToProcess = TRUE;
  } else {
    DEBUG ((DEBUG_WARN, "[OK] Got version %u\n", KernelVersion));
  }
  ASSERT (OcMatchDarwinVersion (KernelVersion, OcParseDarwinVersion("20.00.00"), 0));

  Status = PageableContextInit (&Context, mBase, mBaseSize, BaseAllocSize, FALSE);

  return 0;
}

int
ENTRY_POINT (
  int   argc,
  char  *argv[]
  )
{
  int  code;

  code = WrapMain (argc, argv);
  if (FailedToProcess) {
    code = -1;
  }

  return code;
}
