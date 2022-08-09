/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_DEVICE_MISC_LIB_H
#define OC_DEVICE_MISC_LIB_H

#include <Uefi.h>
#include <Library/OcCpuLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>

/**
  Release UEFI ownership from USB controllers at booting.
**/
EFI_STATUS
ReleaseUsbOwnership (
  VOID
  );

/**
  Reset HDA TCSEL to TC0 state.
**/
VOID
ResetAudioTrafficClass (
  VOID
  );

/**
  Force enables HPET timer.
**/
VOID
ActivateHpetSupport (
  VOID
  );

/**
  Dump CPU info to the specified directory.

  @param[in]  CpuInfo  A pointer to the CPU info.
  @param[in]  Root     Directory to write CPU data.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcCpuInfoDump (
  IN OC_CPU_INFO        *CpuInfo,
  IN EFI_FILE_PROTOCOL  *Root
  );

/**
  Dump PCI info to the specified directory.

  @param[in]  Root     Directory to write PCI info.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcPciInfoDump (
  IN EFI_FILE_PROTOCOL  *Root
  );

/**
  Upgrade UEFI version to 2.x.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcForgeUefiSupport (
  VOID
  );

/**
  Reload Option ROMs.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcReloadOptionRoms (
  VOID
  );

/**
  Valid PCI BAR sizes for PCI 2.0, modern PCI permit up to 8EB.
**/
typedef enum {
  PciBar1MB   =  0,
  PciBar2MB   =  1,
  PciBar4MB   =  2,
  PciBar8MB   =  3,
  PciBar16MB  =  4,
  PciBar32MB  =  5,
  PciBar64MB  =  6,
  PciBar128MB =  7,
  PciBar256MB =  8,
  PciBar512MB =  9,
  PciBar1GB   = 10,
  PciBar2GB   = 11,
  PciBar4GB   = 12,
  PciBar8GB   = 13,
  PciBar16GB  = 14,
  PciBar32GB  = 15,
  PciBar64GB  = 16,
  PciBar128GB = 17,
  PciBar256GB = 18,
  PciBar512GB = 19,

  PciBarMacMax = PciBar1GB,
  PciBarTotal  = PciBar512GB + 1,
} PCI_BAR_SIZE;

/**
  Resize GPU PCIe BARs up to Size.

  Note: this function can increase RBAR size but it currently does not check
  whether the resulting BAR will not overlap other device memory.
  Use with care.

  @param[in]  Size      Maximum BAR size.
  @param[in]  Increase  Increase BAR size if possible.

  @retval EFI_SUCCESS if at least one BAR resized.
**/
EFI_STATUS
ResizeGpuBars (
  IN PCI_BAR_SIZE  Size,
  IN BOOLEAN       Increase
  );

//
// There is no existing structure for this in EDK II.
// MdeModulePkg/.../EhciReg.h struct USB_CLASSC contains exactly the same info, but is not
// intended for general purpose use.
// BaseTools/.../pci22.h struct PCI_DEVICE_INDEPENDENT_REGION contains the same info, but just
// as a UINT8[3] array, and in general where just these three bytes are read, they are either read
// into USB_CLASSC or into a UINT8[3] array (search EDK II for PCI_CLASSCODE_OFFSET for examples).
//
#pragma pack(1)
typedef struct {
  UINT8    ProgInterface;
  UINT8    SubClassCode;
  UINT8    BaseCode;
} PCI_CLASSCODE;
#pragma pack()

#endif // OC_DEVICE_MISC_LIB_H
