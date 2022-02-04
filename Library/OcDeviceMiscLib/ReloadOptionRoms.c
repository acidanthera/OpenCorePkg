/** @file
  Copyright (c) 2020 Dayo Akanji (sf.net/u/dakanji/profile).
  Portions Copyright (c) 2020 Joe van Tunen (joevt@shaw.ca).
  Portions Copyright (c) 2004-2008 The Intel Corporation.
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDeviceMiscLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <IndustryStandard/Pci23.h>
#include <IndustryStandard/PeCoffImage.h>
#include <Protocol/Decompress.h>
#include <Protocol/PciIo.h>

#include "HandleParsingMin.h"

/**
  @param[in] RomBar       The Rom Base address.
  @param[in] RomSize      The Rom size.
  @param[in] FileName     The file name.

  @retval EFI_SUCCESS             The command completed successfully.
  @retval EFI_INVALID_PARAMETER   Command usage error.
  @retval EFI_UNSUPPORTED         Protocols unsupported.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
  @retval EFI_VOLUME_CORRUPTED    Inconsistent signatures.
  @retval Other value             Unknown error.
**/
STATIC
EFI_STATUS
ReloadPciRom (
  IN VOID          *RomBar,
  IN UINTN         RomSize,
  IN CONST CHAR16  *FileName
  )
{
  VOID                          *ImageBuffer;
  VOID                          *DecompressedImageBuffer;
  UINTN                         ImageIndex;
  UINTN                         RomBarOffset;
  UINT8                         *Scratch;
  UINT16                        ImageOffset;
  UINT32                        ImageSize;
  UINT32                        ScratchSize;
  UINT32                        ImageLength;
  UINT32                        DestinationSize;
  UINT32                        InitializationSize;
  BOOLEAN                       LoadROM;
  EFI_STATUS                    Status;
  EFI_HANDLE                    ImageHandle;
  PCI_DATA_STRUCTURE            *Pcir;
  EFI_DECOMPRESS_PROTOCOL       *Decompress;
  EFI_DEVICE_PATH_PROTOCOL      *FilePath;
  EFI_PCI_EXPANSION_ROM_HEADER  *EfiRomHeader;
  CHAR16                        RomFileName[32];

  ImageIndex    = 0;
  Status        = EFI_NOT_FOUND;
  RomBarOffset  = (UINTN) RomBar;

  do {
    LoadROM      = FALSE;
    EfiRomHeader = (EFI_PCI_EXPANSION_ROM_HEADER *) (UINTN) RomBarOffset;

    if (EfiRomHeader->Signature != PCI_EXPANSION_ROM_HEADER_SIGNATURE) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // If the pointer to the PCI Data Structure is invalid, no further images can be located.
    // The PCI Data Structure must be DWORD aligned.
    //
    if (EfiRomHeader->PcirOffset == 0
      || (EfiRomHeader->PcirOffset & 3) != 0
      || RomBarOffset - (UINTN) RomBar + EfiRomHeader->PcirOffset + sizeof (PCI_DATA_STRUCTURE) > RomSize) {
      break;
    }

    Pcir = (PCI_DATA_STRUCTURE *) (UINTN) (RomBarOffset + EfiRomHeader->PcirOffset);

    //
    // If a valid signature is not present in the PCI Data Structure, no further images can be located.
    //
    if (Pcir->Signature != PCI_DATA_STRUCTURE_SIGNATURE) {
      break;
    }

    ImageSize = Pcir->ImageLength * 512;

    if (RomBarOffset - (UINTN) RomBar + ImageSize > RomSize) {
      break;
    }

    if (Pcir->CodeType == PCI_CODE_TYPE_EFI_IMAGE
      && EfiRomHeader->EfiSignature  == EFI_PCI_EXPANSION_ROM_HEADER_EFISIGNATURE
      && (EfiRomHeader->EfiSubsystem == EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER
        || EfiRomHeader->EfiSubsystem  == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER)) {
      ImageOffset         = EfiRomHeader->EfiImageHeaderOffset;
      InitializationSize  = EfiRomHeader->InitializationSize * 512;

      if (InitializationSize <= ImageSize && ImageOffset < InitializationSize) {
        ImageBuffer             = (VOID *) (UINTN) (RomBarOffset + ImageOffset);
        ImageLength             = InitializationSize - ImageOffset;
        DecompressedImageBuffer = NULL;

        if (EfiRomHeader->CompressionType == EFI_PCI_EXPANSION_ROM_HEADER_COMPRESSED) {
          Status = gBS->LocateProtocol (
            &gEfiDecompressProtocolGuid,
            NULL,
            (VOID**) &Decompress
            );

          if (!EFI_ERROR (Status)) {
            Status = Decompress->GetInfo (
              Decompress,
              ImageBuffer,
              ImageLength,
              &DestinationSize,
              &ScratchSize
              );

            if (!EFI_ERROR (Status)) {
              DecompressedImageBuffer = AllocateZeroPool (DestinationSize);
              if (ImageBuffer != NULL) {
                Scratch = AllocateZeroPool (ScratchSize);
                if (Scratch != NULL) {
                  Status = Decompress->Decompress (
                    Decompress,
                    ImageBuffer,
                    ImageLength,
                    DecompressedImageBuffer,
                    DestinationSize,
                    Scratch,
                    ScratchSize
                    );

                  if (!EFI_ERROR (Status)) {
                    LoadROM     = TRUE;
                    ImageBuffer = DecompressedImageBuffer;
                    ImageLength = DestinationSize;
                  }

                  FreePool (Scratch);
                }
              }
            }
          }
        }

        if (LoadROM) {
          UnicodeSPrint (RomFileName, sizeof (RomFileName), L"%s[%d]", FileName, ImageIndex);
          FilePath    = FileDevicePath (NULL, RomFileName);
          Status      = gBS->LoadImage (
            TRUE,
            gImageHandle,
            FilePath,
            ImageBuffer,
            ImageLength,
            &ImageHandle
            );

          if (!EFI_ERROR (Status)) {
            gBS->StartImage (ImageHandle, NULL, NULL);
          } else if (Status == EFI_SECURITY_VIOLATION) {
            gBS->UnloadImage (ImageHandle);
          }
        }

        if (DecompressedImageBuffer != NULL) {
          FreePool (DecompressedImageBuffer);
        }
      }
    }

    RomBarOffset = RomBarOffset + ImageSize;
    ImageIndex++;
  } while ((Pcir->Indicator & 0x80) == 0x00 && RomBarOffset - (UINTN) RomBar < RomSize);

  return Status;
}

