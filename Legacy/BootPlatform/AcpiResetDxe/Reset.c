/*++ @file
  Reset Architectural Protocol implementation.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

--*/

#include <PiDxe.h>

#include <Protocol/Reset.h>

#include <Guid/AcpiDescription.h>

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>

///
/// Handle for the Reset Architectural Protocol
///
EFI_HANDLE            mResetHandle = NULL;

///
/// Copy of ACPI Description HOB in runtime memory
///
EFI_ACPI_DESCRIPTION  mAcpiDescription;

/**
  Reset the system.

  @param[in] ResetType       Warm or cold
  @param[in] ResetStatus     Possible cause of reset
  @param[in] DataSize        Size of ResetData in bytes
  @param[in] ResetData       Optional Unicode string

**/
VOID
EFIAPI
EfiAcpiResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN VOID             *ResetData OPTIONAL
  )
{
  UINT8   Dev;
  UINT8   Func;
  UINT8   Register;
  
  switch (ResetType) {
  case EfiResetShutdown:
    //
    // 1. Write SLP_TYPa
    //
    if ((mAcpiDescription.PM1a_CNT_BLK.Address != 0) && (mAcpiDescription.SLP_TYPa != 0)) {
      switch (mAcpiDescription.PM1a_CNT_BLK.AddressSpaceId) {
      case EFI_ACPI_3_0_SYSTEM_IO:
        IoAndThenOr16 ((UINTN)mAcpiDescription.PM1a_CNT_BLK.Address, 0xc3ff, (UINT16)(0x2000 | (mAcpiDescription.SLP_TYPa << 10)));
        break;
      case EFI_ACPI_3_0_SYSTEM_MEMORY:
        MmioAndThenOr16 ((UINTN)mAcpiDescription.PM1a_CNT_BLK.Address, 0xc3ff, (UINT16)(0x2000 | (mAcpiDescription.SLP_TYPa << 10)));
        break;
      }
    }

    //
    // 2. Write SLP_TYPb
    //
    if ((mAcpiDescription.PM1b_CNT_BLK.Address != 0) && (mAcpiDescription.SLP_TYPb != 0)) {
      switch (mAcpiDescription.PM1b_CNT_BLK.AddressSpaceId) {
      case EFI_ACPI_3_0_SYSTEM_IO:
        IoAndThenOr16 ((UINTN)mAcpiDescription.PM1b_CNT_BLK.Address, 0xc3ff, (UINT16)(0x2000 | (mAcpiDescription.SLP_TYPb << 10)));
        break;
      case EFI_ACPI_3_0_SYSTEM_MEMORY:
        MmioAndThenOr16 ((UINTN)mAcpiDescription.PM1b_CNT_BLK.Address, 0xc3ff, (UINT16)(0x2000 | (mAcpiDescription.SLP_TYPb << 10)));
        break;
      }
    }
    //
    // If Shutdown fails, then let fall through to reset 
    //
  case EfiResetWarm:
  case EfiResetCold:
    if ((mAcpiDescription.RESET_REG.Address != 0) &&
        ((mAcpiDescription.RESET_REG.AddressSpaceId == EFI_ACPI_3_0_SYSTEM_IO) ||
         (mAcpiDescription.RESET_REG.AddressSpaceId == EFI_ACPI_3_0_SYSTEM_MEMORY) ||
         (mAcpiDescription.RESET_REG.AddressSpaceId == EFI_ACPI_3_0_PCI_CONFIGURATION_SPACE))) {
      //
      // Use ACPI System Reset
      //
      switch (mAcpiDescription.RESET_REG.AddressSpaceId) {
      case EFI_ACPI_3_0_SYSTEM_IO:
        //
        // Send reset request through I/O port register
        //
        IoWrite8 ((UINTN)mAcpiDescription.RESET_REG.Address, mAcpiDescription.RESET_VALUE);
        //
        // Halt 
        //
        CpuDeadLoop ();
      case EFI_ACPI_3_0_SYSTEM_MEMORY:
        //
        // Send reset request through MMIO register
        //
        MmioWrite8 ((UINTN)mAcpiDescription.RESET_REG.Address, mAcpiDescription.RESET_VALUE);
        //
        // Halt 
        //
        CpuDeadLoop ();
      case EFI_ACPI_3_0_PCI_CONFIGURATION_SPACE:
        //
        // Send reset request through PCI register
        //
        Register = (UINT8)mAcpiDescription.RESET_REG.Address;
        Func     = (UINT8) (RShiftU64 (mAcpiDescription.RESET_REG.Address, 16) & 0x7);
        Dev      = (UINT8) (RShiftU64 (mAcpiDescription.RESET_REG.Address, 32) & 0x1F);
        PciWrite8 (PCI_LIB_ADDRESS (0, Dev, Func, Register), mAcpiDescription.RESET_VALUE);
        //
        // Halt 
        //
        CpuDeadLoop ();
      }
    }

    //
    // If system comes here, means ACPI reset is not supported, so do Legacy System Reset, assume 8042 available
    //
    IoWrite8 (0x64, 0xfe);
    CpuDeadLoop ();

  default:
    break;
  }

  //
  // Given we should have reset getting here would be bad
  //
  ASSERT (FALSE);
  CpuDeadLoop();
}

/**
  Initialize the state information for the Reset Architectural Protocol.

  @param[in] ImageHandle  Image handle of the loaded driver
  @param[in] SystemTable  Pointer to the System Table

  @retval EFI_SUCCESS           Thread can be successfully created
  @retval EFI_UNSUPPORTED       Cannot find the info to reset system

**/
EFI_STATUS
EFIAPI
InitializeReset (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  EFI_HOB_GUID_TYPE  *HobAcpiDescription;

  //
  // Make sure the Reset Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiResetArchProtocolGuid);

  //
  // Get ACPI Description HOB
  //
  HobAcpiDescription = GetFirstGuidHob (&gEfiAcpiDescriptionGuid);
  if (HobAcpiDescription == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Copy it to Runtime Memory
  //
  ASSERT (sizeof (EFI_ACPI_DESCRIPTION) == GET_GUID_HOB_DATA_SIZE (HobAcpiDescription));
  CopyMem (&mAcpiDescription, GET_GUID_HOB_DATA (HobAcpiDescription), sizeof (EFI_ACPI_DESCRIPTION));

  DEBUG ((DEBUG_INFO, "ACPI Reset Base  - %lx\n", mAcpiDescription.RESET_REG.Address));
  DEBUG ((DEBUG_INFO, "ACPI Reset Value - %02x\n", (UINTN)mAcpiDescription.RESET_VALUE));
  DEBUG ((DEBUG_INFO, "IAPC support     - %x\n", (UINTN)(mAcpiDescription.IAPC_BOOT_ARCH)));
  
  //
  // Hook the runtime service table
  //
  SystemTable->RuntimeServices->ResetSystem = EfiAcpiResetSystem;

  //
  // Install the Reset Architectural Protocol onto a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mResetHandle,
                  &gEfiResetArchProtocolGuid, NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
