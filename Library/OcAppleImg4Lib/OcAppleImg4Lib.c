/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Guid/AppleVariable.h>

#include <Protocol/AppleImg4Verification.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKeysLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "libDER/oids.h"
#include "libDERImg4/libDERImg4.h"
#include "libDERImg4/Img4oids.h"

GLOBAL_REMOVE_IF_UNREFERENCED const uint8_t *DERImg4RootCertificate     = gAppleX86SecureBootRootCaCert;
GLOBAL_REMOVE_IF_UNREFERENCED const size_t  *DERImg4RootCertificateSize = &gAppleX86SecureBootRootCaCertSize;

bool
DERImg4VerifySignature (
  DERByte        *Modulus,
  DERSize        ModulusSize,
  uint32_t       Exponent,
  const uint8_t  *Signature,
  size_t         SignatureSize,
  uint8_t        *Data,
  size_t         DataSize,
  const DERItem  *AlgoOid
  )
{
  OC_SIG_HASH_TYPE AlgoType;

  ASSERT (Modulus != NULL);
  ASSERT (ModulusSize > 0);
  ASSERT (Signature != NULL);
  ASSERT (SignatureSize > 0);
  ASSERT (Data != NULL);
  ASSERT (DataSize > 0);
  ASSERT (AlgoOid != NULL);

  if (DEROidCompare (AlgoOid, &oidSha512Rsa)) {
    AlgoType = OcSigHashTypeSha512;
  } else if (DEROidCompare (AlgoOid, &oidSha384Rsa)) {
    AlgoType = OcSigHashTypeSha384;
  } else if (DEROidCompare (AlgoOid, &oidSha256Rsa)) {
    AlgoType = OcSigHashTypeSha256;
  } else {
    return false;
  }

  return RsaVerifySigDataFromData (
           Modulus,
           ModulusSize,
           Exponent,
           Signature,
           SignatureSize,
           Data,
           DataSize,
           AlgoType
           );
}

VOID
InternalRetrieveHwInfo (
  OUT DERImg4Environment  *Environment
  )
{
  EFI_STATUS Status;
  UINTN      DataSize;
  //
  // FIXME: Retrieve these values from trusted storage and expose the variables.
  //
  ASSERT (Environment != NULL);

  ZeroMem (Environment, sizeof (*Environment));

  DataSize = sizeof (Environment->ecid);
  Status = gRT->GetVariable (
                  L"ApECID",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &Environment->ecid
                  );
  if (EFI_ERROR (Status) || DataSize != sizeof (Environment->ecid)) {
    Environment->ecid = 0;
  }

  DataSize = sizeof (Environment->chipId);
  Status = gRT->GetVariable (
                  L"ApChipID",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &Environment->chipId
                  );
  if (EFI_ERROR (Status) || DataSize != sizeof (Environment->chipId)) {
    Environment->chipId = 0;
  }

  DataSize = sizeof (Environment->boardId);
  Status = gRT->GetVariable (
                  L"ApBoardID",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &Environment->boardId
                  );
  if (EFI_ERROR (Status) || DataSize != sizeof (Environment->boardId)) {
    Environment->boardId = 0;
  }

  Environment->certificateEpoch = 2;

  DataSize = sizeof (Environment->securityDomain);
  Status = gRT->GetVariable (
                  L"ApSecurityDomain",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &Environment->securityDomain
                  );
  if (EFI_ERROR (Status) || DataSize != sizeof (Environment->securityDomain)) {
    Environment->securityDomain = 1;
  }

  DataSize = sizeof (Environment->productionStatus);
  Status = gRT->GetVariable (
                  L"ApProductionStatus",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &Environment->productionStatus
                  );
  if (EFI_ERROR (Status)
   || DataSize != sizeof (Environment->productionStatus)) {
    Environment->productionStatus = TRUE;
  }

  DataSize = sizeof (Environment->securityMode);
  Status = gRT->GetVariable (
                  L"ApSecurityMode",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &Environment->securityMode
                  );
  if (EFI_ERROR (Status) || DataSize != sizeof (Environment->securityMode)) {
    Environment->securityMode = TRUE;
  }

  DataSize = sizeof (Environment->effectiveProductionStatus);
  Status = gRT->GetVariable (
                  L"EffectiveProductionStatus",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &Environment->effectiveProductionStatus
                  );
  if (EFI_ERROR (Status)
   || DataSize != sizeof (Environment->effectiveProductionStatus)) {
    Environment->effectiveProductionStatus = TRUE;
  }

  DataSize = sizeof (Environment->effectiveSecurityMode);
  Status = gRT->GetVariable (
                  L"EffectiveSecurityMode",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &Environment->effectiveSecurityMode
                  );
  if (EFI_ERROR (Status)
   || DataSize != sizeof (Environment->effectiveSecurityMode)) {
    Environment->effectiveSecurityMode = TRUE;
  }

  DataSize = sizeof (Environment->internalUseOnlyUnit);
  Status = gRT->GetVariable (
                  L"InternalUseOnlyUnit",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &Environment->internalUseOnlyUnit
                  );
  if (EFI_ERROR (Status)
   || DataSize != sizeof (Environment->internalUseOnlyUnit)) {
    Environment->internalUseOnlyUnit = FALSE;
  }

  // CHANGE: HardwareModel is unused.
}

