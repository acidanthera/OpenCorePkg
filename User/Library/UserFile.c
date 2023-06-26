/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <UserFile.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#ifdef COVERAGE_TEST
  #if defined (__clang__)
void
__wrap_llvm_gcda_emit_arcs (
  uint32_t  num_counters,
  uint64_t  *counters
  )
{
  uint32_t  i;
  uint64_t  *old_ctrs = NULL;

  old_ctrs = malloc (num_counters * sizeof (uint64_t));
  if (old_ctrs == NULL) {
    return;
  }

  memcpy (old_ctrs, counters, num_counters * sizeof (uint64_t));

  __real_llvm_gcda_emit_arcs (num_counters, counters);

  for (i = 0; i < num_counters; ++i) {
    if ((old_ctrs[i] == counters[i]) && (counters[i] > 0)) {
      fprintf (stdout, "CoverageHit\n");
    }
  }

  free (old_ctrs);
}

  #elif defined (__GNUC__)
void
__gcov_merge_add (
  gcov_type  *counters,
  unsigned   n_counters
  )
{
  gcov_type  prev;

  for ( ; n_counters; counters++, n_counters--) {
    prev = __gcov_read_counter ();
    if ((prev == 0) && (*counters > 0)) {
      fprintf (stdout, "CoverageHit\n");
    }

    *counters += prev;
  }
}

  #endif
#endif

UINT8 *
UserReadFile (
  IN  CONST CHAR8  *FileName,
  OUT UINT32       *Size
  )
{
  FILE   *FilePtr;
  INT64  FileSize;
  UINT8  *Buffer;

  ASSERT (FileName != NULL);
  ASSERT (Size != NULL);

  FilePtr = fopen (FileName, "rb");
  if (FilePtr == NULL) {
    return NULL;
  }

  if (fseek (FilePtr, 0, SEEK_END) != 0) {
    fclose (FilePtr);
    return NULL;
  }

  FileSize = ftell (FilePtr);
  if (FileSize <= 0) {
    fclose (FilePtr);
    return NULL;
  }

  if (fseek (FilePtr, 0, SEEK_SET) != 0) {
    fclose (FilePtr);
    return NULL;
  }

  Buffer = AllocatePool ((UINTN)FileSize + 1);
  if (Buffer == NULL) {
    fclose (FilePtr);
    return NULL;
  }

  if (fread (Buffer, (size_t)FileSize, 1, FilePtr) != 1) {
    fclose (FilePtr);
    free (Buffer);
    return NULL;
  }

  fclose (FilePtr);

  Buffer[FileSize] = 0;
  *Size            = (UINT32)FileSize;

  return Buffer;
}

VOID
UserWriteFile (
  IN  CONST CHAR8  *FileName,
  IN  CONST VOID   *Data,
  IN  UINT32       Size
  )
{
  FILE  *FilePtr;

  ASSERT (FileName != NULL);
  ASSERT (Data != NULL);

  FilePtr = fopen (FileName, "wb");

  if (FilePtr == NULL) {
    abort ();
  }

  if (fwrite (Data, Size, 1, FilePtr) != 1) {
    abort ();
  }

  fclose (FilePtr);
}
