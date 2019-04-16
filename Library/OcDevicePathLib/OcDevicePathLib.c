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

CHAR16 *
DevicePathToText (
  IN EFI_DEVICE_PATH_PROTOCOL       *DevicePath,
  IN BOOLEAN                        DisplayOnly,
  IN BOOLEAN                        AllowShortcuts
  )
{
  CHAR16                           *DevicePathString;
  EFI_DEVICE_PATH_PROTOCOL         *ShortDevicePath  = NULL;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DevicePathToText = NULL;
  EFI_STATUS                       Status;

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **)&DevicePathToText
                  );

  // TODO: Shorten the device path to the last node ?

  if (DisplayOnly == TRUE && AllowShortcuts == TRUE) {

    ShortDevicePath = FindDevicePathNodeWithType (
                        DevicePath,
                        MEDIA_DEVICE_PATH,
                        MEDIA_HARDDRIVE_DP);

  }

  if (ShortDevicePath == NULL) {
    ShortDevicePath = DevicePath;
  }

  DevicePathString = NULL;

  if (!EFI_ERROR (Status)) {
    DevicePathString  = DevicePathToText->ConvertDevicePathToText (
                                            ShortDevicePath,
                                            DisplayOnly,
                                            AllowShortcuts
                                            );
  }

  return DevicePathString;
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

BOOLEAN
EFIAPI
IsDevicePathEqual (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath1,
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath2
  )
{
  BOOLEAN         Equal;
  UINT8           Type1;
  UINT8           SubType1;
  UINTN           Len1;
  CHAR16          *FilePath1;
  CHAR16          *FilePath2;

  // TODO: Compare Normal & Short Device Paths

  Equal = FALSE;

  while (TRUE) {

    Type1 = DevicePathType (DevicePath1);
    SubType1 = DevicePathSubType (DevicePath1);
    Len1 = DevicePathNodeLength (DevicePath1);

    if (Type1 != DevicePathType (DevicePath2) ||
        SubType1 != DevicePathSubType (DevicePath2) ||
        Len1 != DevicePathNodeLength (DevicePath2))
    {

      // Compare short HD paths
      if (DevicePathType (DevicePath1) == MEDIA_DEVICE_PATH &&
          DevicePathSubType(DevicePath1) == MEDIA_HARDDRIVE_DP) {

        DevicePath2 = FindDevicePathNodeWithType (
                        DevicePath2,
                        MEDIA_DEVICE_PATH,
                        MEDIA_HARDDRIVE_DP);

      } else if (DevicePathType (DevicePath2) == MEDIA_DEVICE_PATH &&
                 DevicePathSubType (DevicePath2) == MEDIA_HARDDRIVE_DP) {

        DevicePath1 = FindDevicePathNodeWithType (
                        DevicePath1,
                        MEDIA_DEVICE_PATH,
                        MEDIA_HARDDRIVE_DP);
      } else {

        // Not equal
        break;

      }

      // Fall through with short HD paths to check

    }

    // Same type/subtype/len ...

    if (IsDevicePathEnd (DevicePath1) &&
        IsDevicePathEnd (DevicePath2))
    {
      // END node - they are the same
      Equal = TRUE;
      break;
    }

    // Do mem compare of nodes or special compare for selected types/subtypes
    if (Type1 == MEDIA_DEVICE_PATH && SubType1 == MEDIA_FILEPATH_DP) {

      // Special compare: case insensitive file path compare + skip leading \ char
      FilePath1 = &((FILEPATH_DEVICE_PATH *)DevicePath1)->PathName[0];

      if (FilePath1[0] == L'\\') {
        FilePath1++;
      }

      FilePath2 = &((FILEPATH_DEVICE_PATH *)DevicePath2)->PathName[0];
      if (FilePath2[0] == L'\\') {
        FilePath2++;
      }

      if (StrCmpiBasic (FilePath1, FilePath2) != 0) {
        // Not equal
        break;
      }

    } else {

      if (CompareMem (DevicePath1, DevicePath2, DevicePathNodeLength (DevicePath1)) != 0) {
        // Not equal
        break;
      }
    }

    // Advance to next node
    DevicePath1 =  NextDevicePathNode (DevicePath1);
    DevicePath2 =  NextDevicePathNode (DevicePath2);
  }

  return Equal;
}

