/** @file

AppleEfiSignTool – Tool for signing and verifying Apple EFI binaries.

Copyright (c) 2018, savvas

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "AppleEfiPeImage.h"

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

static uint8_t *Image      = NULL;
static uint32_t ImageSize  = 0;

static char UsageBanner[] = "AppleEfiSignTool v1.0 – Tool for signing and verifying\n"
                            "Apple EFI binaries. It supports PE and Fat binaries.\n"
                            "Usage:\n"
                            "  -i : input file\n"
                            "  -h : show this text\n"
                            "Example: ./AppleEfiSignTool -i apfs.efi\n";


void
OpenFile (
  char *FileName
  )
{
  FILE *ImageFp;
  ImageFp = fopen (FileName, "rb");

  if (ImageFp == NULL) {
    fprintf (stderr, "File not exist, errno = %d\n",errno);
    exit (EXIT_FAILURE);
  }

  //CHECKME: does not guarantee that the file was read correctly!
  fseek (ImageFp, 0, SEEK_END);
  ImageSize = (uint32_t) ftell (ImageFp);
  rewind (ImageFp);
  Image = malloc (ImageSize + 1);
  if (fread (Image, ImageSize, 1, ImageFp) != 1)
    abort();
  fclose (ImageFp);
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  int Opt;

  if (argc == 1){
    puts(UsageBanner);
    exit(EXIT_FAILURE);
  }

  while ((Opt = getopt (argc, argv, "i:vh")) != -1) {
    switch (Opt) {
      case 'i': {
        //
        // Open input file
        //
        OpenFile (optarg);
        break;
      }
      case 'h': {
        puts(UsageBanner);
        exit(0);
        break;
      }
      default:
        puts(UsageBanner);
        exit(EXIT_FAILURE);
    }
  }
  for(int i = optind; i < argc; i++) {
    printf("Unknown argument: %s\n", argv[i]);
    puts(UsageBanner);
    exit(EXIT_FAILURE);
  }


  int code = VerifyAppleImageSignature (Image, ImageSize);

  free(Image);

  return code;
}
