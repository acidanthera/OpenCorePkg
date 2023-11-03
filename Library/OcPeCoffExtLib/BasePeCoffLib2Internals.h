/** @file
  Provides shared private definitions across this library.

  Copyright (c) 2020 - 2021, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef BASE_PE_COFF_LIB2_INTERNALS_H_
#define BASE_PE_COFF_LIB2_INTERNALS_H_

//
// PcdImageLoaderRelocTypePolicy bits.
//

///
/// If set, ARM Thumb Image relocations are supported.
///
#define PCD_RELOC_TYPE_POLICY_ARM  BIT0

///
/// Denotes the alignment requirement for Image certificate sizes.
///
#define IMAGE_CERTIFICATE_ALIGN  8U

//
// The PE/COFF specification guarantees an 8 Byte alignment for certificate
// sizes. This is larger than the alignment requirement for WIN_CERTIFICATE
// implied by the UEFI ABI. ASSERT this holds.
//
STATIC_ASSERT (
  ALIGNOF (WIN_CERTIFICATE) <= IMAGE_CERTIFICATE_ALIGN,
  "The PE/COFF specification guarantee does not suffice."
  );

//
// The 4 Byte alignment guaranteed by the PE/COFF specification has been
// replaced with ALIGNOF (EFI_IMAGE_BASE_RELOCATION_BLOCK) for proof simplicity.
// This obviously was the original intention of the specification. ASSERT in
// case the equality is not given.
//
STATIC_ASSERT (
  sizeof (UINT32) == ALIGNOF (EFI_IMAGE_BASE_RELOCATION_BLOCK),
  "The current model violates the PE/COFF specification"
  );

// FIXME:
RETURN_STATUS
PeCoffLoadImageInplaceNoBase (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *Context
  );

#endif // BASE_PE_COFF_LIB_INTERNALS_H_
