/** @file
  Provides additional services for Apple PE/COFF image.

  Copyright (c) 2018, savvas. All rights reserved.<BR>
  Copyright (c) 2021, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OC_PE_COFF_EXT_LIB_H
#define OC_PE_COFF_EXT_LIB_H

#include <IndustryStandard/Apfs.h>
#include <Library/OcPeCoffLib.h>

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
  IN OUT VOID                                *PeImage,
  IN OUT UINT32                              *ImageSize
  );

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

#endif // OC_PE_COFF_EXT_LIB_H
