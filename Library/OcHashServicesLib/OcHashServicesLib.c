/**
 * Hash service fix for AMI EFIs with broken SHA implementations.
 *
 * Forcibly reinstalls EFI_HASH_PROTOCOL with working MD5, SHA-1,
 * SHA-256 implementations.
 *
 * Author: Joel Höner <athre0z@zyantific.com>
 */

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Hash.h>
#include <Protocol/ServiceBinding.h>

#include "OcHashServicesLibInternal.h"

STATIC EFI_SERVICE_BINDING_PROTOCOL mHashBindingProto = {
  &HSCreateChild,
  &HSDestroyChild
};

STATIC
EFI_STATUS
EFIAPI
HSGetHashSize (
  IN  CONST EFI_HASH_PROTOCOL  *This,
  IN  CONST EFI_GUID           *HashAlgorithm,
  OUT UINTN                    *HashSize
  )
{
  if (!HashAlgorithm || !HashSize) {
    return EFI_INVALID_PARAMETER;
  }

  if (CompareGuid (&gEfiHashAlgorithmMD5Guid, HashAlgorithm)) {
    *HashSize = sizeof (EFI_MD5_HASH);
    return EFI_SUCCESS;
  } else if (CompareGuid (&gEfiHashAlgorithmSha1Guid, HashAlgorithm)) {
    *HashSize = sizeof (EFI_SHA1_HASH);
    return EFI_SUCCESS;
  } else if (CompareGuid (&gEfiHashAlgorithmSha256Guid, HashAlgorithm)) {
    *HashSize = sizeof (EFI_SHA256_HASH);
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
HSHash (
  IN CONST EFI_HASH_PROTOCOL  *This,
  IN CONST EFI_GUID           *HashAlgorithm,
  IN BOOLEAN                  Extend,
  IN CONST UINT8              *Message,
  IN UINT64                   MessageSize,
  IN OUT EFI_HASH_OUTPUT      *Hash
  )
{
  HS_PRIVATE_DATA  *PrivateData;
  HS_CONTEXT_DATA  CtxCopy;

  if (!This || !HashAlgorithm || !Message || !Hash || !MessageSize || MessageSize > MAX_UINTN) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = HS_PRIVATE_FROM_PROTO (This);

  if (CompareGuid (&gEfiHashAlgorithmMD5Guid, HashAlgorithm)) {
    if (!Extend) {
      Md5Init (&PrivateData->Ctx.Md5);
    }

    Md5Update (&PrivateData->Ctx.Md5, Message, (UINTN)MessageSize);
    CopyMem (&CtxCopy, &PrivateData->Ctx, sizeof (PrivateData->Ctx));
    Md5Final (&CtxCopy.Md5, *Hash->Md5Hash);
    return EFI_SUCCESS;
  } else if (CompareGuid (&gEfiHashAlgorithmSha1Guid, HashAlgorithm)) {
    if (!Extend) {
      Sha1Init (&PrivateData->Ctx.Sha1);
    }

    Sha1Update (&PrivateData->Ctx.Sha1, Message, (UINTN)MessageSize);
    CopyMem (&CtxCopy, &PrivateData->Ctx, sizeof (PrivateData->Ctx));
    Sha1Final (&CtxCopy.Sha1, *Hash->Sha1Hash);
    return EFI_SUCCESS;
  } else if (CompareGuid (&gEfiHashAlgorithmSha256Guid, HashAlgorithm)) {
    if (!Extend) {
      Sha256Init (&PrivateData->Ctx.Sha256);
    }

    Sha256Update (&PrivateData->Ctx.Sha256, Message, (UINTN)MessageSize);
    CopyMem (&CtxCopy, &PrivateData->Ctx, sizeof (PrivateData->Ctx));
    Sha256Final (&CtxCopy.Sha256, *Hash->Sha256Hash);
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
HSCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  )
{
  HS_PRIVATE_DATA  *PrivateData;
  EFI_STATUS       Status;

  PrivateData = AllocateZeroPool (sizeof (*PrivateData));
  if (!PrivateData) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->Signature         = HS_PRIVATE_SIGNATURE;
  PrivateData->Proto.GetHashSize = HSGetHashSize;
  PrivateData->Proto.Hash        = HSHash;

  Status = gBS->InstallProtocolInterface (
    ChildHandle,
    &gEfiHashProtocolGuid,
    EFI_NATIVE_INTERFACE,
    &PrivateData->Proto
    );

  if (EFI_ERROR (Status)) {
    FreePool (PrivateData);
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
HSDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_HASH_PROTOCOL *HashProto;
  EFI_STATUS        Status;

  if (!This || !ChildHandle) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->HandleProtocol (
    ChildHandle,
    &gEfiHashProtocolGuid,
    (VOID **) &HashProto
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->UninstallProtocolInterface (
    ChildHandle,
    &gEfiHashProtocolGuid,
    HashProto
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  FreePool (HS_PRIVATE_FROM_PROTO (HashProto));
  return EFI_SUCCESS;
}

/**
  Install and initialise EFI Service Binding protocol.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed protocol.
  @retval NULL  There was an error installing the protocol.
**/
EFI_SERVICE_BINDING_PROTOCOL *
OcHashServicesInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    NewHandle        = NULL;
  EFI_SERVICE_BINDING_PROTOCOL  *OriginalProto   = NULL;

  if (Reinstall) {
    //
    // Uninstall all the existing protocol instances (which are not to be trusted).
    //
    Status = OcUninstallAllProtocolInstances (&gEfiHashServiceBindingProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCHS: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
                    &gEfiHashServiceBindingProtocolGuid,
                    NULL,
                    (VOID **) &OriginalProto);
    if (!EFI_ERROR (Status)) {
      return OriginalProto;
    }
  }

  //
  // Install our own protocol binding
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewHandle,
                  &gEfiHashServiceBindingProtocolGuid,
                  &mHashBindingProto,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mHashBindingProto;
}
