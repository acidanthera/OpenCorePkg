/* Copyright (c) 2005-2007 Apple Inc. All Rights Reserved. */

/*
 * libDER_config.h - platform dependent #defines and typedefs for libDER
 *
 * Created Nov. 4 2005 by dmitch
 */
 
#ifndef	_LIB_DER_CONFIG_H_
#define _LIB_DER_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// edk2 porting - start
//
#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#ifndef EFIUSER
typedef UINT8   uint8_t;
typedef UINT16  uint16_t;
typedef UINT32  uint32_t;
typedef UINT64  uint64_t;
typedef UINTN   size_t;

#ifndef bool
typedef BOOLEAN bool;
#endif /* bool */

#ifndef assert
#define assert ASSERT
#endif /* assert */

#define memset(ptr, c, len)    SetMem(ptr, len, c)
#define memmove(dst, src, len) CopyMem(dst, src, len)
#define memcmp(b1, b2, len)    CompareMem(b1, b2, len)

#else /* EFIUSER */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#endif /* !EFIUSER */

#define DER_ENCODE_ENABLE  0

#define DER_TAG_SIZE  8
//
// edk2 porting - end
//

/*
 * Basic data types: unsigned 8-bit integer, unsigned 32-bit integer
 */
typedef uint8_t DERByte;
typedef uint16_t DERShort;
typedef size_t DERSize;

/* 
 * Use these #defines of you have memset, memmove, and memcmp; else
 * write your own equivalents.
 */

#define DERMemset(ptr, c, len)		memset(ptr, c, len)
#define DERMemmove(dst, src, len)	memmove(dst, src, len)
#define DERMemcmp(b1, b2, len)		memcmp(b1, b2, len)


/***
 *** Compile time options to trim size of the library. 
 ***/
 
// CHANGE: conditionally define
/* enable general DER encode */
#ifndef DER_ENCODE_ENABLE
#define DER_ENCODE_ENABLE		1
#endif

#ifndef DER_MULTIBYTE_TAGS
/* enable general DER decode */
#define DER_DECODE_ENABLE		1
#endif

#ifndef DER_MULTIBYTE_TAGS
/* enable multibyte tag support. */
#define DER_MULTIBYTE_TAGS		1
#endif

#ifndef DER_TAG_SIZE
/* Iff DER_MULTIBYTE_TAGS is 1 this is the sizeof(DERTag) in bytes. Note that
   tags are still encoded and decoded from a minimally encoded DER
   represantation.  This value determines how big each DERItemSpecs is, we
   choose 2 since that makes DERItemSpecs 8 bytes wide.  */
#define DER_TAG_SIZE            2
#endif


/* ---------------------- Do not edit below this line ---------------------- */

/*
 * Logical representation of a tag (the encoded representation is always in
 * the minimal number of bytes). The top 3 bits encode class and method
 * The remaining bits encode the tag value.  To obtain smaller DERItemSpecs
 * sizes, choose the smallest type that fits your needs.  Most standard ASN.1
 * usage only needs single byte tags, but ocasionally custom applications
 * require a larger tag namespace.
 */
#if DER_MULTIBYTE_TAGS

#if DER_TAG_SIZE == 1
typedef uint8_t DERTag;
#elif DER_TAG_SIZE == 2
typedef uint16_t DERTag;
#elif DER_TAG_SIZE == 4
typedef uint32_t DERTag;
#elif DER_TAG_SIZE == 8
typedef uint64_t DERTag;
#else
#error DER_TAG_SIZE invalid
#endif

#else /* DER_MULTIBYTE_TAGS */
typedef DERByte DERTag;
#endif /* !DER_MULTIBYTE_TAGS */

#ifdef __cplusplus
}
#endif

#endif	/* _LIB_DER_CONFIG_H_ */
