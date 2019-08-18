/** @file
Firmware volume driver that overrides the EFI_FIRMWARE_VOLUME_PROTOCOL
and injects images for boot.efi/FileVault 2.

Copyright (C) 2016 Sergey Slice. All rights reserved.<BR>
Portions copyright (C) 2018 savvas.<BR>
Portions copyright (C) 2006-2014 Intel Corporation. All rights reserved.<BR>
Portions copyright (C) 2016-2018 Alex James. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseMemoryLib.h>
#include <Library/OcFirmwareVolumeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FirmwareVolume2.h>

#include "OcFirmwareVolumeLibInternal.h"

//
// Original functions from FirmwareVolume protocol
//
STATIC FRAMEWORK_EFI_FV_GET_ATTRIBUTES  mGetVolumeAttributes = NULL;
STATIC FRAMEWORK_EFI_FV_SET_ATTRIBUTES  mSetVolumeAttributes = NULL;
STATIC FRAMEWORK_EFI_FV_READ_FILE       mReadFile            = NULL;
STATIC FRAMEWORK_EFI_FV_READ_SECTION    mReadSection         = NULL;
STATIC FRAMEWORK_EFI_FV_WRITE_FILE      mWriteFile           = NULL;
STATIC FRAMEWORK_EFI_FV_GET_NEXT_FILE   mGetNextFile         = NULL;

STATIC
EFI_STATUS
EFIAPI
GetVolumeAttributesEx (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL  *This,
  OUT FRAMEWORK_EFI_FV_ATTRIBUTES   *Attributes
  )
{
  EFI_STATUS  Status;

  if (mGetVolumeAttributes != NULL) {
    Status = mGetVolumeAttributes (This, Attributes);
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // UEFI PI Specification 1.6, page 90
    //
    Status = EFI_SUCCESS;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
SetVolumeAttributesEx (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL  *This,
  IN OUT FRAMEWORK_EFI_FV_ATTRIBUTES   *Attributes
  )
{
  EFI_STATUS  Status;

  if (mSetVolumeAttributes != NULL) {
    Status = mSetVolumeAttributes (This, Attributes);
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // UEFI PI Specification 1.6, page 96
    //
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
ReadFileEx (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL  *This,
  IN     EFI_GUID                      *NameGuid,
  IN OUT VOID                          **Buffer,
  IN OUT UINTN                         *BufferSize,
  OUT    EFI_FV_FILETYPE               *FoundType,
  OUT    EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
  OUT    UINT32                        *AuthenticationStatus
  )
{
  EFI_STATUS  Status;

  if (mReadFile != NULL) {
    Status = mReadFile (
      This,
      NameGuid,
      Buffer,
      BufferSize,
      FoundType,
      FileAttributes,
      AuthenticationStatus
      );
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // UEFI PI Specification 1.6, page 98
    //
    Status = EFI_ACCESS_DENIED;
  }


  return Status;
}

STATIC
EFI_STATUS
EFIAPI
ReadSectionEx (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL  *This,
  IN     EFI_GUID                      *NameGuid,
  IN     EFI_SECTION_TYPE              SectionType,
  IN     UINTN                         SectionInstance,
  IN OUT VOID                          **Buffer,
  IN OUT UINTN                         *BufferSize,
  OUT    UINT32                        *AuthenticationStatus
  )
{
  EFI_STATUS Status;

  if (!Buffer || !BufferSize || !AuthenticationStatus) {
    return EFI_INVALID_PARAMETER;
  }

  if (CompareGuid (NameGuid, &gAppleArrowCursorImageGuid)) {
    *BufferSize = sizeof (mAppleArrowCursorImage);
    Status = gBS->AllocatePool (EfiBootServicesData, *BufferSize, (VOID **)Buffer);
    if (!EFI_ERROR (Status)) {
      gBS->CopyMem (*Buffer, &mAppleArrowCursorImage, *BufferSize);
    }

    *AuthenticationStatus = 0;
    return Status;

  } else if (CompareGuid (NameGuid, &gAppleArrowCursor2xImageGuid)) {
    *BufferSize = sizeof (mAppleArrowCursor2xImage);
    Status = gBS->AllocatePool (EfiBootServicesData, *BufferSize, (VOID **)Buffer);
    if (!EFI_ERROR (Status)) {
      gBS->CopyMem (*Buffer, &mAppleArrowCursor2xImage, *BufferSize);
    }

    *AuthenticationStatus = 0;
    return Status;

  } else if (CompareGuid (NameGuid, &gAppleImageListGuid)) {
    *BufferSize = sizeof (mAppleImageTable);
    Status = gBS->AllocatePool (EfiBootServicesData, *BufferSize, (VOID **)Buffer);
    if (!EFI_ERROR (Status)) {
      gBS->CopyMem (*Buffer, &mAppleImageTable, *BufferSize);
    }

    *AuthenticationStatus = 0;
    return Status;
  }
  if (mReadSection != NULL) {
    Status = mReadSection (
      This,
      NameGuid,
      SectionType,
      SectionInstance,
      Buffer,
      BufferSize,
      AuthenticationStatus
      );
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // UEFI PI Specification 1.6, page 98
    //
    Status = EFI_ACCESS_DENIED;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
WriteFileEx (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL      *This,
  IN UINT32                            NumberOfFiles,
  IN FRAMEWORK_EFI_FV_WRITE_POLICY     WritePolicy,
  IN FRAMEWORK_EFI_FV_WRITE_FILE_DATA  *FileData
  )
{
  EFI_STATUS Status;

  if (mWriteFile != NULL) {
    Status = mWriteFile (This, NumberOfFiles, WritePolicy, FileData);
  } else {
      //
      // The firmware volume is configured to disallow writes.
      // According UEFI PI Specification 1.6, page 101
      //
    Status = EFI_WRITE_PROTECTED;
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
GetNextFileEx (
  IN     EFI_FIRMWARE_VOLUME_PROTOCOL  *This,
  IN OUT VOID                          *Key,
  IN OUT EFI_FV_FILETYPE               *FileType,
  OUT    EFI_GUID                      *NameGuid,
  OUT    EFI_FV_FILE_ATTRIBUTES        *Attributes,
  OUT    UINTN                         *Size
  )
{
  EFI_STATUS Status;

  if (mGetNextFile != NULL) {
    Status = mGetNextFile (This, Key, FileType, NameGuid, Attributes, Size);
  } else {
    //
    // The firmware volume is configured to disallow reads.
    // According UEFI PI Specification 1.6, page 101
    //
    Status = EFI_ACCESS_DENIED;
  }

  return Status;
}

STATIC EFI_FIRMWARE_VOLUME_PROTOCOL mFirmwareVolume = {
  GetVolumeAttributesEx,
  SetVolumeAttributesEx,
  ReadFileEx,
  ReadSectionEx,
  WriteFileEx,
  GetNextFileEx,
  16,
  NULL
};

/**
  Install and initialise EFI Firmware Volume protocol.

  @param[in] Reinstall  Replace any installed protocol.

  @returns Installed protocol.
  @retval NULL  There was an error installing the protocol.
**/
EFI_FIRMWARE_VOLUME_PROTOCOL *
OcFirmwareVolumeInstallProtocol (
  IN BOOLEAN  Reinstall
  )
{
  EFI_STATUS                     Status;
  EFI_FIRMWARE_VOLUME_PROTOCOL   *FirmwareVolumeInterface  = NULL;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FirmwareVolume2Interface = NULL;
  EFI_HANDLE                     NewHandle                 = NULL;

  Status = gBS->LocateProtocol (
                  &gEfiFirmwareVolumeProtocolGuid,
                  NULL,
                  (VOID **) &FirmwareVolumeInterface
                  );

  if (EFI_ERROR (Status)) {
    //
    // This is a rough workaround for MSI 100 and 200 series motherboards.
    // These boards do not have Firmware Volume protocol, yet their
    // dispatcher protocol, responsible for loading BIOS UI elements,
    // prefers Firmware Volume to Firmware Volume 2.
    // For this reason we make sure that all the other Firmware Volume 2
    // volumes are backed with Firmware Volume implementations.
    // We assume that by the time this code is executed, Firmware Volume 2
    // protocols are available if exists. Otherwise do not bother installing
    // hunks over nowhere.
    //

    Status = gBS->LocateProtocol (
                    &gEfiFirmwareVolume2ProtocolGuid,
                    NULL,
                    (VOID **) &FirmwareVolume2Interface
                    );

    if (!EFI_ERROR (Status)) {
      InitializeFirmwareVolume2 (gImageHandle, gST);
    }

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &NewHandle,
                    &gEfiFirmwareVolumeProtocolGuid,
                    &mFirmwareVolume,
                    NULL
                    );

    if (EFI_ERROR (Status)) {
      return NULL;
    }
  } else {
    if (Reinstall) {
      //
      // If a protocol already exists, we just wrap the first discovered
      // protocol to handle our special GUIDs. This works, as boot.efi
      // makes sure to check all the available volumes.
      //

      //
      // Remember pointers to original functions
      //
      mGetVolumeAttributes = FirmwareVolumeInterface->GetVolumeAttributes;
      mSetVolumeAttributes = FirmwareVolumeInterface->SetVolumeAttributes;
      mReadFile            = FirmwareVolumeInterface->ReadFile;
      mReadSection         = FirmwareVolumeInterface->ReadSection;
      mWriteFile           = FirmwareVolumeInterface->WriteFile;
      mGetNextFile         = FirmwareVolumeInterface->GetNextFile;
      //
      // Override with our wrappers
      //
      FirmwareVolumeInterface->GetVolumeAttributes = GetVolumeAttributesEx;
      FirmwareVolumeInterface->SetVolumeAttributes = SetVolumeAttributesEx;
      FirmwareVolumeInterface->ReadFile            = ReadFileEx;
      FirmwareVolumeInterface->ReadSection         = ReadSectionEx;
      FirmwareVolumeInterface->WriteFile           = WriteFileEx;
      FirmwareVolumeInterface->GetNextFile         = GetNextFileEx;
    } else {
      return FirmwareVolumeInterface;
    }
  }

  return &mFirmwareVolume;
}
