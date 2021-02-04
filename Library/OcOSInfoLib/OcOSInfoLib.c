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

#include <Guid/AppleOSLoaded.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcOSInfoLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC UINTN mBootVTdEnabled;
STATIC CHAR8 *mOSName;
STATIC CHAR8 *mOSVendor;
STATIC BOOLEAN mAppleOSLoadedSignaled;

STATIC
VOID
InternalOSInfoSet (
  VOID
  )
{
  UINTN    Index;
  UINTN    Length;
  CHAR8    *VersionPtr;
  UINT32   MajorVersion;
  UINT32   MinorVersion;

  DEBUG ((
    DEBUG_INFO,
    "OCOS: OS set: %a %a\n",
    mOSVendor != NULL ? mOSVendor : "<null>",
    mOSName != NULL ? mOSName : "<null>"
    ));

  if (mOSVendor == NULL) {
    return;
  }

  if (AsciiStrCmp (mOSVendor, EFI_OS_INFO_APPLE_VENDOR_NAME) == 0) {
    if (!mAppleOSLoadedSignaled) {
      EfiNamedEventSignal (&gAppleOSLoadedNamedEventGuid);
      mAppleOSLoadedSignaled = TRUE;
    }

    if (mOSName == NULL) {
      return;
    }

    Length = AsciiStrLen (mOSName);
    for (Index = 0; Index < Length; ++Index) {
      if (mOSName[Index] == '.') {
        break;
      }
    }

    if (Index == Length || Index == 0 || mOSName[Index + 1] == '\0') {
      return;
    }

    VersionPtr = &mOSName[Index - 1];
    while (VersionPtr > mOSName) {
      if (*VersionPtr < '0' || *VersionPtr > '9') {
        ++VersionPtr;
        break;
      }
      --VersionPtr;
    }

    if (&mOSName[Index] == VersionPtr) {
      return;
    }

    MajorVersion = 0;
    while (VersionPtr < &mOSName[Index]) {
      MajorVersion = MajorVersion * 10 + (*VersionPtr - '0');
      ++VersionPtr;
    }

    VersionPtr   = &mOSName[Index + 1];
    MinorVersion = 0;
    while (*VersionPtr != '\0') {
      if (*VersionPtr < '0' || *VersionPtr > '9') {
        break;
      }
      MinorVersion = MinorVersion * 10 + (*VersionPtr - '0');
      ++VersionPtr;
    }

    if (&mOSName[Index + 1] == VersionPtr) {
      return;
    }

    if (((MajorVersion << 16U) | MinorVersion) > ((10U << 16U) | 9U)) {
      //
      // if (BootCurrent == 0x80) RTC[0x30] |= 0x1U;
      // REF: E121EC07-9C42-45EE-B0B6-FFF8EF03C521, gAppleRtcRamProtocolGuid.
      //
      DEBUG ((DEBUG_VERBOSE, "OCOS: Should use black background\n"));
    } else {
      //
      // if (BootCurrent == 0x80) RTC[0x30] &= ~0x1U;
      //
      DEBUG ((DEBUG_VERBOSE, "OCOS: Should use grey background\n"));
    }
  }
}

STATIC
VOID
EFIAPI
SetName (
  IN CHAR8  *OSName
  )
{
  UINTN  Size;
  CHAR8  *Buffer;

  if (mOSName != NULL) {
    FreePool (mOSName);
    mOSName = NULL;
  }

  Size   = AsciiStrSize (OSName);
  Buffer = AllocateCopyPool (Size, OSName);

  mOSName = Buffer;

  InternalOSInfoSet ();
}

VOID
EFIAPI
SetVendor (
  IN CHAR8  *OSVendor
  )
{
  UINTN  Size;
  CHAR8  *Buffer;

  if (mOSVendor != NULL) {
    FreePool (mOSVendor);
    mOSVendor = NULL;
  }

  Size   = AsciiStrSize (OSVendor);
  Buffer = AllocateCopyPool (Size, OSVendor);

  mOSVendor = Buffer;

  InternalOSInfoSet ();
}

STATIC
VOID
EFIAPI
SetBootVTdEnabled (
  IN UINTN  *BootVTdEnabled
  )
{
  mBootVTdEnabled = *BootVTdEnabled;
}

STATIC
VOID
EFIAPI
GetBootVTdEnabled (
  OUT UINTN  *BootVTdEnabled
  )
{
  *BootVTdEnabled = mBootVTdEnabled;
}

STATIC
EFI_OS_INFO_PROTOCOL
mOSInfoProtocol = {
  EFI_OS_INFO_PROTOCOL_REVISION,
  SetName,
  SetVendor,
  SetBootVTdEnabled,
  GetBootVTdEnabled
};

EFI_OS_INFO_PROTOCOL *
OcOSInfoInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS           Status;
  EFI_OS_INFO_PROTOCOL *Protocol;
  EFI_HANDLE           NewHandle;

  DEBUG ((DEBUG_VERBOSE, "OcOSInfoInstallProtocol\n"));

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gEfiOSInfoProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCOS: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
      &gEfiOSInfoProtocolGuid,
      NULL,
      (VOID *) &Protocol
      );

    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
     &NewHandle,
     &gEfiOSInfoProtocolGuid,
     (VOID *) &mOSInfoProtocol,
     NULL
     );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return &mOSInfoProtocol;
}
