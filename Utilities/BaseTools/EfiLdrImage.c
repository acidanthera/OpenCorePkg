/** @file
Creates and EFILDR image.
This tool combines several PE Image files together using following format denoted as EBNF:
FILE := EFILDR_HEADER
        EFILDR_IMAGE +
        <PeImageFileContent> +
The order of EFILDR_IMAGE is same as the order of placing PeImageFileContent.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHWARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MAX_PE_IMAGES                  63
#define FILE_TYPE_FIXED_LOADER         0
#define FILE_TYPE_RELOCATABLE_PE_IMAGE 1

typedef struct {
  uint32_t CheckSum;
  uint32_t Offset;
  uint32_t Length;
  uint8_t  FileName[52];
} EFILDR_IMAGE;

typedef struct {
  uint32_t       Signature;
  uint32_t       HeaderCheckSum;
  uint32_t       FileLength;
  uint32_t       NumberOfImages;
} EFILDR_HEADER;

//
// Utility Name
//
#define UTILITY_NAME  "EfiLdrImage"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 1
#define UTILITY_MINOR_VERSION 0

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
  printf ("Usage: EfiLdrImage -o OutImage LoaderImage PeImage1 PeImage2 ... PeImageN\n");
  printf ("%s Version %d.%d\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2017 Intel Corporation. All rights reserved.\n");
  printf ("\n  The EfiLdrImage tool is used to combine PE files into EFILDR image with Efi loader header.\n");
}

int
CountVerboseLevel (
  const char* VerboseLevelString,
  const uint64_t Length,
  uint64_t *ReturnValue
)
{
  uint64_t i = 0;
  for (;i < Length; ++i) {
    if (VerboseLevelString[i] != 'v' && VerboseLevelString[i] != 'V') {
      return -1;
    }
    ++(*ReturnValue);
  }

  return 0;
}

uint64_t
FCopyFile (
  FILE    *in,
  FILE    *out
  )
/*++
Routine Description:
  Write all the content of input file to output file.

Arguments:
  in  - input file pointer
  out - output file pointer

Return:
  uint64_t : file size of input file
--*/
{
  uint32_t          filesize, offset, length;
  char              Buffer[8*1024];

  fseek (in, 0, SEEK_END);
  filesize = ftell(in);

  fseek (in, 0, SEEK_SET);

  offset = 0;
  while (offset < filesize)  {
    length = sizeof(Buffer);
    if (filesize-offset < length) {
      length = filesize-offset;
    }

    int r = fread (Buffer, length, 1, in);
    if (r < 0) {
      abort();
    }
    fwrite (Buffer, length, 1, out);
    offset += length;
  }

  return filesize;
}


int
main (
  int argc,
  char *argv[]
  )
