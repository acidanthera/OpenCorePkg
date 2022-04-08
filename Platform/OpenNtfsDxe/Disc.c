/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#include "NTFS.h"
#include "Helper.h"

UINTN mFileRecordSize;
UINTN mIndexRecordSize;
UINTN mSectorSize;
UINTN mClusterSize;

EFI_STATUS
NtfsDir (
  IN  EFI_FS        *FileSystem,
  IN  CONST CHAR16  *Path,
  OUT EFI_NTFS_FILE *File,
  IN  FUNCTION_TYPE FunctionType
  )
{
  EFI_STATUS   Status;
  NTFS_FILE    *Dir;

  ASSERT (FileSystem != NULL);
  ASSERT (Path != NULL);
  ASSERT (File != NULL);

  Dir = NULL;

  CopyMem (&File->RootFile, FileSystem->RootIndex, sizeof (NTFS_FILE));
  CopyMem (&File->MftFile, FileSystem->MftStart, sizeof (NTFS_FILE));

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
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
NtfsOpen (
  IN EFI_NTFS_FILE *File
  )
{
  EFI_STATUS   Status;
  NTFS_FILE    *BaseMftRecord;

  ASSERT (File != NULL);

  BaseMftRecord = NULL;

  CopyMem (&File->RootFile, File->FileSystem->RootIndex, sizeof (NTFS_FILE));
  CopyMem (&File->MftFile, File->FileSystem->MftStart, sizeof (NTFS_FILE));

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
    FreeFile (BaseMftRecord);

    if (!File->RootFile.InodeRead) {
      Status = InitFile (&File->RootFile, File->RootFile.Inode);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  File->Offset = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
NtfsMount (
  IN EFI_FS   *FileSystem
  )
{
  EFI_STATUS         Status;
  BOOT_FILE_DATA     Boot;
  UINTN              Size;
  EFI_NTFS_FILE      *RootFile;

  ASSERT (FileSystem != NULL);

  Status = DiskRead (FileSystem, 0, sizeof (BOOT_FILE_DATA), &Boot);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Boot.SystemId[0] != SIGNATURE_32 ('N', 'T', 'F', 'S'))
    || (Boot.SectorsPerCluster == 0)
    || ((Boot.SectorsPerCluster & (Boot.SectorsPerCluster - 1)) != 0)
    || (Boot.BytesPerSector == 0)
    || ((Boot.BytesPerSector & (Boot.BytesPerSector - 1)) != 0)) {
    DEBUG ((DEBUG_INFO, "NTFS: (NtfsMount #1) BIOS Parameter Block is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  mSectorSize  = (UINTN) Boot.BytesPerSector;
  mClusterSize = (UINTN) Boot.SectorsPerCluster * mSectorSize;

  if (Boot.MftRecordClusters > 0) {
    Size = (UINTN) Boot.MftRecordClusters * mClusterSize;
  } else if (-Boot.MftRecordClusters >= 31) {
    DEBUG ((DEBUG_INFO, "NTFS: (NtfsMount #2) BIOS Parameter Block is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  } else {
    Size = (UINTN) LShiftU64 (1ULL, -Boot.MftRecordClusters);
  }

  mFileRecordSize = Size;
  if (mFileRecordSize < mSectorSize) {
    DEBUG ((DEBUG_INFO, "NTFS: File Record is smaller than Sector.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  if (Boot.IndexRecordClusters > 0) {
    Size = (UINTN) Boot.IndexRecordClusters * mClusterSize;
  } else if (-Boot.IndexRecordClusters >= 31) {
    DEBUG ((DEBUG_INFO, "NTFS: (NtfsMount #3) BIOS Parameter Block is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  } else {
    Size = (UINTN) LShiftU64 (1ULL, -Boot.IndexRecordClusters);
  }

  mIndexRecordSize = Size;
  if (mIndexRecordSize < mSectorSize) {
    DEBUG ((DEBUG_INFO, "NTFS: Index Record is smaller than Sector.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  FileSystem->FirstMftRecord = Boot.MftLcn * mClusterSize;
  //
  // Driver limitations
  //
  if ((mFileRecordSize > NTFS_MAX_MFT) || (mIndexRecordSize > NTFS_MAX_IDX)) {
    DEBUG ((DEBUG_INFO, "NTFS: (NtfsMount #4) BIOS Parameter Block is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  RootFile = AllocateZeroPool (sizeof (*RootFile));
  if (RootFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (RootFile, &FileSystem->EfiFile, sizeof (EFI_FILE_PROTOCOL));

  RootFile->IsDir         = TRUE;
  RootFile->Path          = L"/";
  RootFile->RefCount      = 1;
  RootFile->FileSystem    = FileSystem;
  RootFile->RootFile.File = RootFile;
  RootFile->MftFile.File  = RootFile;

  RootFile->MftFile.FileRecord = AllocateZeroPool (mFileRecordSize);
  if (RootFile->MftFile.FileRecord == NULL) {
    FreePool (RootFile);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = DiskRead (
    FileSystem,
    FileSystem->FirstMftRecord,
    mFileRecordSize,
    RootFile->MftFile.FileRecord
    );
  if (EFI_ERROR (Status)) {
    FreePool (RootFile->MftFile.FileRecord);
    FreePool (RootFile);
    return Status;
  }

  Status = Fixup (
    RootFile->MftFile.FileRecord,
    mFileRecordSize,
    SIGNATURE_32 ('F', 'I', 'L', 'E')
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
  IN UINT8       *Buffer,
  IN UINT64      Length,
  IN UINT32      Magic
  )
{
  FILE_RECORD_HEADER *Record;
  UINT8              *UpdateSequencePointer;
  UINT16             UpdateSequenceNumber;
  UINT64             USCounter;
  UINT8              *BufferEnd;

  ASSERT (Buffer != NULL);

  Record = (FILE_RECORD_HEADER *) Buffer;

  if (Length < sizeof (FILE_RECORD_HEADER)) {
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

  if (((UINT64) Record->S_Size - 1) != DivU64x64Remainder (Length, mSectorSize, NULL)) {
    DEBUG ((DEBUG_INFO, "NTFS: (Fixup #4) Record is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  UpdateSequencePointer = Buffer + Record->UpdateSequenceOffset;
  UpdateSequenceNumber = ReadUnaligned16 ((UINT16 *) UpdateSequencePointer);
  USCounter = Record->UpdateSequenceOffset;

  if (Length < (mSectorSize - sizeof (UINT16))) {
    DEBUG ((DEBUG_INFO, "NTFS: (Fixup #5) Record is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  BufferEnd = Buffer + Length;
  Buffer += mSectorSize - sizeof (UINT16);
  while ((Buffer + sizeof (UINT16)) <= BufferEnd) {
    UpdateSequencePointer += sizeof (UINT16);

    USCounter += sizeof (UINT16);
    if ((USCounter + sizeof (UINT16)) > Length) {
      DEBUG ((DEBUG_INFO, "NTFS: (Fixup #6) Record is corrupted.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    if (ReadUnaligned16 ((UINT16 *) Buffer) != UpdateSequenceNumber) {
      DEBUG ((DEBUG_INFO, "NTFS: (Fixup #7) Record is corrupted.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    Buffer[0] = UpdateSequencePointer[0];
    Buffer[1] = UpdateSequencePointer[1];
    Buffer += mSectorSize;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InitAttr (
  OUT NTFS_ATTR *Attr,
  IN  NTFS_FILE *File
  )
{
  FILE_RECORD_HEADER *Record;
  UINT64             AttrEnd;

  ASSERT (Attr != NULL);
  ASSERT (File != NULL);

  Record = (FILE_RECORD_HEADER *) File->FileRecord;

  Attr->BaseMftRecord = File;
  Attr->Flags = (File == &File->File->MftFile) ? NTFS_AF_MFT_FILE : 0;

  AttrEnd = Record->AttributeOffset + sizeof (ATTR_HEADER_RES);
  if ((AttrEnd > mFileRecordSize)
    || (AttrEnd > Record->RealSize)
    || (Record->RealSize > Record->AllocatedSize)) {
    DEBUG ((DEBUG_INFO, "NTFS: (InitAttr) File record is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  Attr->Next = File->FileRecord + Record->AttributeOffset;
  Attr->Last = NULL;
  Attr->ExtensionMftRecord = NULL;
  Attr->NonResAttrList = NULL;

  return EFI_SUCCESS;
}

UINT8 *
LocateAttr (
  IN NTFS_ATTR *Attr,
  IN NTFS_FILE *Mft,
  IN UINT32    Type
  )
{
  EFI_STATUS         Status;
  UINT8              *AttrStart;

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
  IN NTFS_ATTR *Attr,
  IN UINT32    Type
  )
{
  EFI_STATUS          Status;
  UINT8               *AttrStart;
  ATTR_HEADER_RES     *Res;
  ATTR_HEADER_NONRES  *NonRes;
  ATTR_LIST_RECORD    *LRecord;
  FILE_RECORD_HEADER  *FRecord;
  UINT64              BufferSize;

  ASSERT (Attr != NULL);

  if ((Attr->Flags & NTFS_AF_ALST) != 0) {
  retry:
    while ((Attr->Next + sizeof (ATTR_LIST_RECORD)) <= Attr->Last) {
      Attr->Current = Attr->Next;
      LRecord = (ATTR_LIST_RECORD *) Attr->Current;

      if (BufferSize < LRecord->RecordLength) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #0) $ATTRIBUTE_LIST is corrupted.\n"));
        return NULL;
      }

      Attr->Next += LRecord->RecordLength;
      BufferSize -= LRecord->RecordLength;
      if (Attr->Next <= Attr->Current) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #1) $ATTRIBUTE_LIST is corrupted.\n"));
        return NULL;
      }

      if ((LRecord->Type == Type) || (Type == 0)) {
        if (Attr->Flags & NTFS_AF_MFT_FILE) {
          // Status = DiskRead (
          //   Attr->BaseMftRecord->File->FileSystem,
          //   ReadUnaligned32((UINT32 *)(Attr->Current + 0x10)),
          //   512,
          //   Attr->ExtensionMftRecord
          //   );
          // if (EFI_ERROR (Status)) {
          //   DEBUG ((DEBUG_INFO, "NTFS: Could not read first part of extension record.\n"));
          //   return NULL;
          // }
          //
          // Status = DiskRead (
          //   Attr->BaseMftRecord->File->FileSystem,
          //   ReadUnaligned32((UINT32 *)(Attr->Current + 0x14)),
          //   512,
          //   Attr->ExtensionMftRecord + 512
          //   );
          // if (EFI_ERROR (Status)) {
          //   DEBUG ((DEBUG_INFO, "NTFS: Could not read second part of extension record.\n"));
          //   return NULL;
          // }
          //
          // Status = Fixup (Attr->ExtensionMftRecord, mFileRecordSize, SIGNATURE_32 ('F', 'I', 'L', 'E'));
          // if (EFI_ERROR (Status)) {
          //   DEBUG ((DEBUG_INFO, "NTFS: Fixup failed.\n"));
          //   return NULL;
          // }
        } else {
          Status = ReadMftRecord (
            Attr->BaseMftRecord->File,
            Attr->ExtensionMftRecord,
            (UINT32) LRecord->BaseFileReference
            );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_INFO, "NTFS: Could not read extension record.\n"));
            return NULL;
          }
        }

        FRecord = (FILE_RECORD_HEADER *) Attr->ExtensionMftRecord;
        BufferSize = mFileRecordSize;
        if ((FRecord->AttributeOffset + sizeof (ATTR_HEADER_RES)) <= BufferSize) {
          Res = (ATTR_HEADER_RES *) ((UINT8 *) FRecord + FRecord->AttributeOffset);
          BufferSize -= FRecord->AttributeOffset;
        } else {
          DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #1) Extension record is corrupted.\n"));
          return NULL;
        }

        while ((BufferSize >= sizeof (UINT32)) && (Res->Type != ATTRIBUTES_END_MARKER)) {
          if (BufferSize < sizeof (ATTR_HEADER_RES)) {
            DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #2) Extension record is corrupted.\n"));
            return NULL;
          }

          if ((Res->Type == LRecord->Type)
            && (Res->AttributeId == LRecord->AttributeId)) {
            return (UINT8 *) Res;
          }

          if ((Res->Length == 0) || (Res->Length >= BufferSize)) {
            DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #3) Extension record is corrupted.\n"));
            return NULL;
          }

          BufferSize -= Res->Length;
          Res = (ATTR_HEADER_RES *) ((UINT8 *) Res + Res->Length);
        }

        DEBUG ((DEBUG_INFO, "NTFS: Can\'t find 0x%X in attribute list\n", Attr->Current));
        return NULL;
      }
    }

    return NULL;
  }

  Attr->Current = Attr->Next;
  Res = (ATTR_HEADER_RES *) Attr->Current;
  BufferSize = mFileRecordSize - (Attr->Current - Attr->BaseMftRecord->FileRecord);

  while ((BufferSize >= sizeof (UINT32)) && (Res->Type != ATTRIBUTES_END_MARKER)) {
    if (BufferSize < sizeof (ATTR_HEADER_RES)) {
      DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #1) File record is corrupted.\n"));
      return NULL;
    }

    if ((Res->Length == 0) || (Res->Length >= BufferSize)) {
      DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #2) File record is corrupted.\n"));
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
    Res = (ATTR_HEADER_RES *) Attr->Current;
  }
  //
  // Continue search in $ATTRIBUTE_LIST
  //
  if (Attr->Last) {
    Attr->ExtensionMftRecord = AllocateZeroPool (mFileRecordSize);
    if (Attr->ExtensionMftRecord == NULL) {
      return NULL;
    }

    AttrStart = Attr->Last;
    Res = (ATTR_HEADER_RES *) Attr->Last;
    BufferSize = mFileRecordSize - (Attr->Last - Attr->BaseMftRecord->FileRecord);

    if (BufferSize < sizeof (ATTR_HEADER_RES)) {
      DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #3) File record is corrupted.\n"));
      return NULL;
    }

    if (Res->NonResFlag != 0) {
      NonRes = (ATTR_HEADER_NONRES *) Res;
      Attr->Current = (UINT8 *) NonRes;

      if (BufferSize < sizeof (ATTR_HEADER_NONRES)) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #4) File record is corrupted.\n"));
        return NULL;
      }

      if (NonRes->RealSize > MAX_FILE_SIZE) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr) File is too huge.\n"));
        return NULL;
      }

      Attr->NonResAttrList = AllocateZeroPool ((UINTN) NonRes->RealSize);
      if (Attr->NonResAttrList == NULL) {
        return NULL;
      }

      Status = ReadData (Attr, AttrStart, Attr->NonResAttrList, 0, (UINTN) NonRes->RealSize);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "NTFS: Failed to read non-resident attribute list\n"));
        return NULL;
      }

      Attr->Next = Attr->NonResAttrList;
      Attr->Last = Attr->NonResAttrList + NonRes->RealSize;
      BufferSize = NonRes->RealSize;
    } else {
      if ((Res->InfoOffset < Res->Length) && (Res->Length <= BufferSize)) {
        Attr->Next = (UINT8 *) Res + Res->InfoOffset;
        Attr->Last = (UINT8 *) Res + Res->Length;
        BufferSize -= Res->InfoOffset;
      } else {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #5) File record is corrupted.\n"));
        return NULL;
      }
    }

    Attr->Flags |= NTFS_AF_ALST;
    LRecord = (ATTR_LIST_RECORD *) Attr->Next;
    while ((Attr->Next + sizeof (ATTR_LIST_RECORD)) < Attr->Last) {
      if ((LRecord->Type == Type) || (Type == 0)) {
        break;
      }

      if (BufferSize < LRecord->RecordLength) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #2) $ATTRIBUTE_LIST is corrupted.\n"));
        return NULL;
      }

      if (LRecord->RecordLength == 0) {
        DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #3) $ATTRIBUTE_LIST is corrupted.\n"));
        return NULL;
      }

      Attr->Next += LRecord->RecordLength;
      BufferSize -= LRecord->RecordLength;
      LRecord = (ATTR_LIST_RECORD *) Attr->Next;
    }

    if ((Attr->Next + sizeof (ATTR_LIST_RECORD)) >= Attr->Last) {
      return NULL;
    }

    // if ((Attr->Flags & NTFS_AF_MFT_FILE) && (Type == AT_DATA)) {
    //   Attr->Flags |= NTFS_AF_GPOS;
    //   Attr->Current = Attr->Next;
    //   AttrStart = Attr->Current;
    //
    //   if (BufferSize >= 0x18) {
    //     *(UINT32 *) (AttrStart + 0x10) = (UINT32)(Attr->BaseMftRecord->File->FileSystem->FirstMftRecord / mSectorSize);
    //     *(UINT32 *) (AttrStart + 0x14) = (UINT32)(Attr->BaseMftRecord->File->FileSystem->FirstMftRecord / mSectorSize + 1);
    //   } else {
    //     DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #7) File record is corrupted.\n"));
    //     return NULL;
    //   }
    //
    //   AttrStart = Attr->Next + ReadUnaligned16((UINT16 *)(AttrStart + 0x04));
    //   while ((AttrStart + sizeof (UINT32) + sizeof (UINT16)) < Attr->Last) {
    //     if (*(UINT32 *)AttrStart != Type) {
    //       break;
    //     }
    //
    //     Status = ReadAttr (
    //       Attr,
    //       AttrStart + 0x10,
    //       ReadUnaligned32((UINT32 *)(AttrStart + 0x10)) * mFileRecordSize,
    //       mFileRecordSize
    //       );
    //     if (EFI_ERROR (Status)) {
    //       return NULL;
    //     }
    //
    //     if (ReadUnaligned16((UINT16 *)(AttrStart + 4)) == 0) {
    //       DEBUG ((DEBUG_INFO, "NTFS: (FindAttr #8) File record is corrupted.\n"));
    //       return NULL;
    //     }
    //
    //     AttrStart += ReadUnaligned16((UINT16 *)(AttrStart + 4));
    //   }
    //   Attr->Next = Attr->Current;
    //   Attr->Flags &= ~NTFS_AF_GPOS;
    // }

    goto retry;
  }

  return NULL;
}

EFI_STATUS
EFIAPI
InitFile (
  IN OUT NTFS_FILE  *File,
  IN     UINT64     RecordNumber
  )
{
  EFI_STATUS         Status;
  ATTR_HEADER_RES    *Attr;
  FILE_RECORD_HEADER *Record;

  ASSERT (File != NULL);

  File->InodeRead = TRUE;

  File->FileRecord = AllocateZeroPool (mFileRecordSize);
  if (File->FileRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = ReadMftRecord (File->File, File->FileRecord, RecordNumber);
  if (EFI_ERROR (Status)) {
    FreePool (File->FileRecord);
    return Status;
  }

  Record = (FILE_RECORD_HEADER *) File->FileRecord;

  if ((Record->Flags & IS_IN_USE) == 0) {
    DEBUG ((DEBUG_INFO, "NTFS: MFT Record 0x%Lx is not in use\n", RecordNumber));
    FreePool (File->FileRecord);
    return EFI_VOLUME_CORRUPTED;
  }

  if ((Record->Flags & IS_A_DIRECTORY) == 0) {
    Attr = (ATTR_HEADER_RES *) LocateAttr (&File->Attr, File, AT_DATA);
    if (Attr == NULL) {
      DEBUG ((DEBUG_INFO, "NTFS: No $DATA in MFT Record 0x%Lx\n", RecordNumber));
      FreePool (File->FileRecord);
      return EFI_VOLUME_CORRUPTED;
    }

    if (Attr->NonResFlag == 0) {
      File->DataAttributeSize = Attr->InfoLength;
    } else {
      File->DataAttributeSize = ((ATTR_HEADER_NONRES *) Attr)->RealSize;
    }

    if ((File->Attr.Flags & NTFS_AF_ALST) == 0) {
      File->Attr.Last = 0;
    }
  } else {
    Status = InitAttr (&File->Attr, File);
    if (EFI_ERROR (Status)) {
      FreePool (File->FileRecord);
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
  }

  if (Attr->NonResAttrList != NULL) {
    FreePool (Attr->NonResAttrList);
  }
}

VOID
FreeFile (
  IN NTFS_FILE  *File
  )
{
  ASSERT (File != NULL);

  FreeAttr (&File->Attr);

  if ((File->FileRecord != NULL)
    && (File->FileRecord != File->File->FileSystem->RootIndex->FileRecord)
    && (File->FileRecord != File->File->FileSystem->MftStart->FileRecord)) {
    FreePool (File->FileRecord);
  }
}
