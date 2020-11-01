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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKeysLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcAppleImg4Lib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "libDER/oids.h"
#include "libDERImg4/libDERImg4.h"
#include "libDERImg4/Img4oids.h"

GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 *DERImg4RootCertificate     = gAppleX86SecureBootRootCaCert;
GLOBAL_REMOVE_IF_UNREFERENCED const UINTN *DERImg4RootCertificateSize = &gAppleX86SecureBootRootCaCertSize;

typedef struct OC_SB_MODEL_DESC_ {
  CONST CHAR8  *HardwareModel;
  UINT32       BoardId;
} OC_SB_MODEL_DESC;

STATIC CHAR8 mCryptoDigestMethod[16] = "sha2-384";
STATIC DERImg4Environment mEnvInfo;
STATIC CONST CHAR8 *mModelDefault = "j137";
///
/// List of model mapping to board identifiers.
/// Alphabetically sorted (!), for release order refer to the documentation.
///
STATIC OC_SB_MODEL_DESC mModelInformation[] = {
  { "j132",      0x0C }, ///< MacBookPro15,2
  { "j137",      0x0A }, ///< iMacPro1,1
  { "j140a",     0x37 }, ///< MacBookAir8,2
  { "j140k",     0x17 }, ///< MacBookAir8,1
  { "j152f",     0x3A }, ///< MacBookPro16,1
  { "j160",      0x0F }, ///< MacPro7,1
  { "j174",      0x0E }, ///< Macmini8,1
  { "j185",      0x22 }, ///< iMac20,1
  { "j185f",     0x23 }, ///< iMac20,2
  { "j213",      0x18 }, ///< MacBookPro15,4
  { "j214k",     0x3E }, ///< MacBookPro16,2
  { "j215",      0x38 }, ///< MacBookPro16,4
  { "j223",      0x3B }, ///< MacBookPro16,3
  { "j230k",     0x3F }, ///< MacBookAir9,1
  { "j680",      0x0B }, ///< MacBookPro15,1
  { "j780",      0x07 }, ///< MacBookPro15,3
  { "x86legacy", 0xF0 }, ///< Generic x86 from Big Sur
};

STATIC BOOLEAN mHasDigestOverride;
STATIC UINT8   mOriginalDigest[SHA384_DIGEST_SIZE];
STATIC UINT8   mOverrideDigest[SHA384_DIGEST_SIZE];

STATIC
OC_SB_MODEL_DESC *
InternalGetModelInfo (
  IN CONST CHAR8  *Model
  )
{
  UINTN  Start;
  UINTN  End;
  UINTN  Curr;
  INTN   Cmp;

  //
  // Classic binary search in a sorted string list.
  //
  Start = 0;
  End   = ARRAY_SIZE (mModelInformation) - 1;

  while (Start <= End) {
    Curr = (Start + End) / 2;
    Cmp = AsciiStrCmp (mModelInformation[Curr].HardwareModel, Model);

    if (Cmp == 0) {
      return &mModelInformation[Curr];
    } else if (Cmp < 0) {
      Start = Curr + 1;
    } else if (Curr > 0) {
      End = Curr - 1;
    } else {
      //
      // Even the first element does not match, required due to unsigned End.
      //
      return NULL;
    }
  }

  return NULL;
}

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

CONST CHAR8 *
OcAppleImg4GetHardwareModel (
  IN CONST CHAR8    *ModelRequest
  )
{
  OC_SB_MODEL_DESC  *ModelInfo;

  if (AsciiStrCmp (ModelRequest, OC_SB_MODEL_DEFAULT) == 0 || ModelRequest[0] == '\0') {
    ModelRequest = mModelDefault;
  }

  ModelInfo = InternalGetModelInfo (ModelRequest);
  if (ModelInfo != NULL) {
    return ModelInfo->HardwareModel;
  }

  return NULL;
}

