/*
 * File: HdaControllerMem.c
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

#include "HdaController.h"
#include <IndustryStandard/HdaRegisters.h>

EFI_STATUS
EFIAPI
HdaControllerInitCorb(
  IN HDA_CONTROLLER_DEV *HdaDev) {
  DEBUG((DEBUG_VERBOSE, "HdaControllerInitCorb(): start\n"));

  // Status and PCI I/O protocol.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

  // HDA register values.
  UINT8 HdaCorbSize;
  UINT32 HdaLowerCorbBaseAddr;
  UINT32 HdaUpperCorbBaseAddr;
  UINT16 HdaCorbWp;
  UINT16 HdaCorbRp;
  UINT64 HdaCorbRpPollResult;

  // CORB buffer.
  VOID *CorbBuffer = NULL;
  UINTN CorbLength;
  UINTN CorbLengthActual;
  VOID *CorbMapping = NULL;
  EFI_PHYSICAL_ADDRESS CorbPhysAddr;

  // Get value of CORBSIZE register.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBSIZE, 1, &HdaCorbSize);
  if (EFI_ERROR(Status))
    return Status;

  // Determine size of CORB.
  if (HdaCorbSize & HDA_REG_CORBSIZE_CORBSZCAP_256) {
    CorbLength = 256 * HDA_CORB_ENTRY_SIZE;
    HdaCorbSize = (HdaCorbSize & ~HDA_REG_CORBSIZE_MASK) | HDA_REG_CORBSIZE_ENT256;
  } else if (HdaCorbSize & HDA_REG_CORBSIZE_CORBSZCAP_16) {
    CorbLength = 16 * HDA_CORB_ENTRY_SIZE;
    HdaCorbSize = (HdaCorbSize & ~HDA_REG_CORBSIZE_MASK) | HDA_REG_CORBSIZE_ENT16;
  } else if (HdaCorbSize & HDA_REG_CORBSIZE_CORBSZCAP_2) {
    CorbLength = 2 * HDA_CORB_ENTRY_SIZE;
    HdaCorbSize = (HdaCorbSize & ~HDA_REG_CORBSIZE_MASK) | HDA_REG_CORBSIZE_ENT2;
  } else {
    // Unsupported size.
    return EFI_UNSUPPORTED;
  }

  // Allocate outbound buffer.
  Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(CorbLength), &CorbBuffer, 0);
  if (EFI_ERROR(Status))
    return Status;
  ZeroMem(CorbBuffer, CorbLength);

  // Map outbound buffer.
  CorbLengthActual = CorbLength;
  Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, CorbBuffer, &CorbLengthActual, &CorbPhysAddr, &CorbMapping);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;
  if (CorbLengthActual != CorbLength) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_BUFFER;
  }

  // Disable CORB.
  Status = HdaControllerSetCorb(HdaDev, FALSE);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;

  // Set outbound buffer lower base address.
  HdaLowerCorbBaseAddr = (UINT32)CorbPhysAddr;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_CORBLBASE, 1, &HdaLowerCorbBaseAddr);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;

  // If 64-bit supported, set upper base address.
  if (HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
    HdaUpperCorbBaseAddr = (UINT32)(CorbPhysAddr >> 32);
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_CORBUBASE, 1, &HdaUpperCorbBaseAddr);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
      goto FREE_BUFFER;
  }

  // Reset write pointer to zero.
  HdaCorbWp = 0;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaCorbWp);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;

  // Reset read pointer by setting bit.
  HdaCorbRp = HDA_REG_CORBRP_RST;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbRp);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;

  // Docs state we need to clear the bit and wait for it to clear.
  HdaCorbRp = 0;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbRp);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;
  Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, HDA_REG_CORBRP_RST, 0, 50000000, &HdaCorbRpPollResult);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;

  // Populate device object properties.
  HdaDev->CorbBuffer = CorbBuffer;
  HdaDev->CorbEntryCount = (UINT32)(CorbLength / HDA_CORB_ENTRY_SIZE);
  HdaDev->CorbMapping = CorbMapping;
  HdaDev->CorbPhysAddr = CorbPhysAddr;
  HdaDev->CorbWritePointer = HdaCorbWp;

  // Buffer allocation successful.
  DEBUG((DEBUG_VERBOSE, "HDA controller CORB allocated @ 0x%p (0x%p) (%u entries)\n",
    HdaDev->CorbBuffer, HdaDev->CorbPhysAddr, HdaDev->CorbEntryCount));
  return EFI_SUCCESS;

FREE_BUFFER:
  // Unmap if needed.
  if (CorbMapping)
    PciIo->Unmap(PciIo, CorbMapping);

  // Free buffer.
  PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(CorbLength), CorbBuffer);
  return Status;
}

EFI_STATUS
EFIAPI
HdaControllerCleanupCorb(
  IN HDA_CONTROLLER_DEV *HdaDev) {
  DEBUG((DEBUG_VERBOSE, "HdaControllerCleanupCorb(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

  // Stop CORB.
  Status = HdaControllerSetCorb(HdaDev, FALSE);
  if (EFI_ERROR(Status))
    return Status;

  // Unmap CORB and free buffer.
  if (HdaDev->CorbMapping)
    PciIo->Unmap(PciIo, HdaDev->CorbMapping);
  PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(HdaDev->CorbEntryCount * HDA_CORB_ENTRY_SIZE), HdaDev->CorbBuffer);

  // Clear device object properties.
  HdaDev->CorbBuffer = NULL;
  HdaDev->CorbEntryCount = 0;
  HdaDev->CorbMapping = NULL;
  HdaDev->CorbPhysAddr = 0;
  HdaDev->CorbWritePointer = 0;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerSetCorb(
  IN HDA_CONTROLLER_DEV *HdaDev,
  IN BOOLEAN Enable) {
  DEBUG((DEBUG_VERBOSE, "HdaControllerSetCorb(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
  UINT8 HdaCorbCtl;
  UINT64 Tmp;

  // Get current value of CORBCTL.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, 1, &HdaCorbCtl);
  if (EFI_ERROR(Status))
    return Status;

  // Change CORB operation.
  if (Enable)
    HdaCorbCtl |= HDA_REG_CORBCTL_CORBRUN;
  else
    HdaCorbCtl &= ~HDA_REG_CORBCTL_CORBRUN;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, 1, &HdaCorbCtl);
  if (EFI_ERROR(Status))
    return Status;

  // Wait for bit to cycle.
  return PciIo->PollMem(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, HDA_REG_CORBCTL_CORBRUN,
    Enable ? HDA_REG_CORBCTL_CORBRUN : 0, MS_TO_NANOSECOND(50), &Tmp);
}

EFI_STATUS
EFIAPI
HdaControllerInitRirb(
  IN HDA_CONTROLLER_DEV *HdaDev) {
  DEBUG((DEBUG_VERBOSE, "HdaControllerInitRirb(): start\n"));

  // Status and PCI I/O protocol.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

  // HDA register values.
  UINT8 HdaRirbSize;
  UINT32 HdaLowerRirbBaseAddr;
  UINT32 HdaUpperRirbBaseAddr;
  UINT16 HdaRirbWp;

  // RIRB buffer.
  VOID *RirbBuffer = NULL;
  UINTN RirbLength;
  UINTN RirbLengthActual;
  VOID *RirbMapping = NULL;
  EFI_PHYSICAL_ADDRESS RirbPhysAddr;

  // Get value of RIRBSIZE register.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBSIZE, 1, &HdaRirbSize);
  if (EFI_ERROR(Status))
    return Status;

  // Determine size of RIRB.
  if (HdaRirbSize & HDA_REG_RIRBSIZE_RIRBSZCAP_256) {
    RirbLength = 256 * HDA_RIRB_ENTRY_SIZE;
    HdaRirbSize = (HdaRirbSize & ~HDA_REG_RIRBSIZE_MASK) | HDA_REG_RIRBSIZE_ENT256;
  } else if (HdaRirbSize & HDA_REG_RIRBSIZE_RIRBSZCAP_16) {
    RirbLength = 16 * HDA_RIRB_ENTRY_SIZE;
    HdaRirbSize = (HdaRirbSize & ~HDA_REG_RIRBSIZE_MASK) | HDA_REG_RIRBSIZE_ENT16;
  } else if (HdaRirbSize & HDA_REG_RIRBSIZE_RIRBSZCAP_2) {
    RirbLength = 2 * HDA_RIRB_ENTRY_SIZE;
    HdaRirbSize = (HdaRirbSize & ~HDA_REG_RIRBSIZE_MASK) | HDA_REG_RIRBSIZE_ENT2;
  } else {
    // Unsupported size.
    return EFI_UNSUPPORTED;
  }

  // Allocate outbound buffer.
  Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(RirbLength), &RirbBuffer, 0);
  if (EFI_ERROR(Status))
    return Status;
  ZeroMem(RirbBuffer, RirbLength);

  // Map outbound buffer.
  RirbLengthActual = RirbLength;
  Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, RirbBuffer, &RirbLengthActual, &RirbPhysAddr, &RirbMapping);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;
  if (RirbLengthActual != RirbLength) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_BUFFER;
  }

  // Disable RIRB.
  Status = HdaControllerSetRirb(HdaDev, FALSE);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;

  // Set outbound buffer lower base address.
  HdaLowerRirbBaseAddr = (UINT32)RirbPhysAddr;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_RIRBLBASE, 1, &HdaLowerRirbBaseAddr);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;

  // If 64-bit supported, set upper base address.
  if (HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
    HdaUpperRirbBaseAddr = (UINT32)(RirbPhysAddr >> 32);
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_RIRBUBASE, 1, &HdaUpperRirbBaseAddr);
    if (EFI_ERROR(Status))
      goto FREE_BUFFER;
  }

  // Reset write pointer by setting reset bit.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWp);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;
  HdaRirbWp |= HDA_REG_RIRBWP_RST;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWp);
  if (EFI_ERROR(Status))
    goto FREE_BUFFER;

  // Populate device object properties.
  HdaDev->RirbBuffer = RirbBuffer;
  HdaDev->RirbEntryCount = (UINT32)(RirbLength / HDA_RIRB_ENTRY_SIZE);
  HdaDev->RirbMapping = RirbMapping;
  HdaDev->RirbPhysAddr = RirbPhysAddr;
  HdaDev->RirbReadPointer = 0;

  // Buffer allocation successful.
  DEBUG((DEBUG_VERBOSE, "HDA controller RIRB allocated @ 0x%p (0x%p) (%u entries)\n",
    HdaDev->RirbBuffer, HdaDev->RirbPhysAddr, HdaDev->RirbEntryCount));
  return EFI_SUCCESS;

FREE_BUFFER:
  // Unmap if needed.
  if (RirbMapping)
    PciIo->Unmap(PciIo, RirbMapping);

  // Free buffer.
  PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(RirbLength), RirbBuffer);
  return Status;
}

EFI_STATUS
EFIAPI
HdaControllerCleanupRirb(
  IN HDA_CONTROLLER_DEV *HdaDev) {
  DEBUG((DEBUG_VERBOSE, "HdaControllerCleanupRirb(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

  // Stop RIRB.
  Status = HdaControllerSetRirb(HdaDev, FALSE);
  if (EFI_ERROR(Status))
    return Status;

  // Unmap RIRB and free buffer.
  if (HdaDev->RirbMapping)
    PciIo->Unmap(PciIo, HdaDev->RirbMapping);
  PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(HdaDev->RirbEntryCount * HDA_RIRB_ENTRY_SIZE), HdaDev->RirbBuffer);

  // Clear device object properties.
  HdaDev->RirbBuffer = NULL;
  HdaDev->RirbEntryCount = 0;
  HdaDev->RirbMapping = NULL;
  HdaDev->RirbPhysAddr = 0;
  HdaDev->RirbReadPointer = 0;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerSetRirb(
  IN HDA_CONTROLLER_DEV *HdaDev,
  IN BOOLEAN Enable) {
  DEBUG((DEBUG_VERBOSE, "HdaControllerSetRirb(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
  UINT8 HdaRirbCtl;
  UINT64 Tmp;

  // Get current value of RIRBCTL.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
  if (EFI_ERROR(Status))
    return Status;

  // Change RIRB operation.
  if (Enable)
    HdaRirbCtl |= HDA_REG_RIRBCTL_RIRBDMAEN;
  else
    HdaRirbCtl &= ~HDA_REG_RIRBCTL_RIRBDMAEN;
  Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
  if (EFI_ERROR(Status))
    return Status;

  // Wait for bit to cycle.
  return PciIo->PollMem(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, HDA_REG_RIRBCTL_RIRBDMAEN,
    Enable ? HDA_REG_RIRBCTL_RIRBDMAEN : 0, MS_TO_NANOSECOND(50), &Tmp);
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
      (EFI_EVENT_NOTIFY)HdaControllerStreamPollTimerHandler, HdaStream, &HdaStream->PollTimer);
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
