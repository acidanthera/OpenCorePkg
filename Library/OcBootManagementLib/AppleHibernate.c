/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Guid/AppleVariable.h>

#include <IndustryStandard/AppleHibernate.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcRtcLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

EFI_STATUS
OcActivateHibernateWake (
  IN UINT32                       HibernateMask
  )
{
  EFI_STATUS               Status;
  UINTN                    Size;
  UINT32                   Attributes;
  VOID                     *Value;
  AppleRTCHibernateVars    RtcVars;
  BOOLEAN                  HasHibernateInfo;
  BOOLEAN                  HasHibernateInfoInRTC;
  UINT8                    Index;
  UINT8                    *RtcRawVars;
  EFI_DEVICE_PATH_PROTOCOL *BootImagePath;
  EFI_DEVICE_PATH_PROTOCOL *RemainingPath;
  INTN                     NumPatchedNodes;

  if (HibernateMask == HIBERNATE_MODE_NONE) {
    return EFI_INVALID_PARAMETER;
  }

  HasHibernateInfo = FALSE;
  HasHibernateInfoInRTC = FALSE;

  //
  // If legacy boot-switch-vars exists (NVRAM working), then use it.
  //
  Status = GetVariable2 (L"boot-switch-vars", &gAppleBootVariableGuid, &Value, &Size);
  if (!EFI_ERROR (Status)) {
    //
    // Leave it as is.
    //
    SecureZeroMem (Value, Size);
    FreePool (Value);
    DEBUG ((DEBUG_INFO, "OCB: Found legacy boot-switch-vars\n"));
    return EFI_SUCCESS;
  }

  Status = GetVariable3 (
             L"boot-image",
             &gAppleBootVariableGuid,
             (VOID **)&BootImagePath,
             &Size,
             &Attributes
             );
  if (!EFI_ERROR (Status)) {
    if (IsDevicePathValid (BootImagePath, Size)) {
      DebugPrintDevicePath (
        DEBUG_INFO,
        "OCB: boot-image pre-fix",
        BootImagePath
        );

      //
      // WARN: BootImagePath must be allocated from pool as it may be reallocated.
      //
      NumPatchedNodes = OcFixAppleBootDevicePath (
        &BootImagePath,
        &RemainingPath
        );
      if (NumPatchedNodes > 0) {
        DebugPrintDevicePath (
          DEBUG_INFO,
          "OCB: boot-image post-fix",
          BootImagePath
          );

        //
        // Updated DevicePath may have different size.
        //
        Size = GetDevicePathSize (BootImagePath);

        Status = gRT->SetVariable (
                        L"boot-image",
                        &gAppleBootVariableGuid,
                        Attributes,
                        Size,
                        BootImagePath
                        );
      }
      if (NumPatchedNodes >= 0) {
        DebugPrintDevicePath (
          DEBUG_INFO,
          "OCB: boot-image post-fix remainder",
          RemainingPath
          );
      }
    } else {
      DEBUG ((DEBUG_INFO, "OCB: Invalid boot-image variable\n"));
    }

    SecureZeroMem (BootImagePath, Size);
    FreePool (BootImagePath);
  }

  DEBUG ((DEBUG_INFO, "OCB: boot-image is %u bytes - %r\n", (UINT32) Size, Status));

  RtcRawVars = (UINT8 *) &RtcVars;

  //
  // Work with RTC memory if allowed.
  //
  if (HibernateMask & HIBERNATE_MODE_RTC) {
    for (Index = 0; Index < sizeof (AppleRTCHibernateVars); Index++) {
      RtcRawVars[Index] = OcRtcRead (Index + 128);
    }

    HasHibernateInfoInRTC = RtcVars.signature[0] == 'A'
                         && RtcVars.signature[1] == 'A'
                         && RtcVars.signature[2] == 'P'
                         && RtcVars.signature[3] == 'L';
    HasHibernateInfo = HasHibernateInfoInRTC;

    DEBUG ((DEBUG_INFO, "OCB: RTC hibernation is %d\n", HasHibernateInfoInRTC));
  }

  if (HibernateMask & HIBERNATE_MODE_NVRAM) {
    //
    // If RTC variables is still written to NVRAM (and RTC is broken).
    // Prior to 10.13.6.
    //
    Status = GetVariable2 (L"IOHibernateRTCVariables", &gAppleBootVariableGuid, &Value, &Size);
    if (!HasHibernateInfo && !EFI_ERROR (Status) && Size == sizeof (RtcVars)) {
      CopyMem (RtcRawVars, Value, sizeof (RtcVars));
      HasHibernateInfo = RtcVars.signature[0] == 'A'
                      && RtcVars.signature[1] == 'A'
                      && RtcVars.signature[2] == 'P'
                      && RtcVars.signature[3] == 'L';
    }

    DEBUG ((
      DEBUG_INFO,
      "OCB: NVRAM hibernation is %d / %r / %u\n",
      HasHibernateInfo,
      Status,
      (UINT32) Size
      ));

    //
    // Erase RTC variables in NVRAM.
    //
    if (!EFI_ERROR (Status)) {
      Status = gRT->SetVariable (
        L"IOHibernateRTCVariables",
        &gAppleBootVariableGuid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        0,
        NULL
        );
      SecureZeroMem (Value, Size);
      FreePool (Value);
    }
  }

  //
  // Convert RTC data to boot-key and boot-signature
  //
  if (HasHibernateInfo) {
    gRT->SetVariable (
      L"boot-image-key",
      &gAppleBootVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof (RtcVars.wiredCryptKey),
      RtcVars.wiredCryptKey
      );
    gRT->SetVariable (
      L"boot-signature",
      &gAppleBootVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof (RtcVars.booterSignature),
      RtcVars.booterSignature
      );
  }

  //
  // Erase RTC memory similarly to AppleBds.
  //
  if (HasHibernateInfoInRTC) {
    SecureZeroMem (RtcRawVars, sizeof(AppleRTCHibernateVars));
    RtcVars.signature[0] = 'D';
    RtcVars.signature[1] = 'E';
    RtcVars.signature[2] = 'A';
    RtcVars.signature[3] = 'D';

    for (Index = 0; Index < sizeof(AppleRTCHibernateVars); Index++) {
      OcRtcWrite (Index + 128, RtcRawVars[Index]);
    }
  }

  //
  // We have everything we need now.
  //
  if (HasHibernateInfo) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

BOOLEAN
OcIsAppleHibernateWake (
  VOID
  )
{
  EFI_STATUS   Status;
  UINTN        ValueSize;

  //
  // This is reverse engineered from boot.efi.
  // To cancel hibernate wake it is enough to delete the variables.
  // Starting with 10.13.6 boot-switch-vars is no longer supported.
  //
  ValueSize = 0;
  Status = gRT->GetVariable (
    L"boot-signature",
    &gAppleBootVariableGuid,
    NULL,
    &ValueSize,
    NULL
    );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    ValueSize = 0;
    Status = gRT->GetVariable (
      L"boot-image-key",
      &gAppleBootVariableGuid,
      NULL,
      &ValueSize,
      NULL
      );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      return TRUE;
    }
  } else {
    ValueSize = 0;
    Status = gRT->GetVariable (
      L"boot-switch-vars",
      &gAppleBootVariableGuid,
      NULL,
      &ValueSize,
      NULL
      );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      return TRUE;
    }
  }

  return FALSE;
}
