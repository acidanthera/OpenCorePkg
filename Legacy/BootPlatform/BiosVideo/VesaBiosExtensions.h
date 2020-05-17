/** @file

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VESA_BIOS_EXTENSIONS_H_
#define _VESA_BIOS_EXTENSIONS_H_

//
// Turn on byte packing of data structures
//
#pragma pack(1)
//
// VESA BIOS Extensions status codes
//
#define VESA_BIOS_EXTENSIONS_STATUS_SUCCESS 0x004f

//
// VESA BIOS Extensions Services
//
#define VESA_BIOS_EXTENSIONS_RETURN_CONTROLLER_INFORMATION  0x4f00

/*++

  Routine Description:
    Function 00 : Return Controller Information

  Arguments:
    Inputs:
      AX    = 0x4f00
      ES:DI = Pointer to buffer to place VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK structure
    Outputs:
      AX    = Return Status

--*/
#define VESA_BIOS_EXTENSIONS_RETURN_MODE_INFORMATION  0x4f01

/*++

  Routine Description:
    Function 01 : Return Mode Information

  Arguments:
    Inputs:
      AX    = 0x4f01
      CX    = Mode Number
      ES:DI = Pointer to buffer to place VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK structure
    Outputs:
      AX    = Return Status

--*/
#define VESA_BIOS_EXTENSIONS_SET_MODE 0x4f02

/*++

  Routine Description:
    Function 02 : Set Mode

  Arguments:
    Inputs:
      AX    = 0x4f02
      BX    = Desired mode to set
        D0-D8   = Mode Number
        D9-D10  = Reserved (must be 0)
        D11     = 0 - Use current default refresh rate
                = 1 - Use user specfieid CRTC values for refresh rate
        D12-D13 = Reserved (must be 0)
        D14     = 0 - Use windowed frame buffer model
                = 1 - Use linear/flat frame buffer model
        D15     = 0 - Clear display memory
                = 1 - Don't clear display memory
      ES:DI = Pointer to buffer to the VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK structure
    Outputs:
      AX    = Return Status

--*/
#define VESA_BIOS_EXTENSIONS_RETURN_CURRENT_MODE  0x4f03

/*++

  Routine Description:
    Function 03 : Return Current Mode

  Arguments:
    Inputs:
      AX    = 0x4f03
    Outputs:
      AX    = Return Status
      BX    = Current mode
        D0-D13  = Mode Number
        D14     = 0 - Windowed frame buffer model
                = 1 - Linear/flat frame buffer model
        D15     = 0 - Memory cleared at last mode set
                = 1 - Memory not cleared at last mode set

--*/
#define VESA_BIOS_EXTENSIONS_SAVE_RESTORE_STATE 0x4f04

/*++

  Routine Description:
    Function 04 : Save/Restore State

  Arguments:
    Inputs:
      AX    = 0x4f03
      DL    = 0x00 - Return Save/Restore State buffer size
            = 0x01 - Save State
            = 0x02 - Restore State
      CX    = Requested Status
        D0  = Save/Restore controller hardware state
        D1  = Save/Restore BIOS data state
        D2  = Save/Restore DAC state
        D3  = Save/Restore Regsiter state
      ES:BX = Pointer to buffer if DL=1 or DL=2
    Outputs:
      AX    = Return Status
      BX    = Number of 64 byte blocks to hold the state buffer if DL=0

--*/
#define VESA_BIOS_EXTENSIONS_EDID  0x4f15

/*++

  Routine Description:
    Function 15 : implement VBE/DDC service

  Arguments:
    Inputs:
      AX    = 0x4f15
      BL    = 0x00 - Report VBE/DDC Capabilities
      CX    = 0x00 - Controller unit number (00 = primary controller)
      ES:DI = Null pointer, must be 0:0 in version 1.0
    Outputs:
      AX    = Return Status
      BH    = Approx. time in seconds, rounded up, to transfer one EDID block(128 bytes)
      BL    = DDC level supported
        D0  = 0 DDC1 not supported
            = 1 DDC1 supported
        D1  = 0 DDC2 not supported
            = 1 DDC2 supported
        D2  = 0 Screen not blanked during data transfer
            = 1 Screen blanked during data transfer

    Inputs:
      AX    = 0x4f15
      BL    = 0x01 - Read EDID
      CX    = 0x00 - Controller unit number (00 = primary controller)
      DX    = 0x00 - EDID block number
      ES:DI = Pointer to buffer in which the EDID block is returned
    Outputs:
      AX    = Return Status
--*/

