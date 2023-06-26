/** @file
  EFI PCI IO protocol functions declaration for PCI Bus module.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2023, xCuri0. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_PCI_IO_PROTOCOL_H_
#define _EFI_PCI_IO_PROTOCOL_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcMiscLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/CpuIo2.h>
#include <Library/IoLib.h>

#define MAX_IO_PORT_ADDRESS  0xFFFF

EFI_CPU_IO2_PROTOCOL *
InitializeCpuIo2 (
  VOID
  );

/**
  Reads memory-mapped registers.

  The I/O operations are carried out exactly as requested. The caller is responsible
  for satisfying any alignment and I/O width restrictions that a PI System on a
  platform might require. For example on some platforms, width requests of
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will
  be handled by the driver.

  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32,
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for
  each of the Count operations that is performed.

  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16,
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times on the same Address.

  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16,
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times from the first element of Buffer.

  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation.
  @param[in]  Count    The number of I/O operations to perform. The number of
                       bytes moved is Width size * Count, starting at Address.
  @param[out] Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this PI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceRead (
  IN  EFI_CPU_IO2_PROTOCOL       *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64                     Address,
  IN  UINTN                      Count,
  OUT VOID                       *Buffer
  );

/**
  Writes memory-mapped registers.

  The I/O operations are carried out exactly as requested. The caller is responsible
  for satisfying any alignment and I/O width restrictions that a PI System on a
  platform might require. For example on some platforms, width requests of
  EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will
  be handled by the driver.

  If Width is EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, EfiCpuIoWidthUint32,
  or EfiCpuIoWidthUint64, then both Address and Buffer are incremented for
  each of the Count operations that is performed.

  If Width is EfiCpuIoWidthFifoUint8, EfiCpuIoWidthFifoUint16,
  EfiCpuIoWidthFifoUint32, or EfiCpuIoWidthFifoUint64, then only Buffer is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times on the same Address.

  If Width is EfiCpuIoWidthFillUint8, EfiCpuIoWidthFillUint16,
  EfiCpuIoWidthFillUint32, or EfiCpuIoWidthFillUint64, then only Address is
  incremented for each of the Count operations that is performed. The read or
  write operation is performed Count times from the first element of Buffer.

  @param[in]  This     A pointer to the EFI_CPU_IO2_PROTOCOL instance.
  @param[in]  Width    Signifies the width of the I/O or Memory operation.
  @param[in]  Address  The base address of the I/O operation.
  @param[in]  Count    The number of I/O operations to perform. The number of
                       bytes moved is Width size * Count, starting at Address.
  @param[in]  Buffer   For read operations, the destination buffer to store the results.
                       For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS            The data was read from or written to the PI system.
  @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL.
  @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
  @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this PI system.

**/
EFI_STATUS
EFIAPI
CpuMemoryServiceWrite (
  IN EFI_CPU_IO2_PROTOCOL       *This,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  );

/**

  Allow read from memory mapped I/O space.

  @param This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  @param Width    -  The width of memory operation.
  @param Address  -  Base address of the memory operation.
  @param Count    -  Number of memory opeartion to perform.
  @param Buffer   -  The destination buffer to store data.

  @retval EFI_SUCCESS            -  Success.
  @retval EFI_INVALID_PARAMETER  -  Invalid parameter found.
  @retval EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoMemRead (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
;

/**

  Allow write to memory mapped I/O space.

  @param This     -  Pointer to EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL instance.
  @param Width    -  The width of memory operation.
  @param Address  -  Base address of the memory operation.
  @param Count    -  Number of memory opeartion to perform.
  @param Buffer   -  The source buffer to write data from.

  @retval EFI_SUCCESS            -  Success.
  @retval EFI_INVALID_PARAMETER  -  Invalid parameter found.
  @retval EFI_OUT_OF_RESOURCES   -  Fail due to lack of resources.

**/
EFI_STATUS
EFIAPI
RootBridgeIoMemWrite (
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                                 Address,
  IN     UINTN                                  Count,
  IN OUT VOID                                   *Buffer
  )
;

#endif
