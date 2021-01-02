/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BdsPlatform.c

Abstract:

  This file include all platform action which can be customized
  by IBV/OEM.
--*/

#include "BdsPlatform.h"

#include <PiDxe.h>

#include <Guid/Acpi.h>
#include <Guid/LdrMemoryDescriptor.h>
#include <Guid/SmBios.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/SmBios.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/PciIo.h>

EFI_GUID *gTableGuidArray[] = {
  &gEfiAcpi10TableGuid,
  &gEfiAcpiTableGuid,
  &gEfiSmbiosTableGuid,
  &gEfiSmbios3TableGuid
};

//
// BDS Platform Functions
//

STATIC
EFI_STATUS
ConvertAcpiTable (
  IN     UINTN                       TableLen,
  IN OUT VOID                        **Table
  )
/*++

Routine Description:
  Convert RSDP of ACPI Table if its location is lower than Address:0x100000
  Assumption here:
   As in legacy Bios, ACPI table is required to place in E/F Seg,
   So here we just check if the range is E/F seg,
   and if Not, assume the Memory type is EfiACPIReclaimMemory/EfiACPIMemoryNVS

Arguments:
  TableLen  - Acpi RSDP length
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  VOID                  *AcpiTableOri;
  VOID                  *AcpiTableNew;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  BufferPtr;

  AcpiTableOri    =  (VOID *)(UINTN)(*(UINT64*)(*Table));
  if (((UINTN)AcpiTableOri < 0x100000) && ((UINTN)AcpiTableOri > 0xE0000)) {
    BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
    Status = gBS->AllocatePages (
      AllocateMaxAddress,
      EfiACPIMemoryNVS,
      EFI_SIZE_TO_PAGES (TableLen),
      &BufferPtr
      );
    ASSERT_EFI_ERROR (Status);

    AcpiTableNew = (VOID *)(UINTN) BufferPtr;
    CopyMem (AcpiTableNew, AcpiTableOri, TableLen);
  } else {
    AcpiTableNew = AcpiTableOri;
  }

  //
  // Change configuration table Pointer
  //
  *Table = AcpiTableNew;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ConvertSmbiosTable (
  IN OUT VOID        **Table
  )
/*++

Routine Description:

  Convert Smbios Table if the Location of the SMBios Table is lower than Addres 0x100000
  Assumption here:
   As in legacy Bios, Smbios table is required to place in E/F Seg,
   So here we just check if the range is F seg,
   and if Not, assume the Memory type is EfiACPIMemoryNVS/EfiRuntimeServicesData
Arguments:
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  SMBIOS_TABLE_ENTRY_POINT *SmbiosTableNew;
  SMBIOS_TABLE_ENTRY_POINT *SmbiosTableOri;
  EFI_STATUS               Status;
  UINT32                   SmbiosEntryLen;
  UINT32                   BufferLen;
  EFI_PHYSICAL_ADDRESS     BufferPtr;

  SmbiosTableNew  = NULL;

  //
  // Get Smibos configuration Table
  //
  SmbiosTableOri =  (SMBIOS_TABLE_ENTRY_POINT *)(UINTN)(*(UINT64*)(*Table));

  if ((SmbiosTableOri == NULL) ||
      ((UINTN)SmbiosTableOri > 0x100000) ||
      ((UINTN)SmbiosTableOri < 0xF0000)){
    return EFI_SUCCESS;
  }
  //
  // Relocate the Smibos memory
  //
  BufferPtr = EFI_SYSTEM_TABLE_MAX_ADDRESS;
  if (SmbiosTableOri->SmbiosBcdRevision != 0x21) {
    SmbiosEntryLen  = SmbiosTableOri->EntryPointLength;
  } else {
    //
    // According to Smbios Spec 2.4, we should set entry point length as 0x1F if version is 2.1
    //
    SmbiosEntryLen = 0x1F;
  }
  BufferLen = SmbiosEntryLen + SYS_TABLE_PAD(SmbiosEntryLen) + SmbiosTableOri->TableLength;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES(BufferLen),
                  &BufferPtr
                  );
  ASSERT_EFI_ERROR (Status);

  SmbiosTableNew = (SMBIOS_TABLE_ENTRY_POINT *)(UINTN)BufferPtr;
  CopyMem (
    SmbiosTableNew,
    SmbiosTableOri,
    SmbiosEntryLen
    );
  //
  // Get Smbios Structure table address, and make sure the start address is 32-bit align
  //
  BufferPtr += SmbiosEntryLen + SYS_TABLE_PAD(SmbiosEntryLen);
  CopyMem (
    (VOID *)(UINTN)BufferPtr,
    (VOID *)(UINTN)(SmbiosTableOri->TableAddress),
    SmbiosTableOri->TableLength
    );
  SmbiosTableNew->TableAddress = (UINT32)BufferPtr;
  SmbiosTableNew->SmbiosBcdRevision = 0x26;
  SmbiosTableNew->IntermediateChecksum = 0;
  SmbiosTableNew->IntermediateChecksum =  CalculateCheckSum8 (
    (UINT8*)SmbiosTableNew + 0x10, SmbiosEntryLen - 0x10
    );
  //
  // Change the SMBIOS pointer
  //
  *Table = SmbiosTableNew;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ConvertSystemTable (
  IN     EFI_GUID        *TableGuid,
  IN OUT VOID            **Table
  )
/*++

Routine Description:
  Convert ACPI Table /Smbios Table /MP Table if its location is lower than Address:0x100000
  Assumption here:
   As in legacy Bios, ACPI/Smbios/MP table is required to place in E/F Seg,
   So here we just check if the range is E/F seg,
   and if Not, assume the Memory type is EfiACPIReclaimMemory/EfiACPIMemoryNVS

Arguments:
  TableGuid - Guid of the table
  Table     - pointer to the table

Returns:
  EFI_SUCEESS - Convert Table successfully
  Other       - Failed

--*/
{
  EFI_STATUS      Status;
  VOID            *AcpiHeader;
  UINTN           AcpiTableLen;

  //
  // If match acpi guid (1.0, 2.0, or later), Convert ACPI table according to version.
  //
  AcpiHeader = (VOID*)(UINTN)(*(UINT64 *)(*Table));

  if (CompareGuid(TableGuid, &gEfiAcpiTableGuid) || CompareGuid(TableGuid, &gEfiAcpi20TableGuid)){
    if (((EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiHeader)->Reserved == 0x00){
      //
      // If Acpi 1.0 Table, then RSDP structure doesn't contain Length field, use structure size
      //
      AcpiTableLen = sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER);
    } else if (((EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiHeader)->Reserved >= 0x02){
      //
      // If Acpi 2.0 or later, use RSDP Length fied.
      //
      AcpiTableLen = ((EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)AcpiHeader)->Length;
    } else {
      //
      // Invalid Acpi Version, return
      //
      return EFI_UNSUPPORTED;
    }
    Status = ConvertAcpiTable (AcpiTableLen, Table);
    return Status;
  }

  //
  // If matches smbios guid, convert Smbios table.
  //
  if (CompareGuid(TableGuid, &gEfiSmbiosTableGuid) ||
      CompareGuid(TableGuid, &gEfiSmbios3TableGuid)){
    Status = ConvertSmbiosTable (Table);
    return Status;
  }

  return EFI_UNSUPPORTED;
}

STATIC
VOID
GetSystemTablesFromHob (
  VOID
  )
/*++

Routine Description:
  Find GUID'ed HOBs that contain EFI_PHYSICAL_ADDRESS of ACPI, SMBIOS, MPs tables

Arguments:
  None

Returns:
  None.

--*/
{
  EFI_PEI_HOB_POINTERS        GuidHob;
  EFI_PEI_HOB_POINTERS        HobStart;
  EFI_PHYSICAL_ADDRESS        *Table;
  UINTN                       Index;

  //
  // Get Hob List
  //
  HobStart.Raw = GetHobList ();
  //
  // Iteratively add ACPI Table, SMBIOS Table, MPS Table to EFI System Table
  //
  for (Index = 0; Index < sizeof (gTableGuidArray) / sizeof (*gTableGuidArray); ++Index) {
    GuidHob.Raw = GetNextGuidHob (gTableGuidArray[Index], HobStart.Raw);
    if (GuidHob.Raw != NULL) {
      Table = GET_GUID_HOB_DATA (GuidHob.Guid);
      if (Table != NULL) {
        //
        // Check if Mps Table/Smbios Table/Acpi Table exists in E/F seg,
        // According to UEFI Spec, we should make sure Smbios table,
        // ACPI table and Mps tables kept in memory of specified type
        //
        ConvertSystemTable (gTableGuidArray[Index], (VOID**)&Table);
        gBS->InstallConfigurationTable (gTableGuidArray[Index], (VOID *)Table);
      }
    }
  }
}

STATIC
VOID
UpdateMemoryMap (
  VOID
  )
{
  EFI_STATUS                      Status;
  UINTN                           NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR *MemorySpaceMap;
  UINT64                          Capabilities;
  EFI_PEI_HOB_POINTERS            GuidHob;
  VOID                            *Table;
  MEMORY_DESC_HOB                 MemoryDescHob;
  UINTN                           Index;
  EFI_PHYSICAL_ADDRESS            Memory;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;
  EFI_PHYSICAL_ADDRESS            FirstNonConventionalAddr;

  //
  // Promote reserved memory to system memory.
  //
  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  ASSERT_EFI_ERROR (Status);

  for (Index = 0; Index < NumberOfDescriptors; ++Index) {
    Capabilities = MemorySpaceMap[Index].Capabilities;

    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeReserved
      && (Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED))
        == (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED)) {
      Status = gDS->RemoveMemorySpace (
        MemorySpaceMap[Index].BaseAddress,
        MemorySpaceMap[Index].Length
        );
      if (!EFI_ERROR (Status)) {
        Status = gDS->AddMemorySpace (
          (Capabilities & EFI_MEMORY_MORE_RELIABLE) == EFI_MEMORY_MORE_RELIABLE
            ? EfiGcdMemoryTypeMoreReliable : EfiGcdMemoryTypeSystemMemory,
          MemorySpaceMap[Index].BaseAddress,
          MemorySpaceMap[Index].Length,
          Capabilities &~ (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME)
          );
      }

      ASSERT_EFI_ERROR (Status);
    }
  }

  gBS->FreePool (MemorySpaceMap);

  //
  // Add ACPINVS, ACPIReclaim, and Reserved memory to MemoryMap.
  //
  GuidHob.Raw = GetFirstGuidHob (&gLdrMemoryDescriptorGuid);
  if (GuidHob.Raw == NULL) {
    return;
  }

  Table = GET_GUID_HOB_DATA (GuidHob.Guid);
  if (Table == NULL) {
    return;
  }

  MemoryDescHob.MemDescCount = *(UINTN *)Table;
  MemoryDescHob.MemDesc      = *(EFI_MEMORY_DESCRIPTOR **)((UINTN)Table + sizeof(UINTN));

  FirstNonConventionalAddr = 0xFFFFFFFF;
  for (Index = 0; Index < MemoryDescHob.MemDescCount; Index++) {
    if (MemoryDescHob.MemDesc[Index].PhysicalStart < 0x100000) {
      continue;
    }
    if (MemoryDescHob.MemDesc[Index].PhysicalStart >= 0x100000000ULL) {
      continue;
    }
    if ((MemoryDescHob.MemDesc[Index].Type == EfiReservedMemoryType)
      || (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesData)
      || (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesCode)
      || (MemoryDescHob.MemDesc[Index].Type == EfiACPIReclaimMemory)
      || (MemoryDescHob.MemDesc[Index].Type == EfiACPIMemoryNVS)) {

      if (MemoryDescHob.MemDesc[Index].PhysicalStart < FirstNonConventionalAddr) {
        FirstNonConventionalAddr = MemoryDescHob.MemDesc[Index].PhysicalStart;
      }

      if ((MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesData)
        || (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesCode)) {
        //
        // For RuntimeSevicesData and RuntimeServicesCode, they are BFV or DxeCore.
        // The memory type is assigned in EfiLdr
        //
        Status = gDS->GetMemorySpaceDescriptor (MemoryDescHob.MemDesc[Index].PhysicalStart, &Descriptor);
        if (EFI_ERROR (Status)) {
          continue;
        }

        if (Descriptor.GcdMemoryType != EfiGcdMemoryTypeReserved) {
          //
          // BFV or tested DXE core
          //
          continue;
        }

        //
        // Untested DXE Core region, free and remove
        //
        Status = gDS->FreeMemorySpace (
          MemoryDescHob.MemDesc[Index].PhysicalStart,
          LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT)
          );

        if (EFI_ERROR (Status)) {
          continue;
        }

        Status = gDS->RemoveMemorySpace (
          MemoryDescHob.MemDesc[Index].PhysicalStart,
          LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT)
          );
        if (EFI_ERROR (Status)) {
          continue;
        }

        //
        // Convert Runtime type to BootTime type
        //
        if (MemoryDescHob.MemDesc[Index].Type == EfiRuntimeServicesData) {
          MemoryDescHob.MemDesc[Index].Type = EfiBootServicesData;
        } else {
          MemoryDescHob.MemDesc[Index].Type = EfiBootServicesCode;
        }

        //
        // PassThrough, let below code add and allocate.
        //
      }
      //
      // ACPI or reserved memory
      //
      Status = gDS->AddMemorySpace (
        EfiGcdMemoryTypeSystemMemory,
        MemoryDescHob.MemDesc[Index].PhysicalStart,
        LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT),
        MemoryDescHob.MemDesc[Index].Attribute
        );
      if (EFI_ERROR (Status)) {
        continue;
      }

      Memory = MemoryDescHob.MemDesc[Index].PhysicalStart;
      Status = gBS->AllocatePages (
                      AllocateAddress,
                      (EFI_MEMORY_TYPE)MemoryDescHob.MemDesc[Index].Type,
                      (UINTN)MemoryDescHob.MemDesc[Index].NumberOfPages,
                      &Memory
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }
    }
  }

  //
  // dmazar: Part of mem fix when "available" memory regions are marked as "reserved"
  // if they were above first non-avaialable region in BIOS mem map.
  // We have left those regions as EfiConventionalMemory in EfiLdr/Support.c GenMemoryMap()
  // and now we will add them to GCD and UEFI mem map
  //
  for (Index = 0; Index < MemoryDescHob.MemDescCount; Index++) {
    if (MemoryDescHob.MemDesc[Index].PhysicalStart < 0x100000) {
      continue;
    }
    if (MemoryDescHob.MemDesc[Index].Type != EfiConventionalMemory) {
      continue;
    }
    if (MemoryDescHob.MemDesc[Index].PhysicalStart < FirstNonConventionalAddr) {
      continue;
    }
    //
    // This is our candidate - add it.
    //
    gDS->AddMemorySpace (
      EfiGcdMemoryTypeSystemMemory,
      MemoryDescHob.MemDesc[Index].PhysicalStart,
      LShiftU64 (MemoryDescHob.MemDesc[Index].NumberOfPages, EFI_PAGE_SHIFT),
      MemoryDescHob.MemDesc[Index].Attribute
      );
  }
}

