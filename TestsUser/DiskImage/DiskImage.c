#include "../Include/Uefi.h"

#include <Library/OcAppleDiskImageLib.h>
#include <Library/OcCompressionLib.h>

/**

clang -g -fsanitize=undefined,address -Wno-incompatible-pointer-types-discards-qualifiers -I../Include -I../../Include -I../../../MdePkg/Include/ -I../../../EfiPkg/Include/ -include ../Include/Base.h DiskImage.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLib.c ../../Library/OcAppleDiskImageLib/OcAppleDiskImageLibInternal.c ../../Library/OcMiscLib/DataPatcher.c ../../Library/OcCompressionLib/zlib/zlib.c -o DiskImage

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

    EFI_STATUS                  Status;
    OC_APPLE_DISK_IMAGE_CONTEXT *DmgContext;

    Status = OcAppleDiskImageInitializeContext (Dmg, DmgSize, &DmgContext);
    if (EFI_ERROR (Status)) {
      printf ("Context initialization error %zx\n", Status);
      continue;
    }

    UncompSize = (DmgContext->Trailer.SectorCount * APPLE_DISK_IMAGE_SECTOR_SIZE);
    UncompDmg  = malloc (UncompSize);
    if (UncompDmg == NULL) {
      printf ("DMG data allocation failed.\n");
      continue;
    }

    Status = OcAppleDiskImageRead (DmgContext, 0, UncompSize, UncompDmg);
    if (EFI_ERROR (Status)) {
      printf ("DMG read error %zx\n", Status);
      continue;
    }

    printf ("Decompressed the entire DMG...\n");

    Status = OcAppleDiskImageFreeContext (DmgContext);
    if (EFI_ERROR (Status)) {
      printf ("Context destruction error %zx\n", Status);
      continue;
    }

    printf ("Success...\n");
  }

  return 0;
}
