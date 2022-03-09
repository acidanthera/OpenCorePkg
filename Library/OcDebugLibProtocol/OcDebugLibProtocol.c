/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.
  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Protocol/OcLog.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugPrintErrorLevelLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define OC_LOG_BUFFER_SIZE (2 * EFI_PAGE_SIZE)

STATIC EFI_EVENT  mLogProtocolArrivedNotifyEvent;
STATIC VOID       *mLogProtocolArrivedNotifyRegistration;

STATIC UINT8      *mLogBuffer   = NULL;
STATIC UINT8      *mLogWalker   = NULL;
STATIC UINTN      mLogCount     = 0;

STATIC
OC_LOG_PROTOCOL *
InternalGetOcLog (
  VOID
  )
{
  EFI_STATUS  Status;

  STATIC OC_LOG_PROTOCOL *mInternalOcLog = NULL;

  if (mInternalOcLog == NULL) {
    Status = gBS->LocateProtocol (
      &gOcLogProtocolGuid,
      NULL,
      (VOID **) &mInternalOcLog
      );

    if (EFI_ERROR (Status) || mInternalOcLog->Revision != OC_LOG_REVISION) {
      mInternalOcLog = NULL;
    }
  }

  return mInternalOcLog;
}

STATIC
VOID
EFIAPI
LogProtocolArrivedNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN            Index;
  EFI_STATUS       Status;
  OC_LOG_PROTOCOL  *OcLog;
  CHAR8            *Walker;
  UINTN            ErrorLevel;
  OC_LOG_OPTIONS   CurrLogOpt;

  //
  // Event arrives. Close it.
  //
  gBS->CloseEvent (mLogProtocolArrivedNotifyEvent);

  Status = gBS->LocateProtocol (
    &gOcLogProtocolGuid,
    NULL,
    (VOID **) &OcLog
    );

  if (EFI_ERROR (Status) || OcLog->Revision != OC_LOG_REVISION) {
    return;
  }

  Walker = (CHAR8 *) mLogBuffer;
  for (Index = 0; Index < mLogCount; ++Index) {
    //
    // Set ErrorLevel.
    //
    CopyMem (&ErrorLevel, Walker, sizeof (ErrorLevel));
    Walker += sizeof (ErrorLevel);

    //
    // Print debug message without onscreen, as it is done by OutputString.
    //
    CurrLogOpt = OcLog->Options;
    OcLog->Options &= ~OC_LOG_CONSOLE;
    DebugPrint (ErrorLevel, "%a", Walker);
    //
    // Restore original value.
    //
    OcLog->Options = CurrLogOpt;

    //
    // Skip message chars.
    //
    while (*Walker != '\0') {
      ++Walker;
    }
    //
    // Matched '\0'. Go to the next message.
    //
    ++Walker;
  }

  FreePool (mLogBuffer);
  mLogBuffer = NULL;
}

STATIC
VOID
OcBufferEarlyLog (
  IN  UINTN         ErrorLevel,
  IN  CONST CHAR16  *Buffer
  )
{
  EFI_STATUS  Status;

  if (mLogBuffer == NULL) {
    mLogBuffer = AllocatePool (OC_LOG_BUFFER_SIZE);
    if (mLogBuffer == NULL) {
      return;
    }

    mLogWalker = mLogBuffer;

    //
    // Notify when log protocol arrives.
    //
    Status = gBS->CreateEvent (
      EVT_NOTIFY_SIGNAL,
      TPL_NOTIFY,
      LogProtocolArrivedNotify,
      NULL,
      &mLogProtocolArrivedNotifyEvent
      );

    if (!EFI_ERROR (Status)) {
      Status = gBS->RegisterProtocolNotify (
        &gOcLogProtocolGuid,
        mLogProtocolArrivedNotifyEvent,
        &mLogProtocolArrivedNotifyRegistration
        );

      if (EFI_ERROR (Status)) {
        gBS->CloseEvent (mLogProtocolArrivedNotifyEvent);
      }
    }
  }

  //
  // The size of ErrorLevel and that of Buffer string including null terminator should not overflow.
  // The current position (mLogWalker) has been moved to the right side for the same purpose.
  //
  if ((sizeof (ErrorLevel) + StrLen (Buffer) + 1) <= (UINTN) (mLogBuffer + OC_LOG_BUFFER_SIZE - mLogWalker)) {
    //
    // Store ErrorLevel into buffer.
    //
    CopyMem (mLogWalker, &ErrorLevel, sizeof (ErrorLevel));
    mLogWalker += sizeof (ErrorLevel);

    //
    // Store logs into buffer.
    //
    while (*Buffer != CHAR_NULL) {
      *mLogWalker++ = (UINT8) *Buffer++;
    }
    *mLogWalker++ = CHAR_NULL;

    //
    // Increment number of messages.
    //
    ++mLogCount;
  }
}

