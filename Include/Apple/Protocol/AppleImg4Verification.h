/** @file
  The Apple IMG4 verification provides services to verify trusted IMG4 images's
  integrity.

Copyright (C) 2019, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_IMG4_VERIFICATION_H
#define APPLE_IMG4_VERIFICATION_H

#define APPLE_IMG4_VERIFICATION_PROTOCOL_REVISION  2U

#define APPLE_IMG4_VERIFICATION_PROTOCOL_GUID  \
  { 0x314735F0, 0x26FE, 0x11E8,                \
    { 0xA4, 0x70, 0xB8, 0xE8, 0x56, 0x2C, 0xBA, 0xFA } }

typedef struct APPLE_IMG4_VERIFICATION_PROTOCOL_ APPLE_IMG4_VERIFICATION_PROTOCOL;

enum {
  AppleImg4SbModeDisabled = 0,
  AppleImg4SbModeMedium   = 1,
  AppleImg4SbModeFull     = 2
};

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
typedef
EFI_STATUS
(EFIAPI *APPLE_IMG4_VERIFY)(
  IN  APPLE_IMG4_VERIFICATION_PROTOCOL  *This,
  IN  UINT32                            ObjType,
  IN  CONST VOID                        *ImageBuffer,
  IN  UINTN                             ImageSize,
  IN  UINT8                             SbMode,
  IN  CONST VOID                        *ManifestBuffer,
  IN  UINTN                             ManifestSize,
  OUT UINT8                             **HashDigest OPTIONAL,
  OUT UINTN                             *DigestSize OPTIONAL
  );

struct APPLE_IMG4_VERIFICATION_PROTOCOL_ {
  UINTN             Revision;
  APPLE_IMG4_VERIFY Verify;
};

extern EFI_GUID gAppleImg4VerificationProtocolGuid;

#endif // APPLE_IMG4_VERIFICATION_H
