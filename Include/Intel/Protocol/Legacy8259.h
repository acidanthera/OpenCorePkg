/** @file
  This protocol abstracts the 8259 interrupt controller. This includes
  PCI IRQ routing needed to program the PCI Interrupt Line register.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This protocol is defined in Framework for EFI Compatibility Support Module spec
  Version 0.97.

**/

#ifndef _EFI_LEGACY_8259_H_
#define _EFI_LEGACY_8259_H_

#define EFI_LEGACY_8259_PROTOCOL_GUID \
  { \
    0x38321dba, 0x4fe0, 0x4e17, {0x8a, 0xec, 0x41, 0x30, 0x55, 0xea, 0xed, 0xc1 } \
  }

typedef struct _EFI_LEGACY_8259_PROTOCOL EFI_LEGACY_8259_PROTOCOL;

typedef enum {
  Efi8259Irq0,
  Efi8259Irq1,
  Efi8259Irq2,
  Efi8259Irq3,
  Efi8259Irq4,
  Efi8259Irq5,
  Efi8259Irq6,
  Efi8259Irq7,
  Efi8259Irq8,
  Efi8259Irq9,
  Efi8259Irq10,
  Efi8259Irq11,
  Efi8259Irq12,
  Efi8259Irq13,
  Efi8259Irq14,
  Efi8259Irq15,
  Efi8259IrqMax
} EFI_8259_IRQ;

typedef enum {
  Efi8259LegacyMode,
  Efi8259ProtectedMode,
  Efi8259MaxMode
} EFI_8259_MODE;