EFI_STATUS
OcReloadOptionRoms (
  VOID
  )
{
  UINTN                Index;
  UINTN                HandleIndex;
  UINTN                HandleArrayCount;
  UINTN                BindingHandleCount;
  EFI_HANDLE           *HandleArray;
  EFI_HANDLE           *BindingHandleBuffer;
  EFI_STATUS           ReturnStatus;
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  CHAR16               RomFileName[16];

  ReturnStatus        = EFI_LOAD_ERROR;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiPciIoProtocolGuid,
    NULL,
    &HandleArrayCount,
    &HandleArray
    );

  if (EFI_ERROR (Status)) {
    return EFI_PROTOCOL_ERROR;
  }

  for (Index = 0; Index < HandleArrayCount; Index++) {
    Status = gBS->HandleProtocol (
      HandleArray[Index],
      &gEfiPciIoProtocolGuid,
      (void **) &PciIo
      );

    if (!EFI_ERROR (Status) && PciIo->RomImage != NULL && PciIo->RomSize > 0) {
      BindingHandleBuffer = NULL;
      BindingHandleCount  = 0;
      PARSE_HANDLE_DATABASE_UEFI_DRIVERS (
        HandleArray[Index],
        &BindingHandleCount,
        &BindingHandleBuffer
        );

      if (BindingHandleCount == 0) {
        HandleIndex = InternalConvertHandleToHandleIndex (HandleArray[Index]);
        UnicodeSPrint (RomFileName, sizeof (RomFileName), L"Handle%X", HandleIndex);
        Status = ReloadPciRom (PciIo->RomImage, (UINTN) PciIo->RomSize, RomFileName);
        if (EFI_ERROR (ReturnStatus)) {
          ReturnStatus = Status;
        }
      }

      if (BindingHandleBuffer != NULL) {
        FreePool (BindingHandleBuffer);
      }
    }
  }

  FreePool (HandleArray);

  return ReturnStatus;
}
