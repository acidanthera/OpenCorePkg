/** @file
  Copyright (c) 2020, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Library/DebugLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcFileLib.h>

EFI_STATUS
OcGetFileSize (
  IN  EFI_FILE_PROTOCOL  *File,
  OUT UINT32             *Size
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
