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

#include <Guid/FileInfo.h>

#include <Protocol/SimpleFileSystem.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "OcFileLibInternal.h"
#include "Macros.h"

// FileExists
/**

  @param[in] DevicePath  A pointer to the device path to check for the file.
  @param[in] FilePath    A pointer to the NULL terminated unicode file name.

  @retval TRUE   The file path was found on the device.
  @retval FALSE  The file path was not found on the device.
**/
BOOLEAN
FileExists (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CHAR16                    *FilePath
  )
{
  BOOLEAN       Exists;

  EFI_STATUS    Status;
  CHAR16        *DirectoryPath;
  CHAR16        *FileName;
  EFI_FILE_INFO *FileInfo;
  UINTN         FilePathSize;

  Exists = FALSE;

  if (DevicePath != NULL && FilePath != NULL) {
    FilePathSize  = StrSize (FilePath);
    DirectoryPath = AllocateZeroPool (FilePathSize);
    FileName      = (FilePath + StrLen (FilePath)) - 1;

    if (DirectoryPath != NULL) {

      // Split FilePath into DirectoryPath and FileName

      CHAR16    *TempDirectoryPath = DirectoryPath;
      UINTN     FilePathLen;

      while ((FileName > FilePath) && (*FileName != L'\\')) {
        --FileName;
      }

      ++FileName;

      FilePathLen = (FileName - FilePath);

      while ((FilePathLen > 0) && (*FilePath != 0)) {
        *(TempDirectoryPath++) = *(FilePath++);
        FilePathLen--;
      }
      *TempDirectoryPath = 0;

      if (*DirectoryPath != 0 && *FileName != 0) {

          FileInfo = NULL;
          Status   = GetFileInfo (
                       DevicePath,
                       DirectoryPath,
                       FileName,
                       &FileInfo
                       );

          FreePool ((VOID *)DirectoryPath);

          if (!EFI_ERROR (Status)) {
            Exists = TRUE;
          }
       }
    }
  }

  return Exists;
}

// FolderFileExists
/**

  @param[in] DevicePath     A pointer to the device path to check for the file.
  @param[in] DirectoryPath  A pointer to the NULL terminated ascii directory name.
  @param[in] FilePath       A pointer to the NULL terminated ascii file name.

  @retval TRUE   The file path was found on the device.
  @retval FALSE  The file path was not found on the device.
**/
BOOLEAN
FolderFileExists (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CHAR8                     *DirectoryPath OPTIONAL,
  IN CHAR8                     *FilePath OPTIONAL
  )
{
  BOOLEAN   Exists;
  CHAR16    *FullUnicodePath;
  CHAR16    *UnicodeFilePath;

  Exists = FALSE;

  if (DevicePath != NULL && (DirectoryPath != NULL || FilePath != NULL)) {
    FullUnicodePath = AllocateZeroPool (EFI_MAX_PATH_SIZE);
    if (FullUnicodePath != NULL) {
      UnicodeFilePath = FullUnicodePath;
      if (DirectoryPath != NULL) {
        OcAsciiStrToUnicode (DirectoryPath, UnicodeFilePath, 0);
        UnicodeFilePath += StrLen (UnicodeFilePath);
      }
      if (FilePath != NULL) {
        *(UnicodeFilePath++) = L'\\';
        OcAsciiStrToUnicode (FilePath, UnicodeFilePath, 0);
      }
      Exists = FileExists (DevicePath, FullUnicodePath);
      FreePool (FullUnicodePath);
    }
  }

  return Exists;
}
