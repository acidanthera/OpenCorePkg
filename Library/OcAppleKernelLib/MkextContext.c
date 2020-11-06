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

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcCompressionLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>

#include "MkextInternal.h"
#include "PrelinkedInternal.h"

//
// Alignment to 8 bytes.
//
#define MKEXT_ALIGN(a)         (ALIGN_VALUE (a, sizeof (UINT64)))

STATIC
BOOLEAN
ParseKextBinary (
  IN OUT UINT8         **Buffer,
  IN OUT UINT32        *BufferSize,
  IN     BOOLEAN       Is32Bit
  )
{
  EFI_STATUS        Status;
  MACH_HEADER_ANY   *MachHeader;
  
  Status = FatFilterArchitectureByType (Buffer, BufferSize, Is32Bit ? MachCpuTypeI386 : MachCpuTypeX8664);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Size/alignment checked by FatFilterArchitectureByType.
  //
  MachHeader = (MACH_HEADER_ANY *)* Buffer;

  if ((!Is32Bit && MachHeader->Signature == MACH_HEADER_64_SIGNATURE)
    || (Is32Bit && MachHeader->Signature == MACH_HEADER_SIGNATURE)) {
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
  OUT UINT32            *PlistSize,
  OUT XML_DOCUMENT      **PlistDoc,
  OUT XML_NODE          **PlistBundles
  )
{
  UINT8               *MkextBuffer;
  UINT32              MkextLength;
  VOID                *PlistBuffer;
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
    if (DecompressZLIB (
      PlistBuffer,
      PlistFullSize,
      &MkextBuffer[PlistOffset],
      PlistCompressedSize
      ) != PlistFullSize) {
      FreePool (PlistBuffer);
      return FALSE;
    }
  } else {
    CopyMem (PlistBuffer, &MkextBuffer[PlistOffset], PlistFullSize);
  }

  PlistXml = XmlDocumentParse (PlistBuffer, PlistFullSize, FALSE);
  if (PlistXml == NULL) {
    FreePool (PlistBuffer);
    return FALSE;
  }

  //
  // Mkext v2 root element is a dictionary containing an array of bundles.
  //
  MkextInfoRoot = PlistNodeCast (XmlDocumentRoot (PlistXml), PLIST_NODE_TYPE_DICT);
  if (MkextInfoRoot == NULL) {
    XmlDocumentFree (PlistXml);
    FreePool (PlistBuffer);
    return FALSE;
  }

  //
  // Get bundle array.
  //
  MkextInfoRootCount = PlistDictChildren (MkextInfoRoot);
  for (MkextInfoRootIndex = 0; MkextInfoRootIndex < MkextInfoRootCount; MkextInfoRootIndex++) {
    BundleArrayKey = PlistKeyValue (PlistDictChild (MkextInfoRoot, MkextInfoRootIndex, &PlistBundleArray));
    if (BundleArrayKey == NULL) {
      continue;
    }

    if (AsciiStrCmp (BundleArrayKey, MKEXT_INFO_DICTIONARIES_KEY) == 0) {
      *Plist        = PlistBuffer;
      *PlistSize    = PlistFullSize;
      *PlistDoc     = PlistXml;
      *PlistBundles = PlistBundleArray;

      return TRUE;
    }
  }

  //
  // No bundle array found.
  //
  XmlDocumentFree (PlistXml);
  FreePool (PlistBuffer);
  return FALSE;
}

STATIC
UINT32
UpdateMkextV2Plist (
  IN OUT MKEXT_V2_HEADER    *Mkext,
  IN     UINT32             AllocatedSize,
  IN     XML_DOCUMENT       *PlistDoc,
  IN     UINT32             Offset
  )
{
  UINT8       *MkextBuffer;
  CHAR8       *ExportedInfo;
  UINT32      ExportedInfoSize;
  UINT32      TmpSize;

  //
  // Export plist and include \0 terminator in size.
  //
  ExportedInfo = XmlDocumentExport (PlistDoc, &ExportedInfoSize, 0, FALSE);
  if (ExportedInfo == NULL) {
    return 0;
  }
  ExportedInfoSize++;

  if (OcOverflowAddU32 (Offset, ExportedInfoSize, &TmpSize)
    || TmpSize > AllocatedSize) {
    FreePool (ExportedInfo);
    return 0;
  }

  MkextBuffer = (UINT8 *) Mkext;
  CopyMem (&MkextBuffer[Offset], ExportedInfo, ExportedInfoSize);
  FreePool (ExportedInfo);

  Mkext->PlistOffset          = SwapBytes32 (Offset);
  Mkext->PlistFullSize        = SwapBytes32 (ExportedInfoSize);
  Mkext->PlistCompressedSize  = 0;
  return ExportedInfoSize;
}

