/** @file
  Copyright (C) 2016 - 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/


#ifndef UEFI_BASE_H
#define UEFI_BASE_H

//
// Includes
//
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <cpuid.h>

#ifndef RSIZE_MAX
#define RSIZE_MAX (SIZE_MAX >> 1)
#endif

//
// Types and limits
//

#define MAX_INT8      INT8_MAX
#define MAX_INT16     INT16_MAX
#define MAX_INT32     INT32_MAX
#define MAX_INT64     INT64_MAX
#define MAX_INTN      INT64_MAX
#define MAX_UINT8     UINT8_MAX
#define MAX_UINT16    UINT16_MAX
#define MAX_UINT32    UINT32_MAX
#define MAX_UINT64    UINT64_MAX
#define MAX_UINTN     UINT64_MAX
#define MAX_BIT       0x8000000000000000
#define EFI_PAGE_SIZE  0x1000
#define EFI_PAGE_MASK  0xFFF
#define EFI_PAGE_SHIFT 12

#define MIN_INT8   (((INT8)  -127) - 1)
#define MIN_INT16  (((INT16) -32767) - 1)
#define MIN_INT32  (((INT32) -2147483647) - 1)
#define MIN_INT64  (((INT64) -9223372036854775807LL) - 1)

#define MDE_CPU_X64
#define VOID void
#define CONST const
#define STATIC static
typedef char CHAR8;
typedef unsigned short CHAR16;
typedef signed char INT8;
typedef unsigned char UINT8;
typedef ssize_t INTN;
typedef size_t UINTN;
typedef bool BOOLEAN;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef size_t EFI_STATUS;
typedef UINTN RETURN_STATUS;
typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;
typedef VOID *EFI_HANDLE;
typedef VOID *EFI_EVENT;
typedef UINTN *BASE_LIST;
typedef UINT64 EFI_LBA;

typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} GUID;

typedef struct {
  UINT16  Year;
  UINT8   Month;
  UINT8   Day;
  UINT8   Hour;
  UINT8   Minute;
  UINT8   Second;
  UINT8   Pad1;
  UINT32  Nanosecond;
  INT16   TimeZone;
  UINT8   Daylight;
  UINT8   Pad2;
} EFI_TIME;

typedef struct {
  UINT8 Addr[4];
} IPv4_ADDRESS;

typedef struct {
  UINT8 Addr[16];
} IPv6_ADDRESS;

typedef enum {
  AllocateAnyPages,
  AllocateMaxAddress,
  AllocateAddress,
  MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
  EfiReservedMemoryType,
  EfiLoaderCode,
  EfiLoaderData,
  EfiBootServicesCode,
  EfiBootServicesData,
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiConventionalMemory,
  EfiUnusableMemory,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS,
  EfiMemoryMappedIO,
  EfiMemoryMappedIOPortSpace,
  EfiPalCode,
  EfiPersistentMemory,
  EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef enum {
  EFI_NATIVE_INTERFACE
} EFI_INTERFACE_TYPE;

///
/// Enumeration of EFI Locate Search Types
///
typedef enum {
  AllHandles,
  ByRegisterNotify,
  ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

typedef struct {
  UINT8 Addr[32];
} EFI_MAC_ADDRESS;

typedef IPv4_ADDRESS EFI_IPv4_ADDRESS;
typedef IPv6_ADDRESS EFI_IPv6_ADDRESS;

typedef union {
  UINT32            Addr[4];
  EFI_IPv4_ADDRESS  v4;
  EFI_IPv6_ADDRESS  v6;
} EFI_IP_ADDRESS;

typedef GUID EFI_GUID;
typedef struct EFI_SYSTEM_TABLE_ EFI_SYSTEM_TABLE;
typedef struct EFI_BOOT_SERVICES_ EFI_BOOT_SERVICES;
typedef struct EFI_RUNTIME_SERVICES_ EFI_RUNTIME_SERVICES;
typedef VOID (*EFI_EVENT_NOTIFY)(EFI_EVENT Event, VOID *Context);

typedef struct _LIST_ENTRY LIST_ENTRY;

struct _LIST_ENTRY {
  LIST_ENTRY  *ForwardLink;
  LIST_ENTRY  *BackLink;
};

typedef struct {
  UINT32                Type;
  EFI_PHYSICAL_ADDRESS  PhysicalStart;
  EFI_VIRTUAL_ADDRESS   VirtualStart;
  UINT64                NumberOfPages;
  UINT64                Attribute;
} EFI_MEMORY_DESCRIPTOR;

#ifndef NEXT_MEMORY_DESCRIPTOR
#define NEXT_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) + (Size)))
#endif

#define MAX_ADDRESS   0xFFFFFFFFFFFFFFFFULL

#define IN
#define OUT
#define OPTIONAL
#define EFIAPI
#define TRUE true
#define FALSE false
#define VA_LIST va_list
#define VA_ARG va_arg
#define VA_START va_start
#define VA_END va_end
#define VERIFY_SIZE_OF(TYPE, Size) extern unsigned char _VerifySizeof##TYPE[(sizeof(TYPE) == (Size)) / (sizeof(TYPE) == (Size))]
#define EFI_D_ERROR stderr
#define EFI_D_VERBOSE stdout
#define DEBUG_ERROR stderr
#define DEBUG_INFO stdout
#define DEBUG_VERBOSE stdout
#define DEBUG_WARN stderr
#define PACKED
#define GLOBAL_REMOVE_IF_UNREFERENCED

//
// Return codes
//
#define ENCODE_ERROR(a)              ((RETURN_STATUS)(MAX_BIT | (a)))

#define ENCODE_WARNING(a)            ((RETURN_STATUS)(a))
#define RETURN_ERROR(a)              (((INTN)(RETURN_STATUS)(a)) < 0)

#define RETURN_SUCCESS               0
#define RETURN_LOAD_ERROR            ENCODE_ERROR (1)
#define RETURN_INVALID_PARAMETER     ENCODE_ERROR (2)
#define RETURN_UNSUPPORTED           ENCODE_ERROR (3)
#define RETURN_BAD_BUFFER_SIZE       ENCODE_ERROR (4)
#define RETURN_BUFFER_TOO_SMALL      ENCODE_ERROR (5)
#define RETURN_NOT_READY             ENCODE_ERROR (6)
#define RETURN_DEVICE_ERROR          ENCODE_ERROR (7)
#define RETURN_WRITE_PROTECTED       ENCODE_ERROR (8)
#define RETURN_OUT_OF_RESOURCES      ENCODE_ERROR (9)
#define RETURN_VOLUME_CORRUPTED      ENCODE_ERROR (10)
#define RETURN_VOLUME_FULL           ENCODE_ERROR (11)
#define RETURN_NO_MEDIA              ENCODE_ERROR (12)
#define RETURN_MEDIA_CHANGED         ENCODE_ERROR (13)
#define RETURN_NOT_FOUND             ENCODE_ERROR (14)
#define RETURN_ACCESS_DENIED         ENCODE_ERROR (15)
#define RETURN_NO_RESPONSE           ENCODE_ERROR (16)
#define RETURN_NO_MAPPING            ENCODE_ERROR (17)
#define RETURN_TIMEOUT               ENCODE_ERROR (18)
#define RETURN_NOT_STARTED           ENCODE_ERROR (19)
#define RETURN_ALREADY_STARTED       ENCODE_ERROR (20)
#define RETURN_ABORTED               ENCODE_ERROR (21)
#define RETURN_ICMP_ERROR            ENCODE_ERROR (22)
#define RETURN_TFTP_ERROR            ENCODE_ERROR (23)
#define RETURN_PROTOCOL_ERROR        ENCODE_ERROR (24)
#define RETURN_INCOMPATIBLE_VERSION  ENCODE_ERROR (25)
#define RETURN_SECURITY_VIOLATION    ENCODE_ERROR (26)
#define RETURN_CRC_ERROR             ENCODE_ERROR (27)
#define RETURN_END_OF_MEDIA          ENCODE_ERROR (28)
#define RETURN_END_OF_FILE           ENCODE_ERROR (31)
#define RETURN_INVALID_LANGUAGE      ENCODE_ERROR (32)
#define RETURN_COMPROMISED_DATA      ENCODE_ERROR (33)
#define RETURN_HTTP_ERROR            ENCODE_ERROR (35)

#define EFI_SUCCESS               RETURN_SUCCESS
#define EFI_LOAD_ERROR            RETURN_LOAD_ERROR
#define EFI_INVALID_PARAMETER     RETURN_INVALID_PARAMETER
#define EFI_UNSUPPORTED           RETURN_UNSUPPORTED
#define EFI_BAD_BUFFER_SIZE       RETURN_BAD_BUFFER_SIZE
#define EFI_BUFFER_TOO_SMALL      RETURN_BUFFER_TOO_SMALL
#define EFI_NOT_READY             RETURN_NOT_READY
#define EFI_DEVICE_ERROR          RETURN_DEVICE_ERROR
#define EFI_WRITE_PROTECTED       RETURN_WRITE_PROTECTED
#define EFI_OUT_OF_RESOURCES      RETURN_OUT_OF_RESOURCES
#define EFI_VOLUME_CORRUPTED      RETURN_VOLUME_CORRUPTED
#define EFI_VOLUME_FULL           RETURN_VOLUME_FULL
#define EFI_NO_MEDIA              RETURN_NO_MEDIA
#define EFI_MEDIA_CHANGED         RETURN_MEDIA_CHANGED
#define EFI_NOT_FOUND             RETURN_NOT_FOUND
#define EFI_ACCESS_DENIED         RETURN_ACCESS_DENIED
#define EFI_NO_RESPONSE           RETURN_NO_RESPONSE
#define EFI_NO_MAPPING            RETURN_NO_MAPPING
#define EFI_TIMEOUT               RETURN_TIMEOUT
#define EFI_NOT_STARTED           RETURN_NOT_STARTED
#define EFI_ALREADY_STARTED       RETURN_ALREADY_STARTED
#define EFI_ABORTED               RETURN_ABORTED
#define EFI_ICMP_ERROR            RETURN_ICMP_ERROR
#define EFI_TFTP_ERROR            RETURN_TFTP_ERROR
#define EFI_PROTOCOL_ERROR        RETURN_PROTOCOL_ERROR
#define EFI_INCOMPATIBLE_VERSION  RETURN_INCOMPATIBLE_VERSION
#define EFI_SECURITY_VIOLATION    RETURN_SECURITY_VIOLATION
#define EFI_CRC_ERROR             RETURN_CRC_ERROR
#define EFI_END_OF_MEDIA          RETURN_END_OF_MEDIA
#define EFI_END_OF_FILE           RETURN_END_OF_FILE
#define EFI_INVALID_LANGUAGE      RETURN_INVALID_LANGUAGE
#define EFI_COMPROMISED_DATA      RETURN_COMPROMISED_DATA
#define EFI_HTTP_ERROR            RETURN_HTTP_ERROR

#define EFI_WARN_UNKNOWN_GLYPH    RETURN_WARN_UNKNOWN_GLYPH
#define EFI_WARN_DELETE_FAILURE   RETURN_WARN_DELETE_FAILURE
#define EFI_WARN_WRITE_FAILURE    RETURN_WARN_WRITE_FAILURE
#define EFI_WARN_BUFFER_TOO_SMALL RETURN_WARN_BUFFER_TOO_SMALL

//
// Bits
//

#define  BIT0     0x00000001
#define  BIT1     0x00000002
#define  BIT2     0x00000004
#define  BIT3     0x00000008
#define  BIT4     0x00000010
#define  BIT5     0x00000020
#define  BIT6     0x00000040
#define  BIT7     0x00000080
#define  BIT8     0x00000100
#define  BIT9     0x00000200
#define  BIT10    0x00000400
#define  BIT11    0x00000800
#define  BIT12    0x00001000
#define  BIT13    0x00002000
#define  BIT14    0x00004000
#define  BIT15    0x00008000
#define  BIT16    0x00010000
#define  BIT17    0x00020000
#define  BIT18    0x00040000
#define  BIT19    0x00080000
#define  BIT20    0x00100000
#define  BIT21    0x00200000
#define  BIT22    0x00400000
#define  BIT23    0x00800000
#define  BIT24    0x01000000
#define  BIT25    0x02000000
#define  BIT26    0x04000000
#define  BIT27    0x08000000
#define  BIT28    0x10000000
#define  BIT29    0x20000000
#define  BIT30    0x40000000
#define  BIT31    0x80000000
#define  BIT32    0x0000000100000000ULL
#define  BIT33    0x0000000200000000ULL
#define  BIT34    0x0000000400000000ULL
#define  BIT35    0x0000000800000000ULL
#define  BIT36    0x0000001000000000ULL
#define  BIT37    0x0000002000000000ULL
#define  BIT38    0x0000004000000000ULL
#define  BIT39    0x0000008000000000ULL
#define  BIT40    0x0000010000000000ULL
#define  BIT41    0x0000020000000000ULL
#define  BIT42    0x0000040000000000ULL
#define  BIT43    0x0000080000000000ULL
#define  BIT44    0x0000100000000000ULL
#define  BIT45    0x0000200000000000ULL
#define  BIT46    0x0000400000000000ULL
#define  BIT47    0x0000800000000000ULL
#define  BIT48    0x0001000000000000ULL
#define  BIT49    0x0002000000000000ULL
#define  BIT50    0x0004000000000000ULL
#define  BIT51    0x0008000000000000ULL
#define  BIT52    0x0010000000000000ULL
#define  BIT53    0x0020000000000000ULL
#define  BIT54    0x0040000000000000ULL
#define  BIT55    0x0080000000000000ULL
#define  BIT56    0x0100000000000000ULL
#define  BIT57    0x0200000000000000ULL
#define  BIT58    0x0400000000000000ULL
#define  BIT59    0x0800000000000000ULL
#define  BIT60    0x1000000000000000ULL
#define  BIT61    0x2000000000000000ULL
#define  BIT62    0x4000000000000000ULL
#define  BIT63    0x8000000000000000ULL

//
// Bases and sizes
//

#define  SIZE_1KB    0x00000400
#define  SIZE_2KB    0x00000800
#define  SIZE_4KB    0x00001000
#define  SIZE_8KB    0x00002000
#define  SIZE_16KB   0x00004000
#define  SIZE_32KB   0x00008000
#define  SIZE_64KB   0x00010000
#define  SIZE_128KB  0x00020000
#define  SIZE_256KB  0x00040000
#define  SIZE_512KB  0x00080000
#define  SIZE_1MB    0x00100000
#define  SIZE_2MB    0x00200000
#define  SIZE_4MB    0x00400000
#define  SIZE_8MB    0x00800000
#define  SIZE_16MB   0x01000000
#define  SIZE_32MB   0x02000000
#define  SIZE_64MB   0x04000000
#define  SIZE_128MB  0x08000000
#define  SIZE_256MB  0x10000000
#define  SIZE_512MB  0x20000000
#define  SIZE_1GB    0x40000000
#define  SIZE_2GB    0x80000000
#define  SIZE_4GB    0x0000000100000000ULL
#define  SIZE_8GB    0x0000000200000000ULL
#define  SIZE_16GB   0x0000000400000000ULL
#define  SIZE_32GB   0x0000000800000000ULL
#define  SIZE_64GB   0x0000001000000000ULL
#define  SIZE_128GB  0x0000002000000000ULL
#define  SIZE_256GB  0x0000004000000000ULL
#define  SIZE_512GB  0x0000008000000000ULL
#define  SIZE_1TB    0x0000010000000000ULL
#define  SIZE_2TB    0x0000020000000000ULL
#define  SIZE_4TB    0x0000040000000000ULL
#define  SIZE_8TB    0x0000080000000000ULL
#define  SIZE_16TB   0x0000100000000000ULL
#define  SIZE_32TB   0x0000200000000000ULL
#define  SIZE_64TB   0x0000400000000000ULL
#define  SIZE_128TB  0x0000800000000000ULL
#define  SIZE_256TB  0x0001000000000000ULL
#define  SIZE_512TB  0x0002000000000000ULL
#define  SIZE_1PB    0x0004000000000000ULL
#define  SIZE_2PB    0x0008000000000000ULL
#define  SIZE_4PB    0x0010000000000000ULL
#define  SIZE_8PB    0x0020000000000000ULL
#define  SIZE_16PB   0x0040000000000000ULL
#define  SIZE_32PB   0x0080000000000000ULL
#define  SIZE_64PB   0x0100000000000000ULL
#define  SIZE_128PB  0x0200000000000000ULL
#define  SIZE_256PB  0x0400000000000000ULL
#define  SIZE_512PB  0x0800000000000000ULL
#define  SIZE_1EB    0x1000000000000000ULL
#define  SIZE_2EB    0x2000000000000000ULL
#define  SIZE_4EB    0x4000000000000000ULL
#define  SIZE_8EB    0x8000000000000000ULL

#define  BASE_1KB    0x00000400
#define  BASE_2KB    0x00000800
#define  BASE_4KB    0x00001000
#define  BASE_8KB    0x00002000
#define  BASE_16KB   0x00004000
#define  BASE_32KB   0x00008000
#define  BASE_64KB   0x00010000
#define  BASE_128KB  0x00020000
#define  BASE_256KB  0x00040000
#define  BASE_512KB  0x00080000
#define  BASE_1MB    0x00100000
#define  BASE_2MB    0x00200000
#define  BASE_4MB    0x00400000
#define  BASE_8MB    0x00800000
#define  BASE_16MB   0x01000000
#define  BASE_32MB   0x02000000
#define  BASE_64MB   0x04000000
#define  BASE_128MB  0x08000000
#define  BASE_256MB  0x10000000
#define  BASE_512MB  0x20000000
#define  BASE_1GB    0x40000000
#define  BASE_2GB    0x80000000
#define  BASE_4GB    0x0000000100000000ULL
#define  BASE_8GB    0x0000000200000000ULL
#define  BASE_16GB   0x0000000400000000ULL
#define  BASE_32GB   0x0000000800000000ULL
#define  BASE_64GB   0x0000001000000000ULL
#define  BASE_128GB  0x0000002000000000ULL
#define  BASE_256GB  0x0000004000000000ULL
#define  BASE_512GB  0x0000008000000000ULL
#define  BASE_1TB    0x0000010000000000ULL
#define  BASE_2TB    0x0000020000000000ULL
#define  BASE_4TB    0x0000040000000000ULL
#define  BASE_8TB    0x0000080000000000ULL
#define  BASE_16TB   0x0000100000000000ULL
#define  BASE_32TB   0x0000200000000000ULL
#define  BASE_64TB   0x0000400000000000ULL
#define  BASE_128TB  0x0000800000000000ULL
#define  BASE_256TB  0x0001000000000000ULL
#define  BASE_512TB  0x0002000000000000ULL
#define  BASE_1PB    0x0004000000000000ULL
#define  BASE_2PB    0x0008000000000000ULL
#define  BASE_4PB    0x0010000000000000ULL
#define  BASE_8PB    0x0020000000000000ULL
#define  BASE_16PB   0x0040000000000000ULL
#define  BASE_32PB   0x0080000000000000ULL
#define  BASE_64PB   0x0100000000000000ULL
#define  BASE_128PB  0x0200000000000000ULL
#define  BASE_256PB  0x0400000000000000ULL
#define  BASE_512PB  0x0800000000000000ULL
#define  BASE_1EB    0x1000000000000000ULL
#define  BASE_2EB    0x2000000000000000ULL
#define  BASE_4EB    0x4000000000000000ULL
#define  BASE_8EB    0x8000000000000000ULL

#define EFI_VARIABLE_NON_VOLATILE       0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS     0x00000004
#define EFI_VARIABLE_READ_ONLY          0x00000008

//
// Functional macros
//

#define SIGNATURE_16(A, B)        ((A) | ((B) << 8))
#define SIGNATURE_32(A, B, C, D)  (SIGNATURE_16 (A, B) | (SIGNATURE_16 (C, D) << 16))
#define DEBUG(X) do { ppprintf X ; } while (0)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define EFI_ERROR(x) ((x) != 0)
#define OFFSET_OF(Base, Type) offsetof(Base, Type)
#define ALIGN_VALUE(Value, Alignment) ((Value) + (((Alignment) - (Value)) & ((Alignment) - 1)))
#define MAX(a, b) \
  (((a) > (b)) ? (a) : (b))
#define MIN(a, b) \
  (((a) < (b)) ? (a) : (b))
#define DEBUG_CODE_BEGIN()  do { if (DebugCodeEnabled ()) { UINT8  __DebugCodeLocal
#define DEBUG_CODE_END()    __DebugCodeLocal = 0; __DebugCodeLocal++; } } while (FALSE)
#define DEBUG_CODE(Expression)  \
  DEBUG_CODE_BEGIN ();          \
  Expression                    \
  DEBUG_CODE_END ()
#define EFI_SIZE_TO_PAGES(a)  (((a) >> EFI_PAGE_SHIFT) + (((a) & EFI_PAGE_MASK) ? 1 : 0))
#define EFI_PAGES_TO_SIZE(a)   ( (a) << EFI_PAGE_SHIFT)
#define BASE_CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - OFFSET_OF (TYPE, Field)))
#define CR(Record, TYPE, Field, TestSignature)                                              \
  (DebugAssertEnabled () && (BASE_CR (Record, TYPE, Field)->Signature != TestSignature)) ?  \
  (TYPE *) (ASSERT (false), Record) :                                       \
  BASE_CR (Record, TYPE, Field)
#define ASSERT_EFI_ERROR(status)  if (EFI_ERROR(status))                \
    DEBUG_CODE ( {                                                      \
      DEBUG((EFI_D_ERROR, "\nASSERT!Status = 0x%x Info :",status));     \
      ASSERT(!EFI_ERROR(status));                                \
    } )
//
// Functions
//

#define AllocatePool(x) (malloc)(x)
#define AllocateZeroPool(x) (calloc)(1, x)
#define ReallocatePool(a,b,c) (realloc)(c,b)
STATIC inline EFI_STATUS FreePool(void *x) { free (x); return EFI_SUCCESS; }
#define CompareMem(a,b,c) (memcmp)((a),(b),(c))
#define CopyMem(a,b,c) (memmove)((a),(b),(c))
#define ZeroMem(a,b) (memset)(a, 0, b)
#define SetMem(Dst, Size, Value) (memset)(Dst, Value, Size)
#define AsciiSPrint snppprintf
#define AsciiStrCmp strcmp
#define AsciiStrLen strlen
#define AsciiStrStr strstr
#define AsciiStrnCmp strncmp
#define AsciiStrSize(x) (strlen(x) + 1)
#define AsciiStrnCpyS(a, b, c, d) oc_strlcpy(a, c, b)
#define AsciiStrDecimalToUint64(a) (strtoull)(a, NULL, 10)
#define AsciiStrHexToUint64(a) (strtoull)(a, NULL, 16)
#define ASSERT(x) assert(x)
#define DebugCodeEnabled() true
#define DebugAssertEnabled() true
static inline void FreePages(void *p,UINTN s) {}
#define UnicodeSPrint(...) assert(false)
#define CompareGuid(a, b) ((memcmp)((a), (b), sizeof (EFI_GUID)) == 0)
#define CopyGuid(a, b) (memcpy)((a), (b), sizeof (EFI_GUID))

EFI_STATUS EfiGetSystemConfigurationTable (EFI_GUID *TableGuid, OUT VOID **Table);

/*
 * Copy string src to buffer dst of size dsize.  At most dsize-1
 * chars will be copied.  Always NUL terminates (unless dsize == 0).
 * Returns strlen(src); if retval >= dsize, truncation occurred.
 */
