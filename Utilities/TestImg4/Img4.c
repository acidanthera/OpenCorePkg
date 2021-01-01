#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <UserFile.h>

#include <Base.h>

#include <Protocol/AppleSecureBoot.h>
#include <Library/OcCryptoLib.h>

#include "libDER/oids.h"
#include "libDERImg4/Img4oids.h"
#include "libDERImg4/libDERImg4.h"

EFI_GUID gAppleSecureBootVariableGuid;

void InternalDebugEnvInfo (
  const DERImg4Environment  *Env
  )
{
  printf (
    "\nEcid: %llx\n"
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
    (unsigned long long)Env->ecid,
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
    );
}

int debugManifest (char *manifestName)
{
  void *Manifest;
  uint32_t ManSize;
  DERImg4ManifestInfo ManInfo;

  Manifest = UserReadFile (manifestName, &ManSize);
  if (Manifest == NULL) {
    printf ("\n!!! read error !!!\n");
    return -1;
  }

  //
  // The keys are iterated in the order in which they are defined here in
  // AppleBds to validate any loaded image.
  //
  uint32_t objs[] = {
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

  uint32_t success = 0;

  for (uint32_t i = 0; i < ARRAY_SIZE(objs); i++) {
    DERReturn r = DERImg4ParseManifest (
      &ManInfo,
      Manifest,
      ManSize,
      objs[i]
      );

    if (r == DR_Success) {
      printf ("Manifest has %c%c%c%c\n",
        ((char *)&objs[i])[3], ((char *)&objs[i])[2], ((char *)&objs[i])[1], ((char *)&objs[i])[0]);
      InternalDebugEnvInfo (&ManInfo.environment);
      ++success;
    }
  }

  free (Manifest);

  if (success == 0) {
    printf ("Supplied manifest is not valid or has no known objects!\n");
    return -1;
  }

  return 0;
}

int verifyImg4 (char *imageName, char *manifestName, char *type)
{
  void *Manifest, *Image;
  uint32_t ManSize, ImgSize;
  DERImg4ManifestInfo ManInfo;

  Manifest = UserReadFile (manifestName, &ManSize);
  if (Manifest == NULL) {
    printf ("\n!!! read error !!!\n");
    return -1;
  }

  DERReturn r = DERImg4ParseManifest (
                  &ManInfo,
                  Manifest,
                  ManSize,
                  SIGNATURE_32 (type[3], type[2], type[1], type[0])
                  );
  free (Manifest);
  if (r != DR_Success) {
    printf ("\n !!! DERImg4ParseManifest failed - %d !!!\n", r);
    return -1;
  }

  InternalDebugEnvInfo (&ManInfo.environment);

  Image = UserReadFile (imageName, &ImgSize);
  if (Image == NULL) {
    printf ("\n!!! read error !!!\n");
    return -1;
  }

  printf("ManInfo.imageDigestSize %02X%02X%02X%02X %zu\n",
    ManInfo.imageDigest[0], ManInfo.imageDigest[1],
    ManInfo.imageDigest[2], ManInfo.imageDigest[3],
    ManInfo.imageDigestSize);

  INTN CmpResult = SigVerifyShaHashBySize (
                     Image,
                     ImgSize,
                     ManInfo.imageDigest,
                     ManInfo.imageDigestSize
                     );

  free (Image);

  if (CmpResult != 0) {
    printf ("\n!!! digest mismatch !!!\n");
    return -1;
  }

  return 0;
}

int ENTRY_POINT (int argc, char *argv[])
{
  if (argc < 2 || ((argc % 3) != 1 && argc != 2)) {
    printf ("Img4 ([image path] [manifest path] [object type])*\n");
    printf ("Img4 [manifest path]\n");
    return -1;
  }

  if (argc == 2) {
    return debugManifest(argv[1]);
  }

  int r = 0;
  for (int i = 1; i < (argc - 1); i += 3) {
    if (strlen (argv[i + 2]) != 4) {
      printf ("Object types require exactly 4 characters.\n");
      return -1;
    }

    r = verifyImg4 (
          argv[i + 0],
          argv[i + 1],
          argv[i + 2]
          );
    if (r != 0) {
      return r;
    }
  }

  return r;
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  const uint32_t signatures[] = {
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

  if (Data == NULL || Size == 0) {
    return 0;
  }

  DERImg4ManifestInfo ManInfo;
  for (unsigned int i = 0; i < ARRAY_SIZE (signatures); ++i) {
    DERImg4ParseManifest (
      &ManInfo,
      Data,
      Size,
      signatures[i]
      );
  }

  return 0;
}