/**
  Verify the signature of ImageBuffer against Type of its IMG4 Manifest.

  @param[in]  This            The pointer to the current protocol instance.
  @param[in]  ObjType         The IMG4 object type to validate against.
  @param[in]  ImageBuffer     The buffer to validate.
  @param[in]  ImageSize       The size, in bytes, of ImageBuffer.
  @param[in]  SbMode          The requested IMG4 Secure Boot mode.
  @param[in]  ManifestBuffer  The buffer of the IMG4 Manifest.
  @param[in]  ManifestSize    The size, in bytes, of ManifestBuffer.
  @param[out] HashDigest      On output, a pointer to ImageBuffer's digest.
  @param[out] DigestSize      On output, the size, in bytes, of *HashDigest.

  @retval EFI_SUCCESS             ImageBuffer is correctly signed.
  @retval EFI_INVALID_PARAMETER   One or more required parameters are NULL.
  @retval EFI_OUT_OF_RESOURCES    Not enough resources are available.
  @retval EFI_SECURITY_VIOLATION  ImageBuffer's signature is invalid.

**/
EFI_STATUS
EFIAPI
AppleImg4Verify (
  IN  APPLE_IMG4_VERIFICATION_PROTOCOL  *This,
  IN  UINT32                            ObjType,
  IN  CONST VOID                        *ImageBuffer,
  IN  UINTN                             ImageSize,
  IN  UINT8                             SbMode,
  IN  CONST VOID                        *ManifestBuffer,
  IN  UINTN                             ManifestSize,
  OUT UINT8                             **HashDigest OPTIONAL,
  OUT UINTN                             *DigestSize OPTIONAL
  )
{
  DERReturn           DerResult;
  INTN                CmpResult;

  DERImg4Environment  EnvInfo;
  DERImg4ManifestInfo ManInfo;

  if ((ImageBuffer    == NULL || ImageSize    == 0)
   || (ManifestBuffer == NULL || ManifestSize == 0)
   || (HashDigest     == NULL && DigestSize   != NULL)
   || (HashDigest     != NULL && DigestSize   == NULL)
   || (SbMode         != AppleImg4SbModeMedium
    && SbMode         != AppleImg4SbModeFull)) {
    return EFI_INVALID_PARAMETER;
  }

  DerResult = DERImg4ParseManifest (
                &ManInfo,
                ManifestBuffer,
                ManifestSize,
                ObjType
                );
  if (DerResult != DR_Success) {
    return EFI_SECURITY_VIOLATION;
  }
  //
  // As ManInfo.imageDigest is a buffer of static size, the bounds check to
  // retrieve it acts as implicit sanitizing of ManInfo.imageDigestSize which
  // can be considered trusted at this point.
  //
  CmpResult = SigVerifyShaHashBySize (
                ImageBuffer,
                ImageSize,
                ManInfo.imageDigest,
                ManInfo.imageDigestSize
                );
  if (CmpResult != 0) {
    return EFI_SECURITY_VIOLATION;
  }

  InternalRetrieveHwInfo (&EnvInfo);

  if (SbMode == AppleImg4SbModeMedium) {
    if (ManInfo.hasEcid
     || !ManInfo.hasXugs || ManInfo.environment.xugs != EnvInfo.xugs
     || (ManInfo.environment.internalUseOnlyUnit
      && ManInfo.environment.internalUseOnlyUnit != EnvInfo.internalUseOnlyUnit)) {
      return EFI_SECURITY_VIOLATION;
    }
  } else if (SbMode == AppleImg4SbModeFull) {
    if (!ManInfo.hasEcid || ManInfo.environment.ecid != EnvInfo.ecid
     || ManInfo.hasXugs
     || ManInfo.environment.internalUseOnlyUnit) {
      return EFI_SECURITY_VIOLATION;
    }
  }

  if ((ManInfo.environment.boardId                    != EnvInfo.boardId)
   || (ManInfo.environment.chipId                     != EnvInfo.chipId)
   || (ManInfo.environment.certificateEpoch            < EnvInfo.certificateEpoch)
   || (ManInfo.environment.productionStatus           != EnvInfo.productionStatus && !ManInfo.environment.productionStatus)
   || (ManInfo.environment.securityMode               != EnvInfo.securityMode && !ManInfo.environment.securityMode)
   || (ManInfo.environment.securityDomain             != EnvInfo.securityDomain)
   || (ManInfo.hasEffectiveProductionStatus
    && (ManInfo.environment.effectiveProductionStatus != EnvInfo.effectiveProductionStatus && !ManInfo.environment.effectiveProductionStatus))
   || (ManInfo.hasEffectiveSecurityMode
    && (ManInfo.environment.effectiveSecurityMode     != EnvInfo.effectiveSecurityMode && !ManInfo.environment.effectiveSecurityMode))
    ) {
    return EFI_SECURITY_VIOLATION;
  }

  if (HashDigest != NULL) {
    *HashDigest = AllocateCopyPool (ManInfo.imageDigestSize, ManInfo.imageDigest);
    if (*HashDigest == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *DigestSize = ManInfo.imageDigestSize;
  }

  return EFI_SUCCESS;
}
