/** @file

  BiosVideo driver produce EFI_GRAPHIC_OUTPUT_PROTOCOL via LegacyBios Video rom.

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BiosVideo.h"

//
// EFI Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gBiosVideoDriverBinding = {
  BiosVideoDriverBindingSupported,
  BiosVideoDriverBindingStart,
  BiosVideoDriverBindingStop,
  0x3,
  NULL,
  NULL
};

//
// Global lookup tables for VGA graphics modes
//
UINT8 mVgaLeftMaskTable[]   = { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };

UINT8 mVgaRightMaskTable[]  = { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

UINT8 mVgaBitMaskTable[]    = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

EFI_LEGACY_8259_PROTOCOL   *mLegacy8259 = NULL;
THUNK_CONTEXT              mThunkContext;

EFI_GRAPHICS_OUTPUT_BLT_PIXEL  mVgaColorToGraphicsOutputColor[] = {
  //
  // {B, G, R, reserved}
  //
  {0x00, 0x00, 0x00, 0x00}, // BLACK
  {0x98, 0x00, 0x00, 0x00}, // LIGHTBLUE
  {0x00, 0x98, 0x00, 0x00}, // LIGHGREEN
  {0x98, 0x98, 0x00, 0x00}, // LIGHCYAN
  {0x00, 0x00, 0x98, 0x00}, // LIGHRED
  {0x98, 0x00, 0x98, 0x00}, // MAGENTA
  {0x00, 0x98, 0x98, 0x00}, // BROWN
  {0x98, 0x98, 0x98, 0x00}, // LIGHTGRAY
  {0x10, 0x10, 0x10, 0x00},
  {0xff, 0x10, 0x10, 0x00}, // BLUE
  {0x10, 0xff, 0x10, 0x00}, // LIME
  {0xff, 0xff, 0x10, 0x00}, // CYAN
  {0x10, 0x10, 0xff, 0x00}, // RED
  {0xf0, 0x10, 0xff, 0x00}, // FUCHSIA
  {0x10, 0xff, 0xff, 0x00}, // YELLOW
  {0xff, 0xff, 0xff, 0x00}  // WHITE
};

/**
  Driver Entry Point.

  @param ImageHandle      Handle of driver image.
  @param SystemTable      Pointer to system table.

  @return EFI_STATUS
**/
EFI_STATUS
EFIAPI
BiosVideoDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
            ImageHandle,
            SystemTable,
            &gBiosVideoDriverBinding,
            ImageHandle,
            &gBiosVideoComponentName,
            &gBiosVideoComponentName2
            );

  return Status;
}

/**
  Test to see if Bios Video could be supported on the Controller.

  @param This                  Pointer to driver binding protocol
  @param Controller            Controller handle to connect
  @param RemainingDevicePath   A pointer to the remaining portion of a device path

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
BiosVideoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_8259_PROTOCOL  *LegacyBios;
  EFI_PCI_IO_PROTOCOL       *PciIo;

  //
  // See if the Legacy 8259 Protocol is available
  //
  Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &LegacyBios);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!BiosVideoIsVga (PciIo)) {
    Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Install Graphics Output Protocol onto VGA device handles

  @param This                   Pointer to driver binding protocol
  @param Controller             Controller handle to connect
  @param RemainingDevicePath    A pointer to the remaining portion of a device path

  @return EFI_STATUS

**/
EFI_STATUS
EFIAPI
BiosVideoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                      Status;
  EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;
  EFI_PCI_IO_PROTOCOL             *PciIo;

  PciIo = NULL;
  //
  // Prepare for status code
  //
  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Establish legacy environment for thunk call for all children handle.
  //
  if (mLegacy8259 == NULL) {
    Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **) &mLegacy8259);
    if (EFI_ERROR (Status)) {
        goto Done;
    }

    InitializeBiosIntCaller(&mThunkContext);
    InitializeInterruptRedirection(mLegacy8259);
  }

  //
  // Create child handle and install GraphicsOutputProtocol on it
  //
  Status = BiosVideoChildHandleInstall (
             This,
             Controller,
             PciIo,
             mLegacy8259,
             &mThunkContext,
             ParentDevicePath,
             RemainingDevicePath
             );

Done:
  if (EFI_ERROR (Status)) {
    if (PciIo != NULL) {
      //
      // Release PCI I/O Protocols on the controller handle.
      //
      gBS->CloseProtocol (
             Controller,
             &gEfiPciIoProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }
  }

  return Status;
}

/**
  Stop this driver on Controller

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed Controller.
  @retval other             This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
BiosVideoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                   Status;
  BOOLEAN                      AllChildrenStopped;
  UINTN                        Index;

  if (NumberOfChildren == 0) {
    //
    // Close PCI I/O protocol on the controller handle
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;
  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = BiosVideoChildHandleUninstall (This, Controller, ChildHandleBuffer[Index]);

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Install child handle for a detect BiosVideo device and install related protocol
  into this handle, such as EFI_GRAPHIC_OUTPUT_PROTOCOL.

  @param This                Instance pointer of EFI_DRIVER_BINDING_PROTOCOL
  @param ParentHandle        Parent's controller handle
  @param ParentPciIo         Parent's EFI_PCI_IO_PROTOCOL instance pointer
  @param ParentLegacy8259    Parent's EFI_LEGACY_8259_PROTOCOL instance pointer
  @param ParentDevicePath    Parent's BIOS Video controller device path
  @param RemainingDevicePath Remaining device path node instance for children.

  @return whether success to create children handle for a VGA device and install
          related protocol into new children handle.

**/
EFI_STATUS
BiosVideoChildHandleInstall (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ParentHandle,
  IN  EFI_PCI_IO_PROTOCOL            *ParentPciIo,
  IN  EFI_LEGACY_8259_PROTOCOL       *ParentLegacy8259,
  IN  THUNK_CONTEXT                  *ParentThunkContext,
  IN  EFI_DEVICE_PATH_PROTOCOL       *ParentDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS               Status;
  BIOS_VIDEO_DEV           *BiosVideoPrivate;
  ACPI_ADR_DEVICE_PATH     AcpiDeviceNode;

  //
  // Allocate the private device structure for video device
  //
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (BIOS_VIDEO_DEV),
                  (VOID**) &BiosVideoPrivate
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  ZeroMem (BiosVideoPrivate, sizeof (BIOS_VIDEO_DEV));

  if (!BiosVideoIsVga (ParentPciIo)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  BiosVideoPrivate->VgaCompatible = TRUE;

  //
  // Initialize the child private structure
  //
  BiosVideoPrivate->Signature = BIOS_VIDEO_DEV_SIGNATURE;
  BiosVideoPrivate->Handle    = NULL;

  //
  // Fill in Graphics Output specific mode structures
  //
  BiosVideoPrivate->HardwareNeedsStarting = TRUE;
  BiosVideoPrivate->ModeData              = NULL;
  BiosVideoPrivate->LineBuffer            = NULL;
  BiosVideoPrivate->VgaFrameBuffer        = NULL;
  BiosVideoPrivate->VbeFrameBuffer        = NULL;

  //
  // Fill in the VGA Mini Port Protocol fields
  //
  BiosVideoPrivate->VgaMiniPort.SetMode                   = BiosVideoVgaMiniPortSetMode;
  BiosVideoPrivate->VgaMiniPort.VgaMemoryOffset           = 0xb8000;
  BiosVideoPrivate->VgaMiniPort.CrtcAddressRegisterOffset = 0x3d4;
  BiosVideoPrivate->VgaMiniPort.CrtcDataRegisterOffset    = 0x3d5;
  BiosVideoPrivate->VgaMiniPort.VgaMemoryBar              = EFI_PCI_IO_PASS_THROUGH_BAR;
  BiosVideoPrivate->VgaMiniPort.CrtcAddressRegisterBar    = EFI_PCI_IO_PASS_THROUGH_BAR;
  BiosVideoPrivate->VgaMiniPort.CrtcDataRegisterBar       = EFI_PCI_IO_PASS_THROUGH_BAR;

  //
  // Assume that Graphics Output Protocol will be produced until proven otherwise
  //
  BiosVideoPrivate->ProduceGraphicsOutput = TRUE;

  //
  // Child handle need to consume the Legacy Bios protocol
  //
  BiosVideoPrivate->Legacy8259   = ParentLegacy8259;
  BiosVideoPrivate->ThunkContext = ParentThunkContext;

  //
  // When check for VBE, PCI I/O protocol is needed, so use parent's protocol interface temporally
  //
  BiosVideoPrivate->PciIo = ParentPciIo;

  //
  // Check for VESA BIOS Extensions for modes that are compatible with Graphics Output
  //
  Status = BiosVideoCheckForVbe (BiosVideoPrivate);
  if (EFI_ERROR (Status)) {
    //
    // The VESA BIOS Extensions are not compatible with Graphics Output, so check for support
    // for the standard 640x480 16 color VGA mode
    //
    if (BiosVideoPrivate->VgaCompatible) {
      Status = BiosVideoCheckForVga (BiosVideoPrivate);
    }

    if (EFI_ERROR (Status)) {
      //
      // Neither VBE nor the standard 640x480 16 color VGA mode are supported, so do
      // not produce the Graphics Output protocol.  Instead, produce the VGA MiniPort Protocol.
      //
      BiosVideoPrivate->ProduceGraphicsOutput = FALSE;

      //
      // INT services are available, so on the 80x25 and 80x50 text mode are supported
      //
      BiosVideoPrivate->VgaMiniPort.MaxMode = 2;
    }
  }

  if (BiosVideoPrivate->ProduceGraphicsOutput) {
    if (RemainingDevicePath == NULL) {
      ZeroMem (&AcpiDeviceNode, sizeof (ACPI_ADR_DEVICE_PATH));
      AcpiDeviceNode.Header.Type    = ACPI_DEVICE_PATH;
      AcpiDeviceNode.Header.SubType = ACPI_ADR_DP;
      AcpiDeviceNode.ADR            = ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0);
      SetDevicePathNodeLength (&AcpiDeviceNode.Header, sizeof (ACPI_ADR_DEVICE_PATH));

      BiosVideoPrivate->DevicePath = AppendDevicePathNode (
                                       ParentDevicePath,
                                       (EFI_DEVICE_PATH_PROTOCOL *) &AcpiDeviceNode
                                       );
    } else {
      BiosVideoPrivate->DevicePath = AppendDevicePathNode (ParentDevicePath, RemainingDevicePath);
    }

    //
    // Creat child handle and install Graphics Output Protocol,EDID Discovered/Active Protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &BiosVideoPrivate->Handle,
                    &gEfiDevicePathProtocolGuid,
                    BiosVideoPrivate->DevicePath,
                    &gEfiGraphicsOutputProtocolGuid,
                    &BiosVideoPrivate->GraphicsOutput,
                    &gEfiEdidDiscoveredProtocolGuid,
                    &BiosVideoPrivate->EdidDiscovered,
                    &gEfiEdidActiveProtocolGuid,
                    &BiosVideoPrivate->EdidActive,
                    NULL
                    );

    if (!EFI_ERROR (Status)) {
      //
      // Open the Parent Handle for the child
      //
      Status = gBS->OpenProtocol (
                      ParentHandle,
                      &gEfiPciIoProtocolGuid,
                      (VOID **) &BiosVideoPrivate->PciIo,
                      This->DriverBindingHandle,
                      BiosVideoPrivate->Handle,
                      EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                      );
      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }
  } else {
    //
    // Install VGA Mini Port Protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &BiosVideoPrivate->Handle,
                    &gEfiVgaMiniPortProtocolGuid,
                    &BiosVideoPrivate->VgaMiniPort,
                    NULL
                    );
  }

