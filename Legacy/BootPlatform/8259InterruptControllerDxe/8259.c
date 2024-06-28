/** @file
  This contains the installation function for the driver.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "8259.h"

//
// Global for the Legacy 8259 Protocol that is produced by this driver
//
EFI_LEGACY_8259_PROTOCOL  mInterrupt8259 = {
  Interrupt8259SetVectorBase,
  Interrupt8259GetMask,
  Interrupt8259SetMask,
  Interrupt8259SetMode,
  Interrupt8259GetVector,
  Interrupt8259EnableIrq,
  Interrupt8259DisableIrq,
  Interrupt8259GetInterruptLine,
  Interrupt8259EndOfInterrupt
};

//
// Global for the handle that the Legacy 8259 Protocol is installed
//
EFI_HANDLE  m8259Handle = NULL;

UINT8          mMasterBase        = 0xff;
UINT8          mSlaveBase         = 0xff;
EFI_8259_MODE  mMode              = Efi8259ProtectedMode;
UINT16         mProtectedModeMask = 0xffff;
UINT16         mLegacyModeMask;
UINT16         mProtectedModeEdgeLevel = 0x0000;
UINT16         mLegacyModeEdgeLevel;

//
// Worker Functions
//

/**
  Write to mask and edge/level triggered registers of master and slave PICs.

  @param[in]  Mask       low byte for master PIC mask register,
                         high byte for slave PIC mask register.
  @param[in]  EdgeLevel  low byte for master PIC edge/level triggered register,
                         high byte for slave PIC edge/level triggered register.

**/
VOID
Interrupt8259WriteMask (
  IN UINT16  Mask,
  IN UINT16  EdgeLevel
  )
{
  IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, (UINT8)Mask);
  IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, (UINT8)(Mask >> 8));
  IoWrite8 (LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_MASTER, (UINT8)EdgeLevel);
  IoWrite8 (LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_SLAVE, (UINT8)(EdgeLevel >> 8));
}

/**
  Read from mask and edge/level triggered registers of master and slave PICs.

  @param[out]  Mask       low byte for master PIC mask register,
                          high byte for slave PIC mask register.
  @param[out]  EdgeLevel  low byte for master PIC edge/level triggered register,
                          high byte for slave PIC edge/level triggered register.

**/
VOID
Interrupt8259ReadMask (
  OUT UINT16  *Mask,
  OUT UINT16  *EdgeLevel
  )
{
  UINT16  MasterValue;
  UINT16  SlaveValue;

  if (Mask != NULL) {
    MasterValue = IoRead8 (LEGACY_8259_MASK_REGISTER_MASTER);
    SlaveValue  = IoRead8 (LEGACY_8259_MASK_REGISTER_SLAVE);

    *Mask = (UINT16)(MasterValue | (SlaveValue << 8));
  }

  if (EdgeLevel != NULL) {
    MasterValue = IoRead8 (LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_MASTER);
    SlaveValue  = IoRead8 (LEGACY_8259_EDGE_LEVEL_TRIGGERED_REGISTER_SLAVE);

    *EdgeLevel = (UINT16)(MasterValue | (SlaveValue << 8));
  }
}

