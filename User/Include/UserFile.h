/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_USER_FILE_H
#define OC_USER_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
  Read a file on disk into buffer.

  @param[in]  FileName  ASCII string containing the name of the file to be opened.
  @param[out] Size      Size of the file.

  @return  A pointer to buffer containing the content of FileName.
**/
UINT8 *
UserReadFile (
  IN  CONST CHAR8 *FileName,
  OUT UINT32      *Size
  );

/**
  Write the buffer to the file.

  @param[in]  FileName  Output filename.
  @param[in]  Data      Data to write.
  @param[in]  Size      Size of the output file.
**/
VOID
UserWriteFile (
  IN  CONST CHAR8 *FileName,
  IN  VOID        *Data,
  IN  UINT32      Size
  );

#endif // OC_USER_FILE_H
