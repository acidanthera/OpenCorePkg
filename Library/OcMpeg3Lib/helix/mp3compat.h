/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef MP3_COMPAT_H
#define MP3_COMPAT_H

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#ifdef memset
#undef memset
#endif

#ifdef memcpy
#undef memcpy
#endif

#ifdef memmove
#undef memmove
#endif

#ifdef malloc
#undef malloc
#endif

#ifdef free
#undef free
#endif

#define memset(Dst, Val, Size) SetMem ((Dst), (Size), (Val))
#define memcpy(dst, src, len) CopyMem(dst, src, len)
#define memmove(dst, src, len) CopyMem(dst, src, len)

#define malloc(Size) AllocatePool (Size)
#define free(Ptr) FreePool (Ptr)

#endif // MP3_COMPAT_H