Done:
  if (EFI_ERROR (Status)) {
    //
    // Free private data structure
    //
    BiosVideoDeviceReleaseResource (BiosVideoPrivate);
  }

  return Status;
}

/**
  Deregister an video child handle and free resources

  @param This            Protocol instance pointer.
  @param Controller      Video controller handle
  @param Handle          Video child handle

  @return EFI_STATUS

**/
EFI_STATUS
BiosVideoChildHandleUninstall (
  EFI_DRIVER_BINDING_PROTOCOL    *This,
  EFI_HANDLE                     Controller,
  EFI_HANDLE                     Handle
  )
{
  EFI_STATUS                   Status;
  IA32_REGISTER_SET        Regs;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
  EFI_VGA_MINI_PORT_PROTOCOL   *VgaMiniPort;
  BIOS_VIDEO_DEV               *BiosVideoPrivate;
  EFI_PCI_IO_PROTOCOL          *PciIo;

  BiosVideoPrivate = NULL;
  GraphicsOutput   = NULL;
  PciIo            = NULL;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (GraphicsOutput);
  }

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiVgaMiniPortProtocolGuid,
                  (VOID **) &VgaMiniPort,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_VGA_MINI_PORT_THIS (VgaMiniPort);
  }

  if (BiosVideoPrivate == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Close PCI I/O protocol that opened by child handle
  //
  Status = gBS->CloseProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  This->DriverBindingHandle,
                  Handle
                  );

  //
  // Uninstall protocols on child handle
  //
  if (BiosVideoPrivate->ProduceGraphicsOutput) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    BiosVideoPrivate->Handle,
                    &gEfiDevicePathProtocolGuid,
                    BiosVideoPrivate->DevicePath,
                    &gEfiGraphicsOutputProtocolGuid,
                    &BiosVideoPrivate->GraphicsOutput,
                    &gEfiEdidDiscoveredProtocolGuid,
                    &BiosVideoPrivate->EdidDiscovered,
                    &gEfiEdidActiveProtocolGuid,
                    &BiosVideoPrivate->EdidActive,
                    NULL
                    );
  } else {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    BiosVideoPrivate->Handle,
                    &gEfiVgaMiniPortProtocolGuid,
                    &BiosVideoPrivate->VgaMiniPort,
                    NULL
                    );
  }
  if (EFI_ERROR (Status)) {
    gBS->OpenProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           (VOID **) &PciIo,
           This->DriverBindingHandle,
           Handle,
           EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
           );
    return Status;
  }

  ZeroMem (&Regs, sizeof (Regs));

  //
  // Set the 80x25 Text VGA Mode
  //
  Regs.H.AH = 0x00;
  Regs.H.AL = 0x03;
  LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);

  Regs.H.AH = 0x11;
  Regs.H.AL = 0x14;
  Regs.H.BL = 0;
  LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);

  //
  // Do not disable IO/memory decode since that would prevent legacy ROM from working
  //

  //
  // Release all allocated resources
  //
  BiosVideoDeviceReleaseResource (BiosVideoPrivate);

  return EFI_SUCCESS;
}

/**
  Collect the resource from destroyed bios video device.

  @param BiosVideoPrivate   Video child device private data structure

**/
VOID
BiosVideoDeviceReleaseResource (
  BIOS_VIDEO_DEV  *BiosVideoPrivate
  )
{
  if (BiosVideoPrivate == NULL) {
    return ;
  }

  //
  // Release all the resourses occupied by the BIOS_VIDEO_DEV
  //

  //
  // Free VGA Frame Buffer
  //
  if (BiosVideoPrivate->VgaFrameBuffer != NULL) {
    gBS->FreePool (BiosVideoPrivate->VgaFrameBuffer);
  }
  //
  // Free VBE Frame Buffer
  //
  if (BiosVideoPrivate->VbeFrameBuffer != NULL) {
    gBS->FreePool (BiosVideoPrivate->VbeFrameBuffer);
  }
  //
  // Free line buffer
  //
  if (BiosVideoPrivate->LineBuffer != NULL) {
    gBS->FreePool (BiosVideoPrivate->LineBuffer);
  }
  //
  // Free mode data
  //
  if (BiosVideoPrivate->ModeData != NULL) {
    gBS->FreePool (BiosVideoPrivate->ModeData);
  }
  //
  // Free memory allocated below 1MB
  //
  if (BiosVideoPrivate->PagesBelow1MB != 0) {
    gBS->FreePages (BiosVideoPrivate->PagesBelow1MB, BiosVideoPrivate->NumberOfPagesBelow1MB);
  }

  if (BiosVideoPrivate->VbeSaveRestorePages != 0) {
    gBS->FreePages (BiosVideoPrivate->VbeSaveRestoreBuffer, BiosVideoPrivate->VbeSaveRestorePages);
  }
  //
  // Free graphics output protocol occupied resource
  //
  if (BiosVideoPrivate->GraphicsOutput.Mode != NULL) {
    if (BiosVideoPrivate->GraphicsOutput.Mode->Info != NULL) {
        gBS->FreePool (BiosVideoPrivate->GraphicsOutput.Mode->Info);
    }
    gBS->FreePool (BiosVideoPrivate->GraphicsOutput.Mode);
  }
  //
  // Free EDID discovered protocol occupied resource
  //
  if (BiosVideoPrivate->EdidDiscovered.Edid != NULL) {
    gBS->FreePool (BiosVideoPrivate->EdidDiscovered.Edid);
  }
  //
  // Free EDID active protocol occupied resource
  //
  if (BiosVideoPrivate->EdidActive.Edid != NULL) {
    gBS->FreePool (BiosVideoPrivate->EdidActive.Edid);
  }

  if (BiosVideoPrivate->DevicePath!= NULL) {
    gBS->FreePool (BiosVideoPrivate->DevicePath);
  }

  gBS->FreePool (BiosVideoPrivate);

  return ;
}

#define PCI_DEVICE_ENABLED  (EFI_PCI_COMMAND_IO_SPACE | EFI_PCI_COMMAND_MEMORY_SPACE)


/**
  Judge whether this device is VGA device.

  @param PciIo      Parent PciIo protocol instance pointer

  @retval TRUE  Is vga device
  @retval FALSE Is no vga device
**/
BOOLEAN
BiosVideoIsVga (
  IN  EFI_PCI_IO_PROTOCOL       *PciIo
  )
{
  EFI_STATUS    Status;
  BOOLEAN       VgaCompatible;
  PCI_TYPE00    Pci;

  VgaCompatible = FALSE;

  //
  // Read the PCI Configuration Header
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    return VgaCompatible;
  }

  //
  // See if this is a VGA compatible controller or not
  //
  if ((Pci.Hdr.Command & PCI_DEVICE_ENABLED) == PCI_DEVICE_ENABLED) {
    if (Pci.Hdr.ClassCode[2] == PCI_CLASS_OLD && Pci.Hdr.ClassCode[1] == PCI_CLASS_OLD_VGA) {
      //
      // Base Class 0x00 Sub-Class 0x01 - Backward compatible VGA device
      //
      VgaCompatible = TRUE;
    }

    if (Pci.Hdr.ClassCode[2] == PCI_CLASS_DISPLAY && Pci.Hdr.ClassCode[1] == PCI_CLASS_DISPLAY_VGA && Pci.Hdr.ClassCode[0] == 0x00) {
      //
      // Base Class 3 Sub-Class 0 Programming interface 0 - VGA compatible Display controller
      //
      VgaCompatible = TRUE;
    }
  }

  return VgaCompatible;
}


