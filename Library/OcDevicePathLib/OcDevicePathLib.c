/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Protocol/DevicePathToText.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/OcDebugLogLib.h>

EFI_DEVICE_PATH_PROTOCOL *
AppendFileNameDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CHAR16                    *FileName
  )
{
  EFI_DEVICE_PATH_PROTOCOL *AppendedDevicePath;

  FILEPATH_DEVICE_PATH     *FilePathNode;
  EFI_DEVICE_PATH_PROTOCOL *DevicePathEndNode;
  UINTN                    FileNameSize;
  UINTN                    FileDevicePathNodeSize;

  AppendedDevicePath = NULL;

  if (DevicePath != NULL && FileName != NULL) {
    FileNameSize           = StrSize (FileName);
    FileDevicePathNodeSize = (FileNameSize + SIZE_OF_FILEPATH_DEVICE_PATH + END_DEVICE_PATH_LENGTH);
    FilePathNode           = AllocateZeroPool (FileDevicePathNodeSize);

    if (FilePathNode != NULL) {
      FilePathNode->Header.Type    = MEDIA_DEVICE_PATH;
      FilePathNode->Header.SubType = MEDIA_FILEPATH_DP;

      SetDevicePathNodeLength (&FilePathNode->Header, FileNameSize + SIZE_OF_FILEPATH_DEVICE_PATH);

      CopyMem (FilePathNode->PathName, FileName, FileNameSize);

      DevicePathEndNode = NextDevicePathNode (&FilePathNode->Header);

      SetDevicePathEndNode (DevicePathEndNode);

      AppendedDevicePath = AppendDevicePath (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)FilePathNode);

      FreePool (FilePathNode);
    }
  }

  return AppendedDevicePath;
}

