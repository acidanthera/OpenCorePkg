/** @file
  Copyright (c) 2020-2021, Ubsefor & koralexa. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Uefi/UefiBaseType.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/Acpi.h>
#include <Library/OcAcpiLib.h>
#include <Library/OcGuardLib.h>
#include <IndustryStandard/AcpiAml.h>
#include <UserFile.h>

/**
  Prints description of error occured in the perser.

  @param[in]   Status  Error occured in the parser.
**/
VOID
PrintParserError (
  IN EFI_STATUS Status
  )
{
  switch (Status) {
    case EFI_SUCCESS:
      break;

    case EFI_INVALID_PARAMETER:
      printf ("EXIT: Invalid parameter!\n");
      break;

    case EFI_DEVICE_ERROR:
      printf ("EXIT: ACPI table is incorrect or not supported by parser!\n");
      break;

    case EFI_NOT_FOUND:
      printf ("EXIT: No entry found in the table.\n");
      break;

    case EFI_OUT_OF_RESOURCES:
      printf ("EXIT: ACPI table has too much nesting!\n");
      break;

    case EFI_LOAD_ERROR:
      printf ("EXIT: File error in table file!\n");
      break;

    default:
      printf ("EXIT: Unknown error!\n");
      break;
  }

  return;
}

/**
  Finds offset of required entry in ACPI table in case it exists.

  @param[in]  FileName   Path to file containing ACPI table.
  @param[in]  PathString Path to entry which must be found.
  @param[in]  Entry      Number of entry which must be found.
  @param[out] Offset     Offset of the entry if it was found.

  @retval EFI_SUCCESS           Required entry was found.
  @retval EFI_NOT_FOUND         Required entry was not found.
  @retval EFI_DEVICE_ERROR      Error occured during parsing ACPI table.
  @retval EFI_OUT_OF_RESOURCES  Nesting limit has been reached.
  @retval EFI_INVALID_PARAMETER Got wrong path to the entry.
  @retval EFI_LOAD_ERROR        Wrong path to the file or the file can't
                                be opened.
**/
EFI_STATUS
AcpiFindEntryInFile (
  IN     CONST CHAR8 *FileName,
  IN     CONST CHAR8 *PathString,
  IN     UINT8       Entry,
     OUT UINT32      *Offset
  )
{
  UINT8               *TableStart;
  EFI_STATUS          Status;
  UINT32              TableLength;

  TableStart = UserReadFile (FileName, &TableLength);
  if (TableStart == NULL) {
    DEBUG ((DEBUG_INFO, "No file %a\n", FileName));
    return EFI_LOAD_ERROR;
  }

  Status = AcpiFindEntryInMemory (TableStart, PathString, Entry, Offset, TableLength);

  FreePool(TableStart);

  return Status;
}

// -[f|a] , CHAR8 ** memory_location , CHAR8 ** path , UINT8 occurance
/**
   Finds sought entry in ACPI table.
   Usage:
   ./ACPIe -f FileName Path [Entry]

   @param[in] FileName  Path to file with ACPI table.
   @param[in] Path      Path to required entry.
   @param[in] Entry     Number of required entry.

 **/
INT32
ENTRY_POINT (
  IN INT32       argc,
  IN CONST CHAR8 *argv[]
  )                     // ENTRY_POINT

{
  UINT32     ReturnedOffset;
  EFI_STATUS Status;

#if defined(VERBOSE) && !defined(FUZZING_TEST)
  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_VERBOSE | DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_VERBOSE | DEBUG_INFO;
#endif

  switch (argc) {
    case 5:

      if ((argv[1][0] == '-' && argv[1][1] == 'f') || (argv[1][0] == '-' && argv[1][1] == 'a')) {
        ReturnedOffset = 0;

        if (argv[1][0] == '-' && argv[1][1] == 'f') {
          DEBUG ((DEBUG_VERBOSE, "Entered main (file)\n"));
          Status = AcpiFindEntryInFile (
            argv[2],
            argv[3],
            atoi (argv[4]),
            &ReturnedOffset
            );

          if (Status == EFI_SUCCESS) {
            printf ("Returned offset: %d\n", ReturnedOffset);
          } else {
            PrintParserError (Status);
          }

          return 0;
        } else {
          DEBUG ((DEBUG_VERBOSE, "Entered main (address)\n"));
          Status = AcpiFindEntryInFile (
            argv[2],
            argv[3],
            atoi (argv[4]),
            &ReturnedOffset
            );

          if (Status == EFI_SUCCESS) {
            printf ("Returned offset: %d\n", ReturnedOffset);
          } else {
            PrintParserError (Status);
          }

          return 0;
        }
      } else {
        printf ("Usage: ACPIe -f *file* *search path* [number of occurance]\n");
        return 2;
      }

      break;

    case 4:

      if ((argv[1][0] == '-' && argv[1][1] == 'f') || (argv[1][0] == '-' && argv[1][1] == 'a')) {

        if (argv[1][0] == '-' && argv[1][1] == 'f') {
          DEBUG ((DEBUG_VERBOSE, "Entered main (file)\n"));
          Status = AcpiFindEntryInFile (
            argv[2],
            argv[3],
            1,
            &ReturnedOffset
            );

          if (Status == EFI_SUCCESS) {
            printf ("Returned offset: %d\n", ReturnedOffset);
          } else {
            PrintParserError (Status);
          }

          return 0;
        } else {
          DEBUG ((DEBUG_VERBOSE, "Entered main (address)\n"));
          Status = AcpiFindEntryInFile (
            argv[2],
            argv[3],
            1,
            &ReturnedOffset
            );

          if (Status == EFI_SUCCESS) {
            printf ("Returned offset: %d\n", ReturnedOffset);
          } else {
            PrintParserError (Status);
          }

          return 0;
        }
      } else {
        printf ("Usage: ACPIe -f *file* *search path* [number of occurance]\n");
        return 2;
      }

      break;

    default:
      printf ("Usage: ACPIe -f *file* *search path* [number of occurance]\n");
      return 0;

      break;
  }

  return 0;
}
INT32
LLVMFuzzerTestOneInput (
  CONST UINT8 *Data,
  UINTN       Size
  )
{
  if (Size > 0) {
    UINT32 offset = 0;
    AcpiFindEntryInMemory (
      (UINT8 *)Data, "_SB.PCI0.GFX0",
      1,
      &offset,
      (UINT32) Size
      );
  }

  return 0;
}