BOOLEAN
EFIAPI
IsDeviceChild (
  IN  EFI_DEVICE_PATH_PROTOCOL      *ParentPath,
  IN  EFI_DEVICE_PATH_PROTOCOL      *ChildPath,
  IN  UINT8                         EndPathType
  )
{
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL    *ChildPathEndNode;

  BOOLEAN   Matched;

  DevicePath = DuplicateDevicePath (ChildPath);

  ChildPathEndNode = DevicePath;

  while (!IsDevicePathEndType (ChildPathEndNode) &&
         !(DevicePathType (ChildPathEndNode) == MEDIA_DEVICE_PATH &&
           DevicePathSubType (ChildPathEndNode) == EndPathType))

  {
    ChildPathEndNode = NextDevicePathNode (ChildPathEndNode);
  }

  SetDevicePathEndNode (ChildPathEndNode);

  Matched = IsDevicePathEqual (ParentPath, DevicePath);

  FreePool (DevicePath);

  return Matched;
}

VOID
DebugPrintDevicePath (
  IN UINTN                     ErrorLevel,
  IN CONST CHAR8               *Message,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  DEBUG_CODE_BEGIN();

  CHAR16   *TextDevicePath;
  BOOLEAN  PrintedOnce;
  BOOLEAN  EndsWithSlash;
  UINTN    Length;

  DEBUG ((ErrorLevel, "%a - ", Message));

  if (DevicePath == NULL) {
    DEBUG ((ErrorLevel, "<missing>\n", Message));
    return;
  }

  PrintedOnce = FALSE;
  EndsWithSlash = FALSE;
  while (!IsDevicePathEnd (DevicePath)) {
    TextDevicePath = ConvertDeviceNodeToText (DevicePath, TRUE, FALSE);
    if (TextDevicePath != NULL) {
      if (PrintedOnce && !EndsWithSlash) {
        DEBUG ((ErrorLevel, "\\%s", TextDevicePath));
      } else {
        DEBUG ((ErrorLevel, "%s", TextDevicePath));
      }

      Length = StrLen (TextDevicePath);
      EndsWithSlash = Length > 0 && TextDevicePath[Length - 1] == '\\';
      PrintedOnce = TRUE;
      FreePool (TextDevicePath);
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  if (!PrintedOnce) {
    DEBUG ((ErrorLevel, "<unconversible>\n"));
  } else {
    DEBUG ((ErrorLevel, "\n"));
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

STATIC
EFI_DEVICE_PATH_PROTOCOL *
InternalApplyAppleDevicePathFixes (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEV_PATH_PTR DevPath;

  ASSERT (DevicePath != NULL);

  DevPath.DevPath = FindDevicePathNodeWithType (
                      DevicePath,
                      MESSAGING_DEVICE_PATH,
                      MSG_SATA_DP
                      );
  if (DevPath.DevPath != NULL) {
    //
    // Must be set to 0xFFFF if the device is directly connected to the HBA.
    //
    DevPath.Sata->PortMultiplierPortNumber = 0xFFFF;
    return DevicePath;
  }
  //
  // FIXME: Add SASEx -> NVMe fix.
  //
  FreePool (DevicePath);
  return NULL;
}

/**
  Fix Apple Boot Device Path to be compatible with usual UEFI implementations.

  @param[in,out] DevicePath  The Device Path to fix.

  @retval DevicePath       DevicePath has been fixed.
  @retval new Device Path  DevicePath has been copied, fixed and freed.
  @retval NULL             DevicePath could not be fixed up and has been freed.

**/
EFI_DEVICE_PATH_PROTOCOL *
OcFixAppleBootDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS               Status;
  EFI_DEVICE_PATH_PROTOCOL *DevPath;
  EFI_HANDLE               Device;

  ASSERT (DevicePath != NULL);

  DevPath = DevicePath;
  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &DevPath,
                  &Device
                  );
  if (!EFI_ERROR (Status)) {
    return DevicePath;
  }

  DevicePath = InternalApplyAppleDevicePathFixes (DevicePath);
  if (DevicePath == NULL) {
    return NULL;
  }

  DevPath = DevicePath;
  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &DevPath,
                  &Device
                  );
  if (!EFI_ERROR (Status)) {
    return DevicePath;
  }

  FreePool (DevicePath);
  return NULL;
}
