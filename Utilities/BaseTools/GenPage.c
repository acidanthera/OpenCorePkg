/** @file
  Pre-Create a 4G page table (2M pages).
  It's used in DUET x64 build needed to enter LongMode.

  Create 4G page table (2M pages)

                              Linear Address
    63    48 47   39 38           30 29       21 20                          0
   +--------+-------+---------------+-----------+-----------------------------+
               PML4   Directory-Ptr   Directory                 Offset

   Paging-Structures :=
                        PML4
                        (
                          Directory-Ptr Directory {512}
                        ) {4}

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "VirtualMemory.h"

#define EFI_PAGE_BASE_OFFSET_IN_LDR 0x70000
#define EFI_PAGE_BASE_ADDRESS       (EFI_PAGE_BASE_OFFSET_IN_LDR + 0x20000)

uint32_t gPageTableBaseAddress  = EFI_PAGE_BASE_ADDRESS;
uint32_t gPageTableOffsetInFile = EFI_PAGE_BASE_OFFSET_IN_LDR;

#define EFI_MAX_ENTRY_NUM     512

#define EFI_PML4_ENTRY_NUM    1
#define EFI_PDPTE_ENTRY_NUM   4
#define EFI_PDE_ENTRY_NUM     EFI_MAX_ENTRY_NUM

#define EFI_PML4_PAGE_NUM     1
#define EFI_PDPTE_PAGE_NUM    EFI_PML4_ENTRY_NUM
#define EFI_PDE_PAGE_NUM      (EFI_PML4_ENTRY_NUM * EFI_PDPTE_ENTRY_NUM)

#define EFI_PAGE_NUMBER       (EFI_PML4_PAGE_NUM + EFI_PDPTE_PAGE_NUM + EFI_PDE_PAGE_NUM)

#define EFI_SIZE_OF_PAGE      0x1000
#define EFI_PAGE_SIZE_2M      0x200000

#define CONVERT_BIN_PAGE_ADDRESS(a)  ((uint8_t *) a - PageTable + gPageTableBaseAddress)

//
// Utility Name
//
#define UTILITY_NAME  "GenPage"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 2

void
Error (
  char      *FileName,
  uint32_t  LineNumber,
  uint32_t  ErrorCode,
  char      *OffendingText,
  char      *MsgFmt,
  ...
  )
{
  va_list List;
  va_start (List, MsgFmt);
  vprintf (MsgFmt, List);
  va_end (List);
  puts("");
}

void
Version (
  void
  )
/*++

Routine Description:

  Displays the standard utility information to SDTOUT

Arguments:

  None

Returns:

  None

--*/
{
  printf ("%s Version %d.%d\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
}

void
Usage (
  void
  )
{
  printf ("Usage: GenPage.exe [options] EfiLoaderImageName \n\n\
Copyright (c) 2008 - 2018, Intel Corporation.  All rights reserved.\n\n\
  Utility to generate the EfiLoader image containing a page table.\n\n\
optional arguments:\n\
  -h, --help            Show this help message and exit\n\
  --version             Show program's version number and exit\n\
  -d [DEBUG], --debug [DEBUG]\n\
                        Output DEBUG statements, where DEBUG_LEVEL is 0 (min)\n\
                        - 9 (max)\n\
  -v, --verbose         Print informational statements\n\
  -q, --quiet           Returns the exit code, error messages will be\n\
                        displayed\n\
  -s, --silent          Returns only the exit code; informational and error\n\
                        messages are not displayed\n\
  -o OUTPUT_FILENAME, --output OUTPUT_FILENAME\n\
                        Output file contain both the non-page table part and\n\
                        the page table\n\
  -b BASE_ADDRESS, --baseaddr BASE_ADDRESS\n\
                        The page table location\n\
  -f OFFSET, --offset OFFSET\n\
                        The position that the page table will appear in the\n\
                        output file\n\
  --sfo                 Reserved for future use\n");

}

void *
CreateIdentityMappingPageTables (
  void
  )
/*++

Routine Description:
  To create 4G PAE 2M pagetable

Return:
  void * - buffer containing created pagetable

--*/
{
  uint64_t                                      PageAddress;
  uint8_t                                       *PageTable;
  uint8_t                                       *PageTablePtr;
  int                                           PML4Index;
  int                                           PDPTEIndex;
  int                                           PDEIndex;
  X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageMapLevel4Entry;
  X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageDirectoryPointerEntry;
  X64_PAGE_TABLE_ENTRY_2M                       *PageDirectoryEntry2MB;

  PageTable = (void *)malloc (EFI_PAGE_NUMBER * EFI_SIZE_OF_PAGE);
  if (PageTable == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    return NULL;
  }
  memset (PageTable, 0, (EFI_PAGE_NUMBER * EFI_SIZE_OF_PAGE));
  PageTablePtr = PageTable;

  PageAddress = 0;

  //
  //  Page Table structure 3 level 2MB.
  //
  //                   Page-Map-Level-4-Table        : bits 47-39
  //                   Page-Directory-Pointer-Table  : bits 38-30
  //
  //  Page Table 2MB : Page-Directory(2M)            : bits 29-21
  //
  //

  PageMapLevel4Entry = (X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *)PageTablePtr;

  for (PML4Index = 0; PML4Index < EFI_PML4_ENTRY_NUM; PML4Index++, PageMapLevel4Entry++) {
    //
    // Each Page-Map-Level-4-Table Entry points to the base address of a Page-Directory-Pointer-Table Entry
    //
    PageTablePtr += EFI_SIZE_OF_PAGE;
    PageDirectoryPointerEntry = (X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *)PageTablePtr;

    //
    // Make a Page-Map-Level-4-Table Entry
    //
    PageMapLevel4Entry->Uint64 = (uint64_t)(uint32_t)(CONVERT_BIN_PAGE_ADDRESS (PageDirectoryPointerEntry));
    PageMapLevel4Entry->Bits.ReadWrite = 1;
    PageMapLevel4Entry->Bits.Present = 1;

    for (PDPTEIndex = 0; PDPTEIndex < EFI_PDPTE_ENTRY_NUM; PDPTEIndex++, PageDirectoryPointerEntry++) {
      //
      // Each Page-Directory-Pointer-Table Entry points to the base address of a Page-Directory Entry
      //
      PageTablePtr += EFI_SIZE_OF_PAGE;
      PageDirectoryEntry2MB = (X64_PAGE_TABLE_ENTRY_2M *)PageTablePtr;

      //
      // Make a Page-Directory-Pointer-Table Entry
      //
      PageDirectoryPointerEntry->Uint64 = (uint64_t)(uint32_t)(CONVERT_BIN_PAGE_ADDRESS (PageDirectoryEntry2MB));
      PageDirectoryPointerEntry->Bits.ReadWrite = 1;
      PageDirectoryPointerEntry->Bits.Present = 1;

      for (PDEIndex = 0; PDEIndex < EFI_PDE_ENTRY_NUM; PDEIndex++, PageDirectoryEntry2MB++) {
        //
        // Make a Page-Directory Entry
        //
        PageDirectoryEntry2MB->Uint64 = (uint64_t)PageAddress;
        PageDirectoryEntry2MB->Bits.ReadWrite = 1;
        PageDirectoryEntry2MB->Bits.Present = 1;
        PageDirectoryEntry2MB->Bits.MustBe1 = 1;

        PageAddress += EFI_PAGE_SIZE_2M;
      }
    }
  }

  return PageTable;
}

int32_t
GenBinPage (
  void *BaseMemory,
  char *NoPageFileName,
  char *PageFileName
  )
/*++

Routine Description:
  Write the buffer containing page table to file at a specified offset.
  Here the offset is defined as EFI_PAGE_BASE_OFFSET_IN_LDR.

Arguments:
  BaseMemory     - buffer containing page table
  NoPageFileName - file to write page table
  PageFileName   - file save to after writing

return:
  0  : successful
  -1 : failed

--*/
{
  FILE          *PageFile;
  FILE          *NoPageFile;
  uint8_t       Data;
  unsigned long FileSize;

  //
  // Open files
  //
  PageFile = fopen (PageFileName, "w+b");
  if (PageFile == NULL) {
    Error (NoPageFileName, 0, 0x4002, "Invalid parameter option", "Output File %s open failure", PageFileName);
    return -1;
  }

  NoPageFile = fopen (NoPageFileName, "r+b");
  if (NoPageFile == NULL) {
    Error (NoPageFileName, 0, 0x4002, "Invalid parameter option", "Input File %s open failure", NoPageFileName);
    fclose (PageFile);
    return -1;
  }

  //
  // Check size - should not be great than EFI_PAGE_BASE_OFFSET_IN_LDR
  //
  fseek (NoPageFile, 0, SEEK_END);
  FileSize = ftell (NoPageFile);
  fseek (NoPageFile, 0, SEEK_SET);
  if (FileSize > gPageTableOffsetInFile) {
    Error (NoPageFileName, 0, 0x4002, "Invalid parameter option", "Input file size (0x%lx) exceeds the Page Table Offset (0x%x)", FileSize, (unsigned) gPageTableOffsetInFile);
    fclose (PageFile);
    fclose (NoPageFile);
    return -1;
  }

  //
  // Write data
  //
  while (fread (&Data, sizeof(uint8_t), 1, NoPageFile)) {
    fwrite (&Data, sizeof(uint8_t), 1, PageFile);
  }

  //
  // Write PageTable
  //
  fseek (PageFile, gPageTableOffsetInFile, SEEK_SET);
  fwrite (BaseMemory, (EFI_PAGE_NUMBER * EFI_SIZE_OF_PAGE), 1, PageFile);

  //
  // Close files
  //
  fclose (PageFile);
  fclose (NoPageFile);

  return 0;
}

int
main (
  int argc,
  char **argv
  )
{
  void        *BaseMemory;
  int32_t     result;
  char        *OutputFile = NULL;
  char        *InputFile = NULL;
  uint64_t    TempValue;

  if (argc == 1) {
    Usage();
    return EXIT_FAILURE;
  }

  argc --;
  argv ++;

  if ((strcmp (argv[0], "-h") == 0) || (strcmp (argv[0], "--help") == 0)) {
    Usage();
    return 0;
  }

  if (strcmp (argv[0], "--version") == 0) {
    Version();
    return 0;
  }

  while (argc > 0) {
    if ((strcmp (argv[0], "-o") == 0) || (strcmp (argv[0], "--output") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Output file is missing for -o option");
        return EXIT_FAILURE;
      }
      OutputFile = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((strcmp (argv[0], "-b") == 0) || (strcmp (argv[0], "--baseaddr") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Base address is missing for -b option");
        return EXIT_FAILURE;
      }
      TempValue = strtoull (argv[1], NULL, 0);
      gPageTableBaseAddress = (uint32_t) TempValue;
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((strcmp (argv[0], "-f") == 0) || (strcmp (argv[0], "--offset") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Offset is missing for -f option");
        return EXIT_FAILURE;
      }
      TempValue = strtoull (argv[1], NULL, 0);
      gPageTableOffsetInFile = (uint32_t) TempValue;
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((strcmp (argv[0], "-q") == 0) || (strcmp (argv[0], "--quiet") == 0)) {
      argc --;
      argv ++;
      continue;
    }

    if ((strcmp (argv[0], "-v") ==0) || (strcmp (argv[0], "--verbose") == 0)) {
      argc --;
      argv ++;
      continue;
    }

    if ((strcmp (argv[0], "-d") == 0) || (strcmp (argv[0], "--debug") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level is not specified.");
        return EXIT_FAILURE;
      }
      TempValue = strtoull (argv[1], NULL, 0);
      if (TempValue > 9) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0-9, currnt input level is %d", (int) TempValue);
        return EXIT_FAILURE;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if (argv[0][0] == '-') {
      Error (NULL, 0, 1000, "Unknown option", argv[0]);
      return EXIT_FAILURE;
    }

    //
    // Don't recognize the parameter.
    //
    InputFile = argv[0];
    argc--;
    argv++;
  }

  if (InputFile == NULL) {
    Error (NULL, 0, 1003, "Invalid option value", "Input file is not specified");
    return EXIT_FAILURE;
  }

  //
  // Create X64 page table
  //
  BaseMemory = CreateIdentityMappingPageTables ();
  if (BaseMemory == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    return EXIT_FAILURE;
  }

  //
  // Add page table to binary file
  //
  result = GenBinPage (BaseMemory, InputFile, OutputFile);
  if (result < 0) {
    free (BaseMemory);
    return EXIT_FAILURE;
  }

  free (BaseMemory);
  return 0;
}

