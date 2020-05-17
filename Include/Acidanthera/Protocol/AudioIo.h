/*
 * File: AudioIo.h
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

#ifndef EFI_AUDIO_IO_H
#define EFI_AUDIO_IO_H

#include <Uefi.h>

/**
  Audio I/O protocol GUID.
**/
#define EFI_AUDIO_IO_PROTOCOL_GUID \
  { 0xF05B559C, 0x1971, 0x4AF5,    \
    { 0xB2, 0xAE, 0xD6, 0x08, 0x08, 0xF7, 0x4F, 0x70 } }

typedef struct EFI_AUDIO_IO_PROTOCOL_ EFI_AUDIO_IO_PROTOCOL;

/**
  Port type.
**/
typedef enum {
  EfiAudioIoTypeOutput,
  EfiAudioIoTypeInput,
  EfiAudioIoTypeMaximum
} EFI_AUDIO_IO_PROTOCOL_TYPE;

/**
  Device type.
**/
typedef enum {
  EfiAudioIoDeviceLine,
  EfiAudioIoDeviceSpeaker,
  EfiAudioIoDeviceHeadphones,
  EfiAudioIoDeviceSpdif,
  EfiAudioIoDeviceMic,
  EfiAudioIoDeviceHdmi,
  EfiAudioIoDeviceOther,
  EfiAudioIoDeviceMaximum
} EFI_AUDIO_IO_PROTOCOL_DEVICE;

/**
  Port location.
**/
typedef enum {
  EfiAudioIoLocationNone,
  EfiAudioIoLocationRear,
  EfiAudioIoLocationFront,
  EfiAudioIoLocationLeft,
  EfiAudioIoLocationRight,
  EfiAudioIoLocationTop,
  EfiAudioIoLocationBottom,
  EfiAudioIoLocationOther,
  EfiAudioIoLocationMaximum
} EFI_AUDIO_IO_PROTOCOL_LOCATION;

/**
  Port surface.
**/
typedef enum {
  EfiAudioIoSurfaceExternal,
  EfiAudioIoSurfaceInternal,
  EfiAudioIoSurfaceOther,
  EfiAudioIoSurfaceMaximum
} EFI_AUDIO_IO_PROTOCOL_SURFACE;

/**
  Size in bits of each sample.
**/
typedef enum {
  EfiAudioIoBits8     = BIT0,
  EfiAudioIoBits16    = BIT1,
  EfiAudioIoBits20    = BIT2,
  EfiAudioIoBits24    = BIT3,
  EfiAudioIoBits32    = BIT4
} EFI_AUDIO_IO_PROTOCOL_BITS;

/**
  Frequency of each sample.
**/
typedef enum {
  EfiAudioIoFreq8kHz      = BIT0,
  EfiAudioIoFreq11kHz     = BIT1,
  EfiAudioIoFreq16kHz     = BIT2,
  EfiAudioIoFreq22kHz     = BIT3,
  EfiAudioIoFreq32kHz     = BIT4,
  EfiAudioIoFreq44kHz     = BIT5,
  EfiAudioIoFreq48kHz     = BIT6,
  EfiAudioIoFreq88kHz     = BIT7,
  EfiAudioIoFreq96kHz     = BIT8,
  EfiAudioIoFreq192kHz    = BIT9
} EFI_AUDIO_IO_PROTOCOL_FREQ;

/**
  Audio input/output structure.
**/
typedef struct {
  EFI_AUDIO_IO_PROTOCOL_TYPE      Type;
  EFI_AUDIO_IO_PROTOCOL_BITS      SupportedBits;
  EFI_AUDIO_IO_PROTOCOL_FREQ      SupportedFreqs;
  EFI_AUDIO_IO_PROTOCOL_DEVICE    Device;
  EFI_AUDIO_IO_PROTOCOL_LOCATION  Location;
  EFI_AUDIO_IO_PROTOCOL_SURFACE   Surface;
} EFI_AUDIO_IO_PROTOCOL_PORT;