//
// Timing data from EDID data block
//
#define VESA_BIOS_EXTENSIONS_EDID_BLOCK_SIZE                    128
#define VESA_BIOS_EXTENSIONS_EDID_ESTABLISHED_TIMING_MAX_NUMBER 17

//
// Established Timings: 24 possible resolutions
// Standard Timings: 8 possible resolutions
// Detailed Timings: 4 possible resolutions
//
#define VESA_BIOS_EXTENSIONS_EDID_TIMING_MAX_NUMBER             36

//
// Timing data size for Established Timings, Standard Timings and Detailed Timings
//
#define VESA_BIOS_EXTENSIONS_ESTABLISHED_TIMING_SIZE                  3
#define VESA_BIOS_EXTENSIONS_STANDARD_TIMING_SIZE                     16
#define VESA_BIOS_EXTENSIONS_DETAILED_TIMING_EACH_DESCRIPTOR_SIZE     18
#define VESA_BIOS_EXTENSIONS_DETAILED_TIMING_DESCRIPTOR_MAX_SIZE      72

typedef struct {
  UINT16  HorizontalResolution;
  UINT16  VerticalResolution;
  UINT16  RefreshRate;
} VESA_BIOS_EXTENSIONS_EDID_TIMING;

typedef struct {
  UINT32  ValidNumber;
  UINT32  Key[VESA_BIOS_EXTENSIONS_EDID_TIMING_MAX_NUMBER];
} VESA_BIOS_EXTENSIONS_VALID_EDID_TIMING;

typedef struct {
  UINT8   Header[8];                        //EDID header "00 FF FF FF FF FF FF 00"
  UINT16  ManufactureName;                  //EISA 3-character ID
  UINT16  ProductCode;                      //Vendor assigned code
  UINT32  SerialNumber;                     //32-bit serial number
  UINT8   WeekOfManufacture;                //Week number
  UINT8   YearOfManufacture;                //Year
  UINT8   EdidVersion;                      //EDID Structure Version
  UINT8   EdidRevision;                     //EDID Structure Revision
  UINT8   VideoInputDefinition;
  UINT8   MaxHorizontalImageSize;           //cm
  UINT8   MaxVerticalImageSize;             //cm
  UINT8   DisplayTransferCharacteristic;
  UINT8   FeatureSupport;
  UINT8   RedGreenLowBits;                  //Rx1 Rx0 Ry1 Ry0 Gx1 Gx0 Gy1Gy0
  UINT8   BlueWhiteLowBits;                 //Bx1 Bx0 By1 By0 Wx1 Wx0 Wy1 Wy0
  UINT8   RedX;                             //Red-x Bits 9 - 2
  UINT8   RedY;                             //Red-y Bits 9 - 2
  UINT8   GreenX;                           //Green-x Bits 9 - 2
  UINT8   GreenY;                           //Green-y Bits 9 - 2
  UINT8   BlueX;                            //Blue-x Bits 9 - 2
  UINT8   BlueY;                            //Blue-y Bits 9 - 2
  UINT8   WhiteX;                           //White-x Bits 9 - 2
  UINT8   WhiteY;                           //White-x Bits 9 - 2
  UINT8   EstablishedTimings[VESA_BIOS_EXTENSIONS_ESTABLISHED_TIMING_SIZE];
  UINT8   StandardTimingIdentification[VESA_BIOS_EXTENSIONS_STANDARD_TIMING_SIZE];
  UINT8   DetailedTimingDescriptions[VESA_BIOS_EXTENSIONS_DETAILED_TIMING_DESCRIPTOR_MAX_SIZE];
  UINT8   ExtensionFlag;                    //Number of (optional) 128-byte EDID extension blocks to follow
  UINT8   Checksum;
} VESA_BIOS_EXTENSIONS_EDID_DATA_BLOCK;

