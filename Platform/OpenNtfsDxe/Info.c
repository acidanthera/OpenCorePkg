/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#include "NTFS.h"
#include "Helper.h"

extern UINT64 mFileRecordSize;

STATIC
EFI_STATUS
GetLabel (
  IN     EFI_FS   *FileSystem,
  IN OUT CHAR16   **Label
  )
{
  EFI_STATUS       Status;
  NTFS_FILE        *BaseMftRecord  = NULL;
  ATTR_HEADER_RES  *Res;

  *Label = NULL;

  Status = FsHelpFindFile (L"/$Volume", FileSystem->RootIndex, &BaseMftRecord, FSHELP_REG);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "NTFS: Could not find $Volume file.\n"));
    return Status;
  }

  if (!BaseMftRecord->InodeRead) {
    BaseMftRecord->FileRecord = AllocateZeroPool (mFileRecordSize);
    if (BaseMftRecord->FileRecord == NULL) {
      DEBUG ((DEBUG_INFO, "Failed to allocate buffer for FileRecord.\n"));
      FreeFile (BaseMftRecord);
      FreePool (BaseMftRecord);
      return EFI_OUT_OF_RESOURCES;
    }

    Status = ReadMftRecord (BaseMftRecord->File, BaseMftRecord->FileRecord, BaseMftRecord->Inode);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_INFO, "NTFS: Could not read Mft Record #%d.\n", BaseMftRecord->Inode));
      FreeFile (BaseMftRecord);
      FreePool (BaseMftRecord);
      return Status;
    }
  }

  Status = InitAttr (&BaseMftRecord->Attr, BaseMftRecord);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "NTFS: Could not initialize attribute.\n"));
    FreeFile (BaseMftRecord);
    FreePool (BaseMftRecord);
    return Status;
  }

  Res = (ATTR_HEADER_RES *) FindAttr (&BaseMftRecord->Attr, AT_VOLUME_NAME);
  if ((Res != NULL) && (Res->NonResFlag == 0) && (Res->InfoLength != 0)) {
    //
    // The Volume Name is not terminated with a Unicode NULL.
    // Its name's length is the size of the attribute as stored in the header.
    //
    *Label = AllocateZeroPool (Res->InfoLength + sizeof (CHAR16));
    if (*Label == NULL) {
      DEBUG ((DEBUG_INFO, "Failed to allocate buffer for *Label\n"));
      FreeFile (BaseMftRecord);
      FreePool (BaseMftRecord);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (*Label, (UINT8 *) Res + Res->InfoOffset, Res->InfoLength);
  }

  FreeFile (BaseMftRecord);
  FreePool (BaseMftRecord);

  return EFI_SUCCESS;
}

