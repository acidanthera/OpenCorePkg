#include "../Include/Uefi.h"

#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcCompressionLib.h>

/**

clang -g -fsanitize=undefined,address -Wno-incompatible-pointer-types-discards-qualifiers -I../Include -I../../Include -I../../../MdePkg/Include/ -I../../../EfiPkg/Include/ -include ../Include/Base.h DiskImage.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLibInternal.c ../../Library/OcMiscLib/DataPatcher.c ../../Library/OcCompressionLib/zlib/zlib.c -o DiskImage

clang-mp-7.0 -DFUZZING_TEST=1 -g -fsanitize=undefined,address,fuzzer -Wno-incompatible-pointer-types-discards-qualifiers -I../Include -I../../Include -I../../../MdePkg/Include/ -I../../../EfiPkg/Include/ -include ../Include/Base.h DiskImage.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLibInternal.c ../../Library/OcMiscLib/DataPatcher.c ../../Library/OcCompressionLib/zlib/zlib.c -o DiskImage
rm -rf DICT fuzz*.log ; mkdir DICT ; UBSAN_OPTIONS='halt_on_error=1' ./DiskImage -jobs=4 DICT -rss_limit_mb=4096

**/

uint8_t *readFile(const char *str, uint32_t *size) {
  FILE *f = fopen(str, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *string = malloc(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);

  string[fsize] = 0;
  *size = fsize;

  return string;
}

#ifdef FUZZING_TEST
#define main no_main
#include <sanitizer/asan_interface.h>
#else
#define ASAN_POISON_MEMORY_REGION(addr, size)
#define ASAN_UNPOISON_MEMORY_REGION(addr, size)
#endif

int main (int argc, char *argv[]) {
  if (argc < 2) {
    printf ("Please provide a valid Disk Image path.\n");
    return -1;
  }

  uint8_t  *Dmg;
  uint32_t DmgSize;
  uint8_t  *UncompDmg = NULL;
  uint32_t UncompSize;

  for (int i = 1; i < argc; ++i, free (Dmg), free (UncompDmg), UncompDmg = NULL) {
    if ((Dmg = readFile (argv[i], &DmgSize)) == NULL) {
      printf ("Read fail\n");
      continue;
    }

    BOOLEAN                     Result;
    OC_APPLE_DISK_IMAGE_CONTEXT *DmgContext;

    Result = OcAppleDiskImageInitializeContext (Dmg, DmgSize, &DmgContext);
    if (!Result) {
      printf ("Context initialization error\n");
      continue;
    }

    UncompSize = (DmgContext->Trailer.SectorCount * APPLE_DISK_IMAGE_SECTOR_SIZE);
    UncompDmg  = malloc (UncompSize);
    if (UncompDmg == NULL) {
      printf ("DMG data allocation failed.\n");
      continue;
    }

    Result = OcAppleDiskImageRead (DmgContext, 0, UncompSize, UncompDmg);
    if (!Result) {
      printf ("DMG read error\n");
      continue;
    }

    printf ("Decompressed the entire DMG...\n");

    OcAppleDiskImageFreeContext (DmgContext);

    printf ("Success...\n");
  }

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  #define MAX_INPUT 256
  #define MAX_OUTPUT 4096

  if (Size > MAX_INPUT) {
    return 0;
  }

  UINT8 *Test = AllocateZeroPool (MAX_OUTPUT);
  if (Test == NULL) {
    return 0;
  }

  for (size_t Index = 0; Index < 4096; ++Index) {
    ASAN_POISON_MEMORY_REGION (Test + Index, MAX_OUTPUT - Index);
    UINT32 CurrentLength = DecompressZLIB (
                           Test,
                           Index,
                           Data,
                           Size
                           );
    ASAN_UNPOISON_MEMORY_REGION (Test + Index, MAX_OUTPUT - Index);
    ASSERT (CurrentLength <= Index);
  }
  return 0;
}
