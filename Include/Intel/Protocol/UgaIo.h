/** @file
  UGA IO protocol from the EFI 1.10 specification.

  Abstraction of a very simple graphics device.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UGA_IO_H__
#define __UGA_IO_H__

#define EFI_UGA_IO_PROTOCOL_GUID \
  { 0x61a4d49e, 0x6f68, 0x4f1b, { 0xb9, 0x22, 0xa8, 0x6e, 0xed, 0xb, 0x7, 0xa2 } }

typedef struct _EFI_UGA_IO_PROTOCOL EFI_UGA_IO_PROTOCOL;

typedef UINT32 UGA_STATUS;

typedef enum {
  UgaDtParentBus = 1,
  UgaDtGraphicsController,
  UgaDtOutputController,
  UgaDtOutputPort,
  UgaDtOther
} UGA_DEVICE_TYPE, *PUGA_DEVICE_TYPE;

typedef UINT32 UGA_DEVICE_ID, *PUGA_DEVICE_ID;

typedef struct {
  UGA_DEVICE_TYPE    deviceType;
  UGA_DEVICE_ID      deviceId;
  UINT32             ui32DeviceContextSize;
  UINT32             ui32SharedContextSize;
} UGA_DEVICE_DATA, *PUGA_DEVICE_DATA;

typedef struct _UGA_DEVICE {
  VOID                  *pvDeviceContext;
  VOID                  *pvSharedContext;
  VOID                  *pvRunTimeContext;
  struct _UGA_DEVICE    *pParentDevice;
  VOID                  *pvBusIoServices;
  VOID                  *pvStdIoServices;
  UGA_DEVICE_DATA       deviceData;
} UGA_DEVICE, *PUGA_DEVICE;

typedef enum {
  UgaIoGetVersion = 1,
  UgaIoGetChildDevice,
  UgaIoStartDevice,
  UgaIoStopDevice,
  UgaIoFlushDevice,
  UgaIoResetDevice,
  UgaIoGetDeviceState,
  UgaIoSetDeviceState,
  UgaIoSetPowerState,
  UgaIoGetMemoryConfiguration,
  UgaIoSetVideoMode,
  UgaIoCopyRectangle,
  UgaIoGetEdidSegment,
  UgaIoDeviceChannelOpen,
  UgaIoDeviceChannelClose,
  UgaIoDeviceChannelRead,
  UgaIoDeviceChannelWrite,
  UgaIoGetPersistentDataSize,
  UgaIoGetPersistentData,
  UgaIoSetPersistentData,
  UgaIoGetDevicePropertySize,
  UgaIoGetDeviceProperty,
  UgaIoBtPrivateInterface
} UGA_IO_REQUEST_CODE, *PUGA_IO_REQUEST_CODE;

typedef struct {
  IN UGA_IO_REQUEST_CODE    ioRequestCode;
  IN VOID                   *pvInBuffer;
  IN UINT64                 ui64InBufferSize;
  OUT VOID                  *pvOutBuffer;
  IN UINT64                 ui64OutBufferSize;
  OUT UINT64                ui64BytesReturned;
} UGA_IO_REQUEST, *PUGA_IO_REQUEST;

/**
  Dynamically allocate storage for a child UGA_DEVICE.

  @param[in]     This            The EFI_UGA_IO_PROTOCOL instance.
  @param[in]     ParentDevice    ParentDevice specifies a pointer to the parent device of Device.
  @param[in]     DeviceData      A pointer to UGA_DEVICE_DATA returned from a call to DispatchService()
                                 with a UGA_DEVICE of Parent and an IoRequest of type UgaIoGetChildDevice.
  @param[in]     RunTimeContext  Context to associate with Device.
  @param[out]    Device          The Device returns a dynamically allocated child UGA_DEVICE object
                                 for ParentDevice. The caller is responsible for deleting Device.


  @retval  EFI_SUCCESS           Device was returned.
  @retval  EFI_INVALID_PARAMETER One of the arguments was not valid.
  @retval  EFI_DEVICE_ERROR      The device had an error and could not complete the request.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UGA_IO_PROTOCOL_CREATE_DEVICE)(
  IN  EFI_UGA_IO_PROTOCOL  *This,
  IN  UGA_DEVICE           *ParentDevice,
  IN  UGA_DEVICE_DATA      *DeviceData,
  IN  VOID                 *RunTimeContext,
  OUT UGA_DEVICE           **Device
  );

/**
  Delete a dynamically allocated child UGA_DEVICE object that was allocated via CreateDevice().

  @param[in]     This            The EFI_UGA_IO_PROTOCOL instance. Type EFI_UGA_IO_PROTOCOL is
                                 defined in Section 10.7.
  @param[in]     Device          The Device points to a UGA_DEVICE object that was dynamically
                                 allocated via a CreateDevice() call.


  @retval  EFI_SUCCESS           Device was returned.
  @retval  EFI_INVALID_PARAMETER The Device was not allocated via CreateDevice().

**/
typedef
EFI_STATUS
(EFIAPI *EFI_UGA_IO_PROTOCOL_DELETE_DEVICE)(
  IN EFI_UGA_IO_PROTOCOL  *This,
  IN UGA_DEVICE           *Device
  );

/**
  This is the main UGA service dispatch routine for all UGA_IO_REQUEST s.

  @param pDevice pDevice specifies a pointer to a device object associated with a
                 device enumerated by a pIoRequest->ioRequestCode of type
                 UgaIoGetChildDevice. The root device for the EFI_UGA_IO_PROTOCOL
                 is represented by pDevice being set to NULL.

  @param pIoRequest
                 pIoRequest points to a caller allocated buffer that contains data
                 defined by pIoRequest->ioRequestCode. See Related Definitions for
                 a definition of UGA_IO_REQUEST_CODE s and their associated data
                 structures.

  @return UGA_STATUS

**/
typedef UGA_STATUS
(EFIAPI *PUGA_FW_SERVICE_DISPATCH)(
  IN PUGA_DEVICE pDevice,
  IN OUT PUGA_IO_REQUEST pIoRequest
  );

///
/// Provides a basic abstraction to send I/O requests to the graphics device and any of its children.
///
struct _EFI_UGA_IO_PROTOCOL {
  EFI_UGA_IO_PROTOCOL_CREATE_DEVICE    CreateDevice;
  EFI_UGA_IO_PROTOCOL_DELETE_DEVICE    DeleteDevice;
  PUGA_FW_SERVICE_DISPATCH             DispatchService;
};

extern EFI_GUID  gEfiUgaIoProtocolGuid;

//
// Data structure that is stored in the EFI Configuration Table with the
// EFI_UGA_IO_PROTOCOL_GUID.  The option ROMs listed in this table may have
// EBC UGA drivers.
//
typedef struct {
  UINT32    Version;
  UINT32    HeaderSize;
  UINT32    SizeOfEntries;
  UINT32    NumberOfEntries;
} EFI_DRIVER_OS_HANDOFF_HEADER;

typedef enum {
  EfiUgaDriverFromPciRom,
  EfiUgaDriverFromSystem,
  EfiDriverHandoffMax
} EFI_DRIVER_HANOFF_ENUM;

typedef struct {
  EFI_DRIVER_HANOFF_ENUM      Type;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  VOID                        *PciRomImage;
  UINT64                      PciRomSize;
} EFI_DRIVER_OS_HANDOFF;

#endif
