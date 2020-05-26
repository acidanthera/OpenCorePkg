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

//
// Command Output Ring Buffer - used for sending commands to codecs.
// Response Input Ring Buffer - used for receiving responses from codecs.
//

BOOLEAN
HdaControllerInitRingBuffer (
  IN HDA_RING_BUFFER    *HdaRingBuffer
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

  PciIo = HdaRingBuffer->HdaDev->PciIo;

  if (HdaRingBuffer->Type == HDA_RING_BUFFER_TYPE_CORB) {
    Offset          = HDA_REG_CORB_BASE;
    EntrySize       = HDA_CORB_ENTRY_SIZE;
  } else if (HdaRingBuffer->Type == HDA_RING_BUFFER_TYPE_RIRB) {
    Offset          = HDA_REG_RIRB_BASE;
    EntrySize       = HDA_RIRB_ENTRY_SIZE;
  } else {
    return FALSE;
  }

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
}

BOOLEAN
HdaControllerSetRingBufferState(
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

EFI_STATUS
EFIAPI
HdaControllerInitStreams(
  IN HDA_CONTROLLER_DEV *HdaControllerDev) {
  DEBUG((DEBUG_VERBOSE, "HdaControllerInitStreams(): start\n"));

  // Status and PCI I/O protocol.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaControllerDev->PciIo;
  UINT32 LowerBaseAddr;
  UINT32 UpperBaseAddr;
  EFI_PHYSICAL_ADDRESS DataBlockAddr;
  HDA_STREAM *HdaStream;

  // Buffers.
  UINTN BdlLengthActual;
  UINTN DataLengthActual;
  UINTN DmaPositionsLengthActual;

  // Reset stream ID bitmap so stream 0 is allocated (reserved).
  HdaControllerDev->StreamIdMapping = BIT0;

  // Determine number of streams.
  HdaControllerDev->BidirStreamsCount = HDA_REG_GCAP_BSS(HdaControllerDev->Capabilities);
  HdaControllerDev->InputStreamsCount = HDA_REG_GCAP_ISS(HdaControllerDev->Capabilities);
  HdaControllerDev->OutputStreamsCount = HDA_REG_GCAP_OSS(HdaControllerDev->Capabilities);
  HdaControllerDev->TotalStreamsCount = HdaControllerDev->BidirStreamsCount +
    HdaControllerDev->InputStreamsCount + HdaControllerDev->OutputStreamsCount;

  // Initialize stream arrays.
  HdaControllerDev->BidirStreams = AllocateZeroPool(sizeof(HDA_STREAM) * HdaControllerDev->BidirStreamsCount);
  HdaControllerDev->InputStreams = AllocateZeroPool(sizeof(HDA_STREAM) * HdaControllerDev->InputStreamsCount);
  HdaControllerDev->OutputStreams = AllocateZeroPool(sizeof(HDA_STREAM) * HdaControllerDev->OutputStreamsCount);

  // Initialize streams.
  UINT8 InputStreamsOffset = HdaControllerDev->BidirStreamsCount;
  UINT8 OutputStreamsOffset = InputStreamsOffset + HdaControllerDev->InputStreamsCount;
  DEBUG((DEBUG_VERBOSE, "HdaControllerInitStreams(): in offset %u, out offset %u\n", InputStreamsOffset, OutputStreamsOffset));
  for (UINT8 i = 0; i < HdaControllerDev->TotalStreamsCount; i++) {
    // Get pointer to stream and set type.
    if (i < InputStreamsOffset) {
      HdaStream = HdaControllerDev->BidirStreams + i;
      HdaStream->Type = HDA_STREAM_TYPE_BIDIR;
    } else if (i < OutputStreamsOffset) {
      HdaStream = HdaControllerDev->InputStreams + (i - InputStreamsOffset);
      HdaStream->Type = HDA_STREAM_TYPE_IN;
    } else {
      HdaStream = HdaControllerDev->OutputStreams + (i - OutputStreamsOffset);
      HdaStream->Type = HDA_STREAM_TYPE_OUT;
    }

    // Set parent controller and index.
    HdaStream->HdaControllerDev = HdaControllerDev;
    HdaStream->Index = i;
    HdaStream->Output = (HdaStream->Type == HDA_STREAM_TYPE_OUT);

    // Initialize polling timer.
    Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_NOTIFY,
      (EFI_EVENT_NOTIFY)HdaControllerStreamOutputPollTimerHandler, HdaStream, &HdaStream->PollTimer);
    if (EFI_ERROR(Status))
      goto FREE_BUFFER;

    // Allocate buffer descriptor list.
    Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(HDA_BDL_SIZE),
      (VOID**)&HdaStream->BufferList, 0);
    if (EFI_ERROR(Status))
      goto FREE_BUFFER;
    ZeroMem(HdaStream->BufferList, HDA_BDL_SIZE);

    // Map buffer descriptor list.
    BdlLengthActual = HDA_BDL_SIZE;
    Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, HdaStream->BufferList, &BdlLengthActual,
      &HdaStream->BufferListPhysAddr, &HdaStream->BufferListMapping);
    if (EFI_ERROR(Status))
      goto FREE_BUFFER;
    if (BdlLengthActual != HDA_BDL_SIZE) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FREE_BUFFER;
    }

    // Reset stream.
    Status = HdaControllerResetStream(HdaStream);
    if (EFI_ERROR(Status))
      goto FREE_BUFFER;

    // Allocate buffer for data.
    Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(HDA_STREAM_BUF_SIZE),
      (VOID**)&HdaStream->BufferData, 0);
    if (EFI_ERROR(Status))
      goto FREE_BUFFER;
    ZeroMem(HdaStream->BufferData, HDA_STREAM_BUF_SIZE);

    // Map buffer descriptor list.
    DataLengthActual = HDA_STREAM_BUF_SIZE;
    Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, HdaStream->BufferData, &DataLengthActual,
      &HdaStream->BufferDataPhysAddr, &HdaStream->BufferDataMapping);
    if (EFI_ERROR(Status))
      goto FREE_BUFFER;
    if (DataLengthActual != HDA_STREAM_BUF_SIZE) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FREE_BUFFER;
    }

    // Fill buffer list.
    for (UINTN b = 0; b < HDA_BDL_ENTRY_COUNT; b++) {
      // Set address and length of entry.
      DataBlockAddr = HdaStream->BufferDataPhysAddr + (b * HDA_BDL_BLOCKSIZE);
      HdaStream->BufferList[b].Address = (UINT32)DataBlockAddr;
      HdaStream->BufferList[b].AddressHigh = (UINT32)(DataBlockAddr >> 32);
      HdaStream->BufferList[b].Length = HDA_BDL_BLOCKSIZE;
      HdaStream->BufferList[b].InterruptOnCompletion = TRUE;
    }
  }

  // Allocate space for DMA positions structure.
  HdaControllerDev->DmaPositionsSize = sizeof(HDA_DMA_POS_ENTRY) * HdaControllerDev->TotalStreamsCount;
  Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData,
    EFI_SIZE_TO_PAGES(HdaControllerDev->DmaPositionsSize), (VOID**)&HdaControllerDev->DmaPositions, 0);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;
  ZeroMem(HdaControllerDev->DmaPositions, HdaControllerDev->DmaPositionsSize);

  // Map buffer descriptor list.
  DmaPositionsLengthActual = HdaControllerDev->DmaPositionsSize;
  Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, HdaControllerDev->DmaPositions,
    &DmaPositionsLengthActual, &HdaControllerDev->DmaPositionsPhysAddr, &HdaControllerDev->DmaPositionsMapping);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;
  if (DmaPositionsLengthActual != HdaControllerDev->DmaPositionsSize) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_BUFFER;
  }

  // Set DMA positions lower base address.
  LowerBaseAddr = ((UINT32)HdaControllerDev->DmaPositionsPhysAddr) | HDA_REG_DPLBASE_EN;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_DPLBASE, 1, &LowerBaseAddr);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;

  // If 64-bit supported, set DMA positions upper base address.
  if (HdaControllerDev->Capabilities & HDA_REG_GCAP_64OK) {
    UpperBaseAddr = (UINT32)(HdaControllerDev->DmaPositionsPhysAddr >> 32);
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_DPUBASE, 1, &UpperBaseAddr);
    if (EFI_ERROR(Status))
      goto FREE_BUFFER;
  }

  // Success.
  return EFI_SUCCESS;

