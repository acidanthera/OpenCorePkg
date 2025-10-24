/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#include "NTFS.h"
#include "Helper.h"

UINT64  mUnitSize;

STATIC
UINT64
GetLcn (
  IN OUT RUNLIST  *Runlist,
  IN     UINT64   Vcn
  )
{
  EFI_STATUS  Status;
  UINT64      Delta;

  ASSERT (Runlist != NULL);

  if (Vcn >= Runlist->NextVcn) {
    Status = ReadRunListElement (Runlist);
    if (EFI_ERROR (Status)) {
      return (UINT64)(-1);
    }

    return Runlist->CurrentLcn;
  }

  Delta = Vcn - Runlist->CurrentVcn;

  return Runlist->IsSparse ? 0 : (Runlist->CurrentLcn + Delta);
}

STATIC
EFI_STATUS
ReadClusters (
  IN  RUNLIST  *Runlist,
  IN  UINT64   Offset,
  IN  UINTN    Length,
  OUT UINT8    *Dest
  )
{
  EFI_STATUS  Status;
  UINT64      Index;
  UINT64      ClustersTotal;
  UINT64      Cluster;
  UINT64      OffsetInsideCluster;
  UINTN       Size;
  UINTN       ClusterSize;

  ASSERT (Runlist != NULL);
  ASSERT (Dest != NULL);

  ClusterSize         = Runlist->Unit.FileSystem->ClusterSize;
  OffsetInsideCluster = Offset & (ClusterSize - 1U);
  Size                = ClusterSize;
  ClustersTotal       = DivU64x64Remainder (Length + Offset + ClusterSize - 1U, ClusterSize, NULL);

  for (Index = Runlist->TargetVcn; Index < ClustersTotal; ++Index) {
    Cluster = GetLcn (Runlist, Index);
    if (Cluster == (UINT64)(-1)) {
      return EFI_DEVICE_ERROR;
    }

    Cluster *= ClusterSize;

    if (Index == (ClustersTotal - 1U)) {
      Size = (UINTN)((Length + Offset) & (ClusterSize - 1U));

      if (Size == 0) {
        Size = ClusterSize;
      }

      if (Index != Runlist->TargetVcn) {
        OffsetInsideCluster = 0;
      }
    }

    if (Index == Runlist->TargetVcn) {
      Size -= (UINTN)OffsetInsideCluster;
    }

    if ((Index != Runlist->TargetVcn) && (Index != (ClustersTotal - 1U))) {
      Size                = ClusterSize;
      OffsetInsideCluster = 0;
    }

    if (Cluster != 0) {
      Status = DiskRead (
                 Runlist->Unit.FileSystem,
                 Cluster + OffsetInsideCluster,
                 Size,
                 Dest
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    } else {
      SetMem (Dest, Size, 0);
    }

    Dest += Size;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DiskRead (
  IN EFI_FS    *FileSystem,
  IN UINT64    Offset,
  IN UINTN     Size,
  IN OUT VOID  *Buffer
  )
{
  EFI_STATUS          Status;
  EFI_BLOCK_IO_MEDIA  *Media;

  ASSERT (FileSystem != NULL);
  ASSERT (FileSystem->DiskIo != NULL);
  ASSERT (FileSystem->BlockIo != NULL);

  Media = FileSystem->BlockIo->Media;

  Status = FileSystem->DiskIo->ReadDisk (
                                 FileSystem->DiskIo,
                                 Media->MediaId,
                                 Offset,
                                 Size,
                                 Buffer
                                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not read disk at address %Lx\n", Offset));
  }

  return Status;
}

EFI_STATUS
EFIAPI
ReadMftRecord (
  IN  EFI_NTFS_FILE  *File,
  OUT UINT8          *Buffer,
  IN  UINT64         RecordNumber
  )
{
  EFI_STATUS  Status;
  UINTN       FileRecordSize;

  ASSERT (File != NULL);
  ASSERT (Buffer != NULL);

  FileRecordSize = File->FileSystem->FileRecordSize;

  Status = ReadAttr (
             &File->MftFile.Attr,
             Buffer,
             RecordNumber * FileRecordSize,
             FileRecordSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "NTFS: Could not read MFT Record 0x%Lx\n", RecordNumber));
    FreeAttr (&File->MftFile.Attr);
    return Status;
  }

  return Fixup (
           Buffer,
           FileRecordSize,
           SIGNATURE_32 ('F', 'I', 'L', 'E'),
           File->FileSystem->SectorSize
           );
}

EFI_STATUS
EFIAPI
ReadAttr (
  IN  NTFS_ATTR  *Attr,
  OUT UINT8      *Dest,
  IN  UINT64     Offset,
  IN  UINTN      Length
  )
{
  EFI_STATUS        Status;
  UINT8             *Current;
  UINT32            Type;
  UINT8             *AttrStart;
  UINT64            Vcn;
  ATTR_LIST_RECORD  *Record;
  ATTR_HEADER_RES   *Res;
  UINTN             ClusterSize;

  ASSERT (Attr != NULL);
  ASSERT (Dest != NULL);

  Current     = Attr->Current;
  if (Current == NULL) {
    DEBUG ((DEBUG_INFO, "NTFS: Internal error! Incorrect initialization of attribute iterator\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  Attr->Next  = Attr->Current;
  ClusterSize = Attr->BaseMftRecord->File->FileSystem->ClusterSize;

  if ((Attr->Flags & NTFS_AF_ALST) != 0) {
    if (Attr->Last < (Attr->Next + sizeof (ATTR_LIST_RECORD))) {
      DEBUG ((DEBUG_INFO, "NTFS: $ATTRIBUTE_LIST does not contain even a single record.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    Record = (ATTR_LIST_RECORD *)Attr->Next;
    Type   = Record->Type;

    Vcn = DivU64x64Remainder (Offset, ClusterSize, NULL);
    if (COMPRESSION_BLOCK >= ClusterSize) {
      Vcn &= ~0xFULL;
    }

    Record = (ATTR_LIST_RECORD *)((UINT8 *)Record + Record->RecordLength);
    while (((UINT8 *)Record + sizeof (ATTR_LIST_RECORD)) <= Attr->Last) {
      if (Record->Type != Type) {
        break;
      }

      if (Record->StartingVCN > Vcn) {
        break;
      }

      if (Record->RecordLength == 0) {
        return EFI_VOLUME_CORRUPTED;
      }

      Attr->Next = (UINT8 *)Record;
      Record     = (ATTR_LIST_RECORD *)((UINT8 *)Record + Record->RecordLength);
    }
  } else {
    Res  = (ATTR_HEADER_RES *)Attr->Next;
    Type = Res->Type;
  }

  AttrStart = FindAttr (Attr, Type);
  if (AttrStart != NULL) {
    Status = ReadData (Attr, AttrStart, Dest, Offset, Length);
  } else {
    DEBUG ((DEBUG_INFO, "NTFS: Attribute not found\n"));
    Status = EFI_VOLUME_CORRUPTED;
  }

  Attr->Current = Current;

  return Status;
}

EFI_STATUS
EFIAPI
ReadData (
  IN  NTFS_ATTR  *Attr,
  IN  UINT8      *AttrStart,
  OUT UINT8      *Dest,
  IN  UINT64     Offset,
  IN  UINTN      Length
  )
{
  EFI_STATUS          Status;
  RUNLIST             *Runlist;
  ATTR_HEADER_NONRES  *NonRes;
  ATTR_HEADER_RES     *Res;
  UINT64              Sector0;
  UINT64              Sector1;
  UINT64              OffsetInsideCluster;
  UINT64              BufferSize;
  UINTN               FileRecordSize;
  UINTN               SectorSize;
  UINTN               ClusterSize;

  if (Length == 0) {
    return EFI_SUCCESS;
  }

  FileRecordSize = Attr->BaseMftRecord->File->FileSystem->FileRecordSize;
  SectorSize     = Attr->BaseMftRecord->File->FileSystem->SectorSize;
  ClusterSize    = Attr->BaseMftRecord->File->FileSystem->ClusterSize;
  BufferSize     = FileRecordSize - (Attr->Current - Attr->BaseMftRecord->FileRecord);
  //
  // Resident Attribute
  //
  if (BufferSize < sizeof (ATTR_HEADER_RES)) {
    DEBUG ((DEBUG_INFO, "NTFS: (ReadData #1) Attribute is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  Res = (ATTR_HEADER_RES *)AttrStart;

  if (Res->NonResFlag == 0) {
    if ((Offset + Length) > Res->InfoLength) {
      DEBUG ((DEBUG_INFO, "NTFS: Read out of range\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    if (BufferSize < (Res->InfoOffset + Offset + Length)) {
      DEBUG ((DEBUG_INFO, "NTFS: Read out of buffer.\n"));
      return EFI_VOLUME_CORRUPTED;
    }

    CopyMem (Dest, (UINT8 *)Res + Res->InfoOffset + Offset, Length);

    return EFI_SUCCESS;
  }

  //
  // Non-Resident Attribute
  //
  if (BufferSize < sizeof (ATTR_HEADER_NONRES)) {
    DEBUG ((DEBUG_INFO, "NTFS: (ReadData #2) Attribute is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  NonRes = (ATTR_HEADER_NONRES *)AttrStart;

  Runlist = (RUNLIST *)AllocateZeroPool (sizeof (RUNLIST));
  if (Runlist == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Runlist->Attr            = Attr;
  Runlist->Unit.FileSystem = Attr->BaseMftRecord->File->FileSystem;

  if (  (NonRes->DataRunsOffset > BufferSize)
     || (NonRes->DataRunsOffset > NonRes->RealSize)
     || (NonRes->RealSize > NonRes->AllocatedSize))
  {
    DEBUG ((DEBUG_INFO, "NTFS: Non-Resident Attribute is corrupted.\n"));
    FreePool (Runlist);
    return EFI_VOLUME_CORRUPTED;
  }

  Runlist->NextDataRun = (UINT8 *)NonRes + NonRes->DataRunsOffset;
  Runlist->NextVcn     = NonRes->StartingVCN;
  Runlist->CurrentLcn  = 0;

  Runlist->TargetVcn = NonRes->StartingVCN + DivU64x64Remainder (Offset, ClusterSize, NULL);
  if (Runlist->TargetVcn < NonRes->StartingVCN) {
    DEBUG ((DEBUG_INFO, "NTFS: Overflow: StartingVCN is too large.\n"));
    FreePool (Runlist);
    return EFI_VOLUME_CORRUPTED;
  }

  if ((Offset + Length) > NonRes->RealSize) {
    DEBUG ((DEBUG_INFO, "NTFS: Read out of range\n"));
    FreePool (Runlist);
    return EFI_VOLUME_CORRUPTED;
  }

  if (  ((NonRes->Flags & FLAG_COMPRESSED) != 0)
     && ((Attr->Flags & NTFS_AF_GPOS) == 0)
     && (NonRes->Type == AT_DATA))
  {
    if (NonRes->CompressionUnitSize != 4U) {
      DEBUG ((DEBUG_INFO, "NTFS: Invalid copmression unit size.\n"));
      FreePool (Runlist);
      return EFI_VOLUME_CORRUPTED;
    }

    mUnitSize = LShiftU64 (1ULL, NonRes->CompressionUnitSize);

    Status = Decompress (Runlist, Offset, Length, Dest);

    FreePool (Runlist);
    return Status;
  }

  while (Runlist->NextVcn <= Runlist->TargetVcn) {
    Status = ReadRunListElement (Runlist);
    if (EFI_ERROR (Status)) {
      FreePool (Runlist);
      return EFI_VOLUME_CORRUPTED;
    }
  }

  if (Attr->Flags & NTFS_AF_GPOS) {
    OffsetInsideCluster = Offset & (ClusterSize - 1U);

    Sector0 = DivU64x64Remainder (
                (Runlist->TargetVcn - Runlist->CurrentVcn + Runlist->CurrentLcn) * ClusterSize + OffsetInsideCluster,
                SectorSize,
                NULL
                );

    Sector1 = Sector0 + 1U;
    if (Sector1 == DivU64x64Remainder (
                     (Runlist->NextVcn - Runlist->CurrentVcn + Runlist->CurrentLcn) * ClusterSize,
                     SectorSize,
                     NULL
                     ))
    {
      Status = ReadRunListElement (Runlist);
      if (EFI_ERROR (Status)) {
        FreePool (Runlist);
        return EFI_VOLUME_CORRUPTED;
      }

      Sector1 = DivU64x64Remainder (
                  Runlist->CurrentLcn * ClusterSize,
                  SectorSize,
                  NULL
                  );
    }

    WriteUnaligned32 ((UINT32 *)Dest, (UINT32)Sector0);
    WriteUnaligned32 ((UINT32 *)(Dest + 4U), (UINT32)Sector1);

    FreePool (Runlist);
    return EFI_SUCCESS;
  }

  Status = ReadClusters (
             Runlist,
             Offset,
             Length,
             Dest
             );

  FreePool (Runlist);
  return Status;
}

STATIC
UINT64
ReadField (
  IN CONST UINT8  *Run,
  IN UINT8        FieldSize,
  IN BOOLEAN      Signed
  )
{
  UINT64  Value;

  ASSERT (Run != NULL);

  Value = 0;
  //
  // Offset to the starting LCN of the previous element is a signed value.
  // So we must check the most significant bit.
  //
  if (Signed && (FieldSize != 0) && ((Run[FieldSize - 1U] & 0x80U) != 0)) {
    Value = (UINT64)(-1);
  }

  CopyMem (&Value, Run, FieldSize);

  return Value;
}

/**
   Table 4.10. Layout of a Data Run
   ____________________________________________________________________
   Offset | Size | Description
   --------------------------------------------------------------------
   0      | 0.5   | F=Size of the Offset field
   0.5    | 0.5   | L=Size of the Length field
   1      | L     | Length of the run
   1+L    | F     | Offset to the starting LCN of the previous element (signed)
   --------------------------------------------------------------------
**/
EFI_STATUS
EFIAPI
ReadRunListElement (
  IN OUT RUNLIST  *Runlist
  )
{
  UINT8               LengthSize;
  UINT8               OffsetSize;
  UINT64              OffsetLcn;
  UINT8               *Run;
  ATTR_HEADER_NONRES  *Attr;
  UINT64              BufferSize;
  UINTN               FileRecordSize;

  ASSERT (Runlist != NULL);

  Run            = Runlist->NextDataRun;
  FileRecordSize = Runlist->Attr->BaseMftRecord->File->FileSystem->FileRecordSize;
  BufferSize     = FileRecordSize - (Run - Runlist->Attr->BaseMftRecord->FileRecord);

retry:
  if (BufferSize == 0) {
    DEBUG ((DEBUG_INFO, "NTFS: (ReadRunListElement #1) Runlist is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  LengthSize = *Run & 0xFU;
  OffsetSize = (*Run >> 4U) & 0xFU;
  ++Run;
  --BufferSize;

  if (  (LengthSize > 8U)
     || (OffsetSize > 8U)
     || ((LengthSize == 0) && (OffsetSize != 0)))
  {
    DEBUG ((DEBUG_INFO, "NTFS: (ReadRunListElement #2) Runlist is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // End of Runlist: LengthSize == 0, OffsetSize == 0
  //
  if ((LengthSize == 0) && (OffsetSize == 0)) {
    if (  (Runlist->Attr != NULL)
       && ((Runlist->Attr->Flags & NTFS_AF_ALST) != 0))
    {
      Attr = (ATTR_HEADER_NONRES *)Runlist->Attr->Current;
      Attr = (ATTR_HEADER_NONRES *)FindAttr (Runlist->Attr, Attr->Type);
      if (Attr != NULL) {
        if (Attr->NonResFlag == 0) {
          DEBUG ((DEBUG_INFO, "NTFS: $DATA should be non-resident\n"));
          return EFI_VOLUME_CORRUPTED;
        }

        Run                 = (UINT8 *)Attr + Attr->DataRunsOffset;
        Runlist->CurrentLcn = 0;
        BufferSize          = FileRecordSize - Attr->DataRunsOffset;
        goto retry;
      }
    }

    DEBUG ((DEBUG_INFO, "NTFS: Run list overflown\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  Runlist->CurrentVcn = Runlist->NextVcn;

  if (BufferSize < LengthSize) {
    DEBUG ((DEBUG_INFO, "NTFS: (ReadRunListElement #3) Runlist is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  Runlist->NextVcn += ReadField (Run, LengthSize, FALSE);
  if (Runlist->NextVcn <= Runlist->CurrentVcn) {
    DEBUG ((DEBUG_INFO, "NTFS: (ReadRunListElement #3.1) Runlist is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  Run        += LengthSize;
  BufferSize -= LengthSize;

  if (BufferSize < OffsetSize) {
    DEBUG ((DEBUG_INFO, "NTFS: (ReadRunListElement #4) Runlist is corrupted.\n"));
    return EFI_VOLUME_CORRUPTED;
  }

  OffsetLcn            = ReadField (Run, OffsetSize, TRUE);
  Runlist->CurrentLcn += OffsetLcn;

  Run                 += OffsetSize;
  BufferSize          -= OffsetSize;
  Runlist->NextDataRun = Run;

  Runlist->IsSparse = (OffsetLcn == 0);

  return EFI_SUCCESS;
}

CHAR16 *
ReadSymlink (
  IN NTFS_FILE  *File
  )
{
  EFI_STATUS  Status;
  SYMLINK     Symlink;
  CHAR16      *Substitute;
  CHAR16      *Letter;
  UINT64      Offset;

  ASSERT (File != NULL);

  File->FileRecord = AllocateZeroPool (File->File->FileSystem->FileRecordSize);
  if (File->FileRecord == NULL) {
    return NULL;
  }

  Status = ReadMftRecord (File->File, File->FileRecord, File->Inode);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  if (LocateAttr (&File->Attr, File, AT_SYMLINK) == NULL) {
    DEBUG ((DEBUG_INFO, "NTFS: no $SYMLINK in MFT 0x%Lx\n", File->Inode));
    return NULL;
  }

  Status = ReadAttr (&File->Attr, (UINT8 *)&Symlink, 0, sizeof (Symlink));
  if (EFI_ERROR (Status)) {
    FreeAttr (&File->Attr);
    return NULL;
  }

  switch (Symlink.Type) {
    case (IS_ALIAS | IS_MICROSOFT | S_SYMLINK):
      Offset = sizeof (Symlink) + 4U + (UINT64)Symlink.SubstituteOffset;
      break;
    case (IS_ALIAS | IS_MICROSOFT | S_FILENAME):
      Offset = sizeof (Symlink) + (UINT64)Symlink.SubstituteOffset;
      break;
    default:
      DEBUG ((DEBUG_INFO, "NTFS: Symlink type invalid (%x)\n", Symlink.Type));
      return NULL;
  }

  Substitute = AllocateZeroPool (Symlink.SubstituteLength + sizeof (CHAR16));
  if (Substitute == NULL) {
    return NULL;
  }

  Status = ReadAttr (&File->Attr, (UINT8 *)Substitute, Offset, Symlink.SubstituteLength);
  if (EFI_ERROR (Status)) {
    FreeAttr (&File->Attr);
    FreePool (Substitute);
    return NULL;
  }

  for (Letter = Substitute; *Letter != L'\0'; ++Letter) {
    if (*Letter == L'\\') {
      *Letter = L'/';
    }
  }

  return Substitute;
}
