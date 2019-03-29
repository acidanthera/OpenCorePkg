#include <Base.h>

#ifndef assert
#include <Library/DebugLib.h>
#define assert ASSERT
#endif

#ifndef malloc
#include <Library/MemoryAllocationLib.h>
#define malloc(Size)  (AllocatePool (Size))
#define free(Ptr)     (FreePool (Ptr))
#define sresize(OldBuffer, OldSize, NewSize, Type)  \
  ((Type *)ReallocatePool((OldSize) * sizeof(Type), (NewSize) * sizeof(Type), (OldBuffer)))
#endif

#ifndef memset
#include <Library/BaseMemoryLib.h>
#define memset(Memory, Value, Length)  (SetMem ((Memory), (Length), (Value)))
#endif

