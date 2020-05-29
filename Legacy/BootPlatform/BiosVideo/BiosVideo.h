/** @file

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BIOS_GRAPHICS_OUTPUT_H
#define _BIOS_GRAPHICS_OUTPUT_H

#include <Uefi.h>

//
// Driver Consumed Protocol Prototypes
//
#include <Guid/GlobalVariable.h>

#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/VgaMiniPort.h>
#include <Protocol/Legacy8259.h>
#include <Protocol/EdidActive.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/EdidOverride.h>
#include <Protocol/DevicePath.h>

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>

#include <IndustryStandard/Pci.h>

#include "VesaBiosExtensions.h"

//
// ** CHANGE **
// Legacy region base is now 0x0C0000 instead of 0x100000.
//
#define LEGACY_REGION_BASE 0x0C0000


//
// Packed format support: The number of bits reserved for each of the colors and the actual
// position of RGB in the frame buffer is specified in the VBE Mode information
//
typedef struct {
  UINT8 Position; // Position of the color
  UINT8 Mask;     // The number of bits expressed as a mask
} BIOS_VIDEO_COLOR_PLACEMENT;

//
// BIOS Graphics Output Graphical Mode Data
//
typedef struct {
  UINT16                      VbeModeNumber;
  UINT16                      BytesPerScanLine;
  VOID                        *LinearFrameBuffer;
  UINTN                       FrameBufferSize;
  UINT32                      HorizontalResolution;
  UINT32                      VerticalResolution;
  UINT32                      ColorDepth;
  UINT32                      RefreshRate;
  UINT32                      BitsPerPixel;
  BIOS_VIDEO_COLOR_PLACEMENT  Red;
  BIOS_VIDEO_COLOR_PLACEMENT  Green;
  BIOS_VIDEO_COLOR_PLACEMENT  Blue;
  BIOS_VIDEO_COLOR_PLACEMENT  Reserved;
  EFI_GRAPHICS_PIXEL_FORMAT   PixelFormat;
  EFI_PIXEL_BITMASK           PixelBitMask;
} BIOS_VIDEO_MODE_DATA;

//
// BIOS video child handle private data Structure
//
#define BIOS_VIDEO_DEV_SIGNATURE    SIGNATURE_32 ('B', 'V', 'M', 'p')

typedef struct {
  UINTN                                       Signature;
  EFI_HANDLE                                  Handle;

  //
  // Consumed Protocols inherited from parent controller.
  //
  EFI_PCI_IO_PROTOCOL                         *PciIo;
  EFI_LEGACY_8259_PROTOCOL                    *Legacy8259;
  THUNK_CONTEXT                               *ThunkContext;

  //
  // Produced Protocols
  //
  EFI_GRAPHICS_OUTPUT_PROTOCOL                GraphicsOutput;
  EFI_EDID_DISCOVERED_PROTOCOL                EdidDiscovered;
  EFI_EDID_ACTIVE_PROTOCOL                    EdidActive;
  EFI_VGA_MINI_PORT_PROTOCOL                  VgaMiniPort;

  //
  // General fields
  //
  BOOLEAN                                     VgaCompatible;
  BOOLEAN                                     ProduceGraphicsOutput;

  //
  // Graphics Output Protocol related fields
  //
  BOOLEAN                                     HardwareNeedsStarting;
  UINTN                                       CurrentMode;
  UINTN                                       MaxMode;
  BIOS_VIDEO_MODE_DATA                        *ModeData;
  UINT8                                       *LineBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL               *VbeFrameBuffer;
  UINT8                                       *VgaFrameBuffer;

  //
  // VESA Bios Extensions related fields
  //
  UINTN                                       NumberOfPagesBelow1MB;    // Number of 4KB pages in PagesBelow1MB
  EFI_PHYSICAL_ADDRESS                        PagesBelow1MB;            // Buffer for all VBE Information Blocks
  VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK      *VbeInformationBlock;     // 0x200 bytes.  Must be allocated below 1MB
  VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK *VbeModeInformationBlock; // 0x100 bytes.  Must be allocated below 1MB
  VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK        *VbeEdidDataBlock;        // 0x80  bytes.  Must be allocated below 1MB
  VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK *VbeCrtcInformationBlock; // 59 bytes.  Must be allocated below 1MB
  UINTN                                       VbeSaveRestorePages;      // Number of 4KB pages in VbeSaveRestoreBuffer
  EFI_PHYSICAL_ADDRESS                        VbeSaveRestoreBuffer;     // Must be allocated below 1MB
  //
  // Status code
  //
  EFI_DEVICE_PATH_PROTOCOL                    *DevicePath;
  EFI_EVENT                                   ExitBootServicesEvent;
} BIOS_VIDEO_DEV;

#define BIOS_VIDEO_DEV_FROM_PCI_IO_THIS(a)      CR (a, BIOS_VIDEO_DEV, PciIo, BIOS_VIDEO_DEV_SIGNATURE)
#define BIOS_VIDEO_DEV_FROM_GRAPHICS_OUTPUT_THIS(a)      CR (a, BIOS_VIDEO_DEV, GraphicsOutput, BIOS_VIDEO_DEV_SIGNATURE)
#define BIOS_VIDEO_DEV_FROM_VGA_MINI_PORT_THIS(a) CR (a, BIOS_VIDEO_DEV, VgaMiniPort, BIOS_VIDEO_DEV_SIGNATURE)

#define GRAPHICS_OUTPUT_INVALIDE_MODE_NUMBER  0xffff

#define EFI_SEGMENT(_Adr)     (UINT16) ((UINT16) (((UINTN) (_Adr)) >> 4) & 0xf000)
#define EFI_OFFSET(_Adr)      (UINT16) (((UINT16) ((UINTN) (_Adr))) & 0xffff)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gBiosVideoDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gBiosVideoComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gBiosVideoComponentName2;

//
// Driver Binding Protocol functions
//
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
  );


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
  );


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
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

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
  IN  THUNK_CONTEXT                  *ThunkContext,
  IN  EFI_DEVICE_PATH_PROTOCOL       *ParentDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

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
  );

/**
  Collect the resource from destroyed bios video device.

  @param BiosVideoPrivate   Video child device private data structure
**/
VOID
BiosVideoDeviceReleaseResource (
  BIOS_VIDEO_DEV  *BiosVideoPrivate
  );

