/** @file
  Copyright (C) 2023, Goldfish64. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LegacyBootInternal.h"

THUNK_CONTEXT  mThunkContext;

//
// PIWG firmware media device path for Apple legacy interface.
// FwFile(2B0585EB-D8B8-49A9-8B8CE21B01AEF2B7)
//
STATIC CONST UINT8                     AppleLegacyInterfaceMediaDevicePathData[] = {
  0x04, 0x06, 0x14, 0x00, 0xEB, 0x85, 0x05, 0x2B,
  0xB8, 0xD8, 0xA9, 0x49, 0x8B, 0x8C, 0xE2, 0x1B,
  0x01, 0xAE, 0xF2, 0xB7, 0x7F, 0xFF, 0x04, 0x00
};
STATIC CONST EFI_DEVICE_PATH_PROTOCOL  *AppleLegacyInterfaceMediaDevicePathPath = (EFI_DEVICE_PATH_PROTOCOL *)AppleLegacyInterfaceMediaDevicePathData;

#define MAX_APPLE_LEGACY_DEVICE_PATHS  16

STATIC
BOOLEAN
CheckLegacySignature (
  IN CONST CHAR8  *SignatureStr,
  IN       UINT8  *Buffer,
  IN       UINTN  BufferSize
  )
{
  UINT32  Offset;

  Offset = 0;

  return FindPattern (
           (CONST UINT8 *)SignatureStr,
           NULL,
           (CONST UINT32)AsciiStrLen (SignatureStr),
           Buffer,
           (UINT32)BufferSize,
           &Offset
           );
}

STATIC
EFI_STATUS
ScanAppleLegacyInterfacePaths (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePaths,
  IN     UINTN                     MaxDevicePaths
  )
{
  EFI_STATUS  Status;
  UINTN       NoHandles;
  EFI_HANDLE  *Handles;
  UINTN       HandleIndex;
  UINTN       PathCount;
  UINTN       PathIndex;
  BOOLEAN     DevicePathExists;

  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;

  MaxDevicePaths--;
  PathCount = 0;

  //
  // Get all LoadedImage protocol handles.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiLoadedImageProtocolGuid,
                  NULL,
                  &NoHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (HandleIndex = 0; HandleIndex < NoHandles && PathCount < MaxDevicePaths; HandleIndex++) {
    Status = gBS->HandleProtocol (
                    Handles[HandleIndex],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    LoadedImage->DeviceHandle,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Legacy boot interface will be behind a memory range node.
    //
    if (  (DevicePathType (DevicePath) != HARDWARE_DEVICE_PATH)
       || (DevicePathSubType (DevicePath) != HW_MEMMAP_DP))
    {
      continue;
    }

    //
    // Ensure we don't add a duplicate path.
    //
    DevicePathExists = FALSE;
    for (PathIndex = 0; PathIndex < PathCount; PathIndex++) {
      if (DevicePathNodeLength (DevicePath) != DevicePathNodeLength (DevicePaths[PathIndex])) {
        continue;
      }

      if (CompareMem (DevicePath, DevicePaths[PathIndex], DevicePathNodeLength (DevicePath))) {
        DevicePathExists = TRUE;
        break;
      }
    }

    if (DevicePathExists) {
      continue;
    }

    DevicePaths[PathCount++] = AppendDevicePath (DevicePath, AppleLegacyInterfaceMediaDevicePathPath);
  }

  FreePool (Handles);

  DevicePaths[PathCount] = NULL;
  return EFI_SUCCESS;
}

EFI_STATUS
InternalIsLegacyInterfaceSupported (
  OUT BOOLEAN  *IsAppleInterfaceSupported
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_8259_PROTOCOL  *Legacy8259;

  ASSERT (IsAppleInterfaceSupported != NULL);

  //
  // Apple legacy boot interface is only available on Apple platforms.
  // Use legacy 16-bit thunks on legacy PC platforms.
  //
  if (OcStriCmp (L"Apple", gST->FirmwareVendor) == 0) {
    *IsAppleInterfaceSupported = TRUE;
    return EFI_SUCCESS;
  } else {
    Status = gBS->LocateProtocol (&gEfiLegacy8259ProtocolGuid, NULL, (VOID **)&Legacy8259);
    if (!EFI_ERROR (Status) && (Legacy8259 != NULL)) {
      *IsAppleInterfaceSupported = FALSE;
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
InternalSetBootCampHDPath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *HdDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                DiskHandle;
  EFI_HANDLE                PartitionHandle;
  UINT8                     PartitionIndex;
  EFI_DEVICE_PATH_PROTOCOL  *WholeDiskPath;

  WholeDiskPath = OcDiskGetDevicePath (HdDevicePath);
  if (WholeDiskPath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DebugPrintDevicePath (DEBUG_INFO, "OLB: Legacy disk device path", WholeDiskPath);

  //
  // Mark target partition as active.
  //
  DiskHandle = OcPartitionGetDiskHandle (HdDevicePath);
  if (DiskHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PartitionHandle = OcPartitionGetPartitionHandle (HdDevicePath);
  if (PartitionHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = OcDiskGetMbrPartitionIndex (PartitionHandle, &PartitionIndex);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = OcDiskMarkMbrPartitionActive (DiskHandle, PartitionIndex);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set BootCampHD variable pointing to target disk.
  //
  return gRT->SetVariable (
                APPLE_BOOT_CAMP_HD_VARIABLE_NAME,
                &gAppleBootVariableGuid,
                EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                GetDevicePathSize (WholeDiskPath),
                WholeDiskPath
                );
}

EFI_STATUS
InternalLoadAppleLegacyInterface (
  IN  EFI_HANDLE                ParentImageHandle,
  OUT EFI_DEVICE_PATH_PROTOCOL  **ImageDevicePath,
  OUT EFI_HANDLE                *ImageHandle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  **LegacyDevicePaths;
  CHAR16                    *UnicodeDevicePath;
  UINTN                     Index;

  //
  // Get list of possible locations for Apple legacy interface and attempt to load.
  //
  LegacyDevicePaths = AllocateZeroPool (sizeof (*LegacyDevicePaths) * MAX_APPLE_LEGACY_DEVICE_PATHS);
  if (LegacyDevicePaths == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = ScanAppleLegacyInterfacePaths (LegacyDevicePaths, MAX_APPLE_LEGACY_DEVICE_PATHS);
  if (!EFI_ERROR (Status)) {
    for (Index = 0; LegacyDevicePaths[Index] != NULL; Index++) {
      Status = gBS->LoadImage (
                      FALSE,
                      ParentImageHandle,
                      LegacyDevicePaths[Index],
                      NULL,
                      0,
                      ImageHandle
                      );
      if (Status != EFI_NOT_FOUND) {
        *ImageDevicePath = LegacyDevicePaths[Index];

        DEBUG_CODE_BEGIN ();

        UnicodeDevicePath = ConvertDevicePathToText (*ImageDevicePath, FALSE, FALSE);
        DEBUG ((
          DEBUG_INFO,
          "OLB: Loaded Apple legacy interface at dp %s - %r\n",
          UnicodeDevicePath != NULL ? UnicodeDevicePath : L"<null>",
          Status
          ));
        if (UnicodeDevicePath != NULL) {
          FreePool (UnicodeDevicePath);
        }

        DEBUG_CODE_END ();

        break;
      }
    }
  }

  FreePool (LegacyDevicePaths);

  return Status;
}

OC_LEGACY_OS_TYPE
InternalGetPartitionLegacyOsType (
  IN EFI_HANDLE  PartitionHandle,
  IN BOOLEAN     IsCdRomSupported
  )
{
  EFI_STATUS                Status;
  UINT8                     *Buffer;
  UINTN                     BufferSize;
  MASTER_BOOT_RECORD        *Mbr;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_HANDLE                DiskHandle;
  OC_LEGACY_OS_TYPE         LegacyOsType;

  ASSERT (PartitionHandle != NULL);

  DevicePath = DevicePathFromHandle (PartitionHandle);
  if (DevicePath == NULL) {
    return OcLegacyOsTypeNone;
  }

  DiskHandle = OcPartitionGetDiskHandle (DevicePath);
  if (DiskHandle == NULL) {
    return OcLegacyOsTypeNone;
  }

  //
  // For CD devices, validate El-Torito structures.
  // For hard disk and USB devices, validate MBR and PBR of target partition.
  //
  if (OcIsDiskCdRom (DevicePath)) {
    if (!IsCdRomSupported) {
      DEBUG ((DEBUG_INFO, "OLB: CD-ROM boot not supported on this platform\n"));
      return OcLegacyOsTypeNone;
    }

    DebugPrintDevicePath (DEBUG_INFO, "OLB: Reading El-Torito for CDROM", DevicePath);
    Status = OcDiskReadElTorito (DevicePath, &Buffer, &BufferSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OLB: Failed reading El-Torito - %r\n", Status));
      return OcLegacyOsTypeNone;
    }
  } else {
    DebugPrintDevicePath (DEBUG_INFO, "OLB: Reading MBR for parent disk", DevicePath);
    Mbr = OcGetDiskMbrTable (DiskHandle, TRUE);
    if (Mbr == NULL) {
      DEBUG ((DEBUG_INFO, "OLB: Disk does not contain a valid MBR\n"));
      return OcLegacyOsTypeNone;
    }

    FreePool (Mbr);

    //
    // Retrieve PBR for partition.
    //
    DebugPrintDevicePath (DEBUG_INFO, "OLB: Reading PBR for partition", DevicePath);
    Mbr = OcGetDiskMbrTable (PartitionHandle, FALSE);
    if (Mbr == NULL) {
      DEBUG ((DEBUG_INFO, "OLB: Partition does not contain a valid PBR\n"));
      return OcLegacyOsTypeNone;
    }

    Buffer     = (UINT8 *)Mbr;
    BufferSize = sizeof (*Mbr);
  }

  DebugPrintHexDump (DEBUG_INFO, "OLB: PbrHEX", Buffer, BufferSize);

  //
  // Validate sector contents and check for known signatures
  // indicating the partition is bootable.
  //
  if (CheckLegacySignature ("BOOTMGR", Buffer, BufferSize)) {
    LegacyOsType = OcLegacyOsTypeWindowsBootmgr;
  } else if (CheckLegacySignature ("NTLDR", Buffer, BufferSize)) {
    LegacyOsType = OcLegacyOsTypeWindowsNtldr;
  } else if (  CheckLegacySignature ("ISOLINUX", Buffer, BufferSize)
            || CheckLegacySignature ("isolinux", Buffer, BufferSize))
  {
    LegacyOsType = OcLegacyOsTypeIsoLinux;
  } else {
    LegacyOsType = OcLegacyOsTypeNone;
    DEBUG ((DEBUG_INFO, "OLB: Unknown legacy bootsector signature\n"));
  }

  FreePool (Buffer);

  return LegacyOsType;
}

EFI_STATUS
InternalLoadLegacyPbr (
  IN  EFI_DEVICE_PATH_PROTOCOL  *PartitionPath
  )
{
  EFI_STATUS          Status;
  EFI_HANDLE          DiskHandle;
  EFI_HANDLE          PartitionHandle;
  MASTER_BOOT_RECORD  *Mbr;
  MASTER_BOOT_RECORD  *Pbr;
  UINT8               PartitionIndex;
  UINT8               BiosDiskAddress;

  IA32_REGISTER_SET         Regs;
  MASTER_BOOT_RECORD        *MbrPtr = (MASTER_BOOT_RECORD *)0x0600;
  MASTER_BOOT_RECORD        *PbrPtr = (MASTER_BOOT_RECORD *)0x7C00;
  EFI_LEGACY_8259_PROTOCOL  *Legacy8259;

  //
  // Locate Legacy8259 protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiLegacy8259ProtocolGuid,
                  NULL,
                  (VOID **)&Legacy8259
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OLB: Could not locate Legacy8259 protocol\n"));
    return Status;
  }

  //
  // Retrieve the PBR from partition and partition index.
  //
  PartitionHandle = OcPartitionGetPartitionHandle (PartitionPath);
  if (PartitionHandle == NULL) {
    DEBUG ((DEBUG_INFO, "OLB: Could not locate partition handle\n"));
    return EFI_INVALID_PARAMETER;
  }

  Pbr = OcGetDiskMbrTable (PartitionHandle, FALSE);
  if (Pbr == NULL) {
    DEBUG ((DEBUG_INFO, "OLB: Partition does not contain a valid PBR\n"));
    return EFI_INVALID_PARAMETER;
  }

  Status = OcDiskGetMbrPartitionIndex (PartitionHandle, &PartitionIndex);
  if (EFI_ERROR (Status)) {
    FreePool (Pbr);
    return Status;
  }

  DebugPrintHexDump (DEBUG_INFO, "OLB: PbrHEX", (UINT8 *)Pbr, sizeof (*Pbr));

  //
  // Retrieve MBR from disk.
  //
  DiskHandle = OcPartitionGetDiskHandle (PartitionPath);
  if (DiskHandle == NULL) {
    FreePool (Pbr);
    return EFI_INVALID_PARAMETER;
  }

  Mbr = OcGetDiskMbrTable (DiskHandle, TRUE);
  if (Mbr == NULL) {
    DEBUG ((DEBUG_INFO, "OLB: Disk does not contain a valid MBR\n"));
    FreePool (Pbr);
    return EFI_INVALID_PARAMETER;
  }

  DebugPrintHexDump (DEBUG_INFO, "OLB: MbrHEX", (UINT8 *)Mbr, sizeof (*Mbr));

  //
  // Create and initialize thunk structures.
  //
  Status = OcLegacyThunkInitializeBiosIntCaller (&mThunkContext);
  if (EFI_ERROR (Status)) {
    FreePool (Pbr);
    FreePool (Mbr);
    return Status;
  }

  Status = OcLegacyThunkInitializeInterruptRedirection (Legacy8259);
  if (EFI_ERROR (Status)) {
    FreePool (Pbr);
    FreePool (Mbr);
    return Status;
  }

  //
  // Determine BIOS disk number.
  //
  Status = InternalGetBiosDiskAddress (
             &mThunkContext,
             Legacy8259,
             DiskHandle,
             &BiosDiskAddress
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OLB: Disk address could not be determined\n"));
    FreePool (Pbr);
    FreePool (Mbr);
    return Status;
  }

  //
  // Copy MBR and PBR to low memory locations for booting.
  //
  CopyMem (PbrPtr, Pbr, sizeof (*Pbr));
  CopyMem (MbrPtr, Mbr, sizeof (*Mbr));

  DebugPrintHexDump (DEBUG_INFO, "OLB: PbrPtr", (UINT8 *)PbrPtr, sizeof (*PbrPtr));

  OcLegacyThunkDisconnectEfiGraphics ();

  //
  // Thunk to real mode and invoke legacy boot sector.
  // If successful, this function will not return.
  //
  ZeroMem (&Regs, sizeof (Regs));

  Regs.H.DL = BiosDiskAddress;
  Regs.X.SI = (UINT16)(UINTN)&MbrPtr->Partition[PartitionIndex];
  OcLegacyThunkFarCall86 (
    &mThunkContext,
    Legacy8259,
    0,
    0x7c00,
    &Regs,
    NULL,
    0
    );

  DEBUG ((DEBUG_WARN, "OLB: Failure calling legacy boot sector\n"));

  return EFI_INVALID_PARAMETER;
}
