/** @file
  Copyright (C) 2018-2021, John Davis, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Guid/AppleVariable.h>

#include "HdaCodec.h"
#include <Protocol/AudioIo.h>
#include <Library/OcMiscLib.h>
#include <Library/PcdLib.h>

//
// Cache playback setup.
//
STATIC UINT64                     mOutputIndexMask = 0;
STATIC INT8                       mGain = APPLE_SYSTEM_AUDIO_VOLUME_DB_MIN;
STATIC EFI_AUDIO_IO_PROTOCOL_FREQ mFreq = 0;
STATIC EFI_AUDIO_IO_PROTOCOL_BITS mBits = 0;
STATIC UINT8                      mChannels = 0xFFU;

// HDA I/O Stream callback.
VOID
EFIAPI
HdaCodecHdaIoStreamCallback(
  IN EFI_HDA_IO_PROTOCOL_TYPE Type,
  IN VOID *Context1,
  IN VOID *Context2,
  IN VOID *Context3) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecHdaIoStreamCallback(): start\n"));

  // Create variables.
  EFI_AUDIO_IO_PROTOCOL *AudioIo = (EFI_AUDIO_IO_PROTOCOL*)Context1;
  EFI_AUDIO_IO_CALLBACK AudioIoCallback = (EFI_AUDIO_IO_CALLBACK)Context2;

  // Ensure required parameters are valid.
  if ((AudioIo == NULL) || (AudioIoCallback == NULL))
    return;

  // Invoke callback.
  AudioIoCallback(AudioIo, Context3);
}

/**
  Gets the collection of output ports.

  @param[in]  This              A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[out] OutputPorts       A pointer to a buffer where the output ports will be placed.
  @param[out] OutputPortsCount  The number of ports in OutputPorts.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecAudioIoGetOutputs(
  IN  EFI_AUDIO_IO_PROTOCOL *This,
  OUT EFI_AUDIO_IO_PROTOCOL_PORT **OutputPorts,
  OUT UINTN *OutputPortsCount) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecAudioIoGetOutputs(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  AUDIO_IO_PRIVATE_DATA *AudioIoPrivateData;
  HDA_CODEC_DEV *HdaCodecDev;
  EFI_AUDIO_IO_PROTOCOL_PORT *HdaOutputPorts;
  UINT32 SupportedRates;

  // If a parameter is invalid, return error.
  if ((This == NULL) || (OutputPorts == NULL) ||
    (OutputPortsCount == NULL))
    return EFI_INVALID_PARAMETER;

  // Get private data.
  AudioIoPrivateData = AUDIO_IO_PRIVATE_DATA_FROM_THIS(This);
  HdaCodecDev = AudioIoPrivateData->HdaCodecDev;

  // Allocate buffer.
  HdaOutputPorts = AllocateZeroPool(sizeof(EFI_AUDIO_IO_PROTOCOL_PORT) * HdaCodecDev->OutputPortsCount);
  if (HdaOutputPorts == NULL)
    return EFI_OUT_OF_RESOURCES;

  // Get output ports.
  for (UINTN i = 0; i < HdaCodecDev->OutputPortsCount; i++) {
    // Port is an output.
    HdaOutputPorts[i].Type = EfiAudioIoTypeOutput;

    // Get device type.
    switch (HDA_VERB_GET_CONFIGURATION_DEFAULT_DEVICE(HdaCodecDev->OutputPorts[i]->DefaultConfiguration)) {
      case HDA_CONFIG_DEFAULT_DEVICE_LINE_OUT:
      case HDA_CONFIG_DEFAULT_DEVICE_LINE_IN:
        HdaOutputPorts[i].Device = EfiAudioIoDeviceLine;
        break;

      case HDA_CONFIG_DEFAULT_DEVICE_SPEAKER:
        HdaOutputPorts[i].Device = EfiAudioIoDeviceSpeaker;
        break;

      case HDA_CONFIG_DEFAULT_DEVICE_HEADPHONE_OUT:
        HdaOutputPorts[i].Device = EfiAudioIoDeviceHeadphones;
        break;

      case HDA_CONFIG_DEFAULT_DEVICE_SPDIF_OUT:
      case HDA_CONFIG_DEFAULT_DEVICE_SPDIF_IN:
        HdaOutputPorts[i].Device = EfiAudioIoDeviceSpdif;
        break;

      case HDA_CONFIG_DEFAULT_DEVICE_MIC_IN:
        HdaOutputPorts[i].Device = EfiAudioIoDeviceMic;
        break;

      default:
        if (HdaCodecDev->OutputPorts[i]->PinCapabilities & HDA_PARAMETER_PIN_CAPS_HDMI)
          HdaOutputPorts[i].Device = EfiAudioIoDeviceHdmi;
        else
          HdaOutputPorts[i].Device = EfiAudioIoDeviceOther;
    }

    // Get location.
    switch (HDA_VERB_GET_CONFIGURATION_DEFAULT_LOC(HdaCodecDev->OutputPorts[i]->DefaultConfiguration)) {
      case HDA_CONFIG_DEFAULT_LOC_SPEC_NA:
        HdaOutputPorts[i].Location = EfiAudioIoLocationNone;
        break;

      case HDA_CONFIG_DEFAULT_LOC_SPEC_REAR:
        HdaOutputPorts[i].Location = EfiAudioIoLocationRear;
        break;

      case HDA_CONFIG_DEFAULT_LOC_SPEC_FRONT:
        HdaOutputPorts[i].Location = EfiAudioIoLocationFront;
        break;

      case HDA_CONFIG_DEFAULT_LOC_SPEC_LEFT:
        HdaOutputPorts[i].Location = EfiAudioIoLocationLeft;
        break;

      case HDA_CONFIG_DEFAULT_LOC_SPEC_RIGHT:
        HdaOutputPorts[i].Location = EfiAudioIoLocationRight;
        break;

      case HDA_CONFIG_DEFAULT_LOC_SPEC_TOP:
        HdaOutputPorts[i].Location = EfiAudioIoLocationTop;
        break;

      case HDA_CONFIG_DEFAULT_LOC_SPEC_BOTTOM:
        HdaOutputPorts[i].Location = EfiAudioIoLocationBottom;
        break;

      default:
        HdaOutputPorts[i].Location = EfiAudioIoLocationOther;
    }

    // Get surface.
    switch (HDA_VERB_GET_CONFIGURATION_DEFAULT_SURF(HdaCodecDev->OutputPorts[i]->DefaultConfiguration)) {
      case HDA_CONFIG_DEFAULT_LOC_SURF_EXTERNAL:
        HdaOutputPorts[i].Surface = EfiAudioIoSurfaceExternal;
        break;

      case HDA_CONFIG_DEFAULT_LOC_SURF_INTERNAL:
        HdaOutputPorts[i].Surface = EfiAudioIoSurfaceInternal;
        break;

      default:
        HdaOutputPorts[i].Surface = EfiAudioIoSurfaceOther;
    }

    // Get supported stream formats.
    Status = HdaCodecGetSupportedPcmRates(HdaCodecDev->OutputPorts[i], &SupportedRates);
    if (EFI_ERROR(Status))
      return Status;

    // Get supported bit depths.
    HdaOutputPorts[i].SupportedBits = 0;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8BIT)
      HdaOutputPorts[i].SupportedBits |= EfiAudioIoBits8;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16BIT)
      HdaOutputPorts[i].SupportedBits |= EfiAudioIoBits16;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_20BIT)
      HdaOutputPorts[i].SupportedBits |= EfiAudioIoBits20;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_24BIT)
      HdaOutputPorts[i].SupportedBits |= EfiAudioIoBits24;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32BIT)
      HdaOutputPorts[i].SupportedBits |= EfiAudioIoBits32;

    // Get supported sample rates.
    HdaOutputPorts[i].SupportedFreqs = 0;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq8kHz;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_11KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq11kHz;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq16kHz;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_22KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq22kHz;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq32kHz;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_44KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq44kHz;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_48KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq48kHz;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_88KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq88kHz;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_96KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq96kHz;
    if (SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_192KHZ)
      HdaOutputPorts[i].SupportedFreqs |= EfiAudioIoFreq192kHz;
  }

  // Ports gotten successfully.
  *OutputPorts = HdaOutputPorts;
  *OutputPortsCount = HdaCodecDev->OutputPortsCount;
  return EFI_SUCCESS;
}

/**
  Convert raw amplifier gain setting to decibel gain value; converts using the parameters of the first channel specified
  in OutputIndexMask which has non-zero amp capabilities.
  Note: It seems very typical - though it is certainly not required by the spec - that all amps on a codec which have
  non-zero amp capabilities, actually all have the same params as each other.

  @param[in]  This              A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in]  OutputIndexMask   A mask indicating the desired outputs.
  @param[in]  GainParam         The raw gain parameter for the amplifier.
  @param[out] Gain              The amplifier gain (or attentuation if negative) in dB to use, relative to 0 dB level (0 dB
                                is usually at at or near max available volume, but is not required to be so in the spec).

  @retval EFI_SUCCESS           The gain value was calculated successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecAudioIoRawGainToDecibels (
  IN  EFI_AUDIO_IO_PROTOCOL       *This,
  IN  UINT64                      OutputIndexMask,
  IN  UINT8                       GainParam,
  OUT INT8                        *Gain
  )
{
  EFI_STATUS            Status;
  AUDIO_IO_PRIVATE_DATA *AudioIoPrivateData;
  HDA_CODEC_DEV         *HdaCodecDev;
  UINTN                 Index;
  UINT64                IndexMask;

  // If a parameter is invalid, return error.
  if (This == NULL || Gain == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Get private data.
  AudioIoPrivateData = AUDIO_IO_PRIVATE_DATA_FROM_THIS (This);
  HdaCodecDev = AudioIoPrivateData->HdaCodecDev;

  Status = EFI_NOT_FOUND;

  // Try to convert on requested outputs.
  for (Index = 0, IndexMask = 1; Index < HdaCodecDev->OutputPortsCount; Index++, IndexMask <<= 1) {
    if ((OutputIndexMask & IndexMask) == 0) {
      continue;
    }
    Status = HdaCodecWidgetRawGainToDecibels (HdaCodecDev->OutputPorts[Index], GainParam, Gain);
    if (!EFI_ERROR (Status) || Status != EFI_NOT_FOUND) {
      return Status;
    }
  }

  return Status;
}

/**
  Sets up the device to play audio data. Basic caching is implemented: no actions are taken
  the second and subsequent times that set up is called again with exactly the same paremeters. 

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in] OutputIndexMask    A mask indicating the desired outputs.
  @param[in] Gain               The amplifier gain (or attentuation if negative) in dB to use, relative to 0 dB level (0 dB
                                is usually at at or near max available volume, but is not required to be so in the spec).
  @param[in] Bits               The width in bits of the source data.
  @param[in] Freq               The frequency of the source data.
  @param[in] Channels           The number of channels the source data contains.
  @param[in] PlaybackDelay      The required delay before playback after a change in setup.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecAudioIoSetupPlayback(
  IN EFI_AUDIO_IO_PROTOCOL *This,
  IN UINT64 OutputIndexMask,
  IN INT8 Gain,
  IN EFI_AUDIO_IO_PROTOCOL_FREQ Freq,
  IN EFI_AUDIO_IO_PROTOCOL_BITS Bits,
  IN UINT8 Channels,
  IN UINTN PlaybackDelay
  )
{
  EFI_STATUS Status;
  AUDIO_IO_PRIVATE_DATA *AudioIoPrivateData;
  HDA_CODEC_DEV *HdaCodecDev;
  EFI_HDA_IO_PROTOCOL *HdaIo;
  UINTN Index;
  UINT64 IndexMask;
  UINT32 Response;
  UINT8 NumGpios;
  UINT8 ChannelPayload;

  // Widgets.
  HDA_WIDGET_DEV *PinWidget;
  HDA_WIDGET_DEV *OutputWidget;
  UINT32 SupportedRates;
  UINT8 HdaStreamId;

  // Stream.
  UINT8 StreamBits;
  UINT8 StreamDiv;
  UINT8 StreamMult;
  BOOLEAN StreamBase44kHz;
  UINT16 StreamFmt;

  DEBUG((DEBUG_VERBOSE, "HdaCodecAudioIoSetupPlayback(): start\n"));

  // Basic settings caching.
  if (mOutputIndexMask == OutputIndexMask
      && mGain == Gain
      && mFreq == Freq
      && mBits == Bits
      && mChannels == Channels) {
    return EFI_SUCCESS;
  }

  mOutputIndexMask = OutputIndexMask;
  mGain = Gain;
  mFreq = Freq;
  mBits = Bits;
  mChannels = Channels;

  // If a parameter is invalid, return error.
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Get private data.
  AudioIoPrivateData = AUDIO_IO_PRIVATE_DATA_FROM_THIS(This);
  HdaCodecDev = AudioIoPrivateData->HdaCodecDev;
  HdaIo = HdaCodecDev->HdaIo;

  // Mask to only outputs which are within bounds.
  if (HdaCodecDev->OutputPortsCount > sizeof (UINT64) * OC_CHAR_BIT) {
    return EFI_UNSUPPORTED;
  }
  OutputIndexMask &= ~LShiftU64(MAX_UINT64, HdaCodecDev->OutputPortsCount);

  // Fail visibily if nothing is requested.
  if (PcdGetBool (PcdAudioCodecErrorOnNoOutputs) && OutputIndexMask == 0) {
    return EFI_INVALID_PARAMETER;
  }

  // Avoid Coverity warnings (the bit mask checks actually ensure that these cannot be used uninitialised).
  StreamBits = 0;
  StreamDiv = 0;
  StreamMult = 0;
  StreamBase44kHz = FALSE;

  // Expand the requested stream frequency and sample size params,
  // and check that every requested channel can support them.
  for (Index = 0, IndexMask = 1; Index < HdaCodecDev->OutputPortsCount; Index++, IndexMask <<= 1) {
    if ((OutputIndexMask & IndexMask) == 0) {
      continue;
    }

    PinWidget = HdaCodecDev->OutputPorts[Index];

    // Get the output DAC for the path.
    Status = HdaCodecGetOutputDac(PinWidget, &OutputWidget);
    if (EFI_ERROR(Status))
      return Status;

    // Get supported stream formats.
    Status = HdaCodecGetSupportedPcmRates(OutputWidget, &SupportedRates);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_INFO, "HdaCodecGetSupportedPcmRates(): failure - %r\n", Status));
      return Status;
    }

    // Determine bitness of samples, ensuring desired bitness is supported.
    switch (Bits) {
      // 8-bit.
      case EfiAudioIoBits8:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8BIT))
          return EFI_UNSUPPORTED;
        StreamBits = HDA_CONVERTER_FORMAT_BITS_8;
        break;

      // 16-bit.
      case EfiAudioIoBits16:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16BIT))
          return EFI_UNSUPPORTED;
        StreamBits = HDA_CONVERTER_FORMAT_BITS_16;
        break;

      // 20-bit.
      case EfiAudioIoBits20:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_20BIT))
          return EFI_UNSUPPORTED;
        StreamBits = HDA_CONVERTER_FORMAT_BITS_20;
        break;

      // 24-bit.
      case EfiAudioIoBits24:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_24BIT))
          return EFI_UNSUPPORTED;
        StreamBits = HDA_CONVERTER_FORMAT_BITS_24;
        break;

      // 32-bit.
      case EfiAudioIoBits32:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32BIT))
          return EFI_UNSUPPORTED;
        StreamBits = HDA_CONVERTER_FORMAT_BITS_32;
        break;

      // Others.
      default:
        return EFI_INVALID_PARAMETER;
    }

    // Determine base, divisor, and multipler.
    switch (Freq) {
      // 8 kHz.
      case EfiAudioIoFreq8kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = FALSE;
        StreamDiv = 6;
        StreamMult = 1;
        break;

      // 11.025 kHz.
      case EfiAudioIoFreq11kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_11KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = FALSE; ///< TODO: Why does Clover has TRUE here?
        StreamDiv = 4;
        StreamMult = 1;
        break;

      // 16 kHz.
      case EfiAudioIoFreq16kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = FALSE;
        StreamDiv = 3;
        StreamMult = 1;
        break;

      // 22.05 kHz.
      case EfiAudioIoFreq22kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_22KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = FALSE; ///< TODO: Why does Clover has TRUE here?
        StreamDiv = 2;
        StreamMult = 1;
        break;

      // 32 kHz.
      case EfiAudioIoFreq32kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = FALSE;
        StreamDiv = 3;
        StreamMult = 2;
        break;

      // 44.1 kHz.
      case EfiAudioIoFreq44kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_44KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = TRUE;
        StreamDiv = 1;
        StreamMult = 1;
        break;

      // 48 kHz.
      case EfiAudioIoFreq48kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_48KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = FALSE;
        StreamDiv = 1;
        StreamMult = 1;
        break;

      // 88 kHz.
      case EfiAudioIoFreq88kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_88KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = TRUE;
        StreamDiv = 1;
        StreamMult = 2;
        break;

      // 96 kHz.
      case EfiAudioIoFreq96kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_96KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = FALSE;
        StreamDiv = 1;
        StreamMult = 2;
        break;

      // 192 kHz.
      case EfiAudioIoFreq192kHz:
        if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_192KHZ))
          return EFI_UNSUPPORTED;
        StreamBase44kHz = FALSE;
        StreamDiv = 1;
        StreamMult = 4;
        break;

      // Others.
      default:
        return EFI_INVALID_PARAMETER;
    }
  }

  // Disable all non-selected widget paths.
  for (Index = 0, IndexMask = 1; Index < HdaCodecDev->OutputPortsCount; Index++, IndexMask <<= 1) {
    if ((OutputIndexMask & IndexMask) != 0) {
      continue;
    }
    Status = HdaCodecDisableWidgetPath(HdaCodecDev->OutputPorts[Index]);
    if (EFI_ERROR(Status))
      return Status;
  }

  // Close stream first.
  Status = HdaIo->CloseStream(HdaIo, EfiHdaIoTypeOutput);
  if (EFI_ERROR(Status))
    return Status;

  // Save requested outputs.
  AudioIoPrivateData->SelectedOutputIndexMask = OutputIndexMask;

  // Nothing to play.
  if (OutputIndexMask == 0) {
    return EFI_SUCCESS;
  }

  // Calculate stream format and setup stream.
  StreamFmt = HDA_CONVERTER_FORMAT_SET(Channels - 1, StreamBits,
    StreamDiv - 1, StreamMult - 1, StreamBase44kHz);
  DEBUG((DEBUG_VERBOSE, "HdaCodecAudioIoPlay(): Stream format 0x%X\n", StreamFmt));
  Status = HdaIo->SetupStream(HdaIo, EfiHdaIoTypeOutput, StreamFmt, &HdaStreamId);
  if (EFI_ERROR(Status))
    return Status;

  // Setup widget path for desired outputs.
  for (Index = 0, IndexMask = 1; Index < HdaCodecDev->OutputPortsCount; Index++, IndexMask <<= 1) {
    if ((OutputIndexMask & IndexMask) == 0) {
      continue;
    }
    Status = HdaCodecEnableWidgetPath(HdaCodecDev->OutputPorts[Index], Gain, HdaStreamId, StreamFmt);
    if (EFI_ERROR(Status))
      goto CLOSE_STREAM;
  }

  //
  // Enable GPIO pins. This is similar to how Linux drivers enable audio on e.g. Cirrus Logic and
  // Realtek devices on Mac, by enabling specific GPIO pins as required.
  // REF:
  // - https://github.com/torvalds/linux/blob/6f513529296fd4f696afb4354c46508abe646541/sound/pci/hda/patch_cirrus.c#L43-L57
  // - https://github.com/torvalds/linux/blob/6f513529296fd4f696afb4354c46508abe646541/sound/pci/hda/patch_cirrus.c#L493-L517
  // - https://github.com/torvalds/linux/blob/6f513529296fd4f696afb4354c46508abe646541/sound/pci/hda/patch_realtek.c#L244-L258
  //
  // Note 1: On at least one WWHC system, waiting for current sound to finish playing stopped working when this was
  // applied automatically, whereas everything already worked fine without this, so we now need to require users to
  // specify at least `-gpio-setup` (which means: all stages, automatic pins') in the driver params, to use this.
  //
  // Note 2: So far we have found no need to specify anything other than 'all stages, automatic (i.e. all) pins', although
  // we do know that not all systems require all stages (e.g. MBP10,2 only requires DATA stage; but based on Linux
  // drivers, it is likely that some do), and almost certainly no systems require all pins.
  //
  if (gGpioSetupStageMask != 0) {
    if (gGpioPinMask != 0) {
      //
      // Enable user-specified pins.
      //
      ChannelPayload = (UINT8)gGpioPinMask;
    } else {
      NumGpios = HDA_PARAMETER_GPIO_COUNT_NUM_GPIOS (HdaCodecDev->AudioFuncGroup->GpioCapabilities);
      //
      // According to Intel HDA spec this can be from 0 to 7, however we
      // have seen 8 in the wild, and values up to 8 are perfectly usable.
      // REF: https://github.com/acidanthera/bugtracker/issues/740#issuecomment-1005279476
      //
      if (NumGpios > 8) {
        Status = EFI_INVALID_PARAMETER;
        goto CLOSE_STREAM;
      }
      ChannelPayload = (1 << NumGpios) - 1; ///< Enable all available pins
    }

    if (ChannelPayload != 0) {
      Status = EFI_SUCCESS;

      if ((gGpioSetupStageMask & GPIO_SETUP_STAGE_ENABLE) != 0) {
        Status = HdaIo->SendCommand (
          HdaIo,
          HdaCodecDev->AudioFuncGroup->NodeId,
          HDA_CODEC_VERB (HDA_VERB_SET_GPIO_ENABLE_MASK, ChannelPayload),
          &Response
          );
      }

      if (!EFI_ERROR (Status)
        && (gGpioSetupStageMask & GPIO_SETUP_STAGE_DIRECTION) != 0) {
        Status = HdaIo->SendCommand (
          HdaIo,
          HdaCodecDev->AudioFuncGroup->NodeId,
          HDA_CODEC_VERB (HDA_VERB_SET_GPIO_DIRECTION, ChannelPayload),
          &Response
          );
      }

      if (!EFI_ERROR (Status)
        && (gGpioSetupStageMask & GPIO_SETUP_STAGE_DATA) != 0) {
        gBS->Stall (MS_TO_MICROSECONDS (1));
        Status = HdaIo->SendCommand (
          HdaIo,
          HdaCodecDev->AudioFuncGroup->NodeId,
          HDA_CODEC_VERB (HDA_VERB_SET_GPIO_DATA, ChannelPayload),
          &Response
          );
      }

      DEBUG ((
        EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
        "HDA: GPIO setup on pins 0x%02X - %r\n",
        ChannelPayload,
        Status
        ));

      if (EFI_ERROR(Status))
        goto CLOSE_STREAM;
    }
  }
  
  //
  // We are required to wait for some time after codec setup on some systems.
  // REF: https://github.com/acidanthera/bugtracker/issues/971
  //
  if (PlaybackDelay > 0) {
    gBS->Stall (PlaybackDelay);
  }

  return EFI_SUCCESS;

CLOSE_STREAM:
  // Close stream.
  HdaIo->CloseStream(HdaIo, EfiHdaIoTypeOutput);
  return Status;
}

/**
  Begins playback on the device and waits for playback to complete.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in] Data               A pointer to the buffer containing the audio data to play.
  @param[in] DataLength         The size, in bytes, of the data buffer specified by Data.
  @param[in] Position           The position in the buffer to start at.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecAudioIoStartPlayback(
  IN EFI_AUDIO_IO_PROTOCOL *This,
  IN VOID *Data,
  IN UINTN DataLength,
  IN UINTN Position OPTIONAL) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecAudioIoStartPlayback(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  AUDIO_IO_PRIVATE_DATA *AudioIoPrivateData;
  EFI_HDA_IO_PROTOCOL *HdaIo;
  BOOLEAN StreamRunning;

  // If a parameter is invalid, return error.
  if ((This == NULL) || (Data == NULL) || (DataLength == 0))
    return EFI_INVALID_PARAMETER;

  // Get private data.
  AudioIoPrivateData = AUDIO_IO_PRIVATE_DATA_FROM_THIS(This);
  HdaIo = AudioIoPrivateData->HdaCodecDev->HdaIo;

  // Nothing to play.
  if (AudioIoPrivateData->SelectedOutputIndexMask == 0) {
    return EFI_SUCCESS;
  }

  // Start stream.
  Status = HdaIo->StartStream(HdaIo, EfiHdaIoTypeOutput, Data, DataLength, Position,
    NULL, NULL, NULL, NULL);
  if (EFI_ERROR(Status))
    return Status;

  // Wait for stream to stop.
  StreamRunning = TRUE;
  while (StreamRunning) {
    Status = HdaIo->GetStream(HdaIo, EfiHdaIoTypeOutput, &StreamRunning);
    if (EFI_ERROR(Status)) {
      HdaIo->StopStream(HdaIo, EfiHdaIoTypeOutput);
      return Status;
    }

    // Wait 100ms.
    //gBS->Stall (MS_TO_MICROSECONDS (100));
  }
  return EFI_SUCCESS;
}

/**
  Begins playback on the device asynchronously.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in] Data               A pointer to the buffer containing the audio data to play.
  @param[in] DataLength         The size, in bytes, of the data buffer specified by Data.
  @param[in] Position           The position in the buffer to start at.
  @param[in] Callback           A pointer to an optional callback to be invoked when playback is complete.
  @param[in] Context            A pointer to data to be passed to the callback function.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecAudioIoStartPlaybackAsync(
  IN EFI_AUDIO_IO_PROTOCOL *This,
  IN VOID *Data,
  IN UINTN DataLength,
  IN UINTN Position OPTIONAL,
  IN EFI_AUDIO_IO_CALLBACK Callback OPTIONAL,
  IN VOID *Context OPTIONAL) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecAudioIoStartPlaybackAsync(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  AUDIO_IO_PRIVATE_DATA *AudioIoPrivateData;
  EFI_HDA_IO_PROTOCOL *HdaIo;

  // If a parameter is invalid, return error.
  if ((This == NULL) || (Data == NULL) || (DataLength == 0))
    return EFI_INVALID_PARAMETER;

  // Get private data.
  AudioIoPrivateData = AUDIO_IO_PRIVATE_DATA_FROM_THIS(This);
  HdaIo = AudioIoPrivateData->HdaCodecDev->HdaIo;

  // Start stream.
  Status = HdaIo->StartStream(HdaIo, EfiHdaIoTypeOutput, Data, DataLength, Position,
    HdaCodecHdaIoStreamCallback, (VOID*)This, (VOID*)Callback, Context);
  return Status;
}

/**
  Stops playback on the device.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecAudioIoStopPlayback(
  IN EFI_AUDIO_IO_PROTOCOL *This) {
  DEBUG((DEBUG_VERBOSE, "HdaCodecAudioIoStopPlayback(): start\n"));

  // Create variables.
  AUDIO_IO_PRIVATE_DATA *AudioIoPrivateData;
  EFI_HDA_IO_PROTOCOL *HdaIo;

  // If a parameter is invalid, return error.
  if (This == NULL)
    return EFI_INVALID_PARAMETER;

  // Get private data.
  AudioIoPrivateData = AUDIO_IO_PRIVATE_DATA_FROM_THIS(This);
  HdaIo = AudioIoPrivateData->HdaCodecDev->HdaIo;

  // Stop stream.
  return HdaIo->StopStream(HdaIo, EfiHdaIoTypeOutput);
}
