/* Copyright (c) 2005-2009 Apple Inc. All Rights Reserved. */

/*
 * DER_CertCrl.h - support for decoding X509 certificates and CRLs
 *
 * Created Nov. 4 2005 by dmitch
 */
 
#ifndef	_DER_CERT_CRL_H_
#define _DER_CERT_CRL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "libDER.h"
#include "DER_Decode.h"

/* 
 * Top level cert or CRL - the two are identical at this level - three 
 * components. The tbs field is saved in full DER form for sig verify. 
 */
typedef struct {
	DERItem		tbs;			/* sequence, DERTBSCert, DER_DEC_SAVE_DER */
	DERItem		sigAlg;			/* sequence, DERAlgorithmId */
	DERItem		sig;			/* bit string */
} DERSignedCertCrl;

/* DERItemSpecs to decode into a DERSignedCertCrl */
extern const DERItemSpec DERSignedCertCrlItemSpecs[];
extern const DERSize DERNumSignedCertCrlItemSpecs;

/* TBS cert components */
typedef struct {
	DERItem		version;		/* integer, optional, EXPLICIT */
	DERItem		serialNum;		/* integer */
	DERItem		tbsSigAlg;		/* sequence, DERAlgorithmId */
	DERItem		issuer;			/* sequence, TBD */
	DERItem		validity;		/* sequence,  DERValidity */
	DERItem		subject;		/* sequence, TBD */
	DERItem		subjectPubKey;	/* sequence, DERSubjPubKeyInfo */
	DERItem		issuerID;		/* bit string, optional */
	DERItem		subjectID;		/* bit string, optional */
	DERItem		extensions;		/* sequence, optional, EXPLICIT */
} DERTBSCert;

/* DERItemSpecs to decode into a DERTBSCert */
extern const DERItemSpec DERTBSCertItemSpecs[];
extern const DERSize DERNumTBSCertItemSpecs;

/* 
 * validity - components can be either UTC or generalized time.
 * Both are ASN_ANY with DER_DEC_SAVE_DER.
 */
typedef struct {
	DERItem		notBefore;
	DERItem		notAfter;
} DERValidity;

/* DERItemSpecs to decode into a DERValidity */
extern const DERItemSpec DERValidityItemSpecs[];
extern const DERSize DERNumValidityItemSpecs;

/* AttributeTypeAndValue components. */
typedef struct {
	DERItem		type;
	DERItem		value;
} DERAttributeTypeAndValue;

/* DERItemSpecs to decode into DERAttributeTypeAndValue */
extern const DERItemSpec DERAttributeTypeAndValueItemSpecs[];
extern const DERSize DERNumAttributeTypeAndValueItemSpecs;

/* Extension components */
typedef struct {
	DERItem		extnID;
	DERItem		critical;
	DERItem		extnValue;
} DERExtension;

/* DERItemSpecs to decode into DERExtension */
extern const DERItemSpec DERExtensionItemSpecs[];
extern const DERSize DERNumExtensionItemSpecs;

/* BasicConstraints components. */
typedef struct {
	DERItem		cA;
	DERItem		pathLenConstraint;
} DERBasicConstraints;

/* DERItemSpecs to decode into DERBasicConstraints */
extern const DERItemSpec DERBasicConstraintsItemSpecs[];
extern const DERSize DERNumBasicConstraintsItemSpecs;

/* PrivateKeyUsagePeriod components. */
typedef struct {
	DERItem		notBefore;
	DERItem		notAfter;
} DERPrivateKeyUsagePeriod;

/* DERItemSpecs to decode into a DERPrivateKeyUsagePeriod */
extern const DERItemSpec DERPrivateKeyUsagePeriodItemSpecs[];
extern const DERSize DERNumPrivateKeyUsagePeriodItemSpecs;

/* DistributionPoint components. */
typedef struct {
	DERItem		distributionPoint;
	DERItem		reasons;
    DERItem     cRLIssuer;
} DERDistributionPoint;

/* DERItemSpecs to decode into a DERDistributionPoint */
extern const DERItemSpec DERDistributionPointItemSpecs[];
extern const DERSize DERNumDistributionPointItemSpecs;

/* PolicyInformation components. */
typedef struct {
    DERItem policyIdentifier;
    DERItem policyQualifiers;
} DERPolicyInformation;