/**
  Prints a debug message to the debug output device if the specified error level is enabled.
  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and the
  associated variable argument list to the debug output device.
  If Format is NULL, then ASSERT().
  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.
  @param  ...         A variable argument list whose contents are accessed
                      based on the format string specified by Format.
**/
VOID
EFIAPI
DebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST          Marker;
  CHAR16           Buffer[256];
  OC_LOG_PROTOCOL  *OcLog;
  BOOLEAN          IsBufferEarlyLogEnabled;
  BOOLEAN          ShouldPrintConsole;

  ASSERT (Format != NULL);

  OcLog = InternalGetOcLog ();
  IsBufferEarlyLogEnabled = PcdGetBool (PcdDebugLibProtocolBufferEarlyLog);
  ShouldPrintConsole = (ErrorLevel & GetDebugPrintErrorLevel ()) != 0;

  VA_START (Marker, Format);

  if (OcLog != NULL) {
    OcLog->AddEntry (OcLog, ErrorLevel, Format, Marker);
  } else if (IsBufferEarlyLogEnabled || ShouldPrintConsole) {
    UnicodeVSPrintAsciiFormat (Buffer, sizeof (Buffer), Format, Marker);

    if (IsBufferEarlyLogEnabled) {
      OcBufferEarlyLog (ErrorLevel, Buffer);
    }

    if (ShouldPrintConsole) {
      gST->ConOut->OutputString (gST->ConOut, Buffer);
    }
  }

  VA_END (Marker);
}

/**
  Prints an assert message containing a filename, line number, and description.
  This may be followed by a breakpoint or a dead loop.

  Print a message of the form "ASSERT <FileName>(<LineNumber>): <Description>\n"
  to the debug output device.  If DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED bit of
  PcdDebugProperyMask is set then CpuBreakpoint() is called. Otherwise, if
  DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED bit of PcdDebugProperyMask is set then
  CpuDeadLoop() is called.  If neither of these bits are set, then this function
  returns immediately after the message is printed to the debug output device.
  DebugAssert() must actively prevent recursion.  If DebugAssert() is called while
  processing another DebugAssert(), then DebugAssert() must return immediately.

  If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.
  If Description is NULL, then a <Description> string of "(NULL) Description" is printed.

  @param  FileName     The pointer to the name of the source file that generated
                       the assert condition.
  @param  LineNumber   The line number in the source file that generated the
                       assert condition
  @param  Description  The pointer to the description of the assert condition.

**/
VOID
EFIAPI
DebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  //
  // Generate the ASSERT() message and log it
  //
  DebugPrint (
    DEBUG_ERROR,
    "ASSERT [%a] %a(%d): %a\n",
    gEfiCallerBaseName,
    FileName,
    LineNumber,
    Description
    );

  //
  // Generate a Breakpoint, DeadLoop, or NOP based on PCD settings
  //
  if ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED) != 0) {
    CpuBreakpoint ();
  } else if ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED) != 0) {
    CpuDeadLoop ();
  }
}


/**
  Fills a target buffer with PcdDebugClearMemoryValue, and returns the target buffer.

  This function fills Length bytes of Buffer with the value specified by
  PcdDebugClearMemoryValue, and returns Buffer.

  If Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param   Buffer  The pointer to the target buffer to be filled with PcdDebugClearMemoryValue.
  @param   Length  The number of bytes in Buffer to fill with zeros PcdDebugClearMemoryValue.

  @return  Buffer  The pointer to the target buffer filled with PcdDebugClearMemoryValue.
**/
VOID *
EFIAPI
DebugClearMemory (
  OUT VOID   *Buffer,
  IN  UINTN  Length
  )
{
  //
  // If Buffer is NULL, then ASSERT().
  //
  ASSERT (Buffer != NULL);

  //
  // SetMem() checks for the the ASSERT() condition on Length and returns Buffer
  //
  return SetMem (Buffer, Length, PcdGet8 (PcdDebugClearMemoryValue));
}


/**
  Returns TRUE if ASSERT() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugAssertEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED) != 0);
}


/**
  Returns TRUE if DEBUG() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugPrintEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_PRINT_ENABLED) != 0);
}


/**
  Returns TRUE if DEBUG_CODE() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugCodeEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_CODE_ENABLED) != 0);
}


/**
  Returns TRUE if DEBUG_CLEAR_MEMORY() macro is enabled.

  This function returns TRUE if the DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugClearMemoryEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED) != 0);
}


/**
  Returns TRUE if any one of the bit is set both in ErrorLevel and PcdFixedDebugPrintErrorLevel.

  This function compares the bit mask of ErrorLevel and PcdFixedDebugPrintErrorLevel.

  @retval  TRUE    Current ErrorLevel is supported.
  @retval  FALSE   Current ErrorLevel is not supported.

**/
BOOLEAN
EFIAPI
DebugPrintLevelEnabled (
  IN  CONST UINTN        ErrorLevel
  )
{
  return (BOOLEAN) ((ErrorLevel & PcdGet32 (PcdFixedDebugPrintErrorLevel)) != 0);
}

/**
  Prints via gST->ConOut without any pool allocations.
  Otherwise equivalent to Print.
  Note: EFIAPI must be present for VA_ARGS forwarding (causes bugs with gcc).

  @param[in]  Format  Formatted string.
**/
VOID
EFIAPI
OcPrintScreen (
  IN  CONST CHAR16   *Format,
  ...
  )
{
  CHAR16 Buffer[1024];
  VA_LIST Marker;

  VA_START (Marker, Format);
  UnicodeVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  //
  // It is safe to call gST->ConOut->OutputString, because in crtitical areas
  // AptioMemoryFix is responsible for overriding gBS->AllocatePool with its own
  // implementation that uses a custom allocator.
  //
  if (gST->ConOut) {
    gST->ConOut->OutputString (gST->ConOut, Buffer);
  }
}
