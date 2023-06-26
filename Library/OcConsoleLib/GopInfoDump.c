/** @file
  Dump GOP info for current/default mode, including memory caching info from MTRR and PAT.

  Copyright (C) 2023, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "OcConsoleLibInternal.h"

#include <Base.h>
#include <Uefi/UefiBaseType.h>

#include <IndustryStandard/ProcessorInfo.h>
#include <IndustryStandard/VirtualMemory.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MtrrLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS
OcGopInfoDump (
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  EFI_STATUS                      Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *Gop;
  MTRR_MEMORY_CACHE_TYPE          MtrrCacheType;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageTable;
  EFI_VIRTUAL_ADDRESS             VirtualAddress;
  EFI_PHYSICAL_ADDRESS            PhysicalAddress;
  EFI_PHYSICAL_ADDRESS            EndAddress;
  UINTN                           FrameBufferSize;
  UINTN                           BytesPerPixel;
  UINT64                          Bits;
  UINT8                           Level;
  BOOLEAN                         HasMtrr;
  BOOLEAN                         HasPat;
  UINT64                          PatMsr;
  PAT_INDEX                       PatIndex;
  BOOLEAN                         HasDefaultPat;

  CHAR8   *FileBuffer;
  UINTN   FileBufferSize;
  CHAR16  TmpFileName[32];

  ASSERT (Root != NULL);

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&Gop
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCC: No console GOP found for dumping - %r\n", Status));
    return Status;
  }

  FileBufferSize = SIZE_1KB;
  FileBuffer     = AllocateZeroPool (FileBufferSize);
  if (FileBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_SUCCESS;

  if ((Gop->Mode == NULL) || (Gop->Mode->Info == NULL)) {
    OcAsciiPrintBuffer (
      &FileBuffer,
      &FileBufferSize,
      "INVALID GOP %p %p!\n",
      Gop->Mode,
      (Gop->Mode != NULL) ? Gop->Mode->Info : NULL
      );
    Status = EFI_UNSUPPORTED;
  }

  if (!EFI_ERROR (Status)) {
    OcAsciiPrintBuffer (
      &FileBuffer,
      &FileBufferSize,
      "GOP INFO %u(%u)x%u fmt %u\n",
      Gop->Mode->Info->HorizontalResolution,
      Gop->Mode->Info->PixelsPerScanLine,
      Gop->Mode->Info->VerticalResolution,
      Gop->Mode->Info->PixelFormat
      );

    if (Gop->Mode->Info->PixelFormat == PixelBitMask) {
      OcAsciiPrintBuffer (
        &FileBuffer,
        &FileBufferSize,
        "GOP pix mask 0x%X:0x%X:0x%X:0x%X\n",
        Gop->Mode->Info->PixelInformation.RedMask,
        Gop->Mode->Info->PixelInformation.GreenMask,
        Gop->Mode->Info->PixelInformation.BlueMask,
        Gop->Mode->Info->PixelInformation.ReservedMask
        );
    }

    OcAsciiPrintBuffer (
      &FileBuffer,
      &FileBufferSize,
      "GOP FB 0x%LX[0x%X]\n",
      Gop->Mode->FrameBufferBase,
      Gop->Mode->FrameBufferSize
      );

    Status = OcGopModeBytesPerPixel (Gop->Mode, &BytesPerPixel);
    OcAsciiPrintBuffer (
      &FileBuffer,
      &FileBufferSize,
      "GOP BPP %u - %r\n",
      BytesPerPixel,
      Status
      );

    if (!EFI_ERROR (Status)) {
      Status = OcGopModeSafeFrameBufferSize (Gop->Mode, &FrameBufferSize);
      OcAsciiPrintBuffer (
        &FileBuffer,
        &FileBufferSize,
        "GOP FBS 0x%X%a - %r\n",
        FrameBufferSize,
        (Gop->Mode->FrameBufferSize != FrameBufferSize) ? " (FBS mismatch! Using this.)" : "",
        Status
        );
    }
  }

  if (!EFI_ERROR (Status)) {
    HasMtrr = IsMtrrSupported ();
    HasPat  = OcIsPatSupported ();

    OcAsciiPrintBuffer (
      &FileBuffer,
      &FileBufferSize,
      "MTRR %asupported PAT %asupported\n",
      HasMtrr ? "" : "not ",
      HasPat ? "" : "not "
      );

    if (HasMtrr) {
      if (HasPat) {
        PatMsr        = AsmReadMsr64 (MSR_IA32_CR_PAT);
        HasDefaultPat = (PatMsr == PAT_DEFAULTS);

        OcAsciiPrintBuffer (
          &FileBuffer,
          &FileBufferSize,
          "PAT 0x%016LX (%a)\n",
          PatMsr,
          HasDefaultPat ? "default" : "not default!"
          );
      }

      PageTable = OcGetCurrentPageTable (NULL);

      VirtualAddress = Gop->Mode->FrameBufferBase;
      EndAddress     = VirtualAddress + FrameBufferSize;

      Status = EFI_SUCCESS;

      do {
        MtrrCacheType = MtrrGetMemoryAttribute (VirtualAddress);

        if (HasPat) {
          Status = OcGetSetPageTableInfoForAddress (
                     PageTable,
                     VirtualAddress,
                     &PhysicalAddress,
                     &Level,
                     &Bits,
                     &PatIndex,
                     FALSE
                     );

          if (EFI_ERROR (Status)) {
            break;
          }

          OcAsciiPrintBuffer (
            &FileBuffer,
            &FileBufferSize,
            "0x%LX~0x%LX MTRR %u=%a PTE%u bits 0x%016LX PAT@%u->%u=%a\n",
            VirtualAddress,
            PhysicalAddress,
            MtrrCacheType,
            OcGetMtrrTypeString (MtrrCacheType),
            Level,
            Bits,
            PatIndex.Index,
            GET_PAT_N (PatMsr, PatIndex.Index),
            OcGetPatTypeString (GET_PAT_N (PatMsr, PatIndex.Index))
            );

          if (VirtualAddress != PhysicalAddress) {
            Status = EFI_UNSUPPORTED;
            break;
          }
        } else {
          Level = 2;

          OcAsciiPrintBuffer (
            &FileBuffer,
            &FileBufferSize,
            "0x%LX MTRR %u=%a\n",
            VirtualAddress,
            MtrrCacheType,
            OcGetMtrrTypeString (MtrrCacheType)
            );
        }

        switch (Level) {
          case 1:
            VirtualAddress += SIZE_1GB;
            break;

          case 2:
            VirtualAddress += SIZE_2MB;
            break;

          case 4:
            VirtualAddress += SIZE_4KB;
            break;

          default:
            Status = EFI_UNSUPPORTED;
            break;
        }
      } while (!EFI_ERROR (Status) && VirtualAddress < EndAddress);
    }

    if (EFI_ERROR (Status)) {
      OcAsciiPrintBuffer (
        &FileBuffer,
        &FileBufferSize,
        "Failure reading page table! - %r\n",
        Status
        );
    }
  }

  //
  // Save dumped GOP info to file.
  //
  if (FileBuffer != NULL) {
    UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"GOPInfo.txt");
    Status = OcSetFileData (Root, TmpFileName, FileBuffer, (UINT32)AsciiStrLen (FileBuffer));
    DEBUG ((DEBUG_INFO, "OCC: Dumped GOP cache info - %r\n", Status));

    FreePool (FileBuffer);
  }

  return EFI_SUCCESS;
}
