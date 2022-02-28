/** @file
  Copyright (C) 2020, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <IndustryStandard/ApplePerfData.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcMiscLib.h>

#include "OcLogInternal.h"

STATIC
BOOLEAN
mAppleDebugLogEnable;

STATIC
CHAR8
mCurrentBuffer[1024];

STATIC
APPLE_PERF_DATA *
mApplePerfBuffer;

STATIC
UINTN
mApplePerfBufferSize;

STATIC
UINT32
mApplePerfDumped;

STATIC
EFI_STATUS
EFIAPI
AppleDebugLogPrintToOcLog (
  IN OC_LOG_PROTOCOL  *OcLog,
  IN CONST CHAR8      *Format,
  ...
  )
{
  EFI_STATUS       Status;
  VA_LIST          Marker;

  VA_START (Marker, Format);

  Status = OcLog->AddEntry (
    OcLog,
    DEBUG_INFO,
    Format,
    Marker
    );

  VA_END (Marker);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
AppleDebugLogPrint (
  IN CONST CHAR8               *Message
  )
{
  OC_LOG_PROTOCOL   *OcLog;
  EFI_STATUS        Status;
  UINTN             Length;
  CHAR8             *NewLinePos;
  APPLE_PERF_ENTRY  *Entry;
  UINT32            Index;

  if (!mAppleDebugLogEnable) {
    return FALSE;
  }

  OcLog = InternalGetOcLog ();
  if (OcLog == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Flush perf data.
  //
  if (mApplePerfBuffer != NULL
    && mApplePerfBuffer->Signature == APPLE_PERF_DATA_SIGNATURE
    && mApplePerfBuffer->NumberOfEntries > mApplePerfDumped) {
    Entry = APPLE_PERF_FIRST_ENTRY (mApplePerfBuffer);

    for (Index = 0; Index < mApplePerfBuffer->NumberOfEntries; ++Index) {
      if (Index == mApplePerfDumped) {
        AppleDebugLogPrintToOcLog (
          OcLog,
          "EBPF: [%u ms] %a\n",
          (UINT32) Entry->TimestampMs,
          Entry->EntryData
          );
        ++mApplePerfDumped;
      }
      Entry = APPLE_PERF_NEXT_ENTRY (Entry);
    }
  }

  //
  // Concatenate with the previous message.
  //
  Status = AsciiStrCatS (
    mCurrentBuffer,
    sizeof (mCurrentBuffer) - 1,
    Message
    );
  if (EFI_ERROR (Status)) {
    Length = AsciiStrLen (mCurrentBuffer);

    if (Length > 0) {
      //
      // Flush the previous message.
      //
      if (mCurrentBuffer[Length - 1] != '\n') {
        //
        // Ensure it is terminated with a newline.
        //
        mCurrentBuffer[Length] = '\n';
        mCurrentBuffer[Length+1] = '\0';
      }
      AppleDebugLogPrintToOcLog (
        OcLog,
        "AAPL: %a",
        Message
        );
      mCurrentBuffer[0] = '\0';

      //
      // Append the new message again.
      //
      Status = AsciiStrCpyS (
        mCurrentBuffer,
        sizeof (mCurrentBuffer) - 1,
        Message
        );
    }

    //
    // New message does not fit, send it directly.
    //
    if (EFI_ERROR (Status)) {
      return AppleDebugLogPrintToOcLog (
        OcLog,
        "AAPL: %a",
        Message
        );
    }
  }

  //
  // New message is in the buffer, send it line-by-line.
  //
  while (TRUE) {
    NewLinePos = AsciiStrStr (mCurrentBuffer, "\n");
    if (NewLinePos == NULL) {
      break;
    }

    AppleDebugLogPrintToOcLog (
      OcLog,
      "AAPL: %.*a",
      (UINTN) (NewLinePos - mCurrentBuffer + 1),
      mCurrentBuffer
      );

    Length = AsciiStrLen (NewLinePos + 1);

    if (Length > 0) {
      CopyMem (mCurrentBuffer, NewLinePos + 1, Length + 1);
    } else {
      mCurrentBuffer[0] = '\0';
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AppleDebugLogExtractBuffer (
  IN OUT UINT32            *Position,
  IN OUT UINTN             *BufferSize,
     OUT CHAR8             *Buffer          OPTIONAL,
     OUT UINT32            *LostCharacters  OPTIONAL
  )
{
  //
  // Do nothing for now.
  //
  return EFI_END_OF_FILE;
}

STATIC
EFI_STATUS
EFIAPI
AppleDebugLogWriteFiles (
  VOID
  )
{
  //
  // Do nothing for now.
  //
  return EFI_SUCCESS;
}

STATIC
VOID
EFIAPI
AppleDebugLogSetupFiles (
  VOID
  )
{
  //
  // Do nothing for now.
  //
}

STATIC
APPLE_DEBUG_LOG_PROTOCOL
mAppleDebugLogProtocol = {
  .Revision      = APPLE_DEBUG_LOG_PROTOCOL_REVISION,
  .Print         = AppleDebugLogPrint,
  .ExtractBuffer = AppleDebugLogExtractBuffer,
  .WriteFiles    = AppleDebugLogWriteFiles,
  .SetupFiles    = AppleDebugLogSetupFiles
};

APPLE_DEBUG_LOG_PROTOCOL *
OcAppleDebugLogInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS Status;

  APPLE_DEBUG_LOG_PROTOCOL    *Protocol;
  EFI_HANDLE                  Handle;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleDebugLogProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCL: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleDebugLogProtocolGuid,
      NULL,
      (VOID *) &Protocol
      );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &Handle,
    &gAppleDebugLogProtocolGuid,
    (VOID **) &mAppleDebugLogProtocol,
    NULL
    );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleDebugLogProtocol;
}

VOID
OcAppleDebugLogConfigure (
  IN BOOLEAN  Enable
  )
{
  mAppleDebugLogEnable = Enable;
}

VOID
OcAppleDebugLogPerfAllocated (
  IN OUT VOID  *PerfBuffer,
  IN     UINTN  PerfBufferSize
  )
{
  DEBUG ((DEBUG_INFO, "OCL: EFI Boot performance buffer %p (%u)\n", PerfBuffer, (UINT32) PerfBufferSize));
  if (mAppleDebugLogEnable) {
    ZeroMem (PerfBuffer, PerfBufferSize);

    mApplePerfBuffer     = PerfBuffer;
    mApplePerfBufferSize = PerfBufferSize;
    mApplePerfDumped     = 0;
  }
}