EFI_DEVICE_PATH_PROTOCOL *
FindDevicePathNodeWithType (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINT8                     Type,
  IN UINT8                     SubType OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL *DevicePathNode;

  DevicePathNode = NULL;

  while (!IsDevicePathEnd (DevicePath)) {
    if ((DevicePathType (DevicePath) == Type)
     && ((SubType == 0) || (DevicePathSubType (DevicePath) == SubType))) {
      DevicePathNode = DevicePath;

      break;
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  return DevicePathNode;
}

EFI_DEVICE_PATH_PROTOCOL *
AbsoluteDevicePath (
  IN EFI_HANDLE                Handle,
  IN EFI_DEVICE_PATH_PROTOCOL  *RelativePath OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *HandlePath;
  EFI_DEVICE_PATH_PROTOCOL  *NewPath;

  HandlePath = DevicePathFromHandle (Handle);
  if (HandlePath == NULL) {
    return NULL;
  }

  if (RelativePath == NULL) {
    return DuplicateDevicePath (HandlePath);
  }

  NewPath = AppendDevicePath (HandlePath, RelativePath);

  if (NewPath == NULL) {
    return DuplicateDevicePath (HandlePath);
  }

  return NewPath;
}

EFI_DEVICE_PATH_PROTOCOL *
TrailedBooterDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathWalker;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  FILEPATH_DEVICE_PATH      *FilePath;
  FILEPATH_DEVICE_PATH      *NewFilePath;
  UINTN                     Length;
  UINTN                     Size;

  DevicePathWalker = DevicePath;

  while (!IsDevicePathEnd (DevicePathWalker)) {
    if ((DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH)
     && (DevicePathSubType (DevicePathWalker) == MEDIA_FILEPATH_DP)
     && IsDevicePathEnd (NextDevicePathNode (DevicePathWalker))) {
      FilePath = (FILEPATH_DEVICE_PATH *) DevicePathWalker;
      Length   = OcFileDevicePathNameLen (FilePath);
      if (Length > 0) {
        if (FilePath->PathName[Length - 1] == L'\\') {
          //
          // Already appended, good. It should never be true with Apple entries though.
          //
          return NULL;
        } else if (Length > 4 &&                      (FilePath->PathName[Length - 4] != '.'
          || (FilePath->PathName[Length - 3] != 'e' && FilePath->PathName[Length - 3] != 'E')
          || (FilePath->PathName[Length - 2] != 'f' && FilePath->PathName[Length - 2] != 'F')
          || (FilePath->PathName[Length - 1] != 'i' && FilePath->PathName[Length - 1] != 'I'))) {
          //
          // Found! We should have gotten something like:
          // PciRoot(0x0)/Pci(...)/Pci(...)/Sata(...)/HD(...)/\com.apple.recovery.boot
          //

          Size          = GetDevicePathSize (DevicePath);
          NewDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocatePool (Size + sizeof (CHAR16));
          if (NewDevicePath == NULL) {
            //
            // Allocation failure, just ignore.
            //
            return NULL;
          }
          //
          // Strip the string termination and DP end node, which will get re-set
          //
          CopyMem (NewDevicePath, DevicePath, Size - sizeof (CHAR16) - END_DEVICE_PATH_LENGTH);
          NewFilePath = (FILEPATH_DEVICE_PATH *) ((UINT8 *)DevicePathWalker - (UINT8 *)DevicePath + (UINT8 *)NewDevicePath);
          Size        = DevicePathNodeLength (DevicePathWalker) + sizeof (CHAR16);
          SetDevicePathNodeLength (NewFilePath, Size);
          NewFilePath->PathName[Length]   = L'\\';
          NewFilePath->PathName[Length+1] = L'\0';
          SetDevicePathEndNode ((UINT8 *) NewFilePath + Size);
          return NewDevicePath;
        }
      }
    }

    DevicePathWalker = NextDevicePathNode (DevicePathWalker);
  }

  //
  // Has .efi suffix or unsupported format.
  //
  return NULL;
}

VOID
OcFixAppleBootDevicePathNodeRestore (
  IN OUT EFI_DEVICE_PATH_PROTOCOL           **DevicePath,
  IN OUT EFI_DEVICE_PATH_PROTOCOL           **DevicePathNode,
  IN     CONST APPLE_BOOT_DP_PATCH_CONTEXT  *RestoreContext
  )
{
  EFI_DEV_PATH_PTR Node;
  UINT8            NodeType;
  UINT8            NodeSubType;

  //
  // ATTENTION: This function must be carefully sync'd with changes to
  //            OcFixAppleBootDevicePathNode().
  //

  Node.DevPath = *DevicePathNode;

  NodeType    = DevicePathType (Node.DevPath);
  NodeSubType = DevicePathSubType (Node.DevPath);

  if (RestoreContext->OldPath != NULL) {
    //
    // Restore the previously stored Device Path prior to patching.
    //
    *DevicePathNode = (EFI_DEVICE_PATH_PROTOCOL *) (
      (UINTN) RestoreContext->OldPath +
        ((UINTN) *DevicePathNode - (UINTN) *DevicePath)
      );
    FreePool (*DevicePath);
    *DevicePath = RestoreContext->OldPath;
    return;
  }

  if (NodeType == MESSAGING_DEVICE_PATH) {
    switch (NodeSubType) {
      case MSG_SATA_DP:
        Node.Sata->PortMultiplierPortNumber = RestoreContext->Types.Sata.PortMultiplierPortNumber;
        break;

      //
      // The related patches in OcFixAppleBootDevicePathNode() are performed
      // from MSG_SASEX_DP and MSG_NVME_NAMESPACE_DP but change the SubType.
      // Please refer to the destination rather than the source SubType when
      // matching the logic.
      //
      case MSG_NVME_NAMESPACE_DP:
      case 0x22:
        Node.NvmeNamespace->Header.SubType = RestoreContext->Types.SasExNvme.SubType;
        break;

      default:
        break;
    }
  } else if (NodeType == ACPI_DEVICE_PATH) {
    switch (NodeSubType) {
      case ACPI_DP:
        Node.Acpi->HID = RestoreContext->Types.Acpi.HID;
        Node.Acpi->UID = RestoreContext->Types.Acpi.UID;
        break;

      case ACPI_EXTENDED_DP:
        Node.ExtendedAcpi->HID = RestoreContext->Types.ExtendedAcpi.HID;
        Node.ExtendedAcpi->CID = RestoreContext->Types.ExtendedAcpi.CID;
        break;

      default:
        break;
    }
  }
}

VOID
OcFixAppleBootDevicePathNodeRestoreFree (
  IN     CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode,
  IN OUT APPLE_BOOT_DP_PATCH_CONTEXT     *RestoreContext
  )
{
  //
  // ATTENTION: This function must be carefully sync'd with changes to
  //            OcFixAppleBootDevicePathNode().
  //

  //
  // Free the previously stored original Device Path for expansion patches.
  //
  if (RestoreContext->OldPath != NULL) {
    FreePool (RestoreContext->OldPath);
  }
}

/*
  Constructs a new device path for an unrecognised device path with a hard drive
  node.

  @param[in,out] DevicePath      A pointer to the device path to fix the node
                                 of. It must be a pool memory buffer.
                                 On success, may be updated with a reallocated
                                 pool memory buffer.
  @param[in,out] DevicePathNode  A pointer to the device path node to fix. It
                                 must be a node of *DevicePath.
                                 On success, may be updated with the
                                 corresponding node of *DevicePath.
  @param[out]    RestoreContext  A pointer to a context that can be used to
                                 restore DevicePathNode's original content in
                                 the case of failure.
                                 On success, data may need to be freed.

  @retval -1  DevicePathNode could not be fixed.
  @retval 1   DevicePathNode and is valid.

*/
STATIC
INTN
InternalExpandNewPath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathNode
  )
{
  EFI_DEVICE_PATH_PROTOCOL *HdNode;
  UINTN                    PrefixSize;
  EFI_DEVICE_PATH_PROTOCOL *ExpandedPath;
  EFI_DEVICE_PATH_PROTOCOL *ExpandedNode;

  DebugPrintDevicePath (DEBUG_VERBOSE, "Expanding new DP from", *DevicePath);
  DebugPrintDevicePath (DEBUG_VERBOSE, "at node", *DevicePathNode);
  //
  // Find HD node to locate a valid Device Path of. We require the prefix
  // till the offending node (e.g. a SATA node, which should be
  // preceeded by a PCI chain) as well as the suffix match (though latter
  // may be expanded). For example SATA should be an arbitrary VendorBlockIo
  // node.
  //
  HdNode = FindDevicePathNodeWithType (
    *DevicePath,
    MEDIA_DEVICE_PATH,
    MEDIA_HARDDRIVE_DP
    );
  if (HdNode == NULL) {
    DEBUG ((DEBUG_VERBOSE, "Failed to find HD node\n"));
    //
    // Expansion makes little sense when we don't have a HD node.
    //
    return -1;
  }

  PrefixSize = (UINTN) *DevicePathNode - (UINTN) *DevicePath;
  //
  // Expand the Device Path based of the HD node and sanity-check it
  // likely describes the intended path.
  //
  for (
    ExpandedPath = OcGetNextLoadOptionDevicePath (HdNode, NULL);
    ExpandedPath != NULL;
    ExpandedPath = OcGetNextLoadOptionDevicePath (HdNode, ExpandedPath)
  ) {
    DebugPrintDevicePath (DEBUG_VERBOSE, "DP candidate", ExpandedPath);
    //
    // Skip this expansion if the prefix does not match.
    //
    if (GetDevicePathSize (ExpandedPath) < PrefixSize
     || CompareMem (ExpandedPath, *DevicePath, PrefixSize) != 0) {
      DEBUG ((DEBUG_VERBOSE, "Prefix does not match\n"));
      continue;
    }
    //
    // The suffix is handled implicitly (by OcGetNextLoadOptionDevicePath).
    // Keep in mind that with this logic our broken node may expand to an
    // arbitrary number of nodes now. 
    //
    ExpandedNode = (EFI_DEVICE_PATH_PROTOCOL *) (
      (UINTN) ExpandedPath + PrefixSize
      );

    DebugPrintDevicePath (DEBUG_VERBOSE, "accepted DP", ExpandedPath);
    DebugPrintDevicePath (DEBUG_VERBOSE, "fix starting at", ExpandedNode);

    DEBUG_CODE (
      EFI_DEVICE_PATH_PROTOCOL *RemDevPath = ExpandedPath;
      EFI_HANDLE Dev;
      EFI_STATUS Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemDevPath, &Dev);
      if (EFI_ERROR (Status) || RemDevPath->Type != END_DEVICE_PATH_TYPE || RemDevPath->SubType != END_ENTIRE_DEVICE_PATH_SUBTYPE) {
        DEBUG ((DEBUG_VERBOSE, "borked piece of crap\n"));
      }
      );

    *DevicePath     = ExpandedPath;
    *DevicePathNode = ExpandedNode;
    return 1;
  }
  //
  // No adequate expansion could be found.
  //
  return -1;
}