//
// Super VGA Information Block
//
typedef struct {
  UINT32  VESASignature;      // 'VESA' 4 byte signature
  UINT16  VESAVersion;        // VBE version number
  UINT32  OEMStringPtr;      // Pointer to OEM string
  UINT32  Capabilities;       // Capabilities of video card
  UINT32  VideoModePtr;      // Pointer to an array of 16-bit supported modes values terminated by 0xFFFF
  UINT16  TotalMemory;        // Number of 64kb memory blocks
  UINT16  OemSoftwareRev;     // VBE implementation Software revision
  UINT32  OemVendorNamePtr;  // VbeFarPtr to Vendor Name String
  UINT32  OemProductNamePtr; // VbeFarPtr to Product Name String
  UINT32  OemProductRevPtr;  // VbeFarPtr to Product Revision String
  UINT8   Reserved[222];      // Reserved for VBE implementation scratch area
  UINT8   OemData[256];       // Data area for OEM strings.  Pad to 512 byte block size
} VESA_BIOS_EXTENSIONS_INFORMATION_BLOCK;

//
// Super VGA Information Block VESASignature values
//
#define VESA_BIOS_EXTENSIONS_VESA_SIGNATURE SIGNATURE_32 ('V', 'E', 'S', 'A')
#define VESA_BIOS_EXTENSIONS_VBE2_SIGNATURE SIGNATURE_32 ('V', 'B', 'E', '2')

//
// Super VGA Information Block VESAVersion values
//
#define VESA_BIOS_EXTENSIONS_VERSION_1_2  0x0102
#define VESA_BIOS_EXTENSIONS_VERSION_2_0  0x0200
#define VESA_BIOS_EXTENSIONS_VERSION_3_0  0x0300

//
// Super VGA Information Block Capabilities field bit definitions
//
#define VESA_BIOS_EXTENSIONS_CAPABILITY_8_BIT_DAC 0x01  // 0: DAC width is fixed at 6 bits/color
// 1: DAC width switchable to 8 bits/color
//
#define VESA_BIOS_EXTENSIONS_CAPABILITY_NOT_VGA 0x02  // 0: Controller is VGA compatible
// 1: Controller is not VGA compatible
//
#define VESA_BIOS_EXTENSIONS_CAPABILITY_NOT_NORMAL_RAMDAC 0x04  // 0: Normal RAMDAC operation
// 1: Use blank bit in function 9 to program RAMDAC
//
#define VESA_BIOS_EXTENSIONS_CAPABILITY_STEREOSCOPIC  0x08  // 0: No hardware stereoscopic signal support
// 1: Hardware stereoscopic signal support
//
#define VESA_BIOS_EXTENSIONS_CAPABILITY_VESA_EVC  0x10  // 0: Stero signaling supported via external VESA stereo connector
// 1: Stero signaling supported via VESA EVC connector
//
// Super VGA mode number bite field definitions
//
#define VESA_BIOS_EXTENSIONS_MODE_NUMBER_VESA 0x0100  // 0: Not a VESA defined VBE mode
// 1: A VESA defined VBE mode
//
#define VESA_BIOS_EXTENSIONS_MODE_NUMBER_REFRESH_CONTROL_USER 0x0800  // 0: Use current BIOS default referesh rate
// 1: Use the user specified CRTC values for refresh rate
//
#define VESA_BIOS_EXTENSIONS_MODE_NUMBER_LINEAR_FRAME_BUFFER  0x4000  // 0: Use a banked/windowed frame buffer
// 1: Use a linear/flat frame buffer
//
#define VESA_BIOS_EXTENSIONS_MODE_NUMBER_PRESERVE_MEMORY  0x8000  // 0: Clear display memory
// 1: Preseve display memory
//
// Super VGA Information Block mode list terminator value
//
#define VESA_BIOS_EXTENSIONS_END_OF_MODE_LIST 0xffff

//
// Window Function
//
typedef
VOID
(*VESA_BIOS_EXTENSIONS_WINDOW_FUNCTION) (
  VOID
  );

