/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#include "NTFS.h"
#include "Helper.h"

EFI_STATUS
NtfsDir (
  IN  EFI_FS         *FileSystem,
  IN  CONST CHAR16   *Path,
  OUT EFI_NTFS_FILE  *File,
  IN  FUNCTION_TYPE  FunctionType
  )
{
  EFI_STATUS  Status;
  NTFS_FILE   *Dir;

  ASSERT (FileSystem != NULL);
  ASSERT (Path != NULL);
  ASSERT (File != NULL);

  Dir = NULL;

  CopyMem (&File->RootFile, FileSystem->RootIndex, sizeof (File->RootFile));
  CopyMem (&File->MftFile, FileSystem->MftStart, sizeof (File->MftFile));

  Status = FsHelpFindFile (
             Path,
             &File->RootFile,
             &Dir,
             FSHELP_DIR
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = IterateDir (Dir, File, FunctionType);

  if (Dir != &File->RootFile) {
    FreeFile (Dir);
    FreePool (Dir);
  }

  return Status;
}

EFI_STATUS
NtfsOpen (
  IN EFI_NTFS_FILE  *File
  )
{
  EFI_STATUS  Status;
  NTFS_FILE   *BaseMftRecord;

  ASSERT (File != NULL);

  BaseMftRecord = NULL;

  CopyMem (&File->RootFile, File->FileSystem->RootIndex, sizeof (File->RootFile));
  CopyMem (&File->MftFile, File->FileSystem->MftStart, sizeof (File->MftFile));

  Status = FsHelpFindFile (
             File->Path,
             &File->RootFile,
             &BaseMftRecord,
             FSHELP_REG
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BaseMftRecord != &File->RootFile) {
    CopyMem (&File->RootFile, BaseMftRecord, sizeof (*BaseMftRecord));

    if (!File->RootFile.InodeRead) {
      Status = InitFile (&File->RootFile, File->RootFile.Inode);
    }

    FreeFile (BaseMftRecord);
    FreePool (BaseMftRecord);
  }

  File->Offset = 0;

  return Status;
}

EFI_STATUS
NtfsMount (
  IN EFI_FS  *FileSystem
  )
{
  EFI_STATUS      Status;
  BOOT_FILE_DATA  Boot;
  UINTN           Size;
  EFI_NTFS_FILE   *RootFile;

  ASSERT (FileSystem != NULL);

  Status = DiskRead (FileSystem, 0, sizeof (Boot), &Boot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (  (Boot.SystemId[0] != SIGNATURE_32 ('N', 'T', 'F', 'S'))
     || (Boot.SectorsPerCluster == 0)
     || ((Boot.SectorsPerCluster & (Boot.SectorsPerCluster - 1U)) != 0)
     || (Boot.BytesPerSector == 0)
     || ((Boot.BytesPerSector & (Boot.BytesPerSector - 1U)) != 0))
  {
    DEBUG ((DEBUG_INFO, "NTFS: (NtfsMount #1) BIOS Parameter Block is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  FileSystem->SectorSize  = (UINTN)Boot.BytesPerSector;
  FileSystem->ClusterSize = (UINTN)Boot.SectorsPerCluster * FileSystem->SectorSize;

  if (Boot.MftRecordClusters > 0) {
    Size = (UINTN)Boot.MftRecordClusters * FileSystem->ClusterSize;
  } else if (-Boot.MftRecordClusters >= 31) {
    DEBUG ((DEBUG_INFO, "NTFS: (NtfsMount #2) BIOS Parameter Block is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  } else {
    Size = (UINTN)LShiftU64 (1ULL, -Boot.MftRecordClusters);
  }

  FileSystem->FileRecordSize = Size;
  if (FileSystem->FileRecordSize < FileSystem->SectorSize) {
    DEBUG ((DEBUG_INFO, "NTFS: File Record is smaller than Sector.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  if (Boot.IndexRecordClusters > 0) {
    Size = (UINTN)Boot.IndexRecordClusters * FileSystem->ClusterSize;
  } else if (-Boot.IndexRecordClusters >= 31) {
    DEBUG ((DEBUG_INFO, "NTFS: (NtfsMount #3) BIOS Parameter Block is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  } else {
    Size = (UINTN)LShiftU64 (1ULL, -Boot.IndexRecordClusters);
  }

  FileSystem->IndexRecordSize = Size;
  if (FileSystem->IndexRecordSize < FileSystem->SectorSize) {
    DEBUG ((DEBUG_INFO, "NTFS: Index Record is smaller than Sector.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  FileSystem->FirstMftRecord = Boot.MftLcn * FileSystem->ClusterSize;
  //
  // Driver limitations
  //
  if (  (FileSystem->FileRecordSize > NTFS_MAX_MFT)
     || (FileSystem->IndexRecordSize > NTFS_MAX_IDX))
  {
    DEBUG ((DEBUG_INFO, "NTFS: (NtfsMount #4) BIOS Parameter Block is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  RootFile = AllocateZeroPool (sizeof (*RootFile));
  if (RootFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (RootFile, &FileSystem->EfiFile, sizeof (FileSystem->EfiFile));

  RootFile->IsDir         = TRUE;
  RootFile->Path          = L"/";
  RootFile->RefCount      = 1U;
  RootFile->FileSystem    = FileSystem;
  RootFile->RootFile.File = RootFile;
  RootFile->MftFile.File  = RootFile;

  RootFile->MftFile.FileRecord = AllocateZeroPool (FileSystem->FileRecordSize);
  if (RootFile->MftFile.FileRecord == NULL) {
    FreePool (RootFile);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = DiskRead (
             FileSystem,
             FileSystem->FirstMftRecord,
             FileSystem->FileRecordSize,
             RootFile->MftFile.FileRecord
             );
  if (EFI_ERROR (Status)) {
    FreePool (RootFile->MftFile.FileRecord);
    FreePool (RootFile);
    return Status;
  }

  Status = Fixup (
             RootFile->MftFile.FileRecord,
             FileSystem->FileRecordSize,
             SIGNATURE_32 ('F', 'I', 'L', 'E'),
             FileSystem->SectorSize
             );
  if (EFI_ERROR (Status)) {
    FreePool (RootFile->MftFile.FileRecord);
    FreePool (RootFile);
    return Status;
  }

  if (LocateAttr (&RootFile->MftFile.Attr, &RootFile->MftFile, AT_DATA) == NULL) {
    FreePool (RootFile->MftFile.FileRecord);
    FreePool (RootFile);
    return EFI_VOLUME_CORRUPTED;
  }

  Status = InitFile (&RootFile->RootFile, ROOT_FILE);
  if (EFI_ERROR (Status)) {
    FreePool (RootFile->MftFile.FileRecord);
    FreePool (RootFile);
    return Status;
  }

  FileSystem->RootIndex = &RootFile->RootFile;
  FileSystem->MftStart  = &RootFile->MftFile;

  return EFI_SUCCESS;
}

/**
   Table 4.21. Layout of a File Record
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   0x00   | 4    | Magic number 'FILE'
   0x04   | 2    | Offset to the update sequence
   0x06   | 2    | Size in words of Update Sequence Number & Array (S)
   ...
          | 2    | Update Sequence Number (a)
          | 2S-2 | Update Sequence Array (a)
   --------------------------------------------------------------------
**/
EFI_STATUS
EFIAPI
Fixup (
  IN UINT8   *Buffer,
  IN UINT64  Length,
  IN UINT32  Magic,
  IN UINTN   SectorSize
  )
{
  FILE_RECORD_HEADER  *Record;
  UINT8               *UpdateSequencePointer;
  UINT16              UpdateSequenceNumber;
  UINT64              USCounter;
  UINT8               *BufferEnd;

  ASSERT (Buffer != NULL);

  Record = (FILE_RECORD_HEADER *)Buffer;

  if (Length < sizeof (*Record)) {
    DEBUG ((DEBUG_INFO, "NTFS: (Fixup #1) Record is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  if (Record->Magic != Magic) {
    DEBUG ((DEBUG_INFO, "NTFS: (Fixup #2) Record is corrupted.\n"));
    return EFI_NOT_FOUND;
  }

  if ((Record->UpdateSequenceOffset + sizeof (UINT16)) > Length) {
    DEBUG ((DEBUG_INFO, "NTFS: (Fixup #3) Record is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  if (((UINT64)Record->S_Size - 1U) != DivU64x64Remainder (Length, SectorSize, NULL)) {
    DEBUG ((DEBUG_INFO, "NTFS: (Fixup #4) Record is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  UpdateSequencePointer = Buffer + Record->UpdateSequenceOffset;
  UpdateSequenceNumber  = ReadUnaligned16 ((UINT16 *)UpdateSequencePointer);
  USCounter             = Record->UpdateSequenceOffset;

  if (Length < (SectorSize - sizeof (UINT16))) {
    DEBUG ((DEBUG_INFO, "NTFS: (Fixup #5) Record is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  BufferEnd = Buffer + Length;
  Buffer   += SectorSize - sizeof (UINT16);
  while ((Buffer + sizeof (UINT16)) <= BufferEnd) {
    UpdateSequencePointer += sizeof (UINT16);

    USCounter += sizeof (UINT16);
    if ((USCounter + sizeof (UINT16)) > Length) {
      DEBUG ((DEBUG_INFO, "NTFS: (Fixup #6) Record is corrupted.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    if (ReadUnaligned16 ((UINT16 *)Buffer) != UpdateSequenceNumber) {
      DEBUG ((DEBUG_INFO, "NTFS: (Fixup #7) Record is corrupted.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    Buffer[0] = UpdateSequencePointer[0];
    Buffer[1] = UpdateSequencePointer[1];
    Buffer   += SectorSize;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InitAttr (
  OUT NTFS_ATTR  *Attr,
  IN  NTFS_FILE  *File
  )
{
  FILE_RECORD_HEADER  *Record;
  UINT64              AttrEnd;

  ASSERT (Attr != NULL);
  ASSERT (File != NULL);

  Record = (FILE_RECORD_HEADER *)File->FileRecord;

  Attr->BaseMftRecord = File;
  Attr->Flags         = (File == &File->File->MftFile) ? NTFS_AF_MFT_FILE : 0;

  AttrEnd = Record->AttributeOffset + sizeof (ATTR_HEADER_RES);
  if (  (AttrEnd > File->File->FileSystem->FileRecordSize)
     || (AttrEnd > Record->RealSize)
     || (Record->RealSize > Record->AllocatedSize))
  {
    DEBUG ((DEBUG_INFO, "NTFS: (InitAttr) File record is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  Attr->Next               = File->FileRecord + Record->AttributeOffset;
  Attr->Last               = NULL;
  Attr->ExtensionMftRecord = NULL;
  Attr->NonResAttrList     = NULL;

  return EFI_SUCCESS;
}

UINT8 *
LocateAttr (
  IN NTFS_ATTR  *Attr,
  IN NTFS_FILE  *Mft,
  IN UINT32     Type
  )
{
  EFI_STATUS  Status;
  UINT8       *AttrStart;

  ASSERT (Attr != NULL);
  ASSERT (Mft  != NULL);

  Status = InitAttr (Attr, Mft);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  AttrStart = FindAttr (Attr, Type);
  if (AttrStart == NULL) {
    FreeAttr (Attr);
    return NULL;
  }

  if ((Attr->Flags & NTFS_AF_ALST) == 0) {
    while (TRUE) {
      AttrStart = FindAttr (Attr, Type);
      if (AttrStart == NULL) {
        break;
      }

      if ((Attr->Flags & NTFS_AF_ALST) != 0) {
        return AttrStart;
      }
    }

    FreeAttr (Attr);

    Status = InitAttr (Attr, Mft);
    if (EFI_ERROR (Status)) {
      return NULL;
    }

    AttrStart = FindAttr (Attr, Type);
    if (AttrStart == NULL) {
      FreeAttr (Attr);
      return NULL;
    }
  }

  return AttrStart;
}

UINT8 *
FindAttr (
  IN NTFS_ATTR  *Attr,
  IN UINT32     Type
  )
{
  EFI_STATUS          Status;
  UINT8               *AttrStart;
  ATTR_HEADER_RES     *Res;
  ATTR_HEADER_NONRES  *NonRes;
  ATTR_LIST_RECORD    *LRecord;
  FILE_RECORD_HEADER  *FRecord;
  UINT64              BufferSize;
  UINTN               FileRecordSize;
  UINTN               SectorSize;

  ASSERT (Attr != NULL);

  BufferSize     = 0;
  FileRecordSize = Attr->BaseMftRecord->File->FileSystem->FileRecordSize;
  SectorSize     = Attr->BaseMftRecord->File->FileSystem->SectorSize;

  if ((Attr->Flags & NTFS_AF_ALST) != 0) {
retry:
    while ((Attr->Next + sizeof (*LRecord)) <= Attr->Last) {
      Attr->Current = Attr->Next;
      LRecord       = (ATTR_LIST_RECORD *)Attr->Current;

      if (BufferSize < LRecord->RecordLength) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #0) $ATTRIBUTE_LIST is corrupted.\n"));
        FreeAttr (Attr);
        return NULL;
      }

      Attr->Next += LRecord->RecordLength;
      BufferSize -= LRecord->RecordLength;
      if (Attr->Next <= Attr->Current) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #1) $ATTRIBUTE_LIST is corrupted.\n"));
        FreeAttr (Attr);
        return NULL;
      }

      if ((LRecord->Type == Type) || (Type == 0)) {
        if (Attr->Flags & NTFS_AF_MFT_FILE) {
          Status = DiskRead (
                     Attr->BaseMftRecord->File->FileSystem,
                     ReadUnaligned32 ((UINT32 *)(Attr->Current + 0x10U)),
                     FileRecordSize / 2U,
                     Attr->ExtensionMftRecord
                     );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_INFO, "NTFS: Could not read first part of extension record.\n"));
            FreeAttr (Attr);
            return NULL;
          }

          Status = DiskRead (
                     Attr->BaseMftRecord->File->FileSystem,
                     ReadUnaligned32 ((UINT32 *)(Attr->Current + 0x14U)),
                     FileRecordSize / 2U,
                     Attr->ExtensionMftRecord + FileRecordSize / 2U
                     );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_INFO, "NTFS: Could not read second part of extension record.\n"));
            FreeAttr (Attr);
            return NULL;
          }

          Status = Fixup (
                     Attr->ExtensionMftRecord,
                     FileRecordSize,
                     SIGNATURE_32 ('F', 'I', 'L', 'E'),
                     Attr->BaseMftRecord->File->FileSystem->SectorSize
                     );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_INFO, "NTFS: Fixup failed.\n"));
            FreeAttr (Attr);
            return NULL;
          }
        } else {
          Status = ReadMftRecord (
                     Attr->BaseMftRecord->File,
                     Attr->ExtensionMftRecord,
                     (UINT32)LRecord->BaseFileReference
                     );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_INFO, "NTFS: Could not read extension record.\n"));
            FreeAttr (Attr);
            return NULL;
          }
        }

        FRecord    = (FILE_RECORD_HEADER *)Attr->ExtensionMftRecord;
        BufferSize = FileRecordSize;
        if ((FRecord->AttributeOffset + sizeof (*Res)) <= BufferSize) {
          Res         = (ATTR_HEADER_RES *)((UINT8 *)FRecord + FRecord->AttributeOffset);
          BufferSize -= FRecord->AttributeOffset;
        } else {
          DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #1) Extension record is corrupted.\n"));
          FreeAttr (Attr);
          return NULL;
        }

        while ((BufferSize >= sizeof (UINT32)) && (Res->Type != ATTRIBUTES_END_MARKER)) {
          if (BufferSize < sizeof (*Res)) {
            DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #2) Extension record is corrupted.\n"));
            FreeAttr (Attr);
            return NULL;
          }

          if (  (Res->Type == LRecord->Type)
             && (Res->AttributeId == LRecord->AttributeId))
          {
            return (UINT8 *)Res;
          }

          if ((Res->Length == 0) || (Res->Length >= BufferSize)) {
            DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #3) Extension record is corrupted.\n"));
            FreeAttr (Attr);
            return NULL;
          }

          BufferSize -= Res->Length;
          Res         = (ATTR_HEADER_RES *)((UINT8 *)Res + Res->Length);
        }

        DEBUG ((DEBUG_INFO, "NTFS: Can\'t find 0x%X in attribute list\n", Attr->Current));
        FreeAttr (Attr);
        return NULL;
      }
    }

    FreeAttr (Attr);
    return NULL;
  }

  Attr->Current = Attr->Next;
  Res           = (ATTR_HEADER_RES *)Attr->Current;
  BufferSize    = FileRecordSize - (Attr->Current - Attr->BaseMftRecord->FileRecord);

  while ((BufferSize >= sizeof (UINT32)) && (Res->Type != ATTRIBUTES_END_MARKER)) {
    if (BufferSize < sizeof (*Res)) {
      DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #1) File record is corrupted.\n"));
      FreeAttr (Attr);
      return NULL;
    }

    if ((Res->Length == 0) || (Res->Length >= BufferSize)) {
      DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #2) File record is corrupted.\n"));
      FreeAttr (Attr);
      return NULL;
    }

    BufferSize -= Res->Length;
    Attr->Next += Res->Length;

    if (Res->Type == AT_ATTRIBUTE_LIST) {
      Attr->Last = Attr->Current;
    }

    if ((Res->Type == Type) || (Type == 0)) {
      return Attr->Current;
    }

    Attr->Current = Attr->Next;
    Res           = (ATTR_HEADER_RES *)Attr->Current;
  }

  //
  // Continue search in $ATTRIBUTE_LIST
  //
  if (Attr->Last != NULL) {
    if (Attr->ExtensionMftRecord != NULL) {
      FreePool (Attr->ExtensionMftRecord);
    }

    Attr->ExtensionMftRecord = AllocateZeroPool (FileRecordSize);
    if (Attr->ExtensionMftRecord == NULL) {
      FreeAttr (Attr);
      return NULL;
    }

    AttrStart  = Attr->Last;
    Res        = (ATTR_HEADER_RES *)Attr->Last;
    BufferSize = FileRecordSize - (Attr->Last - Attr->BaseMftRecord->FileRecord);

    if (BufferSize < sizeof (*Res)) {
      DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #3) File record is corrupted.\n"));
      FreeAttr (Attr);
      return NULL;
    }

    if (Res->NonResFlag != 0) {
      NonRes        = (ATTR_HEADER_NONRES *)Res;
      Attr->Current = (UINT8 *)NonRes;

      if (BufferSize < sizeof (*NonRes)) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #4) File record is corrupted.\n"));
        FreeAttr (Attr);
        return NULL;
      }

      if (NonRes->RealSize > MAX_FILE_SIZE) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr) File is too huge.\n"));
        FreeAttr (Attr);
        return NULL;
      }

      if (Attr->NonResAttrList != NULL) {
        FreePool (Attr->NonResAttrList);
      }

      Attr->NonResAttrList = AllocateZeroPool ((UINTN)NonRes->RealSize);
      if (Attr->NonResAttrList == NULL) {
        FreeAttr (Attr);
        return NULL;
      }

      Status = ReadData (Attr, AttrStart, Attr->NonResAttrList, 0, (UINTN)NonRes->RealSize);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "NTFS: Failed to read non-resident attribute list\n"));
        FreeAttr (Attr);
        return NULL;
      }

      Attr->Next = Attr->NonResAttrList;
      Attr->Last = Attr->NonResAttrList + NonRes->RealSize;
      BufferSize = NonRes->RealSize;
    } else {
      if ((Res->InfoOffset < Res->Length) && (Res->Length <= BufferSize)) {
        Attr->Next  = (UINT8 *)Res + Res->InfoOffset;
        Attr->Last  = (UINT8 *)Res + Res->Length;
        BufferSize -= Res->InfoOffset;
      } else {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #5) File record is corrupted.\n"));
        FreeAttr (Attr);
        return NULL;
      }
    }

    Attr->Flags |= NTFS_AF_ALST;
    LRecord      = (ATTR_LIST_RECORD *)Attr->Next;
    while ((Attr->Next + sizeof (*LRecord)) < Attr->Last) {
      if ((LRecord->Type == Type) || (Type == 0)) {
        break;
      }

      if (BufferSize < LRecord->RecordLength) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #2) $ATTRIBUTE_LIST is corrupted.\n"));
        FreeAttr (Attr);
        return NULL;
      }

      if (LRecord->RecordLength == 0) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #3) $ATTRIBUTE_LIST is corrupted.\n"));
        FreeAttr (Attr);
        return NULL;
      }

      Attr->Next += LRecord->RecordLength;
      BufferSize -= LRecord->RecordLength;
      LRecord     = (ATTR_LIST_RECORD *)Attr->Next;
    }

    if ((Attr->Next + sizeof (*LRecord)) >= Attr->Last) {
      FreeAttr (Attr);
      return NULL;
    }

    if ((Attr->Flags & NTFS_AF_MFT_FILE) && (Type == AT_DATA)) {
      Attr->Flags  |= NTFS_AF_GPOS;
      Attr->Current = Attr->Next;
      AttrStart     = Attr->Current;

      if (BufferSize >= 0x18U) {
        WriteUnaligned32 (
          (UINT32 *)(AttrStart + 0x10U),
          (UINT32)DivU64x64Remainder (Attr->BaseMftRecord->File->FileSystem->FirstMftRecord, SectorSize, NULL)
          );
        WriteUnaligned32 (
          (UINT32 *)(AttrStart + 0x14U),
          (UINT32)DivU64x64Remainder (Attr->BaseMftRecord->File->FileSystem->FirstMftRecord, SectorSize, NULL) + 1U
          );
      } else {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #7) File record is corrupted.\n"));
        FreeAttr (Attr);
        return NULL;
      }

      AttrStart = Attr->Next + ReadUnaligned16 ((UINT16 *)(AttrStart + 0x04U));
      while ((AttrStart + sizeof (UINT32) + sizeof (UINT16)) < Attr->Last) {
        if (ReadUnaligned32 ((UINT32 *)AttrStart) != Type) {
          break;
        }

        Status = ReadAttr (
                   Attr,
                   AttrStart + 0x10U,
                   ReadUnaligned32 ((UINT32 *)(AttrStart + 0x10U)) * FileRecordSize,
                   FileRecordSize
                   );
        if (EFI_ERROR (Status)) {
          FreeAttr (Attr);
          return NULL;
        }

        if (ReadUnaligned16 ((UINT16 *)(AttrStart + 4U)) == 0) {
          DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #8) File record is corrupted.\n"));
          FreeAttr (Attr);
          return NULL;
        }

        AttrStart += ReadUnaligned16 ((UINT16 *)(AttrStart + 4U));
      }

      Attr->Next   = Attr->Current;
      Attr->Flags &= ~NTFS_AF_GPOS;
    }

    goto retry;
  }

  FreeAttr (Attr);
  return NULL;
}

EFI_STATUS
EFIAPI
InitFile (
  IN OUT NTFS_FILE  *File,
  IN     UINT64     RecordNumber
  )
{
  EFI_STATUS          Status;
  ATTR_HEADER_RES     *Attr;
  FILE_RECORD_HEADER  *Record;

  ASSERT (File != NULL);

  File->InodeRead = TRUE;

  File->FileRecord = AllocateZeroPool (File->File->FileSystem->FileRecordSize);
  if (File->FileRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = ReadMftRecord (File->File, File->FileRecord, RecordNumber);
  if (EFI_ERROR (Status)) {
    FreePool (File->FileRecord);
    File->FileRecord = NULL;
    return Status;
  }

  Record = (FILE_RECORD_HEADER *)File->FileRecord;

  if ((Record->Flags & ~IS_SUPPORTED_FLAGS) != 0) {
    DEBUG ((DEBUG_INFO, "NTFS: MFT Record 0x%Lx has invalid or unsupported flags\n", RecordNumber));
    FreePool (File->FileRecord);
    File->FileRecord = NULL;
    return EFI_VOLUME_CORRUPTED;
  }

  if ((Record->Flags & IS_IN_USE) == 0) {
    DEBUG ((DEBUG_INFO, "NTFS: MFT Record 0x%Lx is not in use\n", RecordNumber));
    FreePool (File->FileRecord);
    File->FileRecord = NULL;
    return EFI_VOLUME_CORRUPTED;
  }

  if ((Record->Flags & IS_A_DIRECTORY) == 0) {
    Attr = (ATTR_HEADER_RES *)LocateAttr (&File->Attr, File, AT_DATA);
    if (Attr == NULL) {
      DEBUG ((DEBUG_INFO, "NTFS: No $DATA in MFT Record 0x%Lx\n", RecordNumber));
      FreePool (File->FileRecord);
      File->FileRecord = NULL;
      return EFI_VOLUME_CORRUPTED;
    }

    if (Attr->NonResFlag == 0) {
      File->DataAttributeSize = Attr->InfoLength;
    } else {
      File->DataAttributeSize = ((ATTR_HEADER_NONRES *)Attr)->RealSize;
    }

    if ((File->Attr.Flags & NTFS_AF_ALST) == 0) {
      File->Attr.Last = 0;
    }
  } else {
    Status = InitAttr (&File->Attr, File);
    if (EFI_ERROR (Status)) {
      FreePool (File->FileRecord);
      File->FileRecord = NULL;
      return Status;
    }
  }

  return EFI_SUCCESS;
}

VOID
FreeAttr (
  IN NTFS_ATTR  *Attr
  )
{
  ASSERT (Attr != NULL);

  if (Attr->ExtensionMftRecord != NULL) {
    FreePool (Attr->ExtensionMftRecord);
    Attr->ExtensionMftRecord = NULL;
  }

  if (Attr->NonResAttrList != NULL) {
    FreePool (Attr->NonResAttrList);
    Attr->NonResAttrList = NULL;
  }
}

VOID
FreeFile (
  IN NTFS_FILE  *File
  )
{
  ASSERT (File != NULL);

  FreeAttr (&File->Attr);

  if (  (File->FileRecord != NULL)
     && (File->FileRecord != File->File->FileSystem->RootIndex->FileRecord)
     && (File->FileRecord != File->File->FileSystem->MftStart->FileRecord))
  {
    FreePool (File->FileRecord);
    File->FileRecord = NULL;
  }
}
