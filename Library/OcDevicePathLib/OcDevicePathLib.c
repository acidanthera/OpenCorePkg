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
    FileDevicePathNodeSize = (FileNameSize + sizeof (*FilePathNode) + sizeof (*DevicePath));
    FilePathNode           = AllocateZeroPool (FileDevicePathNodeSize);

    if (FilePathNode != NULL) {
      FilePathNode->Header.Type    = MEDIA_DEVICE_PATH;
      FilePathNode->Header.SubType = MEDIA_FILEPATH_DP;

      SetDevicePathNodeLength (&FilePathNode->Header, FileNameSize + sizeof (*FilePathNode));

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

VOID
DebugPrintDevicePath (
  IN UINTN                     ErrorLevel,
  IN CONST CHAR8               *Message,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  DEBUG_CODE_BEGIN();

  CHAR16 *TextDevicePath;

  TextDevicePath = ConvertDevicePathToText (DevicePath, TRUE, FALSE);
  DEBUG ((ErrorLevel, "%a - %s\n", Message, TextDevicePath));
  if (TextDevicePath != NULL) {
    FreePool (TextDevicePath);
  }

  DEBUG_CODE_END();
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
  CHAR16                    *Path;
  UINTN                     Length;
  UINTN                     Size;

  DevicePathWalker = DevicePath;

  while (!IsDevicePathEnd (DevicePathWalker)) {
    if ((DevicePathType (DevicePathWalker) == MEDIA_DEVICE_PATH)
     && (DevicePathSubType (DevicePathWalker) == MEDIA_FILEPATH_DP)
     && IsDevicePathEnd (NextDevicePathNode (DevicePathWalker))) {
      FilePath = (FILEPATH_DEVICE_PATH *) DevicePathWalker;
      Path     = FilePath->PathName;
      //
      // FIXME: Create an aligned copy.
      //
      ASSERT (((UINTN)Path & BIT0) == 0);
      Length   = StrLen (Path);

      if (Path[Length - 1] == L'\\') {
        //
        // Already appended, good. It should never be true with Apple entries though.
        //
        return NULL;
      } else if (Length > 4 &&        (Path[Length - 4] != '.'
        || (Path[Length - 3] != 'e' && Path[Length - 3] != 'E')
        || (Path[Length - 2] != 'f' && Path[Length - 2] != 'F')
        || (Path[Length - 1] != 'i' && Path[Length - 1] != 'I'))) {
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

    DevicePathWalker = NextDevicePathNode (DevicePathWalker);
  }

  //
  // Has .efi suffix or unsupported format.
  //
  return NULL;
}

/**
  Fix Apple Boot Device Path to be compatible with conventional UEFI
  implementations.

  @param[in,out] DevicePath  On input, a pointer to the device path to fix.
                             On output, the device path pointer is modified to
                             point to the remaining part of the device path.

  @retval -1     DevicePath could not be fixed.
  @retval other  The number of fixed nodes in DevicePath.

**/
INTN
OcFixAppleBootDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  INTN                     Result;

  EFI_DEVICE_PATH_PROTOCOL *OriginalDevPath;

  EFI_DEV_PATH_PTR         InvalidNode;
  UINT8                    NodeType;
  UINT8                    NodeSubType;
  UINTN                    NodeSize;

  EFI_DEVICE_PATH_PROTOCOL *RemainingDevPath;
  EFI_HANDLE               Device;

  ASSERT (DevicePath != NULL);
  ASSERT (*DevicePath != NULL);
  ASSERT (IsDevicePathValid (*DevicePath, 0));
  //
  // CAUTION: When adding new fixes, ensure short-form device paths are not
  //          modified and success is returned.
  //
  OriginalDevPath = *DevicePath;
  //
  // Failure will be returned explicitly within the loop.  If this loop is run
  // only once, it means the Device Path had already been valid.  Hence, Result
  // will be 0 on termination.  Shall any switch-case continue, which it needs
  // to in order to patch subsequent nodes, Result will be incremented.
  //
  Result = -1;
  while (TRUE) {
    if (Result != MAX_INTN) {
      ++Result;
    }

    RemainingDevPath = OriginalDevPath;
    gBS->LocateDevicePath (
           &gEfiDevicePathProtocolGuid,
           &RemainingDevPath,
           &Device
           );

    *DevicePath = RemainingDevPath;

    InvalidNode.DevPath = RemainingDevPath;
    NodeType    = DevicePathType (InvalidNode.DevPath);
    NodeSubType = DevicePathSubType (InvalidNode.DevPath);

    if (NodeType == MESSAGING_DEVICE_PATH) {
      switch (NodeSubType) {
        case MSG_SATA_DP:
        {
          if (InvalidNode.Sata->PortMultiplierPortNumber != 0xFFFF) {
            //
            // Must be set to 0xFFFF if the device is directly connected to the
            // HBA. This rule has been established by UEFI 2.5 via an Erratum
            // and has not been followed by Apple thus far.
            // Reference: AppleACPIPlatform.kext,
            //            appendSATADevicePathNodeForIOMedia
            //
            InvalidNode.Sata->PortMultiplierPortNumber = 0xFFFF;
            continue;
          }

          return -1;
        }

        case MSG_SASEX_DP:
        {
          OC_INLINE_STATIC_ASSERT (
            (sizeof (SASEX_DEVICE_PATH) != sizeof (NVME_NAMESPACE_DEVICE_PATH)),
            "SasEx and NVMe DPs must differ in size for fixing to be accurate."
            );
          //
          // Apple uses SubType 0x16 (SasEx) for NVMe, while the UEFI
          // Specification defines it as SubType 0x17. The structures are
          // identical.
          // Reference: AppleACPIPlatform.kext,
          //            appendNVMeDevicePathNodeForIOMedia
          //
          NodeSize = DevicePathNodeLength (InvalidNode.DevPath);
          if (NodeSize == sizeof (NVME_NAMESPACE_DEVICE_PATH)) {
            InvalidNode.SasEx->Header.SubType = MSG_NVME_NAMESPACE_DP;
            continue;
          }

          return -1;
        }

        default:
        {
          break;
        }
      }
    } else if (NodeType == ACPI_DEVICE_PATH) {
      //
      // Apple uses PciRoot (EISA 0x0A03) nodes while some firmwares might use
      // PcieRoot (EISA 0x0A08).
      //
      switch (NodeSubType) {
        case ACPI_DP:
        {
          if (EISA_ID_TO_NUM (InvalidNode.Acpi->HID) == 0x0A03) {
            InvalidNode.Acpi->HID = BitFieldWrite32 (
                                      InvalidNode.Acpi->HID,
                                      16,
                                      31,
                                      0x0A08
                                      );
            continue;
          }

          return -1;
        }

        case ACPI_EXTENDED_DP:
        {
          if (EISA_ID_TO_NUM (InvalidNode.ExtendedAcpi->HID) == 0x0A03) {
            InvalidNode.ExtendedAcpi->HID = BitFieldWrite32 (
                                              InvalidNode.ExtendedAcpi->HID,
                                              16,
                                              31,
                                              0x0A08
                                              );
            continue;
          }
          
          if (((EISA_ID_TO_NUM (InvalidNode.ExtendedAcpi->CID) == 0x0A03)
            && (EISA_ID_TO_NUM (InvalidNode.ExtendedAcpi->HID) != 0x0A08))) {
            InvalidNode.ExtendedAcpi->CID = BitFieldWrite32 (
                                              InvalidNode.ExtendedAcpi->CID,
                                              16,
                                              31,
                                              0x0A08
                                              );
            continue;
          }

          return -1;
        }

        default:
        {
          break;
        }
      }
    }

    return Result;
  }
}

STATIC
BOOLEAN
InternalFileDevicePathsEqualClipBottom (
  IN OUT UINTN   *FilePathLength,
  IN OUT CHAR16  **FilePath
  )
{
  CHAR16 *Start;

  ASSERT (FilePathLength != NULL);
  ASSERT (*FilePathLength != 0);
  ASSERT (FilePath != NULL);

  Start = *FilePath;
  if (*Start == L'\\') {
    *FilePath = (Start + 1);
    --(*FilePathLength);
    return TRUE;
  }

  return FALSE;
}

STATIC
UINTN
InternalFileDevicePathsEqualClipNode (
  IN  FILEPATH_DEVICE_PATH  **FilePath,
  OUT CHAR16                **PathName
  )
{
  EFI_DEV_PATH_PTR         DevPath;
  CHAR16                   *Start;

  UINTN                    Length;
  EFI_DEVICE_PATH_PROTOCOL *NextNode;

  ASSERT (FilePath != NULL);
  ASSERT (PathName != NULL);
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
      return 0;
    }

    Start  = DevPath.FilePath->PathName;
    //
    // FIXME: Create an aligned copy.
    //
    ASSERT (((UINTN)Start & BIT0) == 0);
    Length = StrLen (Start);
    if (Length > 0) {
      InternalFileDevicePathsEqualClipBottom (&Length, &Start);
      if ((Length > 0) && Start[Length - 1] == L'\\') {
        --Length;
      }
    }
  }

  *FilePath = DevPath.FilePath;
  *PathName = Start;
  return Length;
}