INTN
OcFixAppleBootDevicePathNode (
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **DevicePath,
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **DevicePathNode,
  OUT    APPLE_BOOT_DP_PATCH_CONTEXT  *RestoreContext OPTIONAL
  )
{
  INTN                     Result;
  EFI_DEVICE_PATH_PROTOCOL *OldPath;
  EFI_DEV_PATH_PTR         Node;
  UINT8                    NodeType;
  UINT8                    NodeSubType;
  UINTN                    NodeSize;

  ASSERT (DevicePathNode != NULL);

  //
  // ATTENTION: Changes to this function must be carefully sync'd with
  //            OcFixAppleBootDevicePathNodeRestore().
  //

  if (RestoreContext != NULL) {
    RestoreContext->OldPath = NULL;
  }

  Node.DevPath = *DevicePathNode;

  NodeType    = DevicePathType (Node.DevPath);
  NodeSubType = DevicePathSubType (Node.DevPath);

  if (NodeType == MESSAGING_DEVICE_PATH) {
    switch (NodeSubType) {
      case MSG_SATA_DP:
        if (Node.Sata->PortMultiplierPortNumber != 0xFFFF) {
          if (RestoreContext != NULL) {
            RestoreContext->Types.Sata.PortMultiplierPortNumber = Node.Sata->PortMultiplierPortNumber;
          }
          //
          // Must be set to 0xFFFF if the device is directly connected to the
          // HBA. This rule has been established by UEFI 2.5 via an Erratum
          // and has not been followed by Apple thus far.
          // Reference: AppleACPIPlatform.kext,
          //            appendSATADevicePathNodeForIOMedia
          //
          Node.Sata->PortMultiplierPortNumber = 0xFFFF;
          return 1;
        }
        //
        // Some vendors use proprietary node types over standardised ones. Find
        // a new, valid DevicePath that matches in prefix and suffix to the
        // given one but ignore the data of the node that matches the malformed
        // one.
        // REF: https://github.com/acidanthera/bugtracker/issues/991#issue-643248184
        //
        OldPath = *DevicePath;
        Result  = InternalExpandNewPath (
          DevicePath,
          DevicePathNode
          );
        if (Result > 0) {
          //
          // On success, handle restoring and resources.
          //
          if (RestoreContext != NULL) {
            RestoreContext->OldPath = OldPath;
          } else {
            FreePool (OldPath);
          }
        }

        return Result;

      case MSG_SASEX_DP:
        //
        // Apple uses SubType 0x16 (SasEx) for NVMe, while the UEFI
        // Specification defines it as SubType 0x17. The structures are
        // identical.
        // Reference: AppleACPIPlatform.kext,
        //            appendNVMeDevicePathNodeForIOMedia
        //
        if (RestoreContext != NULL) {
          RestoreContext->Types.SasExNvme.SubType = Node.DevPath->SubType;
        }

        STATIC_ASSERT (
          sizeof (SASEX_DEVICE_PATH) != sizeof (NVME_NAMESPACE_DEVICE_PATH),
          "SasEx and NVMe DPs must differ in size for fixing to be accurate."
          );

        NodeSize = DevicePathNodeLength (Node.DevPath);
        if (NodeSize == sizeof (NVME_NAMESPACE_DEVICE_PATH)) {
          Node.SasEx->Header.SubType = MSG_NVME_NAMESPACE_DP;
          return 1;
        }

        return -1;

      case MSG_NVME_NAMESPACE_DP:
        //
        // Apple MacPro5,1 includes NVMe driver, however, it contains a typo in MSG_SASEX_DP.
        // Instead of 0x16 aka 22 (SasEx) it uses 0x22 aka 34 (Unspecified).
        // Here we replace it with the "right" value.
        // Reference: https://forums.macrumors.com/posts/28169441.
        //
        if (RestoreContext != NULL) {
          RestoreContext->Types.SasExNvme.SubType = Node.DevPath->SubType;
        }

        Node.NvmeNamespace->Header.SubType = 0x22;
        return 1;

      default:
        break;
    }
  } else if (NodeType == ACPI_DEVICE_PATH) {
    switch (NodeSubType) {
      case ACPI_DP:
        if (RestoreContext != NULL) {
          RestoreContext->Types.Acpi.HID = Node.Acpi->HID;
          RestoreContext->Types.Acpi.UID = Node.Acpi->UID;
        }

        if (EISA_ID_TO_NUM (Node.Acpi->HID) == 0x0A03) {
          //
          // In some firmwares UIDs for PciRoot do not match between ACPI tables and UEFI
          // UEFI Device Paths. The former contain 0x00, 0x40, 0x80, 0xC0 values, while
          // the latter have ascending numbers.
          // Reference: https://github.com/acidanthera/bugtracker/issues/664.
          // On other boards it is be even more erratic, refer to:
          // https://github.com/acidanthera/bugtracker/issues/664#issuecomment-647526506
          // On older boards there also is a PCI0/_UID1 issue, refer to:
          // https://github.com/acidanthera/bugtracker/issues/664#issuecomment-663873846
          //
          switch (Node.Acpi->UID) {
            case 0x1:
              Node.Acpi->UID = 0;
              return 1;            

            case 0x10:
            case 0x40:
              Node.Acpi->UID = 1;
              return 1;

            case 0x20:
            case 0x80:
              Node.Acpi->UID = 2;
              return 1;

            case 0x28:
            case 0xC0:
              Node.Acpi->UID = 3;
              return 1;

            case 0x30:
              Node.Acpi->UID = 4;
              return 1;

            case 0x38:
              Node.Acpi->UID = 5;
              return 1;

            default:
              break;
          }
          //
          // Apple uses PciRoot (EISA 0x0A03) nodes while some firmwares might use
          // PcieRoot (EISA 0x0A08).
          //
          Node.Acpi->HID = BitFieldWrite32 (
            Node.Acpi->HID,
            16,
            31,
            0x0A08
            );
          return 1;
        }

        return -1;

      case ACPI_EXTENDED_DP:
        if (RestoreContext != NULL) {
          RestoreContext->Types.ExtendedAcpi.HID = Node.ExtendedAcpi->HID;
          RestoreContext->Types.ExtendedAcpi.CID = Node.ExtendedAcpi->CID;
        }
        //
        // Apple uses PciRoot (EISA 0x0A03) nodes while some firmwares might use
        // PcieRoot (EISA 0x0A08).
        //
        if (EISA_ID_TO_NUM (Node.ExtendedAcpi->HID) == 0x0A03) {
          Node.ExtendedAcpi->HID = BitFieldWrite32 (
            Node.ExtendedAcpi->HID,
            16,
            31,
            0x0A08
            );
          return 1;
        }
        
        if (EISA_ID_TO_NUM (Node.ExtendedAcpi->CID) == 0x0A03
         && EISA_ID_TO_NUM (Node.ExtendedAcpi->HID) != 0x0A08) {
          Node.ExtendedAcpi->CID = BitFieldWrite32 (
            Node.ExtendedAcpi->CID,
            16,
            31,
            0x0A08
            );
          return 1;
        }

        return -1;

      default:
        break;
    }
  }

  return 0;
}