EFI_STATUS
EFIAPI
OcAppleImg4Verify (
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
  UINT8               Digest[SHA384_DIGEST_SIZE];

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
    DEBUG ((
      DEBUG_INFO,
      "OCI4: Manifest (%u) for %08X parse fail with code %d\n",
      ManifestSize,
      ObjType,
      DerResult
      ));
    return EFI_SECURITY_VIOLATION;
  }

  CmpResult = -1;

  DEBUG ((
    DEBUG_INFO,
    "OCI4: Verifying digest %u (%02X%02X%02X%02X) override %d %u (%02X%02X%02X%02X)\n",
    ManInfo.imageDigestSize,
    ManInfo.imageDigest[0],
    ManInfo.imageDigest[1],
    ManInfo.imageDigest[2],
    ManInfo.imageDigest[3],
    mHasDigestOverride,
    SHA384_DIGEST_SIZE,
    mOriginalDigest[0],
    mOriginalDigest[1],
    mOriginalDigest[2],
    mOriginalDigest[3]
    ));

  //
  // Provide a route to accept our modified kernel as long as we can trust it is really it.
  //
  if (mHasDigestOverride
    && ManInfo.imageDigestSize == SHA384_DIGEST_SIZE
    && CompareMem (mOriginalDigest, ManInfo.imageDigest, sizeof (mOriginalDigest)) == 0) {
    Sha384 (Digest, ImageBuffer, ImageSize);
    CmpResult = CompareMem (Digest, mOverrideDigest, sizeof (mOverrideDigest));
    DEBUG ((
      DEBUG_INFO,
      "OCI4: Matching override %02X%02X%02X%02X with %02X%02X%02X%02X - %a\n",
      mOverrideDigest[0],
      mOverrideDigest[1],
      mOverrideDigest[2],
      mOverrideDigest[3],
      Digest[0],
      Digest[1],
      Digest[2],
      Digest[3],
      CmpResult == 0 ? "success" : "failure"
      ));
    if (CmpResult == 0) {
      mHasDigestOverride = FALSE;
    }
  }

  //
  // As ManInfo.imageDigest is a buffer of static size, the bounds check to
  // retrieve it acts as implicit sanitizing of ManInfo.imageDigestSize which
  // can be considered trusted at this point.
  //
  if (CmpResult != 0) {
    CmpResult = SigVerifyShaHashBySize (
      ImageBuffer,
      ImageSize,
      ManInfo.imageDigest,
      ManInfo.imageDigestSize
      );
  }
  if (CmpResult != 0) {
    return EFI_SECURITY_VIOLATION;
  }

  if (SbMode == AppleImg4SbModeMedium) {
    if (ManInfo.hasEcid
     || !ManInfo.hasXugs || ManInfo.environment.xugs != mEnvInfo.xugs
     || (ManInfo.environment.internalUseOnlyUnit
      && ManInfo.environment.internalUseOnlyUnit != mEnvInfo.internalUseOnlyUnit)) {
      return EFI_SECURITY_VIOLATION;
    }
  } else if (SbMode == AppleImg4SbModeFull) {
    if (!ManInfo.hasEcid || ManInfo.environment.ecid != mEnvInfo.ecid
     || ManInfo.hasXugs
     || ManInfo.environment.internalUseOnlyUnit) {
      return EFI_SECURITY_VIOLATION;
    }
  }

  if ((ManInfo.environment.boardId                    != mEnvInfo.boardId)
   || (ManInfo.environment.chipId                     != mEnvInfo.chipId)
   || (ManInfo.environment.certificateEpoch            < mEnvInfo.certificateEpoch)
   || (ManInfo.environment.productionStatus           != mEnvInfo.productionStatus && !ManInfo.environment.productionStatus)
   || (ManInfo.environment.securityMode               != mEnvInfo.securityMode && !ManInfo.environment.securityMode)
   || (ManInfo.environment.securityDomain             != mEnvInfo.securityDomain)
   || (ManInfo.hasEffectiveProductionStatus
    && (ManInfo.environment.effectiveProductionStatus != mEnvInfo.effectiveProductionStatus && !ManInfo.environment.effectiveProductionStatus))
   || (ManInfo.hasEffectiveSecurityMode
    && (ManInfo.environment.effectiveSecurityMode     != mEnvInfo.effectiveSecurityMode && !ManInfo.environment.effectiveSecurityMode))
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

VOID
OcAppleImg4RegisterOverride (
  IN CONST UINT8  *OriginalDigest,
  IN CONST UINT8  *Image,
  IN UINT32       ImageSize
  )
{
  ASSERT (OriginalDigest != NULL);
  ASSERT (Image != NULL);
  ASSERT (ImageSize > 0);

  mHasDigestOverride = TRUE;
  CopyMem (mOriginalDigest, OriginalDigest, sizeof (mOriginalDigest));
  Sha384 (mOverrideDigest, Image, ImageSize);
}

EFI_STATUS
OcAppleImg4BootstrapValues (
  IN CONST CHAR8   *Model,
  IN UINT64        Ecid  OPTIONAL
  )
{
  EFI_STATUS        Status;
  OC_SB_MODEL_DESC  *SbInfo;

  ASSERT (Model != NULL);
  SbInfo = InternalGetModelInfo (Model);
  ASSERT (SbInfo != NULL); ///< Checked by calling OcAppleImg4GetHardwareModel.

  //
  // Predefine most of the values as they are common for all the models.
  //
  mEnvInfo.ecid                       = Ecid;
  mEnvInfo.boardId                    = SbInfo->BoardId;
  mEnvInfo.chipId                     = 0x8012;
  mEnvInfo.certificateEpoch           = 2;
  mEnvInfo.securityDomain             = 1;
  mEnvInfo.productionStatus           = TRUE;
  mEnvInfo.securityMode               = 1;
  mEnvInfo.effectiveProductionStatus  = TRUE;
  mEnvInfo.effectiveSecurityMode      = 1;
  mEnvInfo.internalUseOnlyUnit        = FALSE;
  mEnvInfo.xugs                       = 1;
  mEnvInfo.allowMixNMatch             = FALSE;

  //
  // Expose all the variables via NVRAM.
  //
  Status = gRT->SetVariable (
    L"ApECID",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.ecid),
    &mEnvInfo.ecid
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"ApChipID",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.chipId),
    &mEnvInfo.chipId
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"CertificateEpoch",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.certificateEpoch),
    &mEnvInfo.certificateEpoch
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"ApBoardID",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.boardId),
    &mEnvInfo.boardId
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"ApSecurityDomain",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.securityDomain),
    &mEnvInfo.securityDomain
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"ApProductionStatus",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.productionStatus),
    &mEnvInfo.productionStatus
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"ApSecurityMode",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.securityMode),
    &mEnvInfo.securityMode
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"EffectiveProductionStatus",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.effectiveProductionStatus),
    &mEnvInfo.effectiveProductionStatus
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"EffectiveSecurityMode",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.effectiveSecurityMode),
    &mEnvInfo.effectiveSecurityMode
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"InternalUseOnlyUnit",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.internalUseOnlyUnit),
    &mEnvInfo.internalUseOnlyUnit
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"MixNMatchPreventionStatus",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mEnvInfo.allowMixNMatch),
    &mEnvInfo.allowMixNMatch
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"CryptoDigestMethod",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    sizeof (mCryptoDigestMethod),
    &mCryptoDigestMethod
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  

  return EFI_SUCCESS;
}

APPLE_IMG4_VERIFICATION_PROTOCOL *
OcAppleImg4VerificationInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  STATIC APPLE_IMG4_VERIFICATION_PROTOCOL Img4Verification = {
    APPLE_IMG4_VERIFICATION_PROTOCOL_REVISION,
    OcAppleImg4Verify
  };

  EFI_STATUS                       Status;
  APPLE_IMG4_VERIFICATION_PROTOCOL *Protocol;
  EFI_HANDLE                       Handle;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleImg4VerificationProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCI4: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleImg4VerificationProtocolGuid,
      NULL,
      (VOID **)&Protocol
      );
    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &Handle,
    &gAppleImg4VerificationProtocolGuid,
    (VOID **)&Img4Verification,
    NULL
    );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &Img4Verification;
}
