/** @file
  Copyright (C) 2019, Download-Fritz. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef OC_APPLE_IMG4_LIB_H
#define OC_APPLE_IMG4_LIB_H

#include <Library/OcCryptoLib.h>
#include <Protocol/AppleImg4Verification.h>

/**
  Chooses the default model (most recent broadly supported).
**/
#define OC_SB_MODEL_DEFAULT "Default"

/**
  No secure boot mode special value.
**/
#define OC_SB_MODEL_DISABLED "Disabled"

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
  );

/**
  Register digest override with SHA-384 hash. This allows to replace
  one image with another.

  @param[in] OriginalDigest   Original SHA-384 digest.
  @param[in] Image            Pointer to new image.
  @param[in] ImageSize        Image size.
**/
VOID
OcAppleImg4RegisterOverride (
  IN CONST UINT8  *OriginalDigest,
  IN CONST UINT8  *Image,
  IN UINT32       ImageSize
  );

/**
  Obtain hardware model for secure booting from the model request.

  @param[in]  ModelRequest  Raw model or configuration strings like
                            Latest or Disabled.

  @retval Model in lower case on success.
  @retval NULL  on failure
**/
CONST CHAR8 *
OcAppleImg4GetHardwareModel (
  IN CONST CHAR8    *ModelRequest
  );

/**
  Bootstrap NVRAM and library values for secure booting.

  @param[in] Model          Secure boot model (without ap suffix in lower-case).
  @param[in] Ecid           Secure boot ECID identifier for this model, optional.

  @returns Installed or located protocol.
  @retval NULL  There was an error locating or installing the protocol.

**/
EFI_STATUS
OcAppleImg4BootstrapValues (
  IN CONST CHAR8   *Model,
  IN UINT64        Ecid  OPTIONAL
  );

/**
  Install and initialise the Apple IMG4 verification protocol.

  @param[in] Reinstall          Replace any installed protocol.

  @returns Installed or located protocol.
  @retval NULL  There was an error locating or installing the protocol.

**/
APPLE_IMG4_VERIFICATION_PROTOCOL *
OcAppleImg4VerificationInstallProtocol (
  IN BOOLEAN  Reinstall
  );

#endif // OC_APPLE_IMG4_LIB_H