INTN
OcFixAppleBootDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
  OUT    EFI_DEVICE_PATH_PROTOCOL  **RemainingDevicePath
  )
{
  INTN                        Result;
  INTN                        NodePatched;
  UINTN                       DevicePathSize;

  APPLE_BOOT_DP_PATCH_CONTEXT FirstNodeRestoreContext;
  APPLE_BOOT_DP_PATCH_CONTEXT *RestoreContextPtr;

  EFI_HANDLE                  Device;

  ASSERT (DevicePath != NULL);
  ASSERT (*DevicePath != NULL);
  ASSERT (IsDevicePathValid (*DevicePath, 0));
  //
  // Restoring is only required for the first Device Path node. Please refer
  // to the loop for an explanation.
  //
  RestoreContextPtr = &FirstNodeRestoreContext;
  NodePatched       = 0;
  do {
    //
    // Retrieve the first Device Path node that cannot be located.
    //
    *RemainingDevicePath = *DevicePath;
    gBS->LocateDevicePath (
           &gEfiDevicePathProtocolGuid,
           RemainingDevicePath,
           &Device
           );
    //
    // Patch the potentially invalid node.
    //
    Result = OcFixAppleBootDevicePathNode (
               DevicePath,
               RemainingDevicePath,
               RestoreContextPtr
               );
    //
    // Save a restore context only for the first processing of the first node.
    // The reason for this is when the first node cannot be located with any
    // patch applied, the Device Path may be of a prefix short-form and may
    // possibly be expanded successfully unmodified.
    //
    RestoreContextPtr = NULL;
    //
    // Continue as long as nodes are being patched. Remember patch status.
    //
    NodePatched |= Result > 0;
  } while (Result > 0);

  if (Result < 0) {
    if (NodePatched != 0) {
      if (*RemainingDevicePath == *DevicePath) {
        //
        // If the path could not be fixed, restore the first node if it's the
        // one failing to be located. Please refer to the loop for an
        // explanation.
        // If advancing by at least one node happened, the Device Path cannot
        // be of a prefix short-form and hence restoring is not beneficial (and
        // most especially would require tracking every node individually).
        //
        OcFixAppleBootDevicePathNodeRestore (
          DevicePath,
          RemainingDevicePath,
          &FirstNodeRestoreContext
          );
      } else {
        //
        // If patching was successful, explicitly free the used resources.
        //
        OcFixAppleBootDevicePathNodeRestoreFree (
          *DevicePath,
          &FirstNodeRestoreContext
          );
      }
    }

    return -1;
  }

  //
  // Double-check that *RemainingDevicePath still points into *DevicePath.
  //
  DEBUG_CODE_BEGIN ();
  DevicePathSize = GetDevicePathSize (*DevicePath);
  ASSERT ((UINTN) *RemainingDevicePath >= (UINTN) *DevicePath);
  ASSERT ((UINTN) *RemainingDevicePath < ((UINTN) *DevicePath) + DevicePathSize);
  DEBUG_CODE_END ();

  return NodePatched;
}