STATIC
EFI_STATUS
DisableUsbLegacySupport (
  VOID
  )
/*++

Routine Description:
  Disable the USB legacy Support in all Ehci and Uhci.
  This function assume all PciIo handles have been created in system.

Arguments:
  None

Returns:
  EFI_SUCCESS
  EFI_NOT_FOUND
--*/
{
  EFI_STATUS                            Status;
  EFI_HANDLE                            *HandleArray;
  UINTN                                 HandleArrayCount;
  UINTN                                 Index;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT8                                 Class[3];
  UINT16                                Command;
  UINT32                                HcCapParams;
  UINT32                                ExtendCap;
  UINT32                                Value;
  UINT32                                TimeOut;

  //
  // Find the usb host controller
  //
  Status = gBS->LocateHandleBuffer (
                                    ByProtocol,
                                    &gEfiPciIoProtocolGuid,
                                    NULL,
                                    &HandleArrayCount,
                                    &HandleArray
                                    );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleArrayCount; Index++) {
      Status = gBS->HandleProtocol (
                                    HandleArray[Index],
                                    &gEfiPciIoProtocolGuid,
                                    (VOID **)&PciIo
                                    );
      if (!EFI_ERROR (Status)) {
        //
        // Find the USB host controller controller
        //
        Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
        if (!EFI_ERROR (Status)) {
          if ((PCI_CLASS_SERIAL == Class[2]) &&
              (PCI_CLASS_SERIAL_USB == Class[1])) {
            if (PCI_IF_UHCI == Class[0]) {
              //
              // Found the UHCI, then disable the legacy support
              //
              Command = 0;
              Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, 0xC0, 1, &Command);
            } else if (PCI_IF_EHCI == Class[0]) {

              //
              // Found the EHCI, then disable the legacy support
              //
              Status = PciIo->Mem.Read (
                                        PciIo,
                                        EfiPciIoWidthUint32,
                                        0,                   ///< EHC_BAR_INDEX
                                        (UINT64) 0x08,       ///< EHC_HCCPARAMS_OFFSET
                                        1,
                                        &HcCapParams
                                        );

              ExtendCap = (HcCapParams >> 8) & 0xFF;
              //
              // Disable the SMI in USBLEGCTLSTS firstly
              // Not doing this may result in a hardlock soon after
              //
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);
              Value &= 0xFFFF0000;
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap + 0x4, 1, &Value);

              //
              // Get EHCI Ownership from legacy bios
              //
              PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);
              Value |= (0x1 << 24);
              PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

              TimeOut = 40;
              while (TimeOut--) {
                gBS->Stall (500);

                PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, ExtendCap, 1, &Value);

                if ((Value & 0x01010000) == 0x01000000) {
                  break;
                }
              }
            }
          }
        }
      }
    }
    gBS->FreePool (HandleArray);
  } else {
    return Status;
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ConnectRootBridge (
  VOID
  )
/*++

Routine Description:

  Connect RootBridge

Arguments:

  None.

Returns:

  EFI_SUCCESS             - Connect RootBridge successfully.
  EFI_STATUS              - Connect RootBridge fail.

--*/
{
  EFI_STATUS                Status;
  EFI_HANDLE                RootHandle;

  //
  // Make all the PCI_IO protocols on PCI Seg 0 show up
  //
  BdsLibConnectDevicePath (gPlatformRootBridges[0]);

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &gPlatformRootBridges[0],
                  &RootHandle
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->ConnectController (RootHandle, NULL, NULL, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PrepareLpcBridgeDevicePath (
  IN EFI_HANDLE                DeviceHandle
  )
/*++

Routine Description:

  Add IsaKeyboard to ConIn,
  add IsaSerial to ConOut, ConIn, ErrOut.
  LPC Bridge: 06 01 00

Arguments:

  DeviceHandle            - Handle of PCIIO protocol.

Returns:

  EFI_SUCCESS             - LPC bridge is added to ConOut, ConIn, and ErrOut.
  EFI_STATUS              - No LPC bridge is added.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = NULL;
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID*)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Register Keyboard
  //
  DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&gPnpPs2KeyboardDeviceNode);

  BdsLibUpdateConsoleVariable (VarConsoleInp, DevicePath, NULL);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetGopDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL *PciDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL **GopDevicePath
  )
{
  UINTN                           Index;
  EFI_STATUS                      Status;
  EFI_HANDLE                      PciDeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL        *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL        *TempPciDevicePath;
  UINTN                           GopHandleCount;
  EFI_HANDLE                      *GopHandleBuffer;

  if (PciDevicePath == NULL || GopDevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize the GopDevicePath to be PciDevicePath
  //
  *GopDevicePath    = PciDevicePath;
  TempPciDevicePath = PciDevicePath;

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &TempPciDevicePath,
                  &PciDeviceHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Try to connect this handle, so that GOP driver could start on this
  // device and create child handles with GraphicsOutput Protocol installed
  // on them, then we get device paths of these child handles and select
  // them as possible console device.
  //
  gBS->ConnectController (PciDeviceHandle, NULL, NULL, FALSE);

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &GopHandleCount,
                  &GopHandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Add all the child handles as possible Console Device
    //
    for (Index = 0; Index < GopHandleCount; Index++) {
      Status = gBS->HandleProtocol (GopHandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid, (VOID*)&TempDevicePath);
      if (EFI_ERROR (Status)) {
        continue;
      }
      if (CompareMem (
            PciDevicePath,
            TempDevicePath,
            GetDevicePathSize (PciDevicePath) - END_DEVICE_PATH_LENGTH
            ) == 0) {
        //
        // In current implementation, we only enable one of the child handles
        // as console device, i.e. sotre one of the child handle's device
        // path to variable "ConOut"
        // In future, we could select all child handles to be console device
        //

        *GopDevicePath = TempDevicePath;

        //
        // Delete the PCI device's path that added by GetPlugInPciVgaDevicePath()
        // Add the integrity GOP device path.
        //
        BdsLibUpdateConsoleVariable (VarConsoleOutDev, NULL, PciDevicePath);
        BdsLibUpdateConsoleVariable (VarConsoleOutDev, TempDevicePath, NULL);
      }
    }
    gBS->FreePool (GopHandleBuffer);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PreparePciVgaDevicePath (
  IN EFI_HANDLE                DeviceHandle
  )
/*++

Routine Description:

  Add PCI VGA to ConOut.
  PCI VGA: 03 00 00

Arguments:

  DeviceHandle            - Handle of PCIIO protocol.

Returns:

  EFI_SUCCESS             - PCI VGA is added to ConOut.
  EFI_STATUS              - No PCI VGA device is added.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *GopDevicePath;

  DevicePath = NULL;
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID*)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GetGopDevicePath (DevicePath, &GopDevicePath);
  DevicePath = GopDevicePath;

  BdsLibUpdateConsoleVariable (VarConsoleOut, DevicePath, NULL);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DetectAndPreparePlatformPciDevicePath (
  BOOLEAN DetectVgaOnly
  )
/*++

Routine Description:

  Do platform specific PCI Device check and add them to ConOut, ConIn, ErrOut

Arguments:

  DetectVgaOnly           - Only detect VGA device if it's TRUE.

Returns:

  EFI_SUCCESS             - PCI Device check and Console variable update successfully.
  EFI_STATUS              - PCI Device check or Console variable update fail.

--*/
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

  //
  // Start to check all the PciIo to find all possible device
  //
  HandleCount = 0;
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID*)&PciIo);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Check for all PCI device
    //
    Status = PciIo->Pci.Read (
                      PciIo,
                      EfiPciIoWidthUint32,
                      0,
                      sizeof (Pci) / sizeof (UINT32),
                      &Pci
                      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (!DetectVgaOnly) {
      //
      // Here we decide whether it is LPC Bridge
      //
      if ((IS_PCI_LPC (&Pci))
        || ((IS_PCI_ISA_PDECODE (&Pci))
          && (Pci.Hdr.VendorId == 0x8086)
          && (Pci.Hdr.DeviceId == 0x7110))) {
        //
        // Add IsaKeyboard to ConIn,
        // add IsaSerial to ConOut, ConIn, ErrOut
        //
        PrepareLpcBridgeDevicePath (HandleBuffer[Index]);
        continue;
      }
    }

    //
    // Here we decide which VGA device to enable in PCI bus
    //
    if (IS_PCI_VGA (&Pci)) {
      //
      // Add them to ConOut.
      //
      PreparePciVgaDevicePath (HandleBuffer[Index]);
      continue;
    }
  }

  gBS->FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

VOID
EFIAPI
PlatformBdsInit (
  VOID
  )
/*++

Routine Description:

  Platform Bds init. Include the platform firmware vendor, revision
  and so crc check.

Arguments:

Returns:

  None.

--*/
{
  GetSystemTablesFromHob ();

  UpdateMemoryMap ();

  //
  // Append Usb Keyboard short form DevicePath into "ConInDev"
  //
  BdsLibUpdateConsoleVariable (
    VarConsoleInpDev,
    (EFI_DEVICE_PATH_PROTOCOL *) &gUsbClassKeyboardDevicePath,
    NULL
    );
}

EFI_STATUS
BdsConnectConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
  )
