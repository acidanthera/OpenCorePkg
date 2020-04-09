/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/OcCompressionLib.h>

UINT32
DecompressMaskedRLE24 (
  OUT UINT8   *Dst,
  IN  UINT32  DstLen,
  IN  UINT8   *Src,
  IN  UINT32  SrcLen,
  IN  UINT8   *Mask,
  IN  UINT32  MaskLen
  )
{
  //
  // Data is encoded in three runs one by one: red, green, blue.
  // The amount of pixels is equal to mask size, as it is 8-bit unencoded.
  //
  // Unlike normal RLE, where we process a <N><B><N><B> sequence (N standing
  // for repeat count byte, and B for value byte), this one is more similar
  // to LZ algorithms:
  //  1. <C> & BIT7 != 0 is a normal repeat with N = <C> - 125 bytes.
  //  2. <C> & BIT7 == 0 is a raw sequence of <C> + 1 bytes.
  // Despite this obviously being more a variant of an LZ algorithm, I follow
  // suit by calling it RLE24 just as others do.
  //

  UINT8  ControlValue;
  UINT8  DstValue;
  UINT8  *SrcEnd;
  UINT8  *SrcLast;
  UINT8  *DstCur;
  UINT8  *DstEnd;
  UINT8  RunIndex;

  if (SrcLen < 2 || MaskLen != DstLen / sizeof (UINT32)) {
    return 0;
  }

  SrcEnd  = Src + SrcLen;
  SrcLast = SrcEnd - 1;

  //
  // Run over all channels.
  //
  for (RunIndex = 0; RunIndex < 3 && Src < SrcLast; ++RunIndex) {
    //
    // The resulting image is in BGRA format.
    //
    DstCur = Dst + 2 - RunIndex;
    DstEnd = DstCur + MaskLen * sizeof (UINT32);

    //
    // Iterate as long as we have enough control values.
    //
    while (Src < SrcLast && DstCur < DstEnd) {
      //
      // Process repeat sequences.
      //
      ControlValue = *Src++;
      if (ControlValue & BIT7) {
        ControlValue -= 125;
        DstValue      = *Src++;

        //
        // Early exit on not enough bytes to fill.
        //
        if (DstEnd - DstCur < ControlValue * sizeof (UINT32)) {
          return 0;
        }

        while (ControlValue > 0) {
          *DstCur = DstValue;
          DstCur += sizeof (UINT32);
          --ControlValue;
        }
      } else {
        ++ControlValue;

        //
        // Early exit on not enough bytes to read or fill.
        //
        if (SrcEnd - Src < ControlValue || DstEnd - DstCur < ControlValue * sizeof (UINT32)) {
          return 0;
        }

        while (ControlValue > 0) {
          *DstCur = *Src++;
          DstCur += sizeof (UINT32);
          --ControlValue;
        }
      }
    }

    //
    // We failed fill the current run, abort.
    //
    if (DstCur != DstEnd) {
      return 0;
    }
  }

  //
  // Put mask to the resulting image.
  //
  DstCur = Dst + RunIndex;
  DstEnd = Dst + MaskLen * sizeof (UINT32);
  while (DstCur < DstEnd) {
    *DstCur = *Mask++;
    DstCur += sizeof (UINT32);
  }

  return MaskLen * sizeof (UINT32);
}