FREE_BUFFER:
  // Free stream arrays. TODO
  FreePool(HdaControllerDev->BidirStreams);
  FreePool(HdaControllerDev->InputStreams);
  FreePool(HdaControllerDev->OutputStreams);
  HdaControllerDev->BidirStreams = NULL;
  HdaControllerDev->InputStreams = NULL;
  HdaControllerDev->OutputStreams = NULL;
  HdaControllerDev->TotalStreamsCount = 0;
  return Status;
}

EFI_STATUS
EFIAPI
HdaControllerResetStream(
  IN HDA_STREAM *HdaStream) {
  if (HdaStream == NULL)
    return EFI_INVALID_PARAMETER;

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaStream->HdaControllerDev->PciIo;
  UINT32 LowerBaseAddr;
  UINT32 UpperBaseAddr;
  UINT64 Tmp;

  // Stream regs.
  UINT8 HdaStreamCtl1;
  UINT16 StreamLvi;
  UINT32 StreamCbl;

  // Disable stream.
  Status = HdaControllerSetStreamId(HdaStream, 0);
  if (EFI_ERROR(Status))
    return Status;

  // Get value of control register.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR(Status))
    return Status;

  // Reset stream and wait for bit to be set.
  HdaStreamCtl1 |= HDA_REG_SDNCTL1_SRST;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR(Status))
    return Status;
  Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index),
    HDA_REG_SDNCTL1_SRST, HDA_REG_SDNCTL1_SRST, MS_TO_NANOSECOND(100), &Tmp);
  if (EFI_ERROR(Status))
    return Status;

  // Get value of control register.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR(Status))
    return Status;

  // Docs state we need to clear the bit and wait for it to clear.
  HdaStreamCtl1 &= ~HDA_REG_SDNCTL1_SRST;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR(Status))
    return Status;
  Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index),
    HDA_REG_SDNCTL1_SRST, 0, MS_TO_NANOSECOND(100), &Tmp);
  if (EFI_ERROR(Status))
    return Status;

  // Set buffer list lower base address.
  LowerBaseAddr = (UINT32)HdaStream->BufferListPhysAddr;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNBDPL(HdaStream->Index), 1, &LowerBaseAddr);
  if (EFI_ERROR(Status))
    return Status;

  // If 64-bit supported, set buffer list upper base address.
  if (HdaStream->HdaControllerDev->Capabilities & HDA_REG_GCAP_64OK) {
    UpperBaseAddr = (UINT32)(HdaStream->BufferListPhysAddr >> 32);
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNBDPU(HdaStream->Index), 1, &UpperBaseAddr);
    if (EFI_ERROR(Status))
      return Status;
  }

  // Set last valid index (LVI).
  StreamLvi = HDA_BDL_ENTRY_LAST;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_SDNLVI(HdaStream->Index), 1, &StreamLvi);
  if (EFI_ERROR(Status))
    return Status;

  // Set total buffer length.
  StreamCbl = HDA_STREAM_BUF_SIZE;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNCBL(HdaStream->Index), 1, &StreamCbl);
  if (EFI_ERROR(Status))
    return Status;

  HdaStream->DmaPositionTotal = 0;
  HdaStream->DmaPositionLast = 0;

  return EFI_SUCCESS;
}

