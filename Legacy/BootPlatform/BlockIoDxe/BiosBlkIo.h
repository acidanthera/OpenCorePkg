/** @file

Copyright (c) 1999 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BIOS_BLOCK_IO_H_
#define _BIOS_BLOCK_IO_H_

#include <Uefi.h>

#include <Protocol/BlockIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/Legacy8259.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/DevicePath.h>
#include <Guid/LegacyBios.h>
#include <Guid/BlockIoVendor.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include <IndustryStandard/Pci.h>

#include "Edd.h"

#define LEGACY_REGION_BASE  0x0C0000

//
// Global Variables
//
extern EFI_COMPONENT_NAME_PROTOCOL   gBiosBlockIoComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gBiosBlockIoComponentName2;

//
// Define the I2O class code
//
#define PCI_BASE_CLASS_INTELLIGENT  0x0e
#define PCI_SUB_CLASS_INTELLIGENT   0x00

//
// Number of pages needed for our buffer under 1MB
//
#define BLOCK_IO_BUFFER_PAGE_SIZE  (((sizeof (EDD_DEVICE_ADDRESS_PACKET) + sizeof (BIOS_LEGACY_DRIVE) + MAX_EDD11_XFER) / EFI_PAGE_SIZE) + 1\
        )

//
// Driver Binding Protocol functions
//

/**
  Check whether the driver supports this device.

  @param  This                   The Udriver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
BiosBlockIoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Starts the device with this driver.

  @param  This                   The driver binding instance.
  @param  Controller             Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The controller is controlled by the driver.
  @retval Other                  This controller cannot be started.

**/
EFI_STATUS
EFIAPI
BiosBlockIoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop the device handled by this driver.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
BiosBlockIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

//
// Other internal functions
//

/**
  Build device path for EDD 3.0.

  @param  BaseDevicePath         Base device path.
  @param  Drive                  Legacy drive.
  @param  DevicePath             Device path for output.

  @retval EFI_SUCCESS            The device path is built successfully.
  @retval EFI_UNSUPPORTED        It is failed to built device path.

**/
EFI_STATUS
BuildEdd30DevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *BaseDevicePath,
  IN  BIOS_LEGACY_DRIVE         *Drive,
  IN  EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  );

/**
  Initialize block I/O device instance

  @param  Dev   Instance of block I/O device instance

  @retval TRUE  Initialization succeeds.
  @retval FALSE Initialization fails.

**/
BOOLEAN
BiosInitBlockIo (
  IN  BIOS_BLOCK_IO_DEV  *Dev
  );

/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
Edd30BiosReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
Edd30BiosWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

/**
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writting back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
BiosBlockIoFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This
  );

/**
  Reset the Block Device.

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
BiosBlockIoReset (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  BOOLEAN                ExtendedVerification
  );

/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
Edd11BiosReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
Edd11BiosWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
BiosReadLegacyDrive (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
BiosWriteLegacyDrive (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

/**
  Gets parameters of block I/O device.

  @param  BiosBlockIoDev Instance of block I/O device.
  @param  Drive          Legacy drive.

  @return  Result of device parameter retrieval.

**/
UINTN
Int13GetDeviceParameters (
  IN BIOS_BLOCK_IO_DEV  *BiosBlockIoDev,
  IN BIOS_LEGACY_DRIVE  *Drive
  );

/**
  Extension of INT13 call.

  @param  BiosBlockIoDev Instance of block I/O device.
  @param  Drive          Legacy drive.

  @return  Result of this extension.

**/
UINTN
Int13Extensions (
  IN BIOS_BLOCK_IO_DEV  *BiosBlockIoDev,
  IN BIOS_LEGACY_DRIVE  *Drive
  );

/**
  Gets parameters of legacy drive.

  @param  BiosBlockIoDev Instance of block I/O device.
  @param  Drive          Legacy drive.

  @return  Result of drive parameter retrieval.

**/
UINTN
GetDriveParameters (
  IN BIOS_BLOCK_IO_DEV   *BiosBlockIoDev,
  IN  BIOS_LEGACY_DRIVE  *Drive
  );

/**
  Build device path for device.

  @param  BaseDevicePath         Base device path.
  @param  Drive                  Legacy drive.
  @param  DevicePath             Device path for output.

**/
VOID
SetBiosInitBlockIoDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *BaseDevicePath,
  IN  BIOS_LEGACY_DRIVE         *Drive,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  );

/**
  Initialize legacy environment for BIOS INI caller.

  @param ThunkContext   the instance pointer of THUNK_CONTEXT
**/
VOID
InitializeBiosIntCaller (
  THUNK_CONTEXT  *ThunkContext
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
  IN  BIOS_BLOCK_IO_DEV  *BiosDev,
  IN  UINT8              BiosInt,
  IN  IA32_REGISTER_SET  *Regs
  );

#endif
