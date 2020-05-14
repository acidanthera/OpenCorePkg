//
// FIXME: Fix up TestsUser/Include/Base.h and unify.
//
#ifndef UEFI_COMPAT_H
#define UEFI_COMPAT_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char     CHAR8;
typedef uint16_t CHAR16;
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
typedef intptr_t INTN;
typedef size_t   UINTN;

typedef UINTN         RETURN_STATUS;
typedef RETURN_STATUS EFI_STATUS;

struct EFI_SYSTEM_TABLE_;
typedef struct EFI_SYSTEM_TABLE_ EFI_SYSTEM_TABLE;
typedef void * EFI_HANDLE;

typedef struct {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t  Data4[8];
} EFI_GUID;

#define CONST    const
#define STATIC   static
#define TRUE     true
#define FALSE    false
#define GLOBAL_REMOVE_IF_UNREFERENCED
#define PACKED
#define IN
#define OUT
#define OPTIONAL
#define EFIAPI

#define MAX_INT8      INT8_MAX
#define MAX_INT16     INT16_MAX
#define MAX_INT32     INT32_MAX
#define MAX_INT64     INT64_MAX
#define MAX_INTN      INT64_MAX
#define MAX_UINT8     UINT8_MAX
#define MAX_UINT16    UINT16_MAX
#define MAX_UINT32    UINT32_MAX
#define MAX_UINT64    UINT64_MAX
#define MAX_UINTN     SIZE_MAX
#define MAX_ADDRESS   UINTPTR_MAX

#define ALIGN_VALUE(Value, Alignment) ((Value) + (((Alignment) - (Value)) & ((Alignment) - 1)))
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define PcdGet16(TokenName) _PCD_GET_MODE_16_##TokenName

#define STATIC_ASSERT _Static_assert
#define ASSERT assert
#define DEBUG(...)

#if !defined(_MSC_VER) || defined (__clang__)
#define SwapBytes16 __builtin_bswap16
#define SwapBytes32 __builtin_bswap32
#define SwapBytes64 __builtin_bswap64
#else
#define SwapBytes16 _byteswap_ushort
#define SwapBytes32 _byteswap_ulong
#define SwapBytes64 _byteswap_uint64
#endif
#define LShiftU64(A, B) ((UINT64)(A) << (UINTN)(B))
#define RShiftU64(A, B) ((UINT64)(A) >> (UINTN)(B))

#define ZeroMem(Dst, Size) (memset)((Dst), 0, (Size))
#define CopyMem(Dst, Src, Size) (memcpy)((Dst), (Src), (Size))
#define CompareMem(One, Two, Size) (memcmp)((One),(Two),(Size))

#define AllocatePool(Size) (malloc)(Size)
#define FreePool(Ptr)      (free)(Ptr)

#endif // UEFI_COMPAT_H