/* DERItemSpecs to decode into a DERPolicyInformation */
extern const DERItemSpec DERPolicyInformationItemSpecs[];
extern const DERSize DERNumPolicyInformationItemSpecs;

/* PolicyQualifierInfo components. */
typedef struct {
    DERItem policyQualifierID;
    DERItem qualifier;
} DERPolicyQualifierInfo;

/* DERItemSpecs to decode into a DERPolicyQualifierInfo */
extern const DERItemSpec DERPolicyQualifierInfoItemSpecs[];
extern const DERSize DERNumPolicyQualifierInfoItemSpecs;

/* UserNotice components. */
typedef struct {
    DERItem noticeRef;
    DERItem explicitText;
} DERUserNotice;

/* DERItemSpecs to decode into a DERUserNotice */
extern const DERItemSpec DERUserNoticeItemSpecs[];
extern const DERSize DERNumUserNoticeItemSpecs;

/* NoticeReference components. */
typedef struct {
    DERItem organization;
    DERItem noticeNumbers;
} DERNoticeReference;

/* DERItemSpecs to decode into a DERNoticeReference */
extern const DERItemSpec DERNoticeReferenceItemSpecs[];
extern const DERSize DERNumNoticeReferenceItemSpecs;

/* PolicyMapping components. */
typedef struct {
    DERItem issuerDomainPolicy;
    DERItem subjectDomainPolicy;
} DERPolicyMapping;

/* DERItemSpecs to decode into a DERPolicyMapping */
extern const DERItemSpec DERPolicyMappingItemSpecs[];
extern const DERSize DERNumPolicyMappingItemSpecs;

/* AccessDescription components. */
typedef struct {
    DERItem accessMethod;
    DERItem accessLocation;
} DERAccessDescription;

/* DERItemSpecs to decode into a DERAccessDescription */
extern const DERItemSpec DERAccessDescriptionItemSpecs[];
extern const DERSize DERNumAccessDescriptionItemSpecs;

/* AuthorityKeyIdentifier components. */
typedef struct {
    DERItem keyIdentifier;
    DERItem authorityCertIssuer;
    DERItem authorityCertSerialNumber;
} DERAuthorityKeyIdentifier;

/* DERItemSpecs to decode into a DERAuthorityKeyIdentifier */
extern const DERItemSpec DERAuthorityKeyIdentifierItemSpecs[];
extern const DERSize DERNumAuthorityKeyIdentifierItemSpecs;

/* OtherName components. */
typedef struct {
    DERItem typeIdentifier;
    DERItem value;
} DEROtherName;

/* DERItemSpecs to decode into a DEROtherName */
extern const DERItemSpec DEROtherNameItemSpecs[];
extern const DERSize DERNumOtherNameItemSpecs;

/* PolicyConstraints components. */
typedef struct {
    DERItem requireExplicitPolicy;
    DERItem inhibitPolicyMapping;
} DERPolicyConstraints;

/* DERItemSpecs to decode into a DERPolicyConstraints */
extern const DERItemSpec DERPolicyConstraintsItemSpecs[];
extern const DERSize DERNumPolicyConstraintsItemSpecs;

/* TBS CRL */
typedef struct {
	DERItem		version;		/* integer, optional */
	DERItem		tbsSigAlg;		/* sequence, DERAlgorithmId */
	DERItem		issuer;			/* sequence, TBD */
	DERItem		thisUpdate;		/* ASN_ANY, SAVE_DER */
	DERItem		nextUpdate;		/* ASN_ANY, SAVE_DER */
	DERItem		revokedCerts;	/* sequence of DERRevokedCert, optional */
	DERItem		extensions;		/* sequence, optional, EXPLICIT */
} DERTBSCrl;

/* DERItemSpecs to decode into a DERTBSCrl */
extern const DERItemSpec DERTBSCrlItemSpecs[];
extern const DERSize DERNumTBSCrlItemSpecs;

typedef struct {
	DERItem		serialNum;		/* integer */
	DERItem		revocationDate;	/* time - ASN_ANY, SAVE_DER */
	DERItem		extensions;		/* sequence, optional, EXPLICIT */
} DERRevokedCert;

/* DERItemSpecs to decode into a DERRevokedCert */
extern const DERItemSpec DERRevokedCertItemSpecs[];
extern const DERSize DERNumRevokedCertItemSpecs;

#ifdef __cplusplus
}
#endif

#endif	/* _DER_CERT_CRL_H_ */