/**
  Check for VBE device

  @param BiosVideoPrivate - Pointer to BIOS_VIDEO_DEV structure

  @retval EFI_SUCCESS VBE device found

**/
EFI_STATUS
EFIAPI
BiosVideoCheckForVbe (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate
  )
{
  EFI_STATUS                             Status;
  IA32_REGISTER_SET                      Regs;
  UINT16                                 *ModeNumberPtr;
  BIOS_VIDEO_MODE_DATA                   *ModeBuffer;
  BIOS_VIDEO_MODE_DATA                   *CurrentModeData;
  UINTN                                  PreferMode;
  UINTN                                  ModeNumber;
  VESA_BIOS_EXTENSIONS_VALID_EDID_TIMING ValidEdidTiming;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE      *GraphicsOutputMode;
  UINT32                                 HighestHorizontalResolution;
  UINT32                                 HighestVerticalResolution;
  UINTN                                  HighestResolutionMode;

  //
  // Allocate buffer under 1MB for VBE data structures
  //
  BiosVideoPrivate->NumberOfPagesBelow1MB = EFI_SIZE_TO_PAGES (
                                              sizeof (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK) +
                                              sizeof (VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK) +
                                              sizeof (VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK) +
                                              sizeof (VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK)
                                              );

  BiosVideoPrivate->PagesBelow1MB = LEGACY_REGION_BASE - 1;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiBootServicesData,
                  BiosVideoPrivate->NumberOfPagesBelow1MB,
                  &BiosVideoPrivate->PagesBelow1MB
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (&ValidEdidTiming, sizeof (VESA_BIOS_EXTENSIONS_VALID_EDID_TIMING));

  //
  // Fill in the Graphics Output Protocol
  //
  BiosVideoPrivate->GraphicsOutput.QueryMode = BiosVideoGraphicsOutputQueryMode;
  BiosVideoPrivate->GraphicsOutput.SetMode = BiosVideoGraphicsOutputSetMode;
  BiosVideoPrivate->GraphicsOutput.Blt     = BiosVideoGraphicsOutputVbeBlt;
  BiosVideoPrivate->GraphicsOutput.Mode = NULL;

  //
  // Fill in the VBE related data structures
  //
  BiosVideoPrivate->VbeInformationBlock = (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK *) (UINTN) (BiosVideoPrivate->PagesBelow1MB);
  BiosVideoPrivate->VbeModeInformationBlock = (VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK *) (BiosVideoPrivate->VbeInformationBlock + 1);
  BiosVideoPrivate->VbeEdidDataBlock = (VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK *) (BiosVideoPrivate->VbeModeInformationBlock + 1);
  BiosVideoPrivate->VbeCrtcInformationBlock = (VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK *) (BiosVideoPrivate->VbeEdidDataBlock + 1);
  BiosVideoPrivate->VbeSaveRestorePages   = 0;
  BiosVideoPrivate->VbeSaveRestoreBuffer  = 0;

  HighestHorizontalResolution = 0;
  HighestVerticalResolution   = 0;
  HighestResolutionMode       = 0;

  //
  // Test to see if the Video Adapter is compliant with VBE 3.0
  //
  // INT 10 - VESA SuperVGA BIOS (VBE) - GET SuperVGA INFORMATION
  //
  //  AX = 4F00h
  //  ES:DI -> buffer for SuperVGA information (see #00077)
  // Return: AL = 4Fh if function supported
  //  AH = status
  //      00h successful
  //    ES:DI buffer filled
  //      01h failed
  //      ---VBE v2.0---
  //      02h function not supported by current hardware configuration
  //      03h function invalid in current video mode
  // Desc:  determine whether VESA BIOS extensions are present and the capabilities
  //    supported by the display adapter
  //
  ZeroMem (&Regs, sizeof (Regs));
  Regs.X.AX = VESA_BIOS_EXTENSIONS_RETURN_CONTROLLER_INFORMATION;
  ZeroMem (BiosVideoPrivate->VbeInformationBlock, sizeof (VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK));
  BiosVideoPrivate->VbeInformationBlock->VESASignature  = VESA_BIOS_EXTENSIONS_VBE2_SIGNATURE;
  Regs.E.ES = EFI_SEGMENT ((UINTN) BiosVideoPrivate->VbeInformationBlock);
  Regs.X.DI = EFI_OFFSET ((UINTN) BiosVideoPrivate->VbeInformationBlock);

  LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);

  Status = EFI_DEVICE_ERROR;

  //
  // See if the VESA call succeeded
  //
  if (Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
    return Status;
  }
  //
  // Check for 'VESA' signature
  //
  if (BiosVideoPrivate->VbeInformationBlock->VESASignature != VESA_BIOS_EXTENSIONS_VESA_SIGNATURE) {
    return Status;
  }

  BiosVideoPrivate->EdidDiscovered.SizeOfEdid = 0;
  BiosVideoPrivate->EdidDiscovered.Edid = NULL;

  BiosVideoPrivate->EdidActive.SizeOfEdid = 0;
  BiosVideoPrivate->EdidActive.Edid = NULL;

  //
  // Walk through the mode list to see if there is at least one mode the is compatible with the EDID mode
  //
  ModeNumberPtr = (UINT16 *)
    (
      (((UINTN) BiosVideoPrivate->VbeInformationBlock->VideoModePtr & 0xffff0000) >> 12) |
        ((UINTN) BiosVideoPrivate->VbeInformationBlock->VideoModePtr & 0x0000ffff)
    );

  PreferMode = 0;
  ModeNumber = 0;

  for (; *ModeNumberPtr != VESA_BIOS_EXTENSIONS_END_OF_MODE_LIST; ModeNumberPtr++) {
    //
    // Make sure this is a mode number defined by the VESA VBE specification.  If it isn'tm then skip this mode number.
    //
    if ((*ModeNumberPtr & VESA_BIOS_EXTENSIONS_MODE_NUMBER_VESA) == 0) {
      continue;
    }
    //
    // Get the information about the mode
    //
    // INT 10 - VESA SuperVGA BIOS - GET SuperVGA MODE INFORMATION
    //
    //   AX = 4F01h
    //   CX = SuperVGA video mode (see #04082 for bitfields)
    //   ES:DI -> 256-byte buffer for mode information (see #00079)
    // Return: AL = 4Fh if function supported
    //   AH = status
    //      00h successful
    //    ES:DI buffer filled
    //      01h failed
    // Desc:  determine the attributes of the specified video mode
    //
    ZeroMem (&Regs, sizeof (Regs));
    Regs.X.AX = VESA_BIOS_EXTENSIONS_RETURN_MODE_INFORMATION;
    Regs.X.CX = *ModeNumberPtr;
    ZeroMem (BiosVideoPrivate->VbeModeInformationBlock, sizeof (VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK));
    Regs.E.ES = EFI_SEGMENT ((UINTN) BiosVideoPrivate->VbeModeInformationBlock);
    Regs.X.DI = EFI_OFFSET ((UINTN) BiosVideoPrivate->VbeModeInformationBlock);

    LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);

    //
    // See if the call succeeded.  If it didn't, then try the next mode.
    //
    if (Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
      continue;
    }
    //
    // See if the mode supports color.  If it doesn't then try the next mode.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_COLOR) == 0) {
      continue;
    }
    //
    // See if the mode supports graphics.  If it doesn't then try the next mode.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_GRAPHICS) == 0) {
      continue;
    }
    //
    // See if the mode supports a linear frame buffer.  If it doesn't then try the next mode.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_LINEAR_FRAME_BUFFER) == 0) {
      continue;
    }
    //
    // See if the mode supports 32 bit color.  If it doesn't then try the next mode.
    // 32 bit mode can be implemented by 24 Bits Per Pixels. Also make sure the
    // number of bits per pixel is a multiple of 8 or more than 32 bits per pixel
    //
    if (BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel < 24) {
      continue;
    }

    if (BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel > 32) {
      continue;
    }

    if ((BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel % 8) != 0) {
      continue;
    }
    //
    // See if the physical base pointer for the linear mode is valid.  If it isn't then try the next mode.
    //
    if (BiosVideoPrivate->VbeModeInformationBlock->PhysBasePtr == 0) {
      continue;
    }
    //
    // Skip modes not supported by the hardware.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->ModeAttributes & VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_HARDWARE) == 0) {
      continue;
    }
    //
    // Skip resolutions lower then 640x480.
    //
    if (BiosVideoPrivate->VbeModeInformationBlock->XResolution < 640 ||
      BiosVideoPrivate->VbeModeInformationBlock->YResolution < 480) {
      continue;
    }

    //
    // Record the highest resolution mode to set later.
    //
    if ((BiosVideoPrivate->VbeModeInformationBlock->XResolution >= HighestHorizontalResolution)
      && (BiosVideoPrivate->VbeModeInformationBlock->YResolution >= HighestVerticalResolution)) {
      HighestHorizontalResolution = BiosVideoPrivate->VbeModeInformationBlock->XResolution;
      HighestVerticalResolution = BiosVideoPrivate->VbeModeInformationBlock->YResolution;
      HighestResolutionMode = (UINT16) ModeNumber;
      PreferMode = HighestResolutionMode;
    }

    //
    // Add mode to the list of available modes
    //
    ModeNumber ++;
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    ModeNumber * sizeof (BIOS_VIDEO_MODE_DATA),
                    (VOID **) &ModeBuffer
                    );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    if (ModeNumber > 1) {
      CopyMem (
            ModeBuffer,
            BiosVideoPrivate->ModeData,
            (ModeNumber - 1) * sizeof (BIOS_VIDEO_MODE_DATA)
            );
    }

    if (BiosVideoPrivate->ModeData != NULL) {
      gBS->FreePool (BiosVideoPrivate->ModeData);
    }

    CurrentModeData = &ModeBuffer[ModeNumber - 1];
    CurrentModeData->VbeModeNumber = *ModeNumberPtr;
    if (BiosVideoPrivate->VbeInformationBlock->VESAVersion >= VESA_BIOS_EXTENSIONS_VERSION_3_0) {
      CurrentModeData->BytesPerScanLine = BiosVideoPrivate->VbeModeInformationBlock->LinBytesPerScanLine;
      CurrentModeData->Red.Position = BiosVideoPrivate->VbeModeInformationBlock->LinRedFieldPosition;
      CurrentModeData->Red.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->LinRedMaskSize) - 1);
      CurrentModeData->Blue.Position = BiosVideoPrivate->VbeModeInformationBlock->LinBlueFieldPosition;
      CurrentModeData->Blue.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->LinBlueMaskSize) - 1);
      CurrentModeData->Green.Position = BiosVideoPrivate->VbeModeInformationBlock->LinGreenFieldPosition;
      CurrentModeData->Green.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->LinGreenMaskSize) - 1);
      CurrentModeData->Reserved.Position = BiosVideoPrivate->VbeModeInformationBlock->LinRsvdFieldPosition;
      CurrentModeData->Reserved.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->LinRsvdMaskSize) - 1);
    } else {
      CurrentModeData->BytesPerScanLine = BiosVideoPrivate->VbeModeInformationBlock->BytesPerScanLine;
      CurrentModeData->Red.Position = BiosVideoPrivate->VbeModeInformationBlock->RedFieldPosition;
      CurrentModeData->Red.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->RedMaskSize) - 1);
      CurrentModeData->Blue.Position = BiosVideoPrivate->VbeModeInformationBlock->BlueFieldPosition;
      CurrentModeData->Blue.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->BlueMaskSize) - 1);
      CurrentModeData->Green.Position = BiosVideoPrivate->VbeModeInformationBlock->GreenFieldPosition;
      CurrentModeData->Green.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->GreenMaskSize) - 1);
      CurrentModeData->Reserved.Position = BiosVideoPrivate->VbeModeInformationBlock->RsvdFieldPosition;
      CurrentModeData->Reserved.Mask = (UINT8) ((1 << BiosVideoPrivate->VbeModeInformationBlock->RsvdMaskSize) - 1);
    }
    CurrentModeData->PixelFormat = PixelBitMask;
    if ((BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel == 32) &&
        (CurrentModeData->Red.Mask == 0xff) && (CurrentModeData->Green.Mask == 0xff) && (CurrentModeData->Blue.Mask == 0xff)) {
      if ((CurrentModeData->Red.Position == 0) && (CurrentModeData->Green.Position == 8) && (CurrentModeData->Blue.Position == 16)) {
        CurrentModeData->PixelFormat = PixelRedGreenBlueReserved8BitPerColor;
      } else if ((CurrentModeData->Blue.Position == 0) && (CurrentModeData->Green.Position == 8) && (CurrentModeData->Red.Position == 16)) {
        CurrentModeData->PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
      }
    }
    CurrentModeData->PixelBitMask.RedMask = ((UINT32) CurrentModeData->Red.Mask) << CurrentModeData->Red.Position;
    CurrentModeData->PixelBitMask.GreenMask = ((UINT32) CurrentModeData->Green.Mask) << CurrentModeData->Green.Position;
    CurrentModeData->PixelBitMask.BlueMask = ((UINT32) CurrentModeData->Blue.Mask) << CurrentModeData->Blue.Position;
    CurrentModeData->PixelBitMask.ReservedMask = ((UINT32) CurrentModeData->Reserved.Mask) << CurrentModeData->Reserved.Position;

    CurrentModeData->LinearFrameBuffer = (VOID *) (UINTN)BiosVideoPrivate->VbeModeInformationBlock->PhysBasePtr;
    CurrentModeData->FrameBufferSize = BiosVideoPrivate->VbeInformationBlock->TotalMemory * 64 * 1024;
    CurrentModeData->HorizontalResolution = BiosVideoPrivate->VbeModeInformationBlock->XResolution;
    CurrentModeData->VerticalResolution = BiosVideoPrivate->VbeModeInformationBlock->YResolution;

    CurrentModeData->BitsPerPixel  = BiosVideoPrivate->VbeModeInformationBlock->BitsPerPixel;

    BiosVideoPrivate->ModeData = ModeBuffer;
  }
  //
  // Check to see if we found any modes that are compatible with GRAPHICS OUTPUT
  //
  if (ModeNumber == 0) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // Allocate buffer for Graphics Output Protocol mode information
  //
  Status = gBS->AllocatePool (
                EfiBootServicesData,
                sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE),
                (VOID **) &BiosVideoPrivate->GraphicsOutput.Mode
                );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  GraphicsOutputMode = BiosVideoPrivate->GraphicsOutput.Mode;
  Status = gBS->AllocatePool (
                EfiBootServicesData,
                sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
                (VOID **) &GraphicsOutputMode->Info
                );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  GraphicsOutputMode->MaxMode = (UINT32) ModeNumber;
  //
  // Current mode is unknow till now, set it to an invalid mode.
  //
  GraphicsOutputMode->Mode = GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER;

  //
  // Find the best mode to initialize
  //
  Status = BiosVideoGraphicsOutputSetMode (&BiosVideoPrivate->GraphicsOutput, (UINT32) PreferMode);
  if (EFI_ERROR (Status)) {
    for (PreferMode = 0; PreferMode < ModeNumber; PreferMode ++) {
      Status = BiosVideoGraphicsOutputSetMode (
                &BiosVideoPrivate->GraphicsOutput,
                (UINT32) PreferMode
                );
      if (!EFI_ERROR (Status)) {
        break;
      }
    }
  }