STATIC
BOOLEAN
InternalFileDevicePathsEqualClipBottom (
  IN     CONST FILEPATH_DEVICE_PATH  *FilePath,
  IN OUT UINTN                       *FilePathLength,
  IN OUT UINTN                       *ClipIndex
  )
{
  UINTN Index;

  ASSERT (FilePathLength != NULL);
  ASSERT (*FilePathLength != 0);
  ASSERT (FilePath != NULL);
  ASSERT (ClipIndex != NULL);

  Index = *ClipIndex;
  if (FilePath->PathName[Index] == L'\\') {
    *ClipIndex = Index + 1;
    --(*FilePathLength);
    return TRUE;
  }

  return FALSE;
}

STATIC
UINTN
InternalFileDevicePathsEqualClipNode (
  IN  FILEPATH_DEVICE_PATH  **FilePath,
  OUT UINTN                 *ClipIndex
  )
{
  EFI_DEV_PATH_PTR         DevPath;
  UINTN                    Index;

  UINTN                    Length;
  EFI_DEVICE_PATH_PROTOCOL *NextNode;

  ASSERT (FilePath != NULL);
  ASSERT (ClipIndex != NULL);
  //
  // It is unlikely to be encountered, but empty nodes are not forbidden.
  //
  for (
    Length = 0, NextNode = &(*FilePath)->Header;
    Length == 0;
    NextNode = NextDevicePathNode (DevPath.DevPath)
    ) {
    DevPath.DevPath = NextNode;

    if ((DevicePathType (DevPath.DevPath) != MEDIA_DEVICE_PATH)
     || (DevicePathSubType (DevPath.DevPath) != MEDIA_FILEPATH_DP)) {
      Length = 0;
      break;
    }

    Length = OcFileDevicePathNameLen (DevPath.FilePath);
    if (Length > 0) {
      Index = 0;
      InternalFileDevicePathsEqualClipBottom (DevPath.FilePath, &Length, &Index);
      if ((Length > 0)
       && DevPath.FilePath->PathName[Index + Length - 1] == L'\\') {
        --Length;
      }

      *ClipIndex = Index;
    }
  }

  *FilePath = DevPath.FilePath;
  return Length;
}

