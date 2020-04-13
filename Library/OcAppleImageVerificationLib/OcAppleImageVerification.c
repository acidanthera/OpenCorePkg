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
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcAppleImageVerificationLib.h>
#include <Library/OcAppleKeysLib.h>
#include <Library/OcGuardLib.h>
#include <Protocol/DebugSupport.h>
#include <IndustryStandard/PeImage.h>
#include <Guid/AppleCertificate.h>

EFI_STATUS
BuildPeContext (
  VOID                                *Image,
  UINTN                               ImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  )
{
  EFI_IMAGE_DOS_HEADER             *DosHdr              = NULL;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *PeHdr               = NULL;
  UINT16                           PeHdrMagic           = 0;
  UINT32                           HeaderWithoutDataDir = 0;
  UINT32                           SectionHeaderOffset  = 0;
  EFI_IMAGE_SECTION_HEADER         *SectionCache        = NULL;
  UINT32                           Index                = 0;
  UINT32                           MaxHeaderSize        = 0;
  UINTN                            TempN                = 0;
  UINT32                           Temp32               = 0;

  //
  // Check context existence
  //
  if (Context == NULL) {
    DEBUG ((DEBUG_WARN, "OCAV: No context provided\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Verify Image size
  //
  if (sizeof (EFI_IMAGE_DOS_HEADER) >= sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION)) {
    MaxHeaderSize = sizeof (EFI_IMAGE_DOS_HEADER);
  } else {
    MaxHeaderSize = sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION);
  }
  if (ImageSize < MaxHeaderSize) {
    DEBUG ((DEBUG_WARN, "OCAV: Invalid image\n"));
    return EFI_INVALID_PARAMETER;
  }

  DosHdr = Image;

  //
  // Verify DosHdr magic
  //
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    if (DosHdr->e_lfanew > ImageSize
      || DosHdr->e_lfanew < sizeof (EFI_IMAGE_DOS_HEADER)) {
      DEBUG ((DEBUG_WARN, "OCAV: Invalid PE offset\n"));
      return EFI_INVALID_PARAMETER;
    }

    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *) ((UINT8 *) Image
                                                 + DosHdr->e_lfanew);

    if (OcOverflowSubUN (ImageSize, sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION), &TempN)) {
      DEBUG ((DEBUG_WARN, "OCAV: Underflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    if ((UINT8 *) Image + TempN < (UINT8 *) PeHdr) {
      DEBUG ((DEBUG_WARN, "OCAV: Invalid PE location\n"));
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // DosHdr truncated
    //
    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *) Image;
  }

  PeHdrMagic = PeHdr->Pe32.OptionalHeader.Magic;

  if (PeHdrMagic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Pe32 part
    //

    //
    // Check image header size
    //
    if (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES <
        PeHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes) {
      DEBUG ((DEBUG_WARN, "OCAV: Image header too small\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES >
        PeHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes) {
      DEBUG ((DEBUG_WARN, "OCAV: Non-standard entries present\n"));
      return EFI_INVALID_PARAMETER;
    }

    //
    // Check image header aligment
    //
    HeaderWithoutDataDir = sizeof (EFI_IMAGE_OPTIONAL_HEADER32) -
                           sizeof (EFI_IMAGE_DATA_DIRECTORY) *
                           EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;

    if (OcOverflowSubU32 (PeHdr->Pe32.FileHeader.SizeOfOptionalHeader,
                          HeaderWithoutDataDir, &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Underflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (PeHdr->Pe32.FileHeader.SizeOfOptionalHeader < HeaderWithoutDataDir
      || Temp32 != PeHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes
                 * sizeof (EFI_IMAGE_DATA_DIRECTORY)) {
      DEBUG ((DEBUG_WARN, "OCAV: Image header overflows data directory\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (OcOverflowTriAddU32 (DosHdr->e_lfanew, sizeof (UINT32),
                             sizeof (EFI_IMAGE_FILE_HEADER), &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Overflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }
    if (OcOverflowAddU32 (Temp32, PeHdr->Pe32.FileHeader.SizeOfOptionalHeader, &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Overflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    SectionHeaderOffset = Temp32;

    if (OcOverflowSubU32 (PeHdr->Pe32.OptionalHeader.SizeOfImage, SectionHeaderOffset, &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Underflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (PeHdr->Pe32.OptionalHeader.SizeOfImage < SectionHeaderOffset ||
      (Temp32 / EFI_IMAGE_SIZEOF_SECTION_HEADER) <= PeHdr->Pe32.FileHeader.NumberOfSections) {
      DEBUG ((DEBUG_WARN, "OCAV: Image sections overflow image size\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (OcOverflowSubU32 (PeHdr->Pe32.OptionalHeader.SizeOfHeaders, SectionHeaderOffset, &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Underflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (PeHdr->Pe32.OptionalHeader.SizeOfHeaders < SectionHeaderOffset
      || (Temp32 / EFI_IMAGE_SIZEOF_SECTION_HEADER)
      < (UINT32) PeHdr->Pe32.FileHeader.NumberOfSections) {
        DEBUG ((DEBUG_WARN, "OCAV: Image sections overflow section headers\n"));
        return EFI_INVALID_PARAMETER;
    }

  } else if (PeHdrMagic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    //
    // Pe32+ part
    //

    //
    // Check image header size
    //
    if (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES <
        PeHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes) {
      DEBUG ((DEBUG_WARN, "OCAV: Image header too small\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES >
        PeHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes) {
      DEBUG ((DEBUG_WARN, "OCAV: Non-standard entries present\n"));
      return EFI_INVALID_PARAMETER;
    }

    //
    // Check image header aligment
    //
    HeaderWithoutDataDir = sizeof (EFI_IMAGE_OPTIONAL_HEADER64) -
                           sizeof (EFI_IMAGE_DATA_DIRECTORY) *
                           EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;

    if (OcOverflowSubU32 (PeHdr->Pe32Plus.FileHeader.SizeOfOptionalHeader, HeaderWithoutDataDir, &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Underflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (PeHdr->Pe32Plus.FileHeader.SizeOfOptionalHeader < HeaderWithoutDataDir
      || Temp32 != PeHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes
                 * sizeof (EFI_IMAGE_DATA_DIRECTORY)) {
      DEBUG ((DEBUG_WARN, "OCAV: Image header overflows data directory\n"));
      return EFI_INVALID_PARAMETER;
    }

    //
    // Check image sections overflow
    //
    if (OcOverflowTriAddU32 (DosHdr->e_lfanew, sizeof (UINT32),
                             sizeof (EFI_IMAGE_FILE_HEADER), &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Overflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (OcOverflowAddU32 (Temp32, PeHdr->Pe32Plus.FileHeader.SizeOfOptionalHeader, &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Overflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    SectionHeaderOffset = Temp32;

    if (OcOverflowSubU32 (PeHdr->Pe32Plus.OptionalHeader.SizeOfImage, SectionHeaderOffset, &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Underflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (PeHdr->Pe32Plus.OptionalHeader.SizeOfImage < SectionHeaderOffset
      || (Temp32 / EFI_IMAGE_SIZEOF_SECTION_HEADER)
      <= PeHdr->Pe32Plus.FileHeader.NumberOfSections) {
      DEBUG ((DEBUG_WARN, "OCAV: Image sections overflow image size\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (OcOverflowSubU32 (PeHdr->Pe32Plus.OptionalHeader.SizeOfHeaders, SectionHeaderOffset, &Temp32)) {
      DEBUG ((DEBUG_WARN, "OCAV: Underflow detected!\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (PeHdr->Pe32Plus.OptionalHeader.SizeOfHeaders < SectionHeaderOffset
      || (Temp32 / EFI_IMAGE_SIZEOF_SECTION_HEADER)
      < (UINT32) PeHdr->Pe32Plus.FileHeader.NumberOfSections) {
      DEBUG ((DEBUG_WARN, "OCAV: Image sections overflow section headers\n"));
      return EFI_INVALID_PARAMETER;
    }
  } else {
    DEBUG ((DEBUG_WARN, "OCAV: Unsupported PE header magic\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (PeHdr->Te.Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_WARN, "OCAV: Unsupported image type\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (PeHdr->Pe32.FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED) {
    DEBUG ((DEBUG_WARN, "OCAV: Unsupported image - Relocations have been stripped\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // FIXME: Sanitize missing fields
  //
  if (PeHdrMagic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Fill context for Pe32
    //
    Context->PeHdr = PeHdr;
    Context->ImageAddress = PeHdr->Pe32.OptionalHeader.ImageBase;
    Context->ImageSize = PeHdr->Pe32.OptionalHeader.SizeOfImage;
    Context->SizeOfOptionalHeader = PeHdr->Pe32.FileHeader.SizeOfOptionalHeader;
    Context->OptHdrChecksum= &Context->PeHdr->Pe32.OptionalHeader.CheckSum;
    Context->SizeOfHeaders = PeHdr->Pe32.OptionalHeader.SizeOfHeaders;
    Context->EntryPoint = PeHdr->Pe32.OptionalHeader.AddressOfEntryPoint;
    Context->RelocDir = &PeHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    Context->NumberOfRvaAndSizes = PeHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes;
    Context->NumberOfSections = PeHdr->Pe32.FileHeader.NumberOfSections;
    Context->FirstSection = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) PeHdr
                            + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader
                            + sizeof (UINT32)
                            + sizeof (EFI_IMAGE_FILE_HEADER));
    if (Context->NumberOfSections >= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
      Context->SecDir = &PeHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
    }
  } else {
    //
    // Fill context for Pe32+
    //
    Context->PeHdr = PeHdr;
    Context->ImageAddress = PeHdr->Pe32Plus.OptionalHeader.ImageBase;
    Context->ImageSize = PeHdr->Pe32Plus.OptionalHeader.SizeOfImage;
    Context->SizeOfOptionalHeader = PeHdr->Pe32.FileHeader.SizeOfOptionalHeader;
    Context->OptHdrChecksum= &Context->PeHdr->Pe32Plus.OptionalHeader.CheckSum;
    Context->SizeOfHeaders = PeHdr->Pe32Plus.OptionalHeader.SizeOfHeaders;
    Context->EntryPoint = PeHdr->Pe32Plus.OptionalHeader.AddressOfEntryPoint;
    Context->RelocDir = &PeHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    Context->NumberOfRvaAndSizes = PeHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes;
    Context->NumberOfSections = PeHdr->Pe32.FileHeader.NumberOfSections;
    Context->FirstSection = (EFI_IMAGE_SECTION_HEADER *) ((UINT8 *) PeHdr
                            + PeHdr->Pe32.FileHeader.SizeOfOptionalHeader
                            + sizeof (UINT32)
                            + sizeof (EFI_IMAGE_FILE_HEADER));
    if (Context->NumberOfSections >= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
      Context->SecDir = &PeHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
    }
  }

  //
  // Fill sections info
  //
  Context->PeHdrMagic = PeHdrMagic;
  SectionCache = Context->FirstSection;

  for (Index = 0, Context->SumOfSectionBytes = 0;
       Index < Context->NumberOfSections; Index++, SectionCache++) {
    if (Context->SumOfSectionBytes + SectionCache->SizeOfRawData
      < Context->SumOfSectionBytes) {
      DEBUG ((DEBUG_WARN, "OCAV: Malformed binary: %x %x\n", (UINT32) Context->SumOfSectionBytes, ImageSize));
      return EFI_INVALID_PARAMETER;
    }
    Context->SumOfSectionBytes += SectionCache->SizeOfRawData;
  }

  if (Context->SumOfSectionBytes >= ImageSize) {
    DEBUG ((DEBUG_WARN, "OCAV: Malformed binary: %x %x\n", (UINT32) Context->SumOfSectionBytes, ImageSize));
    return EFI_INVALID_PARAMETER;
  }

  if (Context->ImageSize < Context->SizeOfHeaders) {
    DEBUG ((DEBUG_WARN, "OCAV: Invalid image\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (Context->SecDir != NULL) {
    if ((UINT32) ((UINT8 *) Context->SecDir - (UINT8 *) Image) >
        (ImageSize - sizeof (EFI_IMAGE_DATA_DIRECTORY))) {
      DEBUG ((DEBUG_WARN, "OCAV: Invalid image\n"));
      return EFI_INVALID_PARAMETER;
    }

    if (Context->SecDir->VirtualAddress >= ImageSize) {
      DEBUG ((DEBUG_WARN, "OCAV: Malformed security header\n"));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}


EFI_STATUS
GetApplePeImageSignature (
  VOID                                *Image,
  UINTN                               ImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  APPLE_SIGNATURE_CONTEXT             *SignatureContext
  )
{
  EFI_STATUS                  Status                    = EFI_UNSUPPORTED;
  UINTN                       Index                     = 0;
  UINT32                      Result                    = 0;
  APPLE_EFI_CERTIFICATE       *Cert                     = NULL;
  APPLE_EFI_CERTIFICATE_INFO  *CertInfo                 = NULL;
  UINT8                       PkLe[256];
  UINT8                       SigLe[256];
  //
  // Check SecDir extistence
  //
  if (Context->SecDir != NULL) {
    //
    // Apple EFI_IMAGE_DIRECTORY_ENTRY_SECURITY is always 8 bytes.
    //
    if (Context->SecDir->Size != APPLE_SIGNATURE_SECENTRY_SIZE) {
      DEBUG ((DEBUG_WARN, "OCAV: Certificate entry size mismatch\n"));
      return Status;
    }
    //
    // Extract APPLE_EFI_CERTIFICATE_INFO
    //
    CertInfo = (APPLE_EFI_CERTIFICATE_INFO *)
                      ((UINT8 *) Image + Context->SecDir->VirtualAddress);

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
    // Extract signature directory
    //
    Cert = (APPLE_EFI_CERTIFICATE *)
             ((UINT8 *) Image + CertInfo->CertOffset);

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
      return Status;
    }

    //
    // Verify HashType == Rsa2048Sha256
    //
    if (!CompareGuid (&Cert->CertData.HashType, &gEfiCertTypeRsa2048Sha256Guid)) {
      return EFI_UNSUPPORTED;
    }

    //
    // Extract PublicKey and Signature
    //
    CopyMem (PkLe, Cert->CertData.PublicKey, 256);
    CopyMem (SigLe, Cert->CertData.Signature, 256);

    //
    // Calc public key hash and add in sig context
    //
    Sha256 (SignatureContext->PublicKeyHash, PkLe, 256);

    //
    // Convert to big endian and add in sig context
    //
    for (Index = 0; Index < 256; Index++) {
      SignatureContext->PublicKey[256 - 1 - Index] = PkLe[Index];
      SignatureContext->Signature[256 - 1 - Index] = SigLe[Index];
    }

    Status = EFI_SUCCESS;

  } else {
    DEBUG ((DEBUG_WARN, "OCAV: Certificate entry not exist\n"));
  }

  return Status;
}

VOID
SanitizeApplePeImage (
  VOID                                *Image,
  UINTN                               *RealImageSize,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  )
{
  UINTN                    ImageSize            = *RealImageSize;
  EFI_IMAGE_DOS_HEADER     *DosHdr              = Image;

  //
  // Check DOS header existence
  //

  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // Drop DOS stub if it present
    //
    if ((((EFI_IMAGE_DOS_HEADER *) Image)->e_lfanew
          - sizeof (EFI_IMAGE_DOS_HEADER)) != 0) {
      ZeroMem (
          (UINT8 *) Image + sizeof (EFI_IMAGE_DOS_HEADER),
          ((EFI_IMAGE_DOS_HEADER *) Image)->e_lfanew - sizeof (EFI_IMAGE_DOS_HEADER)
          );
    }
  }

  //
  // Calculate real image size
  //
  DEBUG ((DEBUG_INFO, "OCAV: Real image size: %lu\n", *RealImageSize));

  *RealImageSize = Context->SecDir->VirtualAddress
                   + Context->SecDir->Size
                   + sizeof (APPLE_EFI_CERTIFICATE);

  if (*RealImageSize < ImageSize) {
    DEBUG ((DEBUG_WARN, "OCAV: Droping tail with size: %lu\n", ImageSize - *RealImageSize));
    //
    // Drop tail
    //
    ZeroMem (
      (UINT8 *) Image + *RealImageSize,
      ImageSize - *RealImageSize
      );
  }
}

EFI_STATUS
GetApplePeImageSha256 (
  VOID                                *Image,
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  )
{
  UINTN                    HashSize           = 0;
  UINT8                    *HashBase          = NULL;
  SHA256_CONTEXT           HashContext;

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
}

EFI_STATUS
VerifyApplePeImageSignature (
  IN OUT VOID                                *PeImage,
  IN OUT UINTN                               *ImageSize
  )
{
  UINTN                              Index             = 0;
  APPLE_SIGNATURE_CONTEXT            *SignatureContext = NULL;
  OC_RSA_PUBLIC_KEY                  *Pk               = NULL;
  APPLE_PE_COFF_LOADER_IMAGE_CONTEXT *Context          = NULL;

  Context = AllocateZeroPool (sizeof (APPLE_PE_COFF_LOADER_IMAGE_CONTEXT));
  if (Context == NULL) {
    DEBUG ((DEBUG_WARN, "OCAV: Pe context allocation failure\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Build PE context
  //
  if (EFI_ERROR (BuildPeContext (PeImage, *ImageSize, Context))) {
    DEBUG ((DEBUG_WARN, "OCAV: Malformed ApplePeImage\n"));
    FreePool (Context);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Sanitzie ApplePeImage
  //
  SanitizeApplePeImage (PeImage, ImageSize, Context);

  //
  // Allocate signature context
  //
  SignatureContext = AllocateZeroPool (sizeof (APPLE_SIGNATURE_CONTEXT));
  if (SignatureContext == NULL) {
    DEBUG ((DEBUG_WARN, "OCAV: Signature context allocation failure\n"));
    FreePool (Context);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Extract AppleSignature from PEImage
  //
  if (EFI_ERROR (GetApplePeImageSignature (PeImage, *ImageSize, Context, SignatureContext))) {
    DEBUG ((DEBUG_WARN, "OCAV: AppleSignature broken or not present!\n"));
    FreePool (SignatureContext);
    FreePool (Context);
    return EFI_UNSUPPORTED;
  }

  //
  // Calcucate PeImage hash
  //
  if (EFI_ERROR (GetApplePeImageSha256 (PeImage, Context))) {
    DEBUG ((DEBUG_WARN, "OCAV: Couldn't calcuate hash of PeImage\n"));
    FreePool (SignatureContext);
    FreePool (Context);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Verify existence in DataBase
  //
  for (Index = 0; Index < NUM_OF_PK; Index++) {
    if (CompareMem (PkDataBase[Index].Hash, SignatureContext->PublicKeyHash, 32) == 0) {
      //
      // PublicKey valid. Extract prepared publickey from database
      //
      Pk = (OC_RSA_PUBLIC_KEY *) PkDataBase[Index].PublicKey;
    }
  }

  if (Pk == NULL) {
    DEBUG ((DEBUG_WARN, "OCAV: Unknown publickey or malformed certificate\n"));
    FreePool (SignatureContext);
    FreePool (Context);
    return EFI_UNSUPPORTED;
  }

  //
  // Verify signature
  //
  if (RsaVerifySigHashFromKey (Pk, SignatureContext->Signature, sizeof (SignatureContext->Signature), Context->PeImageHash, sizeof (Context->PeImageHash), OcSigHashTypeSha256) == 1 ) {
    DEBUG ((DEBUG_INFO, "OCAV: Signature verified!\n"));
    FreePool (SignatureContext);
    FreePool (Context);
    return EFI_SUCCESS;
  }

  FreePool (SignatureContext);
  FreePool (Context);

  return EFI_SECURITY_VIOLATION;
}
