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
#include <Library/OcProtocolLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>

// AppendFileNameDevicePath
/**

  @param[in] DevicePath  The device path which to append the file path.
  @param[in] FileName    The file name to append to the device path.

  @retval EFI_SUCCESS            The defaults were initialized successfully.
  @retval EFI_INVALID_PARAMETER  The parameters passed were invalid.
  @retval EFI_OUT_OF_RESOURCES   The system ran out of memory.
**/
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

// DevicePathToText
/**

  @param[in] StorageDevicePath  The device path to convert to unicode string.
  @param[in] DisplayOnly
  @param[in] AllowShortcuts

  @retval EFI_SUCCESS            The defaults were initialized successfully.
  @retval EFI_INVALID_PARAMETER  The parameters passed were invalid.
  @retval EFI_OUT_OF_RESOURCES   The system ran out of memory.
**/
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

  Status = SafeLocateProtocol (
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

// FindDevicePathNodeWithType
/**

  @param[in] DevicePath  The device path used in the search.
  @param[in] Type        The Type field of the device path node specified by Node.
  @param[in] SubType     The SubType field of the device path node specified by Node.

  @return  Returned is the first Device Path Node with the given type.
**/
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

// IsDevicePathEqual
/** 

  @param[in] DevicePath1 The first device path protocol to compare.
  @param[in] DevicePath2 The second device path protocol to compare.

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

// IsDeviceChild
/** 

  @param[in] ParentPath  The parent device path protocol to check against.
  @param[in] ChildPath   The device path protocol of the child device to compare.

  @retval TRUE         The child device path contains the parent device path.
  @retval FALSE        The device paths were different
**/
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

EFI_DEVICE_PATH_PROTOCOL *
LocateFileSystemDevicePath (
  IN  EFI_HANDLE                         DeviceHandle  OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL           *FilePath     OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *FileSystemPath;

  if (DeviceHandle == NULL) {
    //
    // Locate DeviceHandle if we have none (idea by dmazar).
    //
    if (FilePath == NULL) {
      DEBUG ((DEBUG_WARN, "No device handle or path to proceed\n"));
      return NULL;
    }

    Status = gBS->LocateDevicePath (
      &gEfiSimpleFileSystemProtocolGuid,
      &FilePath,
      &DeviceHandle
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "Failed to locate device handle over path - %r\n", Status));
      return NULL;
    }
  }

  FileSystemPath = DevicePathFromHandle (DeviceHandle);

  if (FileSystemPath == NULL) {
    DEBUG ((DEBUG_WARN, "Failed to locate simple fs on handle %p\n", DeviceHandle));

    //
    // Retry by looking up the handle based on FilePath.
    //
    if (FilePath != NULL) {
      DEBUG ((DEBUG_INFO, "Retrying to locate fs with NULL handle\n"));

      return LocateFileSystemDevicePath (
        NULL,
        FilePath
        );
    }
  }

  return FileSystemPath;
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