Done:
  //
  // If there was an error, then free the mode structure
  //
  if (EFI_ERROR (Status)) {
    if (BiosVideoPrivate->ModeData != NULL) {
      gBS->FreePool (BiosVideoPrivate->ModeData);
    }
    if (BiosVideoPrivate->GraphicsOutput.Mode != NULL) {
      if (BiosVideoPrivate->GraphicsOutput.Mode->Info != NULL) {
        gBS->FreePool (BiosVideoPrivate->GraphicsOutput.Mode->Info);
      }
      gBS->FreePool (BiosVideoPrivate->GraphicsOutput.Mode);
    }
  }

  return Status;
}

/**
  Check for VGA device

  @param BiosVideoPrivate - Pointer to BIOS_VIDEO_DEV structure

  @retval EFI_SUCCESS  Standard VGA device found
**/
EFI_STATUS
EFIAPI
BiosVideoCheckForVga (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate
  )
{
  EFI_STATUS            Status;
  BIOS_VIDEO_MODE_DATA  *ModeBuffer;

  //
  // Fill in the Graphics Output Protocol
  //
  BiosVideoPrivate->GraphicsOutput.QueryMode = BiosVideoGraphicsOutputQueryMode;
  BiosVideoPrivate->GraphicsOutput.SetMode = BiosVideoGraphicsOutputSetMode;
  BiosVideoPrivate->GraphicsOutput.Blt     = BiosVideoGraphicsOutputVgaBlt;

  //
  // Allocate buffer for Graphics Output Protocol mode information
  //
  Status = gBS->AllocatePool (
                EfiBootServicesData,
                sizeof (EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE),
                (VOID **) &BiosVideoPrivate->GraphicsOutput.Mode
                );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  Status = gBS->AllocatePool (
                EfiBootServicesData,
                sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
                (VOID **) &BiosVideoPrivate->GraphicsOutput.Mode->Info
                );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Add mode to the list of available modes
  //
  BiosVideoPrivate->GraphicsOutput.Mode->MaxMode = 1;

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (BIOS_VIDEO_MODE_DATA),
                  (VOID **) &ModeBuffer
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  ModeBuffer->VbeModeNumber         = 0x0012;
  ModeBuffer->BytesPerScanLine      = 640;
  ModeBuffer->LinearFrameBuffer     = (VOID *) (UINTN) (0xa0000);
  ModeBuffer->FrameBufferSize       = 0;
  ModeBuffer->HorizontalResolution  = 640;
  ModeBuffer->VerticalResolution    = 480;
  ModeBuffer->BitsPerPixel          = 8;
  ModeBuffer->PixelFormat           = PixelBltOnly;
  ModeBuffer->ColorDepth            = 32;
  ModeBuffer->RefreshRate           = 60;

  BiosVideoPrivate->ModeData = ModeBuffer;

  //
  // Test to see if the Video Adapter support the 640x480 16 color mode
  //
  BiosVideoPrivate->GraphicsOutput.Mode->Mode = GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER;
  Status = BiosVideoGraphicsOutputSetMode (&BiosVideoPrivate->GraphicsOutput, 0);

Done:
  //
  // If there was an error, then free the mode structure
  //
  if (EFI_ERROR (Status)) {
    if (BiosVideoPrivate->ModeData != NULL) {
      gBS->FreePool (BiosVideoPrivate->ModeData);
    }
    if (BiosVideoPrivate->GraphicsOutput.Mode != NULL) {
      if (BiosVideoPrivate->GraphicsOutput.Mode->Info != NULL) {
        gBS->FreePool (BiosVideoPrivate->GraphicsOutput.Mode->Info);
      }
      gBS->FreePool (BiosVideoPrivate->GraphicsOutput.Mode);
    }
  }
  return Status;
}
//
// Graphics Output Protocol Member Functions for VESA BIOS Extensions
//
/**

  Graphics Output protocol interface to get video mode


  @param This            - Protocol instance pointer.
  @param ModeNumber      - The mode number to return information on.
  @param SizeOfInfo      - A pointer to the size, in bytes, of the Info buffer.
  @param Info            - Caller allocated buffer that returns information about ModeNumber.

  @return EFI_SUCCESS           - Mode information returned.
          EFI_DEVICE_ERROR      - A hardware error occurred trying to retrieve the video mode.
          EFI_NOT_STARTED       - Video display is not initialized. Call SetMode ()
          EFI_INVALID_PARAMETER - One of the input args was NULL.

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  BIOS_VIDEO_DEV        *BiosVideoPrivate;
  EFI_STATUS            Status;
  BIOS_VIDEO_MODE_DATA  *ModeData;

  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);

  if (BiosVideoPrivate->HardwareNeedsStarting) {
    return EFI_NOT_STARTED;
  }

  if (This == NULL || Info == NULL || SizeOfInfo == NULL || ModeNumber >= This->Mode->MaxMode) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
                  (VOID**) Info
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *SizeOfInfo = sizeof (EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  ModeData = &BiosVideoPrivate->ModeData[ModeNumber];
  (*Info)->Version = 0;
  (*Info)->HorizontalResolution = ModeData->HorizontalResolution;
  (*Info)->VerticalResolution   = ModeData->VerticalResolution;
  (*Info)->PixelFormat = ModeData->PixelFormat;
  (*Info)->PixelInformation = ModeData->PixelBitMask;

  (*Info)->PixelsPerScanLine =  (ModeData->BytesPerScanLine * 8) / ModeData->BitsPerPixel;

  return EFI_SUCCESS;
}

/**
  Worker function to set video mode.

  @param  BiosVideoPrivate       Instance of BIOS_VIDEO_DEV.
  @param  ModeData               The mode data to be set.
  @param  DevicePath             Pointer to Device Path Protocol.

  @return EFI_SUCCESS      - Graphics mode was changed.
          EFI_DEVICE_ERROR - The device had an error and could not complete the request.
          EFI_UNSUPPORTED  - ModeNumber is not supported by this device.

**/
STATIC
EFI_STATUS
BiosVideoSetModeWorker (
  IN  BIOS_VIDEO_DEV               *BiosVideoPrivate,
  IN  BIOS_VIDEO_MODE_DATA         *ModeData,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  )
{
  IA32_REGISTER_SET   Regs;

  if (BiosVideoPrivate->LineBuffer != NULL) {
    FreePool (BiosVideoPrivate->LineBuffer);
    BiosVideoPrivate->LineBuffer = NULL;
  }

  if (BiosVideoPrivate->VgaFrameBuffer != NULL) {
    FreePool (BiosVideoPrivate->VgaFrameBuffer);
    BiosVideoPrivate->VgaFrameBuffer = NULL;
  }

  if (BiosVideoPrivate->VbeFrameBuffer != NULL) {
    FreePool (BiosVideoPrivate->VbeFrameBuffer);
    BiosVideoPrivate->VbeFrameBuffer = NULL;
  }

  BiosVideoPrivate->LineBuffer = (UINT8 *) AllocatePool (ModeData->BytesPerScanLine);
  if (NULL == BiosVideoPrivate->LineBuffer) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Clear all registers
  //
  ZeroMem (&Regs, sizeof (Regs));
  if (ModeData->VbeModeNumber < 0x100) {
    //
    // Allocate a working buffer for BLT operations to the VGA frame buffer
    //
    BiosVideoPrivate->VgaFrameBuffer = (UINT8 *) AllocatePool (4 * 480 * 80);
    if (NULL == BiosVideoPrivate->VgaFrameBuffer) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Set VGA Mode
    //
    Regs.X.AX = ModeData->VbeModeNumber;
    LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);
  } else {
    //
    // Allocate a working buffer for BLT operations to the VBE frame buffer
    //
    BiosVideoPrivate->VbeFrameBuffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) AllocatePool (
      ModeData->BytesPerScanLine * ModeData->VerticalResolution
      );
    if (NULL == BiosVideoPrivate->VbeFrameBuffer) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Set VBE mode
    //
    Regs.X.AX = VESA_BIOS_EXTENSIONS_SET_MODE;
    Regs.X.BX = (UINT16) (ModeData->VbeModeNumber | VESA_BIOS_EXTENSIONS_MODE_NUMBER_LINEAR_FRAME_BUFFER);
    ZeroMem (BiosVideoPrivate->VbeCrtcInformationBlock, sizeof (VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK));
    Regs.E.ES = EFI_SEGMENT ((UINTN) BiosVideoPrivate->VbeCrtcInformationBlock);
    Regs.X.DI = EFI_OFFSET ((UINTN) BiosVideoPrivate->VbeCrtcInformationBlock);

    LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);

    //
    // Check to see if the call succeeded
    //
    if (Regs.X.AX != VESA_BIOS_EXTENSIONS_STATUS_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}

