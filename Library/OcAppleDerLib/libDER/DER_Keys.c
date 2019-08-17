/* Copyright (c) 2005-2007 Apple Inc.  All Rights Reserved. */

/*
 * DER_Cert.c - support for decoding RSA keys
 *
 * Created Nov. 8 2005 by Doug Mitchell.
 */
 
#include "DER_Decode.h"
#include "DER_Encode.h"
#include "DER_Keys.h"
#include "asn1Types.h"
#include "libDER_config.h"

#ifndef	DER_DECODE_ENABLE
#error Please define DER_DECODE_ENABLE.
#endif
#if		DER_DECODE_ENABLE

/* 
 * DERItemSpecs for decoding RSA keys. 
 */
 
/* Algorithm Identifier */
const DERItemSpec DERAlgorithmIdItemSpecs[] = 
{
	{ DER_OFFSET(DERAlgorithmId, oid),
			ASN1_OBJECT_ID,
			DER_DEC_NO_OPTS },
	{ DER_OFFSET(DERAlgorithmId, params),
			0,				/* no tag - any */
			DER_DEC_ASN_ANY | DER_DEC_OPTIONAL | DER_DEC_SAVE_DER }
};
const DERSize DERNumAlgorithmIdItemSpecs = 
	sizeof(DERAlgorithmIdItemSpecs) / sizeof(DERItemSpec);

/* X509 SubjectPublicKeyInfo */
const DERItemSpec DERSubjPubKeyInfoItemSpecs[] = 
{
	{ DER_OFFSET(DERSubjPubKeyInfo, algId),
			ASN1_CONSTR_SEQUENCE,	
			DER_DEC_NO_OPTS },		
	{ DER_OFFSET(DERSubjPubKeyInfo, pubKey),
			ASN1_BIT_STRING,	
			DER_DEC_NO_OPTS },		

};
const DERSize DERNumSubjPubKeyInfoItemSpecs = 
	sizeof(DERSubjPubKeyInfoItemSpecs) / sizeof(DERItemSpec);

/* 
 * RSA private key in CRT format
 */
const DERItemSpec DERRSAPrivKeyCRTItemSpecs[] = 
{
	/* version, n, e, d - skip */
	{ 0,
			ASN1_INTEGER,
			DER_DEC_SKIP },
	{ 0,
			ASN1_INTEGER,
			DER_DEC_SKIP },
	{ 0,
			ASN1_INTEGER,
			DER_DEC_SKIP },
	{ 0,
			ASN1_INTEGER,
			DER_DEC_SKIP },
	{ DER_OFFSET(DERRSAPrivKeyCRT, p),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS },		
	{ DER_OFFSET(DERRSAPrivKeyCRT, q),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS },		
	{ DER_OFFSET(DERRSAPrivKeyCRT, dp),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS },		
	{ DER_OFFSET(DERRSAPrivKeyCRT, dq),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS },		
	{ DER_OFFSET(DERRSAPrivKeyCRT, qInv),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS },		
	/* ignore the (optional) rest */
};
const DERSize DERNumRSAPrivKeyCRTItemSpecs = 
	sizeof(DERRSAPrivKeyCRTItemSpecs) / sizeof(DERItemSpec);

#endif	/* DER_DECODE_ENABLE */

#if		DER_DECODE_ENABLE || DER_ENCODE_ENABLE

/* RSA public key in PKCS1 format - encode and decode */
const DERItemSpec DERRSAPubKeyPKCS1ItemSpecs[] = 
{
	{ DER_OFFSET(DERRSAPubKeyPKCS1, modulus),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS | DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAPubKeyPKCS1, pubExponent),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS | DER_ENC_SIGNED_INT },		
};
const DERSize DERNumRSAPubKeyPKCS1ItemSpecs = 
	sizeof(DERRSAPubKeyPKCS1ItemSpecs) / sizeof(DERItemSpec);

/* RSA public key in Apple custome format with reciprocal - encode and decode */
const DERItemSpec DERRSAPubKeyAppleItemSpecs[] = 
{
	{ DER_OFFSET(DERRSAPubKeyApple, modulus),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS | DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAPubKeyApple, reciprocal),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS | DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAPubKeyApple, pubExponent),
			ASN1_INTEGER,	
			DER_DEC_NO_OPTS | DER_ENC_SIGNED_INT },		
};
const DERSize DERNumRSAPubKeyAppleItemSpecs = 
	sizeof(DERRSAPubKeyAppleItemSpecs) / sizeof(DERItemSpec);


#endif		/* DER_DECODE_ENABLE || DER_ENCODE_ENABLE */

#ifndef	DER_ENCODE_ENABLE
#error Please define DER_ENCODE_ENABLE.
#endif

#if		DER_ENCODE_ENABLE

/* RSA Key Pair, encode only */
const DERItemSpec DERRSAKeyPairItemSpecs[] = 
{
	{ DER_OFFSET(DERRSAKeyPair, version),
			ASN1_INTEGER,	
			DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAKeyPair, n),
			ASN1_INTEGER,	
			DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAKeyPair, e),
			ASN1_INTEGER,	
			DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAKeyPair, d),
			ASN1_INTEGER,	
			DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAKeyPair, p),
			ASN1_INTEGER,	
			DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAKeyPair, q),
			ASN1_INTEGER,	
			DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAKeyPair, dp),
			ASN1_INTEGER,	
			DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAKeyPair, dq),
			ASN1_INTEGER,	
			DER_ENC_SIGNED_INT },		
	{ DER_OFFSET(DERRSAKeyPair, qInv),
			ASN1_INTEGER,	
			DER_ENC_SIGNED_INT },		
};

const DERSize DERNumRSAKeyPairItemSpecs = 
	sizeof(DERRSAKeyPairItemSpecs) / sizeof(DERItemSpec);

#endif	/* DER_ENCODE_ENABLE */

