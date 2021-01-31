/** @file

AppleDxeImageVerificationLib
- Apple key-storage
- Apple PE-256 hash calculation
- Verifying Rsa2048Sha256 signature

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcAppleImageVerificationLib.h>
#include <Library/OcAppleKeysLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcPeCoffLib.h>
#include <Guid/AppleCertificate.h>

STATIC
EFI_STATUS
PeCoffGetAppleSignature (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  UINTN                           ImageSize,
  OUT APPLE_SIGNATURE_CONTEXT         *SignatureContext
  )
{
  UINTN                       Index;
  UINT32                      Result;
  APPLE_EFI_CERTIFICATE       *Cert;
  APPLE_EFI_CERTIFICATE_INFO  *CertInfo;

  //
  // FIXME: Where to take it from???
  //
  EFI_IMAGE_DATA_DIRECTORY *SecDir = NULL;

  //
  // Check SecDir extistence
  //
  if (SecDir == NULL) {
    return EFI_UNSUPPORTED;
  }

  if (SecDir->Size != APPLE_SIGNATURE_SECENTRY_SIZE) {
    DEBUG ((DEBUG_WARN, "OCAV: Certificate entry size mismatch\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Extract APPLE_EFI_CERTIFICATE_INFO
  //
  CertInfo = (APPLE_EFI_CERTIFICATE_INFO *) (
    (UINT8 *) Context->ImageBuffer + SecDir->VirtualAddress
    );

  //
  // Check for overflow
  //
  if (OcOverflowAddU32 (CertInfo->CertOffset, CertInfo->CertSize, &Result)) {
    DEBUG ((DEBUG_WARN, "OCAV: CertificateInfo causes overflow\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check that Offset+Size in ImageSize range
  //
  if (Result > ImageSize) {
    DEBUG ((DEBUG_WARN, "OCAV: CertificateInfo out of bounds\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check that certificate is expected.
  //
  if (CertInfo->CertSize != sizeof (APPLE_EFI_CERTIFICATE)) {
    DEBUG ((DEBUG_WARN, "OCAV: CertificateInfo has invalid size %u\n", CertInfo->CertSize));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Extract signature directory
  //
  Cert = (APPLE_EFI_CERTIFICATE *) (
    (UINT8 *) Context->ImageBuffer + CertInfo->CertOffset
    );

  //
  // Compare size of signature directory with value from PE SecDir header
  //
  if (CertInfo->CertSize != Cert->CertSize) {
    DEBUG ((DEBUG_WARN, "OCAV: Certificate size mismatch with CertificateInfo size value\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Verify certificate type
  //
  if (Cert->CertType != APPLE_EFI_CERTIFICATE_TYPE) {
    DEBUG ((DEBUG_WARN, "OCAV: Unknown certificate type\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Verify certificate GUID
  //
  if (!CompareGuid (&Cert->AppleSignatureGuid, &gAppleEfiCertificateGuid)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Verify HashType == Rsa2048Sha256
  //
  if (!CompareGuid (&Cert->CertData.HashType, &gEfiCertTypeRsa2048Sha256Guid)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Calc public key hash and add in sig context
  //
  Sha256 (
    SignatureContext->PublicKeyHash,
    Cert->CertData.PublicKey,
    sizeof (Cert->CertData.PublicKey)
    );

  //
  // Convert to big endian and add in sig context
  //
  STATIC_ASSERT (
    sizeof (Cert->CertData.PublicKey) == sizeof (Cert->CertData.Signature),
    "Unexpected CertData sizes"
    );
  for (Index = 0; Index < sizeof (Cert->CertData.PublicKey); Index++) {
    SignatureContext->PublicKey[sizeof (Cert->CertData.PublicKey) - 1 - Index]
      = Cert->CertData.PublicKey[Index];
    SignatureContext->Signature[sizeof (Cert->CertData.PublicKey) - 1 - Index]
      = Cert->CertData.Signature[Index];
  }

  return EFI_SUCCESS;
}

STATIC
BOOLEAN
EFIAPI
PeCoffHashAppleImageUpdate (
  IN OUT VOID        *HashContext,
  IN     CONST VOID  *Data,
  IN     UINTN       DataSize
  )
{
  Sha256Update (HashContext, Data, DataSize);
  return TRUE;
}

STATIC
EFI_STATUS
PeCoffHashAppleImage (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  OUT UINT8                           *Hash
  )
{
  BOOLEAN           Success;
  SHA256_CONTEXT    HashContext;

  //
  // Initialise a SHA hash context
  //
  Sha256Init (&HashContext);

  //
  // Hash image.
  //
  Success = PeCoffHashImage (
    Context,
    PeCoffHashAppleImageUpdate,
    &HashContext
    );
  if (!Success) {
    return EFI_UNSUPPORTED;
  }

  //
  // Return hash.
  //
  Sha256Final (&HashContext, Hash);
  return EFI_SUCCESS;

#if 0
  UINTN                    HashSize;
  UINT8                    *HashBase;

  //
  // Initialise a SHA hash context
  //
  Sha256Init (&HashContext);

  //
  // Hash DOS header and skip DOS stub
  //
  Sha256Update (&HashContext, Image, sizeof (EFI_IMAGE_DOS_HEADER));

  /**
    Measuring PE/COFF Image Header;
    But CheckSum field and SECURITY data directory (certificate) are excluded.
    Calculate the distance from the base of the image header to the image checksum address
    Hash the image header from its base to beginning of the image checksum
  **/

  HashBase = (UINT8 *) Image + ((EFI_IMAGE_DOS_HEADER *) Image)->e_lfanew;
  HashSize = (UINT8 *) Context->OptHdrChecksum - HashBase;
  Sha256Update (&HashContext, HashBase, HashSize);

  //
  // Hash everything from the end of the checksum to the start of the Cert Directory.
  //
  HashBase = (UINT8 *) Context->OptHdrChecksum + sizeof (UINT32);
  HashSize = (UINT8 *) Context->SecDir - HashBase;
  Sha256Update (&HashContext, HashBase, HashSize);

  //
  // Hash from the end of SecDirEntry till SecDir data
  //
  HashBase = (UINT8 *) Context->RelocDir;
  HashSize = (UINT8 *) Image + Context->SecDir->VirtualAddress - HashBase;
  Sha256Update (&HashContext, HashBase, HashSize);

  Sha256Final (&HashContext, Context->PeImageHash);
  return EFI_SUCCESS;
