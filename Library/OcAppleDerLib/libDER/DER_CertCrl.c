/* Copyright (c) 2005-2009 Apple Inc.  All Rights Reserved. */

/*
 * DER_Cert.c - support for decoding X509 certificates
 *
 * Created Nov. 4 2005 by Doug Mitchell.
 */
 
#include "DER_Decode.h"
#include "DER_CertCrl.h"
#include "asn1Types.h"

/* 
 * DERItemSpecs for X509 certificates. 
 */
 
/* top level cert with three components */
const DERItemSpec DERSignedCertCrlItemSpecs[] = 
{
	{ DER_OFFSET(DERSignedCertCrl, tbs),
			ASN1_CONSTR_SEQUENCE,	
			DER_DEC_NO_OPTS | DER_DEC_SAVE_DER},		
	{ DER_OFFSET(DERSignedCertCrl, sigAlg),
			ASN1_CONSTR_SEQUENCE,	
			DER_DEC_NO_OPTS },		
	{ DER_OFFSET(DERSignedCertCrl, sig),
			ASN1_BIT_STRING,		
			DER_DEC_NO_OPTS }	
};

const DERSize DERNumSignedCertCrlItemSpecs = 
	sizeof(DERSignedCertCrlItemSpecs) / sizeof(DERItemSpec);

/* TBS cert */
const DERItemSpec DERTBSCertItemSpecs[] = 
{
	{ DER_OFFSET(DERTBSCert, version),
			ASN1_CONSTRUCTED | ASN1_CONTEXT_SPECIFIC | 0,		
			DER_DEC_OPTIONAL },						/* version - EXPLICIT */
	{ DER_OFFSET(DERTBSCert, serialNum),
			ASN1_INTEGER,			
			DER_DEC_NO_OPTS },		
	{ DER_OFFSET(DERTBSCert, tbsSigAlg),
			ASN1_CONSTR_SEQUENCE, 
			DER_DEC_NO_OPTS },
	{ DER_OFFSET(DERTBSCert, issuer),
			ASN1_CONSTR_SEQUENCE, 
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DERTBSCert, validity),
			ASN1_CONSTR_SEQUENCE, 
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DERTBSCert, subject),
			ASN1_CONSTR_SEQUENCE, 
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DERTBSCert, subjectPubKey),
			ASN1_CONSTR_SEQUENCE, 
			DER_DEC_NO_OPTS },	
	/* libsecurity_asn1 has these two as CONSTRUCTED, but the ASN.1 spec
	 * doesn't look that way to me. I don't have any certs that have these
	 * fields.... */
	{ DER_OFFSET(DERTBSCert, issuerID),
			ASN1_CONTEXT_SPECIFIC | 1, 
			DER_DEC_OPTIONAL },
	{ DER_OFFSET(DERTBSCert, subjectID),
			ASN1_CONTEXT_SPECIFIC | 2, 
			DER_DEC_OPTIONAL },
	{ DER_OFFSET(DERTBSCert, extensions),
			ASN1_CONSTRUCTED | ASN1_CONTEXT_SPECIFIC | 3,
			DER_DEC_OPTIONAL }
};
const DERSize DERNumTBSCertItemSpecs = sizeof(DERTBSCertItemSpecs) / sizeof(DERItemSpec);

/* DERValidity */
const DERItemSpec DERValidityItemSpecs[] = 
{
	{ DER_OFFSET(DERValidity, notBefore),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_SAVE_DER },	
	{ DER_OFFSET(DERValidity, notAfter),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_SAVE_DER }
};
const DERSize DERNumValidityItemSpecs = 
	sizeof(DERValidityItemSpecs) / sizeof(DERItemSpec);

/* DERAttributeTypeAndValue */
const DERItemSpec DERAttributeTypeAndValueItemSpecs[] = {
	{ DER_OFFSET(DERAttributeTypeAndValue, type),
			ASN1_OBJECT_ID,
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DERAttributeTypeAndValue, value),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_SAVE_DER }
};

const DERSize DERNumAttributeTypeAndValueItemSpecs =
	sizeof(DERAttributeTypeAndValueItemSpecs) / sizeof(DERItemSpec);

/* DERExtension */
const DERItemSpec DERExtensionItemSpecs[] = 
{
	{ DER_OFFSET(DERExtension, extnID),
			ASN1_OBJECT_ID,
			DER_DEC_NO_OPTS },
	{ DER_OFFSET(DERExtension, critical),
			ASN1_BOOLEAN,
			DER_DEC_OPTIONAL },
	{ DER_OFFSET(DERExtension, extnValue),
			ASN1_OCTET_STRING,
            DER_DEC_NO_OPTS }
};
const DERSize DERNumExtensionItemSpecs = 
	sizeof(DERExtensionItemSpecs) / sizeof(DERItemSpec);