/*++

Routine Description:


Arguments:


Returns:


--*/
{
  uint64_t      i;
  uint64_t      filesize;
  FILE          *fpIn, *fpOut;
  EFILDR_HEADER EfiLdrHeader;
  EFILDR_IMAGE  EfiLdrImage[MAX_PE_IMAGES];
  char*         OutputFileName = NULL;
  char*         InputFileNames[MAX_PE_IMAGES + 1];
  char*         InputName;
  uint8_t       InputFileCount = 0;
  uint64_t      DebugLevel = 0;
  uint64_t      VerboseLevel = 0;
  int           Status = 0;

  if (argc == 1) {
    printf ("Usage: EfiLdrImage -o OutImage LoaderImage PeImage1 PeImage2 ... PeImageN\n");
    return EXIT_FAILURE;
  }

  argc --;
  argv ++;

  if ((strcmp (argv[0], "-h") == 0) || (strcmp (argv[0], "--help") == 0)) {
    Usage();
    return EXIT_SUCCESS;
  }

  if (strcmp (argv[0], "--version") == 0) {
    Version();
    return EXIT_SUCCESS;
  }

  while (argc > 0) {

    if ((strcmp (argv[0], "-o") == 0) || (strcmp (argv[0], "--output") == 0)) {
      OutputFileName = argv[1];
      if (OutputFileName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Output file can't be null");
        return EXIT_FAILURE;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((strcmp (argv[0], "-q") == 0) || (strcmp (argv[0], "--quiet") == 0)) {
      argc --;
      argv ++;
      continue;
    }

    if ((strlen(argv[0]) >= 2 && argv[0][0] == '-' && (argv[0][1] == 'v' || argv[0][1] == 'V')) || (strcmp (argv[0], "--verbose") == 0)) {
      VerboseLevel = 1;
      if (strlen(argv[0]) > 2) {
        Status = CountVerboseLevel (&argv[0][2], strlen(argv[0]) - 2, &VerboseLevel);
        if (Status != 0) {
          Error (NULL, 0, 1003, "Invalid option value", "%s", argv[0]);
          return EXIT_FAILURE;
        }
      }

      argc --;
      argv ++;
      continue;
    }

    if ((strcmp (argv[0], "-d") == 0) || (strcmp (argv[0], "--debug") == 0)) {
      DebugLevel = strtoull (argv[1], NULL, 0);
      argc -= 2;
      argv += 2;
      continue;
    }
    //
    // Don't recognize the parameter, should be regarded as the input file name.
    //
    InputFileNames[InputFileCount] = argv[0];
    InputFileCount++;
    argc--;
    argv++;
  }

  if (InputFileCount == 0) {
    Error (NULL, 0, 1001, "Missing option", "No input file");
    return EXIT_FAILURE;
  }
  //
  // Open output file for write
  //
  if (OutputFileName == NULL) {
    Error (NULL, 0, 1001, "Missing option", "No output file");
    return EXIT_FAILURE;
  }

  fpOut = fopen (OutputFileName, "w+b");
  if (!fpOut) {
    Error (NULL, 0, 0001, "Could not open output file", OutputFileName);
    return EXIT_FAILURE;
  }

  memset (&EfiLdrHeader, 0, sizeof (EfiLdrHeader));
  memset (&EfiLdrImage, 0, sizeof (EFILDR_IMAGE) * (InputFileCount));

  memcpy (&EfiLdrHeader.Signature, "EFIL", 4);
  EfiLdrHeader.FileLength = sizeof(EFILDR_HEADER) + sizeof(EFILDR_IMAGE)*(InputFileCount);

  //
  // Skip the file header first
  //
  fseek (fpOut, EfiLdrHeader.FileLength, SEEK_SET);

  //
  // copy all the input files to the output file
  //
  for(i=0;i<InputFileCount;i++) {
    //
    // Copy the content of PeImage file to output file
    //
    fpIn = fopen (InputFileNames[i], "rb");
    if (!fpIn) {
      Error (NULL, 0, 0001, "Could not open input file", InputFileNames[i]);
      fclose (fpOut);
      return EXIT_FAILURE;
    }
    filesize = FCopyFile (fpIn, fpOut);
    fclose(fpIn);

    //
    //  And in the same time update the EfiLdrHeader and EfiLdrImage array
    //
    EfiLdrImage[i].Offset = EfiLdrHeader.FileLength;
    EfiLdrImage[i].Length = (uint32_t) filesize;
    InputName = strrchr(InputFileNames[i], '/');
    if (InputName == NULL) {
      InputName = strrchr(InputFileNames[i], '\\');
      if (InputName == NULL) {
        InputName = InputFileNames[i];
      } else {
        ++InputName;
      }
    } else {
      ++InputName;
    }
    strncpy ((char*) EfiLdrImage[i].FileName, InputName, sizeof (EfiLdrImage[i].FileName) - 1);
    EfiLdrHeader.FileLength += (uint32_t) filesize;
    EfiLdrHeader.NumberOfImages++;
  }

  //
  // Write the image header to the output file finally
  //
  fseek (fpOut, 0, SEEK_SET);
  fwrite (&EfiLdrHeader, sizeof(EFILDR_HEADER)        , 1, fpOut);
  fwrite (&EfiLdrImage , sizeof(EFILDR_IMAGE)*(InputFileCount), 1, fpOut);

  fclose (fpOut);
  printf ("Created %s\n", OutputFileName);
  return 0;
}