STATIC
MKEXT_KEXT *
InsertCachedMkextKext (
  IN OUT MKEXT_CONTEXT      *Context,
  IN     CONST CHAR8        *Identifier,
  IN     UINT32             BinOffset,
  IN     UINT32             BinSize
  )
{
  MKEXT_KEXT      *MkextKext;

  MkextKext = AllocateZeroPool (sizeof (*MkextKext));
  if (MkextKext == NULL) {
    return NULL;
  }

  MkextKext->Signature    = MKEXT_KEXT_SIGNATURE;
  MkextKext->BinaryOffset = BinOffset;
  MkextKext->BinarySize   = BinSize;
  MkextKext->Identifier   = AllocateCopyPool (AsciiStrSize (Identifier), Identifier);
  if (MkextKext->Identifier == NULL) {
    FreePool (MkextKext);
    return NULL;
  }

  InsertTailList (&Context->CachedKexts, &MkextKext->Link);

  DEBUG ((DEBUG_VERBOSE, "OCAK: Inserted %a into mkext cache\n", Identifier));

  return MkextKext;
}

MKEXT_KEXT *
InternalCachedMkextKext (
  IN OUT MKEXT_CONTEXT      *Context,
  IN     CONST CHAR8        *Identifier
  )
{
  MKEXT_HEADER_ANY    *MkextHeader;
  MKEXT_V2_FILE_ENTRY *MkextV2FileEntry;

  MKEXT_KEXT          *MkextKext;
  LIST_ENTRY          *KextLink;
  UINT32              Index;
  UINT32              PlistOffsetSize;
  UINT32              BinOffsetSize;
  BOOLEAN             IsKextMatch;

  UINT32              PlistOffset;
  UINT32              PlistSize;
  CHAR8               *PlistBuffer;
  XML_DOCUMENT        *PlistXml;
  XML_NODE            *PlistRoot;

  UINT32              PlistBundlesCount;
  XML_NODE            *PlistBundle;
  UINT32              PlistBundleIndex;
  UINT32              PlistBundleCount;
  CONST CHAR8         *PlistBundleKey;
  XML_NODE            *PlistBundleKeyValue;

  CONST CHAR8         *KextIdentifier;
  UINT32              KextBinOffset;
  UINT32              KextBinSize;

  MkextHeader = Context->MkextHeader;

  //
  // Try to get cached kext.
  //
  KextLink = GetFirstNode (&Context->CachedKexts);
  while (!IsNull (&Context->CachedKexts, KextLink)) {
    MkextKext = GET_MKEXT_KEXT_FROM_LINK (KextLink);

    if (AsciiStrCmp (Identifier, MkextKext->Identifier) == 0) {
      return MkextKext;
    }

    KextLink = GetNextNode (&Context->CachedKexts, KextLink);
  }

  //
  // Search mkext and add kext to cache.
  //
  IsKextMatch = FALSE;
  
  //
  // Mkext v1.
  //
  if (Context->MkextVersion == MKEXT_VERSION_V1) {
    for (Index = 0; Index < Context->NumKexts; Index++) {
      //
      // Do not cache binaryless or compressed kexts.
      //
      if (MkextHeader->V1.Kexts[Index].Plist.CompressedSize != 0
        || MkextHeader->V1.Kexts[Index].Binary.CompressedSize != 0
        || MkextHeader->V1.Kexts[Index].Binary.Offset == 0) {
        continue;
      }

      PlistOffset   = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.Offset);
      PlistSize     = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.FullSize);
      KextBinOffset = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.Offset);
      KextBinSize   = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.FullSize);

      //
      // Verify plist and binary are within bounds.
      //
      if (OcOverflowAddU32 (PlistOffset, PlistSize, &PlistOffsetSize)
        || PlistOffsetSize > Context->MkextSize
        || OcOverflowAddU32 (KextBinOffset, KextBinSize, &BinOffsetSize)
        || BinOffsetSize > Context->MkextSize) {
        return NULL;
      }

      PlistBuffer = AllocateCopyPool (PlistSize, &Context->Mkext[PlistOffset]);
      if (PlistBuffer == NULL) {
        return NULL;
      }

      PlistXml = XmlDocumentParse (PlistBuffer, PlistSize, FALSE);
      if (PlistXml == NULL) {
        FreePool (PlistBuffer);
        return NULL;
      }

      PlistRoot = PlistNodeCast (PlistDocumentRoot (PlistXml), PLIST_NODE_TYPE_DICT);
      if (PlistRoot == NULL) {
        XmlDocumentFree (PlistXml);
        FreePool (PlistBuffer);
        return NULL;
      }

      KextIdentifier = NULL;
      PlistBundleCount = PlistDictChildren (PlistRoot);
      for (PlistBundleIndex = 0; PlistBundleIndex < PlistBundleCount; PlistBundleIndex++) {
        PlistBundleKey = PlistKeyValue (PlistDictChild (PlistRoot, PlistBundleIndex, &PlistBundleKeyValue));
        if (PlistBundleKey == NULL || PlistBundleKeyValue == NULL) {
          continue;
        }
        
        if (AsciiStrCmp (PlistBundleKey, INFO_BUNDLE_IDENTIFIER_KEY) == 0) {
          KextIdentifier = XmlNodeContent (PlistBundleKeyValue);
          break;
        }
      }

      IsKextMatch = KextIdentifier != NULL && AsciiStrCmp (KextIdentifier, Identifier) == 0;
      XmlDocumentFree (PlistXml);
      FreePool (PlistBuffer);

      if (IsKextMatch && KextBinOffset > 0 && KextBinSize > 0) {
        break;
      }
    }

    //
    // Bundle was not found, or invalid.
    //
    if (!IsKextMatch) {
      return NULL;
    }

  //
  // Mkext v2.
  //
  } else if (Context->MkextVersion == MKEXT_VERSION_V2) {
    KextIdentifier  = NULL;
    KextBinOffset   = 0;

    //
    // Enumerate bundle dicts.
    //
    PlistBundlesCount = XmlNodeChildren (Context->MkextKexts);
    for (Index = 0; Index < PlistBundlesCount; Index++) {
      PlistBundle = PlistNodeCast (XmlNodeChild (Context->MkextKexts, Index), PLIST_NODE_TYPE_DICT);
      if (PlistBundle == NULL) {
        return NULL;
      }

      PlistBundleCount = PlistDictChildren (PlistBundle);
      for (PlistBundleIndex = 0; PlistBundleIndex < PlistBundleCount; PlistBundleIndex++) {
        PlistBundleKey = PlistKeyValue (PlistDictChild (PlistBundle, PlistBundleIndex, &PlistBundleKeyValue));
        if (PlistBundleKey == NULL || PlistBundleKeyValue == NULL) {
          continue;
        }
        
        if (AsciiStrCmp (PlistBundleKey, INFO_BUNDLE_IDENTIFIER_KEY) == 0) {
          KextIdentifier = XmlNodeContent (PlistBundleKeyValue);
        }

        if (AsciiStrCmp (PlistBundleKey, MKEXT_EXECUTABLE_KEY) == 0) {
          //
          // Ensure binary offset is before plist offset.
          //
          if (!PlistIntegerValue (PlistBundleKeyValue, &KextBinOffset, sizeof (KextBinOffset), TRUE)) {
            return NULL;
          }
        }
      }

      if (KextIdentifier != NULL
        && AsciiStrCmp (KextIdentifier, Identifier) == 0
        && KextBinOffset > 0
        && KextBinOffset < Context->MkextSize - sizeof (MKEXT_V2_FILE_ENTRY)) {
        IsKextMatch = TRUE;
        break;
      }

      KextIdentifier  = NULL;
      KextBinOffset   = 0;
    }

    //
    // Bundle was not found, or invalid.
    //
    if (!IsKextMatch) {
      return NULL;
    }

    //
    // Parse v2 binary header.
    // We cannot support compressed binaries.
    //
    MkextV2FileEntry = (MKEXT_V2_FILE_ENTRY *) &Context->Mkext[KextBinOffset];
    if (MkextV2FileEntry->CompressedSize != 0) {
      return NULL;
    }
    KextBinOffset += OFFSET_OF (MKEXT_V2_FILE_ENTRY, Data);
    KextBinSize   = SwapBytes32 (MkextV2FileEntry->FullSize);

    //
    // Ensure binary is within mkext bounds.
    //
    if (OcOverflowAddU32 (KextBinOffset, KextBinSize, &BinOffsetSize)
      || BinOffsetSize > Context->MkextSize) {
      return NULL;
    }

  //
  // Unsupported version.
  //
  } else {
    return NULL;
  }

  return InsertCachedMkextKext (Context, Identifier, KextBinOffset, KextBinSize);
}

