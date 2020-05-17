/*
 * File: HdaController.h
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

#ifndef _EFI_HDA_CONTROLLER_H_
#define _EFI_HDA_CONTROLLER_H_

#include "AudioDxe.h"
#include <IndustryStandard/HdaRegisters.h>

//
// Consumed protocols.
//
#include <Protocol/PciIo.h>
#include <IndustryStandard/Pci.h>

//
// Structs.
//
typedef struct _HDA_CONTROLLER_DEV HDA_CONTROLLER_DEV;
typedef struct _HDA_IO_PRIVATE_DATA HDA_IO_PRIVATE_DATA;
typedef struct _HDA_CONTROLLER_INFO_PRIVATE_DATA HDA_CONTROLLER_INFO_PRIVATE_DATA;

// Signature for private data structures.
#define HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE SIGNATURE_32('H','d','a','C')

//
// PCI support.
//
// Structure used for PCI class code parsing.
#pragma pack(1)
typedef struct {
  UINT8 ProgInterface;
  UINT8 SubClass;
  UINT8 Class;
} HDA_PCI_CLASSREG;
#pragma pack()

// HDA controller is accessed via MMIO on BAR #0.
#define PCI_HDA_BAR 0

// Min supported version.
#define HDA_VERSION_MIN_MAJOR   0x1
#define HDA_VERSION_MIN_MINOR   0x0
#define HDA_MAX_CODECS 15

#define PCI_HDA_TCSEL_OFFSET    0x44
#define PCI_HDA_TCSEL_TC0_MASK  ~(BIT0 | BIT1 | BIT2)
#define PCI_HDA_DEVC_OFFSET     0x78
#define PCI_HDA_DEVC_NOSNOOPEN  BIT11

//
// CORB and RIRB.
//
// Entry sizes.
#define HDA_CORB_ENTRY_SIZE sizeof(UINT32)
#define HDA_RIRB_ENTRY_SIZE sizeof(UINT64)

// Misc.
#define HDA_CORB_VERB(Cad, Nid, Verb) ((((UINT32)Cad) << 28) | (((UINT32)Nid) << 20) | (Verb & 0xFFFFF))
#define HDA_RIRB_RESP(Response)     ((UINT32)Response)
#define HDA_RIRB_CAD(Response)      ((Response >> 32) & 0xF)
#define HDA_RIRB_UNSOL(Response)    ((Response >> 36) & 0x1)

//
// Streams.
//
// Buffer Descriptor List Entry.
#pragma pack(1)
typedef struct {
  UINT32 Address;
  UINT32 AddressHigh;
  UINT32 Length;
  UINT32 InterruptOnCompletion;
} HDA_BDL_ENTRY;
#pragma pack()

// Buffer Descriptor List sizes. Max number of entries is 256, min is 2.
#define HDA_BDL_ENTRY_IOC       BIT0
#define HDA_BDL_ENTRY_COUNT     8
#define HDA_BDL_SIZE            (sizeof(HDA_BDL_ENTRY) * HDA_BDL_ENTRY_COUNT)
#define HDA_BDL_ENTRY_HALF      ((HDA_BDL_ENTRY_COUNT / 2) - 1)
#define HDA_BDL_ENTRY_LAST      (HDA_BDL_ENTRY_COUNT - 1)

// Buffer size and block size.
#define HDA_STREAM_BUF_SIZE         BASE_512KB
#define HDA_STREAM_BUF_SIZE_HALF    (HDA_STREAM_BUF_SIZE / 2)
#define HDA_BDL_BLOCKSIZE           (HDA_STREAM_BUF_SIZE / HDA_BDL_ENTRY_COUNT)
#define HDA_STREAM_POLL_TIME        (EFI_TIMER_PERIOD_MILLISECONDS(100))

// DMA position structure.
#pragma pack(1)
typedef struct {
  UINT32 Position;
  UINT32 Reserved;
} HDA_DMA_POS_ENTRY;
#pragma pack()

// Stream types.
#define HDA_STREAM_TYPE_BIDIR   0
#define HDA_STREAM_TYPE_IN      1
#define HDA_STREAM_TYPE_OUT     2

#define HDA_STREAM_ID_MIN       1
#define HDA_STREAM_ID_MAX       15

// Stream structure.
typedef struct {
  // Parent controller, type, and index.
  HDA_CONTROLLER_DEV *HdaControllerDev;
  UINT8 Type;
  UINT8 Index;
  BOOLEAN Output;

  // Buffer Descriptor List.
  HDA_BDL_ENTRY *BufferList;
  VOID *BufferListMapping;
  EFI_PHYSICAL_ADDRESS BufferListPhysAddr;

  // DMA data buffer fed into BDL.
  UINT8 *BufferData;
  VOID *BufferDataMapping;
  EFI_PHYSICAL_ADDRESS BufferDataPhysAddr;

  // Source buffer.
  UINT8 *BufferSource;
  UINTN BufferSourceLength;
  UINTN BufferSourcePosition;
  BOOLEAN BufferSourceDone;

  // Timing elements for buffer filling.
  EFI_EVENT PollTimer;
  EFI_HDA_IO_STREAM_CALLBACK Callback;
  VOID *CallbackContext1;
  VOID *CallbackContext2;
  VOID *CallbackContext3;
} HDA_STREAM;

typedef struct {
  EFI_HANDLE Handle;
  HDA_IO_PRIVATE_DATA *PrivateData;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
} HDA_IO_CHILD;

struct _HDA_CONTROLLER_DEV {
  // Signature.
  UINTN Signature;

  // Consumed protocols and handles.
  EFI_PCI_IO_PROTOCOL *PciIo;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  EFI_HANDLE ControllerHandle;
  UINT32  OpenMode;

  // PCI.
  UINT64 OriginalPciAttributes;
  BOOLEAN OriginalPciAttributesSaved;

  // Published info protocol.
  HDA_CONTROLLER_INFO_PRIVATE_DATA *HdaControllerInfoData;
  HDA_IO_CHILD HdaIoChildren[HDA_MAX_CODECS];

  // Capabilites.
  UINT32 VendorId;
  CHAR16 *Name;
  UINT8 MajorVersion;
  UINT8 MinorVersion;
  UINT16 Capabilities;

  // Command output buffer (CORB).
  UINT32 *CorbBuffer;
  UINT32 CorbEntryCount;
  VOID *CorbMapping;
  EFI_PHYSICAL_ADDRESS CorbPhysAddr;
  UINT16 CorbWritePointer;

  // Response input buffer (RIRB).
  UINT64 *RirbBuffer;
  UINT32 RirbEntryCount;
  VOID *RirbMapping;
  EFI_PHYSICAL_ADDRESS RirbPhysAddr;
  UINT16 RirbReadPointer;

  // Streams.
  UINT8 TotalStreamsCount;
  UINT8 BidirStreamsCount;
  UINT8 InputStreamsCount;
  UINT8 OutputStreamsCount;
  HDA_STREAM *BidirStreams;
  HDA_STREAM *InputStreams;
  HDA_STREAM *OutputStreams;

  // DMA positions.
  HDA_DMA_POS_ENTRY *DmaPositions;
  UINTN DmaPositionsSize;
  VOID *DmaPositionsMapping;
  EFI_PHYSICAL_ADDRESS DmaPositionsPhysAddr;

  // Bitmap for stream ID allocation.
  UINT16 StreamIdMapping;

  // Events.
  EFI_EVENT ResponsePollTimer;
  EFI_EVENT ExitBootServiceEvent;
  SPIN_LOCK SpinLock;

};

// HDA I/O private data.
struct _HDA_IO_PRIVATE_DATA {
  // Signature.
  UINTN Signature;

  // HDA I/O protocol.
  EFI_HDA_IO_PROTOCOL HdaIo;
  UINT8 HdaCodecAddress;

  // Assigned streams.
  HDA_STREAM *HdaOutputStream;
  HDA_STREAM *HdaInputStream;

  // HDA controller device.
  HDA_CONTROLLER_DEV *HdaControllerDev;
};
#define HDA_IO_PRIVATE_DATA_FROM_THIS(This) \
  CR(This, HDA_IO_PRIVATE_DATA, HdaIo, HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE)

// HDA Codec Info private data.
struct _HDA_CONTROLLER_INFO_PRIVATE_DATA {
  // Signature.
  UINTN Signature;

  // HDA Codec Info protocol.
  EFI_HDA_CONTROLLER_INFO_PROTOCOL HdaControllerInfo;

  // HDA controller device.
  HDA_CONTROLLER_DEV *HdaControllerDev;
};
#define HDA_CONTROLLER_INFO_PRIVATE_DATA_FROM_THIS(This) \
  CR(This, HDA_CONTROLLER_INFO_PRIVATE_DATA, HdaControllerInfo, HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE)

//
// HDA I/O protocol functions.
//
EFI_STATUS
EFIAPI
HdaControllerHdaIoGetAddress(
  IN  EFI_HDA_IO_PROTOCOL *This,
  OUT UINT8 *CodecAddress);

EFI_STATUS
EFIAPI
HdaControllerHdaIoSendCommand(
  IN  EFI_HDA_IO_PROTOCOL *This,
  IN  UINT8 Node,
  IN  UINT32 Verb,
  OUT UINT32 *Response);

EFI_STATUS
EFIAPI
HdaControllerHdaIoSendCommands(
  IN EFI_HDA_IO_PROTOCOL *This,
  IN UINT8 Node,
  IN EFI_HDA_IO_VERB_LIST *Verbs);

EFI_STATUS
EFIAPI
HdaControllerHdaIoSetupStream(
  IN  EFI_HDA_IO_PROTOCOL *This,
  IN  EFI_HDA_IO_PROTOCOL_TYPE Type,
  IN  UINT16 Format,
  OUT UINT8 *StreamId);

EFI_STATUS
EFIAPI
HdaControllerHdaIoCloseStream(
  IN EFI_HDA_IO_PROTOCOL *This,
  IN EFI_HDA_IO_PROTOCOL_TYPE Type);

EFI_STATUS
EFIAPI
HdaControllerHdaIoGetStream(
  IN  EFI_HDA_IO_PROTOCOL *This,
  IN  EFI_HDA_IO_PROTOCOL_TYPE Type,
  OUT BOOLEAN *State);

EFI_STATUS
EFIAPI
HdaControllerHdaIoStartStream(
  IN EFI_HDA_IO_PROTOCOL *This,
  IN EFI_HDA_IO_PROTOCOL_TYPE Type,
  IN VOID *Buffer,
  IN UINTN BufferLength,
  IN UINTN BufferPosition OPTIONAL,
  IN EFI_HDA_IO_STREAM_CALLBACK Callback OPTIONAL,
  IN VOID *Context1 OPTIONAL,
  IN VOID *Context2 OPTIONAL,
  IN VOID *Context3 OPTIONAL);

EFI_STATUS
EFIAPI
HdaControllerHdaIoStopStream(
  IN EFI_HDA_IO_PROTOCOL *This,
  IN EFI_HDA_IO_PROTOCOL_TYPE Type);

//
// HDA Controller Info protcol functions.
//
EFI_STATUS
EFIAPI
HdaControllerInfoGetName(
  IN  EFI_HDA_CONTROLLER_INFO_PROTOCOL *This,
  OUT CONST CHAR16 **ControllerName);

//
// HDA controller internal functions.
//
VOID
EFIAPI
HdaControllerStreamPollTimerHandler(
  IN EFI_EVENT Event,
  IN VOID *Context);

EFI_STATUS
EFIAPI
HdaControllerReset(
  IN HDA_CONTROLLER_DEV *HdaControllerDev);

EFI_STATUS
EFIAPI
HdaControllerSendCommands(
  IN HDA_CONTROLLER_DEV *HdaDev,
  IN UINT8 CodecAddress,
  IN UINT8 Node,
  IN EFI_HDA_IO_VERB_LIST *Verbs);

EFI_STATUS
EFIAPI
HdaControllerInitCorb(
  IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerCleanupCorb(
  IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerSetCorb(
  IN HDA_CONTROLLER_DEV *HdaDev,
  IN BOOLEAN Enable);

EFI_STATUS
EFIAPI
HdaControllerInitRirb(
  IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerCleanupRirb(
  IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerSetRirb(
  IN HDA_CONTROLLER_DEV *HdaDev,
  IN BOOLEAN Enable);

EFI_STATUS
EFIAPI
HdaControllerInitStreams(
  IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerResetStream(
  IN HDA_STREAM *HdaStream);

VOID
EFIAPI
HdaControllerCleanupStreams(
  IN HDA_CONTROLLER_DEV *HdaControllerDev);

EFI_STATUS
EFIAPI
HdaControllerGetStream(
  IN  HDA_STREAM *HdaStream,
  OUT BOOLEAN *Run);

EFI_STATUS
EFIAPI
HdaControllerSetStream(
  IN HDA_STREAM *HdaStream,
  IN BOOLEAN Run);

EFI_STATUS
EFIAPI
HdaControllerGetStreamLinkPos(
  IN  HDA_STREAM *HdaStream,
  OUT UINT32 *Position);

EFI_STATUS
EFIAPI
HdaControllerGetStreamId(
  IN  HDA_STREAM *HdaStream,
  OUT UINT8 *Index);

EFI_STATUS
EFIAPI
HdaControllerSetStreamId(
  IN HDA_STREAM *HdaStream,
  IN UINT8 Index);

//
// Driver Binding protocol functions.
//
EFI_STATUS
EFIAPI
HdaControllerDriverBindingSupported(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL);

EFI_STATUS
EFIAPI
HdaControllerDriverBindingStart(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL);

EFI_STATUS
EFIAPI
HdaControllerDriverBindingStop(
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE ControllerHandle,
  IN UINTN NumberOfChildren,
  IN EFI_HANDLE *ChildHandleBuffer OPTIONAL);

#endif
