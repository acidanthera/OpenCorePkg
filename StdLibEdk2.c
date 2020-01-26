#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SortLib.h>

typedef UINTN size_t;

typedef struct {
  size_t cursize;
  char   data[];
} POOL_HDR;

void *InternalAllocPool (size_t size)
{
  POOL_HDR *Pool;

  Pool = AllocatePool (sizeof (*Pool) + size);
  if (Pool == NULL) {
    return NULL;
  }

  Pool->cursize = size;
  return Pool->data;
}

void *calloc (size_t nitems, size_t size)
{
  VOID   *Memory;
  size_t CurSize;

  CurSize = nitems * size;
  Memory  = InternalAllocPool (CurSize);
  if (Memory != NULL) {
    ZeroMem (Memory, CurSize);
  }

  return Memory;
}

void *malloc (size_t size)
{
  return InternalAllocPool (size);
}

void *realloc (void *ptr, size_t size)
{
  POOL_HDR *OldPool;
  VOID     *NewMemory;

  NewMemory = InternalAllocPool (size);
  if (NewMemory != NULL && ptr != NULL) {
    OldPool = BASE_CR (ptr, POOL_HDR, data);
    CopyMem (NewMemory, OldPool, MIN (size, OldPool->cursize));
    FreePool (OldPool);
  }

  return NewMemory;
}

void free (void *ptr)
{
  if (ptr != NULL) {
    FreePool (BASE_CR (ptr, POOL_HDR, data));
  }
}

int memcmp (const void *ptr1, const void *ptr2, size_t num)
{
  return CompareMem (ptr1, ptr2, num);
}

void *memmove (void *destination, const void *source, size_t num)
{
  return CopyMem (destination, source, num);
}

void *memcpy (void *destination, const void *source, size_t num)
{
  return memmove (destination, source, num);
}

void *memset (void *ptr, int value, size_t num)
{
  return SetMem (ptr, num, value);
}

void *memchr (const void *ptr, int value, size_t num)
{
  char   *Buf;
  size_t Index;

  Buf = (char *)ptr;
  for (Index = 0; Index < num; ++Index) {
    if (Buf[Index] == value) {
      return (void *)&Buf[Index];
    }
  }

  return NULL;
}

char *strcat (char *destination, const char *source)
{
  // TODO: fix?
  return StrCatS (destination, MAX_UINTN, source);
}

int strcmp (const char *str1, const char *str2)
{
  return StrCmp (str1, str2);
}

char *strcpy (char *destination, const char *source)
{
  // TODO: fix?
  return StrCpyS (destination, MAX_UINTN, source);
}

size_t strlen (const char *str)
{
  return StrLen (str);
}

int strncmp (const char *str1, const char *str2, size_t num)
{
  return StrnCmp (str1, str2, num);
}

char *strncpy (char *destination, const char *source, size_t num)
{
  // TODO: fix?
  return StrnCpyS (destination, MAX_UINTN, source, num);
}

char *strrchr (const char *str, int character)
{
  size_t Index;

  Index = AsciiStrLen (str) + 1;
  while (Index != 0) {
    --Index;
    if (str[Index] == character) {
      return &str[Index];
    }
  }

  return NULL;
}

char *strstr (const char *str1, const char *str2)
{
  return StrStr (str1, str2);
}

// FIXME: NOT thread-safe, but using ST anyway right now.
STATIC int (*qsort_cmp_static)(const void *, const void*);

INTN
EFIAPI
QuickSortCompareStatic (
  IN CONST VOID                 *Buffer1,
  IN CONST VOID                 *Buffer2
  )
{
  qsort_cmp_static (Buffer1, Buffer2);
}

void qsort (void *base, size_t nitems, size_t size, int (*compar)(const void *, const void*))
{
  qsort_cmp_static = compar;
  PerformQuickSort (base, nitems, size, compar, QuickSortCompareStatic);
}
