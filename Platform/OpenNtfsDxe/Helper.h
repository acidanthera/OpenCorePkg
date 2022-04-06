/** @file
  Copyright (c) 2022, Mikhail Krichanov. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause

  Functional and structural descriptions follow NTFS Documentation
  by Richard Russon and Yuval Fledel
**/

#ifndef HELPER_H
#define HELPER_H

#include "Driver.h"

#define SECS_PER_HOUR           (60 * 60)
#define SECS_PER_DAY            (SECS_PER_HOUR * 24)
#define FSHELP_TYPE_MASK        0xff
#define FSHELP_CASE_INSENSITIVE 0x100
#define DIV(a, b)               ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y)    (DIV (y, 4) - DIV (y, 100) + DIV (y, 400))
//
// Leap year: is every 4 years, except every 100th isn't, and every 400th is.
//
#define __isleap(year) ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
//
// Days before each month (0-12)
//
static const unsigned short int __mon_yday[2][13] = {
  //
  // Normal years
  //
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
  //
  // Leap years
  //
  { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

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
  IN CONST CHAR8 *Magic
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

VOID
NtfsTimeToEfiTime (
  IN CONST INT32  t,
  OUT EFI_TIME    *tp
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

#endif // HELPER_H
