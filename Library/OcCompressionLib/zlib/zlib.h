/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef ZLIB_INTERNAL_H
#define ZLIB_INTERNAL_H

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcCompressionLib.h>

#ifdef memset
#undef memset
#endif

#ifndef assert
#define assert ASSERT
#endif

#define memset(Dst, Value, Size) SetMem ((Dst), (Size), (UINT8)(Value))

#define salloc(size) AllocatePool (size)
#define snew(type) ( (type *) AllocatePool (sizeof(type)) )
#define snewn(n, type) ( (type *) AllocatePool ((n) * sizeof(type)) )
#define sfree(x) ( FreePool ((x)) )
#define sresize(OldBuffer, OldSize, NewSize, Type)  \
  ((Type *) ReallocatePool((OldSize) * sizeof (Type), (NewSize) * sizeof (Type), (OldBuffer)))

#endif // ZLIB_INTERNAL_H
