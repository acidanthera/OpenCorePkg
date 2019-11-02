/** @file

OcGuardLib

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef UBSAN_H
#define UBSAN_H

#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

// Pretend UEFI to mean kernel mode.
#ifndef _KERNEL
#define _KERNEL 1
#endif

#if !defined(HAVE_UBSAN_SUPPORT) && defined(__clang__)
#define HAVE_UBSAN_SUPPORT 1
#endif


// Mark long double as supported (since we may use softfp).
#ifndef __HAVE_LONG_DOUBLE
#define __HAVE_LONG_DOUBLE
#endif

// Non-BSD environments do not support RCSID.
#ifndef __RCSID
#define __RCSID(x)
#endif

#ifndef __KERNEL_RCSID
#define __KERNEL_RCSID(x, s) __RCSID(s)
#endif

// Implement builtin via macros, because some toolchains do include stdint.h.
// This works only because Ubsan.c is not allowed to have any header inclusion.
#define int8_t  INT8
#define int16_t INT16
#define int32_t INT32
#define int64_t INT64
#define uint8_t  UINT8
#define uint16_t UINT16
#define uint32_t UINT32
#define uint64_t UINT64
#define bool BOOLEAN
#define intptr_t INTN
#define uintptr_t UINTN

// Try to provide a somewhat adequate size_t implementation.
// Have to be careful about -Wformat warnings in Clang.
#ifdef __GNUC__
#if defined(MDE_CPU_X64)
#define ssize_t long
#define size_t unsigned long
#elif defined(MDE_CPU_IA32)
#define ssize_t int
#define size_t unsigned int
#else
#error Unknown CPU arch
#endif
#else
#define ssize_t INTN
#define size_t UINTN
#endif

// Provide boolean declarations if missing.
#ifndef false
#define false FALSE
#define true TRUE
#endif

// Provide va_list declarations.
#define va_list VA_LIST
#define va_start VA_START
#define va_end VA_END
#define va_arg VA_ARG

// Printing macros are not supported in EDK2.
#ifndef PRIx8
#define PRIx8 "hhx"
#define PRIx16 "hx"
#define PRIx32 "x"
#define PRIx64 "llx"
#define PRId32 "d"
#define PRId64 "lld"
#define PRIu32 "u"
#define PRIu64 "llu"
#endif

// Hack limits in.
#ifndef UINT16_MAX
#define UINT8_MAX   0xffU
#define UINT16_MAX  0xffffU
#define UINT32_MAX  0xffffffffU
#define UINT64_MAX  0xffffffffffffffffULL
#endif

// Avoid ASSERT/KASSERT conflict from DebugLib.
#ifdef ASSERT
#undef ASSERT
#endif

// Implement KASSERT as the original EDK2 ASSERT.
#if !defined(MDEPKG_NDEBUG)
  #define KASSERT(Expression)        \
    do {                            \
      if (DebugAssertEnabled ()) {  \
        if (!(Expression)) {        \
          _ASSERT (Expression);     \
          ANALYZER_UNREACHABLE ();  \
        }                           \
      }                             \
    } while (FALSE)
#else
  #define KASSERT(Expression)
#endif

// PATH_MAX is an unoffical extension.
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

// CHAR_BIT may not be defined without limits.h.
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

// Bit manipulation is not present
#ifndef __BIT
#define __BIT(__n) \
  (((UINTN)(__n) >= CHAR_BIT * sizeof(UINTN)) ? 0 : \
  ((UINTN)1 << (UINTN)((__n) & (CHAR_BIT * sizeof(UINTN) - 1))))
#endif

// Extended bit manipulation is also not present
#ifndef __LOWEST_SET_BIT
/* find least significant bit that is set */
#define __LOWEST_SET_BIT(__mask) ((((__mask) - 1) & (__mask)) ^ (__mask))
#define __SHIFTOUT(__x, __mask) (((__x) & (__mask)) / __LOWEST_SET_BIT(__mask))
#define __SHIFTIN(__x, __mask) ((__x) * __LOWEST_SET_BIT(__mask))
#define __SHIFTOUT_MASK(__mask) __SHIFTOUT((__mask), (__mask))
#endif

// BSD-like bit manipulation is not present
#ifndef CLR
#define SET(t, f) ((t) |= (f))
#define ISSET(t, f) ((t) & (f))
#define CLR(t, f) ((t) &= ~(f))
#endif

// Note, that we do not use UEFI-like printf.
#ifndef __printflike
#ifdef __GNUC__
#define __printflike(x, y) __attribute__((format(printf, (x), (y))))
#else
#define __printflike(x, y)
#endif
#endif

// Route arraycount to ARRAY_SIZE
#ifndef __arraycount
#define __arraycount(a) ARRAY_SIZE (a)
#endif

// Support unreachable where possible
#ifndef __unreachable
#ifdef __GNUC__
#define __unreachable() __builtin_unreachable()
#else
#define __unreachable()
#endif
#endif

// C-style printing support.
#define TINYPRINTF_DEFINE_TFP_SPRINTF 1
typedef void (*putcf) (void *, char);
void EFIAPI tfp_format(void *putp, putcf putf, const char *fmt, va_list va);
int tfp_vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int EFIAPI tfp_snprintf(char *str, size_t size, const char *fmt, ...) __printflike(3, 4);
int EFIAPI tfp_vsprintf(char *str, const char *fmt, va_list ap);
int EFIAPI tfp_sprintf(char *str, const char *fmt, ...) __printflike(2, 3);

// Redirect log printing to standard output.
#define snprintf tfp_snprintf
#define vprintf(f, v) \
  do { CHAR8 Tmp__[1024]; tfp_vsnprintf (Tmp__, sizeof (Tmp__), f, v); AsciiPrint ("%a", Tmp__); } while (0)
#define vpanic(f, v) \
  do { vprintf (f, v); do { } while (1); } while (0)

// Avoid implementing memcpy as a function to avoid LTO conflicts.
#define memcpy(Dst, Src, Size) do { gBS->CopyMem(Dst, Src, Size); } while (0)

// Forcing VOID for those as the return types actually differ.
#define strlcpy(Dst, Src, Size) do { AsciiStrnCpyS (Dst, Size, Src, AsciiStrLen (Src)); } while (0)
#define strlcat(Dst, Src, Size) do { AsciiStrnCatS (Dst, Size, Src, AsciiStrLen (Src)); } while (0)


#endif // UBSAN_H
