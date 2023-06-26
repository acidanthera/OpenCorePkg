/** @file
   EFI PCI IO protocol functions implementation for PCI Bus module.

   Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
   Copyright (c) 2023, xCuri0. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include "OcPciIoU.h"

STATIC EFI_CPU_IO2_PROTOCOL  *mCpuIo = NULL;

STATIC CONST UINT8  mInStride[] = {
  1,      // EfiCpuIoWidthUint8
  2,      // EfiCpuIoWidthUint16
  4,      // EfiCpuIoWidthUint32
  8,      // EfiCpuIoWidthUint64
  0,      // EfiCpuIoWidthFifoUint8
  0,      // EfiCpuIoWidthFifoUint16
  0,      // EfiCpuIoWidthFifoUint32
  0,      // EfiCpuIoWidthFifoUint64
  1,      // EfiCpuIoWidthFillUint8
  2,      // EfiCpuIoWidthFillUint16
  4,      // EfiCpuIoWidthFillUint32
  8       // EfiCpuIoWidthFillUint64
};

//
// Lookup table for increment values based on transfer widths
//
STATIC CONST UINT8  mOutStride[] = {
  1,      // EfiCpuIoWidthUint8
  2,      // EfiCpuIoWidthUint16
  4,      // EfiCpuIoWidthUint32
  8,      // EfiCpuIoWidthUint64
  1,      // EfiCpuIoWidthFifoUint8
  2,      // EfiCpuIoWidthFifoUint16
  4,      // EfiCpuIoWidthFifoUint32
  8,      // EfiCpuIoWidthFifoUint64
  0,      // EfiCpuIoWidthFillUint8
  0,      // EfiCpuIoWidthFillUint16
  0,      // EfiCpuIoWidthFillUint32
  0       // EfiCpuIoWidthFillUint64
};

EFI_CPU_IO2_PROTOCOL *
InitializeCpuIo2 (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mCpuIo != NULL) {
    return mCpuIo;
  }

  Status = gBS->LocateProtocol (
                  &gEfiCpuIo2ProtocolGuid,
                  NULL,
                  (VOID **)&mCpuIo
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return mCpuIo;
}

/**
   Check parameters to a CPU I/O 2 Protocol service request.

   The I/O operations are carried out exactly as requested. The caller is responsible
   for satisfying any alignment and I/O width restrictions that a PI System on a
   platform might require. For example on some platforms, width requests of
   EfiCpuIoWidthUint64 do not work. Misaligned buffers, on the other hand, will
   be handled by the driver.

   @param[in] MmioOperation  TRUE for an MMIO operation, FALSE for I/O Port operation.
   @param[in] Width          Signifies the width of the I/O or Memory operation.
   @param[in] Address        The base address of the I/O operation.
   @param[in] Count          The number of I/O operations to perform. The number of
                            bytes moved is Width size * Count, starting at Address.
   @param[in] Buffer         For read operations, the destination buffer to store the results.
                            For write operations, the source buffer from which to write data.

   @retval EFI_SUCCESS            The parameters for this request pass the checks.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PI system.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_UNSUPPORTED        The Buffer is not aligned for the given Width.
   @retval EFI_UNSUPPORTED        The address range specified by Address, Width,
                                 and Count is not valid for this PI system.

 **/
