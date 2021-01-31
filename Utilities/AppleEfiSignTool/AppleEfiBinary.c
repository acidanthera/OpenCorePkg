/** @file

AppleEfiSignTool – Tool for signing and verifying Apple EFI binaries.

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <string.h>
#include <stdlib.h>

#include "AppleEfiPeImage.h"
#include "AppleEfiFatBinary.h"
#include <Library/OcCryptoLib.h>
#include <Library/OcAppleKeysLib.h>

/**
  Read Apple's EFI Fat binary and gather
  position of each MZ image inside it and then
  perform ImageVerification of each MZ image
**/
int
VerifyAppleImageSignature (
  uint8_t  *Image,
  uint32_t ImageSize
  )
{
  EFIFatHeader *Hdr         = NULL;
  uint64_t     Index        = 0;
  uint64_t     SizeOfBinary = 0;

  if (ImageSize < sizeof (EFIFatHeader)) {
    DEBUG_PRINT (("Malformed binary\n"));
    return -1;
  }

  //
  // Get AppleEfiFatHeader
  //
  Hdr = (EFIFatHeader *) Image;
  //
  // Verify magic number
  //
  //
  if (Hdr->Magic != EFI_FAT_MAGIC) {
    DEBUG_PRINT (("Binary isn't EFIFat, verifying as single\n"));
    return VerifyApplePeImageSignature (Image, ImageSize);
  }
  DEBUG_PRINT (("It is AppleEfiFatBinary\n"));

  SizeOfBinary += sizeof (EFIFatHeader)
                  + sizeof (EFIFatArchHeader)
                    * Hdr->NumArchs;

  if (SizeOfBinary > ImageSize) {
    DEBUG_PRINT (("Malformed AppleEfiFat header\n"));
    return -1;
  }

  //
  // Loop over number of arch's
  //
  for (Index = 0; Index < Hdr->NumArchs; Index++) {
    //
    // Only X86/X86_64 valid
    //
    if (Hdr->Archs[Index].CpuType == CPU_TYPE_X86
        || Hdr->Archs[Index].CpuType == CPU_TYPE_X86_64) {
      DEBUG_PRINT (("ApplePeImage at offset %u\n", Hdr->Archs[Index].Offset));

      //
      // Check offset boundary and its size
      //
      if (Hdr->Archs[Index].Offset < SizeOfBinary
        || Hdr->Archs[Index].Offset >= ImageSize
        || ImageSize < ((uint64_t) Hdr->Archs[Index].Offset
                        + Hdr->Archs[Index].Size)) {
        DEBUG_PRINT(("Wrong offset of Image or it's size\n"));
        return -1;
      }

      //
      // Verify image with specified arch
      //
      if (VerifyApplePeImageSignature (Image + Hdr->Archs[Index].Offset,
                                       Hdr->Archs[Index].Size) != 0) {
        return -1;
      }
    }
    SizeOfBinary = (uint64_t) Hdr->Archs[Index].Offset + Hdr->Archs[Index].Size;
  }

  if (SizeOfBinary != ImageSize) {
    DEBUG_PRINT (("Malformed AppleEfiFatBinary\n"));
    return -1;
  }

  return 0;
}
