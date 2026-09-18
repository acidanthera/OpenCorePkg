/** @file
  Driver implementing the Tiano Legacy 8259 Protocol

Copyright (c) 2005 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _8259_H__
#define _8259_H__

#include <Protocol/Legacy8259.h>
#include <Protocol/PciIo.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Pci.h>

// 8259 Hardware definitions

#define LEGACY_MODE_BASE_VECTOR_MASTER  0x08
#define LEGACY_MODE_BASE_VECTOR_SLAVE   0x70

#define PROTECTED_MODE_BASE_VECTOR_MASTER  0x68
#define PROTECTED_MODE_BASE_VECTOR_SLAVE   0x70

#define LEGACY_8259_CONTROL_REGISTER_MASTER               0x20
#define LEGACY_8259_MASK_REGISTER_MASTER                  0x21
#define LEGACY_8259_CONTROL_REGISTER_SLAVE                0xA0
#define LEGACY_8259_MASK_REGISTER_SLAVE                   0xA1
#define LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_MASTER  0x4D0
#define LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_SLAVE   0x4D1

#define LEGACY_8259_EOI  0x20

// Protocol Function Prototypes

/**
  Sets the base address for the 8259 master and slave PICs.

  @param[in]  This        Indicates the EFI_LEGACY_8259_PROTOCOL instance.
  @param[in]  MasterBase  Interrupt vectors for IRQ0-IRQ7.
  @param[in]  SlaveBase   Interrupt vectors for IRQ8-IRQ15.

  @retval  EFI_SUCCESS       The 8259 PIC was programmed successfully.
  @retval  EFI_DEVICE_ERROR  There was an error while writing to the 8259 PIC.

**/
EFI_STATUS
EFIAPI
Interrupt8259SetVectorBase (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN UINT8                     MasterBase,
  IN UINT8                     SlaveBase
  );

/**
  Gets the current 16-bit real mode and 32-bit protected-mode IRQ masks.

  @param[in]   This                Indicates the EFI_LEGACY_8259_PROTOCOL instance.
  @param[out]  LegacyMask          16-bit mode interrupt mask for IRQ0-IRQ15.
  @param[out]  LegacyEdgeLevel     16-bit mode edge/level mask for IRQ-IRQ15.
  @param[out]  ProtectedMask       32-bit mode interrupt mask for IRQ0-IRQ15.
  @param[out]  ProtectedEdgeLevel  32-bit mode edge/level mask for IRQ0-IRQ15.

  @retval  EFI_SUCCESS       The 8259 PIC was programmed successfully.
  @retval  EFI_DEVICE_ERROR  There was an error while reading the 8259 PIC.

**/
EFI_STATUS
EFIAPI
Interrupt8259GetMask (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  OUT UINT16                    *LegacyMask  OPTIONAL,
  OUT UINT16                    *LegacyEdgeLevel  OPTIONAL,
  OUT UINT16                    *ProtectedMask  OPTIONAL,
  OUT UINT16                    *ProtectedEdgeLevel OPTIONAL
  );

/**
  Sets the current 16-bit real mode and 32-bit protected-mode IRQ masks.

  @param[in]  This                Indicates the EFI_LEGACY_8259_PROTOCOL instance.
  @param[in]  LegacyMask          16-bit mode interrupt mask for IRQ0-IRQ15.
  @param[in]  LegacyEdgeLevel     16-bit mode edge/level mask for IRQ-IRQ15.
  @param[in]  ProtectedMask       32-bit mode interrupt mask for IRQ0-IRQ15.
  @param[in]  ProtectedEdgeLevel  32-bit mode edge/level mask for IRQ0-IRQ15.

  @retval  EFI_SUCCESS       The 8259 PIC was programmed successfully.
  @retval  EFI_DEVICE_ERROR  There was an error while writing the 8259 PIC.

**/
EFI_STATUS
EFIAPI
Interrupt8259SetMask (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN UINT16                    *LegacyMask  OPTIONAL,
  IN UINT16                    *LegacyEdgeLevel  OPTIONAL,
  IN UINT16                    *ProtectedMask  OPTIONAL,
  IN UINT16                    *ProtectedEdgeLevel OPTIONAL
  );