STATIC
UINTN
InternalFileDevicePathsEqualClipNextNode (
  IN  FILEPATH_DEVICE_PATH  **FilePath,
  OUT UINTN                 *ClipIndex
  )
{
  ASSERT (FilePath != NULL);
  ASSERT (ClipIndex != NULL);

  *FilePath = (FILEPATH_DEVICE_PATH *)NextDevicePathNode (*FilePath);
  return InternalFileDevicePathsEqualClipNode (FilePath, ClipIndex);
}

BOOLEAN
InternalFileDevicePathsEqualWorker (
  IN FILEPATH_DEVICE_PATH  **FilePath1,
  IN FILEPATH_DEVICE_PATH  **FilePath2
  )
{
  UINTN   Clip1Index;
  UINTN   Clip2Index;
  UINTN   Len1;
  UINTN   Len2;
  UINTN   CurrentLen;
  UINTN   Index;
  CHAR16  Char1;
  CHAR16  Char2;
  BOOLEAN Result;

  ASSERT (FilePath1 != NULL);
  ASSERT (*FilePath1 != NULL);
  ASSERT (FilePath2 != NULL);
  ASSERT (*FilePath2 != NULL);

  ASSERT (IsDevicePathValid (&(*FilePath1)->Header, 0));
  ASSERT (IsDevicePathValid (&(*FilePath2)->Header, 0));

  Len1 = InternalFileDevicePathsEqualClipNode (FilePath1, &Clip1Index);
  Len2 = InternalFileDevicePathsEqualClipNode (FilePath2, &Clip2Index);

  do {
    if ((Len1 == 0) && (Len2 == 0)) {
      return TRUE;
    }

    CurrentLen = MIN (Len1, Len2);
    if (CurrentLen == 0) {
      return FALSE;
    }
    //
    // FIXME: Discuss case sensitivity.  For UEFI FAT, case insensitivity is
    //        guaranteed.
    //
    for (Index = 0; Index < CurrentLen; ++Index) {
      Char1 = CharToUpper ((*FilePath1)->PathName[Clip1Index + Index]);
      Char2 = CharToUpper ((*FilePath2)->PathName[Clip2Index + Index]);
      if (Char1 != Char2) {
        return FALSE;
      }
    }

    if (Len1 == Len2) {
      Len1 = InternalFileDevicePathsEqualClipNextNode (FilePath1, &Clip1Index);
      Len2 = InternalFileDevicePathsEqualClipNextNode (FilePath2, &Clip2Index);
    } else if (Len1 < Len2) {
      Len1 = InternalFileDevicePathsEqualClipNextNode (FilePath1, &Clip1Index);
      if (Len1 == 0) {
        return FALSE;
      }

      Len2       -= CurrentLen;
      Clip2Index += CurrentLen;
      //
      // Switching to the next node for the other Device Path implies a path
      // separator.  Verify we hit such in the currently walked path too.
      //
      Result = InternalFileDevicePathsEqualClipBottom (*FilePath2, &Len2, &Clip2Index);
      if (!Result) {
        return FALSE;
      }
    } else {
      Len2 = InternalFileDevicePathsEqualClipNextNode (FilePath2, &Clip2Index);
      if (Len2 == 0) {
        return FALSE;
      }

      Len1       -= CurrentLen;
      Clip1Index += CurrentLen;
      //
      // Switching to the next node for the other Device Path implies a path
      // separator.  Verify we hit such in the currently walked path too.
      //
      Result = InternalFileDevicePathsEqualClipBottom (*FilePath1, &Len1, &Clip1Index);
      if (!Result) {
        return FALSE;
      }
    }
  } while (TRUE);
}