/* DERBasicConstraints */
const DERItemSpec DERBasicConstraintsItemSpecs[] = 
{
	{ DER_OFFSET(DERBasicConstraints, cA),
			ASN1_BOOLEAN,
			DER_DEC_OPTIONAL },
	{ DER_OFFSET(DERBasicConstraints, pathLenConstraint),
			ASN1_INTEGER,
			DER_DEC_OPTIONAL }
};
const DERSize DERNumBasicConstraintsItemSpecs = 
	sizeof(DERBasicConstraintsItemSpecs) / sizeof(DERItemSpec);

/* DERPrivateKeyUsagePeriod. */
const DERItemSpec DERPrivateKeyUsagePeriodItemSpecs[] = 
{
	{ DER_OFFSET(DERPrivateKeyUsagePeriod, notBefore),
			ASN1_CONTEXT_SPECIFIC | 0,
			DER_DEC_OPTIONAL },	
	{ DER_OFFSET(DERPrivateKeyUsagePeriod, notAfter),
			ASN1_CONTEXT_SPECIFIC | 1,
			DER_DEC_OPTIONAL }
};
const DERSize DERNumPrivateKeyUsagePeriodItemSpecs = 
	sizeof(DERPrivateKeyUsagePeriodItemSpecs) / sizeof(DERItemSpec);

/* DERDistributionPoint. */
const DERItemSpec DERDistributionPointItemSpecs[] = 
{
	{ DER_OFFSET(DERDistributionPoint, distributionPoint),
			ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 0,
			DER_DEC_OPTIONAL },	
	{ DER_OFFSET(DERDistributionPoint, reasons),
			ASN1_CONTEXT_SPECIFIC | 1,
			DER_DEC_OPTIONAL },	
	{ DER_OFFSET(DERDistributionPoint, cRLIssuer),
			ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 2,
			DER_DEC_OPTIONAL }
};
const DERSize DERNumDistributionPointItemSpecs = 
	sizeof(DERDistributionPointItemSpecs) / sizeof(DERItemSpec);

/* DERPolicyInformation. */
const DERItemSpec DERPolicyInformationItemSpecs[] = 
{
	{ DER_OFFSET(DERPolicyInformation, policyIdentifier),
			ASN1_OBJECT_ID,
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DERPolicyInformation, policyQualifiers),
			ASN1_CONSTR_SEQUENCE,
			DER_DEC_OPTIONAL }
};
const DERSize DERNumPolicyInformationItemSpecs = 
	sizeof(DERPolicyInformationItemSpecs) / sizeof(DERItemSpec);

/* DERPolicyQualifierInfo. */
const DERItemSpec DERPolicyQualifierInfoItemSpecs[] = 
{
	{ DER_OFFSET(DERPolicyQualifierInfo, policyQualifierID),
			ASN1_OBJECT_ID,
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DERPolicyQualifierInfo, qualifier),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_SAVE_DER }
};
const DERSize DERNumPolicyQualifierInfoItemSpecs = 
	sizeof(DERPolicyQualifierInfoItemSpecs) / sizeof(DERItemSpec);

/* DERUserNotice. */
const DERItemSpec DERUserNoticeItemSpecs[] = 
{
	{ DER_OFFSET(DERUserNotice, noticeRef),
			ASN1_CONSTR_SEQUENCE,
			DER_DEC_OPTIONAL },	
	{ DER_OFFSET(DERUserNotice, explicitText),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_OPTIONAL | DER_DEC_SAVE_DER }
};
const DERSize DERNumUserNoticeItemSpecs = 
	sizeof(DERUserNoticeItemSpecs) / sizeof(DERItemSpec);

/* DERNoticeReference. */
const DERItemSpec DERNoticeReferenceItemSpecs[] = 
{
	{ DER_OFFSET(DERNoticeReference, organization),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_SAVE_DER },
	{ DER_OFFSET(DERNoticeReference, noticeNumbers),
			ASN1_CONSTR_SEQUENCE,
			DER_DEC_NO_OPTS }	
};
const DERSize DERNumNoticeReferenceItemSpecs = 
	sizeof(DERNoticeReferenceItemSpecs) / sizeof(DERItemSpec);

/* DERPolicyMapping. */
const DERItemSpec DERPolicyMappingItemSpecs[] = 
{
	{ DER_OFFSET(DERPolicyMapping, issuerDomainPolicy),
			ASN1_OBJECT_ID,
			DER_DEC_NO_OPTS },
	{ DER_OFFSET(DERPolicyMapping, subjectDomainPolicy),
			ASN1_OBJECT_ID,
			DER_DEC_NO_OPTS }
};
const DERSize DERNumPolicyMappingItemSpecs = 
	sizeof(DERPolicyMappingItemSpecs) / sizeof(DERItemSpec);

