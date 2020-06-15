/*
 * File: HdaControllerMem.c
 *
 * Copyright (c) 2018, 2020 John Davis
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

#include "HdaController.h"
#include <IndustryStandard/HdaRegisters.h>

BOOLEAN
HdaControllerInitRingBuffer (
  IN HDA_RING_BUFFER    *HdaRingBuffer,
  IN HDA_CONTROLLER_DEV *HdaDev,
  IN HDA_RING_BUFFER_TYPE Type
  )
{
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT32                Offset;
  UINT32                EntrySize;

  UINT8                 HdaRingSize;
  UINT32                HdaRingLowerBaseAddr;
  UINT32                HdaRingUpperBaseAddr;

  UINTN                 BufferSizeActual;

  ASSERT (HdaRingBuffer != NULL);
  ASSERT (HdaDev != NULL);

  if (Type == HDA_RING_BUFFER_TYPE_CORB) {
    Offset          = HDA_REG_CORB_BASE;
    EntrySize       = HDA_CORB_ENTRY_SIZE;
  } else if (Type == HDA_RING_BUFFER_TYPE_RIRB) {
    Offset          = HDA_REG_RIRB_BASE;
    EntrySize       = HDA_RIRB_ENTRY_SIZE;
  } else {
    return FALSE;
  }

  HdaRingBuffer->HdaDev = HdaDev;
  HdaRingBuffer->Type   = Type;
  PciIo                 = HdaDev->PciIo;

  //
  // Get current value of size register.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, Offset + HDA_OFFSET_RING_SIZE, 1, &HdaRingSize);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Calculate size of ring buffer based on supported sizes.
  //
  if (HdaRingSize & HDA_OFFSET_RING_SIZE_SZCAP_256) {
    HdaRingBuffer->BufferSize = 256 * EntrySize;
    HdaRingSize = (HdaRingSize & ~HDA_OFFSET_RING_SIZE_MASK) | HDA_OFFSET_RING_SIZE_ENT256;
  } else if (HdaRingSize & HDA_OFFSET_RING_SIZE_SZCAP_16) {
    HdaRingBuffer->BufferSize = 16 * EntrySize;
    HdaRingSize = (HdaRingSize & ~HDA_OFFSET_RING_SIZE_MASK) | HDA_OFFSET_RING_SIZE_ENT16;
  } else if (HdaRingSize & HDA_OFFSET_RING_SIZE_SZCAP_2) {
    HdaRingBuffer->BufferSize = 2 * EntrySize;
    HdaRingSize = (HdaRingSize & ~HDA_OFFSET_RING_SIZE_MASK) | HDA_OFFSET_RING_SIZE_ENT2;
  } else {
    //
    // Unsupported size.
    //
    return FALSE;
  }

  HdaRingBuffer->Buffer   = NULL;
  HdaRingBuffer->Mapping  = NULL;

  //
  // Allocate buffer.
  //
  Status = PciIo->AllocateBuffer (PciIo, AllocateAnyPages, EfiBootServicesData,
    EFI_SIZE_TO_PAGES (HdaRingBuffer->BufferSize), &HdaRingBuffer->Buffer, 0);
  if (EFI_ERROR (Status)) {
    HdaControllerCleanupRingBuffer (HdaRingBuffer);
    return FALSE;
  }
  ZeroMem (HdaRingBuffer->Buffer, HdaRingBuffer->BufferSize);

  //
  // Map buffer.
  //
  BufferSizeActual = HdaRingBuffer->BufferSize;
  Status = PciIo->Map (PciIo, EfiPciIoOperationBusMasterCommonBuffer, HdaRingBuffer->Buffer,
    &BufferSizeActual, &HdaRingBuffer->PhysAddr, &HdaRingBuffer->Mapping);
  if (EFI_ERROR (Status) || BufferSizeActual != HdaRingBuffer->BufferSize) {
    HdaControllerCleanupRingBuffer (HdaRingBuffer);
    return FALSE;
  }

  //
  // Ensure ring buffer is stopped.
  //
  if (!HdaControllerSetRingBufferState (HdaRingBuffer, FALSE)) {
    HdaControllerCleanupRingBuffer (HdaRingBuffer);
    return FALSE;
  }

  //
  // Set buffer lower base address.
  //
  HdaRingLowerBaseAddr = (UINT32)HdaRingBuffer->PhysAddr;
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, Offset + HDA_OFFSET_RING_BASE, 1, &HdaRingLowerBaseAddr);
  if (EFI_ERROR (Status)) {
    HdaControllerCleanupRingBuffer (HdaRingBuffer);
    return FALSE;
  }

  //
  // If 64-bit buffers are supported, also set upper base address.
  //
  if (HdaRingBuffer->HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
    HdaRingUpperBaseAddr = (UINT32)(HdaRingBuffer->PhysAddr >> 32);
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, Offset + HDA_OFFSET_RING_UBASE, 1, &HdaRingUpperBaseAddr);
    if (EFI_ERROR (Status)) {
      HdaControllerCleanupRingBuffer (HdaRingBuffer);
      return FALSE;
    }
  }

  //
  // Reset ring buffer.
  //
  if (!HdaControllerResetRingBuffer (HdaRingBuffer)) {
    HdaControllerCleanupRingBuffer (HdaRingBuffer);
    return FALSE;
  }

  HdaRingBuffer->EntryCount = (UINT32)(HdaRingBuffer->BufferSize / EntrySize);
  DEBUG ((DEBUG_VERBOSE, "HDA controller ring buffer allocated @ 0x%p (0x%p) (%u entries)\n",
    HdaRingBuffer->Buffer, HdaRingBuffer->PhysAddr, HdaRingBuffer->EntryCount));
  return TRUE;
}

VOID
HdaControllerCleanupRingBuffer (
  IN HDA_RING_BUFFER    *HdaRingBuffer
  )
{
  EFI_PCI_IO_PROTOCOL   *PciIo;

  ASSERT (HdaRingBuffer != NULL);

  //
  // Already freed if NULL.
  //
  if (HdaRingBuffer->HdaDev == NULL) {
    return;
  }

  PciIo = HdaRingBuffer->HdaDev->PciIo;

  //
  // Unmap and free ring buffer.
  //
  HdaControllerSetRingBufferState (HdaRingBuffer, FALSE);
  if (HdaRingBuffer->Mapping != NULL) {
    PciIo->Unmap (PciIo, HdaRingBuffer->Mapping);
  }
  if (HdaRingBuffer->Buffer != NULL) {
    PciIo->FreeBuffer (PciIo, EFI_SIZE_TO_PAGES (HdaRingBuffer->BufferSize), HdaRingBuffer->Buffer);
  }

  HdaRingBuffer->Buffer         = NULL;
  HdaRingBuffer->BufferSize     = 0;
  HdaRingBuffer->EntryCount     = 0;
  HdaRingBuffer->Mapping        = NULL;
  HdaRingBuffer->PhysAddr       = 0;
  HdaRingBuffer->Pointer        = 0;
  HdaRingBuffer->HdaDev         = NULL;
}

BOOLEAN
HdaControllerSetRingBufferState (
  IN HDA_RING_BUFFER    *HdaRingBuffer,
  IN BOOLEAN            Enable
  )
{
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT32                Offset;
  UINT8                 HdaRingCtl;
  UINT64                Tmp;

  PciIo = HdaRingBuffer->HdaDev->PciIo;

  if (HdaRingBuffer->Type == HDA_RING_BUFFER_TYPE_CORB) {
    Offset = HDA_REG_CORB_BASE;
  } else if (HdaRingBuffer->Type == HDA_RING_BUFFER_TYPE_RIRB) {
    Offset = HDA_REG_RIRB_BASE;
  } else {
    return FALSE;
  }

  //
  // Get current value of CTL register.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, Offset + HDA_OFFSET_RING_CTL, 1, &HdaRingCtl);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (Enable) {
    HdaRingCtl |= HDA_OFFSET_RING_CTL_RUN;
  }
  else {
    HdaRingCtl &= ~HDA_OFFSET_RING_CTL_RUN;
  }
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, Offset + HDA_OFFSET_RING_CTL, 1, &HdaRingCtl);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Wait for bit to cycle indicating the ring buffer has started.
  //
  Status = PciIo->PollMem (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, Offset + HDA_OFFSET_RING_CTL, HDA_OFFSET_RING_CTL_RUN,
    Enable ? HDA_OFFSET_RING_CTL_RUN : 0, MS_TO_NANOSECOND (50), &Tmp);
  return !EFI_ERROR (Status);
}

BOOLEAN
HdaControllerResetRingBuffer (
  IN HDA_RING_BUFFER    *HdaRingBuffer
  )
{
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT16                HdaRingPointer;
  UINT64                HdaRingPointerPollResult;

  PciIo = HdaRingBuffer->HdaDev->PciIo;

  //
  // Reset read/write pointers.
  //
  if (HdaRingBuffer->Type == HDA_RING_BUFFER_TYPE_CORB) {
    //
    // Clear write pointer.
    //
    HdaRingPointer = 0;
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaRingPointer);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    //
    // Set reset bit in read pointer register.
    //
    HdaRingPointer = HDA_REG_CORBRP_RST;
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaRingPointer);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    //
    // Per spec, clear bit and wait for bit to clear.
    //
    HdaRingPointer = 0;
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaRingPointer);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }
    Status = PciIo->PollMem (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, HDA_REG_CORBRP_RST, 0, 50000000, &HdaRingPointerPollResult); // 50000000 = 50 ms
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

  } else if (HdaRingBuffer->Type == HDA_RING_BUFFER_TYPE_RIRB) {
    //
    // RIRB requires only the reset bit be set.
    //
    Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRingPointer);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }
    HdaRingPointer |= HDA_REG_RIRBWP_RST;
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRingPointer);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

  } else {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
HdaControllerInitStreams (
  IN HDA_CONTROLLER_DEV *HdaDev
  )
{
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT32                LowerBaseAddr;
  UINT32                UpperBaseAddr;
  EFI_PHYSICAL_ADDRESS  DataBlockAddr;
  HDA_STREAM            *HdaStream;

  UINT8                 InputStreamsCount;
  UINT8                 OutputStreamsCount;
  UINT8                 BidirStreamsCount;
  UINT32                TotalStreamsCount;
  UINT32                OutputStreamsOffset;
  UINT32                BidirStreamsOffset;

  UINT32                Index;
  UINT32                Index2;
  UINTN                 PciLengthActual;

  ASSERT (HdaDev != NULL);

  PciIo = HdaDev->PciIo;

  //
  // Reset stream ID bitmap so stream 0 is allocated (reserved).
  //
  HdaDev->StreamIdMapping = BIT0;

  InputStreamsCount       = HDA_REG_GCAP_ISS (HdaDev->Capabilities);
  OutputStreamsCount      = HDA_REG_GCAP_OSS (HdaDev->Capabilities);
  BidirStreamsCount       = HDA_REG_GCAP_BSS (HdaDev->Capabilities);
  TotalStreamsCount       = InputStreamsCount + OutputStreamsCount + BidirStreamsCount;
  OutputStreamsOffset     = InputStreamsCount;
  BidirStreamsOffset      = OutputStreamsOffset + OutputStreamsCount;

  HdaDev->StreamsCount    = OutputStreamsCount + BidirStreamsCount;
  HdaDev->Streams         = AllocateZeroPool (sizeof (HDA_STREAM) * HdaDev->StreamsCount);
  if (HdaDev->Streams == NULL) {
    HdaControllerCleanupStreams (HdaDev);
    return FALSE;
  }
  DEBUG ((DEBUG_VERBOSE, "AudioDxe: Total output-capable streams: %u\n", HdaDev->StreamsCount));

  for (Index = 0; Index < HdaDev->StreamsCount; Index++) {
    HdaStream                   = &HdaDev->Streams[Index];
    HdaStream->HdaDev           = HdaDev;
    HdaStream->Index            = (UINT8)(Index + OutputStreamsOffset);
    HdaStream->IsBidirectional  = Index + OutputStreamsOffset >= BidirStreamsOffset;

    //
    // Initialize polling timer.
    //
    Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_NOTIFY,
      (EFI_EVENT_NOTIFY)HdaControllerStreamOutputPollTimerHandler, HdaStream, &HdaStream->PollTimer);
    if (EFI_ERROR (Status)) {
      HdaControllerCleanupStreams (HdaDev);
      return FALSE;
    }

    //
    // Allocate buffer descriptor list.
    //
    Status = PciIo->AllocateBuffer (PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES (HDA_BDL_SIZE),
      (VOID**)&HdaStream->BufferList, 0);
    if (EFI_ERROR (Status)) {
      HdaControllerCleanupStreams (HdaDev);
      return FALSE;
    }
    ZeroMem (HdaStream->BufferList, HDA_BDL_SIZE);

    //
    // Map buffer descriptor list.
    //
    PciLengthActual = HDA_BDL_SIZE;
    Status = PciIo->Map (PciIo, EfiPciIoOperationBusMasterCommonBuffer, HdaStream->BufferList, &PciLengthActual,
      &HdaStream->BufferListPhysAddr, &HdaStream->BufferListMapping);
    if (EFI_ERROR (Status) || PciLengthActual != HDA_BDL_SIZE) {
      HdaControllerCleanupStreams (HdaDev);
      return FALSE;
    }

    if (!HdaControllerResetStream (HdaStream)) {
      HdaControllerCleanupStreams (HdaDev);
      return FALSE;
    }

    //
    // Allocate buffer for data.
    //
    Status = PciIo->AllocateBuffer (PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES (HDA_STREAM_BUF_SIZE),
      (VOID**)&HdaStream->BufferData, 0);
    if (EFI_ERROR (Status)) {
      HdaControllerCleanupStreams (HdaDev);
      return FALSE;
    }
    ZeroMem (HdaStream->BufferData, HDA_STREAM_BUF_SIZE);

    //
    // Map buffer descriptor list.
    //
    PciLengthActual = HDA_STREAM_BUF_SIZE;
    Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, HdaStream->BufferData, &PciLengthActual,
      &HdaStream->BufferDataPhysAddr, &HdaStream->BufferDataMapping);
    if (EFI_ERROR (Status) || PciLengthActual != HDA_STREAM_BUF_SIZE) {
      HdaControllerCleanupStreams (HdaDev);
      return FALSE;
    }

    //
    // Fill buffer list.
    //
    for (Index2 = 0; Index2 < HDA_BDL_ENTRY_COUNT; Index2++) {
      DataBlockAddr                                       = HdaStream->BufferDataPhysAddr + (Index2 * HDA_BDL_BLOCKSIZE);
      HdaStream->BufferList[Index2].Address               = (UINT32)DataBlockAddr;
      HdaStream->BufferList[Index2].AddressHigh           = (UINT32)(DataBlockAddr >> 32);
      HdaStream->BufferList[Index2].Length                = HDA_BDL_BLOCKSIZE;
      HdaStream->BufferList[Index2].InterruptOnCompletion = TRUE;
    }
  }

  //
  // Allocate DMA positions structure.
  //
  HdaDev->DmaPositionsSize = sizeof (HDA_DMA_POS_ENTRY) * TotalStreamsCount;
  Status = PciIo->AllocateBuffer (PciIo, AllocateAnyPages, EfiBootServicesData,
    EFI_SIZE_TO_PAGES (HdaDev->DmaPositionsSize), (VOID**)&HdaDev->DmaPositions, 0);
  if (EFI_ERROR(Status)) {
    HdaControllerCleanupStreams (HdaDev);
    return FALSE;
  }
  ZeroMem (HdaDev->DmaPositions, HdaDev->DmaPositionsSize);

  // Map buffer descriptor list.
  PciLengthActual = HdaDev->DmaPositionsSize;
  Status = PciIo->Map (PciIo, EfiPciIoOperationBusMasterCommonBuffer, HdaDev->DmaPositions,
    &PciLengthActual, &HdaDev->DmaPositionsPhysAddr, &HdaDev->DmaPositionsMapping);
  if (EFI_ERROR (Status) || PciLengthActual != HdaDev->DmaPositionsSize) {
    HdaControllerCleanupStreams (HdaDev);
    return FALSE;
  }

  //
  // Set DMA positions lower base address.
  //
  LowerBaseAddr = ((UINT32)HdaDev->DmaPositionsPhysAddr) | HDA_REG_DPLBASE_EN;
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_DPLBASE, 1, &LowerBaseAddr);
  if (EFI_ERROR (Status)) {
    HdaControllerCleanupStreams (HdaDev);
    return FALSE;
  }

  //
  // If 64-bit supported, set DMA positions upper base address.
  //
  if (HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
    UpperBaseAddr = (UINT32)(HdaDev->DmaPositionsPhysAddr >> 32);
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_DPUBASE, 1, &UpperBaseAddr);
    if (EFI_ERROR (Status)) {
      HdaControllerCleanupStreams (HdaDev);
      return FALSE;
    }
  }

  return TRUE;
}

BOOLEAN
HdaControllerResetStream (
  IN HDA_STREAM *HdaStream
  )
{
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT32                LowerBaseAddr;
  UINT32                UpperBaseAddr;

  UINT8                 HdaStreamCtl1;
  UINT8                 HdaStreamCtl3;
  UINT16                StreamLvi;
  UINT32                StreamCbl;
  UINT64                Tmp;

  ASSERT (HdaStream != NULL);

  PciIo = HdaStream->HdaDev->PciIo;

  //
  // Get value of control register.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Set and wait for reset bit to be set.
  //
  HdaStreamCtl1 |= HDA_REG_SDNCTL1_SRST;
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  Status = PciIo->PollMem (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index),
    HDA_REG_SDNCTL1_SRST, HDA_REG_SDNCTL1_SRST, MS_TO_NANOSECOND (100), &Tmp);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Docs state we need to clear the bit and wait for it to clear.
  //
  HdaStreamCtl1 &= ~HDA_REG_SDNCTL1_SRST;
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  Status = PciIo->PollMem (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index),
    HDA_REG_SDNCTL1_SRST, 0, MS_TO_NANOSECOND (100), &Tmp);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // If bidirectional stream, configure for output.
  //   This bit should be set immediately after reset.
  //
  if (HdaStream->IsBidirectional) {
    //
    // Get current value of stream control register.
    //
    Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3 (HdaStream->Index), 1, &HdaStreamCtl3);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    //
    // Set DIR bit and write stream control register.
    //
    HdaStreamCtl3 |= HDA_REG_SDNCTL3_DIR;
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3 (HdaStream->Index), 1, &HdaStreamCtl3);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }
  }

  //
  // Set BDL lower base address.
  //
  LowerBaseAddr = (UINT32)HdaStream->BufferListPhysAddr;
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNBDPL (HdaStream->Index), 1, &LowerBaseAddr);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // If 64-bit supported, set BDL upper base address.
  //
  if (HdaStream->HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
    UpperBaseAddr = (UINT32)(HdaStream->BufferListPhysAddr >> 32);
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNBDPU (HdaStream->Index), 1, &UpperBaseAddr);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }
  }

  //
  // Set last valid index (LVI) and total buffer length.
  //
  StreamLvi = HDA_BDL_ENTRY_LAST;
  StreamCbl = HDA_STREAM_BUF_SIZE;
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_SDNLVI (HdaStream->Index), 1, &StreamLvi);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNCBL (HdaStream->Index), 1, &StreamCbl);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  HdaStream->DmaPositionTotal = 0;
  HdaStream->DmaPositionLast = 0;
  HdaStream->DmaPositionChangedMax = 0;
  HdaStream->UseLpib = FALSE; // TODO: Allow being forced by NVRAM variable?
  HdaStream->DmaCheckCount = 0;
  HdaStream->DmaCheckComplete = FALSE;

  return TRUE;
}

VOID
HdaControllerCleanupStreams (
  IN HDA_CONTROLLER_DEV *HdaDev
  )
{
  EFI_PCI_IO_PROTOCOL   *PciIo;
  HDA_STREAM            *HdaStream;

  UINT32                Index;
  UINT32                Tmp;

  ASSERT (HdaDev != NULL);

  PciIo = HdaDev->PciIo;

  if (HdaDev->Streams != NULL && HdaDev->StreamsCount > 0) {
    for (Index = 0; Index < HdaDev->StreamsCount; Index++) {
      HdaStream = &HdaDev->Streams[Index];

      //
      // Close poll timer and disable stream.
      //
      gBS->CloseEvent (HdaStream->PollTimer);
      HdaControllerSetStreamState (HdaStream, FALSE);
      HdaControllerSetStreamId (HdaStream, 0);

      //
      // Unmap and free BDL.
      //
      if (HdaStream->BufferListMapping != NULL)
        PciIo->Unmap (PciIo, HdaStream->BufferListMapping);
      if (HdaStream->BufferList != NULL)
        PciIo->FreeBuffer (PciIo, EFI_SIZE_TO_PAGES (HDA_BDL_SIZE), HdaStream->BufferList);

      //
      // Unmap and free data buffer.
      //
      if (HdaStream->BufferDataMapping != NULL)
        PciIo->Unmap (PciIo, HdaStream->BufferDataMapping);
      if (HdaStream->BufferData != NULL)
        PciIo->FreeBuffer (PciIo, EFI_SIZE_TO_PAGES (HDA_STREAM_BUF_SIZE), HdaStream->BufferData);
    }
  }

  //
  // Clear DMA positions structure base address.
  //
  Tmp = 0;
  PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_DPLBASE, 1, &Tmp);
  if (HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
    PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_DPUBASE, 1, &Tmp);
  }

  //
  // Unmap and free DMA positions structure.
  //
  if (HdaDev->DmaPositionsMapping != NULL)
    PciIo->Unmap (PciIo, HdaDev->DmaPositionsMapping);
  if (HdaDev->DmaPositions != NULL)
    PciIo->FreeBuffer (PciIo, EFI_SIZE_TO_PAGES (HdaDev->DmaPositionsSize), HdaDev->DmaPositions);

  if (HdaDev->Streams != NULL) {
    FreePool (HdaDev->Streams);
  }

  HdaDev->DmaPositionsMapping   = NULL;
  HdaDev->DmaPositions          = NULL;
  HdaDev->DmaPositionsSize      = 0;
  HdaDev->DmaPositionsPhysAddr  = 0;
  HdaDev->Streams               = NULL;
  HdaDev->StreamsCount          = 0;
}

BOOLEAN
HdaControllerGetStreamState (
  IN  HDA_STREAM *HdaStream,
  OUT BOOLEAN *Run
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT8               HdaStreamCtl1;

  ASSERT (HdaStream != NULL);
  ASSERT (Run != NULL);

  PciIo = HdaStream->HdaDev->PciIo;

  //
  // Get current value of stream control register.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  *Run = (HdaStreamCtl1 & HDA_REG_SDNCTL1_RUN) != 0;
  return TRUE;
}

BOOLEAN
HdaControllerSetStreamState (
  IN HDA_STREAM *HdaStream,
  IN BOOLEAN Run
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT8               HdaStreamCtl1;
  UINT64              Tmp;

  ASSERT (HdaStream != NULL);

  PciIo = HdaStream->HdaDev->PciIo;

  //
  // Get current value of stream control register.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (Run) {
    HdaStreamCtl1 |= HDA_REG_SDNCTL1_RUN;
  } else {
    HdaStreamCtl1 &= ~HDA_REG_SDNCTL1_RUN;
  }

  
  //
  // Write updated run bit to stream control register and wait for bit status to change.
  //
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  Status = PciIo->PollMem (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1 (HdaStream->Index),
    HDA_REG_SDNCTL1_RUN, HdaStreamCtl1 & HDA_REG_SDNCTL1_RUN, MS_TO_NANOSECOND (10), &Tmp);
  return !EFI_ERROR (Status);
}

BOOLEAN
HdaControllerGetStreamLinkPos (
  IN  HDA_STREAM *HdaStream,
  OUT UINT32 *Position
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;

  ASSERT (HdaStream != NULL);
  ASSERT (Position != NULL);

  PciIo = HdaStream->HdaDev->PciIo;

  //
  // Get current value of stream link position register.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNLPIB (HdaStream->Index), 1, Position);
  return !EFI_ERROR (Status);
}

BOOLEAN
HdaControllerGetStreamId (
  IN  HDA_STREAM *HdaStream,
  OUT UINT8 *Index
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT8               HdaStreamCtl3;

  ASSERT (HdaStream != NULL);
  ASSERT (Index != NULL);

  PciIo = HdaStream->HdaDev->PciIo;

  //
  // Get current value of stream control register.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3 (HdaStream->Index), 1, &HdaStreamCtl3);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  *Index = HDA_REG_SDNCTL3_STRM_GET (HdaStreamCtl3);
  return TRUE;
}

BOOLEAN
HdaControllerSetStreamId (
  IN HDA_STREAM *HdaStream,
  IN UINT8 Index
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT8               HdaStreamCtl3;

  ASSERT (HdaStream != NULL);

  PciIo = HdaStream->HdaDev->PciIo;

  //
  // Stop stream.
  //
  if (!HdaControllerSetStreamState (HdaStream, FALSE)) {
    return FALSE;
  }

  //
  // Get current value of stream control register.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3 (HdaStream->Index), 1, &HdaStreamCtl3);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Update stream index and write stream control register.
  //
  HdaStreamCtl3 = HDA_REG_SDNCTL3_STRM_SET (HdaStreamCtl3, Index);
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3 (HdaStream->Index), 1, &HdaStreamCtl3);
  return !EFI_ERROR (Status);
}

VOID
HdaControllerStreamIdle (
  IN HDA_STREAM *HdaStream
  )
{
  ASSERT (HdaStream != NULL);

  //
  // Reset buffer information to idle stream.
  //
  HdaStream->BufferActive           = FALSE;
  HdaStream->BufferSource           = NULL;
  HdaStream->BufferSourcePosition   = 0;
  HdaStream->BufferSourceLength     = 0;
  HdaStream->DmaPositionTotal       = 0;

  ZeroMem (HdaStream->BufferData, HDA_STREAM_BUF_SIZE);
} 

VOID
HdaControllerStreamAbort (
  IN HDA_STREAM *HdaStream
  )
{
  ASSERT (HdaStream != NULL);

  HdaControllerStreamIdle (HdaStream);

  //
  // Stop stream and timer.
  //
  HdaControllerSetStreamState (HdaStream, FALSE);
  gBS->SetTimer (HdaStream->PollTimer, TimerCancel, 0);

  //DEBUG ((DEBUG_INFO, "AudioDxe: Stream %u aborted!\n", HdaStream->Index));
}