/**
  Check whether File Device Paths are equal.

  @param[in] FilePath1  The first device path protocol to compare.
  @param[in] FilePath2  The second device path protocol to compare.

  @retval TRUE         The device paths matched
  @retval FALSE        The device paths were different
**/
BOOLEAN
FileDevicePathsEqual (
  IN FILEPATH_DEVICE_PATH  *FilePath1,
  IN FILEPATH_DEVICE_PATH  *FilePath2
  )
{
  ASSERT (FilePath1 != NULL);
  ASSERT (FilePath2 != NULL);

  return InternalFileDevicePathsEqualWorker (&FilePath1, &FilePath2);
}

STATIC
BOOLEAN
InternalDevicePathCmpWorker (
  IN EFI_DEVICE_PATH_PROTOCOL  *ParentPath,
  IN EFI_DEVICE_PATH_PROTOCOL  *ChildPath,
  IN BOOLEAN                   CheckChild
  )
{
  BOOLEAN          Result;
  INTN             CmpResult;

  EFI_DEV_PATH_PTR ChildPathPtr;
  EFI_DEV_PATH_PTR ParentPathPtr;

  UINT8            NodeType;
  UINT8            NodeSubType;
  UINTN            NodeSize;

  ASSERT (ParentPath != NULL);
  ASSERT (IsDevicePathValid (ParentPath, 0));
  ASSERT (ChildPath != NULL);
  ASSERT (IsDevicePathValid (ChildPath, 0));

  ParentPathPtr.DevPath = ParentPath;
  ChildPathPtr.DevPath  = ChildPath;

  while (TRUE) {
    NodeType    = DevicePathType (ChildPathPtr.DevPath);
    NodeSubType = DevicePathSubType (ChildPathPtr.DevPath);

    if (NodeType == END_DEVICE_PATH_TYPE) {
      //
      // We only support single-instance Device Paths.
      //
      ASSERT (NodeSubType == END_ENTIRE_DEVICE_PATH_SUBTYPE);
      return (CheckChild
          || (DevicePathType (ParentPathPtr.DevPath) == END_DEVICE_PATH_TYPE));
    }

    if ((DevicePathType (ParentPathPtr.DevPath) != NodeType)
     || (DevicePathSubType (ParentPathPtr.DevPath) != NodeSubType)) {
      return FALSE;
    }

    if ((NodeType == MEDIA_DEVICE_PATH)
     && (NodeSubType == MEDIA_FILEPATH_DP)) {
      //
      // File Paths need special consideration for prepended and appended
      // terminators, as well as multiple nodes.
      //
      Result = InternalFileDevicePathsEqualWorker (
                 &ParentPathPtr.FilePath,
                 &ChildPathPtr.FilePath
                 );
      if (!Result) {
        return FALSE;
      }
      //
      // InternalFileDevicePathsEqualWorker advances the nodes.
      //
    } else {
      NodeSize = DevicePathNodeLength (ChildPathPtr.DevPath);
      if (DevicePathNodeLength (ParentPathPtr.DevPath) != NodeSize) {
        return FALSE;
      }

      STATIC_ASSERT (
        (sizeof (*ChildPathPtr.DevPath) == 4),
        "The Device Path comparison logic depends on the entire header being checked"
        );

      CmpResult = CompareMem (
                    (ChildPathPtr.DevPath + 1),
                    (ParentPathPtr.DevPath + 1),
                    (NodeSize - sizeof (*ChildPathPtr.DevPath))
                    );
      if (CmpResult != 0) {
        return FALSE;
      }

      ParentPathPtr.DevPath = NextDevicePathNode (ParentPathPtr.DevPath);
      ChildPathPtr.DevPath  = NextDevicePathNode (ChildPathPtr.DevPath);
    }
  }
}

BOOLEAN
EFIAPI
IsDevicePathEqual (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath1,
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath2
  )
{
  return InternalDevicePathCmpWorker (DevicePath1, DevicePath2, FALSE);
}

BOOLEAN
EFIAPI
IsDevicePathChild (
  IN  EFI_DEVICE_PATH_PROTOCOL      *ParentPath,
  IN  EFI_DEVICE_PATH_PROTOCOL      *ChildPath
  )
{
  return InternalDevicePathCmpWorker (ParentPath, ChildPath, TRUE);
}

UINTN
OcFileDevicePathNameSize (
  IN CONST FILEPATH_DEVICE_PATH  *FilePath
  )
{
  ASSERT (FilePath != NULL);
  ASSERT (IsDevicePathValid (&FilePath->Header, 0));
  return (OcFileDevicePathNameLen (FilePath) + 1) * sizeof (*FilePath->PathName);
}

