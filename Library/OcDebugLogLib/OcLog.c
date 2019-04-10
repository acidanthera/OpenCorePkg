/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Guid/OcLogVariable.h>

#include <Protocol/OcLog.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/OcDataHubLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcTimerLib.h>
#include <Library/SerialPortLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "OcLogInternal.h"

STATIC
CHAR8 *
GetTiming  (
  IN OC_LOG_PROTOCOL  *This
  )
{
  OC_LOG_PRIVATE_DATA *Private = NULL;

  UINT64                dTStartSec = 0;
  UINT64                dTStartMs = 0;
  UINT64                dTLastSec = 0;
  UINT64                dTLastMs = 0;
  UINT64                CurrentTsc = 0;

  if (This == NULL) {
    return NULL;
  }

  Private = OC_LOG_PRIVATE_DATA_FROM_OC_LOG_THIS (This);

  //
  // Calibrate TSC for timings.
  //

  if (Private->TscFrequency == 0)  {
    Private->TscFrequency = GetPerformanceCounterProperties (NULL, NULL);

    if (Private->TscFrequency != 0) {
      CurrentTsc = AsmReadTsc ();

      Private->TscStart = CurrentTsc;
      Private->TscLast  = CurrentTsc;
    }
  }

  if (Private->TscFrequency > 0) {
    CurrentTsc = AsmReadTsc ();

    dTStartMs  = DivU64x64Remainder (MultU64x32 (CurrentTsc - Private->TscStart, 1000), Private->TscFrequency, NULL);
    dTStartSec = DivU64x64Remainder (dTStartMs, 1000, &dTStartMs);
    dTLastMs   = DivU64x64Remainder (MultU64x32 (CurrentTsc - Private->TscLast, 1000), Private->TscFrequency, NULL);
    dTLastSec  = DivU64x64Remainder (dTLastMs, 1000, &dTLastMs);

    Private->TscLast = CurrentTsc;
  }

  AsciiSPrint (
    Private->TimingTxt,
    OC_LOG_TIMING_BUFFER_SIZE,
    "%02d:%03d %02d:%03d ",
    dTStartSec,
    dTStartMs,
    dTLastSec,
    dTLastMs
    );

  return Private->TimingTxt;
}

EFI_STATUS
EFIAPI
OcLogAddEntry  (
  IN OC_LOG_PROTOCOL    *OcLog,
  IN UINTN              ErrorLevel,
  IN CONST CHAR8        *FormatString,
  IN VA_LIST            Marker
  )
{
  EFI_STATUS             Status;

  OC_LOG_PRIVATE_DATA    *Private;
  UINT32                 Attributes;
  UINT32                 TimingLength;
  UINT32                 LineLength;
  PLATFORM_DATA_HEADER   *Entry;
  UINT32                 KeySize;
  UINT32                 DataSize;
  UINT32                 TotalSize;

  Private = OC_LOG_PRIVATE_DATA_FROM_OC_LOG_THIS (OcLog);

  if (OcLog->Options == OC_LOG_DISABLE) {
    //
    // Silently ignore when disabled.
    //
    return EFI_SUCCESS;
  }

  AsciiVSPrint (
    Private->LineBuffer,
    sizeof (Private->LineBuffer),
    FormatString,
    Marker
    );

  //
  // Add Entry.
  //

  Status = EFI_SUCCESS;

  if (*Private->LineBuffer != '\0') {
    GetTiming (OcLog);

    //
    // Send the string to the console output device.
    //
    if ((OcLog->Options & OC_LOG_CONSOLE) != 0 && (OcLog->DisplayLevel & ErrorLevel) != 0) {
      UnicodeSPrint (
        Private->UnicodeLineBuffer,
        sizeof (Private->UnicodeLineBuffer),
        L"%a",
        Private->LineBuffer
        );
      gST->ConOut->OutputString (gST->ConOut, Private->UnicodeLineBuffer);
    }

    TimingLength = (UINT32) AsciiStrLen (Private->TimingTxt);
    LineLength   = (UINT32) AsciiStrLen (Private->LineBuffer);

    //
    // Write to serial port.
    //
    if ((OcLog->Options & OC_LOG_SERIAL) != 0) {
      Status = SerialPortWrite ((UINT8 *) Private->TimingTxt, TimingLength);
      if (Status == EFI_NO_MAPPING) {
        //
        // Disable serial port option.
        //
        OcLog->Options &= ~OC_LOG_SERIAL;
      }
      SerialPortWrite ((UINT8 *) Private->LineBuffer, LineLength);
    }

    //
    // Write to DataHub.
    //
    if ((OcLog->Options & OC_LOG_DATA_HUB) != 0) {
      if (Private->DataHub == NULL) {
        gBS->LocateProtocol (
          &gEfiDataHubProtocolGuid,
          NULL,
          (VOID **) &Private->DataHub
          );
      }

      if (Private->DataHub != NULL) {
        KeySize   = (L_STR_LEN (OC_LOG_VARIABLE_NAME) + 6) * sizeof (CHAR16);
        DataSize  = TimingLength + LineLength + 1;
        TotalSize = KeySize + DataSize + sizeof (*Entry);

        Entry = AllocatePool (TotalSize);

        if (Entry != NULL) {
          ZeroMem (Entry, sizeof (*Entry));
          Entry->KeySize  = KeySize;
          Entry->DataSize = DataSize;

          UnicodeSPrint (
            (CHAR16 *) &Entry->Data[0],
            Entry->KeySize,
            L"%s%05u",
            OC_LOG_VARIABLE_NAME,
            Private->LogCounter++
            );

          CopyMem (
            &Entry->Data[Entry->KeySize],
            Private->TimingTxt,
            TimingLength
            );

          CopyMem (
            &Entry->Data[Entry->KeySize + TimingLength],
            Private->LineBuffer,
            LineLength + 1
            );

          Private->DataHub->LogData (
            Private->DataHub,
            &gEfiMiscSubClassGuid,
            &gApplePlatformProducerNameGuid,
            EFI_DATA_RECORD_CLASS_DATA,
            Entry,
            TotalSize
            );

          FreePool (Entry);
        }
      }
    }

    if (OcLog->Delay > 0) {
      gBS->Stall (OcLog->Delay);
    }

    //
    // Write to internal buffer.
    //

    Status = AsciiStrCatS (Private->AsciiBuffer, Private->AsciiBufferSize, Private->TimingTxt);
    if (!EFI_ERROR (Status)) {
      Status = AsciiStrCatS (Private->AsciiBuffer, Private->AsciiBufferSize, Private->LineBuffer);
    }

    //
    // TODO: Write to a file.
    //
    if ((OcLog->Options & OC_LOG_FILE) != 0) {
      OcLog->Options &= ~OC_LOG_FILE;
    }

    //
    // Write to a variable.
    //
    if (!EFI_ERROR (Status) && (OcLog->Options & (OC_LOG_VARIABLE | OC_LOG_NONVOLATILE)) != 0) {
      Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
      if ((OcLog->Options & OC_LOG_NONVOLATILE) != 0) {
        Attributes |= EFI_VARIABLE_NON_VOLATILE;
      }

      gRT->SetVariable (
        OC_LOG_VARIABLE_NAME,
        &gOcLogVariableGuid,
        Attributes,
        AsciiStrSize (Private->AsciiBuffer),
        Private->AsciiBuffer
        );
    }
  }

  if ((ErrorLevel & OcLog->HaltLevel) != 0) {
    CpuDeadLoop ();
  }

  return Status;
}