VOID
EFIAPI
HdaControllerCleanupStreams(
  IN HDA_CONTROLLER_DEV *HdaControllerDev) {
  DEBUG((DEBUG_VERBOSE, "HdaControllerInitStreams(): start\n"));

  // Status and PCI I/O protocol.
  EFI_PCI_IO_PROTOCOL *PciIo = HdaControllerDev->PciIo;
   UINT8 InputStreamsOffset = HdaControllerDev->BidirStreamsCount;
  UINT8 OutputStreamsOffset = InputStreamsOffset + HdaControllerDev->InputStreamsCount;
  HDA_STREAM *HdaStream;
  UINT32 Tmp;

  // Clean streams.
  for (UINT8 i = 0; i < HdaControllerDev->TotalStreamsCount; i++) {
    // Get pointer to stream and set type.
    if (i < InputStreamsOffset) {
      HdaStream = HdaControllerDev->BidirStreams + i;
      HdaStream->Type = HDA_STREAM_TYPE_BIDIR;
    } else if (i < OutputStreamsOffset) {
      HdaStream = HdaControllerDev->InputStreams + (i - InputStreamsOffset);
      HdaStream->Type = HDA_STREAM_TYPE_IN;
    } else {
      HdaStream = HdaControllerDev->OutputStreams + (i - OutputStreamsOffset);
      HdaStream->Type = HDA_STREAM_TYPE_OUT;
    }

    // Close polling timer.
    gBS->CloseEvent(HdaStream->PollTimer);

    // Stop stream.
    HdaControllerSetStreamId(HdaStream, 0);

    // Unmap and free buffer descriptor list.
    if (HdaStream->BufferListMapping != NULL)
      PciIo->Unmap(PciIo, HdaStream->BufferListMapping);
    if (HdaStream->BufferList != NULL)
      PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(HDA_BDL_SIZE), HdaStream->BufferList);

    // Unmap and free data buffer.
    if (HdaStream->BufferDataMapping != NULL)
      PciIo->Unmap(PciIo, HdaStream->BufferDataMapping);
    if (HdaStream->BufferData != NULL)
      PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(HDA_STREAM_BUF_SIZE), HdaStream->BufferData);
  }

  // Clear DMA positions structure base address.
  Tmp = 0;
  PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_DPLBASE, 1, &Tmp);
  if (HdaControllerDev->Capabilities & HDA_REG_GCAP_64OK) {
    Tmp = 0;
    PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_DPUBASE, 1, &Tmp);
  }

  // Unmap and free DMA positions structure.
  if (HdaControllerDev->DmaPositionsMapping != NULL)
    PciIo->Unmap(PciIo, HdaControllerDev->DmaPositionsMapping);
  if (HdaControllerDev->DmaPositions != NULL)
    PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(HdaControllerDev->DmaPositionsSize), HdaControllerDev->DmaPositions);
}