STATIC
UINTN
InternalFileDevicePathsEqualClipNextNode (
  IN  FILEPATH_DEVICE_PATH  **FilePath,
  OUT CHAR16                **PathName
  )
{
  ASSERT (FilePath != NULL);
  ASSERT (PathName != NULL);

  *FilePath = (FILEPATH_DEVICE_PATH *)NextDevicePathNode (*FilePath);
  return InternalFileDevicePathsEqualClipNode (FilePath, PathName);
}

BOOLEAN
InternalFileDevicePathsEqualWorker (
  IN FILEPATH_DEVICE_PATH  **FilePath1,
  IN FILEPATH_DEVICE_PATH  **FilePath2
  )
{
  CHAR16  *Clip1;
  CHAR16  *Clip2;
  UINTN   Len1;
  UINTN   Len2;
  UINTN   CurrentLen;
  INTN    CmpResult;
  BOOLEAN Result;

  ASSERT (FilePath1 != NULL);
  ASSERT (*FilePath1 != NULL);
  ASSERT (FilePath2 != NULL);
  ASSERT (*FilePath2 != NULL);

  ASSERT (IsDevicePathValid (&(*FilePath1)->Header, 0));
  ASSERT (IsDevicePathValid (&(*FilePath2)->Header, 0));

  Len1 = InternalFileDevicePathsEqualClipNode (FilePath1, &Clip1);
  Len2 = InternalFileDevicePathsEqualClipNode (FilePath2, &Clip2);

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
    CmpResult = StrniCmp (Clip1, Clip2, CurrentLen);
    if (CmpResult != 0) {
      return FALSE;
    }

    if (Len1 == Len2) {
      Len1 = InternalFileDevicePathsEqualClipNextNode (FilePath1, &Clip1);
      Len2 = InternalFileDevicePathsEqualClipNextNode (FilePath2, &Clip2);
    } else if (Len1 < Len2) {
      Len1 = InternalFileDevicePathsEqualClipNextNode (FilePath1, &Clip1);
      if (Len1 == 0) {
        return FALSE;
      }

      Len2  -= CurrentLen;
      Clip2 += CurrentLen;
      //
      // Switching to the next node for the other Device Path implies a path
      // separator.  Verify we hit such in the currently walked path too.
      //
      Result = InternalFileDevicePathsEqualClipBottom (&Len2, &Clip2);
      if (!Result) {
        return FALSE;
      }
    } else {
      Len2 = InternalFileDevicePathsEqualClipNextNode (FilePath2, &Clip2);
      if (Len2 == 0) {
        return FALSE;
      }

      Len1  -= CurrentLen;
      Clip1 += CurrentLen;
      //
      // Switching to the next node for the other Device Path implies a path
      // separator.  Verify we hit such in the currently walked path too.
      //
      Result = InternalFileDevicePathsEqualClipBottom (&Len1, &Clip1);
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

      OC_INLINE_STATIC_ASSERT (
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

/**
  Check whether device paths are equal.

  @param[in] DevicePath1  The first device path protocol to compare.
  @param[in] DevicePath2  The second device path protocol to compare.

  @retval TRUE         The device paths matched
  @retval FALSE        The device paths were different
**/
BOOLEAN
EFIAPI
IsDevicePathEqual (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath1,
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath2
  )
{
  return InternalDevicePathCmpWorker (DevicePath1, DevicePath2, FALSE);
}

/**
  Check whether one device path exists in the other.

  @param[in] ParentPath  The parent device path protocol to check against.
  @param[in] ChildPath   The device path protocol of the child device to compare.

  @retval TRUE         The child device path contains the parent device path.
  @retval FALSE        The device paths were different
**/
BOOLEAN
EFIAPI
IsDevicePathChild (
  IN  EFI_DEVICE_PATH_PROTOCOL      *ParentPath,
  IN  EFI_DEVICE_PATH_PROTOCOL      *ChildPath
  )
{
  return InternalDevicePathCmpWorker (ParentPath, ChildPath, TRUE);
}
