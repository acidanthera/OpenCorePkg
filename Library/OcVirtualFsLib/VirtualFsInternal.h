/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef VIRTUAL_FS_INTERNAL_H
#define VIRTUAL_FS_INTERNAL_H

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

#define VIRTUAL_VOLUME_DATA_SIGNATURE  \
  SIGNATURE_32 ('V', 'F', 'S', 'v')

#define VIRTUAL_VOLUME_FROM_FILESYSTEM_PROTOCOL(This) \
  CR (                             \
    This,                          \
    VIRTUAL_FILESYSTEM_DATA,       \
    FileSystem,                    \
    VIRTUAL_VOLUME_DATA_SIGNATURE  \
    )

#define VIRTUAL_FILE_DATA_SIGNATURE  \
  SIGNATURE_32 ('V', 'F', 'S', 'f')

#define VIRTUAL_FILE_FROM_PROTOCOL(This) \
  CR (                           \
    This,                        \
    VIRTUAL_FILE_DATA,           \
    Protocol,                    \
    VIRTUAL_FILE_DATA_SIGNATURE  \
    )

#define VIRTUAL_DIR_DATA_SIGNATURE  \
  SIGNATURE_32 ('V', 'F', 'S', 'd')

#define VIRTUAL_DIR_FROM_PROTOCOL(This) \
  CR (                           \
    This,                        \
    VIRTUAL_DIR_DATA,            \
    Protocol,                    \
    VIRTUAL_DIR_DATA_SIGNATURE   \
    )

typedef struct VIRTUAL_FILESYSTEM_DATA_ VIRTUAL_FILESYSTEM_DATA;
typedef struct VIRTUAL_FILE_DATA_ VIRTUAL_FILE_DATA;
typedef struct VIRTUAL_DIR_DATA_ VIRTUAL_DIR_DATA;

struct VIRTUAL_FILESYSTEM_DATA_ {
  UINT32                           Signature;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *OriginalFileSystem;
  EFI_FILE_OPEN                    OpenCallback;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  FileSystem;
};

struct VIRTUAL_FILE_DATA_ {
  UINT32                   Signature;
  CHAR16                   *FileName;
  UINT8                    *FileBuffer;
  UINT64                   FileSize;
  UINT64                   FilePosition;
  EFI_TIME                 ModificationTime;
  EFI_FILE_OPEN            OpenCallback;
  EFI_FILE_PROTOCOL        *OriginalProtocol;
  EFI_FILE_PROTOCOL        Protocol;
};

struct VIRTUAL_DIR_DATA_ {
  UINT32                   Signature;
  CHAR16                   *FileName;
  LIST_ENTRY               Entries;
  LIST_ENTRY               *CurrentEntry;
  EFI_TIME                 ModificationTime;
  EFI_FILE_PROTOCOL        *UnderlyingProtocol;
  EFI_FILE_PROTOCOL        Protocol;
};

typedef struct {
  UINT32                   Signature;
  LIST_ENTRY               Link;
  EFI_FILE_INFO            *FileInfo;
} VIRTUAL_DIR_ENTRY;

//
// PRELINKED_KEXT signature for list identification.
//
#define VIRTUAL_DIR_ENTRY_SIGNATURE  SIGNATURE_32 ('V', 'S', 'd', 'L')

/**
  Gets the next element in list of VIRTUAL_DIR_ENTRY.

  @param[in] This  The current ListEntry.
**/
#define GET_VIRTUAL_DIR_ENTRY_FROM_LINK(This)  \
  (CR (                                        \
    (This),                                    \
    VIRTUAL_DIR_ENTRY,                         \
    Link,                                      \
    VIRTUAL_DIR_ENTRY_SIGNATURE                \
    ))

#endif // VIRTUAL_FS_INTERNAL_H
