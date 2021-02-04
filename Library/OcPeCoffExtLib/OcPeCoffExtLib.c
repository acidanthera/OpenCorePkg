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
#include <Library/OcAppleKeysLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcPeCoffLib.h>
#include <Guid/AppleCertificate.h>

#include "OcPeCoffExtInternal.h"

STATIC
RETURN_STATUS
PeCoffGetSecurityDirectoryEntry (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  UINT32                          FileSize,
  OUT CONST EFI_IMAGE_DATA_DIRECTORY  **DirectoryEntry
  )
{
  CONST EFI_IMAGE_NT_HEADERS32          *Pe32Hdr;
  CONST EFI_IMAGE_NT_HEADERS64          *Pe32PlusHdr;
  UINT32                                EntryTop;
  BOOLEAN                               Result;

  ASSERT (Context != NULL);
  ASSERT (DirectoryEntry != NULL);

  switch (Context->ImageType) {
    case ImageTypePe32:
      Pe32Hdr = (CONST EFI_IMAGE_NT_HEADERS32 *) (CONST VOID *) (
                  (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                  );

      if (Pe32Hdr->NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
        return RETURN_UNSUPPORTED;
      }

      *DirectoryEntry = &Pe32Hdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
      break;

    case ImageTypePe32Plus:
      Pe32PlusHdr = (CONST EFI_IMAGE_NT_HEADERS64 *) (CONST VOID *) (
                      (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                      );

      if (Pe32PlusHdr->NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
        return RETURN_UNSUPPORTED;
      }

      *DirectoryEntry = &Pe32PlusHdr->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
      break;

    default:
      //
      // TE entries do not have SecDir.
      //
      return RETURN_INVALID_PARAMETER;
  }

  //
  // Security entry must be outside of the headers.
  //
  if ((*DirectoryEntry)->VirtualAddress < Context->SizeOfHeaders) {
    return RETURN_INVALID_PARAMETER;
  }

  Result = OcOverflowAddU32 (
             (*DirectoryEntry)->VirtualAddress,
             (*DirectoryEntry)->Size,
             &EntryTop
             );
  if (Result || EntryTop > FileSize) {
    return RETURN_INVALID_PARAMETER;
  }

  return RETURN_SUCCESS;
}

STATIC
EFI_STATUS
PeCoffGetAppleCertificateInfo (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  UINT32                          FileSize,
  OUT APPLE_EFI_CERTIFICATE_INFO      **CertInfo,
  OUT UINT32                          *SecDirOffset,
  OUT UINT32                          *SignedFileSize
  )
{
  EFI_STATUS                      Status;
  CONST EFI_IMAGE_DATA_DIRECTORY  *SecDir;
  UINT32                          EndOffset;

  Status = PeCoffGetSecurityDirectoryEntry (
    Context,
    FileSize,
    &SecDir
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCPE: PeCoff has no SecDir - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  *SecDirOffset = (UINT32) ((UINT8 *) SecDir - (UINT8 *) Context->FileBuffer);

  if (SecDir->Size != sizeof (APPLE_EFI_CERTIFICATE_INFO)) {
    DEBUG ((DEBUG_INFO, "OCPE: Certificate info size mismatch\n"));
    return EFI_UNSUPPORTED;
  }

  if (!OC_TYPE_ALIGNED (APPLE_EFI_CERTIFICATE_INFO, SecDir->VirtualAddress)) {
    DEBUG ((DEBUG_INFO, "OCPE: Certificate info is misaligned %X\n", SecDir->VirtualAddress));
    return EFI_UNSUPPORTED;
  }

  //
  // Obtain certificate info.
  //
  *CertInfo = (APPLE_EFI_CERTIFICATE_INFO *) (
    (UINT8 *) Context->FileBuffer + SecDir->VirtualAddress
    );
  if (OcOverflowAddU32 ((*CertInfo)->CertOffset, (*CertInfo)->CertSize, &EndOffset)
    || EndOffset > FileSize) {
    DEBUG ((DEBUG_INFO, "OCPE: Certificate entry is beyond file area\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update signed file size to signature location.
  //
  *SignedFileSize = SecDir->VirtualAddress;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PeCoffGetAppleSignature (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  APPLE_EFI_CERTIFICATE_INFO      *CertInfo,
  OUT APPLE_SIGNATURE_CONTEXT         *SignatureContext
  )
{
  UINTN                           Index;
  UINT8                           PublicKeyHash[SHA256_DIGEST_SIZE];
  APPLE_EFI_CERTIFICATE           *Cert;

  //
  // Check that certificate is expected.
  //
  if (CertInfo->CertSize != sizeof (APPLE_EFI_CERTIFICATE)) {
    DEBUG ((DEBUG_INFO, "OCPE: Certificate has invalid size %u\n", CertInfo->CertSize));
    return EFI_UNSUPPORTED;
  }

  if (!OC_TYPE_ALIGNED (APPLE_EFI_CERTIFICATE, CertInfo->CertOffset)) {
    DEBUG ((DEBUG_INFO, "OCPE: Certificate is misaligned %X\n", CertInfo->CertOffset));
    return EFI_UNSUPPORTED;
  }

  //
  // Extract signature directory
  //
  Cert = (APPLE_EFI_CERTIFICATE *) (
    (UINT8 *) Context->FileBuffer + CertInfo->CertOffset
    );

  //
  // Compare size of signature directory with value from PE SecDir header
  //
  if (CertInfo->CertSize != Cert->CertSize) {
    DEBUG ((DEBUG_INFO, "OCPE: Certificate size mismatch %u vs %u\n", CertInfo->CertSize, Cert->CertSize));
    return EFI_UNSUPPORTED;
  }

  //
  // Verify certificate type
  //
  if (Cert->CertType != APPLE_EFI_CERTIFICATE_TYPE) {
    DEBUG ((DEBUG_INFO, "OCPE: Unknown certificate type %u\n", Cert->CertType));
    return EFI_UNSUPPORTED;
  }

  //
  // Verify certificate GUID
  //
  if (!CompareGuid (&Cert->AppleSignatureGuid, &gAppleEfiCertificateGuid)) {
    DEBUG ((DEBUG_INFO, "OCPE: Unknown certificate signature %g\n", Cert->AppleSignatureGuid));
    return EFI_UNSUPPORTED;
  }

  //
  // Verify HashType == Rsa2048Sha256
  //
  if (!CompareGuid (&Cert->CertData.HashType, &gEfiCertTypeRsa2048Sha256Guid)) {
    DEBUG ((DEBUG_INFO, "OCPE: Unknown certificate hash %g\n", Cert->CertData.HashType));
    return EFI_UNSUPPORTED;
  }

  //
  // Calculate public key hash and find it.
  //
  Sha256 (
    PublicKeyHash,
    Cert->CertData.PublicKey,
    sizeof (Cert->CertData.PublicKey)
    );

  //
  // Verify public key existence in the database and store it in the context.
  //
  for (Index = 0; Index < NUM_OF_PK; ++Index) {
    if (CompareMem (PkDataBase[Index].Hash, PublicKeyHash, sizeof (PublicKeyHash)) == 0) {
      SignatureContext->PublicKey = (OC_RSA_PUBLIC_KEY *) PkDataBase[Index].PublicKey;
      break;
    }
  }

  if (Index == NUM_OF_PK) {
    DEBUG ((DEBUG_INFO, "OCPE: Unknown publickey or malformed certificate\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Convert signature to big endian and store it in the context as well.
  //
  for (Index = 0; Index < sizeof (Cert->CertData.PublicKey); Index++) {
    SignatureContext->Signature[sizeof (Cert->CertData.PublicKey) - 1 - Index]
      = Cert->CertData.Signature[Index];
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PeCoffSanitiseAppleImage (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  UINT32                          SecDirOffset,
  IN  UINT32                          SignedFileSize,
  IN  UINT32                          FileSize
  )
{
  //
  // While TE files cannot technically have SecDir,
  // one might add more PE types in the future technically.
  // Restrict file type as early as possible.
  //
  if (Context->ImageType != ImageTypePe32
    && Context->ImageType != ImageTypePe32Plus) {
    DEBUG ((DEBUG_INFO, "OCPE: Unsupported image type %d for Apple Image\n", Context->ImageType));
    return EFI_UNSUPPORTED;
  }

  //
  // Apple images are required to start with DOS header.
  // DOS header is really the only reason ExeHdrOffset can be
  // non-zero, do not bother extra checking.
  //
  if (Context->ExeHdrOffset < sizeof (EFI_IMAGE_DOS_HEADER)) {
    DEBUG ((DEBUG_INFO, "OCPE: DOS header is required for signed Apple Image\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Zero memory between DOS and optional header.
  //
  ZeroMem (
    (UINT8 *) Context->FileBuffer + sizeof (EFI_IMAGE_DOS_HEADER),
    Context->ExeHdrOffset - sizeof (EFI_IMAGE_DOS_HEADER)
    );

  //
  // Zero checksum as we do not hash it.
  //
  ZeroMem (
    (UINT8 *) Context->FileBuffer + Context->ExeHdrOffset + APPLE_CHECKSUM_OFFSET,
    APPLE_CHECKSUM_SIZE
    );

  //
  // Zero sec entry as we do not hash it as well.
  //
  ZeroMem (
    (UINT8 *) Context->FileBuffer + SecDirOffset,
    sizeof (EFI_IMAGE_DATA_DIRECTORY)
    );

  //
  // Zero signature as we do not hash it.
  //
  ZeroMem (
    (UINT8 *) Context->FileBuffer + SignedFileSize,
    FileSize - SignedFileSize
    );

  return EFI_SUCCESS;
}

STATIC
VOID
PeCoffHashAppleImage (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  UINT32                          SecDirOffset,
  IN  UINT32                          SignedFileSize,
  OUT UINT8                           *Hash
  )
{
  UINTN                    HashSize;
  UINT8                    *HashBase;
  SHA256_CONTEXT           HashContext;

  //
  // Initialise a SHA hash context
  //
  Sha256Init (&HashContext);

  //
  // Hash DOS header and without DOS stub.
  //
  HashBase = (UINT8 *) Context->FileBuffer;
  HashSize = sizeof (EFI_IMAGE_DOS_HEADER);
  Sha256Update (&HashContext, HashBase, HashSize);

  //
  // Hash optional header prior to checksum.
  //
  HashBase += Context->ExeHdrOffset;
  HashSize = APPLE_CHECKSUM_OFFSET;
  Sha256Update (&HashContext, HashBase, HashSize);

  //
  // Hash the rest of the header up to security directory.
  //
  HashBase += HashSize + APPLE_CHECKSUM_SIZE;
  HashSize = SecDirOffset - (UINT32) (HashBase - (UINT8 *) Context->FileBuffer);
  Sha256Update (&HashContext, HashBase, HashSize);

  //
  // Hash the rest of the image skipping security directory.
  //
  HashBase += HashSize + sizeof (EFI_IMAGE_DATA_DIRECTORY);
  HashSize = SignedFileSize - (UINT32) (HashBase - (UINT8 *) Context->FileBuffer);
  Sha256Update (&HashContext, HashBase, HashSize);

  Sha256Final (&HashContext, Hash);
}

EFI_STATUS
PeCoffVerifyAppleSignature (
  IN OUT VOID                                *PeImage,
  IN OUT UINT32                              *ImageSize
  )
{
  EFI_STATUS                      ImageStatus;
  PE_COFF_IMAGE_CONTEXT           ImageContext;
  APPLE_SIGNATURE_CONTEXT         SignatureContext;
  UINT8                           Hash[SHA256_DIGEST_SIZE];
  BOOLEAN                         Success;
  APPLE_EFI_CERTIFICATE_INFO      *CertInfo;
  UINT32                          SecDirOffset;
  UINT32                          SignedFileSize;

  ImageStatus = PeCoffInitializeContext (
    &ImageContext,
    PeImage,
    *ImageSize
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCPE: PeCoff init failure - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  ImageStatus = PeCoffGetAppleCertificateInfo (
    &ImageContext,
    *ImageSize,
    &CertInfo,
    &SecDirOffset,
    &SignedFileSize
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCPE: PeCoff no cert info - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  ImageStatus = PeCoffGetAppleSignature (
    &ImageContext,
    CertInfo,
    &SignatureContext
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCPE: PeCoff no valid signature - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  ImageStatus = PeCoffSanitiseAppleImage (
    &ImageContext,
    SecDirOffset,
    SignedFileSize,
    *ImageSize
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCPE: PeCoff cannot be sanitised - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  *ImageSize = SignedFileSize;

  PeCoffHashAppleImage (
    &ImageContext,
    SecDirOffset,
    SignedFileSize,
    &Hash[0]
    );

  //
  // Verify signature
  //
  Success = RsaVerifySigHashFromKey (
    SignatureContext.PublicKey,
    SignatureContext.Signature,
    sizeof (SignatureContext.Signature),
    &Hash[0],
    sizeof (Hash),
    OcSigHashTypeSha256
    );
  if (!Success) {
    return EFI_SECURITY_VIOLATION;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PeCoffGetApfsDriverVersion (
  IN  VOID                 *DriverBuffer,
  IN  UINT32               DriverSize,
  OUT APFS_DRIVER_VERSION  **DriverVersionPtr
  )
{
  //
  // apfs.efi versioning is more restricted than generic PE parsing.
  //

  EFI_STATUS                ImageStatus;
  PE_COFF_IMAGE_CONTEXT     ImageContext;
  EFI_IMAGE_NT_HEADERS64    *OptionalHeader;
  EFI_IMAGE_SECTION_HEADER  *SectionHeader;
  APFS_DRIVER_VERSION       *DriverVersion;
  UINT32                    ImageVersion;

  ImageStatus = PeCoffInitializeContext (
    &ImageContext,
    DriverBuffer,
    DriverSize
    );
  if (EFI_ERROR (ImageStatus)) {
    DEBUG ((DEBUG_INFO, "OCPE: PeCoff init apfs failure - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  if (ImageContext.Machine != IMAGE_FILE_MACHINE_X64
    || ImageContext.ImageType != ImageTypePe32Plus
    || ImageContext.Subsystem != EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER) {
    DEBUG ((DEBUG_INFO, "OCPE: PeCoff unsupported image\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Get image version. The header is already verified for us.
  //
  OptionalHeader = (EFI_IMAGE_NT_HEADERS64 *)(
    (CONST UINT8 *) ImageContext.FileBuffer
    + ImageContext.ExeHdrOffset
    );
  ImageVersion = (UINT32) OptionalHeader->MajorImageVersion << 16
    | (UINT32) OptionalHeader->MinorImageVersion;

  //
  // Get .text contents. The sections are already verified for us,
  // but it can be smaller than APFS version.
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *)(
    (CONST UINT8 *) ImageContext.FileBuffer
    + ImageContext.SectionsOffset
    );

  if (AsciiStrnCmp ((CHAR8*) SectionHeader->Name, ".text", sizeof (SectionHeader->Name)) != 0) {
    return EFI_UNSUPPORTED;
  }

  if (sizeof (APFS_DRIVER_VERSION) > SectionHeader->SizeOfRawData) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Finally get driver version.
  //
  DriverVersion = (APFS_DRIVER_VERSION  *)(
    (CONST UINT8 *) ImageContext.FileBuffer
    + SectionHeader->PointerToRawData
    );

  if (DriverVersion->Magic != APFS_DRIVER_VERSION_MAGIC
    || DriverVersion->ImageVersion != ImageVersion) {
    return EFI_INVALID_PARAMETER;
  }

  *DriverVersionPtr = DriverVersion;
  return EFI_SUCCESS;
}
