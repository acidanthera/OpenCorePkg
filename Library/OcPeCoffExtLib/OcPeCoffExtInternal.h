/** @file
  Provides additional services for Apple PE/COFF image.

  Copyright (c) 2018, savvas. All rights reserved.<BR>
  Copyright (c) 2021, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OC_PE_COFF_EXT_INTERNAL_H
#define OC_PE_COFF_EXT_INTERNAL_H

#include <Library/OcPeCoffExtLib.h>
#include <Library/OcCryptoLib.h>

STATIC_ASSERT (
  OFFSET_OF (EFI_IMAGE_NT_HEADERS64, CheckSum)
  == OFFSET_OF (EFI_IMAGE_NT_HEADERS32, CheckSum),
  "CheckSum is expected to be at the same place"
  );

#define APPLE_CHECKSUM_OFFSET  OFFSET_OF (EFI_IMAGE_NT_HEADERS64, CheckSum)
#define APPLE_CHECKSUM_SIZE    sizeof (UINT32)

//
// Signature context
//
typedef struct APPLE_SIGNATURE_CONTEXT_ {
  OC_RSA_PUBLIC_KEY    *PublicKey;
  UINT8                Signature[256];
} APPLE_SIGNATURE_CONTEXT;

/**
  Fix W^X and section overlap issues in loaded TE, PE32, or PE32+ Image in
  memory while initialising Context.

  Closely based on PeCoffInitializeContext from PeCoffLib2.

  The approach of modifying the image in memory is basically incompatible
  with secure boot, athough:
    a) Certain firmware may allow optionally registering the hash of any
       image which does not load, which would still work.
    b) It is fairly crazy anyway to want to apply secure boot to the old,
       insecure .efi files which need these fixups.

  @param[out] Context     The context describing the Image.
  @param[in]  FileBuffer  The file data to parse as PE Image.
  @param[in]  FileSize    The size, in Bytes, of FileBuffer.

  @retval RETURN_SUCCESS  The Image context has been initialised successfully.
  @retval other           The file data is malformed.
**/
RETURN_STATUS
InternalPeCoffFixup (
  OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context,
  IN  CONST VOID                    *FileBuffer,
  IN  UINT32                        FileSize
  );

#endif // OC_PE_COFF_EXT_INTERNAL_H
