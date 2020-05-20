/** @file

  OcRtcLib - library with RTC I/O functions

  Copyright (c) 2020, vit9696

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Guid/OcVariable.h>
#include <Protocol/AppleRtcRam.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcRtcLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include "OcRtcLibInternal.h"

STATIC EFI_LOCK mAppleRtcRamLock;
STATIC UINT8 mEmulatedRtcArea[APPLE_RTC_TOTAL_SIZE];
STATIC BOOLEAN mEmulatedRtcStatus[APPLE_RTC_TOTAL_SIZE];


STATIC
EFI_STATUS
SyncRtcRead (
  IN UINT8  Address,
  IN UINT8  *ValuePtr
  )
{
  EFI_STATUS Status;

  if (mEmulatedRtcStatus[Address]) {
    return mEmulatedRtcArea[Address];
  }

  Status = EfiAcquireLockOrFail (&mAppleRtcRamLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *ValuePtr = OcRtcRead (Address);
  EfiReleaseLock (&mAppleRtcRamLock);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SyncRtcWrite (
  IN UINT8  Address,
  IN UINT8  Value
  )
{
  EFI_STATUS Status;

  if (mEmulatedRtcStatus[Address]) {
    mEmulatedRtcArea[Address] = Value;
    return EFI_SUCCESS;
  }

  Status = EfiAcquireLockOrFail (&mAppleRtcRamLock);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OcRtcWrite (Address, Value);
  EfiReleaseLock (&mAppleRtcRamLock);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SyncRtcWaitForReady (
  VOID
  )
{
  EFI_STATUS Status;
  UINTN      Count;
  UINT8      RegisterA;

  for (Count = 0; Count < 100; ++Count) {
    Status = EfiAcquireLockOrFail (&mAppleRtcRamLock);

    if (!EFI_ERROR (Status)) {
      RegisterA = OcRtcRead (RTC_ADDRESS_REGISTER_A);
      EfiReleaseLock (&mAppleRtcRamLock);

      if ((RegisterA & RTC_UPDATE_IN_PROGRESS) == 0) {
        return EFI_SUCCESS;
      }
    }

    //
    // Wait 1 ms and retry.
    //
    gBS->Stall (1000);
  }

  return EFI_TIMEOUT;
}

STATIC
UINTN
EFIAPI
AppleRtcGetAvailableMemory (
  IN APPLE_RTC_RAM_PROTOCOL  *This
  )
{
  return APPLE_RTC_TOTAL_SIZE;
}

STATIC
EFI_STATUS
EFIAPI
AppleRtcRamReadData (
  IN  APPLE_RTC_RAM_PROTOCOL  *This,
  OUT UINT8                   *Buffer,
  IN  UINTN                   BufferSize,
  IN  UINTN                   Address
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT8       Temp;

  if (Buffer == NULL
    || BufferSize == 0
    || Address >= APPLE_RTC_TOTAL_SIZE
    || BufferSize >= APPLE_RTC_TOTAL_SIZE
    || Address + BufferSize > APPLE_RTC_TOTAL_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < BufferSize; ++Index) {
    Status = SyncRtcWaitForReady ();
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = SyncRtcRead ((UINT8) Address, Buffer);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (Address == APPLE_RTC_BG_COLOR_ADDR) {
      Status = SyncRtcWaitForReady ();
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = SyncRtcRead (APPLE_RTC_BG_COMPLEMENT_ADDR, &Temp);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if ((Temp ^ *Buffer) != 0xFF) {
        *Buffer = 0;
      }
    }

    ++Buffer;
    ++Address;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AppleRtcRamWriteData (
  IN  APPLE_RTC_RAM_PROTOCOL  *This,
  IN  CONST UINT8             *Buffer,
  IN  UINTN                   BufferSize,
  IN  UINTN                   Address
  )
{
  EFI_STATUS  Status;
  UINT8       TempBuffer[APPLE_RTC_TOTAL_SIZE];
  UINTN       Index;
  UINT16      Checksum;

  if (Buffer == NULL
    || BufferSize == 0
    || Address < APPLE_RTC_CHECKSUM_START
    || Address >= APPLE_RTC_TOTAL_SIZE
    || BufferSize >= APPLE_RTC_TOTAL_SIZE
    || Address + BufferSize > APPLE_RTC_TOTAL_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  Status = AppleRtcRamReadData (This, TempBuffer, APPLE_RTC_TOTAL_SIZE, 0);
  if (EFI_ERROR (Status)) {
    return Status; ///< This is not checked in the original.
  }

  for (Index = 0; Index < BufferSize; ++Index) {
    if (Address != APPLE_RTC_BG_COMPLEMENT_ADDR) {
      TempBuffer[Address] = *Buffer;

      if (Address == APPLE_RTC_BG_COLOR_ADDR) {
        TempBuffer[APPLE_RTC_BG_COMPLEMENT_ADDR] = (UINT8) ~((UINT32) *Buffer);
      }
    }

    ++Buffer;
    ++Address;
  }

  Checksum = OcRtcChecksumApple (TempBuffer, APPLE_RTC_CORE_CHECKSUM_ADDR1);
  TempBuffer[APPLE_RTC_CORE_CHECKSUM_ADDR1] = APPLE_RTC_CORE_CHECKSUM_BYTE1 (Checksum);
  TempBuffer[APPLE_RTC_CORE_CHECKSUM_ADDR2] = APPLE_RTC_CORE_CHECKSUM_BYTE2 (Checksum);

  TempBuffer[APPLE_RTC_MAIN_CHECKSUM_ADDR1] = 0;
  TempBuffer[APPLE_RTC_MAIN_CHECKSUM_ADDR2] = 0;
  Checksum = OcRtcChecksumApple (TempBuffer, APPLE_RTC_TOTAL_SIZE);
  TempBuffer[APPLE_RTC_MAIN_CHECKSUM_ADDR1] = APPLE_RTC_MAIN_CHECKSUM_BYTE1 (Checksum);
  TempBuffer[APPLE_RTC_MAIN_CHECKSUM_ADDR2] = APPLE_RTC_MAIN_CHECKSUM_BYTE2 (Checksum);

  for (Index = APPLE_RTC_CHECKSUM_START; Index < APPLE_RTC_TOTAL_SIZE; ++Index) {
    SyncRtcWrite ((UINT8) Index, TempBuffer[Index]); ///< Does not check the error code.
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
AppleRtcRamReset (
  IN  APPLE_RTC_RAM_PROTOCOL  *This
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT8       Buffer[APPLE_RTC_TOTAL_SIZE];

  ZeroMem (Buffer, sizeof (Buffer));

  for (Index = APPLE_RTC_CHECKSUM_START; Index < APPLE_RTC_CORE_SIZE; ++Index) {
    Status = SyncRtcWaitForReady ();
    if (!EFI_ERROR (Status)) {
      SyncRtcRead ((UINT8) Index, &Buffer[Index]);
    }
  }

  Status = SyncRtcWaitForReady ();
  if (!EFI_ERROR (Status)) {
    SyncRtcRead ((UINT8) APPLE_RTC_FIRMWARE_57_ADDR, &Buffer[APPLE_RTC_FIRMWARE_57_ADDR]);
  }

  for (Index = APPLE_RTC_RESERVED_ADDR; Index < APPLE_RTC_RESERVED_ADDR + APPLE_RTC_RESERVED_LENGTH; ++Index) {
    Status = SyncRtcWaitForReady ();
    if (!EFI_ERROR (Status)) {
      SyncRtcRead ((UINT8) Index, &Buffer[Index]);
    }
  }

  return AppleRtcRamWriteData (
    This,
    Buffer + APPLE_RTC_CHECKSUM_START,
    APPLE_RTC_TOTAL_SIZE - APPLE_RTC_CHECKSUM_START,
    APPLE_RTC_CHECKSUM_START
    );
}

STATIC
APPLE_RTC_RAM_PROTOCOL
mAppleRtcRamProtocol = {
  AppleRtcGetAvailableMemory,
  AppleRtcRamReadData,
  AppleRtcRamWriteData,
  AppleRtcRamReset,
};

APPLE_RTC_RAM_PROTOCOL *
OcAppleRtcRamInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS              Status;
  APPLE_RTC_RAM_PROTOCOL  *Protocol;
  UINT8                   *RtcBlacklist;
  UINTN                   Index;
  UINTN                   RtcBlacklistSize;

  DEBUG ((DEBUG_VERBOSE, "OCRTC: OcAppleRtcRamInstallProtocol\n"));

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleRtcRamProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCRTC: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gAppleRtcRamProtocolGuid,
      NULL,
      (VOID *) &Protocol
      );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "OCRTC: Wake log is 0x%02X 0x%02X % 3d 0x%02X\n",
    OcRtcRead (APPLE_RTC_TRACE_DATA_ADDR),
    OcRtcRead (APPLE_RTC_WL_MASK_ADDR),
    OcRtcRead (APPLE_RTC_WL_EVENT_ADDR),
    OcRtcRead (APPLE_RTC_WL_EVENT_EXTRA_ADDR)
    ));

  Status = GetVariable2 (
    OC_RTC_BLACKLIST_VARIABLE_NAME,
    &gOcVendorVariableGuid,
    (VOID **) &RtcBlacklist,
    &RtcBlacklistSize
    );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < RtcBlacklistSize; ++Index) {
      mEmulatedRtcStatus[RtcBlacklist[Index]] = TRUE;
      DEBUG ((DEBUG_INFO, "OCRTC: Blacklisted %02x address\n", RtcBlacklist[Index]));
    }

    FreePool (RtcBlacklist);
  }

  //
  // Note, for debugging on QEMU this will need to changed to TPL_CALLBACK.
  // By default we follow AppleRtcRam implementation.
  // Also, AppleRtcRam function may be called after ExitBootServices or around
  // GetMemoryMap, so debugging is not perfectly safe.
  //
  EfiInitializeLock (&mAppleRtcRamLock, TPL_NOTIFY);

  //
  // Note, Apple implementation calls AppleRtcRamReset on checksum mismatch.
  //

  Status = gBS->InstallMultipleProtocolInterfaces (
     &gImageHandle,
     &gAppleRtcRamProtocolGuid,
     (VOID *) &mAppleRtcRamProtocol,
     NULL
     );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mAppleRtcRamProtocol;
}
