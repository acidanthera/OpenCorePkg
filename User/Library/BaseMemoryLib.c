#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

#include <string.h>
#include <stdlib.h>

VOID *
EFIAPI
CopyMem (
  OUT VOID       *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  )
{
  return memmove (DestinationBuffer, SourceBuffer, Length);
}

VOID *
EFIAPI
SetMem (
  OUT VOID  *Buffer,
  IN UINTN  Length,
  IN UINT8  Value
  )
{
  return memset (Buffer, Value, Length);
}

VOID *
EFIAPI
ZeroMem (
  OUT VOID  *Buffer,
  IN UINTN  Length
  )
{
  return memset (Buffer, 0, Length);
}

VOID *
EFIAPI
AllocatePool (
  IN UINTN  AllocationSize
  )
{
  return malloc (AllocationSize);
}

VOID *
EFIAPI
AllocateZeroPool (
  IN UINTN  AllocationSize
  )
{
  VOID *Memory;

  Memory = AllocatePool (AllocationSize);

  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

VOID
EFIAPI
FreePool (
  IN VOID   *Buffer
  )
{
  ASSERT (Buffer != NULL);
  free (Buffer);
}

GUID *
EFIAPI
CopyGuid (
  OUT GUID       *DestinationGuid,
  IN CONST GUID  *SourceGuid
  )
{
  memmove (DestinationGuid, SourceGuid, sizeof (GUID));
  return DestinationGuid;
}

UINT16
EFIAPI
ReadUnaligned16 (
  IN CONST UINT16              *Buffer
  )
{
  UINT16 Value;
  memmove (&Value, Buffer, sizeof (UINT16));
  return Value;
}

UINT16
EFIAPI
WriteUnaligned16 (
  OUT UINT16                    *Buffer,
  IN  UINT16                    Value
  )
{
  ASSERT (Buffer != NULL);

  memmove (Buffer, &Value, sizeof (UINT16));
  return Value;
}

UINT32
EFIAPI
ReadUnaligned32 (
  IN CONST UINT32              *Buffer
  )
{
  UINT32 Value;
  memmove (&Value, Buffer, sizeof (UINT32));
  return Value;
}