EFI_STATUS
MkextDecompress (
  IN     CONST UINT8      *Buffer,
  IN     UINT32           BufferSize,
  IN     UINT32           NumReservedKexts,
  IN OUT UINT8            *OutBuffer OPTIONAL,
  IN     UINT32           OutBufferSize OPTIONAL,
  IN OUT UINT32           *OutMkextSize
  )
{
  BOOLEAN               Decompress;

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
  UINT32                PlistFullSizeAligned;
  UINT32                BinOffset;
  UINT32                BinCompSize;
  UINT32                BinFullSize;
  UINT32                BinFullSizeAligned;

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

  Decompress = OutBufferSize > 0;
  if (Decompress) {
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

  MkextHeaderOut = NULL;

  //
  // Mkext v1.
  //
  if (MkextVersion == MKEXT_VERSION_V1) {
    //
    // Validate header and array size.
    // We need to start our offset after the header including reserved kext slots.
    //
    if (OcOverflowMulAddU32 (sizeof (MKEXT_V1_KEXT), NumKexts, sizeof (MKEXT_V1_HEADER), &Tmp)
      || Tmp > MkextSize
      || OcOverflowMulAddU32 (sizeof (MKEXT_V1_KEXT), NumMaxKexts, sizeof (MKEXT_V1_HEADER), &CurrentOffset)
      || MKEXT_ALIGN (CurrentOffset) < CurrentOffset) {
      return EFI_INVALID_PARAMETER;
    }
    CurrentOffset = MKEXT_ALIGN (CurrentOffset);

    if (Decompress) {
      //
      // When decompressing, CurrentOffset needs to be within bounds of OutBufferSize.
      // If not decompressing, this is unnecessary as we are calculating decompressed size only.
      //
      if (CurrentOffset > OutBufferSize) {
        return EFI_BUFFER_TOO_SMALL;
      }
      
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
      PlistFullSize         = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.FullSize);
      BinFullSize           = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.FullSize);
      PlistFullSizeAligned  = MKEXT_ALIGN (PlistFullSize);
      BinFullSizeAligned    = MKEXT_ALIGN (BinFullSize);

      if (PlistFullSizeAligned < PlistFullSize
        || BinFullSizeAligned < BinFullSize) {
        return EFI_INVALID_PARAMETER;
      }

      if (Decompress) {
        //
        // Decompress entry.
        //
        PlistOffset     = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.Offset);
        PlistCompSize   = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.CompressedSize);
        BinOffset       = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.Offset);
        BinCompSize     = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.CompressedSize);

        if (OcOverflowTriAddU32 (CurrentOffset, PlistFullSizeAligned, BinFullSizeAligned, &Tmp)) {
          return EFI_INVALID_PARAMETER;
        }
        if (Tmp > OutBufferSize) {
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
              &((UINT8 *) Buffer)[PlistOffset],
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
        CurrentOffset                                         += PlistFullSizeAligned;

        if (BinFullSize > 0) {
          //
          // Compressed size == 0 means no compression.
          //
          if (BinCompSize > 0) {
            if (OcOverflowAddU32 (BinOffset, BinCompSize, &Tmp)
              || Tmp > MkextSize
              || DecompressLZSS (
                &OutBuffer[CurrentOffset],
                BinFullSize,
                &((UINT8 *) Buffer)[BinOffset],
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
          CurrentOffset                                           += BinFullSizeAligned;
        } else {
          ZeroMem (&MkextHeaderOut->V1.Kexts[Index].Binary, sizeof (MKEXT_V1_KEXT_FILE));
        }

      } else {
        //
        // Calculate size only.
        //
        if (OcOverflowTriAddU32 (
          CurrentOffset,
          PlistFullSizeAligned,
          BinFullSizeAligned,
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
    if (MkextSize < sizeof (MKEXT_V2_HEADER)) {
      return EFI_INVALID_PARAMETER;
    }
    CurrentOffset = MKEXT_ALIGN (sizeof (MKEXT_V2_HEADER));

    if (Decompress) {
      //
      // When decompressing, CurrentOffset needs to be within bounds of OutBufferSize.
      // If not decompressing, this is unnecessary as we are calculating decompressed size only.
      //
      if (CurrentOffset > OutBufferSize) {
        return EFI_BUFFER_TOO_SMALL;
      }

      //
      // Copy header.
      //
      CopyMem (OutBuffer, Buffer, sizeof (MKEXT_V2_HEADER));
      MkextHeaderOut = (MKEXT_HEADER_ANY *) OutBuffer;
    }

    if (!ParseMkextV2Plist (&MkextHeader->V2, &PlistBuffer, &PlistFullSize, &PlistXml, &PlistBundles)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // All offset strings are kept in array until the plist is regenerated.
    //
    PlistBundlesCount   = XmlNodeChildren (PlistBundles);
    BinaryOffsetStrings = NULL;

    if (Decompress) {
      if (OcOverflowTriMulU32 (PlistBundlesCount, KEXT_OFFSET_STR_LEN, sizeof (CHAR8), &BinaryOffsetStringsSize)) {
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
        if (Decompress) {
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
        if (PlistBundleKey == NULL) {
          continue;
        }

        if (AsciiStrCmp (PlistBundleKey, MKEXT_EXECUTABLE_KEY) == 0) {
          if (!PlistIntegerValue (BundleExecutable, &BinOffset, sizeof (BinOffset), TRUE)
            || BinOffset == 0
            || BinOffset > MkextSize - sizeof (MKEXT_V2_FILE_ENTRY)) {
            XmlDocumentFree (PlistXml);
            FreePool (PlistBuffer);
            if (Decompress) {
              FreePool (BinaryOffsetStrings);
            }
            return EFI_INVALID_PARAMETER;
          }

          MkextExecutableEntry  = (MKEXT_V2_FILE_ENTRY *) &Buffer[BinOffset];
          BinCompSize           = SwapBytes32 (MkextExecutableEntry->CompressedSize);
          BinFullSize           = SwapBytes32 (MkextExecutableEntry->FullSize);
          BinFullSizeAligned    = MKEXT_ALIGN (BinFullSize);

          if (OcOverflowTriAddU32 (CurrentOffset, sizeof (MKEXT_V2_FILE_ENTRY), BinFullSizeAligned, &NewOffset)) {
            XmlDocumentFree (PlistXml);
            FreePool (PlistBuffer);
            if (Decompress) {
              FreePool (BinaryOffsetStrings);
            }
            return EFI_INVALID_PARAMETER;
          }

          if (Decompress) {
            if (NewOffset > OutBufferSize) {
              XmlDocumentFree (PlistXml);
              FreePool (PlistBuffer);
              FreePool (BinaryOffsetStrings);
              return EFI_BUFFER_TOO_SMALL;
            }

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
              &BinaryOffsetStrings[Index * KEXT_OFFSET_STR_LEN],
              KEXT_OFFSET_STR_LEN,
              CurrentOffset
              )) {
              XmlDocumentFree (PlistXml);
              FreePool (PlistBuffer);
              FreePool (BinaryOffsetStrings);
              return EFI_INVALID_PARAMETER;
            }
            XmlNodeChangeContent (BundleExecutable, &BinaryOffsetStrings[Index * KEXT_OFFSET_STR_LEN]);
          }

          //
          // Move to next bundle.
          //
          CurrentOffset = NewOffset;
          break;
        }
      }
    }

    if (Decompress) {
      PlistFullSize = UpdateMkextV2Plist (&MkextHeaderOut->V2, OutBufferSize, PlistXml, CurrentOffset);
    }
    XmlDocumentFree (PlistXml);
    FreePool (PlistBuffer);
    if (Decompress) {
      FreePool (BinaryOffsetStrings);

      if (PlistFullSize == 0
        || OcOverflowAddU32 (CurrentOffset, PlistFullSize, OutMkextSize)) {
        return EFI_INVALID_PARAMETER;
      }
    } else {
      //
      // Account for plist, future plist expansion for each bundle, 
      //   and additional headers for future kext injection.
      //
      PlistFullSize = SwapBytes32 (MkextHeader->V2.PlistFullSize);
      if (OcOverflowAddU32 (CurrentOffset, PlistFullSize, &CurrentOffset)
        || OcOverflowMulAddU32 (PlistBundlesCount, PLIST_EXPANSION_SIZE, CurrentOffset, &CurrentOffset)
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

  if (Decompress) {
    UpdateMkextLengthChecksum (MkextHeaderOut, *OutMkextSize);
  }
  return EFI_SUCCESS;
}

BOOLEAN
MkextCheckCpuType (
  IN UINT8            *Mkext,
  IN UINT32           MkextSize,
  IN MACH_CPU_TYPE    CpuType
  )
{
  MKEXT_HEADER_ANY    *MkextHeader;
  MACH_CPU_TYPE       MkextCpuType;

  ASSERT (Mkext != NULL);
  ASSERT (MkextSize > 0);

  if (MkextSize < sizeof (MKEXT_CORE_HEADER)
    || !OC_TYPE_ALIGNED (MKEXT_CORE_HEADER, Mkext)) {
    return FALSE;
  }

  MkextHeader   = (MKEXT_HEADER_ANY *) Mkext;
  MkextCpuType  = SwapBytes32 (MkextHeader->Common.CpuType);

  if (MkextHeader->Common.Magic != MKEXT_INVERT_MAGIC
    || MkextHeader->Common.Signature != MKEXT_INVERT_SIGNATURE
    || MkextSize != SwapBytes32 (MkextHeader->Common.Length)) {
    return FALSE;
  }

  return MkextCpuType == CpuType;
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
  UINT32              MkextHeaderSize;
  MACH_CPU_TYPE       CpuType;
  BOOLEAN             Is32Bit;
  UINT32              NumKexts;
  UINT32              NumMaxKexts;

  UINT32              Tmp;
  UINT32              Index;
  UINT32              StartingOffset;
  UINT32              CurrentOffset;

  UINT8               *PlistBuffer;
  XML_DOCUMENT        *PlistXml;
  UINT32              PlistOffset;
  UINT32              PlistFullSize;
  XML_NODE            *PlistBundles;

  //
  // Assumptions:
  //    Kexts are aligned to 8 bytes.
  //    Patching or blocking requires kexts to be decompressed.
  //    Plist (for v2) is at end of mkext.
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
    Is32Bit = TRUE;
  } else if (CpuType == MachCpuTypeX8664) {
    Is32Bit = FALSE;
  } else {
    return EFI_UNSUPPORTED;
  }

  //
  // Mkext v1.
  //
  if (MkextVersion == MKEXT_VERSION_V1) {
    //
    // Validate header and array size.
    //
    if (OcOverflowMulAddU32 (sizeof (MKEXT_V1_KEXT), NumKexts, sizeof (MKEXT_V1_HEADER), &MkextHeaderSize)
      || MkextHeaderSize > MkextSize) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Calculate available kext slots. This value is assumed to be under the UINT32 max later on.
    //
    // Below finds the lowest offset to a plist or a binary, which is used to locate
    // the end of the usable space allocated for kext slots.
    //
    StartingOffset = 0;
    for (Index = 0; Index < NumKexts; Index++) {
      CurrentOffset = SwapBytes32 (MkextHeader->V1.Kexts[Index].Plist.Offset);
      if (StartingOffset == 0 || CurrentOffset < StartingOffset) {
        StartingOffset = CurrentOffset;
      }

      //
      // A binary entry of zero size indicates no binary is present.
      //
      if (MkextHeader->V1.Kexts[Index].Binary.FullSize > 0) {
        CurrentOffset = SwapBytes32 (MkextHeader->V1.Kexts[Index].Binary.Offset);
        if (CurrentOffset < StartingOffset) {
          StartingOffset = CurrentOffset;
        }
      }
    }

    if (StartingOffset < MkextHeaderSize
      || StartingOffset > MkextSize) {
      return EFI_INVALID_PARAMETER;
    }

    Tmp = (StartingOffset - MkextHeaderSize) / sizeof (MKEXT_V1_KEXT);
    if (OcOverflowAddU32 (Tmp, NumKexts, &NumMaxKexts)
      || NumMaxKexts == MAX_UINT32) {
      return EFI_INVALID_PARAMETER;
    }
    
  //
  // Mkext v2.
  //
  } else if (MkextVersion == MKEXT_VERSION_V2) {
    if (MkextSize < sizeof (MKEXT_V2_HEADER)
      || !ParseMkextV2Plist (&MkextHeader->V2, &PlistBuffer, &PlistFullSize, &PlistXml, &PlistBundles)) {
      return EFI_INVALID_PARAMETER;
    }
    PlistOffset = SwapBytes32 (MkextHeader->V2.PlistOffset);

    if (OcOverflowAddU32 (PlistOffset, PlistFullSize, &Tmp)
      || Tmp != MkextSize) {
      return EFI_INVALID_PARAMETER;
    }

  //
  // Unsupported version.
  //
  } else {
    return EFI_UNSUPPORTED;
  }

  ZeroMem (Context, sizeof (*Context));
  Context->Mkext                = Mkext;
  Context->MkextSize            = MkextSize;
  Context->MkextHeader          = MkextHeader;
  Context->MkextAllocSize       = MkextAllocSize;
  Context->MkextVersion         = MkextVersion;
  Context->Is32Bit              = Is32Bit;
  Context->NumKexts             = NumKexts;
  InitializeListHead (&Context->CachedKexts);

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

VOID
MkextContextFree (
  IN OUT MKEXT_CONTEXT      *Context
  )
{
  MKEXT_KEXT          *MkextKext;
  LIST_ENTRY          *KextLink;

  ASSERT (Context != NULL);

  while (!IsListEmpty (&Context->CachedKexts)) {
    KextLink = GetFirstNode (&Context->CachedKexts);
    MkextKext = GET_MKEXT_KEXT_FROM_LINK (KextLink);
    RemoveEntryList (KextLink);

    FreePool (MkextKext->Identifier);
    FreePool (MkextKext);
  }

  if (Context->MkextInfoDocument != NULL) {
    XmlDocumentFree (Context->MkextInfoDocument);
  }
  if (Context->MkextInfo != NULL) {
    FreePool (Context->MkextInfo);
  }

  ZeroMem (Context, sizeof (*Context));
}

EFI_STATUS
MkextReserveKextSize (
  IN OUT UINT32       *ReservedInfoSize,
  IN OUT UINT32       *ReservedExeSize,
  IN     UINT32       InfoPlistSize,
  IN     UINT8        *Executable OPTIONAL,
  IN     UINT32       ExecutableSize OPTIONAL,
  IN     BOOLEAN      Is32Bit
  )
{
  OC_MACHO_CONTEXT  Context;

  ASSERT (ReservedInfoSize != NULL);
  ASSERT (ReservedExeSize != NULL);

  InfoPlistSize = MACHO_ALIGN (InfoPlistSize);

  if (Executable != NULL) {
    ASSERT (ExecutableSize > 0);
    if (!MachoInitializeContext (&Context, Executable, ExecutableSize, 0, Is32Bit)) {
      return EFI_INVALID_PARAMETER;
    }

    ExecutableSize = MachoGetVmSize (&Context);
    if (ExecutableSize == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (OcOverflowAddU32 (*ReservedInfoSize, InfoPlistSize, &InfoPlistSize)
   || OcOverflowAddU32 (*ReservedExeSize, ExecutableSize, &ExecutableSize)) {
    return EFI_INVALID_PARAMETER;
  }

  *ReservedInfoSize = InfoPlistSize;
  *ReservedExeSize  = ExecutableSize;

  return EFI_SUCCESS;
}

EFI_STATUS
MkextInjectKext (
  IN OUT MKEXT_CONTEXT      *Context,
  IN     CONST CHAR8        *Identifier OPTIONAL,
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
  UINT32                InfoPlistSizeAligned;
  UINT32                ExecutableSizeAligned;

  CHAR8                 *PlistBuffer;
  CHAR8                 *PlistExported;
  UINT32                PlistExportedSize;
  XML_DOCUMENT          *PlistXml;
  XML_NODE              *PlistRoot;
  BOOLEAN               PlistFailed;
  UINT32                PlistBundleIndex;
  UINT32                PlistBundleCount;
  CONST CHAR8           *PlistBundleKey;
  XML_NODE              *PlistBundleKeyValue;
  
  CONST CHAR8           *TmpKeyValue;
  UINT32                FieldCount;
  UINT32                FieldIndex;

  CHAR8                 ExecutableSourceAddrStr[24];
  MKEXT_V2_FILE_ENTRY   *MkextExecutableEntry;

  ASSERT (Context != NULL);
  ASSERT (BundlePath != NULL);
  ASSERT (InfoPlist != NULL);
  ASSERT (InfoPlistSize > 0);

  BinOffset = 0;

  //
  // If an identifier was passed, ensure it does not already exist.
  //
  if (Identifier != NULL) {
    if (InternalCachedMkextKext (Context, Identifier) != NULL) {
      DEBUG ((DEBUG_INFO, "OCAK: Bundle %a is already present in mkext\n", Identifier));
      return EFI_ALREADY_STARTED;
    }
  }

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

  Identifier = NULL;
  PlistBundleCount = PlistDictChildren (PlistRoot);
  for (PlistBundleIndex = 0; PlistBundleIndex < PlistBundleCount; PlistBundleIndex++) {
    PlistBundleKey = PlistKeyValue (PlistDictChild (PlistRoot, PlistBundleIndex, &PlistBundleKeyValue));
    if (PlistBundleKey == NULL || PlistBundleKeyValue == NULL) {
      continue;
    }
    
    if (AsciiStrCmp (PlistBundleKey, INFO_BUNDLE_IDENTIFIER_KEY) == 0) {
      Identifier = XmlNodeContent (PlistBundleKeyValue);
      break;
    }
  }

  if (Identifier == NULL) {
    XmlDocumentFree (PlistXml);
    FreePool (PlistBuffer);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Mkext v1.
  //
  if (Context->MkextVersion == MKEXT_VERSION_V1) {
    XmlDocumentFree (PlistXml);
    FreePool (PlistBuffer);

    if (Context->NumKexts >= Context->NumMaxKexts) {
      return EFI_BUFFER_TOO_SMALL;
    }

    InfoPlistSizeAligned = MKEXT_ALIGN (InfoPlistSize);
    if (InfoPlistSizeAligned < InfoPlistSize) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Plist will be placed at end of mkext.
    //
    PlistOffset = Context->MkextSize;
    if (OcOverflowAddU32 (PlistOffset, InfoPlistSizeAligned, &MkextNewSize)) {
      return EFI_INVALID_PARAMETER;
    }
    if (MkextNewSize > Context->MkextAllocSize) {
      return EFI_BUFFER_TOO_SMALL;
    }

    //
    // Executable, if present, will be placed right after plist.
    //
    if (Executable != NULL) {
      ASSERT (ExecutableSize > 0);

      BinOffset = MkextNewSize;
      if (!ParseKextBinary (&Executable, &ExecutableSize, Context->Is32Bit)) {
        return EFI_INVALID_PARAMETER;
      }

      ExecutableSizeAligned = MKEXT_ALIGN (ExecutableSize);
      if (ExecutableSizeAligned < ExecutableSize
        || OcOverflowAddU32 (BinOffset, ExecutableSizeAligned, &MkextNewSize)) {
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
    //
    // Executable, if present, will be placed at end of mkext and plist will be moved further out.
    //
    PlistOffset = Context->MkextInfoOffset;
    PlistFailed = FALSE;
    if (Executable != NULL) {
      ASSERT (ExecutableSize > 0);
      
      BinOffset = PlistOffset;
      if (!ParseKextBinary (&Executable, &ExecutableSize, Context->Is32Bit)) {
        XmlDocumentFree (PlistXml);
        FreePool (PlistBuffer);
        return EFI_INVALID_PARAMETER;
      }
      ExecutableSizeAligned = MKEXT_ALIGN (ExecutableSize);
      if (ExecutableSizeAligned < ExecutableSize
        || OcOverflowTriAddU32 (BinOffset, sizeof (MKEXT_V2_FILE_ENTRY), ExecutableSizeAligned, &PlistOffset)) {
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

      BinOffset += OFFSET_OF (MKEXT_V2_FILE_ENTRY, Data);
    }
  
  //
  // Unsupported version.
  //
  } else {
    XmlDocumentFree (PlistXml);
    FreePool (PlistBuffer);
    return EFI_UNSUPPORTED;
  }

  //
  // Add kext to cache.
  //
  InsertCachedMkextKext (Context, Identifier, BinOffset, ExecutableSize);

  return EFI_SUCCESS;
}

EFI_STATUS
MkextContextApplyPatch (
  IN OUT MKEXT_CONTEXT          *Context,
  IN     CONST CHAR8            *Identifier,
  IN     PATCHER_GENERIC_PATCH  *Patch
  )
{
  EFI_STATUS            Status;
  PATCHER_CONTEXT       Patcher;

  ASSERT (Context != NULL);
  ASSERT (Identifier != NULL);
  ASSERT (Patch != NULL);

  Status = PatcherInitContextFromMkext (&Patcher, Context, Identifier);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to mkext find %a - %r\n", Identifier, Status));
    return Status;
  }

  return PatcherApplyGenericPatch (&Patcher, Patch);
}

EFI_STATUS
MkextContextApplyQuirk (
  IN OUT MKEXT_CONTEXT        *Context,
  IN     KERNEL_QUIRK_NAME    Quirk,
  IN     UINT32               KernelVersion
  )
{
  EFI_STATUS            Status;
  KERNEL_QUIRK          *KernelQuirk;
  PATCHER_CONTEXT       Patcher;

  ASSERT (Context != NULL);

  KernelQuirk = &gKernelQuirks[Quirk];
  ASSERT (KernelQuirk->Identifier != NULL);

  Status = PatcherInitContextFromMkext (&Patcher, Context, KernelQuirk->Identifier);
  if (!EFI_ERROR (Status)) {
    return KernelQuirk->PatchFunction (&Patcher, KernelVersion);
  }

  //
  // It is up to the function to decide whether this is critical or not.
  //
  DEBUG ((DEBUG_INFO, "OCAK: Failed to mkext find %a - %r\n", KernelQuirk->Identifier, Status));
  return KernelQuirk->PatchFunction (NULL, KernelVersion);
}

EFI_STATUS
MkextContextBlock (
  IN OUT MKEXT_CONTEXT          *Context,
  IN     CONST CHAR8            *Identifier
  )
{
  EFI_STATUS            Status;
  PATCHER_CONTEXT       Patcher;

  ASSERT (Context != NULL);
  ASSERT (Identifier != NULL);

  Status = PatcherInitContextFromMkext (&Patcher, Context, Identifier);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to mkext find %a - %r\n", Identifier, Status));
    return Status;
  }

  return PatcherBlockKext (&Patcher);
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
    MkextPlistSize = UpdateMkextV2Plist (
      &Context->MkextHeader->V2,
      Context->MkextAllocSize,
      Context->MkextInfoDocument,
      Context->MkextInfoOffset
      );
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