//
// Super VGA Mode Information Block
//
typedef struct {
  //
  // Manadory fields for all VESA Bios Extensions revisions
  //
  UINT16                                ModeAttributes;   // Mode attributes
  UINT8                                 WinAAttributes;   // Window A attributes
  UINT8                                 WinBAttributes;   // Window B attributes
  UINT16                                WinGranularity;   // Window granularity in k
  UINT16                                WinSize;          // Window size in k
  UINT16                                WinASegment;      // Window A segment
  UINT16                                WinBSegment;      // Window B segment
  UINT32                                WindowFunction;   // Pointer to window function
  UINT16                                BytesPerScanLine; // Bytes per scanline
  //
  // Manadory fields for VESA Bios Extensions 1.2 and above
  //
  UINT16                                XResolution;          // Horizontal resolution
  UINT16                                YResolution;          // Vertical resolution
  UINT8                                 XCharSize;            // Character cell width
  UINT8                                 YCharSize;            // Character cell height
  UINT8                                 NumberOfPlanes;       // Number of memory planes
  UINT8                                 BitsPerPixel;         // Bits per pixel
  UINT8                                 NumberOfBanks;        // Number of CGA style banks
  UINT8                                 MemoryModel;          // Memory model type
  UINT8                                 BankSize;             // Size of CGA style banks
  UINT8                                 NumberOfImagePages;   // Number of images pages
  UINT8                                 Reserved1;            // Reserved
  UINT8                                 RedMaskSize;          // Size of direct color red mask
  UINT8                                 RedFieldPosition;     // Bit posn of lsb of red mask
  UINT8                                 GreenMaskSize;        // Size of direct color green mask
  UINT8                                 GreenFieldPosition;   // Bit posn of lsb of green mask
  UINT8                                 BlueMaskSize;         // Size of direct color blue mask
  UINT8                                 BlueFieldPosition;    // Bit posn of lsb of blue mask
  UINT8                                 RsvdMaskSize;         // Size of direct color res mask
  UINT8                                 RsvdFieldPosition;    // Bit posn of lsb of res mask
  UINT8                                 DirectColorModeInfo;  // Direct color mode attributes
  //
  // Manadory fields for VESA Bios Extensions 2.0 and above
  //
  UINT32                                PhysBasePtr;  // Physical Address for flat memory frame buffer
  UINT32                                Reserved2;    // Reserved
  UINT16                                Reserved3;    // Reserved
  //
  // Manadory fields for VESA Bios Extensions 3.0 and above
  //
  UINT16                                LinBytesPerScanLine;    // Bytes/scan line for linear modes
  UINT8                                 BnkNumberOfImagePages;  // Number of images for banked modes
  UINT8                                 LinNumberOfImagePages;  // Number of images for linear modes
  UINT8                                 LinRedMaskSize;         // Size of direct color red mask (linear mode)
  UINT8                                 LinRedFieldPosition;    // Bit posiiton of lsb of red mask (linear modes)
  UINT8                                 LinGreenMaskSize;       // Size of direct color green mask (linear mode)
  UINT8                                 LinGreenFieldPosition;  // Bit posiiton of lsb of green mask (linear modes)
  UINT8                                 LinBlueMaskSize;        // Size of direct color blue mask (linear mode)
  UINT8                                 LinBlueFieldPosition;   // Bit posiiton of lsb of blue mask (linear modes)
  UINT8                                 LinRsvdMaskSize;        // Size of direct color reserved mask (linear mode)
  UINT8                                 LinRsvdFieldPosition;   // Bit posiiton of lsb of reserved mask (linear modes)
  UINT32                                MaxPixelClock;          // Maximum pixel clock (in Hz) for graphics mode
  UINT8                                 Pad[190];               // Pad to 256 byte block size
} VESA_BIOS_EXTENSIONS_MODE_INFORMATION_BLOCK;

