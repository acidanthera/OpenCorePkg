/* Copyright (c) 2005-2009 Apple Inc. All Rights Reserved. */

/*
 * oids.h - declaration of OID consts 
 *
 * Created Nov. 11 2005 by dmitch
 */

#ifndef	_LIB_DER_OIDS_H_
#define _LIB_DER_OIDS_H_

#include "libDER.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Algorithm oids. */
extern const DERItem
    oidRsa,         /* PKCS1 RSA encryption, used to identify RSA keys */
    oidMd2Rsa,      /* PKCS1 md2withRSAEncryption signature alg */
    oidMd5Rsa,      /* PKCS1 md5withRSAEncryption signature alg */
    oidSha1Rsa,     /* PKCS1 sha1withRSAEncryption signature alg */
    oidSha256Rsa,   /* PKCS1 sha256WithRSAEncryption signature alg */
    oidEcPubKey,    /* ECDH or ECDSA public key in a certificate */
    oidSha1Ecdsa,   /* ECDSA with SHA1 signature alg */
    oidSha224Ecdsa, /* ECDSA with SHA224 signature alg */
    oidSha256Ecdsa, /* ECDSA with SHA256 signature alg */
    oidSha384Ecdsa, /* ECDSA with SHA384 signature alg */
    oidSha512Ecdsa, /* ECDSA with SHA512 signature alg */
    oidMd2,         /* OID_RSA_HASH 2 */
    oidMd4,         /* OID_RSA_HASH 4 */
    oidMd5,         /* OID_RSA_HASH 5 */
    oidSha1,        /* OID_OIW_ALGORITHM 26 */
    oidSha256,      /* OID_NIST_HASHALG 1 */
    oidSha384,      /* OID_NIST_HASHALG 2 */
    oidSha512,      /* OID_NIST_HASHALG 3 */
    oidSha224;      /* OID_NIST_HASHALG 4 */

/* Standard X.509 Cert and CRL extensions. */
extern const DERItem
    oidSubjectKeyIdentifier,
    oidKeyUsage,
    oidPrivateKeyUsagePeriod,
    oidSubjectAltName,
    oidIssuerAltName,
    oidBasicConstraints,
    oidCrlDistributionPoints,
    oidCertificatePolicies,
    oidAnyPolicy,
    oidPolicyMappings,
    oidAuthorityKeyIdentifier,
    oidPolicyConstraints,
    oidExtendedKeyUsage,
    oidAnyExtendedKeyUsage,
    oidInhibitAnyPolicy,
    oidAuthorityInfoAccess,
    oidSubjectInfoAccess,
    oidAdOCSP,
    oidAdCAIssuer,
    oidNetscapeCertType,
    oidEntrustVersInfo,
    oidMSNTPrincipalName,
    /* Policy Qualifier IDs for Internet policy qualifiers. */
    oidQtCps,
    oidQtUNotice,
    /* X.501 Name IDs. */
	oidCommonName,
    oidCountryName,
    oidLocalityName,
    oidStateOrProvinceName,
	oidOrganizationName,
	oidOrganizationalUnitName,
	oidDescription,
	oidEmailAddress,
    oidFriendlyName,
    oidLocalKeyId,
    oidExtendedKeyUsageServerAuth,
    oidExtendedKeyUsageClientAuth,
    oidExtendedKeyUsageCodeSigning,
    oidExtendedKeyUsageEmailProtection,
    oidExtendedKeyUsageOCSPSigning,
    oidExtendedKeyUsageIPSec,
    oidExtendedKeyUsageMicrosoftSGC,
    oidExtendedKeyUsageNetscapeSGC,
	/* Secure Boot Spec oid */
	oidAppleSecureBootCertSpec,
    oidAppleProvisioningProfile,
    oidAppleApplicationSigning,
    oidAppleExtendedKeyUsageAppleID,
    oidAppleIntmMarkerAppleID;

/* Compare two decoded OIDs.  Returns true iff they are equivalent. */
bool DEROidCompare(const DERItem *oid1, const DERItem *oid2);

#ifdef __cplusplus
}
#endif

#endif	/* _LIB_DER_UTILS_H_ */