//
// Legacy 8259 Protocol Interface Functions
//

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
  )
{
  UINT8    Mask;
  EFI_TPL  OriginalTpl;

  OriginalTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  //
  // Set vector base for slave PIC
  //
  if (SlaveBase != mSlaveBase) {
    mSlaveBase = SlaveBase;

    //
    // Initialization sequence is needed for setting vector base.
    //

    //
    // Preserve interrtup mask register before initialization sequence
    // because it will be cleared during initialization
    //
    Mask = IoRead8 (LEGACY_8259_MASK_REGISTER_SLAVE);

    //
    // ICW1: cascade mode, ICW4 write required
    //
    IoWrite8 (LEGACY_8259_CONTROL_REGISTER_SLAVE, 0x11);

    //
    // ICW2: new vector base (must be multiple of 8)
    //
    IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, mSlaveBase);

    //
    // ICW3: slave indentification code must be 2
    //
    IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0x02);

    //
    // ICW4: fully nested mode, non-buffered mode, normal EOI, IA processor
    //
    IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, 0x01);

    //
    // Restore interrupt mask register
    //
    IoWrite8 (LEGACY_8259_MASK_REGISTER_SLAVE, Mask);
  }

  //
  // Set vector base for master PIC
  //
  if (MasterBase != mMasterBase) {
    mMasterBase = MasterBase;

    //
    // Initialization sequence is needed for setting vector base.
    //

    //
    // Preserve interrtup mask register before initialization sequence
    // because it will be cleared during initialization
    //
    Mask = IoRead8 (LEGACY_8259_MASK_REGISTER_MASTER);

    //
    // ICW1: cascade mode, ICW4 write required
    //
    IoWrite8 (LEGACY_8259_CONTROL_REGISTER_MASTER, 0x11);

    //
    // ICW2: new vector base (must be multiple of 8)
    //
    IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, mMasterBase);

    //
    // ICW3: slave PIC is cascaded on IRQ2
    //
    IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0x04);

    //
    // ICW4: fully nested mode, non-buffered mode, normal EOI, IA processor
    //
    IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, 0x01);

    //
    // Restore interrupt mask register
    //
    IoWrite8 (LEGACY_8259_MASK_REGISTER_MASTER, Mask);
  }

  IoWrite8 (LEGACY_8259_CONTROL_REGISTER_SLAVE, LEGACY_8259_EOI);
  IoWrite8 (LEGACY_8259_CONTROL_REGISTER_MASTER, LEGACY_8259_EOI);

  gBS->RestoreTPL (OriginalTpl);

  return EFI_SUCCESS;
}

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
  )
{
  if (LegacyMask != NULL) {
    *LegacyMask = mLegacyModeMask;
  }

  if (LegacyEdgeLevel != NULL) {
    *LegacyEdgeLevel = mLegacyModeEdgeLevel;
  }

  if (ProtectedMask != NULL) {
    *ProtectedMask = mProtectedModeMask;
  }

  if (ProtectedEdgeLevel != NULL) {
    *ProtectedEdgeLevel = mProtectedModeEdgeLevel;
  }

  return EFI_SUCCESS;
}

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
  )
{
  if (LegacyMask != NULL) {
    mLegacyModeMask = *LegacyMask;
  }

  if (LegacyEdgeLevel != NULL) {
    mLegacyModeEdgeLevel = *LegacyEdgeLevel;
  }

  if (ProtectedMask != NULL) {
    mProtectedModeMask = *ProtectedMask;
  }

  if (ProtectedEdgeLevel != NULL) {
    mProtectedModeEdgeLevel = *ProtectedEdgeLevel;
  }

  return EFI_SUCCESS;
}

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
  )
{
  if (Mode == mMode) {
    return EFI_SUCCESS;
  }

  if (Mode == Efi8259LegacyMode) {
    //
    // In Efi8259ProtectedMode, mask and edge/level trigger registers should
    // be changed through this protocol, so we can track them in the
    // corresponding module variables.
    //
    Interrupt8259ReadMask (&mProtectedModeMask, &mProtectedModeEdgeLevel);

    if (Mask != NULL) {
      //
      // Update the Mask for the new mode
      //
      mLegacyModeMask = *Mask;
    }

    if (EdgeLevel != NULL) {
      //
      // Update the Edge/Level triggered mask for the new mode
      //
      mLegacyModeEdgeLevel = *EdgeLevel;
    }

    mMode = Mode;

    //
    // Write new legacy mode mask/trigger level
    //
    Interrupt8259WriteMask (mLegacyModeMask, mLegacyModeEdgeLevel);

    return EFI_SUCCESS;
  }

  if (Mode == Efi8259ProtectedMode) {
    //
    // Save the legacy mode mask/trigger level
    //
    Interrupt8259ReadMask (&mLegacyModeMask, &mLegacyModeEdgeLevel);
    //
    // Always force Timer to be enabled after return from 16-bit code.
    // This always insures that on next entry, timer is counting.
    //
    mLegacyModeMask &= 0xFFFE;

    if (Mask != NULL) {
      //
      // Update the Mask for the new mode
      //
      mProtectedModeMask = *Mask;
    }

    if (EdgeLevel != NULL) {
      //
      // Update the Edge/Level triggered mask for the new mode
      //
      mProtectedModeEdgeLevel = *EdgeLevel;
    }

    mMode = Mode;

    //
    // Write new protected mode mask/trigger level
    //
    Interrupt8259WriteMask (mProtectedModeMask, mProtectedModeEdgeLevel);

    return EFI_SUCCESS;
  }

  return EFI_INVALID_PARAMETER;
}

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
  )
{
  if ((UINT32)Irq > Efi8259Irq15) {
    return EFI_INVALID_PARAMETER;
  }

  if (Irq <= Efi8259Irq7) {
    *Vector = (UINT8)(mMasterBase + Irq);
  } else {
    *Vector = (UINT8)(mSlaveBase + (Irq - Efi8259Irq8));
  }

  return EFI_SUCCESS;
}

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
  )
{
  if ((UINT32)Irq > Efi8259Irq15) {
    return EFI_INVALID_PARAMETER;
  }

  mProtectedModeMask = (UINT16)(mProtectedModeMask & ~(1 << Irq));
  if (LevelTriggered) {
    mProtectedModeEdgeLevel = (UINT16)(mProtectedModeEdgeLevel | (1 << Irq));
  } else {
    mProtectedModeEdgeLevel = (UINT16)(mProtectedModeEdgeLevel & ~(1 << Irq));
  }

  Interrupt8259WriteMask (mProtectedModeMask, mProtectedModeEdgeLevel);

  return EFI_SUCCESS;
}

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
  )
{
  if ((UINT32)Irq > Efi8259Irq15) {
    return EFI_INVALID_PARAMETER;
  }

  mProtectedModeMask = (UINT16)(mProtectedModeMask | (1 << Irq));

  mProtectedModeEdgeLevel = (UINT16)(mProtectedModeEdgeLevel & ~(1 << Irq));

  Interrupt8259WriteMask (mProtectedModeMask, mProtectedModeEdgeLevel);

  return EFI_SUCCESS;
}

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
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT8                InterruptLine;
  EFI_STATUS           Status;

  Status = gBS->HandleProtocol (
                  PciHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo
                  );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  PciIo->Pci.Read (
               PciIo,
               EfiPciIoWidthUint8,
               PCI_INT_LINE_OFFSET,
               1,
               &InterruptLine
               );
  //
  // Interrupt line is same location for standard PCI cards, standard
  // bridge and CardBus bridge.
  //
  *Vector = InterruptLine;

  return EFI_SUCCESS;
}

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
  IN EFI_LEGACY_8259_PROTOCOL  *This,
  IN EFI_8259_IRQ              Irq
  )
{
  if ((UINT32)Irq > Efi8259Irq15) {
    return EFI_INVALID_PARAMETER;
  }

  if (Irq >= Efi8259Irq8) {
    IoWrite8 (LEGACY_8259_CONTROL_REGISTER_SLAVE, LEGACY_8259_EOI);
  }

  IoWrite8 (LEGACY_8259_CONTROL_REGISTER_MASTER, LEGACY_8259_EOI);

  return EFI_SUCCESS;
}

