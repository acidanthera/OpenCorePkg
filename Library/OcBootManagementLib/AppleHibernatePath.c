/** @file
  Resolve a legacy ATAPI hibernation path against an unambiguous NVMe disk.

  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Protocol/BlockIo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "BootManagementInternal.h"

BOOLEAN
InternalFixAppleHibernateDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *Suffix;
  EFI_DEVICE_PATH_PROTOCOL  *DiskPath;
  EFI_DEVICE_PATH_PROTOCOL  *DiskNode;
  EFI_DEVICE_PATH_PROTOCOL  *FixedPath;
  EFI_DEVICE_PATH_PROTOCOL  *MatchPath;
  EFI_HANDLE                *Handles;
  EFI_HANDLE                Match;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_STATUS                Status;
  UINTN                     PrefixSize;
  UINTN                     Index;
  UINTN                     Count;
  UINTN                     PciNodes;

  //
  // This fallback only covers a full PCI path followed by one legacy ATAPI
  // node and the image's file-path suffix. In particular, do not remap paths
  // containing partition, USB, multiple-instance or arbitrary vendor nodes.
  //
  if (  (DevicePath == NULL) || (*DevicePath == NULL)
     || !IsDevicePathValid (*DevicePath, 0))
  {
    return FALSE;
  }

  Node = *DevicePath;
  if (  (DevicePathType (Node) != ACPI_DEVICE_PATH)
     || (DevicePathSubType (Node) != ACPI_DP)
     || (DevicePathNodeLength (Node) != sizeof (ACPI_HID_DEVICE_PATH)))
  {
    return FALSE;
  }

  Node     = NextDevicePathNode (Node);
  PciNodes = 0;
  while (  (DevicePathType (Node) == HARDWARE_DEVICE_PATH)
        && (DevicePathSubType (Node) == HW_PCI_DP)
        && (DevicePathNodeLength (Node) == sizeof (PCI_DEVICE_PATH)))
  {
    ++PciNodes;
    Node = NextDevicePathNode (Node);
  }

  if (  (PciNodes == 0)
     || (DevicePathType (Node) != MESSAGING_DEVICE_PATH)
     || (DevicePathSubType (Node) != MSG_ATAPI_DP)
     || (DevicePathNodeLength (Node) != sizeof (ATAPI_DEVICE_PATH)))
  {
    return FALSE;
  }

  PrefixSize = (UINTN)Node - (UINTN)*DevicePath;
  Suffix     = NextDevicePathNode (Node);
  if (  (DevicePathType (Suffix) != MEDIA_DEVICE_PATH)
     || (DevicePathSubType (Suffix) != MEDIA_FILEPATH_DP)
     || (DevicePathNodeLength (Suffix) < SIZE_OF_FILEPATH_DEVICE_PATH + sizeof (CHAR16))
     || ((DevicePathNodeLength (Suffix) & 1U) != 0)
     || !IsDevicePathEnd (NextDevicePathNode (Suffix)))
  {
    return FALSE;
  }

  //
  // A path that already resolves to Block I/O needs no fallback.
  //
  Node   = *DevicePath;
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &Node, &Match);
  if (!EFI_ERROR (Status) && (Node == Suffix)) {
    return FALSE;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &Count,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Match     = NULL;
  MatchPath = NULL;
  for (Index = 0; Index < Count; ++Index) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **)&BlockIo
                    );
    if (  EFI_ERROR (Status) || (BlockIo->Media == NULL)
       || !BlockIo->Media->MediaPresent || BlockIo->Media->LogicalPartition
       || BlockIo->Media->RemovableMedia)
    {
      continue;
    }

    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DiskPath
                    );
    if (  EFI_ERROR (Status) || (DiskPath == NULL) || !IsDevicePathValid (DiskPath, 0)
       || (GetDevicePathSize (DiskPath) != PrefixSize + sizeof (NVME_NAMESPACE_DEVICE_PATH) + END_DEVICE_PATH_LENGTH)
       || (CompareMem (DiskPath, *DevicePath, PrefixSize) != 0))
    {
      continue;
    }

    DiskNode = (EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)DiskPath + PrefixSize);
    //
    // Apple firmware uses subtype 0x16 for its 16-byte NVMe namespace node.
    //
    if (  (DevicePathType (DiskNode) != MESSAGING_DEVICE_PATH)
       || (  (DevicePathSubType (DiskNode) != MSG_NVME_NAMESPACE_DP)
          && (DevicePathSubType (DiskNode) != MSG_SASEX_DP))
       || (DevicePathNodeLength (DiskNode) != sizeof (NVME_NAMESPACE_DEVICE_PATH))
       || !IsDevicePathEnd (NextDevicePathNode (DiskNode)))
    {
      continue;
    }

    if (Match != NULL) {
      DEBUG ((DEBUG_INFO, "OCB: Ambiguous ATAPI hibernation disk, keeping original path\n"));
      FreePool (Handles);
      return FALSE;
    }

    Match     = Handles[Index];
    MatchPath = DiskPath;
  }

  FreePool (Handles);
  if (Match == NULL) {
    return FALSE;
  }

  //
  // Preserve the complete image offset / volume UUID suffix byte for byte.
  // Do not write NVRAM here; the caller retains its existing write handling.
  //
  FixedPath = AppendDevicePath (MatchPath, Suffix);
  if (FixedPath == NULL) {
    return FALSE;
  }

  Node   = FixedPath;
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &Node, &Match);
  if (  EFI_ERROR (Status)
     || ((UINTN)Node - (UINTN)FixedPath != PrefixSize + sizeof (NVME_NAMESPACE_DEVICE_PATH)))
  {
    FreePool (FixedPath);
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "OCB: Fixed ATAPI hibernation path using matching NVMe disk\n"));
  FreePool (*DevicePath);
  *DevicePath = FixedPath;
  return TRUE;
}
