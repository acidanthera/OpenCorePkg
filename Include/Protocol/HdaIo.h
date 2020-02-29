/*
 * File: HdaIo.h
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

#ifndef EFI_HDA_IO_H
#define EFI_HDA_IO_H

#include <Uefi.h>
#include <Protocol/DevicePath.h>

//
// HDA I/O protocol.
//

//
// HDA I/O protocol GUID.
//
#define EFI_HDA_IO_PROTOCOL_GUID \
  { 0xA090D7F9, 0xB50A, 0x4EA1,  \
    { 0xBD, 0xE9, 0x1A, 0xA5, 0xE9, 0x81, 0x2F, 0x45 } }

typedef struct EFI_HDA_IO_PROTOCOL_ EFI_HDA_IO_PROTOCOL;

/**
  Stream type.
**/
typedef enum {
  EfiHdaIoTypeInput,
  EfiHdaIoTypeOutput,
  EfiHdaIoTypeMaximum
} EFI_HDA_IO_PROTOCOL_TYPE;

/**
  Verb list structure.
**/
typedef struct {
  UINT32 Count;
  UINT32 *Verbs ;
  UINT32 *Responses;
} EFI_HDA_IO_VERB_LIST;

/**
  Callback function.
**/
typedef
VOID
(EFIAPI* EFI_HDA_IO_STREAM_CALLBACK) (
  IN EFI_HDA_IO_PROTOCOL_TYPE   Type,
  IN VOID                       *Context1,
  IN VOID                       *Context2,
  IN VOID                       *Context3
  );

/**
  Retrieves this codec's address.

  @param[in]  This              A pointer to the HDA_IO_PROTOCOL instance.
  @param[out] CodecAddress      The codec's address.

  @retval EFI_SUCCESS           The codec's address was returned.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_IO_GET_ADDRESS) (
  IN  EFI_HDA_IO_PROTOCOL       *This,
  OUT UINT8                     *CodecAddress
  );

/**
  Sends a single command to the codec.

  @param[in]  This              A pointer to the HDA_IO_PROTOCOL instance.
  @param[in]  Node              The destination node.
  @param[in]  Verb              The verb to send.
  @param[out] Response          The response received.

  @retval EFI_SUCCESS           The verb was sent successfully and a response received.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_IO_SEND_COMMAND) (
  IN  EFI_HDA_IO_PROTOCOL       *This,
  IN  UINT8                     Node,
  IN  UINT32                    Verb,
  OUT UINT32                    *Response
  );

/**
  Sends a set of commands to the codec.

  @param[in] This               A pointer to the HDA_IO_PROTOCOL instance.
  @param[in] Node               The destination node.
  @param[in] Verbs              The verbs to send. Responses will be delievered in the same list.

  @retval EFI_SUCCESS           The verbs were sent successfully and all responses received.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_IO_SEND_COMMANDS) (
  IN     EFI_HDA_IO_PROTOCOL    *This,
  IN     UINT8                  Node,
  IN OUT EFI_HDA_IO_VERB_LIST   *Verbs
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HDA_IO_SETUP_STREAM) (
  IN  EFI_HDA_IO_PROTOCOL       *This,
  IN  EFI_HDA_IO_PROTOCOL_TYPE  Type,
  IN  UINT16                    Format,
  OUT UINT8                     *StreamId
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HDA_IO_CLOSE_STREAM) (
  IN EFI_HDA_IO_PROTOCOL         *This,
  IN EFI_HDA_IO_PROTOCOL_TYPE    Type
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HDA_IO_GET_STREAM) (
  IN  EFI_HDA_IO_PROTOCOL        *This,
  IN  EFI_HDA_IO_PROTOCOL_TYPE   Type,
  OUT BOOLEAN                    *State
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HDA_IO_START_STREAM) (
  IN EFI_HDA_IO_PROTOCOL         *This,
  IN EFI_HDA_IO_PROTOCOL_TYPE    Type,
  IN VOID                        *Buffer,
  IN UINTN                       BufferLength,
  IN UINTN                       BufferPosition  OPTIONAL,
  IN EFI_HDA_IO_STREAM_CALLBACK  Callback        OPTIONAL,
  IN VOID                        *Context1       OPTIONAL,
  IN VOID                        *Context2       OPTIONAL,
  IN VOID                        *Context3       OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HDA_IO_STOP_STREAM) (
  IN EFI_HDA_IO_PROTOCOL         *This,
  IN EFI_HDA_IO_PROTOCOL_TYPE    Type
  );

/**
  HDA I/O protocol structure.
**/
struct EFI_HDA_IO_PROTOCOL_ {
  EFI_HDA_IO_GET_ADDRESS      GetAddress;
  EFI_HDA_IO_SEND_COMMAND     SendCommand;
  EFI_HDA_IO_SEND_COMMANDS    SendCommands;
  EFI_HDA_IO_SETUP_STREAM     SetupStream;
  EFI_HDA_IO_CLOSE_STREAM     CloseStream;
  EFI_HDA_IO_GET_STREAM       GetStream;
  EFI_HDA_IO_START_STREAM     StartStream;
  EFI_HDA_IO_STOP_STREAM      StopStream;
};

extern EFI_GUID gEfiHdaIoProtocolGuid;

//
// HDA I/O Device Path protocol.
//

/**
  HDA I/O Device Path GUID.
**/
#define EFI_HDA_IO_DEVICE_PATH_GUID \
  { 0xA9003FEB, 0xD806, 0x41DB,     \
    { 0xA4, 0x91, 0x54, 0x05, 0xFE, 0xEF, 0x46, 0xC3 } }

/**
  HDA I/O Device Path structure.
**/
typedef struct {
  ///
  /// Vendor-specific device path fields.
  ///
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  ///
  /// Codec address.
  ///
  UINT8                     Address;
} EFI_HDA_IO_DEVICE_PATH;

extern EFI_GUID gEfiHdaIoDevicePathGuid;

/**
  Template for HDA I/O Device Path protocol.
**/
#define EFI_HDA_IO_DEVICE_PATH_TEMPLATE \
  { \
    { \
      MESSAGING_DEVICE_PATH, \
      MSG_VENDOR_DP, \
      { \
        (UINT8) (sizeof (EFI_HDA_IO_DEVICE_PATH) & 0xFFU), \
        (UINT8) ((sizeof (EFI_HDA_IO_DEVICE_PATH) >> 8U) & 0xFFU) \
      } \
    }, \
    gEfiHdaIoDevicePathGuid, \
    0 \
  }

#endif // EFI_HDA_IO_H