/**

  Graphics Output protocol interface to set video mode


  @param This            - Protocol instance pointer.
  @param ModeNumber      - The mode number to be set.

  @return EFI_SUCCESS      - Graphics mode was changed.
          EFI_DEVICE_ERROR - The device had an error and could not complete the request.
          EFI_UNSUPPORTED  - ModeNumber is not supported by this device.

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
  IN  UINT32                       ModeNumber
  )
{
  EFI_STATUS              Status;
  BIOS_VIDEO_DEV          *BiosVideoPrivate;
  BIOS_VIDEO_MODE_DATA    *ModeData;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);

  ModeData = &BiosVideoPrivate->ModeData[ModeNumber];

  if (ModeNumber >= This->Mode->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  if (ModeNumber == This->Mode->Mode) {
    return EFI_SUCCESS;
  }

  Status = BiosVideoSetModeWorker (BiosVideoPrivate, ModeData, BiosVideoPrivate->DevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  This->Mode->Mode = ModeNumber;
  This->Mode->Info->Version = 0;
  This->Mode->Info->HorizontalResolution = ModeData->HorizontalResolution;
  This->Mode->Info->VerticalResolution = ModeData->VerticalResolution;
  This->Mode->Info->PixelFormat = ModeData->PixelFormat;
  This->Mode->Info->PixelInformation = ModeData->PixelBitMask;
  This->Mode->Info->PixelsPerScanLine =  (ModeData->BytesPerScanLine * 8) / ModeData->BitsPerPixel;
  This->Mode->SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

  //
  // Frame BufferSize remain unchanged
  //
  This->Mode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS)(UINTN)ModeData->LinearFrameBuffer;
  This->Mode->FrameBufferSize = ModeData->FrameBufferSize;

  BiosVideoPrivate->HardwareNeedsStarting = FALSE;

  return EFI_SUCCESS;
}

/**

  Update physical frame buffer, copy 4 bytes block, then copy remaining bytes.


  @param PciIo           - The pointer of EFI_PCI_IO_PROTOCOL
  @param VbeBuffer       - The data to transfer to screen
  @param MemAddress      - Physical frame buffer base address
  @param DestinationX    - The X coordinate of the destination for BltOperation
  @param DestinationY    - The Y coordinate of the destination for BltOperation
  @param TotalBytes      - The total bytes of copy
  @param VbePixelWidth   - Bytes per pixel
  @param BytesPerScanLine - Bytes per scan line

  @return None.

**/
VOID
CopyVideoBuffer (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT8                 *VbeBuffer,
  IN  VOID                  *MemAddress,
  IN  UINTN                 DestinationX,
  IN  UINTN                 DestinationY,
  IN  UINTN                 TotalBytes,
  IN  UINT32                VbePixelWidth,
  IN  UINTN                 BytesPerScanLine
  )
{
  UINTN                 FrameBufferAddr;
  UINTN                 CopyBlockNum;
  UINTN                 RemainingBytes;
  UINTN                 UnalignedBytes;
  EFI_STATUS            Status;

  FrameBufferAddr = (UINTN) MemAddress + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth;

  //
  // If TotalBytes is less than 4 bytes, only start byte copy.
  //
  if (TotalBytes < 4) {
    Status = PciIo->Mem.Write (
                     PciIo,
                     EfiPciIoWidthUint8,
                     EFI_PCI_IO_PASS_THROUGH_BAR,
                     (UINT64) FrameBufferAddr,
                     TotalBytes,
                     VbeBuffer
                     );
    ASSERT_EFI_ERROR (Status);
    return;
  }

  //
  // If VbeBuffer is not 4-byte aligned, start byte copy.
  //
  UnalignedBytes  = (4 - ((UINTN) VbeBuffer & 0x3)) & 0x3;

  if (UnalignedBytes != 0) {
    Status = PciIo->Mem.Write (
                     PciIo,
                     EfiPciIoWidthUint8,
                     EFI_PCI_IO_PASS_THROUGH_BAR,
                     (UINT64) FrameBufferAddr,
                     UnalignedBytes,
                     VbeBuffer
                     );
    ASSERT_EFI_ERROR (Status);
    FrameBufferAddr += UnalignedBytes;
    VbeBuffer       += UnalignedBytes;
  }

  //
  // Calculate 4-byte block count and remaining bytes.
  //
  CopyBlockNum   = (TotalBytes - UnalignedBytes) >> 2;
  RemainingBytes = (TotalBytes - UnalignedBytes) &  3;

  //
  // Copy 4-byte block and remaining bytes to physical frame buffer.
  //
  if (CopyBlockNum != 0) {
    Status = PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthUint32,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) FrameBufferAddr,
                    CopyBlockNum,
                    VbeBuffer
                    );
    ASSERT_EFI_ERROR (Status);
  }

  if (RemainingBytes != 0) {
    FrameBufferAddr += (CopyBlockNum << 2);
    VbeBuffer       += (CopyBlockNum << 2);
    Status = PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) FrameBufferAddr,
                    RemainingBytes,
                    VbeBuffer
                    );
    ASSERT_EFI_ERROR (Status);
  }
}

