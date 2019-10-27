/* Copyright (c) 2005-2008,2010 Apple Inc. All Rights Reserved. */

/*
 * DER_Digest.h - DER encode a DigestInfo
 *
 * Created Nov. 9 2005 by dmitch
 */

#include "DER_Digest.h"

/* 
 * Create an encoded DigestInfo based on the specified SHA1 digest. 
 * The digest must be 20 bytes long. 
 *
 * Result is placed in caller's buffer, which must be at least of
 * length DER_DIGEST_INFO_LEN bytes. 
 *
 * The *resultLen parameter is the available size in the result
 * buffer on input, and the actual length of the encoded DigestInfo 
 * on output. 
 *
 * In the interest of saving code space, this just drops the caller's 
 * digest into an otherwise hard-coded, fixed, encoded SHA1 DigestInfo.
 * Nothing is variable so we know the whole thing. It looks like this:
 *
 * SEQUENCE OF <33> {
 *		SEQUENCE OF <9> {
 *			OID <5>: OID : < 06 05 2B 0E 03 02 1A >
 *			NULL
 *		}
 *		OCTET STRING <20>:
 *			55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 
 *			55 55 55 55 
 *		}
 *		
 *
 * tower.local:digestInfo> hexdump -x /tmp/encodedDigest 
 * 0000000    3021    3009    0605    2b0e    0302    1a05    0004    1455
 * 0000010    5555    5555    5555    5555    5555    5555    5555    5555
 * *
 * 0000020
 */
 
static const unsigned char encodedSha1Digest[] = 
{
	0x30, 0x21,				/* top level sequence length 33 */
		  0x30, 0x09,		/* algorithm ID, sequence length 9 */
			    0x06, 0x05,	/* alg OID, length 5, SHA1 */
					  0x2b, 0x0e, 0x03, 0x02, 0x1a,
				0x05, 0x00,	/* NULL parameters */
		  0x04, 0x14		/* integer length 20 */
							/* digest follows */
};

DERReturn DEREncodeSHA1DigestInfo(
	const DERByte	*sha1Digest,
	DERSize			sha1DigestLen,
	DERByte			*result,		/* encoded result RETURNED here */
	DERSize			*resultLen)		/* IN/OUT */
{
	DERSize totalLen = sizeof(encodedSha1Digest) + DER_SHA1_DIGEST_LEN;
	
	if((sha1Digest == NULL) || (sha1DigestLen != DER_SHA1_DIGEST_LEN) ||
		(result == NULL) || (resultLen == NULL)) {
		return DR_ParamErr;
	}
	if(*resultLen < DER_SHA1_DIGEST_INFO_LEN) {
		return DR_BufOverflow;
	}
	DERMemmove(result, encodedSha1Digest, sizeof(encodedSha1Digest));
	DERMemmove(result + sizeof(encodedSha1Digest), sha1Digest, DER_SHA1_DIGEST_LEN);
	*resultLen = totalLen;
	return DR_Success;
}

/* 
        joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) 
        csor(3) nistalgorithm(4) hashAlgs(2) sha256(1)
        
        future ones to add: sha384(2)  sha512(3)  sha224(4)
*/
static const unsigned char encodedSha256Digest[] = 
{
	0x30, 0x31,				/* top level sequence length 49 */
		  0x30, 0x0d,		/* algorithm ID, sequence length 13 */
                0x06, 0x09,
                      0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,
                0x05, 0x00, /* NULL parameters */
		  0x04, 0x20		/* integer length 32 */
							/* digest follows */
};

DERReturn DEREncodeSHA256DigestInfo(
	const DERByte	*sha256Digest,
	DERSize			sha256DigestLen,
	DERByte			*result,		/* encoded result RETURNED here */
	DERSize			*resultLen)		/* IN/OUT */
{
	DERSize totalLen = sizeof(encodedSha256Digest) + DER_SHA256_DIGEST_LEN;
	
	if((sha256Digest == NULL) || (sha256DigestLen != DER_SHA256_DIGEST_LEN) ||
		(result == NULL) || (resultLen == NULL)) {
		return DR_ParamErr;
	}
	if(*resultLen < DER_SHA256_DIGEST_INFO_LEN) {
		return DR_BufOverflow;
	}
	DERMemmove(result, encodedSha256Digest, sizeof(encodedSha256Digest));
	DERMemmove(result + sizeof(encodedSha256Digest), sha256Digest, DER_SHA256_DIGEST_LEN);
	*resultLen = totalLen;
	return DR_Success;
}


/* Same thing, MD5/MD2 */
static const unsigned char encodedMdDigest[] = 
{
	0x30, 0x20,				/* top level sequence length 32 */
		  0x30, 0x0c,		/* algorithm ID, sequence length 12 */
			    0x06, 0x08,	/* alg OID, length 8, MD2/MD5 */
					  0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x02, 
							0x05,	/* 5 = MD5, 2 = MD2 */
				0x05, 0x00,	/* NULL parameters */
		  0x04, 0x10		/* integer length 16 */
							/* digest follows */
};

#define WHICH_DIGEST_INDEX	13
#define WHICH_DIGEST_MD2	2
#define WHICH_DIGEST_MD5	5

DERReturn DEREncodeMDDigestInfo(
	WhichDigest		whichDigest,	
	const DERByte	*mdDigest,
	DERSize			mdDigestLen,
	DERByte			*result,		/* encoded result RETURNED here */
	DERSize			*resultLen)		/* IN/OUT */
{
	DERSize totalLen = sizeof(encodedMdDigest) + DER_MD_DIGEST_LEN;
	
	if((mdDigest == NULL) || (mdDigestLen != DER_MD_DIGEST_LEN) ||
		(result == NULL) || (resultLen == NULL)) {
		return DR_ParamErr;
	}
	if(*resultLen < totalLen) {
		return DR_BufOverflow;
	}
	DERMemmove(result, encodedMdDigest, sizeof(encodedMdDigest));
	DERMemmove(result + sizeof(encodedMdDigest), mdDigest, DER_MD_DIGEST_LEN);
	switch(whichDigest) {
		case WD_MD2:
			result[WHICH_DIGEST_INDEX] = WHICH_DIGEST_MD2;
			break;
		case WD_MD5:
			result[WHICH_DIGEST_INDEX] = WHICH_DIGEST_MD5;
			break;
		default:
			return DR_ParamErr;
	}
	*resultLen = totalLen;
	return DR_Success;
}
