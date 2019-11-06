#include <Base.h>

#include <Library/OcCryptoLib.h>
#include <Library/OcAppleKeysLib.h>

#include <BigNumLib.h>

uint8_t *readFile(const char *str, uint32_t *size) {
  FILE *f = fopen(str, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *string = malloc(fsize);
  fread(string, fsize, 1, f);
  fclose(f);

  *size = fsize;

  return string;
}

int verifyRsa (CONST OC_RSA_PUBLIC_KEY *PublicKey, char *Name)
{
  OC_BN_WORD N0Inv;
  UINTN ModulusSize = PublicKey->Hdr.NumQwords * sizeof (UINT64);

  OC_BN_WORD *RSqrMod = malloc(ModulusSize);
  if (RSqrMod == NULL) {
    printf ("memory allocation error!\n");
    return -1;
  }

  N0Inv = BigNumCalculateMontParams (
            RSqrMod,
            ModulusSize / OC_BN_WORD_SIZE,
            PublicKey->Data
            );

  printf (
    "%s: results: %d %d\n",
    Name,
    memcmp (
      RSqrMod,
      &PublicKey->Data[PublicKey->Hdr.NumQwords],
      ModulusSize
      ),
    N0Inv != PublicKey->Hdr.N0Inv
    );

  free(RSqrMod);
  return 0;
}

int main(int argc, char *argv[]) {
  unsigned int      Index;
  OC_RSA_PUBLIC_KEY *PublicKey;
  uint32_t          PkSize;

  for (Index = 1; Index < argc; ++Index) {
    PublicKey = (OC_RSA_PUBLIC_KEY *)readFile (argv[Index], &PkSize);
    if (PublicKey == NULL) {
      printf ("read error\n");
      return -1;
    }

    verifyRsa (PublicKey, argv[Index]);
    free (PublicKey);
  }

  for (Index = 0; Index < ARRAY_SIZE (PkDataBase); ++Index) {
    verifyRsa (PkDataBase[Index].PublicKey, "inbuilt");
  }

  return 0;
}