//
// BUGBUG : Add Blt for 16 bit color, 15 bit color, and 8 bit color modes
//
/**

  Graphics Output protocol instance to block transfer for VBE device


  @param This            - Pointer to Graphics Output protocol instance
  @param BltBuffer       - The data to transfer to screen
  @param BltOperation    - The operation to perform
  @param SourceX         - The X coordinate of the source for BltOperation
  @param SourceY         - The Y coordinate of the source for BltOperation
  @param DestinationX    - The X coordinate of the destination for BltOperation
  @param DestinationY    - The Y coordinate of the destination for BltOperation
  @param Width           - The width of a rectangle in the blt rectangle in pixels
  @param Height          - The height of a rectangle in the blt rectangle in pixels
  @param Delta           - Not used for EfiBltVideoFill and EfiBltVideoToVideo operation.
                         If a Delta of 0 is used, the entire BltBuffer will be operated on.
                         If a subrectangle of the BltBuffer is used, then Delta represents
                         the number of bytes in a row of the BltBuffer.

  @return EFI_INVALID_PARAMETER - Invalid parameter passed in
          EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputVbeBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta
  )
{
  BIOS_VIDEO_DEV                 *BiosVideoPrivate;
  BIOS_VIDEO_MODE_DATA           *Mode;
  EFI_PCI_IO_PROTOCOL            *PciIo;
  EFI_TPL                        OriginalTPL;
  UINTN                          DstY;
  UINTN                          SrcY;
  UINTN                          DstX;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Blt;
  VOID                           *MemAddress;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *VbeFrameBuffer;
  UINTN                          BytesPerScanLine;
  UINTN                          Index;
  UINT8                          *VbeBuffer;
  UINT8                          *VbeBuffer1;
  UINT8                          *BltUint8;
  UINT32                         VbePixelWidth;
  UINT32                         Pixel;
  UINTN                          TotalBytes;

  if (This == NULL || ((UINTN) BltOperation) >= EfiGraphicsOutputBltOperationMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }

  BiosVideoPrivate  = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);
  Mode              = &BiosVideoPrivate->ModeData[This->Mode->Mode];
  PciIo             = BiosVideoPrivate->PciIo;

  VbeFrameBuffer    = BiosVideoPrivate->VbeFrameBuffer;
  MemAddress        = Mode->LinearFrameBuffer;
  BytesPerScanLine  = Mode->BytesPerScanLine;
  VbePixelWidth     = Mode->BitsPerPixel / 8;
  BltUint8          = (UINT8 *) BltBuffer;
  TotalBytes        = Width * VbePixelWidth;

  //
  // We need to fill the Virtual Screen buffer with the blt data.
  // The virtual screen is upside down, as the first row is the bootom row of
  // the image.
  //
  if (BltOperation == EfiBltVideoToBltBuffer) {
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if (SourceY + Height > Mode->VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > Mode->HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > Mode->VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > Mode->HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta
  // is the number of bytes in each row of BltBuffer.  Since BltBuffer is Width pixels size,
  // the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }
  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  switch (BltOperation) {
  case EfiBltVideoToBltBuffer:
    for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {
      Blt = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) (BltUint8 + DstY * Delta + DestinationX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      //
      // Shuffle the packed bytes in the hardware buffer to match EFI_GRAPHICS_OUTPUT_BLT_PIXEL
      //
      VbeBuffer = ((UINT8 *) VbeFrameBuffer + (SrcY * BytesPerScanLine + SourceX * VbePixelWidth));
      for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
        Pixel         = *(UINT32 *) (VbeBuffer);
        Blt->Red      = (UINT8) ((Pixel >> Mode->Red.Position) & Mode->Red.Mask);
        Blt->Blue     = (UINT8) ((Pixel >> Mode->Blue.Position) & Mode->Blue.Mask);
        Blt->Green    = (UINT8) ((Pixel >> Mode->Green.Position) & Mode->Green.Mask);
        Blt->Reserved = 0;
        Blt++;
        VbeBuffer += VbePixelWidth;
      }

    }
    break;

  case EfiBltVideoToVideo:
    for (Index = 0; Index < Height; Index++) {
      if (DestinationY <= SourceY) {
        SrcY  = SourceY + Index;
        DstY  = DestinationY + Index;
      } else {
        SrcY  = SourceY + Height - Index - 1;
        DstY  = DestinationY + Height - Index - 1;
      }

      VbeBuffer   = ((UINT8 *) VbeFrameBuffer + DstY * BytesPerScanLine + DestinationX * VbePixelWidth);
      VbeBuffer1  = ((UINT8 *) VbeFrameBuffer + SrcY * BytesPerScanLine + SourceX * VbePixelWidth);

      CopyMem (
            VbeBuffer,
            VbeBuffer1,
            TotalBytes
            );

      //
      // Update physical frame buffer.
      //
      CopyVideoBuffer (
        PciIo,
        VbeBuffer,
        MemAddress,
        DestinationX,
        DstY,
        TotalBytes,
        VbePixelWidth,
        BytesPerScanLine
        );
    }
    break;

  case EfiBltVideoFill:
    VbeBuffer = (UINT8 *) ((UINTN) VbeFrameBuffer + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth);
    Blt       = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) BltUint8;
    //
    // Shuffle the RGB fields in EFI_GRAPHICS_OUTPUT_BLT_PIXEL to match the hardware buffer
    //
    Pixel = ((Blt->Red & Mode->Red.Mask) << Mode->Red.Position) |
      (
        (Blt->Green & Mode->Green.Mask) <<
        Mode->Green.Position
      ) |
          ((Blt->Blue & Mode->Blue.Mask) << Mode->Blue.Position);

    for (Index = 0; Index < Width; Index++) {
      CopyMem (
            VbeBuffer,
            &Pixel,
            VbePixelWidth
            );
      VbeBuffer += VbePixelWidth;
    }

    VbeBuffer = (UINT8 *) ((UINTN) VbeFrameBuffer + (DestinationY * BytesPerScanLine) + DestinationX * VbePixelWidth);
    for (DstY = DestinationY + 1; DstY < (Height + DestinationY); DstY++) {
      CopyMem (
            (VOID *) ((UINTN) VbeFrameBuffer + (DstY * BytesPerScanLine) + DestinationX * VbePixelWidth),
            VbeBuffer,
            TotalBytes
            );
    }
    for (DstY = DestinationY; DstY < (Height + DestinationY); DstY++) {
      //
      // Update physical frame buffer.
      //
      CopyVideoBuffer (
        PciIo,
        VbeBuffer,
        MemAddress,
        DestinationX,
        DstY,
        TotalBytes,
        VbePixelWidth,
        BytesPerScanLine
        );
    }
    break;

  case EfiBltBufferToVideo:
    for (SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); SrcY++, DstY++) {
      Blt       = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) (BltUint8 + (SrcY * Delta) + (SourceX) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      VbeBuffer = ((UINT8 *) VbeFrameBuffer + (DstY * BytesPerScanLine + DestinationX * VbePixelWidth));
      for (DstX = DestinationX; DstX < (Width + DestinationX); DstX++) {
        //
        // Shuffle the RGB fields in EFI_GRAPHICS_OUTPUT_BLT_PIXEL to match the hardware buffer
        //
        Pixel = ((Blt->Red & Mode->Red.Mask) << Mode->Red.Position) |
          ((Blt->Green & Mode->Green.Mask) << Mode->Green.Position) |
            ((Blt->Blue & Mode->Blue.Mask) << Mode->Blue.Position);
        CopyMem (
              VbeBuffer,
              &Pixel,
              VbePixelWidth
              );
        Blt++;
        VbeBuffer += VbePixelWidth;
      }

      VbeBuffer = ((UINT8 *) VbeFrameBuffer + (DstY * BytesPerScanLine + DestinationX * VbePixelWidth));

      //
      // Update physical frame buffer.
      //
      CopyVideoBuffer (
        PciIo,
        VbeBuffer,
        MemAddress,
        DestinationX,
        DstY,
        TotalBytes,
        VbePixelWidth,
        BytesPerScanLine
        );
    }
    break;
  default:
    break;
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}

/**

  Write graphics controller registers


  @param PciIo           - Pointer to PciIo protocol instance of the controller
  @param Address         - Register address
  @param Data            - Data to be written to register

  @return None

**/
STATIC
VOID
WriteGraphicsController (
  IN  EFI_PCI_IO_PROTOCOL  *PciIo,
  IN  UINTN                Address,
  IN  UINTN                Data
  )
{
  Address = Address | (Data << 8);
  PciIo->Io.Write (
              PciIo,
              EfiPciIoWidthUint16,
              EFI_PCI_IO_PASS_THROUGH_BAR,
              VGA_GRAPHICS_CONTROLLER_ADDRESS_REGISTER,
              1,
              &Address
              );
}

