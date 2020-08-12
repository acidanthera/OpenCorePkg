/** @file
  Mkext support.

  Copyright (c) 2020, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <IndustryStandard/AppleFatBinaryImage.h>


#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcCompressionLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>

#define MKEXT_OFFSET_STR_LEN    24

//
// Alignment to 8 bytes.
//
#define MKEXT_ALIGN(a)         (ALIGN_VALUE (a, sizeof (UINT64)))

STATIC
BOOLEAN
ParseKextBinary (
  IN OUT UINT8         **Buffer,
  IN OUT UINT32        *BufferSize,
  IN     BOOLEAN       Is64Bit
  )
{
  MACH_HEADER_ANY *MachHeader;
  
  MachoFilterFatArchitectureByType (Buffer, BufferSize, Is64Bit ? MachCpuTypeX8664 : MachCpuTypeI386);
  MachHeader = (MACH_HEADER_ANY *)* Buffer; // TODO alignment?

  if ((!Is64Bit && MachHeader->Signature == MACH_HEADER_SIGNATURE)
    || (Is64Bit && MachHeader->Signature == MACH_HEADER_64_SIGNATURE)) {
    return TRUE;
  }

  return FALSE;
}

STATIC
VOID
UpdateMkextLengthChecksum (
  IN MKEXT_HEADER_ANY   *Mkext,
  IN UINT32             Length
  )
{
  Mkext->Common.Length  = SwapBytes32 (Length);
  Mkext->Common.Adler32 = SwapBytes32 (
    Adler32 ((UINT8*)&Mkext->Common.Version,
      Length - OFFSET_OF (MKEXT_CORE_HEADER, Version))
      );
}

STATIC
BOOLEAN
ParseMkextV2Plist (
  IN  MKEXT_V2_HEADER   *Mkext,
  OUT UINT8             **Plist,
  OUT XML_DOCUMENT      **PlistDoc,
  OUT XML_NODE          **PlistBundles
  )
{
  UINT8               *MkextBuffer;
  UINT32              MkextLength;
  UINT8               *PlistBuffer;
  XML_DOCUMENT        *PlistXml;
  UINT32              PlistOffset;
  UINT32              PlistCompressedSize;
  UINT32              PlistFullSize;
  UINT32              PlistStoredSize;
  UINT32              Tmp;

  XML_NODE            *MkextInfoRoot;
  UINT32              MkextInfoRootIndex;
  UINT32              MkextInfoRootCount;

  XML_NODE            *PlistBundleArray;
  CONST CHAR8         *BundleArrayKey;
  

  MkextBuffer         = (UINT8 *) Mkext;
  MkextLength         = SwapBytes32 (Mkext->Header.Length);
  PlistOffset         = SwapBytes32 (Mkext->PlistOffset);
  PlistCompressedSize = SwapBytes32 (Mkext->PlistCompressedSize);
  PlistFullSize       = SwapBytes32 (Mkext->PlistFullSize);

  PlistStoredSize = PlistCompressedSize;
  if (PlistStoredSize == 0) {
    PlistStoredSize = PlistFullSize;
  }

  if (OcOverflowAddU32 (PlistOffset, PlistStoredSize, &Tmp) || Tmp > MkextLength) {
    return FALSE;
  }

  PlistBuffer = AllocatePool (PlistFullSize);
  if (PlistBuffer == NULL) {
    return FALSE;
  }

  //
  // Copy/decompress plist.
  //
  if (PlistCompressedSize > 0) {
    DecompressZLIB (
      (UINT8 *) PlistBuffer, PlistFullSize,
      &MkextBuffer[PlistOffset], PlistCompressedSize
      );
  } else {
    CopyMem (PlistBuffer, &MkextBuffer[PlistOffset], PlistFullSize);
  }

  PlistXml = XmlDocumentParse ((CHAR8 *) PlistBuffer, PlistFullSize, FALSE);
  if (PlistXml == NULL) {
    FreePool (PlistBuffer);
    return FALSE;
  }

  //
  // Mkext v2 root element is a dictionary containing an array of bundles.
  //
  MkextInfoRoot = PlistNodeCast (XmlDocumentRoot (PlistXml), PLIST_NODE_TYPE_DICT);
  if (MkextInfoRoot == NULL) {
    FreePool (PlistBuffer);
    return FALSE;
  }

  //
  // Get bundle array.
  //
  MkextInfoRootCount = PlistDictChildren (MkextInfoRoot);
  for (MkextInfoRootIndex = 0; MkextInfoRootIndex < MkextInfoRootCount; MkextInfoRootIndex++) {
    BundleArrayKey = PlistKeyValue (PlistDictChild (MkextInfoRoot, MkextInfoRootIndex, &PlistBundleArray));
    if (AsciiStrCmp (BundleArrayKey, MKEXT_INFO_DICTIONARIES_KEY) == 0) {
      *Plist        = PlistBuffer;
      *PlistDoc     = PlistXml;
      *PlistBundles = PlistBundleArray;

      return TRUE;
    }
  }

  //
  // No bundle array found.
  //
  FreePool (PlistBuffer);
  return FALSE;
}

STATIC
UINT32
UpdateMkextV2Plist (
  IN MKEXT_V2_HEADER  *Mkext,
  IN XML_DOCUMENT     *PlistDoc,
  IN UINT32           Offset
  )
{
  UINT8       *MkextBuffer;
  CHAR8       *ExportedInfo;
  UINT32      ExportedInfoSize;

  //
  // Export plist and include \0 terminator in size.
  //
  ExportedInfo = XmlDocumentExport (PlistDoc, &ExportedInfoSize, 0, FALSE);
  if (ExportedInfo == NULL) {
    return 0;
  }
  ExportedInfoSize++;

  MkextBuffer = (UINT8*)Mkext;
  CopyMem (&MkextBuffer[Offset], ExportedInfo, ExportedInfoSize);
  FreePool (ExportedInfo);

  Mkext->PlistOffset = SwapBytes32 (Offset);
  Mkext->PlistFullSize = SwapBytes32 (ExportedInfoSize);
  Mkext->PlistCompressedSize = 0;
  return ExportedInfoSize;
}

EFI_STATUS
MkextDecompress (
  IN     CONST UINT8      *Buffer,
  IN     UINT32           BufferSize,
  IN     UINT32           NumReservedKexts,
  IN OUT UINT8            *OutBuffer,
  IN     UINT32           OutBufferSize,
  IN OUT UINT32           *OutMkextSize
  )
{
  BOOLEAN               MkextDecompress;

  MKEXT_HEADER_ANY      *MkextHeader;
  MKEXT_HEADER_ANY      *MkextHeaderOut;
  UINT32                MkextSize;
  UINT32                MkextVersion;
  UINT32                NumKexts;
  UINT32                NumMaxKexts;

  UINT32                Tmp;
  UINT32                Index;
  UINT32                CurrentOffset;
  UINT32                NewOffset;
  UINT32                PlistOffset;
  UINT32                PlistCompSize;
  UINT32                PlistFullSize;
  UINT32                BinOffset;
  UINT32                BinCompSize;
  UINT32                BinFullSize;

  UINT8                 *PlistBuffer;
  XML_DOCUMENT          *PlistXml;
  XML_NODE              *PlistBundles;
  UINT32                PlistBundlesCount;
  XML_NODE              *PlistBundle;
  UINT32                PlistBundleIndex;
  UINT32                PlistBundleCount;
  CONST CHAR8           *PlistBundleKey;
  XML_NODE              *BundleExecutable;

  MKEXT_V2_FILE_ENTRY   *MkextExecutableEntry;
  MKEXT_V2_FILE_ENTRY   *MkextOutExecutableEntry;
  CHAR8                 *BinaryOffsetStrings;
  UINT32                BinaryOffsetStringsSize;

  ASSERT (Buffer != NULL);
  ASSERT (BufferSize > 0);

  MkextDecompress = OutBufferSize > 0;
  if (MkextDecompress) {
    ASSERT (OutBuffer != NULL);
    ASSERT (OutMkextSize != NULL);
  }

  if (BufferSize < sizeof (MKEXT_CORE_HEADER)
    || !OC_TYPE_ALIGNED (MKEXT_CORE_HEADER, Buffer)) {
    return EFI_INVALID_PARAMETER;
  }

  MkextHeader   = (MKEXT_HEADER_ANY *) Buffer;
  MkextSize     = SwapBytes32 (MkextHeader->Common.Length);
  MkextVersion  = SwapBytes32 (MkextHeader->Common.Version);
  NumKexts      = SwapBytes32 (MkextHeader->Common.NumKexts);

  if (MkextHeader->Common.Magic != MKEXT_INVERT_MAGIC
    || MkextHeader->Common.Signature != MKEXT_INVERT_SIGNATURE
    || BufferSize != MkextSize
    || OcOverflowAddU32 (NumKexts, NumReservedKexts, &NumMaxKexts)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Mkext v1.
  //
  if (MkextVersion == MKEXT_VERSION_V1) {
    if (OcOverflowMulAddU32 (sizeof (MKEXT_V1_KEXT), NumMaxKexts, sizeof (MKEXT_V1_HEADER), &CurrentOffset)) {
      return EFI_INVALID_PARAMETER;
    }
    CurrentOffset = MKEXT_ALIGN (CurrentOffset);

    if (MkextDecompress) {
      //
      // Copy header.
      //
      CopyMem (OutBuffer, Buffer, sizeof (MKEXT_V1_HEADER));
      MkextHeaderOut = (MKEXT_HEADER_ANY *) OutBuffer;
    }

    //
    // Process plists and binaries if present, decompressing as needed.
    //
    for (Index = 0; Index < NumKexts; Index++) {
      PlistFullSize   = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.FullSize);
      BinFullSize     = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.FullSize);

      if (MkextDecompress) {
        //
        // Decompress entry.
        //
        PlistOffset     = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.Offset);
        PlistCompSize   = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.CompressedSize);
        BinOffset       = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.Offset);
        BinCompSize     = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.CompressedSize);

        if (CurrentOffset > OutBufferSize) {
          return EFI_BUFFER_TOO_SMALL;
        }

        //
        // Compressed size == 0 means no compression.
        //
        if (PlistCompSize > 0) {
          if (OcOverflowAddU32 (PlistOffset, PlistCompSize, &Tmp)
            || Tmp > MkextSize
            || DecompressLZSS (
              &OutBuffer[CurrentOffset],
              PlistFullSize,
              &((UINT8*)Buffer)[PlistOffset],
              PlistCompSize
            ) != PlistFullSize) {
            return EFI_INVALID_PARAMETER;
          }
        } else {
          if (OcOverflowAddU32 (PlistOffset, PlistFullSize, &Tmp)
            || Tmp > MkextSize) {
            return EFI_INVALID_PARAMETER;
          }
          CopyMem (&OutBuffer[CurrentOffset], &Buffer[PlistOffset], PlistFullSize);
        }

        MkextHeaderOut->V1.Kexts[Index].Plist.Offset          = SwapBytes32 (CurrentOffset);
        MkextHeaderOut->V1.Kexts[Index].Plist.CompressedSize  = 0;
        MkextHeaderOut->V1.Kexts[Index].Plist.FullSize        = SwapBytes32 (PlistFullSize);
        MkextHeaderOut->V1.Kexts[Index].Plist.ModifiedSeconds = MkextHeader->V1.Kexts[Index].Plist.ModifiedSeconds;
        if (OcOverflowAddU32 (CurrentOffset, MKEXT_ALIGN (PlistFullSize), &CurrentOffset)) {
          return EFI_INVALID_PARAMETER;
        }

        if (BinFullSize > 0) {
          if (CurrentOffset > OutBufferSize) {
            return EFI_INVALID_PARAMETER;
          }

          //
          // Compressed size == 0 means no compression.
          //
          if (BinCompSize > 0) {
            if (OcOverflowAddU32 (BinOffset, BinCompSize, &Tmp)
              || Tmp > MkextSize
              || DecompressLZSS (
                &OutBuffer[CurrentOffset],
                BinFullSize,
                &((UINT8*)Buffer)[BinOffset],
                BinCompSize
              ) != BinFullSize) {
              return EFI_INVALID_PARAMETER;
            }
          } else {
            if (OcOverflowAddU32 (BinOffset, BinFullSize, &Tmp)
              || Tmp > MkextSize) {
              return EFI_INVALID_PARAMETER;
            }
            CopyMem (&OutBuffer[CurrentOffset], &Buffer[BinOffset], BinFullSize);
          }

          MkextHeaderOut->V1.Kexts[Index].Binary.Offset           = SwapBytes32 (CurrentOffset);
          MkextHeaderOut->V1.Kexts[Index].Binary.CompressedSize   = 0;
          MkextHeaderOut->V1.Kexts[Index].Binary.FullSize         = SwapBytes32 (BinFullSize);
          MkextHeaderOut->V1.Kexts[Index].Binary.ModifiedSeconds  = MkextHeader->V1.Kexts[Index].Binary.ModifiedSeconds;
          if (OcOverflowAddU32 (CurrentOffset, MKEXT_ALIGN (BinFullSize), &CurrentOffset)) {
            return EFI_INVALID_PARAMETER;
          }
        } else {
          ZeroMem (&MkextHeaderOut->V1.Kexts[Index].Binary, sizeof (MKEXT_V1_KEXT_FILE));
        }

      } else {
        //
        // Calculate size only.
        //
        if (OcOverflowTriAddU32 (
          CurrentOffset,
          MKEXT_ALIGN (PlistFullSize),
          MKEXT_ALIGN (BinFullSize),
          &CurrentOffset
          )) {
          return EFI_INVALID_PARAMETER;
        }
      }
    }

    *OutMkextSize = CurrentOffset;

  //
  // Mkext v2.
  //
  } else if (MkextVersion == MKEXT_VERSION_V2) {
    if (MkextDecompress) {
      //
      // Copy header.
      //
      CopyMem (OutBuffer, Buffer, sizeof (MKEXT_V2_HEADER));
      MkextHeaderOut = (MKEXT_HEADER_ANY *) OutBuffer;
    }
    CurrentOffset = MKEXT_ALIGN (sizeof (MKEXT_V2_HEADER));

    if (!ParseMkextV2Plist (&MkextHeader->V2, &PlistBuffer, &PlistXml, &PlistBundles)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // All offset strings are kept in array until the plist is regenerated.
    //
    PlistBundlesCount   = XmlNodeChildren (PlistBundles);
    BinaryOffsetStrings = NULL;

    if (MkextDecompress) {
      if (OcOverflowTriMulU32 (PlistBundlesCount, MKEXT_OFFSET_STR_LEN, sizeof (CHAR8), &BinaryOffsetStringsSize)) {
        XmlDocumentFree (PlistXml);
        FreePool (PlistBuffer);
        return EFI_INVALID_PARAMETER;
      }

      BinaryOffsetStrings = AllocateZeroPool (BinaryOffsetStringsSize);
      if (BinaryOffsetStrings == NULL) {
        XmlDocumentFree (PlistXml);
        FreePool (PlistBuffer);
        return EFI_OUT_OF_RESOURCES;
      }
    }

    //
    // Enumerate bundles.
    //
    for (Index = 0; Index < PlistBundlesCount; Index++) {
      PlistBundle = PlistNodeCast (XmlNodeChild (PlistBundles, Index), PLIST_NODE_TYPE_DICT);
      if (PlistBundle == NULL) {
        XmlDocumentFree (PlistXml);
        FreePool (PlistBuffer);
        if (MkextDecompress) {
          FreePool (BinaryOffsetStrings);
        }
        return EFI_INVALID_PARAMETER;
      }

      //
      // Get executable information, if present.
      //
      PlistBundleCount = PlistDictChildren (PlistBundle);
      for (PlistBundleIndex = 0; PlistBundleIndex < PlistBundleCount; PlistBundleIndex++) {
        PlistBundleKey = PlistKeyValue (PlistDictChild (PlistBundle, PlistBundleIndex, &BundleExecutable));
        if (AsciiStrCmp (PlistBundleKey, MKEXT_EXECUTABLE_KEY) == 0) {
          if (!PlistIntegerValue (BundleExecutable, &BinOffset, sizeof (BinOffset), TRUE)
            || BinOffset == 0
            || BinOffset > MkextSize - sizeof (MKEXT_V2_FILE_ENTRY)) {
            XmlDocumentFree (PlistXml);
            FreePool (PlistBuffer);
            if (MkextDecompress) {
              FreePool (BinaryOffsetStrings);
            }
            return EFI_INVALID_PARAMETER;
          }

          MkextExecutableEntry  = (MKEXT_V2_FILE_ENTRY *) &Buffer[BinOffset];
          BinCompSize           = SwapBytes32 (MkextExecutableEntry->CompressedSize);
          BinFullSize           = SwapBytes32 (MkextExecutableEntry->FullSize);

          if (OcOverflowTriAddU32 (CurrentOffset, sizeof (MKEXT_V2_FILE_ENTRY), MKEXT_ALIGN (BinFullSize), &NewOffset)
            || (MkextDecompress && NewOffset > OutBufferSize)) {
            XmlDocumentFree (PlistXml);
            FreePool (PlistBuffer);
            if (MkextDecompress) {
              FreePool (BinaryOffsetStrings);
            }
            return EFI_BUFFER_TOO_SMALL;
          }

          if (MkextDecompress) {
            MkextOutExecutableEntry                 = (MKEXT_V2_FILE_ENTRY *) &OutBuffer[CurrentOffset];
            MkextOutExecutableEntry->CompressedSize = 0;
            MkextOutExecutableEntry->FullSize       = SwapBytes32 (BinFullSize);

            //
            // Compressed size == 0 means no compression.
            //
            if (BinCompSize > 0) {
              if (OcOverflowAddU32 (BinOffset, BinCompSize, &Tmp)
                || Tmp > MkextSize - sizeof (MKEXT_V2_FILE_ENTRY)
                || DecompressZLIB (
                  MkextOutExecutableEntry->Data,
                  BinFullSize,
                  MkextExecutableEntry->Data,
                  BinCompSize
                ) != BinFullSize) {
                XmlDocumentFree (PlistXml);
                FreePool (PlistBuffer);
                FreePool (BinaryOffsetStrings);
                return EFI_INVALID_PARAMETER;
              }
            } else {
              if (OcOverflowAddU32 (BinOffset, BinFullSize, &Tmp)
                || Tmp > MkextSize - sizeof (MKEXT_V2_FILE_ENTRY)) {
                XmlDocumentFree (PlistXml);
                FreePool (PlistBuffer);
                FreePool (BinaryOffsetStrings);
                return EFI_INVALID_PARAMETER;
              }
              CopyMem (MkextOutExecutableEntry->Data, MkextExecutableEntry->Data, BinFullSize);
            }

            if (!AsciiUint64ToLowerHex (
              &BinaryOffsetStrings[Index * MKEXT_OFFSET_STR_LEN],
              MKEXT_OFFSET_STR_LEN,
              CurrentOffset
              )) {
              XmlDocumentFree (PlistXml);
              FreePool (PlistBuffer);
              FreePool (BinaryOffsetStrings);
              return EFI_INVALID_PARAMETER;
            }
            XmlNodeChangeContent (BundleExecutable, &BinaryOffsetStrings[Index * MKEXT_OFFSET_STR_LEN]);
          }

          //
          // Move to next bundle.
          //
          CurrentOffset = NewOffset;
          break;
        }
      }
    }

    if (MkextDecompress) {
      PlistFullSize = UpdateMkextV2Plist (&MkextHeaderOut->V2, PlistXml, CurrentOffset);
    }
    XmlDocumentFree (PlistXml);
    FreePool (PlistBuffer);
    if (MkextDecompress) {
      FreePool (BinaryOffsetStrings);

      if (PlistFullSize == 0
        || OcOverflowAddU32 (CurrentOffset, PlistFullSize, OutMkextSize)) {
        return EFI_INVALID_PARAMETER;
      }
    } else {
      //
      // Account for plist, 128 bytes per bundle for future plist expansion, 
      //   and additional headers for future kext injection.
      //
      PlistFullSize = SwapBytes32 (MkextHeader->V2.PlistFullSize);
      if (OcOverflowAddU32 (CurrentOffset, PlistFullSize, &CurrentOffset)
        || OcOverflowMulAddU32 (PlistBundlesCount, 128, CurrentOffset, &CurrentOffset)
        || OcOverflowMulAddU32 (NumReservedKexts, sizeof (MKEXT_V2_FILE_ENTRY), CurrentOffset, OutMkextSize)) {
        return EFI_INVALID_PARAMETER;
      }
    }

  //
  // Unsupported version.
  //
  } else {
    return EFI_UNSUPPORTED;
  }

  if (MkextDecompress) {
    UpdateMkextLengthChecksum (MkextHeaderOut, *OutMkextSize);
  }
  return EFI_SUCCESS;
}

EFI_STATUS
MkextContextInit (
  IN OUT  MKEXT_CONTEXT      *Context,
  IN OUT  UINT8              *Mkext,
  IN      UINT32             MkextSize,
  IN      UINT32             MkextAllocSize
  )
{
  MKEXT_HEADER_ANY    *MkextHeader;
  UINT32              MkextVersion;
  MACH_CPU_TYPE       CpuType;
  BOOLEAN             Is64Bit;
  UINT32              NumKexts;
  UINT32              NumMaxKexts;

  UINT32              Tmp;
  UINT32              Index;
  UINT32              StartingOffset;
  UINT32              CurrentOffset;

  UINT8               *PlistBuffer;
  XML_DOCUMENT        *PlistXml;
  UINT32              PlistOffset;
  XML_NODE            *PlistBundles;
  UINT32              PlistBundlesCount;
  XML_NODE            *PlistBundle;
  UINT32              PlistBundleIndex;
  UINT32              PlistBundleCount;
  CONST CHAR8         *PlistBundleKey;
  XML_NODE            *BundleExecutable;
  UINT32              BinOffset;

  //
  // Assumptions:
  //    Kexts are fully decompressed and aligned to 8 bytes with plist (for v2) at end of mkext.
  //    Mkext is big-endian per XNU requirements.
  //

  ASSERT (Context != NULL);
  ASSERT (Mkext != NULL);
  ASSERT (MkextSize > 0);
  ASSERT (MkextAllocSize >= MkextSize);

  if (MkextSize < sizeof (MKEXT_CORE_HEADER)
    || !OC_TYPE_ALIGNED (MKEXT_CORE_HEADER, Mkext)) {
    return EFI_INVALID_PARAMETER;
  }

  MkextHeader   = (MKEXT_HEADER_ANY *) Mkext;
  MkextVersion  = SwapBytes32 (MkextHeader->Common.Version);
  NumKexts      = SwapBytes32 (MkextHeader->Common.NumKexts);
  CpuType       = SwapBytes32 (MkextHeader->Common.CpuType);

  if (MkextHeader->Common.Magic != MKEXT_INVERT_MAGIC
    || MkextHeader->Common.Signature != MKEXT_INVERT_SIGNATURE
    || MkextSize != SwapBytes32 (MkextHeader->Common.Length)) {
    return EFI_INVALID_PARAMETER;
  }

  if (CpuType == MachCpuTypeI386) {
    Is64Bit = FALSE;
  } else if (CpuType == MachCpuTypeX8664) {
    Is64Bit = TRUE;
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Mkext v1.
  //
  if (MkextVersion == MKEXT_VERSION_V1) {
    //
    // Calculate available kext slots. This value is assumed to be under the UINT32 max later on.
    //
    // Below finds the lowest offset to a plist or a binary, which is used to locate
    // the end of the usable space allocated for kext slots.
    //
    StartingOffset = 0;
    for (Index = 0; Index < NumKexts; Index++) {
      CurrentOffset = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.Offset);
      if (MkextHeader->V1.Kexts[Index].Plist.CompressedSize != 0) {
        return EFI_UNSUPPORTED;
      }
      if (StartingOffset == 0 || CurrentOffset < StartingOffset) {
        StartingOffset = CurrentOffset;
      }

      //
      // A binary entry of zero size indicates no binary is present.
      //
      if (MkextHeader->V1.Kexts[Index].Binary.FullSize > 0) {
        CurrentOffset = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.Offset);
        if (MkextHeader->V1.Kexts[Index].Binary.CompressedSize != 0) {
          return EFI_UNSUPPORTED;
        }
        if (CurrentOffset < StartingOffset) {
          StartingOffset = CurrentOffset;
        }
      }
    }

    if (OcOverflowMulAddU32 (sizeof (MKEXT_V1_KEXT), NumKexts, sizeof (MKEXT_V1_HEADER), &Tmp)
      || StartingOffset < Tmp
      || StartingOffset > MkextSize) {
      return EFI_INVALID_PARAMETER;
    }

    Tmp = (StartingOffset - Tmp) / sizeof (MKEXT_V1_KEXT);
    if (OcOverflowAddU32 (Tmp, NumKexts, &NumMaxKexts)
      || NumMaxKexts == MAX_UINT32) {
      return EFI_INVALID_PARAMETER;
    }
    
  //
  // Mkext v2.
  //
  } else if (MkextVersion == MKEXT_VERSION_V2) {
    if (!ParseMkextV2Plist (&MkextHeader->V2, &PlistBuffer, &PlistXml, &PlistBundles)) {
      return EFI_INVALID_PARAMETER;
    }
    PlistOffset = SwapBytes32 (MkextHeader->V2.PlistOffset);

    //
    // Enumerate bundle dicts.
    //
    PlistBundlesCount = XmlNodeChildren (PlistBundles);
    for (Index = 0; Index < PlistBundlesCount; Index++) {
      PlistBundle = PlistNodeCast (XmlNodeChild (PlistBundles, Index), PLIST_NODE_TYPE_DICT);
      if (PlistBundle == NULL) {
        XmlDocumentFree (PlistXml);
        FreePool (PlistBuffer);
        return EFI_INVALID_PARAMETER;
      }

      PlistBundleCount = PlistDictChildren (PlistBundle);
      for (PlistBundleIndex = 0; PlistBundleIndex < PlistBundleCount; PlistBundleIndex++) {
        PlistBundleKey = PlistKeyValue (PlistDictChild (PlistBundle, PlistBundleIndex, &BundleExecutable));
        if (AsciiStrCmp (PlistBundleKey, MKEXT_EXECUTABLE_KEY) == 0) {
          //
          // Ensure binary offset is before plist offset.
          // Try in hex first, and then decimal.
          //
          if (!PlistIntegerValue (BundleExecutable, &BinOffset, sizeof (BinOffset), TRUE)
            || BinOffset >= PlistOffset) {
            XmlDocumentFree (PlistXml);
            FreePool (PlistBuffer);
            return EFI_INVALID_PARAMETER;
          }
          if (BinOffset == 0) {
            if (!PlistIntegerValue (BundleExecutable, &BinOffset, sizeof (BinOffset), FALSE)
              || BinOffset == 0
              || BinOffset >= PlistOffset) {
              XmlDocumentFree (PlistXml);
              FreePool (PlistBuffer);
              return EFI_INVALID_PARAMETER;
            }
          }
        }
      }
    }

  //
  // Unsupported version.
  //
  } else {
    return EFI_UNSUPPORTED;
  }

  ZeroMem (Context, sizeof (MKEXT_CONTEXT));
  Context->Mkext                = Mkext;
  Context->MkextSize            = MkextSize;
  Context->MkextHeader          = MkextHeader;
  Context->MkextAllocSize       = MkextAllocSize;
  Context->MkextVersion         = MkextVersion;
  Context->Is64Bit              = Is64Bit;
  Context->NumKexts             = NumKexts;

  if (MkextVersion == MKEXT_VERSION_V1) {
    Context->NumMaxKexts        = NumMaxKexts;
  } else if (MkextVersion == MKEXT_VERSION_V2) {
    Context->MkextInfoOffset    = PlistOffset;
    Context->MkextInfo          = PlistBuffer;
    Context->MkextInfoDocument  = PlistXml;
    Context->MkextKexts         = PlistBundles;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MkextInjectKext (
  IN OUT MKEXT_CONTEXT      *Context,
  IN     CONST CHAR8        *BundlePath,
  IN     CONST CHAR8        *InfoPlist,
  IN     UINT32             InfoPlistSize,
  IN     UINT8              *Executable OPTIONAL,
  IN     UINT32             ExecutableSize OPTIONAL
  )
{
  UINT32                MkextNewSize;
  UINT32                PlistOffset;
  UINT32                BinOffset;

  CHAR8                 *PlistBuffer;
  CHAR8                 *PlistExported;
  UINT32                PlistExportedSize;
  XML_DOCUMENT          *PlistXml;
  XML_NODE              *PlistRoot;
  BOOLEAN               PlistFailed;
  
  CONST CHAR8           *TmpKeyValue;
  UINT32                FieldCount;
  UINT32                FieldIndex;

  CHAR8                 ExecutableSourceAddrStr[24];
  MKEXT_V2_FILE_ENTRY   *MkextExecutableEntry;

  ASSERT (Context != NULL);
  ASSERT (BundlePath != NULL);
  ASSERT (InfoPlist != NULL);
  ASSERT (InfoPlistSize > 0);

  //
  // Mkext v1.
  //
  if (Context->MkextVersion == MKEXT_VERSION_V1) {
    if (Context->NumKexts >= Context->NumMaxKexts) {
      return EFI_BUFFER_TOO_SMALL;
    }

    //
    // Plist will be placed at end of mkext.
    //
    PlistOffset = Context->MkextSize;
    if (OcOverflowAddU32 (PlistOffset, MKEXT_ALIGN (InfoPlistSize), &MkextNewSize)) {
      return EFI_INVALID_PARAMETER;
    } else if (MkextNewSize > Context->MkextAllocSize) {
      return EFI_BUFFER_TOO_SMALL;
    }

    //
    // Executable, if present, will be placed right after plist.
    //
    if (Executable != NULL) {
      ASSERT (ExecutableSize > 0);

      BinOffset = MkextNewSize;
      if (!ParseKextBinary (&Executable, &ExecutableSize, Context->Is64Bit)
        || OcOverflowAddU32 (BinOffset, MKEXT_ALIGN (ExecutableSize), &MkextNewSize)) {
        return EFI_INVALID_PARAMETER;
      }
      
      if (MkextNewSize > Context->MkextAllocSize) {
        return EFI_BUFFER_TOO_SMALL;
      }

      CopyMem (&Context->Mkext[BinOffset], Executable, ExecutableSize);
      Context->MkextHeader->V1.Kexts[Context->NumKexts].Binary.Offset = SwapBytes32 (BinOffset);
      Context->MkextHeader->V1.Kexts[Context->NumKexts].Binary.CompressedSize = 0;
      Context->MkextHeader->V1.Kexts[Context->NumKexts].Binary.FullSize = SwapBytes32 (ExecutableSize);
      Context->MkextHeader->V1.Kexts[Context->NumKexts].Binary.ModifiedSeconds = 0;
    }

    CopyMem (&Context->Mkext[PlistOffset], InfoPlist, InfoPlistSize);
    Context->MkextHeader->V1.Kexts[Context->NumKexts].Plist.Offset = SwapBytes32 (PlistOffset);
    Context->MkextHeader->V1.Kexts[Context->NumKexts].Plist.CompressedSize = 0;
    Context->MkextHeader->V1.Kexts[Context->NumKexts].Plist.FullSize = SwapBytes32 (InfoPlistSize);
    Context->MkextHeader->V1.Kexts[Context->NumKexts].Plist.ModifiedSeconds = 0;

    //
    // Assumption:
    //    NumKexts is checked to be under MaxNumKexts, which is assumed to be under
    //    the max value of UINT32 during context creation. This operation thus cannot overflow.
    //
    Context->MkextSize = MkextNewSize;
    Context->NumKexts++;

  //
  // Mkext v2.
  //
  } else if (Context->MkextVersion == MKEXT_VERSION_V2) {
    PlistBuffer = AllocateCopyPool (InfoPlistSize, InfoPlist);
    if (PlistBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    PlistXml = XmlDocumentParse (PlistBuffer, InfoPlistSize, FALSE);
    if (PlistXml == NULL) {
      FreePool (PlistBuffer);
      return EFI_INVALID_PARAMETER;
    }

    PlistRoot = PlistNodeCast (PlistDocumentRoot (PlistXml), PLIST_NODE_TYPE_DICT);
    if (PlistRoot == NULL) {
      XmlDocumentFree (PlistXml);
      FreePool (PlistBuffer);
      return EFI_INVALID_PARAMETER;
    }

    //
    // We are not supposed to check for this, it is XNU responsibility, which reliably panics.
    // However, to avoid certain users making this kind of mistake, we still provide some
    // code in debug mode to diagnose it.
    //
    DEBUG_CODE_BEGIN ();
    if (Executable == NULL) {
      FieldCount = PlistDictChildren (PlistRoot);
      for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
        TmpKeyValue = PlistKeyValue (PlistDictChild (PlistRoot, FieldIndex, NULL));
        if (TmpKeyValue == NULL) {
          continue;
        }

        if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_EXECUTABLE_KEY) == 0) {
          DEBUG ((DEBUG_ERROR, "OCK: Plist-only kext has %a key\n", INFO_BUNDLE_EXECUTABLE_KEY));
          ASSERT (FALSE);
          CpuDeadLoop ();
        }
      }
    }
    DEBUG_CODE_END ();

    //
    // Executable, if present, will be placed at end of mkext and plist will be moved further out.
    //
    PlistFailed = FALSE;
    if (Executable != NULL) {
      ASSERT (ExecutableSize > 0);
      
      BinOffset = Context->MkextInfoOffset;
      if (!ParseKextBinary (&Executable, &ExecutableSize, Context->Is64Bit)
        || OcOverflowTriAddU32 (BinOffset, sizeof (MKEXT_V2_FILE_ENTRY), MKEXT_ALIGN (ExecutableSize), &PlistOffset)) {
        XmlDocumentFree (PlistXml);
        FreePool (PlistBuffer);
        return EFI_INVALID_PARAMETER;
      } 
      
      if (PlistOffset >= Context->MkextAllocSize) {
        XmlDocumentFree (PlistXml);
        FreePool (PlistBuffer);
        return EFI_BUFFER_TOO_SMALL;
      }

      PlistFailed |= !AsciiUint64ToLowerHex (ExecutableSourceAddrStr, sizeof (ExecutableSourceAddrStr), BinOffset);
      PlistFailed |= XmlNodeAppend (PlistRoot, "key", NULL, MKEXT_EXECUTABLE_KEY) == NULL;
      PlistFailed |= XmlNodeAppend (PlistRoot, "integer", MKEXT_INFO_INTEGER_ATTRIBUTES, ExecutableSourceAddrStr) == NULL;
    }

    //
    // Add bundle path.
    //
    PlistFailed |= XmlNodeAppend (PlistRoot, "key", NULL, MKEXT_BUNDLE_PATH_KEY) == NULL;
    PlistFailed |= XmlNodeAppend (PlistRoot, "string", NULL, BundlePath) == NULL;
    if (PlistFailed) {
      XmlDocumentFree (PlistXml);
      FreePool (PlistBuffer);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Strip down Info.plist for appending to primary plist.
    //
    PlistExported = XmlDocumentExport (PlistXml, &PlistExportedSize, 2, FALSE);
    XmlDocumentFree (PlistXml);
    FreePool (PlistBuffer);

    if (XmlNodeAppend (Context->MkextKexts, "dict", NULL, PlistExported) == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Context->MkextInfoOffset = PlistOffset;

    if (Executable != NULL) {
      MkextExecutableEntry = (MKEXT_V2_FILE_ENTRY *) &Context->Mkext[BinOffset];
      MkextExecutableEntry->CompressedSize = 0;
      MkextExecutableEntry->FullSize = SwapBytes32 (ExecutableSize);
      CopyMem (MkextExecutableEntry->Data, Executable, ExecutableSize);
    }
  
  //
  // Unsupported version.
  //
  } else {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
MkextInjectPatchComplete (
  IN OUT MKEXT_CONTEXT      *Context
  )
{
  UINT32 MkextPlistSize;

  ASSERT (Context != NULL);

  //
  // Mkext v1.
  //
  if (Context->MkextVersion == MKEXT_VERSION_V1) {
    Context->MkextHeader->Common.NumKexts = SwapBytes32 (Context->NumKexts);

  //
  // Mkext v2.
  //
  } else if (Context->MkextVersion == MKEXT_VERSION_V2) {
    MkextPlistSize = UpdateMkextV2Plist (&Context->MkextHeader->V2, Context->MkextInfoDocument, Context->MkextInfoOffset);
    Context->MkextSize = Context->MkextInfoOffset + MkextPlistSize;

  //
  // Unsupported version.
  //
  } else {
    return EFI_UNSUPPORTED;
  }

  UpdateMkextLengthChecksum (Context->MkextHeader, Context->MkextSize);
  return EFI_SUCCESS;
}