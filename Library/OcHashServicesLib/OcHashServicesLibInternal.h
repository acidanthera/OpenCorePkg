/**
 * Hash service fix for AMI EFIs with broken SHA implementations.
 *
 * Forcibly reinstalls EFI_HASH_PROTOCOL with working MD5, SHA-1,
 * SHA-256 implementations.
 *
 * Author: Joel Hoener <athre0z@zyantific.com>
 */

#ifndef HASH_SERVICES_H
#define HASH_SERVICES_H

#include <Library/OcCryptoLib.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Hash.h>

typedef union _HS_CONTEXT_DATA
{
  MD5_CONTEXT     Md5;
  SHA1_CONTEXT    Sha1;
  SHA256_CONTEXT  Sha256;
} HS_CONTEXT_DATA;

typedef struct _HS_PRIVATE_DATA
{
  HS_CONTEXT_DATA   Ctx;
  UINTN             Signature;
  EFI_HASH_PROTOCOL Proto;
} HS_PRIVATE_DATA;

#define HS_PRIVATE_SIGNATURE  SIGNATURE_32('H','S','r','v')

#define HS_PRIVATE_FROM_PROTO(a) \
  CR(a, HS_PRIVATE_DATA, Proto, HS_PRIVATE_SIGNATURE)

STATIC
EFI_STATUS
EFIAPI
HSGetHashSize (
  IN  CONST EFI_HASH_PROTOCOL *This,
  IN  CONST EFI_GUID          *HashAlgorithm,
  OUT UINTN                   *HashSize
  );

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
  );

STATIC
EFI_STATUS
EFIAPI
HSCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  );

STATIC
EFI_STATUS
EFIAPI
HSDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                   ChildHandle
  );

#endif // HASH_SERVICES_H
