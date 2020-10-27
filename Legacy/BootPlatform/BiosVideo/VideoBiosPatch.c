/** @file
  Copyright (C) 2020, Goldfish64. All rights reserved.

  Based on 915resolution and related projects.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "BiosVideo.h"

STATIC
BOOLEAN
GetEdidMaxResolution (
  IN  BIOS_VIDEO_DEV    *BiosVideoPrivate,
  OUT UINT32            *Width,
  OUT UINT32            *Height
  )
{
  UINT8         Checksum;
  UINT16        MaxWidth;
  UINT16        MaxHeight;
  UINT16        DtdWidth;
  UINT16        DtdHeight;
  UINT32        Index;

  VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK  *Edid;
  EDID_DTD                              *Dtds;

  Edid = (VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK *) BiosVideoPrivate->EdidActive.Edid;
  if (Edid == NULL) {
    return FALSE;
  }

  //
  // Ensure checksum matches.
  //
  Checksum = CalculateCheckSum8 (
               (UINT8 *) Edid,
               sizeof (VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK)
               );
  if (Checksum != 0) {
    return FALSE;
  }

  Dtds      = (EDID_DTD *) Edid->DetailedTimingDescriptions;
  MaxWidth  = 0;
  MaxHeight = 0;

  //
  // Pull maximum supported resolution from detailed timing descriptors.
  //
  for (Index = 0; Index < VESA_BIOS_EXTENSIONS_DETAILED_TIMING_DESCRIPTOR_COUNT; Index++) {
    DtdWidth  = Dtds[Index].HorzActivePixelsLsb | (Dtds[Index].HorzActivePixelsMsb << 8);
    DtdHeight = Dtds[Index].VertActivePixelsLsb | (Dtds[Index].VertActivePixelsMsb << 8);

    if (DtdWidth > MaxWidth && DtdHeight > MaxHeight) {
      MaxWidth  = DtdWidth;
      MaxHeight = DtdHeight;
    }
  }

  if (MaxWidth == 0 || MaxHeight == 0) {
    return FALSE;
  }

  *Width  = MaxWidth;
  *Height = MaxHeight;
  return TRUE;
}

STATIC
VOID
CalculateGtfTimings (
  IN  UINT16      Width,
  IN  UINT16      Height,
  IN  UINT16      Frequency,
  OUT UINT32      *ClockHz,
  OUT UINT16      *HSyncStart,
  OUT UINT16      *HSyncEnd,
  OUT UINT16      *HBlank,
  OUT UINT16      *VSyncStart,
  OUT UINT16      *VSyncEnd,
  OUT UINT16      *VBlank
  )
{
  UINT32 Tmp1;
  UINT32 Tmp2;
  UINT32 Tmp3;

  UINT32 Vbl;
  UINT32 VFreq;
  UINT32 Hbl;

  //
  // Below is based on 915resolution. GTF is the original VESA standard for timing calculation.
  //
  // The VESA GTF standard can be found here: https://glenwing.github.io/docs/VESA-GTF-1.1.pdf.
  //
  Tmp1  = 11 * Frequency;
  Vbl   = Height + (Height + 1) / (((20000 + Tmp1 / 2) / Tmp1) - 1) + 1;
  VFreq = Vbl * Frequency;

  Tmp1  = ((300000 * 1000) + VFreq / 2) / VFreq;
  Tmp2  = ((30 * 1000) - Tmp1) * 1000;
  Tmp3  = (70 * 1000) + Tmp1;
  Hbl   = Width * ((Tmp2 + Tmp3 / 2) / Tmp3);
  Hbl   = (Hbl + 16 / 2) / 16;
  Hbl   = (Hbl + 500) / 1000 * 16;

  *ClockHz    = (Width + Hbl) * VFreq / 1000;
  *HSyncStart = (UINT16)(Width + Hbl / 2 - (Width + Hbl + 50) / 100 * 8 - 1);
  *HSyncEnd   = (UINT16)(Width + Hbl / 2 - 1);
  *HBlank     = (UINT16)(Width + Hbl - 1);
  *VSyncStart = Height;
  *VSyncEnd   = (UINT16)Height + 3;
  *VBlank     = (UINT16)(Vbl - 1);
}

STATIC
VOID
CalculateDtd (
  IN  UINT16      Width,
  IN  UINT16      Height,
  IN  UINT32      ClockHz,
  IN  UINT16      HSyncStart,
  IN  UINT16      HSyncEnd,
  IN  UINT16      HBlank,
  IN  UINT16      VSyncStart,
  IN  UINT16      VSyncEnd,
  IN  UINT16      VBlank,
  OUT EDID_DTD    *Dtd
  )
{
  UINT16        HorzSyncOffset;
  UINT16        HorzSyncPulseWidth;
  UINT16        VertSyncOffset;
  UINT16        VertSyncPulseWidth;

  HorzSyncOffset              = HSyncStart - Width;
  HorzSyncPulseWidth          = HSyncEnd - HSyncStart;
  VertSyncOffset              = VSyncStart - Height;
  VertSyncPulseWidth          = VSyncEnd - VSyncStart;

  Dtd->PixelClock             = (UINT16)(ClockHz / 10);

  Dtd->HorzActivePixelsLsb    = Width & 0xFF;
  Dtd->HorzActivePixelsMsb    = (Width >> 8) & 0xF;
  Dtd->HorzBlankPixels        = HBlank - Width;
  
  Dtd->VertActivePixelsLsb    = Height & 0xFF;
  Dtd->VertActivePixelsMsb    = (Height >> 8) & 0xF;
  Dtd->VertBlankPixels        = VBlank - Height;

  Dtd->HorzSyncOffsetLsb      = HorzSyncOffset & 0xFF;
  Dtd->HorzSyncPulseWidthLsb  = HorzSyncPulseWidth & 0xFF;
  Dtd->VertSyncOffsetLsb      = VertSyncOffset & 0xFF;
  Dtd->VertSyncPulseWidthLsb  = VertSyncPulseWidth & 0xFF;

  Dtd->VertSyncPulseWidthMsb  = (VertSyncPulseWidth >> 8) & 0xF;
  Dtd->VertSyncOffsetMsb      = (VertSyncOffset >> 8) & 0xF;
  Dtd->HorzSyncPulseWidthMsb  = (HorzSyncPulseWidth >> 8) & 0xF;
  Dtd->HorzSyncOffsetMsb      = (HorzSyncOffset >> 8) & 0xF;

  Dtd->HorzImageSizeLsb       = 0;
  Dtd->HorzImageSizeMsb       = 0;
  Dtd->VertImageSize          = 0;
  Dtd->HorzBorderPixels       = 0;
  Dtd->VertBorderPixels       = 0;
  Dtd->Features               = 0x18;
}

STATIC
BOOLEAN
PatchIntelVbiosCustom (
  IN UINT8      *Vbios,
  IN UINT32     VbiosSize,
  IN UINT16     Width,
  IN UINT16     Height
  )
{
  UINT32      ClockHz;
  UINT16      HSyncStart;
  UINT16      HSyncEnd;
  UINT16      HBlank;
  UINT16      VSyncStart;
  UINT16      VSyncEnd;
  UINT16      VBlank;

  EDID_DTD    Dtd;

  //
  // DTD entry for 1024x768 resolution for the Intel VBIOS.
  // This will be replaced with our custom resolution.
  //
  STATIC CONST UINT8 IntelDtd1024[] = {
    0x64, 0x19, 0x00, 0x40, 0x41, 0x00, 0x26, 0x30, 0x18, 
    0x88, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18
    };

  CalculateGtfTimings (
    Width,
    Height,
    60,
    &ClockHz,
    &HSyncStart,
    &HSyncEnd,
    &HBlank,
    &VSyncStart,
    &VSyncEnd,
    &VBlank
    );
  
  CalculateDtd (
    Width,
    Height,
    ClockHz,
    HSyncStart,
    HSyncEnd,
    HBlank,
    VSyncStart,
    VSyncEnd,
    VBlank,
    &Dtd
    );
  
  return ApplyPatch (
    IntelDtd1024,
    NULL,
    sizeof (IntelDtd1024),
    (UINT8 *) &Dtd,
    NULL,
    Vbios,
    VbiosSize,
    0,
    0
    ) != 0;
}

EFI_STATUS
EFIAPI
BiosVideoForceResolutionSetResolution (
  IN OUT OC_FORCE_RESOLUTION_PROTOCOL *This,
  IN     UINT32                       Width   OPTIONAL,
  IN     UINT32                       Height  OPTIONAL
  )
{
  EFI_STATUS              Status;
  BOOLEAN                 Result;
  BIOS_VIDEO_DEV          *BiosVideoPrivate;
  BIOS_VIDEO_MODE_DATA    *ModeData;
  UINT32                  ModeIndex;

  UINT8                   *Vbios;

  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_OC_FORCE_RESOLUTION_THIS (This);
  
  if (!BiosVideoPrivate->ProduceGraphicsOutput) {
    return EFI_UNSUPPORTED;
  }

  //
  // If height/width are zero, try to get max from EDID.
  //
  if (Width == 0 && Height == 0) {
    if (!GetEdidMaxResolution (BiosVideoPrivate, &Width, &Height)) {
      return EFI_UNSUPPORTED;
    }
  }

  //
  // We cannot support resolutions under 640x480 due to checks
  // elsewhere in the driver.
  //
  if (Width < 640 || Height < 480) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if resolution is already supported.
  //
  for (ModeIndex = 0; ModeIndex < BiosVideoPrivate->GraphicsOutput.Mode->MaxMode; ModeIndex++) {
    ModeData = &BiosVideoPrivate->ModeData[ModeIndex];
    if (ModeData->HorizontalResolution == Width
      && ModeData->VerticalResolution == Height) {
      return EFI_ALREADY_STARTED;
    }
  }

  //
  // Unlock VBIOS region.
  //
  LegacyRegionUnlock (LEGACY_REGION_BASE, LEGACY_REGION_SIZE);
  Vbios = (UINT8 *) LEGACY_REGION_BASE;

  //
  // Patch VBIOS according to vendor specific patching.
  //
  Result = FALSE;
  switch (BiosVideoPrivate->PciVendorId) {
    case PCI_VENDOR_INTEL:
      Result = PatchIntelVbiosCustom (Vbios, LEGACY_REGION_SIZE, (UINT16)Width, (UINT16)Height);
      break;
  }

  if (!Result) {
    return EFI_UNSUPPORTED;
  }
  
  //
  // Refresh VBE data.
  //
  Status = BiosVideoGetVbeData (BiosVideoPrivate);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Reconnect protocols as drivers like ConSplitter will consume our GOP.
  //
  gBS->DisconnectController (BiosVideoPrivate->Handle, NULL, NULL);
  gBS->ConnectController (BiosVideoPrivate->Handle, NULL, NULL, TRUE);

  return EFI_SUCCESS;
}