/**

  Read the four bit plane of VGA frame buffer


  @param PciIo           - Pointer to PciIo protocol instance of the controller
  @param HardwareBuffer  - Hardware VGA frame buffer address
  @param MemoryBuffer    - Memory buffer address
  @param WidthInBytes    - Number of bytes in a line to read
  @param Height          - Height of the area to read

  @return None

**/
VOID
VgaReadBitPlanes (
  EFI_PCI_IO_PROTOCOL  *PciIo,
  UINT8                *HardwareBuffer,
  UINT8                *MemoryBuffer,
  UINTN                WidthInBytes,
  UINTN                Height
  )
{
  UINTN BitPlane;
  UINTN Rows;
  UINTN FrameBufferOffset;
  UINT8 *Source;
  UINT8 *Destination;

  //
  // Program the Mode Register Write mode 0, Read mode 0
  //
  WriteGraphicsController (
    PciIo,
    VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
    VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_0
    );

  for (BitPlane = 0, FrameBufferOffset = 0;
       BitPlane < VGA_NUMBER_OF_BIT_PLANES;
       BitPlane++, FrameBufferOffset += VGA_BYTES_PER_BIT_PLANE
      ) {
    //
    // Program the Read Map Select Register to select the correct bit plane
    //
    WriteGraphicsController (
      PciIo,
      VGA_GRAPHICS_CONTROLLER_READ_MAP_SELECT_REGISTER,
      BitPlane
      );

    Source      = HardwareBuffer;
    Destination = MemoryBuffer + FrameBufferOffset;

    for (Rows = 0; Rows < Height; Rows++, Source += VGA_BYTES_PER_SCAN_LINE, Destination += VGA_BYTES_PER_SCAN_LINE) {
      PciIo->Mem.Read (
                  PciIo,
                  EfiPciIoWidthUint8,
                  (UINT8) EFI_PCI_IO_PASS_THROUGH_BAR,
                  (UINT64)(UINTN) Source,
                  WidthInBytes,
                  (VOID *) Destination
                  );
    }
  }
}

/**

  Internal routine to convert VGA color to Graphics Output color


  @param MemoryBuffer    - Buffer containing VGA color
  @param X               - The X coordinate of pixel on screen
  @param Y               - The Y coordinate of pixel on screen
  @param BltBuffer       - Buffer to contain converted Graphics Output color

  @return None

**/
VOID
VgaConvertToGraphicsOutputColor (
  UINT8                          *MemoryBuffer,
  UINTN                          X,
  UINTN                          Y,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer
  )
{
  UINTN Mask;
  UINTN Bit;
  UINTN Color;

  MemoryBuffer += ((Y << 6) + (Y << 4) + (X >> 3));
  Mask = mVgaBitMaskTable[X & 0x07];
  for (Bit = 0x01, Color = 0; Bit < 0x10; Bit <<= 1, MemoryBuffer += VGA_BYTES_PER_BIT_PLANE) {
    if ((*MemoryBuffer & Mask) != 0) {
      Color |= Bit;
    }
  }

  *BltBuffer = mVgaColorToGraphicsOutputColor[Color];
}

/**

  Internal routine to convert Graphics Output color to VGA color


  @param BltBuffer       - buffer containing Graphics Output color

  @return Converted VGA color

**/
UINT8
VgaConvertColor (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL          *BltBuffer
  )
{
  UINT8 Color;

  Color = (UINT8) ((BltBuffer->Blue >> 7) | ((BltBuffer->Green >> 6) & 0x02) | ((BltBuffer->Red >> 5) & 0x04));
  if ((BltBuffer->Red + BltBuffer->Green + BltBuffer->Blue) > 0x180) {
    Color |= 0x08;
  }

  return Color;
}