/* DERAccessDescription. */
const DERItemSpec DERAccessDescriptionItemSpecs[] = 
{
	{ DER_OFFSET(DERAccessDescription, accessMethod),
			ASN1_OBJECT_ID,
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DERAccessDescription, accessLocation),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_SAVE_DER }
};
const DERSize DERNumAccessDescriptionItemSpecs = 
	sizeof(DERAccessDescriptionItemSpecs) / sizeof(DERItemSpec);

/* DERAuthorityKeyIdentifier. */
const DERItemSpec DERAuthorityKeyIdentifierItemSpecs[] = 
{
	{ DER_OFFSET(DERAuthorityKeyIdentifier, keyIdentifier),
			ASN1_CONTEXT_SPECIFIC | 0,
			DER_DEC_OPTIONAL },
	{ DER_OFFSET(DERAuthorityKeyIdentifier, authorityCertIssuer),
			ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 1,
			DER_DEC_OPTIONAL },
	{ DER_OFFSET(DERAuthorityKeyIdentifier, authorityCertSerialNumber),
			ASN1_CONTEXT_SPECIFIC | 2,
			DER_DEC_OPTIONAL }
};
const DERSize DERNumAuthorityKeyIdentifierItemSpecs = 
	sizeof(DERAuthorityKeyIdentifierItemSpecs) / sizeof(DERItemSpec);

/* DEROtherName. */
const DERItemSpec DEROtherNameItemSpecs[] = 
{
	{ DER_OFFSET(DEROtherName, typeIdentifier),
			ASN1_OBJECT_ID,
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DEROtherName, value),
			ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 0,
			DER_DEC_NO_OPTS },
};
const DERSize DERNumOtherNameItemSpecs = 
	sizeof(DEROtherNameItemSpecs) / sizeof(DERItemSpec);

/* DERPolicyConstraints. */
const DERItemSpec DERPolicyConstraintsItemSpecs[] = 
{
	{ DER_OFFSET(DERPolicyConstraints, requireExplicitPolicy),
			ASN1_CONTEXT_SPECIFIC | 0,
			DER_DEC_OPTIONAL },
	{ DER_OFFSET(DERPolicyConstraints, inhibitPolicyMapping),
			ASN1_CONTEXT_SPECIFIC | 1,
			DER_DEC_OPTIONAL }
};
const DERSize DERNumPolicyConstraintsItemSpecs = 
	sizeof(DERPolicyConstraintsItemSpecs) / sizeof(DERItemSpec);

/* DERTBSCrl */
const DERItemSpec DERTBSCrlItemSpecs[] = 
{
	{ DER_OFFSET(DERTBSCrl, version),
			ASN1_INTEGER,		
			DER_DEC_OPTIONAL },	
	{ DER_OFFSET(DERTBSCrl, tbsSigAlg),
			ASN1_CONSTR_SEQUENCE, 
			DER_DEC_NO_OPTS },
	{ DER_OFFSET(DERTBSCrl, issuer),
			ASN1_CONSTR_SEQUENCE, 
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DERTBSCrl, thisUpdate),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_SAVE_DER },	
	{ DER_OFFSET(DERTBSCrl, nextUpdate),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_SAVE_DER },	
	{ DER_OFFSET(DERTBSCrl, revokedCerts),
			ASN1_CONSTR_SEQUENCE, 
			DER_DEC_OPTIONAL },
	{ DER_OFFSET(DERTBSCrl, extensions),
			ASN1_CONSTRUCTED | ASN1_CONTEXT_SPECIFIC | 0,
			DER_DEC_OPTIONAL }
};
const DERSize DERNumTBSCrlItemSpecs = sizeof(DERTBSCrlItemSpecs) / sizeof(DERItemSpec);

/* DERRevokedCert */
const DERItemSpec DERRevokedCertItemSpecs[] = 
{
	{ DER_OFFSET(DERRevokedCert, serialNum),
			ASN1_INTEGER,		
			DER_DEC_NO_OPTS },	
	{ DER_OFFSET(DERRevokedCert, revocationDate),
			0,					/* no tag - ANY */
			DER_DEC_ASN_ANY | DER_DEC_SAVE_DER },	
	{ DER_OFFSET(DERRevokedCert, extensions),
			ASN1_CONSTR_SEQUENCE, 
			DER_DEC_OPTIONAL }
};

const DERSize DERNumRevokedCertItemSpecs = 
	sizeof(DERRevokedCertItemSpecs) / sizeof(DERItemSpec);
