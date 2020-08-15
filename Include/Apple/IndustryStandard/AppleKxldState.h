/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_KXLD_STATE_H
#define APPLE_KXLD_STATE_H

#include <IndustryStandard/AppleMachoImage.h>

/**
  The format of the link state object is as follows:
 
  *****************************************************
  *      Field            ***       Type              *
  *****************************************************
  * Link state header     *** KXLD_LINK_STATE_HEADER  *
  *****************************************************
  * Section order entries *** KXLD_SECTION_NAME       *
  *****************************************************
  * Vtable headers        *** KXLD_VTABLE_HEADER      *
  *****************************************************
  * VTables               *** KXLD_SYM_ENTRY_[32|64]  *
  *****************************************************
  * Exported symbols      *** KXLD_SYM_ENTRY_[32|64]  *
  *****************************************************
  * String table          *** CHAR8[]                 *
  *****************************************************
**/

/**
  Normal KXLD state state signature.
**/
#define KXLD_LINK_STATE_SIGNATURE            0xF00DD00D
#define KXLD_LINK_STATE_INVERT_SIGNATURE     0x0DD00DF0

/**
  64-bit signature was never used even for 64-bit state
  as 64-bit KXLD state header had never been defined.
**/
#define KXLD_LINK_STATE_SIGNATURE_64         0xCAFEF00D
#define KXLD_LINK_STATE_INVERT_SIGNATURE_64  0x0DF0FECA

/**
  The only existent KXLD state version.
**/
#define KXLD_LINK_STATE_VERSION 1

/**
  Link state header.
**/
typedef struct {
  UINT32            Signature;       ///< Always KXLD_LINK_STATE_SIGNATURE.
  UINT32            Version;         ///< Always LINK_STATE_VERSION.
  MACH_CPU_TYPE     CpuType;         ///< Processor type as in Mach-O.
  MACH_CPU_SUBTYPE  CpuSubtype;      ///< Processor subtype as in Mach-O.
  UINT32            NumSections;     ///< Unused for kernel objects.
  UINT32            SectionOffset;   ///< Unused for kernel objects.
  UINT32            NumVtables;      ///< Number of virtual table headers.
  UINT32            VtableOffset;    ///< Offset to virtual table headers.
  UINT32            NumSymbols;      ///< Number of normal symbols.
  UINT32            SymbolOffset;    ///< Offset to normal symbols.
} KXLD_LINK_STATE_HEADER;

#pragma pack(push, 1)

typedef struct {
  UINT32            NameOffset;
  UINT32            EntryOffset;
  UINT32            NumEntries;
} KXLD_VTABLE_HEADER;

typedef struct {
  CHAR8             SegmentName[16];
  CHAR8             SectionName[16];
} KXLD_SECTION_NAME;

typedef struct {
  UINT32            Address;
  UINT32            NameOffset;
  UINT32            Flags;
} KXLD_SYM_ENTRY_32;

typedef struct {
  UINT64            Address;
  UINT32            NameOffset;
  UINT32            Flags;
} KXLD_SYM_ENTRY_64;

STATIC_ASSERT (sizeof (KXLD_SYM_ENTRY_32) == 12, "Invalid KXLD_SYM_ENTRY_32 size");
STATIC_ASSERT (sizeof (KXLD_SYM_ENTRY_64) == 16, "Invalid KXLD_SYM_ENTRY_64 size");

#pragma pack(pop)

/**
  Symbol marked with this flag is obsolete (deprecated).
**/
#define KXLD_SYM_OBSOLETE BIT0

#endif // APPLE_KXLD_STATE_H