//
// Super VGA Mode Information Block ModeAttributes field bit definitions
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_HARDWARE  0x0001  // 0: Mode not supported in handware
// 1: Mode supported in handware
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_TTY 0x0004  // 0: TTY Output functions not supported by BIOS
// 1: TTY Output functions supported by BIOS
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_COLOR 0x0008  // 0: Monochrome mode
// 1: Color mode
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_GRAPHICS  0x0010  // 0: Text mode
// 1: Graphics mode
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_NOT_VGA 0x0020  // 0: VGA compatible mode
// 1: Not a VGA compatible mode
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_NOT_WINDOWED  0x0040  // 0: VGA compatible windowed memory mode
// 1: Not a VGA compatible windowed memory mode
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_LINEAR_FRAME_BUFFER 0x0080  // 0: No linear fram buffer mode available
// 1: Linear frame buffer mode available
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_DOUBLE_SCAN 0x0100  // 0: No double scan mode available
// 1: Double scan mode available
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_INTERLACED  0x0200  // 0: No interlaced mode is available
// 1: Interlaced mode is available
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_NO_TRIPPLE_BUFFER 0x0400  // 0: No hardware triple buffer mode support available
// 1: Hardware triple buffer mode support available
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_STEREOSCOPIC  0x0800  // 0: No hardware steroscopic display support
// 1: Hardware steroscopic display support
//
#define VESA_BIOS_EXTENSIONS_MODE_ATTRIBUTE_DUAL_DISPLAY  0x1000  // 0: No dual display start address support
// 1: Dual display start address support
//
// Super VGA Mode Information Block WinAAttribite/WinBAttributes field bit definitions
//
#define VESA_BIOS_EXTENSIONS_WINX_ATTRIBUTE_RELOCATABLE 0x01  // 0: Single non-relocatable window only
// 1: Relocatable window(s) are supported
//
#define VESA_BIOS_EXTENSIONS_WINX_ATTRIBUTE_READABLE  0x02  // 0: Window is not readable
// 1: Window is readable
//
#define VESA_BIOS_EXTENSIONS_WINX_ATTRIBUTE_WRITABLE  0x04  // 0: Window is not writable
// 1: Window is writable
//
// Super VGA Mode Information Block DirectColorMode field bit definitions
//
#define VESA_BIOS_EXTENSIONS_DIRECT_COLOR_MODE_PROG_COLOR_RAMP  0x01  // 0: Color ram is fixed
// 1: Color ramp is programmable
//
#define VESA_BIOS_EXTENSIONS_DIRECT_COLOR_MODE_RSVD_USABLE  0x02  // 0: Bits in Rsvd field are reserved
// 1: Bits in Rsdv field are usable
//
// Super VGA Memory Models
//
typedef enum {
  MemPL = 3,  // Planar memory model
  MemPK = 4,  // Packed pixel memory model
  MemRGB= 6,  // Direct color RGB memory model
  MemYUV= 7   // Direct color YUV memory model
} VESA_BIOS_EXTENSIONS_MEMORY_MODELS;

//
// Super VGA CRTC Information Block
//
typedef struct {
  UINT16  HorizontalTotal;      // Horizontal total in pixels
  UINT16  HorizontalSyncStart;  // Horizontal sync start in pixels
  UINT16  HorizontalSyncEnd;    // Horizontal sync end in pixels
  UINT16  VericalTotal;         // Vertical total in pixels
  UINT16  VericalSyncStart;     // Vertical sync start in pixels
  UINT16  VericalSyncEnd;       // Vertical sync end in pixels
  UINT8   Flags;                // Flags (Interlaced/DoubleScan/etc).
  UINT32  PixelClock;           // Pixel clock in units of Hz
  UINT16  RefreshRate;          // Refresh rate in units of 0.01 Hz
  UINT8   Reserved[40];         // Pad
} VESA_BIOS_EXTENSIONS_CRTC_INFORMATION_BLOCK;

#define VESA_BIOS_EXTENSIONS_CRTC_FLAGS_DOUBLE_SCAN 0x01  // 0: Graphics mode is not souble scanned
// 1: Graphics mode is double scanned
//
#define VESA_BIOS_EXTENSIONS_CRTC_FLAGSINTERLACED 0x02  // 0: Graphics mode is not interlaced
// 1: Graphics mode is interlaced
//
#define VESA_BIOS_EXTENSIONS_CRTC_HORIZONTAL_SYNC_NEGATIVE  0x04  // 0: Horizontal sync polarity is positive(+)
// 0: Horizontal sync polarity is negative(-)
//
#define VESA_BIOS_EXTENSIONS_CRTC_VERITICAL_SYNC_NEGATIVE 0x08  // 0: Verical sync polarity is positive(+)
// 0: Verical sync polarity is negative(-)
//
// Turn off byte packing of data structures
//
#pragma pack()

#endif
