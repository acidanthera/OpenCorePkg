/** @file
  Provides additional services for Apple PE/COFF image.

  Copyright (c) 2018, savvas. All rights reserved.<BR>
  Copyright (c) 2021, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OC_PE_COFF_EXT_LIB_H
#define OC_PE_COFF_EXT_LIB_H

#include <IndustryStandard/Apfs.h>
#include <Library/PeCoffLib2.h>

/**
  Verify Apple COFF legacy signature.
  Image buffer is sanitized where necessary (zeroed),
  and an updated length is returned through size parameter.

  @param[in,out]  PeImage    Image buffer.
  @param[in,out]  ImageSize  Size of the image.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
PeCoffVerifyAppleSignature (
  IN OUT VOID    *PeImage,
  IN OUT UINT32  *ImageSize
  );

#ifdef EFIUSER

/**
  Verify Apple COFF legacy signature with pre-initialized image context.
  Image buffer referenced via context is sanitized where necessary (zeroed),
  and an updated length is returned through size parameter.

  @param[in,out]  ImageContext    Image context.
  @param[in,out]  ImageSize       Size of the image.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
InternalPeCoffVerifyAppleSignatureFromContext (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN OUT  UINT32                       *ImageSize
  );

#endif

/**
  Obtain APFS driver version.

  @param[in]  DriverBuffer      Image buffer.
  @param[in]  DriverSize        Size of the image.
  @param[out] DriverVersionPtr  Driver version within image buffer.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
PeCoffGetApfsDriverVersion (
  IN  VOID                 *DriverBuffer,
  IN  UINT32               DriverSize,
  OUT APFS_DRIVER_VERSION  **DriverVersionPtr
  );

#ifdef EFIUSER

/**
  Obtain APFS driver version from pre-initialized image context.

  @param[in]  ImageContext      Image context.
  @param[in]  DriverSize        Size of the image.
  @param[out] DriverVersionPtr  Driver version within image buffer.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
InternalPeCoffGetApfsDriverVersionFromContext (
  IN  PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN  UINT32                        DriverSize,
  OUT APFS_DRIVER_VERSION           **DriverVersionPtr
  );

#endif

/**
  Detect and patch W^X and overlapping section errors in legacy boot.efi.
  Expected to fix overlapping sections in 10.4 and 10.5 32-bit only, and
  W^X errors in most macOS binaries.

  @param[in]  DriverBuffer      Image buffer.
  @param[in]  DriverSize        Size of the image.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcPatchLegacyEfi (
  IN  VOID    *DriverBuffer,
  IN  UINT32  DriverSize
  );

/**
  Fix W^X and section overlap issues in loaded TE, PE32, or PE32+ Image in
  memory while initialising Context.

  Closely based on PeCoffInitializeContext from PeCoffLib2.

  The approach of modifying the image in memory is basically incompatible
  with secure boot, although:
    a) Certain firmware may allow optionally registering the hash of any
       image which does not load, which would still work.
    b) It is fairly crazy anyway to want to apply secure boot to the old,
       insecure .efi files which need these fixups.

  @param[out] Context        The context describing the Image.
  @param[in]  FileBuffer     The file data to parse as PE Image.
  @param[in]  FileSize       The size, in Bytes, of FileBuffer.
  @param[in]  InMemoryFixup  If TRUE, fixes are made to image in memory.
                             If FALSE, Context is initialised as if fixes were
                             made, but no changes are made to loaded image.

  @retval RETURN_SUCCESS  The Image context has been initialised successfully.
  @retval other           The file data is malformed.
**/
RETURN_STATUS
OcPeCoffFixupInitializeContext (
  OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN  CONST VOID                    *FileBuffer,
  IN  UINT32                        FileSize,
  IN  BOOLEAN                       InMemoryFixup
  );

#endif // OC_PE_COFF_EXT_LIB_H
