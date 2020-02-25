/*
 * File: HdaCodecDump.c
 *
 * Copyright (c) 2018 John Davis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "HdaCodecDump.h"

STATIC CONST CHAR16 *mWidgetNames[HDA_WIDGET_TYPE_VENDOR + 1] = {
  L"Audio Output",
  L"Audio Input",
  L"Audio Mixer",
  L"Audio Selector",
  L"Pin Complex",
  L"Power Widget",
  L"Volume Knob Widget",
  L"Beep Generator Widget",
  L"Reserved",
  L"Reserved",
  L"Reserved",
  L"Reserved",
  L"Reserved",
  L"Reserved",
  L"Reserved",
  L"Vendor Defined Widget"
};

STATIC CONST CHAR16 *mPortConnectivities[4] = {
  L"Jack",
  L"None",
  L"Fixed",
  L"Int Jack"
};

STATIC CONST CHAR16 *mDefaultDevices[HDA_CONFIG_DEFAULT_DEVICE_OTHER + 1] = {
  L"Line Out",
  L"Speaker",
  L"HP Out",
  L"CD",
  L"SPDIF Out",
  L"Digital Out",
  L"Modem Line",
  L"Modem Handset",
  L"Line In",
  L"Aux",
  L"Mic",
  L"Telephone",
  L"SPDIF In",
  L"Digital In",
  L"Reserved",
  L"Other"
};

STATIC CONST CHAR16 *mSurfaces[4] = {
  L"Ext",
  L"Int",
  L"Ext",
  L"Other"
};

STATIC CONST CHAR16 *mLocations[16] = {
  L"N/A",
  L"Rear",
  L"Front",
  L"Left",
  L"Right",
  L"Top",
  L"Bottom",
  L"Special",
  L"Special",
  L"Special",
  L"Reserved",
  L"Reserved",
  L"Reserved",
  L"Reserved"
};

STATIC CONST CHAR16 *mConnTypes[HDA_CONFIG_DEFAULT_CONN_OTHER + 1] = {
  L"Unknown",
  L"1/8",
  L"1/4",
  L"ATAPI",
  L"RCA",
  L"Optical",
  L"Digital",
  L"Analog",
  L"Multi",
  L"XLR",
  L"RJ11",
  L"Combo",
  L"Other",
  L"Other",
  L"Other",
  L"Other"
};

STATIC CONST CHAR16 *mColors[HDA_CONFIG_DEFAULT_COLOR_OTHER + 1] = {
  L"Unknown",
  L"Black",
  L"Grey",
  L"Blue",
  L"Green",
  L"Red",
  L"Orange",
  L"Yellow",
  L"Purple",
  L"Pink",
  L"Reserved",
  L"Reserved",
  L"Reserved",
  L"Reserved",
  L"White",
  L"Other"
};


STATIC
VOID
HdaCodecDumpPrintRatesFormats (
  IN  UINT32  Rates,
  IN  UINT32  Formats
  )
{
  //
  // Print sample rates.
  //
  Print (L"    rates [0x%X]:", (UINT16) Rates);
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8KHZ) != 0) {
    Print (L" 8000");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_11KHZ) != 0) {
    Print (L" 11025");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16KHZ) != 0) {
    Print (L" 16000");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_22KHZ) != 0) {
    Print (L" 22050");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32KHZ) != 0) {
    Print (L" 32000");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_44KHZ) != 0) {
    Print (L" 44100");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_48KHZ) != 0) {
    Print (L" 48000");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_88KHZ) != 0) {
    Print (L" 88200");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_96KHZ) != 0) {
    Print (L" 96000");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_176KHZ) != 0) {
    Print (L" 176400");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_192KHZ) != 0) {
    Print (L" 192000");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_384KHZ) != 0) {
    Print (L" 384000");
  }
  Print (L"\n");

  //
  // Print bits.
  //
  Print (L"    bits [0x%X]:", (UINT16) (Rates >> 16U));
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8BIT) != 0) {
    Print (L" 8");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16BIT) != 0) {
    Print (L" 16");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_20BIT) != 0) {
    Print (L" 20");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_24BIT) != 0) {
    Print (L" 24");
  }
  if ((Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32BIT) != 0) {
    Print (L" 32");
  }
  Print (L"\n");

  //
  // Print formats.
  //
  Print (L"    formats [0x%X]:", Formats);
  if ((Formats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_PCM) != 0) {
    Print (L" PCM");
  }
  if ((Formats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_FLOAT32) != 0) {
    Print (L" FLOAT32");
  }
  if ((Formats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_AC3) != 0) {
    Print (L" AC3");
  }
  Print (L"\n");
}

STATIC
VOID
HdaCodecDumpPrintAmpCaps (
  IN UINT32  AmpCaps
  )
{
  if (AmpCaps) {
    Print (
      L"ofs=0x%2X, nsteps=0x%2X, stepsize=0x%2X, mute=%u\n",
      HDA_PARAMETER_AMP_CAPS_OFFSET (AmpCaps),
      HDA_PARAMETER_AMP_CAPS_NUM_STEPS (AmpCaps),
      HDA_PARAMETER_AMP_CAPS_STEP_SIZE (AmpCaps),
      (AmpCaps & HDA_PARAMETER_AMP_CAPS_MUTE) != 0
      );
  } else {
    Print (L"N/A\n");
  }
}

STATIC
VOID
HdaCodecDumpPrintWidgets (
  IN HDA_WIDGET  *Widgets,
  IN UINTN       WidgetCount
  )
{
  UINTN  Index;
  UINTN  Index2;

  for (Index = 0; Index < WidgetCount; ++Index) {
    //
    // Print each widget.
    //

    //
    // Print header and capabilities.
    //
    Print (
      L"Node %2u [%s] wcaps 0x%X:",
      Widgets[Index].NodeId,
      mWidgetNames[HDA_PARAMETER_WIDGET_CAPS_TYPE (Widgets[Index].Capabilities)],
      Widgets[Index].Capabilities
      );
    if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_STEREO) != 0) {
      Print (L" Stereo");
    } else {
      Print (L" Mono");
    }

    if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_DIGITAL) != 0) {
      Print (L" Digital");
    }
    if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_IN_AMP) != 0) {
      Print (L" Amp-In");
    }
    if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_OUT_AMP) != 0) {
      Print (L" Amp-Out");
    }
    if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_L_R_SWAP) != 0) {
      Print (L" R/L");
    }
    Print (L"\n");

    //
    // Print input amp info.
    //
    if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_IN_AMP) != 0) {
      //
      // Print caps.
      //
      Print (L"  Amp-in caps: ");
      HdaCodecDumpPrintAmpCaps (Widgets[Index].AmpInCapabilities);

      //
      // Print default values.
      //
      Print (L"  Amp-In vals:");
      for (Index2 = 0; Index2 < HDA_PARAMETER_CONN_LIST_LENGTH_LEN (Widgets[Index].ConnectionListLength); ++Index2) {
        if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_STEREO) != 0) {
          Print (
            L" [0x%2X 0x%2X]",
            Widgets[Index].AmpInLeftDefaultGainMute[Index2],
            Widgets[Index].AmpInRightDefaultGainMute[Index2]
            );
        } else {
          Print (L" [0x%2X]", Widgets[Index].AmpInLeftDefaultGainMute[Index2]);
        }
      }
      Print (L"\n");
    }

    //
    // Print output amp info.
    //
    if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_OUT_AMP) != 0) {
      //
      // Print caps.
      //
      Print (L"  Amp-Out caps: ");
      HdaCodecDumpPrintAmpCaps (Widgets[Index].AmpOutCapabilities);

      //
      // Print default values.
      //
      Print (L"  Amp-Out vals:");
      if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_STEREO) != 0) {
        Print (L" [0x%2X 0x%2X]\n", Widgets[Index].AmpOutLeftDefaultGainMute, Widgets[Index].AmpOutRightDefaultGainMute);
      } else {
        Print (L" [0x%2X]\n", Widgets[Index].AmpOutLeftDefaultGainMute);
      }
    }

    //
    // Print pin complexe info.
    //
    if (HDA_PARAMETER_WIDGET_CAPS_TYPE (Widgets[Index].Capabilities) == HDA_WIDGET_TYPE_PIN_COMPLEX) {
      //
      // Print pin capabilities.
      //
      Print (L"  Pincap 0x%8X:", Widgets[Index].PinCapabilities);
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_INPUT) != 0) {
        Print (L" IN");
      }
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_OUTPUT) != 0) {
        Print (L" OUT");
      }
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_HEADPHONE) != 0) {
        Print (L" HP");
      }
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_EAPD) != 0) {
        Print (L" EAPD");
      }
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_TRIGGER) != 0) {
        Print (L" Trigger");
      }
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_PRESENCE) != 0) {
        Print (L" Detect");
      }
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_HBR) != 0) {
        Print (L" HBR");
      }
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_HDMI) != 0) {
        Print (L" HDMI");
      }
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_DISPLAYPORT) != 0) {
        Print (L" DP");
      }
      Print (L"\n");

      //
      // Print EAPD info.
      //
      if ((Widgets[Index].PinCapabilities & HDA_PARAMETER_PIN_CAPS_EAPD) != 0) {
        Print (L"  EAPD 0x%X:", Widgets[Index].DefaultEapd);
        if ((Widgets[Index].DefaultEapd & HDA_EAPD_BTL_ENABLE_BTL) != 0) {
          Print (L" BTL");
        }
        if ((Widgets[Index].DefaultEapd & HDA_EAPD_BTL_ENABLE_EAPD) != 0) {
          Print (L" EAPD");
        }
        if ((Widgets[Index].DefaultEapd & HDA_EAPD_BTL_ENABLE_L_R_SWAP) != 0) {
          Print (L" R/L");
        }
        Print (L"\n");
      }

      //
      // Create pin default names.
      //

      //
      // Print pin default header.
      //
      Print (
        L"  Pin Default 0x%8X: [%s] %s at %s %s\n",
        Widgets[Index].DefaultConfiguration,
        mPortConnectivities[HDA_VERB_GET_CONFIGURATION_DEFAULT_PORT_CONN (Widgets[Index].DefaultConfiguration)],
        mDefaultDevices[HDA_VERB_GET_CONFIGURATION_DEFAULT_DEVICE (Widgets[Index].DefaultConfiguration)],
        mSurfaces[HDA_VERB_GET_CONFIGURATION_DEFAULT_SURF (Widgets[Index].DefaultConfiguration)],
        mLocations[HDA_VERB_GET_CONFIGURATION_DEFAULT_LOC (Widgets[Index].DefaultConfiguration)]);

      //
      // Print connection type and color.
      //
      Print (
        L"    Conn = %s, Color = %s\n",
        mConnTypes[HDA_VERB_GET_CONFIGURATION_DEFAULT_CONN_TYPE (Widgets[Index].DefaultConfiguration)],
        mColors[HDA_VERB_GET_CONFIGURATION_DEFAULT_COLOR (Widgets[Index].DefaultConfiguration)]
        );

      //
      // Print default association and sequence.
      //
      Print (
        L"    DefAssociation = 0x%X, Sequence = 0x%X\n",
        HDA_VERB_GET_CONFIGURATION_DEFAULT_ASSOCIATION (Widgets[Index].DefaultConfiguration),
        HDA_VERB_GET_CONFIGURATION_DEFAULT_SEQUENCE (Widgets[Index].DefaultConfiguration)
        );

      //
      // Print default pin control.
      //
      Print (L"Pin-ctls: 0x%2X:", Widgets[Index].DefaultPinControl);
      if ((Widgets[Index].DefaultPinControl & HDA_PIN_WIDGET_CONTROL_VREF_EN) != 0) {
        Print (L" VREF");
      }
      if ((Widgets[Index].DefaultPinControl & HDA_PIN_WIDGET_CONTROL_IN_EN) != 0) {
        Print (L" IN");
      }
      if ((Widgets[Index].DefaultPinControl & HDA_PIN_WIDGET_CONTROL_OUT_EN) != 0) {
        Print (L" OUT");
      }
      if ((Widgets[Index].DefaultPinControl & HDA_PIN_WIDGET_CONTROL_HP_EN) != 0) {
        Print (L" HP");
      }
      Print (L"\n");
    }

    //
    // Print connections.
    //
    if ((Widgets[Index].Capabilities & HDA_PARAMETER_WIDGET_CAPS_CONN_LIST) != 0) {
      Print (
        L"  Connection: %u\n    ",
        HDA_PARAMETER_CONN_LIST_LENGTH_LEN (Widgets[Index].ConnectionListLength)
        );
      for (Index2 = 0; Index2 < HDA_PARAMETER_CONN_LIST_LENGTH_LEN (Widgets[Index].ConnectionListLength); ++Index2) {
        Print (L" %2u", Widgets[Index].Connections[Index2]);
      }
      Print (L"\n");
    }
  }
}

EFI_STATUS
EFIAPI
HdaCodecDumpMain (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HdaCodecHandles;
  UINTN                         HdaCodecHandleCount;
  EFI_HDA_CODEC_INFO_PROTOCOL   *HdaCodecInfo;
  UINTN                         Index;
  CONST CHAR16                  *Name;
  UINT32                        VendorId;
  UINT32                        RevisionId;
  UINT32                        Rates;
  UINT32                        Formats;
  UINT32                        AmpInCaps;
  UINT32                        AmpOutCaps;
  HDA_WIDGET                    *Widgets;
  UINTN                         WidgetCount;
  UINT8                         AudioFuncId;
  BOOLEAN                       Unsol;

  Print (L"HdaCodecDump start\n");
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiHdaCodecInfoProtocolGuid,
    NULL,
    &HdaCodecHandleCount,
    &HdaCodecHandles
    );
  if (EFI_ERROR (Status)) {
    Print (L"No audio devices were found (%r). Ensure AudioDxe is loaded.\n", Status);
    return Status;
  }

  //
  // Print each codec found.
  //
  for (Index = 0; Index < HdaCodecHandleCount; ++Index) {
    Status = gBS->HandleProtocol (
      HdaCodecHandles[Index],
      &gEfiHdaCodecInfoProtocolGuid,
      (VOID**) &HdaCodecInfo
      );
    if (EFI_ERROR (Status)) {
      Print (
        L"Cannot open audio protocol for %p handle - %r.\n",
        HdaCodecHandles[Index],
        Status
        );
      continue;
    }

    //
    // Get name.
    //
    Status = HdaCodecInfo->GetName (HdaCodecInfo, &Name);
    if (!EFI_ERROR (Status)) {
      Print (L"Codec: %s\n", Name);
    }

    //
    // Get AFG ID.
    //
    Status = HdaCodecInfo->GetAudioFuncId (HdaCodecInfo, &AudioFuncId, &Unsol);
    if (!EFI_ERROR (Status)) {
      Print (L"AFG Function Id: 0x%X (unsol %d)\n", AudioFuncId, Unsol);
    }

    //
    // Get vendor.
    //
    Status = HdaCodecInfo->GetVendorId (HdaCodecInfo, &VendorId);
    if (!EFI_ERROR (Status)) {
      Print (L"Vendor ID: 0x%X\n", VendorId);
    }

    //
    // Get revision.
    //
    Status = HdaCodecInfo->GetRevisionId (HdaCodecInfo, &RevisionId);
    if (!EFI_ERROR (Status)) {
      Print (L"Revision ID: 0x%X\n", RevisionId);
    }

    //
    // Get supported rates/formats.
    //
    Status = HdaCodecInfo->GetDefaultRatesFormats (HdaCodecInfo, &Rates, &Formats);
    if (!EFI_ERROR (Status) && (Rates != 0 || Formats != 0)) {
      Print (L"Default PCM:\n");
      HdaCodecDumpPrintRatesFormats(Rates, Formats);
    } else {
      Print (L"Default PCM: N/A\n");
    }

    //
    // Get default amp caps.
    //
    Status = HdaCodecInfo->GetDefaultAmpCaps(HdaCodecInfo, &AmpInCaps, &AmpOutCaps);
    if (!EFI_ERROR (Status)) {
      Print (L"Default Amp-In caps: ");
      HdaCodecDumpPrintAmpCaps (AmpInCaps);
      Print (L"Default Amp-Out caps: ");
      HdaCodecDumpPrintAmpCaps (AmpOutCaps);
    }

    //
    // Get widgets.
    //
    Status = HdaCodecInfo->GetWidgets(HdaCodecInfo, &Widgets, &WidgetCount);
    if (!EFI_ERROR (Status)) {
      HdaCodecDumpPrintWidgets (Widgets, WidgetCount);
      HdaCodecInfo->FreeWidgetsBuffer(Widgets, WidgetCount);
    }
  }

  return EFI_SUCCESS;
}