//
// Private worker functions
//
/**
  Check for VBE device.

  @param BiosVideoPrivate - Pointer to BIOS_VIDEO_DEV structure

  @retval EFI_SUCCESS VBE device found

**/
EFI_STATUS
EFIAPI
BiosVideoCheckForVbe (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate
  );


/**
  Check for VGA device.

  @param BiosVideoPrivate - Pointer to BIOS_VIDEO_DEV structure

  @retval EFI_SUCCESS  Standard VGA device found
**/

EFI_STATUS
EFIAPI
BiosVideoCheckForVga (
  IN OUT BIOS_VIDEO_DEV  *BiosVideoPrivate
  );




/**
  Collect the resource from destroyed bios video device.

  @param BiosVideoPrivate   Video child device private data structure

**/
VOID
BiosVideoDeviceReleaseResource (
  BIOS_VIDEO_DEV  *BiosVideoChildPrivate
  )
;

//
// BIOS Graphics Output Protocol functions
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
  );


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
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL * This,
  IN  UINT32                       ModeNumber
  );


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
  );


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
  );

//
// BIOS VGA Mini Port Protocol functions
//
/**
  VgaMiniPort protocol interface to set mode.

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
  );

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
;

//
// Standard VGA Definitions
//
#define VGA_HORIZONTAL_RESOLUTION                         640
#define VGA_VERTICAL_RESOLUTION                           480
#define VGA_NUMBER_OF_BIT_PLANES                          4
#define VGA_PIXELS_PER_BYTE                               8
#define VGA_BYTES_PER_SCAN_LINE                           (VGA_HORIZONTAL_RESOLUTION / VGA_PIXELS_PER_BYTE)
#define VGA_BYTES_PER_BIT_PLANE                           (VGA_VERTICAL_RESOLUTION * VGA_BYTES_PER_SCAN_LINE)

#define VGA_GRAPHICS_CONTROLLER_ADDRESS_REGISTER          0x3ce
#define VGA_GRAPHICS_CONTROLLER_DATA_REGISTER             0x3cf

#define VGA_GRAPHICS_CONTROLLER_SET_RESET_REGISTER        0x00

#define VGA_GRAPHICS_CONTROLLER_ENABLE_SET_RESET_REGISTER 0x01

#define VGA_GRAPHICS_CONTROLLER_COLOR_COMPARE_REGISTER    0x02

#define VGA_GRAPHICS_CONTROLLER_DATA_ROTATE_REGISTER      0x03
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_REPLACE          0x00
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_AND              0x08
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_OR               0x10
#define VGA_GRAPHICS_CONTROLLER_FUNCTION_XOR              0x18

#define VGA_GRAPHICS_CONTROLLER_READ_MAP_SELECT_REGISTER  0x04

#define VGA_GRAPHICS_CONTROLLER_MODE_REGISTER             0x05
#define VGA_GRAPHICS_CONTROLLER_READ_MODE_0               0x00
#define VGA_GRAPHICS_CONTROLLER_READ_MODE_1               0x08
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_0              0x00
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_1              0x01
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_2              0x02
#define VGA_GRAPHICS_CONTROLLER_WRITE_MODE_3              0x03

#define VGA_GRAPHICS_CONTROLLER_MISCELLANEOUS_REGISTER    0x06

#define VGA_GRAPHICS_CONTROLLER_COLOR_DONT_CARE_REGISTER  0x07

#define VGA_GRAPHICS_CONTROLLER_BIT_MASK_REGISTER         0x08

/**
  Initialize legacy environment for BIOS INI caller.

  @param ThunkContext   the instance pointer of THUNK_CONTEXT
**/
VOID
InitializeBiosIntCaller (
  THUNK_CONTEXT     *ThunkContext
  );

/**
   Initialize interrupt redirection code and entries, because
   IDT Vectors 0x68-0x6f must be redirected to IDT Vectors 0x08-0x0f.
   Or the interrupt will lost when we do thunk.
   NOTE: We do not reset 8259 vector base, because it will cause pending
   interrupt lost.

   @param Legacy8259  Instance pointer for EFI_LEGACY_8259_PROTOCOL.

**/
VOID
InitializeInterruptRedirection (
  IN  EFI_LEGACY_8259_PROTOCOL  *Legacy8259
  );

/**
  Thunk to 16-bit real mode and execute a software interrupt with a vector
  of BiosInt. Regs will contain the 16-bit register context on entry and
  exit.

  @param  This    Protocol instance pointer.
  @param  BiosInt Processor interrupt vector to invoke
  @param  Reg     Register contexted passed into (and returned) from thunk to 16-bit mode

  @retval TRUE   Thunk completed, and there were no BIOS errors in the target code.
                 See Regs for status.
  @retval FALSE  There was a BIOS erro in the target code.
**/
BOOLEAN
EFIAPI
LegacyBiosInt86 (
  IN  BIOS_VIDEO_DEV                 *BiosDev,
  IN  UINT8                           BiosInt,
  IN  IA32_REGISTER_SET           *Regs
  );

#endif