STATIC
size_t
oc_strlcpy(char * __restrict dst, const char * __restrict src, size_t dsize)
{
	const char *osrc = src;
	size_t nleft = dsize;

	/* Copy as many bytes as will fit. */
	if (nleft != 0) {
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src. */
	if (nleft == 0) {
		if (dsize != 0)
			*dst = '\0';		/* NUL-terminate dst */
		while (*src++)
			;
	}

	return(src - osrc - 1);	/* count does not include NUL */
}

//
// Dirty printf implementation
//

STATIC
VOID
ppptransform (
  CHAR8  *Buffer
  )
{
  char *AAA = NULL;

  #define REPL(P, I, V) \
    do { \
      AAA = strstr(Buffer, P); \
      if (AAA) \
        AAA[I] = V; \
    } while (AAA != NULL)


  REPL("%a", 1, 's');
  REPL("%-16a", 4, 's');
  REPL("%r", 1, 'p');
  REPL("%g", 1, 'p');
  REPL("%Lx", 1, 'z');

  #undef REPL
}

STATIC
VOID
ppprintf (
  FILE        *Shit,
  CONST CHAR8  *Src,
  ...
) {
  char  Buffer[1024];
  oc_strlcpy (Buffer, Src, sizeof (Buffer));

  ppptransform(Buffer);

  va_list args;

  va_start(args, Src);
  vprintf(Buffer, args);
  va_end(args);
}

STATIC
VOID
snppprintf (
  CHAR8  *Buf,
  UINTN  Size,
  CONST CHAR8 *Src,
  ...
  )
{
  char  Buffer[1024];
  oc_strlcpy (Buffer, Src, sizeof (Buffer));

  ppptransform(Buffer);

  va_list args;

  va_start(args, Src);
  vsnprintf(Buf, Size, Buffer, args);
  va_end(args);
}

STATIC
UINT64
MultU64x32 (
  UINT64 Multiplicand,
  UINT32 Multiplier
  )
{
  return Multiplicand * Multiplier;
}

STATIC
UINT64
DivU64x32 (
  UINT64 Dividend,
  UINT32 Divisor
  )
{
  return Dividend / Divisor;
}

STATIC
UINT64
LShiftU64 (
  UINT64 Operand,
  UINTN Count
  )
{
  return Operand << Count;
}

STATIC
UINT64
RShiftU64 (
  UINT64 Operand,
  UINTN Count
  )
{
  return Operand >> Count;
}

STATIC
UINT16
SwapBytes16 (
  IN      UINT16                    Value
  )
{
  return (UINT16) ((Value<< 8) | (Value>> 8));
}

STATIC
UINT32
SwapBytes32 (
  IN      UINT32                    Value
  )
{
  UINT32  LowerBytes;
  UINT32  HigherBytes;

  LowerBytes  = (UINT32) SwapBytes16 ((UINT16) Value);
  HigherBytes = (UINT32) SwapBytes16 ((UINT16) (Value >> 16));

  return (LowerBytes << 16 | HigherBytes);
}

STATIC
UINT64
EFIAPI
SwapBytes64 (
  IN      UINT64                    Value
  )
{
  UINT64  LowerBytes;
  UINT64  HigherBytes;

  LowerBytes  = (UINT64) SwapBytes32 ((UINT32) Value);
  HigherBytes = (UINT64) SwapBytes32 ((UINT32) (Value >> 32));

  return (LowerBytes << 32 | HigherBytes);
}

STATIC
UINT8
CalculateSum8 (
  IN UINT8  *Buffer,
  IN UINTN  Size
  )
{
  UINTN Index;
  UINT8 Sum;

  Sum = 0;

  //
  // Perform the byte sum for buffer
  //
  for (Index = 0; Index < Size; Index++) {
    Sum = (UINT8) (Sum + Buffer[Index]);
  }

  return Sum;
}

STATIC
UINT8
CalculateCheckSum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
{
  return (UINT8) (0x100 - CalculateSum8 (Buffer, Size));
}

STATIC
UINT32
AsmCpuid (
  UINT32 Index,
  UINT32 *Eax,
  UINT32 *Ebx,
  UINT32 *Ecx,
  UINT32 *Edx
  )
{
  uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;

  asm ("xchgq %%rbx, %q1\n"
       "cpuid\n"
       "xchgq %%rbx, %q1"
       : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
       : "0" (Index));

  if (Eax) *Eax = eax;
  if (Ebx) *Ebx = ebx;
  if (Ecx) *Ecx = ecx;
  if (Edx) *Edx = edx;

  return Index;
}

STATIC
UINT32
AsmCpuidEx (
  IN      UINT32                    Index,
  IN      UINT32                    SubIndex,
  OUT     UINT32                    *Eax,  OPTIONAL
  OUT     UINT32                    *Ebx,  OPTIONAL
  OUT     UINT32                    *Ecx,  OPTIONAL
  OUT     UINT32                    *Edx   OPTIONAL
  )
{
  uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;

  asm ("xchgq %%rbx, %q1\n"
       "cpuid\n"
       "xchgq %%rbx, %q1"
       : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
       : "0" (Index), "2" (SubIndex));

  if (Eax) *Eax = eax;
  if (Ebx) *Ebx = ebx;
  if (Ecx) *Ecx = ecx;
  if (Edx) *Edx = edx;

  return Index;
}

STATIC
UINT64
AsmReadMsr64 (
  UINT32 Index
  )
{
  return 0;
}

STATIC
VOID
AsmWriteMsr64 (
  UINT32  Index,
  UINT64  Value
  )
{
}

STATIC
UINT64
EFIAPI
GetPerformanceCounterProperties (
  UINT64  *StartValue,
  UINT64  *EndValue
  )
{
  return 0;
}

STATIC
UINTN
StrLen (
  CONST CHAR16              *String
  )
{
  UINTN   Length;

  ASSERT (String != NULL);
  ASSERT (((UINTN) String & BIT0) == 0);

  for (Length = 0; *String != L'\0'; String++, Length++) { }
  return Length;
}

#define SAFE_STRING_CONSTRAINT_CHECK(Expression, Status)  \
  do { \
    ASSERT (Expression); \
    if (!(Expression)) { \
      return Status; \
    } \
  } while (FALSE)

STATIC
UINTN
StrnLenS (
   CONST CHAR16              *String,
   UINTN                     MaxSize
  )
{
  UINTN     Length;

  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // If String is a null pointer or MaxSize is 0, then the StrnLenS function returns zero.
  //
  if ((String == NULL) || (MaxSize == 0)) {
    return 0;
  }

  Length = 0;
  while (String[Length] != 0) {
    if (Length >= MaxSize - 1) {
      return MaxSize;
    }
    Length++;
  }
  return Length;
}

STATIC
UINTN
StrSize (
  CONST CHAR16              *String
  )
{
  return (StrLen (String) + 1) * sizeof (*String);
}

STATIC
RETURN_STATUS
StrCpyS (
  CHAR16       *Destination,
  UINTN        DestMax,
  CONST CHAR16 *Source
  )
{
  UINTN            SourceLen;

  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than StrnLenS(Source, DestMax).
  //
  SourceLen = StrnLenS (Source, DestMax);
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  // SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrCpyS function copies the string pointed to by Source (including the terminating
  // null character) into the array pointed to by Destination.
  //
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

STATIC
RETURN_STATUS
EFIAPI
StrCatS (
  IN OUT CHAR16       *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR16 *Source
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;

  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // Let CopyLen denote the value DestMax - StrnLenS(Destination, DestMax) upon entry to StrCatS.
  //
  DestLen = StrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. CopyLen shall be greater than StrnLenS(Source, CopyLen).
  //
  SourceLen = StrnLenS (Source, CopyLen);
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 6. Copying shall not take place between objects that overlap.
  //
  // SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrCatS function appends a copy of the string pointed to by Source (including the
  // terminating null character) to the end of the string pointed to by Destination. The initial character
  // from Source overwrites the null character at the end of Destination.
  //
  Destination = Destination + DestLen;
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}


STATIC
INTN
StrCmp (
  CONST CHAR16              *FirstString,
  CONST CHAR16              *SecondString
  )
{
  while ((*FirstString != L'\0') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }
  return *FirstString - *SecondString;
}

STATIC
CHAR16
CharToUpper (
  IN      CHAR16                    Char
  )
{
  if (Char >= L'a' && Char <= L'z') {
    return (CHAR16) (Char - (L'a' - L'A'));
  }

  return Char;
}

STATIC
CHAR16 *
StrStr (
  IN      CONST CHAR16              *String,
  IN      CONST CHAR16              *SearchString
  )
{
  CONST CHAR16 *FirstMatch;
  CONST CHAR16 *SearchStringTmp;

  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (String) != 0);
  ASSERT (StrSize (SearchString) != 0);

  if (*SearchString == L'\0') {
    return (CHAR16 *) String;
  }

  while (*String != L'\0') {
    SearchStringTmp = SearchString;
    FirstMatch = String;

    while ((*String == *SearchStringTmp)
            && (*String != L'\0')) {
      String++;
      SearchStringTmp++;
    }

    if (*SearchStringTmp == L'\0') {
      return (CHAR16 *) FirstMatch;
    }

    if (*String == L'\0') {
      return NULL;
    }

    String = FirstMatch + 1;
  }

  return NULL;
}

STATIC
VOID
InitializeListHead (
  LIST_ENTRY       *List
  )
{
  List->ForwardLink = List;
  List->BackLink    = List;
}

STATIC
VOID
InsertTailList (
  LIST_ENTRY  *ListHead,
  LIST_ENTRY  *Entry
  )
{
  LIST_ENTRY *_ListHead;
  LIST_ENTRY *_BackLink;

  _ListHead              = ListHead;
  _BackLink              = _ListHead->BackLink;
  Entry->ForwardLink     = _ListHead;
  Entry->BackLink        = _BackLink;
  _BackLink->ForwardLink = Entry;
  _ListHead->BackLink    = Entry;
}

STATIC
VOID
RemoveEntryList (
  LIST_ENTRY  *Entry
  )
{
  LIST_ENTRY  *_ForwardLink;
  LIST_ENTRY  *_BackLink;

  _ForwardLink           = Entry->ForwardLink;
  _BackLink              = Entry->BackLink;
  _BackLink->ForwardLink = _ForwardLink;
  _ForwardLink->BackLink = _BackLink;
}

STATIC
LIST_ENTRY *
GetFirstNode (
  LIST_ENTRY  *List
  )
{
  return List->ForwardLink;
}

STATIC
BOOLEAN
IsNull (
  LIST_ENTRY  *List,
  LIST_ENTRY  *Node
  )
{
  return (BOOLEAN)(Node == List);
}

STATIC
BOOLEAN
IsListEmpty (
  LIST_ENTRY  *List
  )
{
  return (BOOLEAN)(List->ForwardLink == List);
}

STATIC
LIST_ENTRY *
GetNextNode (
  LIST_ENTRY  *List,
  LIST_ENTRY  *Node
  )
{
  if (Node == List) {
    return List;
  }
  return Node->ForwardLink;
}

STATIC
UINT16
EFIAPI
ReadUnaligned16 (
  IN CONST UINT16              *Buffer
  )
{
  volatile UINT8 LowerByte;
  volatile UINT8 HigherByte;

  ASSERT (Buffer != NULL);

  LowerByte = ((UINT8*)Buffer)[0];
  HigherByte = ((UINT8*)Buffer)[1];

  return (UINT16)(LowerByte | (HigherByte << 8));
}

STATIC
UINT16
EFIAPI
WriteUnaligned16 (
  OUT UINT16                    *Buffer,
  IN  UINT16                    Value
  )
{
  ASSERT (Buffer != NULL);

  ((volatile UINT8*)Buffer)[0] = (UINT8)Value;
  ((volatile UINT8*)Buffer)[1] = (UINT8)(Value >> 8);

  return Value;
}

STATIC
UINT32
EFIAPI
ReadUnaligned32 (
  IN      CONST UINT32              *Buffer
  )
{
  UINT16  LowerBytes;
  UINT16  HigherBytes;

  ASSERT (Buffer != NULL);

  LowerBytes  = ReadUnaligned16 ((UINT16*) Buffer);
  HigherBytes = ReadUnaligned16 ((UINT16*) Buffer + 1);

  return (UINT32) (LowerBytes | ((UINT32)HigherBytes << 16));
}

STATIC
UINT32
EFIAPI
WriteUnaligned32 (
  OUT     UINT32                    *Buffer,
  IN      UINT32                    Value
  )
{
  ASSERT (Buffer != NULL);

  WriteUnaligned16 ((UINT16*)Buffer, (UINT16)Value);
  WriteUnaligned16 ((UINT16*)Buffer + 1, (UINT16)(Value >> 16));
  return Value;
}

STATIC
UINT64
EFIAPI
ReadUnaligned64 (
  IN      CONST UINT64              *Buffer
  )
{
  UINT32  LowerBytes;
  UINT32  HigherBytes;

  ASSERT (Buffer != NULL);

  LowerBytes  = ReadUnaligned32 ((UINT32*) Buffer);
  HigherBytes = ReadUnaligned32 ((UINT32*) Buffer + 1);

  return (UINT64) (LowerBytes | LShiftU64 (HigherBytes, 32));
}

STATIC
UINT64
EFIAPI
WriteUnaligned64 (
  OUT UINT64                    *Buffer,
  IN  UINT64                    Value
  )
{
  ASSERT (Buffer != NULL);

  WriteUnaligned32 ((UINT32*)Buffer, (UINT32)Value);
  WriteUnaligned32 ((UINT32*)Buffer + 1, (UINT32)RShiftU64 (Value, 32));
  return Value;
}

STATIC
UINTN
InternalBaseLibBitFieldReadUint (
  IN      UINTN                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  //
  // ~((UINTN)-2 << EndBit) is a mask in which bit[0] thru bit[EndBit]
  // are 1's while bit[EndBit + 1] thru the most significant bit are 0's.
  //
  return (Operand & ~((UINTN)-2 << EndBit)) >> StartBit;
}

STATIC
UINT8
BitFieldRead8 (
  IN      UINT8                     Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  ASSERT (EndBit < 8);
  ASSERT (StartBit <= EndBit);
  return (UINT8)InternalBaseLibBitFieldReadUint (Operand, StartBit, EndBit);
}


STATIC
UINT64
BitFieldRead64 (
  IN      UINT64                    Operand,
  IN      UINTN                     StartBit,
  IN      UINTN                     EndBit
  )
{
  ASSERT (EndBit < 64);
  ASSERT (StartBit <= EndBit);
  return RShiftU64 (Operand & ~LShiftU64 ((UINT64)-2, EndBit), StartBit);
}

STATIC
INTN
HighBitSet32 (
  IN      UINT32                    Operand
  )
{
  INTN                              BitIndex;

  if (Operand == 0) {
    return - 1;
  }
  for (BitIndex = 31; (INT32)Operand > 0; BitIndex--, Operand <<= 1);
  return BitIndex;
}


STATIC
UINT32
GetPowerOfTwo32 (
  IN      UINT32                    Operand
  )
{
  if (0 == Operand) {
    return 0;
  }

  return 1ul << HighBitSet32 (Operand);
}

STATIC
VOID *
AllocateCopyPool (
   UINTN            AllocationSize,
   CONST VOID       *Buffer
  )
{
  VOID  *Memory;

  Memory = malloc (AllocationSize);
  if (Memory != NULL) {
     Memory = memcpy (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

STATIC CONST UINT32  mCrcTable[256] = {
  0x00000000,
  0x77073096,
  0xEE0E612C,
  0x990951BA,
  0x076DC419,
  0x706AF48F,
  0xE963A535,
  0x9E6495A3,
  0x0EDB8832,
  0x79DCB8A4,
  0xE0D5E91E,
  0x97D2D988,
  0x09B64C2B,
  0x7EB17CBD,
  0xE7B82D07,
  0x90BF1D91,
  0x1DB71064,
  0x6AB020F2,
  0xF3B97148,
  0x84BE41DE,
  0x1ADAD47D,
  0x6DDDE4EB,
  0xF4D4B551,
  0x83D385C7,
  0x136C9856,
  0x646BA8C0,
  0xFD62F97A,
  0x8A65C9EC,
  0x14015C4F,
  0x63066CD9,
  0xFA0F3D63,
  0x8D080DF5,
  0x3B6E20C8,
  0x4C69105E,
  0xD56041E4,
  0xA2677172,
  0x3C03E4D1,
  0x4B04D447,
  0xD20D85FD,
  0xA50AB56B,
  0x35B5A8FA,
  0x42B2986C,
  0xDBBBC9D6,
  0xACBCF940,
  0x32D86CE3,
  0x45DF5C75,
  0xDCD60DCF,
  0xABD13D59,
  0x26D930AC,
  0x51DE003A,
  0xC8D75180,
  0xBFD06116,
  0x21B4F4B5,
  0x56B3C423,
  0xCFBA9599,
  0xB8BDA50F,
  0x2802B89E,
  0x5F058808,
  0xC60CD9B2,
  0xB10BE924,
  0x2F6F7C87,
  0x58684C11,
  0xC1611DAB,
  0xB6662D3D,
  0x76DC4190,
  0x01DB7106,
  0x98D220BC,
  0xEFD5102A,
  0x71B18589,
  0x06B6B51F,
  0x9FBFE4A5,
  0xE8B8D433,
  0x7807C9A2,
  0x0F00F934,
  0x9609A88E,
  0xE10E9818,
  0x7F6A0DBB,
  0x086D3D2D,
  0x91646C97,
  0xE6635C01,
  0x6B6B51F4,
  0x1C6C6162,
  0x856530D8,
  0xF262004E,
  0x6C0695ED,
  0x1B01A57B,
  0x8208F4C1,
  0xF50FC457,
  0x65B0D9C6,
  0x12B7E950,
  0x8BBEB8EA,
  0xFCB9887C,
  0x62DD1DDF,
  0x15DA2D49,
  0x8CD37CF3,
  0xFBD44C65,
  0x4DB26158,
  0x3AB551CE,
  0xA3BC0074,
  0xD4BB30E2,
  0x4ADFA541,
  0x3DD895D7,
  0xA4D1C46D,
  0xD3D6F4FB,
  0x4369E96A,
  0x346ED9FC,
  0xAD678846,
  0xDA60B8D0,
  0x44042D73,
  0x33031DE5,
  0xAA0A4C5F,
  0xDD0D7CC9,
  0x5005713C,
  0x270241AA,
  0xBE0B1010,
  0xC90C2086,
  0x5768B525,
  0x206F85B3,
  0xB966D409,
  0xCE61E49F,
  0x5EDEF90E,
  0x29D9C998,
  0xB0D09822,
  0xC7D7A8B4,
  0x59B33D17,
  0x2EB40D81,
  0xB7BD5C3B,
  0xC0BA6CAD,
  0xEDB88320,
  0x9ABFB3B6,
  0x03B6E20C,
  0x74B1D29A,
  0xEAD54739,
  0x9DD277AF,
  0x04DB2615,
  0x73DC1683,
  0xE3630B12,
  0x94643B84,
  0x0D6D6A3E,
  0x7A6A5AA8,
  0xE40ECF0B,
  0x9309FF9D,
  0x0A00AE27,
  0x7D079EB1,
  0xF00F9344,
  0x8708A3D2,
  0x1E01F268,
  0x6906C2FE,
  0xF762575D,
  0x806567CB,
  0x196C3671,
  0x6E6B06E7,
  0xFED41B76,
  0x89D32BE0,
  0x10DA7A5A,
  0x67DD4ACC,
  0xF9B9DF6F,
  0x8EBEEFF9,
  0x17B7BE43,
  0x60B08ED5,
  0xD6D6A3E8,
  0xA1D1937E,
  0x38D8C2C4,
  0x4FDFF252,
  0xD1BB67F1,
  0xA6BC5767,
  0x3FB506DD,
  0x48B2364B,
  0xD80D2BDA,
  0xAF0A1B4C,
  0x36034AF6,
  0x41047A60,
  0xDF60EFC3,
  0xA867DF55,
  0x316E8EEF,
  0x4669BE79,
  0xCB61B38C,
  0xBC66831A,
  0x256FD2A0,
  0x5268E236,
  0xCC0C7795,
  0xBB0B4703,
  0x220216B9,
  0x5505262F,
  0xC5BA3BBE,
  0xB2BD0B28,
  0x2BB45A92,
  0x5CB36A04,
  0xC2D7FFA7,
  0xB5D0CF31,
  0x2CD99E8B,
  0x5BDEAE1D,
  0x9B64C2B0,
  0xEC63F226,
  0x756AA39C,
  0x026D930A,
  0x9C0906A9,
  0xEB0E363F,
  0x72076785,
  0x05005713,
  0x95BF4A82,
  0xE2B87A14,
  0x7BB12BAE,
  0x0CB61B38,
  0x92D28E9B,
  0xE5D5BE0D,
  0x7CDCEFB7,
  0x0BDBDF21,
  0x86D3D2D4,
  0xF1D4E242,
  0x68DDB3F8,
  0x1FDA836E,
  0x81BE16CD,
  0xF6B9265B,
  0x6FB077E1,
  0x18B74777,
  0x88085AE6,
  0xFF0F6A70,
  0x66063BCA,
  0x11010B5C,
  0x8F659EFF,
  0xF862AE69,
  0x616BFFD3,
  0x166CCF45,
  0xA00AE278,
  0xD70DD2EE,
  0x4E048354,
  0x3903B3C2,
  0xA7672661,
  0xD06016F7,
  0x4969474D,
  0x3E6E77DB,
  0xAED16A4A,
  0xD9D65ADC,
  0x40DF0B66,
  0x37D83BF0,
  0xA9BCAE53,
  0xDEBB9EC5,
  0x47B2CF7F,
  0x30B5FFE9,
  0xBDBDF21C,
  0xCABAC28A,
  0x53B39330,
  0x24B4A3A6,
  0xBAD03605,
  0xCDD70693,
  0x54DE5729,
  0x23D967BF,
  0xB3667A2E,
  0xC4614AB8,
  0x5D681B02,
  0x2A6F2B94,
  0xB40BBE37,
  0xC30C8EA1,
  0x5A05DF1B,
  0x2D02EF8D
};

STATIC
UINT32
EFIAPI
CalculateCrc32(
  IN  VOID                         *Buffer,
  IN  UINTN                        Length
  )
{
  UINTN   Index;
  UINT32  Crc;
  UINT8   *Ptr;

  ASSERT (Buffer != NULL);
  ASSERT (Length <= (MAX_ADDRESS - ((UINTN) Buffer) + 1));

  //
  // Compute CRC
  //
  Crc = 0xffffffff;
  for (Index = 0, Ptr = Buffer; Index < Length; Index++, Ptr++) {
    Crc = (Crc >> 8) ^ mCrcTable[(UINT8) Crc ^ *Ptr];
  }

  return Crc ^ 0xffffffff;
}

#include <Protocol/DevicePath.h>

#define END_DEVICE_PATH_LENGTH               (sizeof (EFI_DEVICE_PATH_PROTOCOL))

STATIC CONST EFI_DEVICE_PATH_PROTOCOL  mUefiDevicePathLibEndDevicePath = {
  END_DEVICE_PATH_TYPE,
  END_ENTIRE_DEVICE_PATH_SUBTYPE,
  {
    END_DEVICE_PATH_LENGTH,
    0
  }
};

STATIC
VOID
SetDevicePathEndNode (
  OUT VOID  *Node
  )
{
  ASSERT (Node != NULL);
  CopyMem (Node, &mUefiDevicePathLibEndDevicePath, sizeof (mUefiDevicePathLibEndDevicePath));
}


//
// Services
//

struct EFI_BOOT_SERVICES_ {
  EFI_STATUS (*LocateProtocol)(EFI_GUID *ProtocolGuid, VOID *Registration, VOID **Interface);
  EFI_STATUS (*AllocatePages)(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory);
  EFI_STATUS (*FreePages)(EFI_PHYSICAL_ADDRESS Memory, UINTN Pages);
  EFI_STATUS (*InstallConfigurationTable)(EFI_GUID *Guid, VOID *Table);
  EFI_STATUS (*LocateHandleBuffer) (EFI_LOCATE_SEARCH_TYPE SearchType, EFI_GUID * Protocol, VOID *SearchKey, UINTN *NumberHandles, EFI_HANDLE **Buffer);
  EFI_STATUS (*HandleProtocol)(EFI_HANDLE Handle, EFI_GUID *Protocol, VOID **Interface);
  EFI_STATUS (*InstallProtocolInterface) (EFI_HANDLE *Handle, EFI_GUID *Protocol, EFI_INTERFACE_TYPE InterfaceType, VOID *Interface);
  EFI_STATUS (*GetMemoryMap) (UINTN *MemoryMapSize, EFI_MEMORY_DESCRIPTOR *MemoryMap, UINTN *MapKey, UINTN *DescriptorSize, UINT32 *DescriptorVersion);
  EFI_STATUS (*FreePool) (void *x);
  EFI_STATUS (*LocateDevicePath) (EFI_GUID *Protocol, EFI_DEVICE_PATH_PROTOCOL **DevicePath, EFI_HANDLE *Device);
};

typedef EFI_STATUS (*EFI_GET_VARIABLE)(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 *Attributes, UINTN *DataSize, VOID *Data);
typedef EFI_STATUS (*EFI_SET_VARIABLE)(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 Attributes, UINTN DataSize, VOID *Data);

struct EFI_RUNTIME_SERVICES_ {
  EFI_GET_VARIABLE GetVariable;
  EFI_SET_VARIABLE SetVariable;
};

STATIC EFI_STATUS NilLocateProtocol(EFI_GUID *ProtocolGuid, VOID *Registration, VOID **Interface) {
  (VOID) ProtocolGuid;
  (VOID) Registration;
  (VOID) Interface;
  return EFI_NOT_FOUND;
}

#define TOTAL_PAGES 1000
extern _Thread_local uint32_t externalUsedPages;
extern _Thread_local uint8_t externalBlob[EFI_PAGE_SIZE*TOTAL_PAGES];

STATIC EFI_STATUS NilAllocatePages(EFI_ALLOCATE_TYPE Type, EFI_MEMORY_TYPE MemoryType, UINTN Pages, EFI_PHYSICAL_ADDRESS *Memory) {
#if 0
  if (TOTAL_PAGES - externalUsedPages >= Pages) {
    *Memory = (EFI_PHYSICAL_ADDRESS)(externalBlob + externalUsedPages * EFI_PAGE_SIZE);
    externalUsedPages += Pages;
    return EFI_SUCCESS;
  }
  return EFI_OUT_OF_RESOURCES;
#else
  *Memory = (EFI_PHYSICAL_ADDRESS) malloc (Pages * EFI_PAGE_SIZE);
  assert(Memory != NULL);
  return EFI_SUCCESS;
#endif
}

STATIC EFI_STATUS NilFreePages(EFI_PHYSICAL_ADDRESS Page, UINTN Pages) {
  return EFI_SUCCESS;
}

STATIC EFI_STATUS NilInstallConfigurationTable(EFI_GUID *Guid, VOID *Table) {
  return EFI_UNSUPPORTED;
}

STATIC EFI_STATUS NilLocateHandleBuffer (EFI_LOCATE_SEARCH_TYPE SearchType, EFI_GUID * Protocol, VOID *SearchKey, UINTN *NumberHandles, EFI_HANDLE **Buffer) {
  return EFI_NOT_FOUND;
}

STATIC EFI_STATUS NilHandleProtocol(EFI_HANDLE Handle, EFI_GUID *Protocol, VOID **Interface) {
  return EFI_NOT_FOUND;
}

STATIC EFI_STATUS NilGetVariable(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 *Attributes, UINTN *DataSize, VOID *Data) {
  return EFI_NOT_FOUND;
}

STATIC EFI_STATUS NilSetVariable(CHAR16 *VariableName, EFI_GUID *VendorGuid, UINT32 Attributes, UINTN DataSize, VOID *Data) {
  return EFI_SUCCESS;
}

STATIC EFI_STATUS NilInstallProtocolInterface (EFI_HANDLE *Handle, EFI_GUID *Protocol, EFI_INTERFACE_TYPE InterfaceType, VOID *Interface) {
  return EFI_SUCCESS;
}

STATIC EFI_STATUS NilGetMemoryMap (UINTN *MemoryMapSize, EFI_MEMORY_DESCRIPTOR *MemoryMap, UINTN *MapKey, UINTN *DescriptorSize, UINT32 *DescriptorVersion) {
  return EFI_UNSUPPORTED;
}

STATIC EFI_STATUS NilLocateDevicePath (EFI_GUID *Protocol, EFI_DEVICE_PATH_PROTOCOL **DevicePath, EFI_HANDLE *Device) {
  return EFI_UNSUPPORTED;
}

extern EFI_STATUS NilInstallConfigurationTableCustom(EFI_GUID *Guid, VOID *Table);

#ifndef CONFIG_TABLE_INSTALLER
#define CONFIG_TABLE_INSTALLER NilInstallConfigurationTable
#endif

STATIC EFI_BOOT_SERVICES gNilBS = {
  .LocateProtocol = NilLocateProtocol,
  .AllocatePages = NilAllocatePages,
  .FreePages = NilFreePages,
  .InstallConfigurationTable = CONFIG_TABLE_INSTALLER,
  .LocateHandleBuffer = NilLocateHandleBuffer,
  .HandleProtocol = NilHandleProtocol,
  .InstallProtocolInterface = NilInstallProtocolInterface,
  .GetMemoryMap = NilGetMemoryMap,
  .FreePool = FreePool,
  .LocateDevicePath = NilLocateDevicePath
};

STATIC EFI_BOOT_SERVICES *gBS = &gNilBS;

STATIC EFI_RUNTIME_SERVICES gNilRT = {
  .SetVariable = NilSetVariable,
  .GetVariable = NilGetVariable
};

STATIC EFI_RUNTIME_SERVICES *gRT = &gNilRT;

typedef
EFI_STATUS
(EFIAPI* EFI_IMAGE_UNLOAD)(
  IN  EFI_HANDLE                   ImageHandle
  );

//
// Specific
//
#define _PCD_GET_MODE_BOOL_PcdEnableAppleThunderboltSync false
#define _PCD_GET_MODE_BOOL_PcNvramInitDevicePropertyDatabase false
#define _PCD_GET_MODE_32_PcdMaximumDevicePathNodeCount 11
#define _PCD_GET_MODE_32_PcdMaximumUnicodeStringLength 0xFFFFFFFF

#ifndef ABS
#define ABS(x) (((x)<0) ? -(x) : (x))
#endif

STATIC EFI_GUID gEfiFileInfoGuid;
STATIC EFI_GUID gEfiSimpleFileSystemProtocolGuid;

STATIC VOID CpuDeadLoop(VOID) {
  volatile int i = 0;
  while (i == 0) {}
}

///
/// GPT Partition Entry.
///
typedef struct {
  ///
  /// Unique ID that defines the purpose and type of this Partition. A value of
  /// zero defines that this partition entry is not being used.
  ///
  EFI_GUID  PartitionTypeGUID;
  ///
  /// GUID that is unique for every partition entry. Every partition ever
  /// created will have a unique GUID.
  /// This GUID must be assigned when the GUID Partition Entry is created.
  ///
  EFI_GUID  UniquePartitionGUID;
  ///
  /// Starting LBA of the partition defined by this entry
  ///
  EFI_LBA   StartingLBA;
  ///
  /// Ending LBA of the partition defined by this entry.
  ///
  EFI_LBA   EndingLBA;
  ///
  /// Attribute bits, all bits reserved by UEFI
  /// Bit 0:      If this bit is set, the partition is required for the platform to function. The owner/creator of the
  ///             partition indicates that deletion or modification of the contents can result in loss of platform
  ///             features or failure for the platform to boot or operate. The system cannot function normally if
  ///             this partition is removed, and it should be considered part of the hardware of the system.
  ///             Actions such as running diagnostics, system recovery, or even OS install or boot, could
  ///             potentially stop working if this partition is removed. Unless OS software or firmware
  ///             recognizes this partition, it should never be removed or modified as the UEFI firmware or
  ///             platform hardware may become non-functional.
  /// Bit 1:      If this bit is set, then firmware must not produce an EFI_BLOCK_IO_PROTOCOL device for
  ///             this partition. By not producing an EFI_BLOCK_IO_PROTOCOL partition, file system
  ///             mappings will not be created for this partition in UEFI.
  /// Bit 2:      This bit is set aside to let systems with traditional PC-AT BIOS firmware implementations
  ///             inform certain limited, special-purpose software running on these systems that a GPT
  ///             partition may be bootable. The UEFI boot manager must ignore this bit when selecting
  ///             a UEFI-compliant application, e.g., an OS loader.
  /// Bits 3-47:  Undefined and must be zero. Reserved for expansion by future versions of the UEFI
  ///             specification.
  /// Bits 48-63: Reserved for GUID specific use. The use of these bits will vary depending on the
  ///             PartitionTypeGUID. Only the owner of the PartitionTypeGUID is allowed
  ///             to modify these bits. They must be preserved if Bits 0-47 are modified..
  ///
  UINT64    Attributes;
  ///
  /// Null-terminated name of the partition.
  ///
  CHAR16    PartitionName[36];
} EFI_PARTITION_ENTRY;

STATIC
EFI_MEMORY_DESCRIPTOR *
GetCurrentMemoryMap (
  OUT UINTN   *MemoryMapSize,
  OUT UINTN   *DescriptorSize,
  OUT UINTN   *MapKey             OPTIONAL,
  OUT UINT32  *DescriptorVersion  OPTIONAL
  )
{
  ASSERT (FALSE);
  return NULL;
}

#endif
