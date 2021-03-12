/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CpuIO.h

Abstract:

  CPU IO Protocol as defined in Tiano

  This code abstracts the CPU IO Protocol

--*/

#ifndef _CPUIO_H_
#define _CPUIO_H_

#define EFI_CPU_IO_PROTOCOL_GUID \
  { \
    0xB0732526, 0x38C8, 0x4b40, { 0x88, 0x77, 0x61, 0xC7, 0xB0, 0x6A, 0xAC, 0x45 } \
  }

typedef struct EFI_CPU_IO_PROTOCOL_ EFI_CPU_IO_PROTOCOL;

//
// *******************************************************
// EFI_CPU_IO_PROTOCOL_WIDTH
// *******************************************************
//
typedef enum {
  EfiCpuIoWidthUint8,
  EfiCpuIoWidthUint16,
  EfiCpuIoWidthUint32,
  EfiCpuIoWidthUint64,
  EfiCpuIoWidthFifoUint8,
  EfiCpuIoWidthFifoUint16,
  EfiCpuIoWidthFifoUint32,
  EfiCpuIoWidthFifoUint64,
  EfiCpuIoWidthFillUint8,
  EfiCpuIoWidthFillUint16,
  EfiCpuIoWidthFillUint32,
  EfiCpuIoWidthFillUint64,
  EfiCpuIoWidthMaximum
} EFI_CPU_IO_PROTOCOL_WIDTH;

//
// *******************************************************
// EFI_CPU_IO_PROTOCOL_IO_MEM
// *******************************************************
//
typedef
EFI_STATUS
EFIAPI
(EFIAPI *EFI_CPU_IO_PROTOCOL_IO_MEM) (
  IN EFI_CPU_IO_PROTOCOL                * This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

//
// *******************************************************
// EFI_CPU_IO_PROTOCOL_ACCESS
// *******************************************************
//
typedef struct {
  EFI_CPU_IO_PROTOCOL_IO_MEM  Read;
  EFI_CPU_IO_PROTOCOL_IO_MEM  Write;
} EFI_CPU_IO_PROTOCOL_ACCESS;

//
// *******************************************************
// EFI_CPU_IO_PROTOCOL
// *******************************************************
//
typedef struct EFI_CPU_IO_PROTOCOL_ {
  EFI_CPU_IO_PROTOCOL_ACCESS  Mem;
  EFI_CPU_IO_PROTOCOL_ACCESS  Io;
} EFI_CPU_IO_PROTOCOL;

extern EFI_GUID gEfiCpuIoProtocolGuid;

#endif