/**
  Get the 8259 interrupt masks for Irq0 - Irq15. A different mask exists for
  the legacy mode mask and the protected mode mask. The base address for the 8259
  is different for legacy and protected mode, so two masks are required.

  @param  This                  The protocol instance pointer.
  @param  MasterBase            The base vector for the Master PIC in the 8259 controller.
  @param  SlaveBase             The base vector for the Slave PIC in the 8259 controller.

  @retval EFI_SUCCESS           The new bases were programmed.
  @retval EFI_DEVICE_ERROR      A device error occurred programming the vector bases.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_8259_SET_VECTOR_BASE)(
  IN EFI_LEGACY_8259_PROTOCOL           *This,
  IN  UINT8                             MasterBase,
  IN  UINT8                             SlaveBase
  );

/**
  Get the 8259 interrupt masks for Irq0 - Irq15. A different mask exists for
  the legacy mode mask and the protected mode mask. The base address for the 8259
  is different for legacy and protected mode, so two masks are required.

  @param  This                  The protocol instance pointer.
  @param  LegacyMask            Bit 0 is Irq0 - Bit 15 is Irq15.
  @param  LegacyEdgeLevel       Bit 0 is Irq0 - Bit 15 is Irq15.
  @param  ProtectedMask         Bit 0 is Irq0 - Bit 15 is Irq15.
  @param  ProtectedEdgeLevel    Bit 0 is Irq0 - Bit 15 is Irq15.

  @retval EFI_SUCCESS           8259 status returned.
  @retval EFI_DEVICE_ERROR      Error reading 8259.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_8259_GET_MASK)(
  IN EFI_LEGACY_8259_PROTOCOL           *This,
  OUT UINT16                            *LegacyMask  OPTIONAL,
  OUT UINT16                            *LegacyEdgeLevel  OPTIONAL,
  OUT UINT16                            *ProtectedMask  OPTIONAL,
  OUT UINT16                            *ProtectedEdgeLevel OPTIONAL
  );

/**
  Set the 8259 interrupt masks for Irq0 - Irq15. A different mask exists for
  the legacy mode mask and the protected mode mask. The base address for the 8259
  is different for legacy and protected mode, so two masks are required.
  Also set the edge/level masks.

  @param  This                  The protocol instance pointer.
  @param  LegacyMask            Bit 0 is Irq0 - Bit 15 is Irq15.
  @param  LegacyEdgeLevel       Bit 0 is Irq0 - Bit 15 is Irq15.
  @param  ProtectedMask         Bit 0 is Irq0 - Bit 15 is Irq15.
  @param  ProtectedEdgeLevel    Bit 0 is Irq0 - Bit 15 is Irq15.

  @retval EFI_SUCCESS           8259 status returned.
  @retval EFI_DEVICE_ERROR      Error writing 8259.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_8259_SET_MASK)(
  IN EFI_LEGACY_8259_PROTOCOL           *This,
  IN  UINT16                            *LegacyMask  OPTIONAL,
  IN  UINT16                            *LegacyEdgeLevel  OPTIONAL,
  IN  UINT16                            *ProtectedMask  OPTIONAL,
  IN  UINT16                            *ProtectedEdgeLevel OPTIONAL
  );

/**
  Set the 8259 mode of operation. The base address for the 8259 is different for
  legacy and protected mode. The legacy mode requires the master 8259 to have a
  master base of 0x08 and the slave base of 0x70. The protected mode base locations
  are not defined. Interrupts must be masked by the caller before this function
  is called. The interrupt mask from the current mode is saved. The interrupt
  mask for the new mode is Mask, or if Mask does not exist the previously saved
  mask is used.

  @param  This                  The protocol instance pointer.
  @param  Mode                  The mode of operation. i.e. the real mode or protected mode.
  @param  Mask                  Optional interupt mask for the new mode.
  @param  EdgeLevel             Optional trigger mask for the new mode.

  @retval EFI_SUCCESS           8259 programmed.
  @retval EFI_DEVICE_ERROR      Error writing to 8259.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_8259_SET_MODE)(
  IN EFI_LEGACY_8259_PROTOCOL         *This,
  IN  EFI_8259_MODE                   Mode,
  IN  UINT16                          *Mask  OPTIONAL,
  IN  UINT16                          *EdgeLevel OPTIONAL
  );

/**
  Convert from IRQ to processor interrupt vector number.

  @param  This                  The protocol instance pointer.
  @param  Irq                   8259 IRQ0 - IRQ15.
  @param  Vector                The processor vector number that matches an Irq.

  @retval EFI_SUCCESS           The Vector matching Irq is returned.
  @retval EFI_INVALID_PARAMETER The Irq not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_8259_GET_VECTOR)(
  IN EFI_LEGACY_8259_PROTOCOL           *This,
  IN  EFI_8259_IRQ                      Irq,
  OUT UINT8                             *Vector
  );

/**
  Enable Irq by unmasking interrupt in 8259

  @param  This                  The protocol instance pointer.
  @param  Irq                   8259 IRQ0 - IRQ15.
  @param  LevelTriggered        TRUE if level triggered. FALSE if edge triggered.

  @retval EFI_SUCCESS           The Irq was enabled on 8259.
  @retval EFI_INVALID_PARAMETER The Irq is not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_8259_ENABLE_IRQ)(
  IN EFI_LEGACY_8259_PROTOCOL           *This,
  IN  EFI_8259_IRQ                      Irq,
  IN  BOOLEAN                           LevelTriggered
  );

/**
  Disable Irq by masking interrupt in 8259

  @param  This                  The protocol instance pointer.
  @param  Irq                   8259 IRQ0 - IRQ15.

  @retval EFI_SUCCESS           The Irq was disabled on 8259.
  @retval EFI_INVALID_PARAMETER The Irq is not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_8259_DISABLE_IRQ)(
  IN EFI_LEGACY_8259_PROTOCOL           *This,
  IN  EFI_8259_IRQ                      Irq
  );

/**
  PciHandle represents a PCI config space of a PCI function. Vector
  represents Interrupt Pin (from PCI config space) and it is the data
  that is programmed into the Interrupt Line (from the PCI config space)
  register.

  @param  This                  The protocol instance pointer.
  @param  PciHandle             The PCI function to return the vector for.
  @param  Vector                The vector for the function it matches.

  @retval EFI_SUCCESS           A valid Vector was returned.
  @retval EFI_INVALID_PARAMETER PciHandle not valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_8259_GET_INTERRUPT_LINE)(
  IN EFI_LEGACY_8259_PROTOCOL           *This,
  IN  EFI_HANDLE                        PciHandle,
  OUT UINT8                             *Vector
  );

/**
  Send an EOI to 8259

  @param  This                  The protocol instance pointer.
  @param  Irq                   8259 IRQ0 - IRQ15.

  @retval EFI_SUCCESS           EOI was successfully sent to 8259.
  @retval EFI_INVALID_PARAMETER The Irq isnot valid.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_8259_END_OF_INTERRUPT)(
  IN EFI_LEGACY_8259_PROTOCOL           *This,
  IN  EFI_8259_IRQ                      Irq
  );

/**
  @par Protocol Description:
  Abstracts the 8259 and APIC hardware control between EFI usage and
  Compatibility16 usage.

  @param SetVectorBase
  Sets the vector bases for master and slave PICs.

  @param GetMask
  Gets IRQ and edge/level masks for 16-bit real mode and 32-bit protected mode.

  @param SetMask
  Sets the IRQ and edge\level masks for 16-bit real mode and 32-bit protected mode.

  @param SetMode
  Sets PIC mode to 16-bit real mode or 32-bit protected mode.

  @param GetVector
  Gets the base vector assigned to an IRQ.

  @param EnableIrq
  Enables an IRQ.

  @param DisableIrq
  Disables an IRQ.

  @param GetInterruptLine
  Gets an IRQ that is assigned to a PCI device.

  @param EndOfInterrupt
  Issues the end of interrupt command.

**/
struct _EFI_LEGACY_8259_PROTOCOL {
  EFI_LEGACY_8259_SET_VECTOR_BASE       SetVectorBase;
  EFI_LEGACY_8259_GET_MASK              GetMask;
  EFI_LEGACY_8259_SET_MASK              SetMask;
  EFI_LEGACY_8259_SET_MODE              SetMode;
  EFI_LEGACY_8259_GET_VECTOR            GetVector;
  EFI_LEGACY_8259_ENABLE_IRQ            EnableIrq;
  EFI_LEGACY_8259_DISABLE_IRQ           DisableIrq;
  EFI_LEGACY_8259_GET_INTERRUPT_LINE    GetInterruptLine;
  EFI_LEGACY_8259_END_OF_INTERRUPT      EndOfInterrupt;
};

extern EFI_GUID  gEfiLegacy8259ProtocolGuid;

#endif
