/** @file
  Copyright (C) 2021, Goldfish64. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <IndustryStandard/HdaVerbs.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAudioLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePathToText.h>
#include <Protocol/HdaIo.h>
#include <Protocol/HdaCodecInfo.h>
#include <Protocol/HdaControllerInfo.h>

#include "OcAudioInternal.h"

//
// Widget names.
//
STATIC
CHAR8 *mWidgetNames[HDA_WIDGET_TYPE_VENDOR + 1] =
{
  "Audio Output",
  "Audio Input",
  "Audio Mixer",
  "Audio Selector",
  "Pin Complex",
  "Power Widget",
  "Volume Knob Widget",
  "Beep Generator Widget",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "Vendor Defined Widget"
};

//
// Port names.
//
STATIC
CHAR8 *mPortConnectivities[4] =
{
  "Jack",
  "None",
  "Fixed",
  "Int Jack"
};

//
// Device names.
//
STATIC
CHAR8 *mDefaultDevices[HDA_CONFIG_DEFAULT_DEVICE_OTHER + 1] =
{
  "Line Out",
  "Speaker",
  "HP Out",
  "CD",
  "SPDIF Out",
  "Digital Out",
  "Modem Line",
  "Modem Handset",
  "Line In",
  "Aux",
  "Mic",
  "Telephone",
  "SPDIF In",
  "Digital In",
  "Reserved",
  "Other"
};

//
// Port surface type names.
//
STATIC
CHAR8 *mSurfaces[4] =
{
  "Ext",
  "Int",
  "Ext",
  "Other"
};

//
// Port location names.
//
STATIC
CHAR8 *mLocations[0xF + 1] =
{
  "N/A",
  "Rear",
  "Front",
  "Left",
  "Right",
  "Top",
  "Bottom",
  "Special",
  "Special",
  "Special",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved"
};

//
// Connection type names.
//
STATIC
CHAR8 *mConnTypes[HDA_CONFIG_DEFAULT_CONN_OTHER + 1] =
{
  "Unknown",
  "1/8",
  "1/4",
  "ATAPI",
  "RCA",
  "Optical",
  "Digital",
  "Analog",
  "Multi",
  "XLR",
  "RJ11",
  "Combo",
  "Other",
  "Other",
  "Other",
  "Other"
};

//
// Port color names.
STATIC
CHAR8 *mColors[HDA_CONFIG_DEFAULT_COLOR_OTHER + 1] =
{
  "Unknown",
  "Black",
  "Grey",
  "Blue",
  "Green",
  "Red",
  "Orange",
  "Yellow",
  "Purple",
  "Pink",
  "Reserved",
  "Reserved",
  "Reserved",
  "Reserved",
  "White",
  "Other"
};

STATIC
VOID
PrintRatesFormats (
  IN OUT CHAR8        **AsciiBuffer,
  IN     UINTN        *AsciiBufferSize,
  IN     UINT32       Rates,
  IN     UINT32       Formats
  )
{
  //
  // Print sample rates.
  //
  OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "    rates [0x%X]:", (UINT16) Rates);
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 8000");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_11KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 11025");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 16000");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_22KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 22050");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 32000");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_44KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 44100");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_48KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 48000");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_88KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 88200");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_96KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 96000");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_176KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 176400");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_192KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 192000");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_384KHZ) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 384000");
  }
  OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");

  //
  // Print sample bits.
  //
  OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "    bits [0x%X]:", (UINT16)(Rates >> 16));
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8BIT) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 8");
  }  
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16BIT) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 16");
  }  
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_20BIT) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 20");
  } 
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_24BIT) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 24");
  }
  if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32BIT) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 32");
  }
  OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");

  //
  // Print sample formats.
  //
  OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "    formats [0x%X]:", Formats);
  if (Formats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_PCM) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " PCM");
  }
  if (Formats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_FLOAT32) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " FLOAT32");
  }
  if (Formats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_AC3) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " AC3");
  }
  OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");
}

STATIC
VOID
PrintAmpCaps (
  IN OUT CHAR8        **AsciiBuffer,
  IN     UINTN        *AsciiBufferSize,
  IN     UINT32       AmpCaps
  )
{
  if (AmpCaps != 0) {
    OcAsciiPrintBuffer (
      AsciiBuffer,
      AsciiBufferSize,
      "ofs=0x%2X, nsteps=0x%2X, stepsize=0x%2X, mute=%u\n",
      HDA_PARAMETER_AMP_CAPS_OFFSET (AmpCaps),
      HDA_PARAMETER_AMP_CAPS_NUM_STEPS (AmpCaps),
      HDA_PARAMETER_AMP_CAPS_STEP_SIZE (AmpCaps),
      (AmpCaps & HDA_PARAMETER_AMP_CAPS_MUTE) != 0
      );
  } else {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "N/A\n");
  }
}

STATIC
VOID
PrintWidget (
  IN OUT CHAR8        **AsciiBuffer,
  IN     UINTN        *AsciiBufferSize,
  IN     HDA_WIDGET   *HdaWidget
  )
{
  UINT32      Index;
  
  //
  // Node header and widget capabilities.
  //
  OcAsciiPrintBuffer (
    AsciiBuffer,
    AsciiBufferSize,
    "Node 0x%2X (%u) [%a] wcaps 0x%X:",
    HdaWidget->NodeId,
    HdaWidget->NodeId,
    mWidgetNames[HDA_PARAMETER_WIDGET_CAPS_TYPE (HdaWidget->Capabilities)],
    HdaWidget->Capabilities
    );

  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_STEREO) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " Stereo");
  } else {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " Mono");
  }

  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_DIGITAL) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " Digital");
  }
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_IN_AMP) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " Amp-In");
  }
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_OUT_AMP) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " Amp-Out");
  }
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_L_R_SWAP) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " R/L");
  }
  OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");

  //
  // Input amp capabilities and defaults.
  //
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_IN_AMP) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "  Amp-In caps: ");
    PrintAmpCaps (AsciiBuffer, AsciiBufferSize, HdaWidget->AmpInCapabilities);

    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "  Amp-In vals:");
    for (Index = 0; Index < HdaWidget->ConnectionCount; Index++) {
      if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_STEREO) {
        OcAsciiPrintBuffer (
          AsciiBuffer,
          AsciiBufferSize,
          " [0x%2X 0x%2X]",
          HdaWidget->AmpInLeftDefaultGainMute[Index],
          HdaWidget->AmpInRightDefaultGainMute[Index]
          );
      } else {
        OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " [0x%2X]", HdaWidget->AmpInLeftDefaultGainMute[Index]);
      }       
    }
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");
  }

  //
  // Output amp capabilities and defaults.
  //
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_OUT_AMP) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "  Amp-Out caps: ");
    PrintAmpCaps (AsciiBuffer, AsciiBufferSize, HdaWidget->AmpOutCapabilities);

    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "  Amp-Out vals:");
    if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_STEREO) {
      OcAsciiPrintBuffer (
        AsciiBuffer,
        AsciiBufferSize,
        " [0x%2X 0x%2X]",
        HdaWidget->AmpOutLeftDefaultGainMute,
        HdaWidget->AmpOutRightDefaultGainMute
        );
    } else {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " [0x%2X]", HdaWidget->AmpOutLeftDefaultGainMute);
    }   
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");
  }

  //
  // Input/output capabilities and defaults.
  //
  if (HDA_PARAMETER_WIDGET_CAPS_TYPE (HdaWidget->Capabilities) == HDA_WIDGET_TYPE_INPUT
    || HDA_PARAMETER_WIDGET_CAPS_TYPE (HdaWidget->Capabilities) == HDA_WIDGET_TYPE_OUTPUT) {

    OcAsciiPrintBuffer (
      AsciiBuffer,
      AsciiBufferSize,
      "  Converter: stream=%u, channel=%u\n",
      HDA_VERB_GET_CONVERTER_STREAM_STR (HdaWidget->DefaultConvStreamChannel),
      HDA_VERB_GET_CONVERTER_STREAM_CHAN (HdaWidget->DefaultConvStreamChannel)
      );
    
    if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_FORMAT_OVERRIDE) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "  PCM:\n");
      PrintRatesFormats (AsciiBuffer, AsciiBufferSize, HdaWidget->SupportedPcmRates, HdaWidget->SupportedFormats);
    }
  }

  //
  // Pin complex capabilities and defaults.
  //
  if (HDA_PARAMETER_WIDGET_CAPS_TYPE (HdaWidget->Capabilities) == HDA_WIDGET_TYPE_PIN_COMPLEX) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "  Pincap 0x%8X:", HdaWidget->PinCapabilities);
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_INPUT) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " IN");
    }   
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_OUTPUT) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " OUT");
    } 
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_HEADPHONE) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " HP");
    }
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_EAPD) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " EAPD");
    }
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_TRIGGER) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " Trigger");
    }
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_PRESENCE) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " Detect");
    }
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_HBR) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " HBR");
    }
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_HDMI) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " HDMI");
    }
    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_DISPLAYPORT) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " DP");
    }
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");

    if (HdaWidget->PinCapabilities & HDA_PARAMETER_PIN_CAPS_EAPD) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "  EAPD 0x%X:", HdaWidget->DefaultEapd);
      if (HdaWidget->DefaultEapd & HDA_EAPD_BTL_ENABLE_BTL) {
        OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " BTL");
      }
      if (HdaWidget->DefaultEapd & HDA_EAPD_BTL_ENABLE_EAPD) {
        OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " EAPD");
      }
      if (HdaWidget->DefaultEapd & HDA_EAPD_BTL_ENABLE_L_R_SWAP) {
        OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " R/L");
      }
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");
    }

    OcAsciiPrintBuffer (
      AsciiBuffer,
      AsciiBufferSize,
      "  Pin Default 0x%8X: [%a] %a at %a %a\n",
      HdaWidget->DefaultConfiguration,
      mPortConnectivities[HDA_VERB_GET_CONFIGURATION_DEFAULT_PORT_CONN (HdaWidget->DefaultConfiguration)],
      mDefaultDevices[HDA_VERB_GET_CONFIGURATION_DEFAULT_DEVICE (HdaWidget->DefaultConfiguration)],
      mSurfaces[HDA_VERB_GET_CONFIGURATION_DEFAULT_SURF (HdaWidget->DefaultConfiguration)],
      mLocations[HDA_VERB_GET_CONFIGURATION_DEFAULT_LOC (HdaWidget->DefaultConfiguration)]
      );

    OcAsciiPrintBuffer (
      AsciiBuffer,
      AsciiBufferSize,
      "    Conn = %a, Color = %a\n",
      mConnTypes[HDA_VERB_GET_CONFIGURATION_DEFAULT_CONN_TYPE (HdaWidget->DefaultConfiguration)],
      mColors[HDA_VERB_GET_CONFIGURATION_DEFAULT_COLOR (HdaWidget->DefaultConfiguration)]
      );

    OcAsciiPrintBuffer (
      AsciiBuffer,
      AsciiBufferSize,
      "    DefAssociation = 0x%X, Sequence = 0x%X\n",
      HDA_VERB_GET_CONFIGURATION_DEFAULT_ASSOCIATION (HdaWidget->DefaultConfiguration),
      HDA_VERB_GET_CONFIGURATION_DEFAULT_SEQUENCE (HdaWidget->DefaultConfiguration)
      );

    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "Pin-ctls: 0x%2X:", HdaWidget->DefaultPinControl);
    if (HdaWidget->DefaultPinControl & HDA_PIN_WIDGET_CONTROL_VREF_EN) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " VREF");
    }
    if (HdaWidget->DefaultPinControl & HDA_PIN_WIDGET_CONTROL_IN_EN) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " IN");
    }
    if (HdaWidget->DefaultPinControl & HDA_PIN_WIDGET_CONTROL_OUT_EN) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " OUT");
    }
    if (HdaWidget->DefaultPinControl & HDA_PIN_WIDGET_CONTROL_HP_EN) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " HP");
    }
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");
  }

  //
  // Connections to other widgets.
  //
  if (HdaWidget->Capabilities & HDA_PARAMETER_WIDGET_CAPS_CONN_LIST) {
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "  Connection: %u\n    ", HdaWidget->ConnectionCount);
    for (Index = 0; Index < HdaWidget->ConnectionCount; Index++) {
      OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, " 0x%2X", HdaWidget->Connections[Index]);
    }
    OcAsciiPrintBuffer (AsciiBuffer, AsciiBufferSize, "\n");
  }
}

EFI_STATUS
OcAudioDump (
  IN EFI_FILE_PROTOCOL  *Root
  ) 
{
  EFI_STATUS                        Status;
  UINTN                             HandleCount;
  EFI_HANDLE                        *HandleBuffer;
  UINT32                            Index;

  EFI_HDA_CONTROLLER_INFO_PROTOCOL  *HdaControllerInfo;
  EFI_HDA_CODEC_INFO_PROTOCOL       *HdaCodecInfo;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;

  CHAR8                             *FileBuffer;
  UINTN                             FileBufferSize;
  CHAR16                            TmpFileName[32];

  CONST CHAR16                      *Name;
  CHAR16                            *DevicePathStr;
  BOOLEAN                           TmpBool;
  UINT8                             Tmp8;
  UINT32                            Tmp32A;
  UINT32                            Tmp32B;

  HDA_WIDGET                        *HdaWidgets;
  UINTN                             HdaWidgetCount;
  UINT32                            HdaWidgetIndex;

  HandleCount = 0;

  //
  // Get all HDA controller instances.
  //
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiHdaControllerInfoProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  DEBUG ((DEBUG_INFO, "OCAU: %u HDA controllers installed - %r\n", (UINT32) HandleCount, Status));

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiHdaControllerInfoProtocolGuid, (VOID **) &HdaControllerInfo);
      DEBUG ((DEBUG_INFO, "OCAU: HDA controller %u info open result - %r\n", Index, Status));
      if (EFI_ERROR (Status)) {
        continue;
      }

      FileBufferSize = SIZE_1KB;
      FileBuffer     = AllocateZeroPool (FileBufferSize);
      if (FileBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Get device path of controller.
      //
      DevicePath    = DevicePathFromHandle (HandleBuffer[Index]);
      DevicePathStr = NULL;
      if (DevicePath != NULL) {
        DevicePathStr = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
      }

      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Device path: %s\n", DevicePathStr != NULL ? DevicePathStr : L"<NULL>");
      DEBUG ((DEBUG_INFO, "OCAU: Dumping controller at %s\n", DevicePathStr != NULL ? DevicePathStr : L"<NULL>"));
      if (DevicePathStr != NULL) {
        FreePool (DevicePathStr);
      }

      Status = HdaControllerInfo->GetName (HdaControllerInfo, &Name);
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Controller: %s\n", Status == EFI_SUCCESS ? Name : L"<NULL>");

      Status = HdaControllerInfo->GetVendorId (HdaControllerInfo, &Tmp32A);
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Vendor Id: 0x%X\n", Tmp32A);

      //
      // Save dumped controller data to file.
      //
      if (FileBuffer != NULL) {
        UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"Controller%u.txt", Index);
        Status = SetFileData (Root, TmpFileName, FileBuffer, (UINT32) AsciiStrLen (FileBuffer));
        DEBUG ((DEBUG_INFO, "OCAU: Dumped HDA controller %u info result - %r\n", Index, Status));

        FreePool (FileBuffer);
      }
    }

    FreePool (HandleBuffer);
  }

  HandleCount = 0;

  //
  // Get all HDA codec instances.
  //
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiHdaCodecInfoProtocolGuid,
    NULL,
    &HandleCount,
    &HandleBuffer
    );
  DEBUG ((DEBUG_INFO, "OCAU: %u HDA codecs installed - %r\n", (UINT32) HandleCount, Status));

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiHdaCodecInfoProtocolGuid, (VOID **) &HdaCodecInfo);
      DEBUG ((DEBUG_INFO, "OCAU: HDA codec %u info open result - %r\n", Index, Status));
      if (EFI_ERROR (Status)) {
        continue;
      }

      Status = HdaCodecInfo->GetWidgets (HdaCodecInfo, &HdaWidgets, &HdaWidgetCount);
      if (EFI_ERROR (Status)) {
        continue;
      }
      
      if (OcOverflowMulAddUN (SIZE_4KB, HdaWidgetCount, SIZE_4KB, &FileBufferSize)) {
        HdaCodecInfo->FreeWidgetsBuffer (HdaWidgets, HdaWidgetCount);
        continue;
      }

      FileBuffer = AllocateZeroPool (FileBufferSize);
      if (FileBuffer == NULL) {
        HdaCodecInfo->FreeWidgetsBuffer (HdaWidgets, HdaWidgetCount);
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Get device path of codec.
      //
      DevicePath    = DevicePathFromHandle (HandleBuffer[Index]);
      DevicePathStr = NULL;
      if (DevicePath != NULL) {
        DevicePathStr = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
      }

      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Device path: %s\n", DevicePathStr != NULL ? DevicePathStr : L"<NULL>");
      DEBUG ((DEBUG_INFO, "OCAU: Dumping codec at %s\n", DevicePathStr != NULL ? DevicePathStr : L"<NULL>"));
      if (DevicePathStr != NULL) {
        FreePool (DevicePathStr);
      }

      Status = HdaCodecInfo->GetAddress (HdaCodecInfo, &Tmp8);
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Address: 0x%X\n\n", Tmp8);

      Status = HdaCodecInfo->GetName (HdaCodecInfo, &Name);
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Codec: %s\n", Status == EFI_SUCCESS ? Name : L"<NULL>");

      Status = HdaCodecInfo->GetAudioFuncId (HdaCodecInfo, &Tmp8, &TmpBool);
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "AFG Function Id: 0x%X (unsol %u)\n", Tmp8, TmpBool);

      Status = HdaCodecInfo->GetVendorId (HdaCodecInfo, &Tmp32A);
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Vendor Id: 0x%X\n", Tmp32A);

      Status = HdaCodecInfo->GetRevisionId (HdaCodecInfo, &Tmp32A);
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Revision Id: 0x%X\n", Tmp32A);

      Status = HdaCodecInfo->GetDefaultRatesFormats (HdaCodecInfo, &Tmp32A, &Tmp32B);
      if (!EFI_ERROR (Status) && (Tmp32A != 0 || Tmp32B != 0)) {
        OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Default PCM:\n");
        PrintRatesFormats (&FileBuffer, &FileBufferSize, Tmp32A, Tmp32B);
      } else {
        OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Default PCM: N/A\n");
      }

      Status = HdaCodecInfo->GetDefaultAmpCaps (HdaCodecInfo, &Tmp32A, &Tmp32B);
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Default Amp-In caps: ");
      PrintAmpCaps (&FileBuffer, &FileBufferSize, Status == EFI_SUCCESS ? Tmp32A : 0);
      OcAsciiPrintBuffer (&FileBuffer, &FileBufferSize, "Default Amp-Out caps: ");
      PrintAmpCaps (&FileBuffer, &FileBufferSize, Status == EFI_SUCCESS ? Tmp32B : 0);

      //
      // Print all widgets.
      //
      for (HdaWidgetIndex = 0; HdaWidgetIndex < HdaWidgetCount; HdaWidgetIndex++) {
        PrintWidget (&FileBuffer, &FileBufferSize, &HdaWidgets[HdaWidgetIndex]);
      }
      HdaCodecInfo->FreeWidgetsBuffer (HdaWidgets, HdaWidgetCount);

      //
      // Save dumped codec data to file.
      //
      if (FileBuffer != NULL) {
        UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"Codec%u.txt", Index);
        Status = SetFileData (Root, TmpFileName, FileBuffer, (UINT32) AsciiStrLen (FileBuffer));
        DEBUG ((DEBUG_INFO, "OCAU: Dumped HDA codec %u info result - %r\n", Index, Status));

        FreePool (FileBuffer);
      }
    }

    FreePool (HandleBuffer);
  }

  return EFI_SUCCESS;
}
