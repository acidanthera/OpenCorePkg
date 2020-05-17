/** @file
  Declare the GUID that is expected:

  - as EFI_SIGNATURE_DATA.SignatureOwner GUID in association with X509 and
    RSA2048 Secure Boot certificates issued by/for Microsoft,

  - as UEFI variable vendor GUID in association with (unspecified)
    Microsoft-owned variables.

  Copyright (C) 2014-2019, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  - MSDN: System.Fundamentals.Firmware at
    <https://msdn.microsoft.com/en-us/ie/dn932805(v=vs.94)>.
**/

#ifndef MICROSOFT_VARIABLE_H
#define MICROSOFT_VARIABLE_H

#include <Uefi/UefiBaseType.h>

//
// The following test cases of the Secure Boot Logo Test in the Microsoft
// Hardware Certification Kit:
//
// - Microsoft.UefiSecureBootLogo.Tests.OutOfBoxVerifyMicrosoftKEKpresent
// - Microsoft.UefiSecureBootLogo.Tests.OutOfBoxConfirmMicrosoftSignatureInDB
//
// expect the EFI_SIGNATURE_DATA.SignatureOwner GUID to be
// 77FA9ABD-0359-4D32-BD60-28F4E78F784B, when the
// EFI_SIGNATURE_DATA.SignatureData field carries any of the following X509
// certificates:
//
// - "Microsoft Corporation KEK CA 2011" (in KEK)
// - "Microsoft Windows Production PCA 2011" (in db)
// - "Microsoft Corporation UEFI CA 2011" (in db)
//
// This is despite the fact that the UEFI specification requires
// EFI_SIGNATURE_DATA.SignatureOwner to reflect the agent (i.e., OS,
// application or driver) that enrolled and therefore owns
// EFI_SIGNATURE_DATA.SignatureData, and not the organization that issued
// EFI_SIGNATURE_DATA.SignatureData.
//
#define MICROSOFT_VENDOR_GUID                           \
  { 0x77fa9abd,                                         \
    0x0359,                                             \
    0x4d32,                                             \
    { 0xbd, 0x60, 0x28, 0xf4, 0xe7, 0x8f, 0x78, 0x4b }, \
  }

extern EFI_GUID gMicrosoftVariableGuid;

#endif /* MICROSOFT_VARIABLE_H */
