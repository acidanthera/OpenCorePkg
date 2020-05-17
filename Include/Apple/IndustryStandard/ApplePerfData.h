/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_PERF_DATA_H
#define APPLE_PERF_DATA_H

#pragma pack(push, 1)

/**
  Performance data header area (total reserved).
**/
#define APPLE_PERF_DATA_HEADER_SIZE  64

/**
  Performance data header signature.
**/
#define APPLE_PERF_DATA_SIGNATURE  0x6F6F746C65666962ULL ///< ootlefib (EfiBoot L)

/**
  Performance data header.
**/
typedef struct APPLE_PERF_DATA_ {
  UINT64  Signature;
  UINT32  NumberOfEntries;
  UINT32  NextPerfData;
  UINT64  TimerFrequencyMs;
  UINT64  TimerStartMs;
} APPLE_PERF_DATA;

/**
  Performance data entry.
**/
typedef struct APPLE_PERF_ENTRY_ {
  UINT32  StreamId;      ///< Identifier, normally 1.
  UINT32  EntryDataSize; ///< EntryData size aligned up to 8.
  UINT64  TimestampMs;
  UINT64  TimestampTsc;
  CHAR8   EntryData[];   ///< Null terminated
} APPLE_PERF_ENTRY;

#define APPLE_PERF_FIRST_ENTRY(Data) ((APPLE_PERF_ENTRY *) ((UINTN) (Data) + APPLE_PERF_DATA_HEADER_SIZE))
#define APPLE_PERF_NEXT_ENTRY(Entry) ((APPLE_PERF_ENTRY *) ((UINTN) (Entry) + sizeof (APPLE_PERF_ENTRY) + (Entry)->EntryDataSize))

#pragma pack(pop)

#endif // APPLE_PERF_DATA_H
