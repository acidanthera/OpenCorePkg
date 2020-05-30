/** @file
  Copyright (C) 2019, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_CHUNKLIST_H
#define APPLE_CHUNKLIST_H

//
// Magic number used to identify chunklist.
//
#define APPLE_CHUNKLIST_MAGIC               0x4C4B4E43 // "CNKL"

//
// Supported chunklist versions.
//
#define APPLE_CHUNKLIST_FILE_VERSION_10     0x1
#define APPLE_CHUNKLIST_CHUNK_METHOD_10     0x1
#define APPLE_CHUNKLIST_SIG_METHOD_10       0x1
#define APPLE_CHUNKLIST_CHECKSUM_LENGTH     32
#define APPLE_CHUNKLIST_SIG_LENGTH          256

#pragma pack(push, 1)

//
// Chunklist chunk.
//
typedef struct APPLE_CHUNKLIST_CHUNK_ {
  UINT32  Length;
  UINT8   Checksum[APPLE_CHUNKLIST_CHECKSUM_LENGTH];
} APPLE_CHUNKLIST_CHUNK;

//
// Chunklist signature.
//
typedef struct APPLE_CHUNKLIST_SIG_ {
  UINT8   Signature[APPLE_CHUNKLIST_SIG_LENGTH];
} APPLE_CHUNKLIST_SIG;

//
// Chunklist header.
//
typedef struct {
  UINT32  Magic;
  UINT32  Length;
  UINT8   FileVersion;
  UINT8   ChunkMethod;
  UINT8   SigMethod;
  UINT8   Unused;

  UINT64  ChunkCount;
  UINT64  ChunkOffset;
  UINT64  SigOffset;
} APPLE_CHUNKLIST_HEADER;

#pragma pack(pop)

#endif // APPLE_CHUNKLIST_H
