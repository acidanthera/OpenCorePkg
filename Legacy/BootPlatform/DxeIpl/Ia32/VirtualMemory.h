/** @file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  VirtualMemory.h

Abstract:

Revision History:

**/
  
#ifndef _VIRTUAL_MEMORY_H_
#define _VIRTUAL_MEMORY_H_

#pragma pack(1)

//
// Page Directory Entry 4K
//
typedef union {
  struct {
    UINT32  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT32  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT32  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT32  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT32  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT32  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT32  MustBeZero:3;             // Must Be Zero
    UINT32  Available:3;              // Available for use by system software
    UINT32  PageTableBaseAddress:20;  // Page Table Base Address
  } Bits;
  UINT32    Uint32;
} IA32_PAGE_DIRECTORY_ENTRY_4K;

//
// Page Table Entry 4K
//
typedef union {
  struct {
    UINT32  Present:1;                // 0 = Not present in memory, 1 = Present in memory 
    UINT32  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT32  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT32  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT32  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT32  Accessed:1;               // 0 = Not accessed (cleared by software), 1 = Accessed (set by CPU)
    UINT32  Dirty:1;                  // 0 = Not written to (cleared by software), 1 = Written to (set by CPU)
    UINT32  PAT:1;                    // 0 = Disable PAT, 1 = Enable PAT
    UINT32  Global:1;                 // Ignored
    UINT32  Available:3;              // Available for use by system software
    UINT32  PageTableBaseAddress:20;  // Page Table Base Address
  } Bits;
  UINT32    Uint32;
} IA32_PAGE_TABLE_ENTRY_4K;

//
// Page Table Entry 4M
//
typedef union {
  struct {
    UINT32  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT32  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT32  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT32  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT32  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT32  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT32  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT32  MustBe1:1;                // Must be 1 
    UINT32  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT32  Available:3;              // Available for use by system software
    UINT32  PAT:1;                    //
    UINT32  MustBeZero:9;             // Must be zero;
    UINT32  PageTableBaseAddress:10;  // Page Table Base Address
  } Bits;
  UINT32    Uint32;
} IA32_PAGE_TABLE_ENTRY_4M;

#pragma pack()

#endif 