/*++

Routine Description:

  Connect the predefined platform default console device. Always try to find
  and enable the vga device if have.

Arguments:

  PlatformConsole         - Predfined platform default console device array.

Returns:

  EFI_SUCCESS             - Success connect at least one ConIn and ConOut
                            device, there must have one ConOut device is
                            active vga device.

  EFI_STATUS              - Return the status of
                            BdsLibConnectAllDefaultConsoles ()

--*/
{
  EFI_STATUS                         Status;
  UINTN                              Index;
  EFI_DEVICE_PATH_PROTOCOL           *VarConout;
  EFI_DEVICE_PATH_PROTOCOL           *VarConin;
  UINTN                              DevicePathSize;

  //
  // Connect RootBridge
  //
  ConnectRootBridge ();

  VarConout = BdsLibGetVariableAndSize (
                VarConsoleOut,
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );
  VarConin = BdsLibGetVariableAndSize (
               VarConsoleInp,
               &gEfiGlobalVariableGuid,
               &DevicePathSize
               );

  if (VarConout == NULL || VarConin == NULL) {
    //
    // Do platform specific PCI Device check and add them to ConOut, ConIn, ErrOut
    //
    DetectAndPreparePlatformPciDevicePath (FALSE);

    //
    // Have chance to connect the platform default console,
    // the platform default console is the minimue device group
    // the platform should support
    //
    for (Index = 0; PlatformConsole[Index].DevicePath != NULL; ++Index) {
      //
      // Update the console variable with the connect type
      //
      if ((PlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
        BdsLibUpdateConsoleVariable (VarConsoleInp, PlatformConsole[Index].DevicePath, NULL);
      }
      if ((PlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
        BdsLibUpdateConsoleVariable (VarConsoleOut, PlatformConsole[Index].DevicePath, NULL);
      }
      if ((PlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
        BdsLibUpdateConsoleVariable (VarErrorOut, PlatformConsole[Index].DevicePath, NULL);
      }
    }
  } else {
    //
    // Only detect VGA device and add them to ConOut
    //
    DetectAndPreparePlatformPciDevicePath (TRUE);
  }

  //
  // The ConIn devices connection will start the USB bus, should disable all
  // Usb legacy support firstly.
  // Caution: Must ensure the PCI bus driver has been started. Since the
  // ConnectRootBridge() will create all the PciIo protocol, it's safe here now
  //
  DisableUsbLegacySupport ();

  //
  // Connect the all the default console with current console variable
  //
  Status = BdsLibConnectAllDefaultConsoles ();

  return Status;
}

VOID
EFIAPI
PlatformBdsPolicyBehavior (
  VOID
  )
/*++

Routine Description:

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.

Arguments:

  DriverOptionList - The header of the driver option link list

  BootOptionList   - The header of the boot option link list

Returns:

  None.

--*/
{
  BdsConnectConsole (gPlatformConsole);

  BdsLibConnectAll ();
}