/**
  Maximum number of channels.
**/
#define EFI_AUDIO_IO_PROTOCOL_MAX_CHANNELS 16
#define EFI_AUDIO_IO_PROTOCOL_MAX_VOLUME   100

/**
  Callback function.
**/
typedef
VOID
(EFIAPI* EFI_AUDIO_IO_CALLBACK) (
  IN EFI_AUDIO_IO_PROTOCOL        *AudioIo,
  IN VOID                         *Context
  );

/**
  Gets the collection of output ports.

  @param[in]  This              A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[out] OutputPorts       A pointer to a buffer where the output ports will be placed.
  @param[out] OutputPortsCount  The number of ports in OutputPorts.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_IO_GET_OUTPUTS) (
  IN  EFI_AUDIO_IO_PROTOCOL       *This,
  OUT EFI_AUDIO_IO_PROTOCOL_PORT  **OutputPorts,
  OUT UINTN                       *OutputPortsCount
  );

/**
  Sets up the device to play audio data.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in] OutputIndex        The zero-based index of the desired output.
  @param[in] Volume             The volume (0-100) to use.
  @param[in] Bits               The width in bits of the source data.
  @param[in] Freq               The frequency of the source data.
  @param[in] Channels           The number of channels the source data contains.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_IO_SETUP_PLAYBACK) (
  IN EFI_AUDIO_IO_PROTOCOL        *This,
  IN UINT8                        OutputIndex,
  IN UINT8                        Volume,
  IN EFI_AUDIO_IO_PROTOCOL_FREQ   Freq,
  IN EFI_AUDIO_IO_PROTOCOL_BITS   Bits,
  IN UINT8                        Channels
  );

/**
  Begins playback on the device and waits for playback to complete.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in] Data               A pointer to the buffer containing the audio data to play.
  @param[in] DataLength         The size, in bytes, of the data buffer specified by Data.
  @param[in] Position           The position in the buffer to start at.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_IO_START_PLAYBACK) (
  IN EFI_AUDIO_IO_PROTOCOL        *This,
  IN VOID                         *Data,
  IN UINTN                        DataLength,
  IN UINTN                        Position OPTIONAL
  );

/**
  Begins playback on the device asynchronously.
  The callback if specified will be executed with TPL_NOTIFY.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in] Data               A pointer to the buffer containing the audio data to play.
  @param[in] DataLength         The size, in bytes, of the data buffer specified by Data.
  @param[in] Position           The position in the buffer to start at.
  @param[in] Callback           A pointer to an optional callback to be invoked when playback is complete.
  @param[in] Context            A pointer to data to be passed to the callback function.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_IO_START_PLAYBACK_ASYNC) (
  IN EFI_AUDIO_IO_PROTOCOL        *This,
  IN VOID                         *Data,
  IN UINTN                        DataLength,
  IN UINTN                        Position     OPTIONAL,
  IN EFI_AUDIO_IO_CALLBACK        Callback     OPTIONAL,
  IN VOID                         *Context     OPTIONAL
  );

/**
  Stops playback on the device.
  Note, this will not call registered callbacks for stop audio.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_IO_STOP_PLAYBACK) (
  IN EFI_AUDIO_IO_PROTOCOL        *This
  );

/**
  Protocol struct.
**/
struct EFI_AUDIO_IO_PROTOCOL_ {
  EFI_AUDIO_IO_GET_OUTPUTS            GetOutputs;
  EFI_AUDIO_IO_SETUP_PLAYBACK         SetupPlayback;
  EFI_AUDIO_IO_START_PLAYBACK         StartPlayback;
  EFI_AUDIO_IO_START_PLAYBACK_ASYNC   StartPlaybackAsync;
  EFI_AUDIO_IO_STOP_PLAYBACK          StopPlayback;
};

extern EFI_GUID gEfiAudioIoProtocolGuid;

#endif // EFI_AUDIO_IO_H
