/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#ifndef HELPER_H
#define HELPER_H

#include "Driver.h"

#define FSHELP_TYPE_MASK        0xff
#define FSHELP_CASE_INSENSITIVE 0x100
//
// Leap year: is every 4 years, except every 100th isn't, and every 400th is.
//
#define LEAP_YEAR ((Year % 400U == 0) || ((Year % 4U == 0) && (Year % 100U != 0)))
#define YEAR_IN_100NS   (365ULL * DAY_IN_100NS)
#define DAY_IN_100NS    (24ULL  * HOUR_IN_100NS)
#define HOUR_IN_100NS   (60ULL  * MINUTE_IN_100NS)
#define MINUTE_IN_100NS (60U    * SECOND_IN_100NS)
#define SECOND_IN_100NS 10000000U
#define UNIT_IN_NS      100U
#define GREGORIAN_START 1601U

typedef enum {
  FSHELP_UNKNOWN,
  FSHELP_REG,
  FSHELP_DIR,
  FSHELP_SYMLINK
} FSHELP_FILETYPE;

typedef struct _STACK_ELEMENT {
  struct _STACK_ELEMENT *Parent;
  NTFS_FILE             *Node;
  FSHELP_FILETYPE       Type;
} STACK_ELEMENT;

typedef struct {
  CONST CHAR16  *Path;
  NTFS_FILE     *RootNode;
  UINT32        SymlinkDepth;
  STACK_ELEMENT *CurrentNode;
} FSHELP_CTX;

typedef struct {
  CONST CHAR16     *Name;
  NTFS_FILE        **FoundNode;
  FSHELP_FILETYPE  *FoundType;
} FSHELP_ITER_CTX;

typedef enum {
  INFO_HOOK,
  DIR_HOOK,
  FILE_ITER,
} FUNCTION_TYPE;

VOID
FreeAttr (
  IN NTFS_ATTR  *Attr
  );

VOID
FreeFile (
  IN NTFS_FILE  *File
  );

EFI_STATUS
EFIAPI
DiskRead (
  IN EFI_FS   *FileSystem,
  IN UINT64   Offset,
  IN UINTN    Size,
  IN OUT VOID *Buffer
  );

EFI_STATUS
EFIAPI
ReadMftRecord (
  IN  EFI_NTFS_FILE *File,
  OUT UINT8         *Buffer,
  IN  UINT64        RecordNumber
  );

EFI_STATUS
EFIAPI
ReadAttr (
  IN  NTFS_ATTR *NtfsAttr,
  OUT UINT8     *Dest,
  IN  UINT64    Offset,
  IN  UINTN     Length
  );

EFI_STATUS
EFIAPI
ReadData (
  IN  NTFS_ATTR *NtfsAttr,
  IN  UINT8     *pa,
  OUT UINT8     *Dest,
  IN  UINT64    Offset,
  IN  UINTN     Length
  );

EFI_STATUS
EFIAPI
ReadRunListElement (
  IN OUT RUNLIST  *Runlist
  );

EFI_STATUS
NtfsDir (
  IN  EFI_FS        *FileSystem,
  IN  CONST CHAR16  *Path,
  OUT EFI_NTFS_FILE *File,
  IN  FUNCTION_TYPE FunctionType
  );

EFI_STATUS
NtfsOpen (
  IN EFI_NTFS_FILE *File
  );

EFI_STATUS
NtfsMount (
  IN EFI_FS   *FileSystem
  );

EFI_STATUS
EFIAPI
Fixup (
  IN UINT8       *Buffer,
  IN UINT64      Length,
  IN UINT32      Magic
  );

EFI_STATUS
InitAttr (
  OUT NTFS_ATTR *Attr,
  IN  NTFS_FILE *File
  );

UINT8 *
LocateAttr (
  IN NTFS_ATTR *Attr,
  IN NTFS_FILE *Mft,
  IN UINT32    Type
  );

UINT8 *
FindAttr (
  IN NTFS_ATTR *Attr,
  IN UINT32    Type
  );

EFI_STATUS
EFIAPI
InitFile (
  IN OUT NTFS_FILE  *File,
  IN     UINT64     RecordNumber
  );

EFI_STATUS
FsHelpFindFile (
  IN  CONST CHAR16     *Path,
  IN  NTFS_FILE        *RootNode,
  OUT NTFS_FILE        **FoundNode,
  IN  FSHELP_FILETYPE  Type
  );

EFI_STATUS
IterateDir (
  IN NTFS_FILE     *dir,
  IN VOID          *FileOrCtx,
  IN FUNCTION_TYPE FunctionType
  );

EFI_STATUS
RelativeToAbsolute (
  OUT CHAR16 *Dest,
  IN  CHAR16 *Source
  );

CHAR16 *
ReadSymlink (
  IN NTFS_FILE  *Node
  );

EFI_STATUS
Decompress (
  IN  RUNLIST *Runlist,
  IN  UINT64  Offset,
  IN  UINTN   Length,
  OUT UINT8   *Dest
  );

VOID
NtfsToEfiTime (
  EFI_TIME *EfiTime,
  UINT64   NtfsTime
  );

#endif // HELPER_H
