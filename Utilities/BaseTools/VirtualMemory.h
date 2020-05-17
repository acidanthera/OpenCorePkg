/** @file
x64 Long Mode Virtual Memory Management Definitions

  References:
    1) IA-32 Intel(R) Atchitecture Software Developer's Manual Volume 1:Basic Architecture, Intel
    2) IA-32 Intel(R) Atchitecture Software Developer's Manual Volume 2:Instruction Set Reference, Intel
    3) IA-32 Intel(R) Atchitecture Software Developer's Manual Volume 3:System Programmer's Guide, Intel
    4) AMD64 Architecture Programmer's Manual Volume 2: System Programming

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VIRTUAL_MEMORY_H_
#define _VIRTUAL_MEMORY_H_

#include <stdint.h>

#pragma pack(1)

//
// Page-Map Level-4 Offset (PML4) and
// Page-Directory-Pointer Offset (PDPE) entries 4K & 2MB
//

typedef union {
  struct {
    uint64_t  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    uint64_t  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    uint64_t  UserSupervisor:1;         // 0 = Supervisor, 1=User
    uint64_t  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    uint64_t  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    uint64_t  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    uint64_t  Reserved:1;               // Reserved
    uint64_t  MustBeZero:2;             // Must Be Zero
    uint64_t  Available:3;              // Available for use by system software
    uint64_t  PageTableBaseAddress:40;  // Page Table Base Address
    uint64_t  AvabilableHigh:11;        // Available for use by system software
    uint64_t  Nx:1;                     // No Execute bit
  } Bits;
  uint64_t    Uint64;
} X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K;

//
// Page-Directory Offset 4K
//
typedef union {
  struct {
    uint64_t  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    uint64_t  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    uint64_t  UserSupervisor:1;         // 0 = Supervisor, 1=User
    uint64_t  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    uint64_t  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    uint64_t  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    uint64_t  Reserved:1;               // Reserved
    uint64_t  MustBeZero:1;             // Must Be Zero
    uint64_t  Reserved2:1;              // Reserved
    uint64_t  Available:3;              // Available for use by system software
    uint64_t  PageTableBaseAddress:40;  // Page Table Base Address
    uint64_t  AvabilableHigh:11;        // Available for use by system software
    uint64_t  Nx:1;                     // No Execute bit
  } Bits;
  uint64_t    Uint64;
} X64_PAGE_DIRECTORY_ENTRY_4K;

//
// Page Table Entry 4K
//
typedef union {
  struct {
    uint64_t  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    uint64_t  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    uint64_t  UserSupervisor:1;         // 0 = Supervisor, 1=User
    uint64_t  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    uint64_t  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    uint64_t  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    uint64_t  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    uint64_t  PAT:1;                    // 0 = Ignore Page Attribute Table
    uint64_t  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    uint64_t  Available:3;              // Available for use by system software
    uint64_t  PageTableBaseAddress:40;  // Page Table Base Address
    uint64_t  AvabilableHigh:11;        // Available for use by system software
    uint64_t  Nx:1;                     // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  uint64_t    Uint64;
} X64_PAGE_TABLE_ENTRY_4K;


//
// Page Table Entry 2MB
//
typedef union {
  struct {
    uint64_t  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    uint64_t  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    uint64_t  UserSupervisor:1;         // 0 = Supervisor, 1=User
    uint64_t  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    uint64_t  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    uint64_t  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    uint64_t  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    uint64_t  MustBe1:1;                // Must be 1
    uint64_t  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    uint64_t  Available:3;              // Available for use by system software
    uint64_t  PAT:1;                    //
    uint64_t  MustBeZero:8;             // Must be zero;
    uint64_t  PageTableBaseAddress:31;  // Page Table Base Address
    uint64_t  AvabilableHigh:11;        // Available for use by system software
    uint64_t  Nx:1;                     // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  uint64_t    Uint64;
} X64_PAGE_TABLE_ENTRY_2M;

#pragma pack()

#endif
