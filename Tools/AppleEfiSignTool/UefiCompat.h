#ifndef UEFI_COMPAT_H
#define UEFI_COMPAT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef int8_t   INT8;
typedef int16_t  INT16;
typedef int32_t  INT32;
typedef int64_t  INT64;
typedef uint8_t  UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef bool     BOOLEAN;
typedef void     VOID;
typedef size_t   UINTN;

#define CONST    const
#define STATIC   static
#define TRUE     true
#define FALSE    false

#define ZeroMem(Dst, Size) (memset)((Dst), 0, (Size))
#define CopyMem(Dst, Src, Size) (memcpy)((Dst), (Src), (Size))
#define CompareMem(One, Two, Size) (memcmp)((One),(Two),(Size))

#endif // UEFI_COMPAT_H