/**
  Driver Entry point.

  @param[in]  ImageHandle  ImageHandle of the loaded driver.
  @param[in]  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS  One or more of the drivers returned a success code.
  @retval  !EFI_SUCCESS  Error installing Legacy 8259 Protocol.

**/
EFI_STATUS
EFIAPI
Install8259 (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_8259_IRQ  Irq;

  //
  // Initialze mask values from PCDs
  //
  mLegacyModeMask      = PcdGet16 (Pcd8259LegacyModeMask);
  mLegacyModeEdgeLevel = PcdGet16 (Pcd8259LegacyModeEdgeLevel);

  //
  // Clear all pending interrupt
  //
  for (Irq = Efi8259Irq0; Irq <= Efi8259Irq15; Irq++) {
    Interrupt8259EndOfInterrupt (&mInterrupt8259, Irq);
  }

  //
  // Set the 8259 Master base to 0x68 and the 8259 Slave base to 0x70
  //
  Status = Interrupt8259SetVectorBase (&mInterrupt8259, PROTECTED_MODE_BASE_VECTOR_MASTER, PROTECTED_MODE_BASE_VECTOR_SLAVE);

  //
  // Set all 8259 interrupts to edge triggered and disabled
  //
  Interrupt8259WriteMask (mProtectedModeMask, mProtectedModeEdgeLevel);

  //
  // Install 8259 Protocol onto a new handle
  //
  Status = gBS->InstallProtocolInterface (
                  &m8259Handle,
                  &gEfiLegacy8259ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mInterrupt8259
                  );
  return Status;
}