UINTN
OcFileDevicePathNameLen (
  IN CONST FILEPATH_DEVICE_PATH  *FilePath
  )
{
  UINTN Size;
  UINTN Len;

  ASSERT (FilePath != NULL);
  ASSERT (IsDevicePathValid (&FilePath->Header, 0));

  Size = DevicePathNodeLength (FilePath) - SIZE_OF_FILEPATH_DEVICE_PATH;
  //
  // Account for more than one termination character.
  //
  Len = (Size / sizeof (*FilePath->PathName)) - 1;
  while (Len > 0 && FilePath->PathName[Len - 1] == L'\0') {
    --Len;
  }

  return Len;
}

/**
  Retrieve the size of the full file path described by DevicePath.

  @param[in] DevicePath  The Device Path to inspect.

  @returns   The size of the full file path.
  @retval 0  DevicePath does not start with a File Path node or contains
             non-terminating nodes that are not File Path nodes.

**/
UINTN
OcFileDevicePathFullNameSize (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  UINTN                      PathSize;
  CONST FILEPATH_DEVICE_PATH *FilePath;

  ASSERT (DevicePath != NULL);
  ASSERT (IsDevicePathValid (DevicePath, 0));

  PathSize = 1;
  do {
    //
    // On the first iteration, this ensures the path is not immediately
    // terminated.
    //
    if (DevicePath->Type    != MEDIA_DEVICE_PATH
     || DevicePath->SubType != MEDIA_FILEPATH_DP) {
      return 0;
    }

    FilePath  = (FILEPATH_DEVICE_PATH *)DevicePath;
    PathSize += OcFileDevicePathNameLen (FilePath);

    DevicePath = NextDevicePathNode (DevicePath);
  } while (!IsDevicePathEnd (DevicePath));
  return PathSize * sizeof (*FilePath->PathName);
}

/**
  Retrieve the full file path described by FilePath.
  The caller is expected to call OcFileDevicePathFullNameSize() or ensure its
  guarantees are met.

  @param[out] PathName      On output, the full file path of FilePath.
  @param[in]  FilePath      The File Device Path to inspect.
  @param[in]  PathNameSize  The size, in bytes, of PathnName.  Must equal the
                            actual fill file path size.

**/
VOID
OcFileDevicePathFullName (
  OUT CHAR16                      *PathName,
  IN  CONST FILEPATH_DEVICE_PATH  *FilePath,
  IN  UINTN                       PathNameSize
  )
{
  UINTN                          PathLen;

  ASSERT (PathName != NULL);
  ASSERT (FilePath != NULL);
  ASSERT (IsDevicePathValid (&FilePath->Header, 0));
  ASSERT (PathNameSize == OcFileDevicePathFullNameSize (&FilePath->Header));

  //
  // FIXME: Insert separators between nodes if not present already.
  //

  do {
    PathLen = OcFileDevicePathNameLen (FilePath);
    CopyMem (
      PathName,
      FilePath->PathName,
      PathLen * sizeof (*FilePath->PathName)
      );
    PathName += PathLen;

    FilePath = (CONST FILEPATH_DEVICE_PATH *)NextDevicePathNode (FilePath);
  } while (!IsDevicePathEnd (FilePath));
  *PathName = CHAR_NULL;
}

EFI_DEVICE_PATH_PROTOCOL *
OcAppendDevicePathInstanceDedupe (
  IN EFI_DEVICE_PATH_PROTOCOL        *DevicePath OPTIONAL,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathInstance
  )
{
  INTN                           CmpResult;

  EFI_DEVICE_PATH_PROTOCOL       *DevPathWalker;
  EFI_DEVICE_PATH_PROTOCOL       *CurrentInstance;

  UINTN                          AppendInstanceSize;
  UINTN                          CurrentInstanceSize;

  ASSERT (DevicePathInstance != NULL);

  if (DevicePath != NULL) {
    AppendInstanceSize = GetDevicePathSize (DevicePathInstance);
    DevPathWalker = DevicePath;

    while (TRUE) {
      CurrentInstance = GetNextDevicePathInstance (
        &DevPathWalker,
        &CurrentInstanceSize
        );
      if (CurrentInstance == NULL) {
        break;
      }

      if (CurrentInstanceSize != AppendInstanceSize) {
        FreePool (CurrentInstance);
        continue;
      }

      CmpResult = CompareMem (
        CurrentInstance,
        DevicePathInstance,
        CurrentInstanceSize
        );

      FreePool (CurrentInstance);

      if (CmpResult == 0) {
        return DuplicateDevicePath (DevicePath);
      }
    }
  }

  return AppendDevicePathInstance (DevicePath, DevicePathInstance);
}

UINTN
OcGetNumDevicePathInstances (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  UINTN NumInstances;

  NumInstances = 1;

  while (!IsDevicePathEnd (DevicePath)) {
    if (IsDevicePathEndInstance (DevicePath)) {
      ++NumInstances;
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  return NumInstances;
}