/**
  Returns file info, system info, or the label of a volume
  depending on information type specified.

  @param  This   Pointer to this instance of file protocol
  @param  Type   Pointer to information type requested
  @param  Len    Pointer to size of buffer
  @param  Data   Pointer to buffer for returned info

  @retval EFI_STATUS  Status of the operation
**/
EFI_STATUS
EFIAPI
FileGetInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN EFI_GUID          *Type,
  IN OUT UINTN         *Len,
  OUT VOID             *Data
  )
{
  EFI_STATUS                    Status;
  EFI_NTFS_FILE                 *File;
  EFI_FILE_INFO                 *Info;
  EFI_FILE_SYSTEM_INFO          *FSInfo;
  EFI_FILE_SYSTEM_VOLUME_LABEL  *VLInfo;
  EFI_TIME                      Time;
  CHAR16                        *Label;
  UINTN                         Length = 0;

  ASSERT (This != NULL);
  ASSERT ((Data != NULL) || ((Data == NULL) && (*Len == 0)));

  File = (EFI_NTFS_FILE *) This;

  if (CompareMem (Type, &gEfiFileInfoGuid, sizeof (*Type)) == 0) {
    //
    // Fill in file information
    //
    Info = (EFI_FILE_INFO *) Data;

    if (File->BaseName != NULL) {
      Length = sizeof (EFI_FILE_INFO) + StrSize (File->BaseName);
    } else if (File->Path != NULL) {
      Length = sizeof (EFI_FILE_INFO) + StrSize (File->Path);
    } else {
      return EFI_VOLUME_CORRUPTED;
    }

    if (*Len < Length) {
      *Len = Length;
      return EFI_BUFFER_TOO_SMALL;
    }

    ZeroMem (Data, sizeof (EFI_FILE_INFO));

    Info->Size = (UINT64) Length;
    Info->Attribute = EFI_FILE_READ_ONLY;
    NtfsTimeToEfiTime (File->Mtime, &Time);
    CopyMem (&Info->CreateTime, &Time, sizeof (Time));
    CopyMem (&Info->LastAccessTime, &Time, sizeof (Time));
    CopyMem (&Info->ModificationTime, &Time, sizeof (Time));

    if (File->IsDir) {
      Info->Attribute |= EFI_FILE_DIRECTORY;
    } else {
      Info->FileSize = File->RootFile.DataAttributeSize;
      Info->PhysicalSize = File->RootFile.DataAttributeSize;
    }

    if (File->BaseName != NULL) {
      CopyMem (Info->FileName, File->BaseName, StrSize (File->BaseName));
    } else {
      CopyMem (Info->FileName, File->Path, StrSize (File->Path));
    }

    *Len = Length;

    return EFI_SUCCESS;
  } else if (CompareMem (Type, &gEfiFileSystemInfoGuid, sizeof (*Type)) == 0) {
    //
    // Get file system information
    //
    FSInfo = (EFI_FILE_SYSTEM_INFO *) Data;

    if (*Len < MINIMUM_FS_INFO_LENGTH) {
      *Len = MINIMUM_FS_INFO_LENGTH;
      return EFI_BUFFER_TOO_SMALL;
    }

    FSInfo->ReadOnly = 1;
    FSInfo->BlockSize = File->FileSystem->BlockIo->Media->BlockSize;
    if (FSInfo->BlockSize  == 0) {
      DEBUG((DEBUG_INFO, "NTFS: Corrected Media BlockSize\n"));
      FSInfo->BlockSize = 512;
    }
    FSInfo->VolumeSize = (File->FileSystem->BlockIo->Media->LastBlock + 1) * FSInfo->BlockSize;
    FSInfo->FreeSpace = 0; /* The device is Read Only */

    Length = *Len - sizeof (EFI_FILE_SYSTEM_INFO);
    Status = FileGetInfo (
      This,
      &gEfiFileSystemVolumeLabelInfoIdGuid,
      &Length,
      FSInfo->VolumeLabel
      );

    FSInfo->Size = (UINT64) Length + sizeof (EFI_FILE_SYSTEM_INFO);
    *Len = (UINTN) FSInfo->Size;

    return Status;
  } else if (CompareMem (Type, &gEfiFileSystemVolumeLabelInfoIdGuid, sizeof (*Type)) == 0) {
    //
    // Get the volume label
    //
    VLInfo = (EFI_FILE_SYSTEM_VOLUME_LABEL *)Data;

    Status = GetLabel (File->FileSystem, &Label);
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_INFO, "NTFS: Could not read disk label - %r\n", Status));
      return EFI_DEVICE_ERROR;
    }

    if (Label == NULL) {
      DEBUG((DEBUG_INFO, "NTFS: Volume has no label\n"));
      *Len = 0;
    } else {
      if (*Len < StrSize (Label)) {
        *Len = StrSize (Label);
        return EFI_BUFFER_TOO_SMALL;
      }

      *Len = StrSize (Label);
      CopyMem (VLInfo->VolumeLabel, Label, *Len);
    }

    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
}

EFI_STATUS
EFIAPI
FileSetInfo (
  IN EFI_FILE_PROTOCOL *This,
  IN EFI_GUID          *InformationType,
  IN UINTN             BufferSize,
  IN VOID              *Buffer
  )
{
  return EFI_WRITE_PROTECTED;
}
