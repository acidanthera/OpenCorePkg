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
PeCoffGetDataDirectoryEntry (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  UINT32                          FileSize,
  IN  UINT32                          DirectoryEntryIndex,
  OUT CONST EFI_IMAGE_DATA_DIRECTORY  **DirectoryEntry,
  OUT UINT32                          *FileOffset
  )
{
  CONST EFI_IMAGE_SECTION_HEADER        *Sections;
  CONST EFI_IMAGE_NT_HEADERS32          *Pe32Hdr;
  CONST EFI_IMAGE_NT_HEADERS64          *Pe32PlusHdr;
  UINT32                                EntryTop;
  UINT32                                SectionOffset;
  UINT32                                SectionRawTop;
  UINT16                                SectIndex;
  BOOLEAN                               Result;

  switch (Context->ImageType) {
    case ImageTypePe32:
      Pe32Hdr = (CONST EFI_IMAGE_NT_HEADERS32 *) (CONST VOID *) (
                  (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                  );

      if (Pe32Hdr->NumberOfRvaAndSizes <= DirectoryEntryIndex) {
        return RETURN_UNSUPPORTED;
      }

      *DirectoryEntry = &Pe32Hdr->DataDirectory[DirectoryEntryIndex];
      break;

    case ImageTypePe32Plus:
      Pe32PlusHdr = (CONST EFI_IMAGE_NT_HEADERS64 *) (CONST VOID *) (
                      (CONST CHAR8 *) Context->FileBuffer + Context->ExeHdrOffset
                      );

      if (Pe32PlusHdr->NumberOfRvaAndSizes <= DirectoryEntryIndex) {
        return RETURN_UNSUPPORTED;
      }

      *DirectoryEntry = &Pe32PlusHdr->DataDirectory[DirectoryEntryIndex];
      break;

    default:
      return RETURN_INVALID_PARAMETER;
  }

  Result = OcOverflowAddU32 (
             (*DirectoryEntry)->VirtualAddress,
             (*DirectoryEntry)->Size,
             &EntryTop
             );
  if (Result || EntryTop > Context->SizeOfImage) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Determine the file offset of the debug directory...  This means we walk
  // the sections to find which section contains the RVA of the debug
  // directory
  //
  Sections = (CONST EFI_IMAGE_SECTION_HEADER *) (CONST VOID *) (
               (CONST CHAR8 *) Context->FileBuffer + Context->SectionsOffset
               );

  for (SectIndex = 0; SectIndex < Context->NumberOfSections; ++SectIndex) {
    if ((*DirectoryEntry)->VirtualAddress >= Sections[SectIndex].VirtualAddress
     && EntryTop <= Sections[SectIndex].VirtualAddress + Sections[SectIndex].VirtualSize) {
       break;
     }
  }

  if (SectIndex == Context->NumberOfSections
    || DirectoryEntryIndex == EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
    *FileOffset = (*DirectoryEntry)->VirtualAddress;
    return EFI_SUCCESS;
  }

  SectionOffset = (*DirectoryEntry)->VirtualAddress - Sections[SectIndex].VirtualAddress;
  SectionRawTop = SectionOffset + (*DirectoryEntry)->Size;
  if (SectionRawTop > Sections[SectIndex].SizeOfRawData) {
    return EFI_OUT_OF_RESOURCES;
  }

  *FileOffset = (Sections[SectIndex].PointerToRawData - Context->TeStrippedOffset) + SectionOffset;


  return RETURN_SUCCESS;
}

STATIC
EFI_STATUS
PeCoffGetAppleSignature (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  UINTN                           ImageSize,
  OUT APPLE_SIGNATURE_CONTEXT         *SignatureContext
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  UINT32                          Result;
  APPLE_EFI_CERTIFICATE           *Cert;
  APPLE_EFI_CERTIFICATE_INFO      *CertInfo;
  CONST EFI_IMAGE_DATA_DIRECTORY  *SecDir;
  UINT32                          SecOffset;

  Status = PeCoffGetDataDirectoryEntry (
    Context,
    ImageSize,
    EFI_IMAGE_DIRECTORY_ENTRY_SECURITY,
    &SecDir,
    &SecOffset
    );

  //
  // Check SecDir extistence
  //
  if (EFI_ERROR (Status)) {
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
    (UINT8 *) Context->FileBuffer + SecOffset
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
    (UINT8 *) Context->FileBuffer + CertInfo->CertOffset
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
EFI_STATUS
PeCoffHashAppleImage (
  IN  PE_COFF_IMAGE_CONTEXT           *Context,
  IN  UINT32                          ImageSize,
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
  // Hash DOS header and skip DOS stub
  //
  Sha256Update (&HashContext, Context->FileBuffer, sizeof (EFI_IMAGE_DOS_HEADER));

  /**
    Measuring PE/COFF Image Header;
    But CheckSum field and SECURITY data directory (certificate) are excluded.
    Calculate the distance from the base of the image header to the image checksum address
    Hash the image header from its base to beginning of the image checksum
  **/

  UINT8* OptHdrChecksum = (UINT8 *) Context->FileBuffer + Context->ExeHdrOffset
    + OFFSET_OF(EFI_IMAGE_NT_HEADERS64, CheckSum);
  CONST EFI_IMAGE_DATA_DIRECTORY  *SecDir;
  CONST EFI_IMAGE_DATA_DIRECTORY  *RelocDir;

  UINT32 SecOffset;
  UINT32 RelocOffset;
  EFI_STATUS Status = PeCoffGetDataDirectoryEntry (
    Context,
    ImageSize,
    EFI_IMAGE_DIRECTORY_ENTRY_SECURITY,
    &SecDir,
    &SecOffset
    );
  ASSERT_EFI_ERROR (Status);
  Status = PeCoffGetDataDirectoryEntry (
    Context,
    ImageSize,
    EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC,
    &RelocDir,
    &RelocOffset
    );
  ASSERT_EFI_ERROR (Status);

  HashBase = (UINT8 *) Context->FileBuffer + Context->ExeHdrOffset;
  HashSize = (UINT8 *) OptHdrChecksum - HashBase;
  Sha256Update (&HashContext, HashBase, HashSize);

  //
  // Hash everything from the end of the checksum to the start of the Cert Directory.
  //
  HashBase = (UINT8 *) OptHdrChecksum + sizeof (UINT32);
  HashSize = (UINT8 *) SecDir - HashBase;
  Sha256Update (&HashContext, HashBase, HashSize);

  //
  // Hash from the end of SecDirEntry till SecDir data
  //
  HashBase = (UINT8 *) RelocDir;
  HashSize = (UINT8 *) Context->FileBuffer + SecOffset - HashBase;
  Sha256Update (&HashContext, HashBase, HashSize);

  Sha256Final (&HashContext, Hash);
  return EFI_SUCCESS;
}

EFI_STATUS
PeCoffVerifyAppleSignature (
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
    *ImageSize,
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
    DEBUG ((DEBUG_INFO, "OCJS: PeCoff init failure - %r\n", ImageStatus));
    return EFI_UNSUPPORTED;
  }

  if (ImageContext.Machine != IMAGE_FILE_MACHINE_X64
    || ImageContext.ImageType != ImageTypePe32Plus
    || ImageContext.Subsystem != EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER) {
    DEBUG ((DEBUG_INFO, "OCJS: PeCoff unsupported image\n"));
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
  // Get .text contents. The sections are already verified for us.
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

  DriverVersion = (VOID *) ((UINT8 *) ImageContext.FileBuffer + SectionHeader->VirtualAddress);
  if (DriverVersion->Magic != APFS_DRIVER_VERSION_MAGIC
    || DriverVersion->ImageVersion != ImageVersion) {
    return EFI_INVALID_PARAMETER;
  }

  *DriverVersionPtr = DriverVersion;
  return EFI_SUCCESS;
}