/**
  Grahpics Output protocol instance to block transfer for VGA device

  @param This            Pointer to Grahpics Output protocol instance
  @param BltBuffer       The data to transfer to screen
  @param BltOperation    The operation to perform
  @param SourceX         The X coordinate of the source for BltOperation
  @param SourceY         The Y coordinate of the source for BltOperation
  @param DestinationX    The X coordinate of the destination for BltOperation
  @param DestinationY    The Y coordinate of the destination for BltOperation
  @param Width           The width of a rectangle in the blt rectangle in pixels
  @param Height          The height of a rectangle in the blt rectangle in pixels
  @param Delta           Not used for EfiBltVideoFill and EfiBltVideoToVideo operation.
                         If a Delta of 0 is used, the entire BltBuffer will be operated on.
                         If a subrectangle of the BltBuffer is used, then Delta represents
                         the number of bytes in a row of the BltBuffer.

  @retval EFI_INVALID_PARAMETER Invalid parameter passed in
  @retval EFI_SUCCESS           Blt operation success

**/
EFI_STATUS
EFIAPI
BiosVideoGraphicsOutputVgaBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta
  )
{
  BIOS_VIDEO_DEV      *BiosVideoPrivate;
  EFI_TPL             OriginalTPL;
  UINT8               *MemAddress;
  UINTN               BytesPerScanLine;
  //UINTN               BytesPerBitPlane;
  UINTN               Bit;
  UINTN               Index;
  UINTN               Index1;
  UINTN               StartAddress;
  UINTN               Bytes;
  UINTN               Offset;
  UINT8               LeftMask;
  UINT8               RightMask;
  UINTN               Address;
  UINTN               AddressFix;
  UINT8               *Address1;
  UINT8               *SourceAddress;
  UINT8               *DestinationAddress;
  EFI_PCI_IO_PROTOCOL *PciIo;
  UINT8               Data;
  UINT8               PixelColor;
  UINT8               *VgaFrameBuffer;
  UINTN               SourceOffset;
  UINTN               SourceWidth;
  UINTN               Rows;
  UINTN               Columns;
  UINTN               X;
  UINTN               Y;
  UINTN               CurrentMode;

  if (This == NULL || ((UINTN) BltOperation) >= EfiGraphicsOutputBltOperationMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }

  BiosVideoPrivate  = BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS (This);

  CurrentMode = This->Mode->Mode;
  PciIo             = BiosVideoPrivate->PciIo;
  MemAddress        = BiosVideoPrivate->ModeData[CurrentMode].LinearFrameBuffer;
  BytesPerScanLine  = BiosVideoPrivate->ModeData[CurrentMode].BytesPerScanLine >> 3;
  //BytesPerBitPlane  = BytesPerScanLine * BiosVideoPrivate->ModeData[CurrentMode].VerticalResolution;
  VgaFrameBuffer    = BiosVideoPrivate->VgaFrameBuffer;

  //
  // We need to fill the Virtual Screen buffer with the blt data.
  // The virtual screen is upside down, as the first row is the bootom row of
  // the image.
  //
  if (BltOperation == EfiBltVideoToBltBuffer) {
    //
    // Video to BltBuffer: Source is Video, destination is BltBuffer
    //
    if (SourceY + Height > BiosVideoPrivate->ModeData[CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (SourceX + Width > BiosVideoPrivate->ModeData[CurrentMode].HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // BltBuffer to Video: Source is BltBuffer, destination is Video
    //
    if (DestinationY + Height > BiosVideoPrivate->ModeData[CurrentMode].VerticalResolution) {
      return EFI_INVALID_PARAMETER;
    }

    if (DestinationX + Width > BiosVideoPrivate->ModeData[CurrentMode].HorizontalResolution) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta
  // is the number of bytes in each row of BltBuffer.  Since BltBuffer is Width pixels size,
  // the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }
  //
  // We have to raise to TPL Notify, so we make an atomic write the frame buffer.
  // We would not want a timer based event (Cursor, ...) to come in while we are
  // doing this operation.
  //
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Compute some values we need for VGA
  //
  switch (BltOperation) {
  case EfiBltVideoToBltBuffer:

    SourceOffset  = (SourceY << 6) + (SourceY << 4) + (SourceX >> 3);
    SourceWidth   = ((SourceX + Width - 1) >> 3) - (SourceX >> 3) + 1;

    //
    // Read all the pixels in the 4 bit planes into a memory buffer that looks like the VGA buffer
    //
    VgaReadBitPlanes (
      PciIo,
      MemAddress + SourceOffset,
      VgaFrameBuffer + SourceOffset,
      SourceWidth,
      Height
      );

    //
    // Convert VGA Bit Planes to a Graphics Output 32-bit color value
    //
    BltBuffer += (DestinationY * (Delta >> 2) + DestinationX);
    for (Rows = 0, Y = SourceY; Rows < Height; Rows++, Y++, BltBuffer += (Delta >> 2)) {
      for (Columns = 0, X = SourceX; Columns < Width; Columns++, X++, BltBuffer++) {
        VgaConvertToGraphicsOutputColor (VgaFrameBuffer, X, Y, BltBuffer);
      }

      BltBuffer -= Width;
    }

    break;

  case EfiBltVideoToVideo:
    //
    // Check for an aligned Video to Video operation
    //
    if ((SourceX & 0x07) == 0x00 && (DestinationX & 0x07) == 0x00 && (Width & 0x07) == 0x00) {
      //
      // Program the Mode Register Write mode 1, Read mode 0
      //
      WriteGraphicsController (
        PciIo,
        VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
        VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_1
        );

      SourceAddress       = (UINT8 *) (MemAddress + (SourceY << 6) + (SourceY << 4) + (SourceX >> 3));
      DestinationAddress  = (UINT8 *) (MemAddress + (DestinationY << 6) + (DestinationY << 4) + (DestinationX >> 3));
      Bytes               = Width >> 3;
      for (Index = 0, Offset = 0; Index < Height; Index++, Offset += BytesPerScanLine) {
        PciIo->CopyMem (
                PciIo,
                EfiPciIoWidthUint8,
                EFI_PCI_IO_PASS_THROUGH_BAR,
                (UINT64) ((UINTN)DestinationAddress + Offset),
                EFI_PCI_IO_PASS_THROUGH_BAR,
                (UINT64) ((UINTN)SourceAddress + Offset),
                Bytes
                );
      }
    } else {
      SourceOffset  = (SourceY << 6) + (SourceY << 4) + (SourceX >> 3);
      SourceWidth   = ((SourceX + Width - 1) >> 3) - (SourceX >> 3) + 1;

      //
      // Read all the pixels in the 4 bit planes into a memory buffer that looks like the VGA buffer
      //
      VgaReadBitPlanes (
        PciIo,
        MemAddress + SourceOffset,
        VgaFrameBuffer + SourceOffset,
        SourceWidth,
        Height
        );
    }

    break;

  case EfiBltVideoFill:
    StartAddress  = (UINTN) (MemAddress + (DestinationY << 6) + (DestinationY << 4) + (DestinationX >> 3));
    Bytes         = ((DestinationX + Width - 1) >> 3) - (DestinationX >> 3);
    LeftMask      = mVgaLeftMaskTable[DestinationX & 0x07];
    RightMask     = mVgaRightMaskTable[(DestinationX + Width - 1) & 0x07];
    if (Bytes == 0) {
      LeftMask = (UINT8) (LeftMask & RightMask);
      RightMask = 0;
    }

    if (LeftMask == 0xff) {
      StartAddress--;
      Bytes++;
      LeftMask = 0;
    }

    if (RightMask == 0xff) {
      Bytes++;
      RightMask = 0;
    }

    PixelColor = VgaConvertColor (BltBuffer);

    //
    // Program the Mode Register Write mode 2, Read mode 0
    //
    WriteGraphicsController (
      PciIo,
      VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
      VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_2
      );

    //
    // Program the Data Rotate/Function Select Register to replace
    //
    WriteGraphicsController (
      PciIo,
      VGA_GRAPHICS_CONTROLLER_DATA_ROTATE_REGISTER,
      VGA_GRAPHICS_CONTROLLER_FUNCTION_REPLACE
      );

    if (LeftMask != 0) {
      //
      // Program the BitMask register with the Left column mask
      //
      WriteGraphicsController (
        PciIo,
        VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
        LeftMask
        );

      for (Index = 0, Address = StartAddress; Index < Height; Index++, Address += BytesPerScanLine) {
        //
        // Read data from the bit planes into the latches
        //
        PciIo->Mem.Read (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) (UINTN) Address,
                    1,
                    &Data
                    );
        //
        // Write the lower 4 bits of PixelColor to the bit planes in the pixels enabled by BitMask
        //
        PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) (UINTN) Address,
                    1,
                    &PixelColor
                    );
      }
    }

    if (Bytes > 1) {
      //
      // Program the BitMask register with the middle column mask of 0xff
      //
      WriteGraphicsController (
        PciIo,
        VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
        0xff
        );

      for (Index = 0, Address = StartAddress + 1; Index < Height; Index++, Address += BytesPerScanLine) {
        PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthFillUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) (UINTN) Address,
                    Bytes - 1,
                    &PixelColor
                    );
      }
    }

    if (RightMask != 0) {
      //
      // Program the BitMask register with the Right column mask
      //
      WriteGraphicsController (
        PciIo,
        VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
        RightMask
        );

      for (Index = 0, Address = StartAddress + Bytes; Index < Height; Index++, Address += BytesPerScanLine) {
        //
        // Read data from the bit planes into the latches
        //
        PciIo->Mem.Read (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) (UINTN) Address,
                    1,
                    &Data
                    );
        //
        // Write the lower 4 bits of PixelColor to the bit planes in the pixels enabled by BitMask
        //
        PciIo->Mem.Write (
                    PciIo,
                    EfiPciIoWidthUint8,
                    EFI_PCI_IO_PASS_THROUGH_BAR,
                    (UINT64) (UINTN) Address,
                    1,
                    &PixelColor
                    );
      }
    }
    break;

  case EfiBltBufferToVideo:
    StartAddress  = (UINTN) (MemAddress + (DestinationY << 6) + (DestinationY << 4) + (DestinationX >> 3));
    LeftMask      = mVgaBitMaskTable[DestinationX & 0x07];

    //
    // Program the Mode Register Write mode 2, Read mode 0
    //
    WriteGraphicsController (
      PciIo,
      VGA_GRAPHICS_CONTROLLER_MODE_REGISTER,
      VGA_GRAPHICS_CONTROLLER_READ_MODE_0 | VGA_GRAPHICS_CONTROLLER_WRITE_MODE_2
      );

    //
    // Program the Data Rotate/Function Select Register to replace
    //
    WriteGraphicsController (
      PciIo,
      VGA_GRAPHICS_CONTROLLER_DATA_ROTATE_REGISTER,
      VGA_GRAPHICS_CONTROLLER_FUNCTION_REPLACE
      );

    for (Index = 0, Address = StartAddress; Index < Height; Index++, Address += BytesPerScanLine) {
      for (Index1 = 0; Index1 < Width; Index1++) {
        BiosVideoPrivate->LineBuffer[Index1] = VgaConvertColor (&BltBuffer[(SourceY + Index) * (Delta >> 2) + SourceX + Index1]);
      }
      AddressFix = Address;

      for (Bit = 0; Bit < 8; Bit++) {
        //
        // Program the BitMask register with the Left column mask
        //
        WriteGraphicsController (
          PciIo,
          VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER,
          LeftMask
          );

        for (Index1 = Bit, Address1 = (UINT8 *) AddressFix; Index1 < Width; Index1 += 8, Address1++) {
          //
          // Read data from the bit planes into the latches
          //
          PciIo->Mem.Read (
                      PciIo,
                      EfiPciIoWidthUint8,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      (UINT64)(UINTN) Address1,
                      1,
                      &Data
                      );

          PciIo->Mem.Write (
                      PciIo,
                      EfiPciIoWidthUint8,
                      EFI_PCI_IO_PASS_THROUGH_BAR,
                      (UINT64)(UINTN) Address1,
                      1,
                      &BiosVideoPrivate->LineBuffer[Index1]
                      );
        }

        LeftMask = (UINT8) (LeftMask >> 1);
        if (LeftMask == 0) {
          LeftMask = 0x80;
          AddressFix++;
        }
      }
    }

    break;
  default:
    break;
  }

  gBS->RestoreTPL (OriginalTPL);

  return EFI_SUCCESS;
}
//
// VGA Mini Port Protocol Functions
//
/**
  VgaMiniPort protocol interface to set mode

  @param This             Pointer to VgaMiniPort protocol instance
  @param ModeNumber       The index of the mode

  @retval EFI_UNSUPPORTED The requested mode is not supported
  @retval EFI_SUCCESS     The requested mode is set successfully

**/
EFI_STATUS
EFIAPI
BiosVideoVgaMiniPortSetMode (
  IN  EFI_VGA_MINI_PORT_PROTOCOL  *This,
  IN  UINTN                       ModeNumber
  )
{
  BIOS_VIDEO_DEV        *BiosVideoPrivate;
  IA32_REGISTER_SET Regs;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Make sure the ModeNumber is a valid value
  //
  if (ModeNumber >= This->MaxMode) {
    return EFI_UNSUPPORTED;
  }
  //
  // Get the device structure for this device
  //
  BiosVideoPrivate = BIOS_VIDEO_DEV_FROM_VGA_MINI_PORT_THIS (This);

  ZeroMem (&Regs, sizeof (Regs));

  switch (ModeNumber) {
  case 0:
    //
    // Set the 80x25 Text VGA Mode
    //
    Regs.H.AH = 0x00;
    Regs.H.AL = 0x83;
    LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);

    Regs.H.AH = 0x11;
    Regs.H.AL = 0x14;
    Regs.H.BL = 0;
    LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);

    break;

  case 1:
    //
    // Set the 80x50 Text VGA Mode
    //
    Regs.H.AH = 0x00;
    Regs.H.AL = 0x83;
    LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);

    Regs.H.AH = 0x11;
    Regs.H.AL = 0x12;
    Regs.H.BL = 0;
    LegacyBiosInt86 (BiosVideoPrivate, 0x10, &Regs);
    break;

  default:
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
