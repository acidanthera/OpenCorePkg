/** @file
  Copyright (c) 2020, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Library/DebugLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcFileLib.h>

EFI_MEMORY_DESCRIPTOR *
OcGetCurrentMemoryMap (
  OUT UINTN   *MemoryMapSize,
  OUT UINTN   *DescriptorSize,
  OUT UINTN   *MapKey                 OPTIONAL,
  OUT UINT32  *DescriptorVersion      OPTIONAL,
  OUT UINTN   *OriginalMemoryMapSize  OPTIONAL,
  IN  BOOLEAN IncludeSplitSpace
  )
{
  ASSERT (FALSE);
  return NULL;
}

EFI_STATUS
GetFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