/**
  Sets the mode of the PICs.

  @param[in]  This       Indicates the EFI_LEGACY_8259_PROTOCOL instance.
  @param[in]  Mode       16-bit real or 32-bit protected mode.
  @param[in]  Mask       The value with which to set the interrupt mask.
  @param[in]  EdgeLevel  The value with which to set the edge/level mask.

  @retval  EFI_SUCCESS            The mode was set successfully.
  @retval  EFI_INVALID_PARAMETER  The mode was not set.

**/
EFI_STATUS
EFIAPI
Interrupt8259SetMode (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN EFI_8259_MODE             Mode,
  IN UINT16                    *Mask  OPTIONAL,
  IN UINT16                    *EdgeLevel OPTIONAL
  );

/**
  Translates the IRQ into a vector.

  @param[in]   This    Indicates the EFI_LEGACY_8259_PROTOCOL instance.
  @param[in]   Irq     IRQ0-IRQ15.
  @param[out]  Vector  The vector that is assigned to the IRQ.

  @retval  EFI_SUCCESS            The Vector that matches Irq was returned.
  @retval  EFI_INVALID_PARAMETER  Irq is not valid.

**/
EFI_STATUS
EFIAPI
Interrupt8259GetVector (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_8259_IRQ              Irq,
  OUT UINT8                     *Vector
  );

/**
  Enables the specified IRQ.

  @param[in]  This            Indicates the EFI_LEGACY_8259_PROTOCOL instance.
  @param[in]  Irq             IRQ0-IRQ15.
  @param[in]  LevelTriggered  0 = Edge triggered; 1 = Level triggered.

  @retval  EFI_SUCCESS            The Irq was enabled on the 8259 PIC.
  @retval  EFI_INVALID_PARAMETER  The Irq is not valid.

**/
EFI_STATUS
EFIAPI
Interrupt8259EnableIrq (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN EFI_8259_IRQ              Irq,
  IN BOOLEAN                   LevelTriggered
  );

/**
  Disables the specified IRQ.

  @param[in]  This  Indicates the EFI_LEGACY_8259_PROTOCOL instance.
  @param[in]  Irq   IRQ0-IRQ15.

  @retval  EFI_SUCCESS            The Irq was disabled on the 8259 PIC.
  @retval  EFI_INVALID_PARAMETER  The Irq is not valid.

**/
EFI_STATUS
EFIAPI
Interrupt8259DisableIrq (
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN EFI_8259_IRQ              Irq
  );

/**
  Reads the PCI configuration space to get the interrupt number that is assigned to the card.

  @param[in]   This       Indicates the EFI_LEGACY_8259_PROTOCOL instance.
  @param[in]   PciHandle  PCI function for which to return the vector.
  @param[out]  Vector     IRQ number that corresponds to the interrupt line.

  @retval  EFI_SUCCESS  The interrupt line value was read successfully.

**/
EFI_STATUS
EFIAPI
Interrupt8259GetInterruptLine (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_HANDLE                PciHandle,
  OUT UINT8                     *Vector
  );

/**
  Issues the End of Interrupt (EOI) commands to PICs.

  @param[in]  This  Indicates the EFI_LEGACY_8259_PROTOCOL instance.
  @param[in]  Irq   The interrupt for which to issue the EOI command.

  @retval  EFI_SUCCESS            The EOI command was issued.
  @retval  EFI_INVALID_PARAMETER  The Irq is not valid.

**/
EFI_STATUS
EFIAPI
Interrupt8259EndOfInterrupt (
  IN  EFI_LEGACY_8259_PROTOCOL  *This,
  IN  EFI_8259_IRQ              Irq
  );

#endif
