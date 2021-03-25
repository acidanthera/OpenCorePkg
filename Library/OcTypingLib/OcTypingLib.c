/** @file
  Apple Event typing.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcAppleEventLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcTimerLib.h>
#include <Library/OcTypingLib.h>

STATIC APPLE_EVENT_PROTOCOL     *mProtocol;

STATIC
VOID
EFIAPI
HandleKeyEvent (
  IN APPLE_EVENT_INFORMATION    *Information,
  IN OC_TYPING_CONTEXT          *Context
  )
{
  APPLE_KEY_CODE        AppleKeyCode;
  CHAR16                UnicodeChar;
  UINTN                 NewHead;

  AppleKeyCode = 0;
  UnicodeChar  = L'\0';
  
  if (Information->EventData.KeyData != NULL) {
    AppleKeyCode = Information->EventData.KeyData->AppleKeyCode;
    UnicodeChar = Information->EventData.KeyData->InputKey.UnicodeChar;
  }

  DEBUG ((OC_TRACE_TYPING, "OCTY: HandleKeyEvent[%p] %x:%d[%x] %c\n", &HandleKeyEvent, Information->EventType, Information->Modifiers, AppleKeyCode, UnicodeChar));

  //
  // We now handle modifiers immediately, rather than in the queue. By comparison this reduces
  // memory footprint, simplifies queue and dequeue code and removes any possibility of
  // stuck modifiers if queue fills. It also still behaves at least as well from end user pov.
  // Difference is that modifiers held when action occurs are what apply, not modifiers
  // held when key which caused action was pressed - but it is not at all clear that
  // this is incorrect, it may even be more correct.
  //
  if ((Information->EventType & APPLE_EVENT_TYPE_MODIFIER_UP) != 0) {
    Context->CurrentModifiers &= ~Information->Modifiers;
    return;
  } else if ((Information->EventType & APPLE_EVENT_TYPE_MODIFIER_DOWN) != 0) {
    Context->CurrentModifiers |= Information->Modifiers;
    return;
  } else if ((Information->EventType & APPLE_EVENT_TYPE_KEY_DOWN) == 0) {
    return;
  }

  NewHead = Context->Head + 1;
  if (NewHead >= OC_TYPING_BUFFER_SIZE) {
    NewHead = 0;
  }

  //
  // Now that we are returning key presses only (not modifiers) in the queue, just drop any over queue length
  //
  if (NewHead == Context->Tail) {
    return;
  }

  Context->Head = NewHead;

  DEBUG_CODE_BEGIN ();
  ASSERT (AppleKeyCode != 0);
  DEBUG_CODE_END ();
  
  DEBUG ((OC_TRACE_TYPING, "OCTY: Writing to %d\n", Context->Head));

  Context->Buffer[Context->Head].AppleKeyCode = AppleKeyCode;
  Context->Buffer[Context->Head].UnicodeChar  = UnicodeChar;

#if defined(OC_TRACE_KEY_TIMES)
  DEBUG_CODE_BEGIN ();
  Context->KeyTimes[Context->Head] = GetPerformanceCounter ();
  DEBUG_CODE_END ();
#endif
}

EFI_STATUS
OcRegisterTypingHandler (
  OUT OC_TYPING_CONTEXT   **Context
  )
{
  EFI_STATUS    Status;

  DEBUG ((OC_TRACE_TYPING, "OCTY: OcRegisterTypingHandler\n"));

  *Context = NULL;

  Status = gBS->LocateProtocol (
    &gAppleEventProtocolGuid,
    NULL,
    (VOID **) &mProtocol
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCTY: Failed to locate apple event protocol - %r\n", Status));
    return Status;
  }

  DEBUG ((OC_TRACE_TYPING, "OCTY: About to allocate %d x %d for buffer, plus rest of context = %d\n", OC_TYPING_BUFFER_SIZE, sizeof((*Context)->Buffer[0]), sizeof(**Context)));

  *Context = AllocatePool (sizeof(**Context));

  if (*Context == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "OCTY: Failed to allocate context - %r\n", Status));
    return Status;
  }

  (*Context)->KeyTimes = NULL;

#if defined(OC_TRACE_KEY_TIMES)
  DEBUG_CODE_BEGIN ();
  DEBUG ((OC_TRACE_TYPING, "OCTY: About to allocate %d x %d for key times buffer\n", OC_TYPING_BUFFER_SIZE, sizeof((*Context)->KeyTimes[0])));

  (*Context)->KeyTimes = AllocatePool (OC_TYPING_BUFFER_SIZE * sizeof((*Context)->KeyTimes[0]));

  if ((*Context)->KeyTimes == NULL) {
    DEBUG ((DEBUG_ERROR, "OCTY: Failed to allocate typing context key times buffer - %r\n", Status));
    FreePool (*Context);
    *Context = NULL;
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }
  DEBUG_CODE_END ();
#endif

  DEBUG ((OC_TRACE_TYPING, "OCTY: RegisterHandler (F, [%p], %p, %p)\n", &HandleKeyEvent, &(*Context)->Handle, *Context));
  Status = mProtocol->RegisterHandler (
    APPLE_ALL_KEYBOARD_EVENTS,
    (APPLE_EVENT_NOTIFY_FUNCTION)&HandleKeyEvent,
    &(*Context)->Handle,
    *Context
  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCTY: Failed to register handler - %r\n", Status));
#if defined(OC_TRACE_KEY_TIMES)
    DEBUG_CODE_BEGIN ();
    FreePool ((*Context)->KeyTimes);
    DEBUG_CODE_END ();
#endif
    FreePool (*Context);
    *Context = NULL;
    return Status;
  }

  //
  // Init
  //
  OcFlushTypingBuffer (*Context);

  DEBUG ((OC_TRACE_TYPING, "OCTY: reg c=%p h=%p k=%p\n", *Context, (*Context)->Handle, (*Context)->KeyTimes));
  DEBUG ((DEBUG_INFO, "OCTY: Registered handler\n"));

  return EFI_SUCCESS;
}

EFI_STATUS
OcUnregisterTypingHandler (
  IN OC_TYPING_CONTEXT   **Context
  )
{
  EFI_STATUS    Status;

  ASSERT (Context != NULL);

  if (*Context == NULL) {
    return EFI_SUCCESS;
  }

  DEBUG ((OC_TRACE_TYPING, "OCTY: unreg c=%p h=%p k=%p\n", *Context, (*Context)->Handle, (*Context)->KeyTimes));

  if ((*Context)->Handle == NULL) {
    Status = EFI_NOT_STARTED;
  } else {
    Status = mProtocol->UnregisterHandler ((*Context)->Handle);
    (*Context)->Handle = NULL;

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCTY: Failed to unregister handler - %r\n", Status));
    } else {
      // show this here, before freeing the pools; any stray events in log after this point would be a problem
      DEBUG ((DEBUG_INFO, "OCTY: Unregistered handler\n"));
    }
  }

#if defined(OC_TRACE_KEY_TIMES)
  DEBUG_CODE_BEGIN ();
  if ((*Context)->KeyTimes != NULL) {
    FreePool ((*Context)->KeyTimes);
    (*Context)->KeyTimes = NULL;
  }
  DEBUG_CODE_END ();
#endif

  FreePool (*Context);
  *Context = NULL;

  //
  // Prevent chars which have queued up in ConIn (which is not used by us) affecting subsequent applications.
  //
  OcConsoleFlush ();

  return Status;
}

VOID
OcGetNextKeystroke (
   IN OC_TYPING_CONTEXT           *Context,
  OUT APPLE_MODIFIER_MAP          *Modifiers,
  OUT APPLE_KEY_CODE              *AppleKeyCode,
  OUT CHAR16                      *UnicodeChar
  )
{
  UINTN   NewTail;
  UINT64  CurrentMillis;

  CurrentMillis = 0;

  ASSERT (Context != NULL);

  DEBUG ((OC_TRACE_TYPING, "OCTY: OcGetNextKeystroke(%p, M, K)\n", Context));

  *Modifiers      = Context->CurrentModifiers;
  *AppleKeyCode   = 0;
  *UnicodeChar    = L'\0';

  //
  // Buffer empty.
  //
  if (Context->Tail == Context->Head) {
    return;
  }

  DEBUG ((OC_TRACE_TYPING, "OCTY: %d != %d\n", Context->Tail, Context->Head));

  //
  // Write can potentially occur in the middle of read, so don't update
  // Context->Tail until data is read.
  //
  NewTail = Context->Tail + 1;

  if (NewTail >= OC_TYPING_BUFFER_SIZE) {
    NewTail = 0;
  }

  DEBUG ((OC_TRACE_TYPING, "OCTY: Reading from %d\n", NewTail));

  *AppleKeyCode = Context->Buffer[NewTail].AppleKeyCode;
  *UnicodeChar  = Context->Buffer[NewTail].UnicodeChar;

  Context->Tail = NewTail;

#if defined(OC_TRACE_KEY_TIMES)
  DEBUG_CODE_BEGIN ();
  CurrentMillis = DivU64x64Remainder (GetTimeInNanoSecond (Context->KeyTimes[Context->Tail]), 1000000ULL, NULL);
  DEBUG ((DEBUG_INFO, "OCTY: OcGetNextKeystroke @%d %d[%x] %,Lu\n", Context->Tail, *Modifiers, *AppleKeyCode, CurrentMillis));
  DEBUG_CODE_END ();
#else
  DEBUG ((OC_TRACE_TYPING, "OCTY: OcGetNextKeystroke @%d %d[%x] %,Lu\n", Context->Tail, *Modifiers, *AppleKeyCode, CurrentMillis));
#endif
}

VOID
OcFlushTypingBuffer (
   IN OC_TYPING_CONTEXT           *Context
  )
{
  Context->Tail = 0;
  Context->Head = 0;
  Context->CurrentModifiers = 0;
  DEBUG ((OC_TRACE_TYPING, "OCTY: OcFlushTypingBuffer %d %d %d\n", Context->Tail, Context->Head, Context->CurrentModifiers));
}