/**
  Retrieve pointer to the log buffer

  @param[in] This           This protocol.
  @param[in] OcLogBuffer  Address to store the buffer pointer.

**/
EFI_STATUS
EFIAPI
OcLogGetLog  (
  IN  OC_LOG_PROTOCOL  *This,
  OUT CHAR8            **OcLogBuffer
  )
{
  EFI_STATUS            Status;

  OC_LOG_PRIVATE_DATA *Private;

  Status = EFI_INVALID_PARAMETER;

  if (OcLogBuffer != NULL) {
    Private        = OC_LOG_PRIVATE_DATA_FROM_OC_LOG_THIS (This);
    *OcLogBuffer   = Private->AsciiBuffer;

    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Save the current log

  @param[in] This         This protocol.
  @param[in] NonVolatile  Variable.
  @param[in] FilePath     Filepath to save the log, optional.

  @retval EFI_SUCCESS  The log was saved successfully.
**/
EFI_STATUS
EFIAPI
OcLogSaveLog (
  IN OC_LOG_PROTOCOL           *This,
  IN UINT32                    NonVolatile OPTIONAL,
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath OPTIONAL
  )
{
  return EFI_NOT_FOUND;
}

/**
  Reset the internal timers

  @param[in] This  This protocol.

  @retval EFI_SUCCESS  The timers were reset successfully.
**/
EFI_STATUS
EFIAPI
OcLogResetTimers (
  IN OC_LOG_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

/**
  Install or update the OcLog protocol with specified options.

  @param[in] Options       Logging options.
  @param[in] Delay         Delay in microseconds after each log entry.
  @param[in] DisplayLevel  Console visible error level.
  @param[in] HaltLevel     Error level causing CPU halt.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
OcConfigureLogProtocol (
  IN OC_LOG_OPTIONS      Options,
  IN UINT32              Delay,
  IN UINTN               DisplayLevel,
  IN UINTN               HaltLevel
  )
{
  EFI_STATUS            Status;

  OC_LOG_PROTOCOL       *OcLog;
  OC_LOG_PRIVATE_DATA   *Private;
  EFI_HANDLE            Handle;


  //
  // Check if protocol already exists.
  //

  OcLog = NULL;
  Status  = gBS->LocateProtocol (
                   &gOcLogProtocolGuid,
                   NULL,
                   (VOID **)&OcLog
                   );

  if (!EFI_ERROR (Status)) {
    //
    // Set desired options in existing protocol.
    //
    OcLog->Options      = Options;
    OcLog->Delay        = Delay;
    OcLog->DisplayLevel = DisplayLevel;
    OcLog->HaltLevel    = HaltLevel;

    //
    // Keep EFI_SUCCESS...
    //
  } else {
    Private = AllocateZeroPool (sizeof (*Private));
    Status  = EFI_OUT_OF_RESOURCES;

    if (Private != NULL) {
      Private->Signature = OC_LOG_PRIVATE_DATA_SIGNATURE;

      //
      // TODO: Dynamically resizeable buffer.
      //

      Private->AsciiBufferSize    = OC_LOG_BUFFER_SIZE;
      Private->OcLog.Revision     = OC_LOG_REVISION;
      Private->OcLog.AddEntry     = OcLogAddEntry;
      Private->OcLog.GetLog       = OcLogGetLog;
      Private->OcLog.SaveLog      = OcLogSaveLog;
      Private->OcLog.ResetTimers  = OcLogResetTimers;
      Private->OcLog.Options      = Options;
      Private->OcLog.Delay        = Delay;
      Private->OcLog.DisplayLevel = DisplayLevel;
      Private->OcLog.HaltLevel    = HaltLevel;

      Handle = NULL;
      Status = gBS->InstallProtocolInterface (
        &Handle,
        &gOcLogProtocolGuid,
        EFI_NATIVE_INTERFACE,
        &Private->OcLog
        );

      if (EFI_ERROR (Status)) {
        FreePool (Private);
      }
    }
  }

  return Status;
}