#endif
}

EFI_STATUS
VerifyApplePeImageSignature (
  IN OUT VOID                                *PeImage,
  IN OUT UINT32                              *ImageSize
  )
{
  EFI_STATUS                ImageStatus;
  PE_COFF_IMAGE_CONTEXT     ImageContext;
  APPLE_SIGNATURE_CONTEXT   SignatureContext;
  UINT8                     Hash[SHA256_DIGEST_SIZE];
  UINTN                     Result;
  UINTN                     Index;
  OC_RSA_PUBLIC_KEY         *Pk;
  BOOLEAN                   Success;

  ImageStatus = PeCoffInitializeContext (
    &ImageContext,
    PeImage,
    *ImageSize
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCAV: PeCoff init failure - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  ImageStatus = PeCoffGetAppleSignature (
    &ImageContext,
    *ImageSize,
    &SignatureContext
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCAV: PeCoff apple signature failure - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  ImageStatus = PeCoffHashAppleImage (
    &ImageContext,
    &Hash[0]
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCAV: PeCoff apple hash failure - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  //
  // Verify existence in DataBase
  //
  Pk = NULL;
  for (Index = 0; Index < NUM_OF_PK; Index++) {
    Result = CompareMem (
      PkDataBase[Index].Hash,
      SignatureContext.PublicKeyHash,
      SHA256_DIGEST_SIZE
      );
    if (Result == 0) {
      //
      // PublicKey valid. Extract prepared publickey from database
      //
      Pk = (OC_RSA_PUBLIC_KEY *) PkDataBase[Index].PublicKey;
      break;
    }
  }

  if (Pk == NULL) {
    DEBUG ((DEBUG_WARN, "OCAV: Unknown publickey or malformed certificate\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Verify signature
  //
  Success = RsaVerifySigHashFromKey (
    Pk,
    SignatureContext.Signature,
    sizeof (SignatureContext.Signature),
    &Hash[0],
    SHA256_DIGEST_SIZE,
    OcSigHashTypeSha256
    );

  if (Success) {
    DEBUG ((DEBUG_INFO, "OCAV: Signature verified!\n"));
    return EFI_SUCCESS;
  }

  return EFI_SECURITY_VIOLATION;
}