EFI_STATUS
CpuIoCheckParameter (
  IN BOOLEAN                    MmioOperation,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  IN VOID                       *Buffer
  )
{
  UINT64  MaxCount;
  UINT64  Limit;

  //
  // Check to see if Buffer is NULL
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if Width is in the valid range
  //
  if ((UINT32)Width >= EfiCpuIoWidthMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // For FIFO type, the target address won't increase during the access,
  // so treat Count as 1
  //
  if ((Width >= EfiCpuIoWidthFifoUint8) && (Width <= EfiCpuIoWidthFifoUint64)) {
    Count = 1;
  }

  //
  // Check to see if Width is in the valid range for I/O Port operations
  //
  Width = (EFI_CPU_IO_PROTOCOL_WIDTH)(Width & 0x03);
  if (!MmioOperation && (Width == EfiCpuIoWidthUint64)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check to see if Address is aligned
  //
  if ((Address & ((UINT64)mInStride[Width] - 1)) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check to see if any address associated with this transfer exceeds the maximum
  // allowed address.  The maximum address implied by the parameters passed in is
  // Address + Size * Count.  If the following condition is met, then the transfer
  // is not supported.
  //
  //    Address + Size * Count > (MmioOperation ? MAX_ADDRESS : MAX_IO_PORT_ADDRESS) + 1
  //
  // Since MAX_ADDRESS can be the maximum integer value supported by the CPU and Count
  // can also be the maximum integer value supported by the CPU, this range
  // check must be adjusted to avoid all oveflow conditions.
  //
  // The following form of the range check is equivalent but assumes that
  // MAX_ADDRESS and MAX_IO_PORT_ADDRESS are of the form (2^n - 1).
  //
  Limit = (MmioOperation ? MAX_ADDRESS : MAX_IO_PORT_ADDRESS);
  if (Count == 0) {
    if (Address > Limit) {
      return EFI_UNSUPPORTED;
    }
  } else {
    MaxCount = RShiftU64 (Limit, Width);
    if (MaxCount < (Count - 1)) {
      return EFI_UNSUPPORTED;
    }

    if (Address > LShiftU64 (MaxCount - Count + 1, Width)) {
      return EFI_UNSUPPORTED;
    }
  }

  //
  // Check to see if Buffer is aligned
  // (IA-32 allows UINT64 and INT64 data types to be 32-bit aligned.)
  //
  if (((UINTN)Buffer & ((MIN (sizeof (UINTN), mInStride[Width])  - 1))) != 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

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
  IN EFI_CPU_IO2_PROTOCOL       *This,
  IN EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                     Address,
  IN UINTN                      Count,
  OUT VOID                      *Buffer
  )
{
  EFI_STATUS                 Status;
  UINT8                      InStride;
  UINT8                      OutStride;
  EFI_CPU_IO_PROTOCOL_WIDTH  OperationWidth;
  UINT8                      *Uint8Buffer;

  Status = CpuIoCheckParameter (TRUE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_CPU_IO_PROTOCOL_WIDTH)(Width & 0x03);
  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiCpuIoWidthUint8) {
      *((UINT8 *)Uint8Buffer) = *((UINT8 *)(UINTN)Address);
    } else if (OperationWidth == EfiCpuIoWidthUint16) {
      WriteUnaligned16 ((VOID *)Uint8Buffer, ReadUnaligned16 ((VOID *)(UINTN)Address));
    } else if (OperationWidth == EfiCpuIoWidthUint32) {
      WriteUnaligned32 ((VOID *)Uint8Buffer, ReadUnaligned32 ((VOID *)(UINTN)Address));
    } else if (OperationWidth == EfiCpuIoWidthUint64) {
      WriteUnaligned64 ((VOID *)Uint8Buffer, ReadUnaligned64 ((VOID *)(UINTN)Address));
    }
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                 Status;
  UINT8                      InStride;
  UINT8                      OutStride;
  EFI_CPU_IO_PROTOCOL_WIDTH  OperationWidth;
  UINT8                      *Uint8Buffer;

  Status = CpuIoCheckParameter (TRUE, Width, Address, Count, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Select loop based on the width of the transfer
  //
  InStride       = mInStride[Width];
  OutStride      = mOutStride[Width];
  OperationWidth = (EFI_CPU_IO_PROTOCOL_WIDTH)(Width & 0x03);
  for (Uint8Buffer = Buffer; Count > 0; Address += InStride, Uint8Buffer += OutStride, Count--) {
    if (OperationWidth == EfiCpuIoWidthUint8) {
      *((UINT8 *)(UINTN)Address) = *((UINT8 *)Uint8Buffer);
    } else if (OperationWidth == EfiCpuIoWidthUint16) {
      WriteUnaligned16 ((VOID *)(UINTN)Address, ReadUnaligned16 ((VOID *)Uint8Buffer));
    } else if (OperationWidth == EfiCpuIoWidthUint32) {
      WriteUnaligned32 ((VOID *)(UINTN)Address, ReadUnaligned32 ((VOID *)Uint8Buffer));
    } else if (OperationWidth == EfiCpuIoWidthUint64) {
      WriteUnaligned64 ((VOID *)(UINTN)Address, ReadUnaligned64 ((VOID *)Uint8Buffer));
    }
  }

  return EFI_SUCCESS;
}

/**
   Enables a PCI driver to access PCI controller registers in the PCI root
   bridge memory space.

   The Mem.Read(), and Mem.Write() functions enable a driver to access PCI
   controller registers in the PCI root bridge memory space.
   The memory operations are carried out exactly as requested. The caller is
   responsible for satisfying any alignment and memory width restrictions that a
   PCI Root Bridge on a platform might require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operation.
   @param[in]   Address   The base address of the memory operation. The caller
                         is responsible for aligning the Address if required.
   @param[in]   Count     The number of memory operations to perform. Bytes
                         moved is Width size * Count, starting at Address.
   @param[out]  Buffer    For read operations, the destination buffer to store
                         the results. For write operations, the source buffer
                         to write data from.

   @retval EFI_SUCCESS            The data was read from or written to the PCI
                                 root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a
                                 lack of resources.
 **/
EFI_STATUS
EFIAPI
RootBridgeIoMemRead (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 Address,
  IN UINTN                                  Count,
  OUT VOID                                  *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Width < 0) || (Width >= EfiPciWidthMaximum)) {
    return EFI_INVALID_PARAMETER;
  }

  // Don't check check memory access limit because firmware data is incorrect
  return mCpuIo->Mem.Read (
                       mCpuIo,
                       (EFI_CPU_IO_PROTOCOL_WIDTH)Width,
                       Address,
                       Count,
                       Buffer
                       );
}

/**
   Enables a PCI driver to access PCI controller registers in the PCI root
   bridge memory space.

   The Mem.Read(), and Mem.Write() functions enable a driver to access PCI
   controller registers in the PCI root bridge memory space.
   The memory operations are carried out exactly as requested. The caller is
   responsible for satisfying any alignment and memory width restrictions that a
   PCI Root Bridge on a platform might require.

   @param[in]   This      A pointer to the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
   @param[in]   Width     Signifies the width of the memory operation.
   @param[in]   Address   The base address of the memory operation. The caller
                         is responsible for aligning the Address if required.
   @param[in]   Count     The number of memory operations to perform. Bytes
                         moved is Width size * Count, starting at Address.
   @param[in]   Buffer    For read operations, the destination buffer to store
                         the results. For write operations, the source buffer
                         to write data from.

   @retval EFI_SUCCESS            The data was read from or written to the PCI
                                 root bridge.
   @retval EFI_INVALID_PARAMETER  Width is invalid for this PCI root bridge.
   @retval EFI_INVALID_PARAMETER  Buffer is NULL.
   @retval EFI_OUT_OF_RESOURCES   The request could not be completed due to a
                                 lack of resources.
 **/
EFI_STATUS
EFIAPI
RootBridgeIoMemWrite (
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL        *This,
  IN EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN UINT64                                 Address,
  IN UINTN                                  Count,
  IN VOID                                   *Buffer
  )
{
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Width < 0) || (Width >= EfiPciWidthMaximum)) {
    return EFI_INVALID_PARAMETER;
  }

  // Don't check check memory access limit because firmware data is incorrect
  return mCpuIo->Mem.Write (
                       mCpuIo,
                       (EFI_CPU_IO_PROTOCOL_WIDTH)Width,
                       Address,
                       Count,
                       Buffer
                       );
}