EFI_STATUS
EFIAPI
HdaControllerGetStream(
  IN  HDA_STREAM *HdaStream,
  OUT BOOLEAN *Run) {
  if ((HdaStream == NULL) || (Run == NULL))
    return EFI_INVALID_PARAMETER;
  //DEBUG((DEBUG_INFO, "HdaControllerGetStream(%u): start\n", HdaStream->Index));

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaStream->HdaControllerDev->PciIo;
  UINT8 HdaStreamCtl1;

  // Get current value of register.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR(Status))
    return Status;

  // Success.
  *Run = (HdaStreamCtl1 & HDA_REG_SDNCTL1_RUN) != 0;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerSetStream(
  IN HDA_STREAM *HdaStream,
  IN BOOLEAN Run) {
  if (HdaStream == NULL)
    return EFI_INVALID_PARAMETER;
  DEBUG((DEBUG_VERBOSE, "HdaControllerSetStream(%u, %u): start\n", HdaStream->Index, Run));

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaStream->HdaControllerDev->PciIo;
  UINT8 HdaStreamCtl1;
  UINT64 Tmp;

  // Get current value of register.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR(Status))
    return Status;

  // Update stream operation.
  if (Run)
    HdaStreamCtl1 |= HDA_REG_SDNCTL1_RUN;
  else
    HdaStreamCtl1 &= ~HDA_REG_SDNCTL1_RUN;

  // Write register.
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
  if (EFI_ERROR(Status))
    return Status;

  // Wait for bit to be set or cleared.
  return PciIo->PollMem(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index),
    HDA_REG_SDNCTL1_RUN, HdaStreamCtl1 & HDA_REG_SDNCTL1_RUN, MS_TO_NANOSECOND(10), &Tmp);
}

EFI_STATUS
EFIAPI
HdaControllerGetStreamLinkPos(
  IN  HDA_STREAM *HdaStream,
  OUT UINT32 *Position) {
  if ((HdaStream == NULL) || (Position == NULL))
    return EFI_INVALID_PARAMETER;
  //DEBUG((DEBUG_INFO, "HdaControllerGetStreamLinkPos(%u): start\n", HdaStream->Index));

  // Get current value of register.
  EFI_PCI_IO_PROTOCOL *PciIo = HdaStream->HdaControllerDev->PciIo;
  return PciIo->Mem.Read(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNLPIB(HdaStream->Index), 1, Position);
}

EFI_STATUS
EFIAPI
HdaControllerGetStreamId(
  IN  HDA_STREAM *HdaStream,
  OUT UINT8 *Index) {
  if ((HdaStream == NULL) || (Index == NULL))
    return EFI_INVALID_PARAMETER;
  //DEBUG((DEBUG_INFO, "HdaControllerGetStreamId(%u): start\n", HdaStream->Index));

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaStream->HdaControllerDev->PciIo;
  UINT8 HdaStreamCtl3;

  // Get current value of register.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3(HdaStream->Index), 1, &HdaStreamCtl3);
  if (EFI_ERROR(Status))
    return Status;

  // Update stream index.
  *Index = HDA_REG_SDNCTL3_STRM_GET(HdaStreamCtl3);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerSetStreamId(
  IN HDA_STREAM *HdaStream,
  IN UINT8 Index) {
  if (HdaStream == NULL)
    return EFI_INVALID_PARAMETER;
  //DEBUG((DEBUG_INFO, "HdaControllerSetStreamId(%u, %u): start\n", HdaStream->Index, Index));

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaStream->HdaControllerDev->PciIo;
  UINT8 HdaStreamCtl3;

  // Stop stream.
  Status = HdaControllerSetStream(HdaStream, FALSE);
  if (EFI_ERROR(Status))
    return Status;

  // Get current value of register.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3(HdaStream->Index), 1, &HdaStreamCtl3);
  if (EFI_ERROR(Status))
    return Status;

  // Update stream index.
  HdaStreamCtl3 = HDA_REG_SDNCTL3_STRM_SET(HdaStreamCtl3, Index);

  // Write register.
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3(HdaStream->Index), 1, &HdaStreamCtl3);
  if (EFI_ERROR(Status))
    return Status;

  // Success.
  return EFI_SUCCESS;
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
  HdaControllerSetStream (HdaStream, FALSE);
  gBS->SetTimer (HdaStream->PollTimer, TimerCancel, 0);

  //DEBUG ((DEBUG_INFO, "AudioDxe: Stream %u aborted!\n", HdaStream->Index));
}
