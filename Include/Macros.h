/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/


#ifndef OPEN_CORE_MACROS_H_
#define OPEN_CORE_MACROS_H_

#ifndef FUNCTION_NAME
  #ifdef _MSC_VER
    #define FUNCTION_NAME  __FUNCTION__
  #else
    #define FUNCTION_NAME  __func__
  #endif
#endif

// FIXME: Find EFI original decls

#define EFI_MAX_PATH_LENGTH		0x200
#define EFI_MAX_PATH_SIZE   	(EFI_MAX_PATH_LENGTH * sizeof (CHAR16))

// PTR_OFFSET
/// Adds Offset bytes to SourcePtr and returns new pointer as ReturnType.
#define PTR_OFFSET(SourcePtr, Offset, ReturnType) ((ReturnType)(((UINT8 *)(UINTN)SourcePtr) + Offset))

#define QUAD(hi, lo)    (((UINT64)(hi)) << 32 | (lo))
#define BIT(n)          (1ULL << (n))
#define BITMASK(h, l)   ((BIT (h) | (BIT (h) - 1)) & ~(BIT (l) - 1))
#define BITFIELD(x,h,l) (((x) & BITMASK(h, l)) >> l)

#define IS_DIGIT(a) (((a) >= '0') && ((a) <= '9'))
#define IS_HEX(a)   (((a) >= 'a') && ((a) <= 'f'))
#define IS_UPPER(a) (((a) >= 'A') && ((a) <= 'Z'))
#define IS_ALPHA(x) ((((x) >= 'a') && ((x) <='z')) || (((x) >= 'A') && ((x) <= 'Z')))
#define IS_ASCII(x) (((x) >= 0x20) && ((x) <= 0x7F))
#define IS_PUNCT(x) (((x) == '.') || ((x) == '-'))

#define SWAP16(V) ((((UINT16)((V) & 0x00FF)) << 8)  \
                 | (((UINT16)((V) & 0xFF00)) >> 8))

#define SWAP32(V) ((((UINT32)(V) & 0x000000FF) << 24)   \
                 | (((UINT32)(V) & 0x0000FF00) << 8)  \
                 | (((UINT32)(V) & 0x00FF0000) >> 8)  \
                 | (((UINT32)(V) & 0xFF000000) >> 24))

#define SWAP64(V) (((((UINT64)(V)) << 56) & 0xFF00000000000000ULL)  \
                 | ((((UINT64)(V)) << 40) & 0x00FF000000000000ULL)  \
                 | ((((UINT64)(V)) << 24) & 0x0000FF0000000000ULL)  \
                 | ((((UINT64)(V)) << 8)  & 0x000000FF00000000ULL)  \
                 | ((((UINT64)(V)) >> 8)  & 0x00000000FF000000ULL)  \
                 | ((((UINT64)(V)) >> 24) & 0x0000000000FF0000ULL)  \
                 | ((((UINT64)(V)) >> 40) & 0x000000000000FF00ULL)  \
                 | ((((UINT64)(V)) >> 56) & 0x00000000000000FFULL))

#endif // OPEN_CORE_MACROS_H_
