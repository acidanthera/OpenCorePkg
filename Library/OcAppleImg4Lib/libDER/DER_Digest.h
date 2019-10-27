/* Copyright (c) 2005-2008,2010 Apple Inc. All Rights Reserved. */

/*
 * DER_Digest.h - DER encode a DigestInfo
 *
 * Created Nov. 9 2005 by dmitch
 */

#ifndef	_DER_DIGEST_H_
#define _DER_DIGEST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "libDER.h"

/* 
 * Create an encoded DigestInfo based on the specified SHA1 digest. 
 * The incoming digest must be 20 bytes long. 
 *
 * Result is placed in caller's buffer, which must be at least of
 * length DER_SHA1_DIGEST_INFO_LEN bytes. 
 *
 * The *resultLen parameter is the available size in the result
 * buffer on input, and the actual length of the encoded DigestInfo 
 * on output. 
 */
#define DER_SHA1_DIGEST_LEN			20
#define DER_SHA1_DIGEST_INFO_LEN	35 

DERReturn DEREncodeSHA1DigestInfo(
	const DERByte	*sha1Digest,
	DERSize			sha1DigestLen,
	DERByte			*result,		/* encoded result RETURNED here */
	DERSize			*resultLen);	/* IN/OUT */

#define DER_SHA256_DIGEST_LEN		32
#define DER_SHA256_DIGEST_INFO_LEN	51 

DERReturn DEREncodeSHA256DigestInfo(
	const DERByte	*sha256Digest,
	DERSize			sha256DigestLen,
	DERByte			*result,		/* encoded result RETURNED here */
	DERSize			*resultLen);	/* IN/OUT */

/*
 * Likewise, create an encoded DIgestInfo for specified MD5 or MD2 digest. 
 */
#define DER_MD_DIGEST_LEN			16
#define DER_MD_DIGEST_INFO_LEN		34 

typedef enum {
	WD_MD2 = 1,
	WD_MD5 = 2
} WhichDigest;

DERReturn DEREncodeMDDigestInfo(
	WhichDigest		whichDigest,
	const DERByte	*mdDigest,
	DERSize			mdDigestLen,
	DERByte			*result,		/* encoded result RETURNED here */
	DERSize			*resultLen);	/* IN/OUT */

/* max sizes you'll need in the general cases */
#define DER_MAX_DIGEST_LEN			DER_SHA256_DIGEST_LEN
#define DER_MAX_ENCODED_INFO_LEN	DER_SHA256_DIGEST_INFO_LEN

#ifdef __cplusplus
}
#endif

#endif	/* _DER_DIGEST_H_ */

