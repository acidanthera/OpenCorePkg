/** @file
  Copyright (C) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_TYPING_LIB_H
#define OC_TYPING_LIB_H

#include <Uefi.h>
#include <Protocol/AppleEvent.h>
#include <Library/DebugLib.h>

#if !defined(OC_TRACE_TYPING)
#define OC_TRACE_TYPING DEBUG_VERBOSE
#endif

//
// Define OC_TRACE_KEY_TIMES to allocate additional memory and enable
// additional code to print time in millis at which key events were recieved.
// Key received times in TSC units are quickly stored in buffer, then more
// slowly converted to millis and printed later, as dequeued.
// Has effect on NOOPT and DEBUG builds only.
//

//#define OC_TRACE_KEY_TIMES

//
// Max. num. keystrokes buffered is one less than buffer size.
// 20 would be 1s of keystrokes at 50ms repeat, and it also
// gives a fair size to handle any user key mashing.
//
#define OC_TYPING_BUFFER_SIZE    21

#pragma pack(1)

typedef PACKED struct {
  APPLE_KEY_CODE              AppleKeyCode;
  CHAR16                      UnicodeChar;
} OC_TYPING_BUFFER_ENTRY;

typedef PACKED struct {
  OC_TYPING_BUFFER_ENTRY      Buffer[OC_TYPING_BUFFER_SIZE];
  APPLE_MODIFIER_MAP          CurrentModifiers;
  UINT16                      Pad1;
  APPLE_EVENT_HANDLE          Handle;
  UINTN                       Head;
  UINTN                       Tail;
  UINT64                      *KeyTimes; // only used in DEBUG builds with OC_TRACE_KEY_TIMES defined
} OC_TYPING_CONTEXT;

#pragma pack()

/**
  Register typing handler with Apple Event protocol.

  @param[out]     Context             Typing handler context.

  @retval EFI_SUCCESS                 Registered successfully.
  @retval EFI_OUT_OF_RESOURCES        Could not allocate buffer memory.
  @retval other                       An error returned by a sub-operation.
**/
EFI_STATUS
OcRegisterTypingHandler (
  OUT OC_TYPING_CONTEXT   **Context
  );

/**
  Unregister typing handler.

  @param[in]      Context             Typing handler context.

  @retval EFI_SUCCESS                 Unregistered successfully, or context was already null.
  @retval EFI_NOT_STARTED             Null Apple Event handle present in context.
  @retval other                       An error returned by a sub-operation.
**/
EFI_STATUS
OcUnregisterTypingHandler (
   IN OC_TYPING_CONTEXT   **Context
  );

/**
  Get next keystroke from typing buffer. Will always return immediately.

  @param[in]      Context         Typing handler context.
  @param[out]     Modifiers       Current key modifiers, returned even if no key is available.
  @param[out]     AppleKeyCode    Next keycode if one is available, zero otherwsie.
  @param[out]     UnicodeChar     Next unicode char if one is available, L'\0' otherwsie.
**/
VOID
OcGetNextKeystroke (
   IN OC_TYPING_CONTEXT           *Context,
  OUT APPLE_MODIFIER_MAP          *Modifiers,
  OUT APPLE_KEY_CODE              *AppleKeyCode,
  OUT CHAR16                      *UnicodeChar
  );

/**
  Flush typing buffer.

  @param[in]      Context             Typing handler context.

**/
VOID
OcFlushTypingBuffer (
   IN OC_TYPING_CONTEXT           *Context
  );

#endif // OC_TYPING_LIB_H
