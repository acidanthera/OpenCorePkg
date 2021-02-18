/*
 * File: HdaController.c
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
#include "HdaControllerComponentName.h"

#include <Library/OcGuardLib.h>
#include <Library/OcHdaDevicesLib.h>
#include <Library/OcStringLib.h>
#include <Protocol/VMwareHda.h>

VOID
EFIAPI
HdaControllerStreamOutputPollTimerHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS            Status;
  HDA_STREAM            *HdaStream;
  EFI_PCI_IO_PROTOCOL   *PciIo;

  UINT8                 HdaStreamSts;
  UINT32                HdaStreamDmaPos;
  UINT32                HdaSourceLength;
  UINT32                HdaCurrentBlock;
  UINT32                HdaNextBlock;

  UINT32                DmaChanged;
  UINT32                Tmp;

  HdaStream       = (HDA_STREAM*)Context;
  PciIo           = HdaStream->HdaDev->PciIo;
  HdaStreamSts    = 0;

  //
  // Get current stream status bits.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthFifoUint8, PCI_HDA_BAR, HDA_REG_SDNSTS (HdaStream->Index), 1, &HdaStreamSts);
  if (EFI_ERROR (Status) || ((HdaStreamSts & (HDA_REG_SDNSTS_FIFOE | HDA_REG_SDNSTS_DESE)) != 0)) {
    HdaControllerStreamAbort (HdaStream);
    return;
  }

  if (HdaStream->UseLpib) {
    //
    // Get stream position through LPIB register.
    //
    Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthFifoUint32, PCI_HDA_BAR, HDA_REG_SDNLPIB (HdaStream->Index), 1, &HdaStreamDmaPos);
    if (EFI_ERROR (Status)) {
      HdaControllerStreamAbort (HdaStream);
      return;
    }
  } else {
    //
    // Get stream position through DMA positions buffer.
    //
    HdaStreamDmaPos = HdaStream->HdaDev->DmaPositions[HdaStream->Index].Position;

    //
    // If zero, give the stream a few cycles to catch up before falling back to LPIB.
    // Fallback occurs after the set amount of cycles the DMA position is zero.
    //
    if (HdaStreamDmaPos == 0 && !HdaStream->DmaCheckComplete) {
      if (HdaStream->DmaCheckCount >= HDA_STREAM_DMA_CHECK_THRESH) {
        HdaStream->UseLpib = TRUE;
      }

      //
      // Get stream position through LPIB register in the meantime.
      //
      DEBUG ((DEBUG_VERBOSE, "AudioDxe: Falling back to LPIB after %u more tries!\n", HDA_STREAM_DMA_CHECK_THRESH - HdaStream->DmaCheckCount));
      Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthFifoUint32, PCI_HDA_BAR, HDA_REG_SDNLPIB (HdaStream->Index), 1, &HdaStreamDmaPos);
      if (EFI_ERROR (Status)) {
        HdaControllerStreamAbort (HdaStream);
        return;
      }
    }
  }

  //
  // Increment cycle counter. Once complete, store status to avoid false fallbacks later on.
  //
  if (HdaStream->DmaCheckCount < HDA_STREAM_DMA_CHECK_THRESH) {
    HdaStream->DmaCheckCount++;
  } else {
    HdaStream->DmaCheckComplete = TRUE;
  }
  
  if (HdaStreamDmaPos >= HdaStream->DmaPositionLast) {
    DmaChanged = HdaStreamDmaPos - HdaStream->DmaPositionLast;
  } else {
    DmaChanged = (HDA_STREAM_BUF_SIZE - HdaStream->DmaPositionLast) + HdaStreamDmaPos;
  }
  HdaStream->DmaPositionLast = HdaStreamDmaPos;

  if (HdaStream->BufferActive) {
    if (OcOverflowAddU32 (HdaStream->DmaPositionTotal, DmaChanged, &HdaStream->DmaPositionTotal)) {
      HdaControllerStreamAbort (HdaStream);
      return;
    }

    //
    // Padding added to account for delay between DMA transfer to controller and actual playback.
    //
    if (HdaStream->DmaPositionTotal > HdaStream->BufferSourceLength + HDA_STREAM_BUFFER_PADDING) {
      DEBUG ((DEBUG_VERBOSE, "AudioDxe: Completed playback of 0x%X buffer with 0x%X bytes read, current DMA: 0x%X\n", HdaStream->BufferSourceLength, HdaStream->DmaPositionTotal, HdaStreamDmaPos));
      HdaControllerStreamIdle (HdaStream);

      //
      // TODO: Remove when continous stream run is implemented.
      //
      HdaControllerStreamAbort (HdaStream);

      if (HdaStream->Callback != NULL) {
        HdaStream->Callback (EfiHdaIoTypeOutput, HdaStream->CallbackContext1, HdaStream->CallbackContext2, HdaStream->CallbackContext3);
      }
    }

    //
    // Fill next block on IOC.
    //
    if (HdaStreamSts & HDA_REG_SDNSTS_BCIS && HdaStream->BufferSourcePosition < HdaStream->BufferSourceLength) {
      HdaCurrentBlock = HdaStreamDmaPos / HDA_BDL_BLOCKSIZE;
      HdaNextBlock    = HdaCurrentBlock + 1;
      HdaNextBlock    %= HDA_BDL_ENTRY_COUNT;

      HdaSourceLength = HDA_BDL_BLOCKSIZE;
      
      if (OcOverflowAddU32 (HdaStream->BufferSourcePosition, HdaSourceLength, &Tmp)) {
        HdaControllerStreamAbort (HdaStream);
        return;
      }
      if (Tmp > HdaStream->BufferSourceLength) {
        HdaSourceLength = HdaStream->BufferSourceLength - HdaStream->BufferSourcePosition;
      }

      //
      // Copy data to DMA buffer.
      //
      if (HdaSourceLength < HDA_BDL_BLOCKSIZE) {
        ZeroMem (HdaStream->BufferData + HdaNextBlock * HDA_BDL_BLOCKSIZE, HDA_BDL_BLOCKSIZE);
      }
      CopyMem (HdaStream->BufferData + HdaNextBlock * HDA_BDL_BLOCKSIZE, HdaStream->BufferSource + HdaStream->BufferSourcePosition, HdaSourceLength);
      if (OcOverflowAddU32 (HdaStream->BufferSourcePosition, HdaSourceLength, &HdaStream->BufferSourcePosition)) {
        HdaControllerStreamAbort (HdaStream);
        return;
      }

      DEBUG ((DEBUG_VERBOSE, "AudioDxe: Block %u of %u filled! (current position 0x%X, buffer 0x%X)\n",
        HdaStreamDmaPos / HDA_BDL_BLOCKSIZE, HDA_BDL_ENTRY_COUNT, HdaStreamDmaPos, HdaStream->BufferSourcePosition));
    }
  }

  //
  // Reset IOC bit as needed.
  //
  if (HdaStreamSts & HDA_REG_SDNSTS_BCIS) {
    HdaStreamSts = HDA_REG_SDNSTS_BCIS;
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNSTS (HdaStream->Index), 1, &HdaStreamSts);
    if (EFI_ERROR (Status)) {
      HdaControllerStreamAbort (HdaStream);
      return;
    }
  }
}

EFI_STATUS
EFIAPI
HdaControllerInitPciHw(
  IN HDA_CONTROLLER_DEV *HdaControllerDev
  ) 
{
  DEBUG ((DEBUG_VERBOSE, "HdaControllerInitPciHw(): start\n"));

  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaControllerDev->PciIo;
  UINT64              PciSupports;
  UINT8               HdaTcSel;
  UINT16              HdaDevC;

  PciSupports = 0;

  //
  // Save original PCI I/O attributes and get currently supported ones.
  //
  Status = PciIo->Attributes (PciIo, EfiPciIoAttributeOperationGet, 0, &HdaControllerDev->OriginalPciAttributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  HdaControllerDev->OriginalPciAttributesSaved = TRUE;
  Status = PciIo->Attributes (PciIo, EfiPciIoAttributeOperationSupported, 0, &PciSupports);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Enable the PCI device.
  //
  PciSupports &= EFI_PCI_DEVICE_ENABLE;
  Status = PciIo->Attributes (PciIo, EfiPciIoAttributeOperationEnable, PciSupports, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get vendor and device IDs of PCI device.
  //
  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, PCI_VENDOR_ID_OFFSET, 1, &HdaControllerDev->VendorId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "HdaControllerInitPciHw(): controller %4X:%4X\n",
    GET_PCI_VENDOR_ID (HdaControllerDev->VendorId), GET_PCI_DEVICE_ID (HdaControllerDev->VendorId)));

  if (GET_PCI_VENDOR_ID (HdaControllerDev->VendorId) == VEN_INTEL_ID) {
    //
    // Set TC0 in TCSEL register for Intel controllers.
    //
    Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_TCSEL_OFFSET, 1, &HdaTcSel);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    HdaTcSel &= PCI_HDA_TCSEL_TC0_MASK;
    Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, PCI_HDA_TCSEL_OFFSET, 1, &HdaTcSel);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Get device control PCI register.
  //
  Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_DEVC_OFFSET, 1, &HdaDevC);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // If No Snoop is currently enabled, disable it.
  //
  if (HdaDevC & PCI_HDA_DEVC_NOSNOOPEN) {
    HdaDevC &= ~PCI_HDA_DEVC_NOSNOOPEN;
    Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, PCI_HDA_DEVC_OFFSET, 1, &HdaDevC);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Get major/minor version.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_VMAJ, 1, &HdaControllerDev->MajorVersion);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_VMIN, 1, &HdaControllerDev->MinorVersion);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "HdaControllerInitPciHw(): controller version %u.%u\n",
    HdaControllerDev->MajorVersion, HdaControllerDev->MinorVersion));
  if (HdaControllerDev->MajorVersion < HDA_VERSION_MIN_MAJOR) {
    Status = EFI_UNSUPPORTED;
    return Status;
  }

  //
  // Get capabilities.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_GCAP, 1, &HdaControllerDev->Capabilities);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  DEBUG ((DEBUG_INFO, "HdaControllerInitPciHw(): capabilities:\n  64-bit: %s  Serial Data Out Signals: %u\n",
    HdaControllerDev->Capabilities & HDA_REG_GCAP_64OK ? L"Yes" : L"No",
    HDA_REG_GCAP_NSDO (HdaControllerDev->Capabilities)));
  DEBUG ((DEBUG_INFO, "  Bidir streams: %u  Input streams: %u  Output streams: %u\n",
    HDA_REG_GCAP_BSS (HdaControllerDev->Capabilities), HDA_REG_GCAP_ISS (HdaControllerDev->Capabilities),
    HDA_REG_GCAP_OSS (HdaControllerDev->Capabilities)));

  return EFI_SUCCESS;
}

VOID
EFIAPI
HdaControllerGetName (
  IN HDA_CONTROLLER_DEV *HdaControllerDev
  ) 
{
  DEBUG ((DEBUG_VERBOSE, "HdaControllerGetName(): start\n"));

  //
  // Try to match controller name.
  //
  HdaControllerDev->Name = AsciiStrCopyToUnicode (OcHdaControllerGetName (HdaControllerDev->VendorId), 0);
  DEBUG ((DEBUG_INFO, "HdaControllerGetName(): controller is %s\n", HdaControllerDev->Name));
}

EFI_STATUS
EFIAPI
HdaControllerReset (
  IN HDA_CONTROLLER_DEV *HdaControllerDev
  ) 
{
  DEBUG ((DEBUG_VERBOSE, "HdaControllerReset(): start\n"));

  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaControllerDev->PciIo;
  UINT32              HdaGCtl;
  UINT64              Tmp;

  //
  // Check if the controller is already in reset. If not, clear CRST bit.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &HdaGCtl);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (!(HdaGCtl & HDA_REG_GCTL_CRST)) {
    HdaGCtl &= ~HDA_REG_GCTL_CRST;
    Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &HdaGCtl);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Set CRST bit to begin the process of coming out of reset.
  //
  HdaGCtl |= HDA_REG_GCTL_CRST;
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &HdaGCtl);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Wait for bit to be set. Once bit is set, the controller is ready.
  //
  Status = PciIo->PollMem (PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL,
    HDA_REG_GCTL_CRST, HDA_REG_GCTL_CRST, MS_TO_NANOSECOND (100), &Tmp);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Wait 100ms to ensure all codecs have also reset.
  //
  gBS->Stall (MS_TO_MICROSECOND (100));
  DEBUG ((DEBUG_VERBOSE, "HdaControllerReset(): done\n"));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerScanCodecs (
  IN HDA_CONTROLLER_DEV *HdaControllerDev
  )
{
  DEBUG ((DEBUG_VERBOSE, "HdaControllerScanCodecs(): start\n"));

  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT16                HdaStatests;
  EFI_HDA_IO_VERB_LIST  HdaCodecVerbList;
  UINT32                VendorVerb;
  UINT32                VendorResponse;

  UINTN                 CurrentOutputStreamIndex = 0;

  HDA_IO_PRIVATE_DATA   *HdaIoPrivateData;
  VOID                  *TmpProtocol;
  UINT32                Index;

  if (HdaControllerDev == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PciIo = HdaControllerDev->PciIo;

  //
  // Get STATESTS register.
  //
  Status = PciIo->Mem.Read (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_STATESTS, 1, &HdaStatests);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create verb list with single item.
  //
  VendorVerb = HDA_CODEC_VERB(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_VENDOR_ID);
  ZeroMem(&HdaCodecVerbList, sizeof(EFI_HDA_IO_VERB_LIST));
  HdaCodecVerbList.Count      = 1;
  HdaCodecVerbList.Verbs      = &VendorVerb;
  HdaCodecVerbList.Responses  = &VendorResponse;

  //
  // Iterate through register looking for active codecs.
  //
  for (Index = 0; Index < HDA_MAX_CODECS; Index++) {
    //
    // Do we have a codec at this address?
    //
    if (HdaStatests & (1 << Index)) {
      DEBUG ((DEBUG_VERBOSE, "HdaControllerScanCodecs(): found codec @ 0x%X\n", Index));

      //
      // Try to get the vendor ID. If this fails, ignore the codec.
      //
      VendorResponse = 0;
      Status = HdaControllerSendCommands (HdaControllerDev, (UINT8) Index, HDA_NID_ROOT, &HdaCodecVerbList);
      if ((EFI_ERROR (Status)) || (VendorResponse == 0)) {
        continue;
      }

      HdaIoPrivateData = AllocateZeroPool (sizeof(HDA_IO_PRIVATE_DATA));
      if (HdaIoPrivateData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        return Status;
      }

      HdaIoPrivateData->Signature         = HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE;
      HdaIoPrivateData->HdaCodecAddress   = (UINT8) Index;
      HdaIoPrivateData->HdaControllerDev  = HdaControllerDev;
      HdaIoPrivateData->HdaIo.GetAddress  = HdaControllerHdaIoGetAddress;
      HdaIoPrivateData->HdaIo.SendCommand = HdaControllerHdaIoSendCommand;
      HdaIoPrivateData->HdaIo.SetupStream = HdaControllerHdaIoSetupStream;
      HdaIoPrivateData->HdaIo.CloseStream = HdaControllerHdaIoCloseStream;
      HdaIoPrivateData->HdaIo.GetStream   = HdaControllerHdaIoGetStream;
      HdaIoPrivateData->HdaIo.StartStream = HdaControllerHdaIoStartStream;
      HdaIoPrivateData->HdaIo.StopStream  = HdaControllerHdaIoStopStream;

      //
      // Assign streams.
      //
      if (CurrentOutputStreamIndex < HdaControllerDev->StreamsCount) {
        DEBUG ((DEBUG_VERBOSE, "Assigning output stream %u to codec\n", CurrentOutputStreamIndex));
        HdaIoPrivateData->HdaOutputStream = HdaControllerDev->Streams + CurrentOutputStreamIndex;
        CurrentOutputStreamIndex++;
      }

      HdaControllerDev->HdaIoChildren[Index].PrivateData = HdaIoPrivateData;
    }
  }

  //
  // Clear STATESTS register.
  //
  HdaStatests = HDA_REG_STATESTS_CLEAR;
  Status = PciIo->Mem.Write (PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_STATESTS, 1, &HdaStatests);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install protocols on each codec.
  //
  for (Index = 0; Index < HDA_MAX_CODECS; Index++) {
    if (HdaControllerDev->HdaIoChildren[Index].PrivateData != NULL) {
      // Create Device Path for codec.
      EFI_HDA_IO_DEVICE_PATH HdaIoDevicePathNode; //EFI_HDA_IO_DEVICE_PATH_TEMPLATE;
      HdaIoDevicePathNode.Header.Type       = MESSAGING_DEVICE_PATH;
      HdaIoDevicePathNode.Header.SubType    = MSG_VENDOR_DP;
      HdaIoDevicePathNode.Header.Length[0]  = (UINT8)(sizeof (EFI_HDA_IO_DEVICE_PATH));
      HdaIoDevicePathNode.Header.Length[1]  = (UINT8)((sizeof (EFI_HDA_IO_DEVICE_PATH)) >> 8);
      HdaIoDevicePathNode.Guid              = gEfiHdaIoDevicePathGuid;
      HdaIoDevicePathNode.Address           = (UINT8) Index;
      HdaControllerDev->HdaIoChildren[Index].DevicePath = AppendDevicePathNode (HdaControllerDev->DevicePath, (EFI_DEVICE_PATH_PROTOCOL*)&HdaIoDevicePathNode);
      if (HdaControllerDev->HdaIoChildren[Index].DevicePath == NULL) {
        Status = EFI_INVALID_PARAMETER;
        return Status;
      }

      //
      // Install protocols for the codec. The codec driver will later bind to this.
      //
      HdaControllerDev->HdaIoChildren[Index].Handle = NULL;
      Status = gBS->InstallMultipleProtocolInterfaces (&HdaControllerDev->HdaIoChildren[Index].Handle,
        &gEfiDevicePathProtocolGuid, HdaControllerDev->HdaIoChildren[Index].DevicePath,
        &gEfiHdaIoProtocolGuid, &HdaControllerDev->HdaIoChildren[Index].PrivateData->HdaIo, NULL);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // Connect child to parent.
      //
      Status = gBS->OpenProtocol (HdaControllerDev->ControllerHandle, &gEfiPciIoProtocolGuid, &TmpProtocol,
        HdaControllerDev->DriverBinding->DriverBindingHandle, HdaControllerDev->HdaIoChildren[Index].Handle, EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerSendCommands(
  IN HDA_CONTROLLER_DEV *HdaDev,
  IN UINT8 CodecAddress,
  IN UINT8 Node,
  IN EFI_HDA_IO_VERB_LIST *Verbs) {
  //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): start\n"));

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
  UINT32 *HdaCorb;
  UINT64 *HdaRirb;

  // Ensure parameters are valid.
  if (CodecAddress >= HDA_MAX_CODECS || Verbs == NULL || Verbs->Count < 1)
    return EFI_INVALID_PARAMETER;

  // Get pointers to CORB and RIRB.
  HdaCorb = HdaDev->Corb.Buffer;
  HdaRirb = HdaDev->Rirb.Buffer;

  UINT32 RemainingVerbs;
  UINT32 RemainingResponses;
  UINT16 HdaCorbReadPointer;
  UINT16 HdaRirbWritePointer;
  BOOLEAN ResponseReceived;
  UINT8 ResponseTimeout;
  UINT64 RirbResponse;
  UINT32 VerbCommand;
  BOOLEAN Retry = FALSE;

  // Lock.
  AcquireSpinLock(&HdaDev->SpinLock);

START:
  RemainingVerbs = Verbs->Count;
  RemainingResponses = Verbs->Count;
  do {
    // Keep sending verbs until they are all sent.
    if (RemainingVerbs) {
      // Get current CORB read pointer.
      Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbReadPointer);
      if (EFI_ERROR(Status))
        goto DONE;
      //DEBUG((DEBUG_INFO, "old RP: 0x%X\n", HdaCorbReadPointer));

      // Add verbs to CORB until all of them are added or the CORB becomes full.
      while (RemainingVerbs && ((HdaDev->Corb.Pointer + 1 % HdaDev->Corb.EntryCount) != HdaCorbReadPointer)) {
        // Move write pointer and write verb to CORB.
        HdaDev->Corb.Pointer++;
        HdaDev->Corb.Pointer %= HdaDev->Corb.EntryCount;
        VerbCommand = HDA_CORB_VERB(CodecAddress, Node, Verbs->Verbs[Verbs->Count - RemainingVerbs]);
        HdaCorb[HdaDev->Corb.Pointer] = VerbCommand;

        // Move to next verb.
        RemainingVerbs--;
      }

      // Set CORB write pointer.
      Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaDev->Corb.Pointer);
      if (EFI_ERROR(Status))
        goto DONE;
    }

    // Get responses from RIRB.
    ResponseReceived = FALSE;
    ResponseTimeout = 10;
    while (!ResponseReceived) {
      // Get current RIRB write pointer.
      Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWritePointer);
      if (EFI_ERROR(Status))
        goto DONE;

      // If the read and write pointers differ, there are responses waiting.
      while (HdaDev->Rirb.Pointer != HdaRirbWritePointer) {
        // Increment RIRB read pointer.
        HdaDev->Rirb.Pointer++;
        HdaDev->Rirb.Pointer %= HdaDev->Rirb.EntryCount;

        // Get response and ensure it belongs to the current codec.
        RirbResponse = HdaRirb[HdaDev->Rirb.Pointer];
        if (HDA_RIRB_CAD(RirbResponse) != CodecAddress || HDA_RIRB_UNSOL(RirbResponse)) {
          DEBUG((DEBUG_INFO, "Unknown response!\n"));
          continue;
        }

        // Add response to list.
        Verbs->Responses[Verbs->Count - RemainingResponses] = HDA_RIRB_RESP(RirbResponse);
        RemainingResponses--;
        ResponseReceived = TRUE;
      }

      // If no response still, wait a bit.
      if (!ResponseReceived) {
        // If timeout reached, fail.
        if (!ResponseTimeout) {
          DEBUG((DEBUG_INFO, "Command: 0x%X\n", VerbCommand));
          Status = EFI_TIMEOUT;
          goto TIMEOUT;
        }

        ResponseTimeout--;
        gBS->Stall(MS_TO_MICROSECOND(5));
        if (ResponseTimeout < 5)
          DEBUG((DEBUG_INFO, "%u timeouts reached while waiting for response!\n", ResponseTimeout));
      }
    }

  } while (RemainingVerbs || RemainingResponses);
  Status = EFI_SUCCESS;
  goto DONE;

TIMEOUT:
  DEBUG((DEBUG_INFO, "Timeout!\n"));
  if (!Retry) {
    DEBUG((DEBUG_INFO, "Stall detected, restarting CORB and RIRB!\n"));
    Status = HdaControllerSetRingBufferState (&HdaDev->Corb, FALSE);
    if (EFI_ERROR(Status))
      goto DONE;
    Status = HdaControllerSetRingBufferState (&HdaDev->Rirb, FALSE);
    if (EFI_ERROR(Status))
      goto DONE;
    Status = HdaControllerSetRingBufferState (&HdaDev->Corb, TRUE);
    if (EFI_ERROR(Status))
      goto DONE;
    Status = HdaControllerSetRingBufferState (&HdaDev->Rirb, TRUE);
    if (EFI_ERROR(Status))
      goto DONE;

    // Try again.
    Retry = TRUE;
    goto START;
  }

DONE:
  ReleaseSpinLock(&HdaDev->SpinLock);
  return Status;
}

EFI_STATUS
EFIAPI
HdaControllerInstallProtocols (
  IN HDA_CONTROLLER_DEV *HdaControllerDev
  )
{
  DEBUG ((DEBUG_INFO, "HdaControllerInstallProtocols(): start\n"));

  HDA_CONTROLLER_INFO_PRIVATE_DATA *HdaControllerInfoData;

  HdaControllerInfoData = AllocateZeroPool (sizeof (HDA_CONTROLLER_INFO_PRIVATE_DATA));
  if (HdaControllerInfoData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HdaControllerInfoData->Signature                     = HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE;
  HdaControllerInfoData->HdaControllerDev              = HdaControllerDev;
  HdaControllerInfoData->HdaControllerInfo.GetName     = HdaControllerInfoGetName;
  HdaControllerInfoData->HdaControllerInfo.GetVendorId = HdaControllerInfoGetVendorId;

  //
  // Install protocols.
  //
  HdaControllerDev->HdaControllerInfoData = HdaControllerInfoData;
  return gBS->InstallMultipleProtocolInterfaces (&HdaControllerDev->ControllerHandle,
    &gEfiHdaControllerInfoProtocolGuid, &HdaControllerInfoData->HdaControllerInfo,
    &gEfiCallerIdGuid, HdaControllerDev, NULL);
}

VOID
EFIAPI
HdaControllerCleanup(
  IN HDA_CONTROLLER_DEV *HdaControllerDev) {
  DEBUG((DEBUG_VERBOSE, "HdaControllerCleanup(): start\n"));

  // If controller device is already free, we are done.
  if (HdaControllerDev == NULL)
    return;

  // Create variables.
  EFI_STATUS Status;
  EFI_PCI_IO_PROTOCOL *PciIo = HdaControllerDev->PciIo;
  UINT32 HdaGCtl;

  // Clean HDA Controller info protocol.
  if (HdaControllerDev->HdaControllerInfoData != NULL) {
    // Uninstall protocol.
    DEBUG((DEBUG_VERBOSE, "HdaControllerCleanup(): clean HDA Controller Info\n"));
    Status = gBS->UninstallProtocolInterface(HdaControllerDev->ControllerHandle,
      &gEfiHdaControllerInfoProtocolGuid, &HdaControllerDev->HdaControllerInfoData->HdaControllerInfo);
    ASSERT_EFI_ERROR(Status);

    // Free data.
    FreePool(HdaControllerDev->HdaControllerInfoData);
  }

  // Clean HDA I/O children.
  for (UINT8 i = 0; i < HDA_MAX_CODECS; i++) {
    // Clean Device Path protocol.
    if (HdaControllerDev->HdaIoChildren[i].DevicePath != NULL) {
      // Uninstall protocol.
      DEBUG((DEBUG_VERBOSE, "HdaControllerCleanup(): clean Device Path index %u\n", i));
      Status = gBS->UninstallProtocolInterface(HdaControllerDev->HdaIoChildren[i].Handle,
        &gEfiDevicePathProtocolGuid, HdaControllerDev->HdaIoChildren[i].DevicePath);
      ASSERT_EFI_ERROR(Status);

      // Free Device Path.
      FreePool(HdaControllerDev->HdaIoChildren[i].DevicePath);
    }

    // Clean HDA I/O protocol.
    if (HdaControllerDev->HdaIoChildren[i].PrivateData != NULL) {
      // Uninstall protocol.
      DEBUG((DEBUG_VERBOSE, "HdaControllerCleanup(): clean HDA I/O index %u\n", i));
      Status = gBS->UninstallProtocolInterface(HdaControllerDev->HdaIoChildren[i].Handle,
        &gEfiHdaIoProtocolGuid, &HdaControllerDev->HdaIoChildren[i].PrivateData->HdaIo);
      ASSERT_EFI_ERROR(Status);

      // Free private data.
      FreePool(HdaControllerDev->HdaIoChildren[i].PrivateData);
    }
  }

  //
  // Cleanup streams, CORB, and RIRB.
  //
  HdaControllerCleanupStreams (HdaControllerDev);
  HdaControllerCleanupRingBuffer (&HdaControllerDev->Corb);
  HdaControllerCleanupRingBuffer (&HdaControllerDev->Rirb);

  // Get value of CRST bit.
  Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &HdaGCtl);

  // Place controller into a reset state to stop it.
  if (!(EFI_ERROR(Status))) {
    HdaGCtl &= ~HDA_REG_GCTL_CRST;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &HdaGCtl);
  }

  // Free controller device.
  gBS->UninstallProtocolInterface(HdaControllerDev->ControllerHandle,
    &gEfiCallerIdGuid, HdaControllerDev);
  FreePool(HdaControllerDev);
}

EFI_STATUS
EFIAPI
HdaControllerDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS             Status;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  HDA_PCI_CLASSREG       HdaClassReg;

  //
  // Open PCI I/O protocol. If this fails, it's not a PCI device.
  //
  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiPciIoProtocolGuid,
    (VOID**) &PciIo,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read class code from PCI.
  //
  Status = PciIo->Pci.Read (
    PciIo,
    EfiPciIoWidthUint8,
    PCI_CLASSCODE_OFFSET,
    sizeof (HDA_PCI_CLASSREG),
    &HdaClassReg
    );

  //
  // Check class code, ignore everything but HDA controllers.
  //
  if (!EFI_ERROR (Status)
    && (HdaClassReg.Class != PCI_CLASS_MEDIA
    || HdaClassReg.SubClass != PCI_CLASS_MEDIA_MIXED_MODE)) {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

EFI_STATUS
EFIAPI
HdaControllerDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *HdaControllerDevicePath;
  HDA_CONTROLLER_DEV        *HdaControllerDev;
  VOID                      *VMwareHda;
  UINT32                    OpenMode;

  DEBUG ((DEBUG_INFO, "HDA: Starting for %p\n", ControllerHandle));

  OpenMode = EFI_OPEN_PROTOCOL_BY_DRIVER;

  do {
    //
    // Open PCI I/O protocol.
    //
    Status = gBS->OpenProtocol (
      ControllerHandle,
      &gEfiPciIoProtocolGuid,
      (VOID**) &PciIo,
      This->DriverBindingHandle,
      ControllerHandle,
      OpenMode
      );

    if (EFI_ERROR (Status)) {
      if (Status == EFI_ACCESS_DENIED && OpenMode == EFI_OPEN_PROTOCOL_BY_DRIVER) {
        Status = gBS->HandleProtocol (
          ControllerHandle,
          &gVMwareHdaProtocolGuid,
          &VMwareHda
          );
        if (!EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "HDA: Found VMware protocol, using GET mode\n"));
          OpenMode = EFI_OPEN_PROTOCOL_GET_PROTOCOL;
          continue;
        }
      }

      return Status;
    }
  } while (EFI_ERROR (Status));

  //
  // Open Device Path protocol.
  //
  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiDevicePathProtocolGuid,
    (VOID**) &HdaControllerDevicePath,
    This->DriverBindingHandle,
    ControllerHandle,
    OpenMode
    );
  if (EFI_ERROR (Status)) {
    goto CLOSE_PCIIO;
  }

  //
  // Allocate controller device.
  //
  HdaControllerDev = AllocateZeroPool (sizeof (HDA_CONTROLLER_DEV));
  if (HdaControllerDev == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CLOSE_PCIIO;
  }

  //
  // Fill controller device data.
  //
  HdaControllerDev->Signature        = HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE;
  HdaControllerDev->PciIo            = PciIo;
  HdaControllerDev->DevicePath       = HdaControllerDevicePath;
  HdaControllerDev->DriverBinding    = This;
  HdaControllerDev->ControllerHandle = ControllerHandle;
  HdaControllerDev->OpenMode         = OpenMode;
  InitializeSpinLock (&HdaControllerDev->SpinLock);

  //
  // Setup PCI hardware.
  //
  Status = HdaControllerInitPciHw (HdaControllerDev);
  if (EFI_ERROR (Status)) {
    goto FREE_CONTROLLER;
  }

  //
  // Get controller name.
  //
  HdaControllerGetName (HdaControllerDev);

  //
  // Reset controller.
  //
  Status = HdaControllerReset (HdaControllerDev);
  if (EFI_ERROR (Status)) {
    goto FREE_CONTROLLER;
  }

  //
  // Install info protocol.
  //
  Status = HdaControllerInstallProtocols (HdaControllerDev);
  if (EFI_ERROR (Status)) {
    goto FREE_CONTROLLER;
  }

  //
  // Initialize CORB and RIRB.
  //
  if (!HdaControllerInitRingBuffer (&HdaControllerDev->Corb, HdaControllerDev, HDA_RING_BUFFER_TYPE_CORB)
    || !HdaControllerInitRingBuffer (&HdaControllerDev->Rirb, HdaControllerDev, HDA_RING_BUFFER_TYPE_RIRB)) {
    goto FREE_CONTROLLER;
  }

  // needed for QEMU.
  // UINT16 dd = 0xFF;
  // PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RINTCNT, 1, &dd);

  //
  // Start CORB and RIRB.
  //
  if (!HdaControllerSetRingBufferState (&HdaControllerDev->Corb, TRUE)
    || !HdaControllerSetRingBufferState (&HdaControllerDev->Rirb, TRUE)) {
    goto FREE_CONTROLLER;
  }

  DEBUG ((DEBUG_INFO, "Gotten here\n"));

  //
  // Init streams.
  //
  Status = HdaControllerInitStreams (HdaControllerDev);
  if (EFI_ERROR (Status)) {
    goto FREE_CONTROLLER;
  }

  //
  // Scan for codecs.
  //
  Status = HdaControllerScanCodecs (HdaControllerDev);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "HDA: Starting done for %p\n", ControllerHandle));
  return EFI_SUCCESS;

FREE_CONTROLLER:

  //
  // Restore PCI attributes if needed.
  //
  if (HdaControllerDev->OriginalPciAttributesSaved) {
    PciIo->Attributes (
      PciIo,
      EfiPciIoAttributeOperationSet,
      HdaControllerDev->OriginalPciAttributes,
      NULL
      );
  }

  //
  // Free controller device.
  //
  HdaControllerCleanup (HdaControllerDev);

CLOSE_PCIIO:
  
  //
  // Close protocols.
  //
  if (OpenMode == EFI_OPEN_PROTOCOL_BY_DRIVER) {
    gBS->CloseProtocol (
      ControllerHandle,
      &gEfiDevicePathProtocolGuid,
      This->DriverBindingHandle,
      ControllerHandle
      );
    gBS->CloseProtocol (
      ControllerHandle,
      &gEfiPciIoProtocolGuid,
      This->DriverBindingHandle,
      ControllerHandle
      );
  }

  return Status;
}

EFI_STATUS
EFIAPI
HdaControllerDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS          Status;
  HDA_CONTROLLER_DEV  *HdaControllerDev;
  UINT32              OpenMode;

  DEBUG ((DEBUG_INFO, "HDA: Stopping for %p\n", ControllerHandle));

  OpenMode = EFI_OPEN_PROTOCOL_BY_DRIVER;

  //
  // Get codec device.
  //
  Status = gBS->OpenProtocol (
    ControllerHandle,
    &gEfiCallerIdGuid,
    (VOID**) &HdaControllerDev,
    This->DriverBindingHandle,
    ControllerHandle,
    EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

  if (!EFI_ERROR (Status)) {
    //
    // Gather open mode.
    //
    OpenMode = HdaControllerDev->OpenMode;

    //
    // Ensure controller device is valid.
    //
    if (HdaControllerDev->Signature != HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Restore PCI attributes if needed.
    //
    if (HdaControllerDev->OriginalPciAttributesSaved) {
      HdaControllerDev->PciIo->Attributes (
        HdaControllerDev->PciIo,
        EfiPciIoAttributeOperationSet,
        HdaControllerDev->OriginalPciAttributes,
        NULL
        );
    }

    //
    // Cleanup controller.
    //
    HdaControllerCleanup (HdaControllerDev);
  }

  //
  // Close protocols.
  //
  if (OpenMode == EFI_OPEN_PROTOCOL_BY_DRIVER) {
    gBS->CloseProtocol (
      ControllerHandle,
      &gEfiDevicePathProtocolGuid,
      This->DriverBindingHandle,
      ControllerHandle
      );
    gBS->CloseProtocol (
      ControllerHandle,
      &gEfiPciIoProtocolGuid,
      This->DriverBindingHandle,
      ControllerHandle
      );
  }

  DEBUG ((DEBUG_INFO, "HDA: Stopping done for %p\n", ControllerHandle));

  return EFI_SUCCESS;
}
