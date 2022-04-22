#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <UserFile.h>

#include <Protocol/AppleSecureBoot.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/OcCryptoLib.h>

#include <libDER/oids.h>
#include <libDERImg4/Img4oids.h>
#include <libDERImg4/libDERImg4.h>

STATIC
VOID
InternalDebugEnvInfo (
  IN CONST  DERImg4Environment  *Env
  )
{
  DEBUG ((
    DEBUG_ERROR,
    "\nEcid: %Lx\n"
    "BoardId: %x\n"
    "ChipId: %x\n"
    "CertificateEpoch: %x\n"
    "SecurityDomain: %x\n"
    "ProductionStatus: %x\n"
    "SecurityMode: %x\n"
    "EffectiveProductionStatus: %x\n"
    "EffectiveSecurityMode: %x\n"
    "InternalUseOnlyUnit: %x\n"
    "Xugs: %x\n\n",
    (UINT64) Env->ecid,
    Env->boardId,
    Env->chipId,
    Env->certificateEpoch,
    Env->securityDomain,
    Env->productionStatus,
    Env->securityMode,
    Env->effectiveProductionStatus,
    Env->effectiveSecurityMode,
    Env->internalUseOnlyUnit,
    Env->xugs
    ));
}

STATIC
INT32
DebugManifest (
  IN CONST CHAR8  *ManifestName
  )
{
  //
  // The keys are iterated in the order in which they are defined here in
  // AppleBds to validate any loaded image.
  //
  STATIC CONST UINT32  Objs[] = {
    APPLE_SB_OBJ_EFIBOOT,
    APPLE_SB_OBJ_EFIBOOT_DEBUG,
    APPLE_SB_OBJ_EFIBOOT_BASE,
    APPLE_SB_OBJ_MUPD,
    APPLE_SB_OBJ_HPMU,
    APPLE_SB_OBJ_THOU,
    APPLE_SB_OBJ_GPUU,
    APPLE_SB_OBJ_ETHU,
    APPLE_SB_OBJ_SDFU,
    APPLE_SB_OBJ_DTHU,
    APPLE_SB_OBJ_KERNEL,
    APPLE_SB_OBJ_KERNEL_DEBUG,
  };

  UINT8                *Manifest;
  UINT32               ManSize;
  DERImg4ManifestInfo  ManInfo;
  UINT32               Success;
  UINT32               Index;
  DERReturn            RetVal;

  Manifest = UserReadFile (ManifestName, &ManSize);
  if (Manifest == NULL) {
    DEBUG ((DEBUG_ERROR, "\n!!! read error !!!\n"));
    return -1;
  }

  Success = 0;
  for (Index = 0; Index < ARRAY_SIZE (Objs); ++Index) {
    RetVal = DERImg4ParseManifest (
               &ManInfo,
               Manifest,
               ManSize,
               Objs[Index]
               );

    if (RetVal == DR_Success) {
      DEBUG ((
        DEBUG_ERROR,
        "Manifest has %c%c%c%c\n",
        ((CHAR8 *) &Objs[Index])[3],
        ((CHAR8 *) &Objs[Index])[2],
        ((CHAR8 *) &Objs[Index])[1],
        ((CHAR8 *) &Objs[Index])[0]
        ));
      InternalDebugEnvInfo (&ManInfo.environment);
      ++Success;
    }
  }

  FreePool (Manifest);

  if (Success == 0) {
    DEBUG ((DEBUG_ERROR, "Supplied manifest is not valid or has no known objects!\n"));
    return -1;
  }

  return 0;
}

STATIC
INT32
VerifyImg4 (
  IN CONST CHAR8  *ImageName,
  IN CONST CHAR8  *ManifestName,
  IN CONST CHAR8  *type
  )
{
  UINT8                *Manifest;
  UINT8                *Image;
  UINT32               ManSize;
  UINT32               ImgSize;
  DERImg4ManifestInfo  ManInfo;
  DERReturn            RetVal;
  INTN                 CmpResult;

  Manifest = UserReadFile (ManifestName, &ManSize);
  if (Manifest == NULL) {
    DEBUG ((DEBUG_ERROR, "\n!!! read error !!!\n"));
    return -1;
  }

  RetVal = DERImg4ParseManifest (
             &ManInfo,
             Manifest,
             ManSize,
             SIGNATURE_32 (type[3], type[2], type[1], type[0])
             );
  FreePool (Manifest);
  if (RetVal != DR_Success) {
    DEBUG ((DEBUG_ERROR, "\n !!! DERImg4ParseManifest failed - %d !!!\n", RetVal));
    return -1;
  }

  InternalDebugEnvInfo (&ManInfo.environment);

  Image = UserReadFile (ImageName, &ImgSize);
  if (Image == NULL) {
    DEBUG ((DEBUG_ERROR, "\n!!! read error !!!\n"));
    return -1;
  }

  DEBUG ((
    DEBUG_ERROR,
    "ManInfo.imageDigestSize %02X%02X%02X%02X %u\n",
    ManInfo.imageDigest[0],
    ManInfo.imageDigest[1],
    ManInfo.imageDigest[2],
    ManInfo.imageDigest[3],
    ManInfo.imageDigestSize
    ));

  CmpResult = SigVerifyShaHashBySize (
                Image,
                ImgSize,
                ManInfo.imageDigest,
                ManInfo.imageDigestSize
                );

  FreePool (Image);

  if (CmpResult != 0) {
    DEBUG ((DEBUG_ERROR, "\n!!! digest mismatch !!!\n"));
    return -1;
  }

  return 0;
}

int
ENTRY_POINT (
  int   argc,
  char  *argv[]
  )
{
  INT32  RetVal;
  INT32  Index;

  if (argc < 2 || ((argc % 3) != 1 && argc != 2)) {
    DEBUG ((DEBUG_ERROR, "Usage: ./Img4 ([image path] [manifest path] [object type])*\n"));
    DEBUG ((DEBUG_ERROR, "Usage: Img4 [manifest path]\n"));
    return -1;
  }

  if (argc == 2) {
    return DebugManifest (argv[1]);
  }

  RetVal = 0;
  for (Index = 1; Index < (argc - 1); Index += 3) {
    if (AsciiStrLen (argv[Index + 2]) != 4) {
      DEBUG ((DEBUG_ERROR, "Object types require exactly 4 characters.\n"));
      return -1;
    }

    RetVal = VerifyImg4 (
               argv[Index + 0],
               argv[Index + 1],
               argv[Index + 2]
               );
    if (RetVal != 0) {
      return RetVal;
    }
  }

  return RetVal;
}

int
LLVMFuzzerTestOneInput (
  const uint8_t  *Data,
  size_t         Size
  )
{
  STATIC CONST UINT32  Signatures[] = {
    APPLE_SB_OBJ_EFIBOOT,
    APPLE_SB_OBJ_EFIBOOT_DEBUG,
    APPLE_SB_OBJ_EFIBOOT_BASE,
    APPLE_SB_OBJ_MUPD,
    APPLE_SB_OBJ_HPMU,
    APPLE_SB_OBJ_THOU,
    APPLE_SB_OBJ_GPUU,
    APPLE_SB_OBJ_ETHU,
    APPLE_SB_OBJ_SDFU,
    APPLE_SB_OBJ_DTHU
  };

  DERImg4ManifestInfo  ManInfo;
  UINTN                Index;

  if (Data == NULL || Size == 0) {
    return 0;
  }

  for (Index = 0; Index < ARRAY_SIZE (Signatures); ++Index) {
    DERImg4ParseManifest (
      &ManInfo,
      Data,
      Size,
      Signatures[Index]
      );
  }

  return 0;
}
