/* Copyright (c) 2005-2007 Apple Inc. All Rights Reserved. */

/*
 * DER_Keys.h - support for decoding RSA keys
 *
 * Created Nov. 8 2005 by dmitch
 */
 
#ifndef	_DER_KEYS_H_
#define _DER_KEYS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "libDER.h"
#include "DER_Decode.h"

/* Algorithm Identifier components */
typedef struct {
	DERItem		oid;			/* OID */
	DERItem		params;			/* ASN_ANY, optional, DER_DEC_SAVE_DER */
} DERAlgorithmId;

/* DERItemSpecs to decode into a DERAlgorithmId */
extern const DERItemSpec DERAlgorithmIdItemSpecs[];
extern const DERSize DERNumAlgorithmIdItemSpecs;

/* X509 SubjectPublicKeyInfo */
typedef struct {
	DERItem		algId;			/* sequence, DERAlgorithmId */
	DERItem		pubKey;			/* BIT STRING */
} DERSubjPubKeyInfo;

/* DERItemSpecs to decode into a DERSubjPubKeyInfo */
extern const DERItemSpec DERSubjPubKeyInfoItemSpecs[];
extern const DERSize DERNumSubjPubKeyInfoItemSpecs;

/* 
 * RSA public key in PKCS1 format; this is inside the BIT_STRING in 
 * DERSubjPubKeyInfo.pubKey.
 */
typedef struct {
	DERItem		modulus;		/* n - INTEGER */
	DERItem		pubExponent;	/* e - INTEGER */
} DERRSAPubKeyPKCS1;

/* DERItemSpecs to decode/encode into/from a DERRSAPubKeyPKCS1 */
extern const DERItemSpec DERRSAPubKeyPKCS1ItemSpecs[];
extern const DERSize DERNumRSAPubKeyPKCS1ItemSpecs;

/* 
 * RSA public key in custom (to this library) format, including
 * the reciprocal. All fields are integers. 
 */
typedef struct {
	DERItem		modulus;		/* n */
	DERItem		reciprocal;		/* reciprocal of modulus */
	DERItem		pubExponent;	/* e */
} DERRSAPubKeyApple;

/* DERItemSpecs to decode/encode into/from a DERRSAPubKeyApple */
extern const DERItemSpec DERRSAPubKeyAppleItemSpecs[];
extern const DERSize DERNumRSAPubKeyAppleItemSpecs;

/* 
 * RSA Private key, PKCS1 format, CRT option.
 * All fields are integers. 
 */
typedef struct {
	DERItem		p;				/* p * q = n */
	DERItem		q;
	DERItem		dp;				/* d mod (p-1) */
	DERItem		dq;				/* d mod (q-1) */
	DERItem		qInv;		
} DERRSAPrivKeyCRT;

/* DERItemSpecs to decode into a DERRSAPrivKeyCRT */
extern const DERItemSpec DERRSAPrivKeyCRTItemSpecs[];
extern const DERSize DERNumRSAPrivKeyCRTItemSpecs;

/* Fully formed RSA key pair, for generating a PKCS1 private key */
typedef struct {
	DERItem		version;	
	DERItem		n;		/* modulus */
	DERItem		e;		/* public exponent */
	DERItem		d;		/* private exponent */
	DERItem		p;		/* n = p*q */
	DERItem		q;
	DERItem		dp;		/* d mod (p-1) */
	DERItem		dq;		/* d mod (q-1) */
	DERItem		qInv;	/* q^(-1) mod p */
} DERRSAKeyPair;

/* DERItemSpecs to encode a DERRSAKeyPair */
extern const DERItemSpec DERRSAKeyPairItemSpecs[];
extern const DERSize DERNumRSAKeyPairItemSpecs;

#ifdef __cplusplus
}
#endif

#endif	/* _DER_KEYS_H_ */

