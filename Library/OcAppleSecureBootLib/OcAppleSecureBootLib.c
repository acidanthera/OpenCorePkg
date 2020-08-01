/** @file
  OC Apple Secure Boot library.

Copyright (C) 2019, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Guid/AppleVariable.h>

#include <Protocol/AppleSecureBoot.h>
#include <Protocol/AppleImg4Verification.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePathLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC BOOLEAN mSbAvailable = TRUE;

STATIC UINT8   mSbPolicy             = AppleImg4SbModeMedium;
STATIC UINT8   mSbWindowsPolicy      = 1;
STATIC BOOLEAN mSbWindowsPolicyValid = TRUE;
STATIC CHAR8   mSbHardwareModel[16];

STATIC
UINT8
InternalImg4GetFailureReason (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN UINT8                       SbPolicy,
  IN EFI_STATUS                  Status
  )
{
  UINT8 Reason;

  ASSERT (This != NULL);

  switch (Status) {
    case EFI_SUCCESS:
    case EFI_UNSUPPORTED:
    {
      Reason = 0x00;
      break;
    }

    case EFI_NOT_FOUND:
    {
      Reason = (((SbPolicy == AppleImg4SbModeFull) ? 1U : 0U) << 4U) | 1U;
      break;
    }

    case EFI_SECURITY_VIOLATION:
    {
      Reason = (((SbPolicy == AppleImg4SbModeFull) ? 1U : 0U) << 4U) | 2U;
      break;
    }

    default:
    {
      Reason = 0xFF;
      break;
    }
  }

  return Reason;
}

EFI_STATUS
OcAppleSecureBootBootstrapValues (
  IN CONST CHAR8  *Model
  )
{
  EFI_STATUS  Status;

  ASSERT (Model != NULL);

  Status = OcAsciiSafeSPrint (
    mSbHardwareModel,
    sizeof (mSbHardwareModel),
    "%aap",
    Model
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
    L"HardwareModel",
    &gAppleSecureBootVariableGuid,
    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
    AsciiStrSize (mSbHardwareModel),
    mSbHardwareModel
    );

  return Status;
}

STATIC
BOOLEAN
InternalGetApEcid (
  OUT UINT64  *Ecid
  )
{
  //
  // FIXME: Retrieve this value from trusted storage and expose the variable.
  //
  EFI_STATUS Status;
  UINTN      DataSize;
  UINT64     ApEcid;

  ASSERT (Ecid != NULL);

  DataSize = sizeof (ApEcid);
  Status = gRT->GetVariable (
                  L"ApECID",
                  &gAppleSecureBootVariableGuid,
                  NULL,
                  &DataSize,
                  &ApEcid
                  );
  if (EFI_ERROR (Status) || DataSize != sizeof (ApEcid)) {
    return FALSE;
  }

  *Ecid = ApEcid;
  return TRUE;
}

/**
  Sets the Secure Boot availability state.

  @param[in] This       A pointer to the current protocol instance.
  @param[in] Available  The new availability status for Secure Boot.

**/
STATIC
VOID
EFIAPI
AppleSbSetAvailability (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN BOOLEAN                     Available
  )
{
  mSbAvailable = Available;
}

/**
  Retrieves the current Secure Boot Windows policy.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Policy  On output, the current Secure Boot Windows policy.

  @retval EFI_SUCCESS            The current policy has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.
  @retval EFI_NOT_FOUND          The current policy could not be retrieved.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbGetWindowsPolicy (
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Policy
  )
{
  if (Policy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mSbAvailable) {
    *Policy = 0;
    return EFI_SUCCESS;
  }

  if (!mSbWindowsPolicyValid) {
    return EFI_NOT_FOUND;
  }

  *Policy = mSbWindowsPolicy;
  return EFI_SUCCESS;
}

/**
  Retrieves the current Secure Boot Windows failure reason.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Reason  On output, the current Windows failure reason.

  @retval EFI_SUCCESS            The current failure reason has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbGetWindowsFailureReason (
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Reason
  )
{
  UINTN DataSize;
  UINT8 FailReason;

  if (Reason == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FailReason = 0;

  DataSize = sizeof (FailReason);
  gRT->GetVariable (
         L"AppleSecureBootWindowsFailureReason",
         &gAppleSecureBootVariableGuid,
         NULL,
         &DataSize,
         &FailReason
         );

  *Reason = FailReason;
  return EFI_SUCCESS;
}

/**
  Sets the Secure Boot Windows failure reason.

  @param[in] This    A pointer to the current protocol instance.
  @param[in] Reason  The Windows failure reason to set.

  @retval EFI_SUCCESS      The failure reason has been set successfully.
  @retval EFI_UNSUPPORTED  Secure Boot is currently unavailable.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbSetWindowsFailureReason (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN UINT8                       Reason
  )
{
  if (!mSbAvailable) {
    return EFI_UNSUPPORTED;
  }

  return gRT->SetVariable (
                L"AppleSecureBootWindowsFailureReason",
                &gAppleSecureBootVariableGuid,
                EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                sizeof (Reason),
                &Reason
                );
}

STATIC
VOID *
InternalReadFile (
  IN  EFI_FILE_PROTOCOL  *Volume,
  IN  CHAR16             *FilePath,
  OUT UINT32             *FileSize
  )
{
  EFI_STATUS        Status;

  EFI_FILE_PROTOCOL *FileHandle;
  UINT8             *FileBuffer;
  UINT32            FileReadSize;

  ASSERT (Volume != NULL);
  ASSERT (FilePath != NULL);
  ASSERT (FileSize != NULL);

  Status = SafeFileOpen (Volume, &FileHandle, FilePath, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = GetFileSize (FileHandle, &FileReadSize);
  if (EFI_ERROR (Status)) {
    FileHandle->Close (FileHandle);
    return NULL;
  }

  FileBuffer = AllocatePool (FileReadSize);
  if (FileBuffer == NULL) {
    FileHandle->Close (FileHandle);
    return NULL;
  }

  Status = GetFileData (
             FileHandle,
             0,
             FileReadSize,
             FileBuffer
             );

  FileHandle->Close (FileHandle);

  if (EFI_ERROR (Status)) {
    FreePool (FileBuffer);
    return NULL;
  }

  *FileSize = FileReadSize;
  return FileBuffer;
}

/**
  Retrieves the current Secure Boot failure reason.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Reason  On output, the current failure reason.

  @retval EFI_SUCCESS            The current failure reason has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbGetFailureReason (
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Reason
  )
{
  UINTN DataSize;
  UINT8 FailReason;

  if (Reason == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FailReason = 0;

  DataSize = sizeof (FailReason);
  gRT->GetVariable (
         L"AppleSecureBootFailureReason",
         &gAppleSecureBootVariableGuid,
         NULL,
         &DataSize,
         &FailReason
         );

  *Reason = FailReason;
  return EFI_SUCCESS;
}

/**
  Sets the Secure Boot failure reason.

  @param[in] This    A pointer to the current protocol instance.
  @param[in] Reason  The failure reason to set.

  @retval EFI_SUCCESS      The failure reason has been set successfully.
  @retval EFI_UNSUPPORTED  Secure Boot is currently unavailable.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbSetFailureReason (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN UINT8                       Reason
  )
{
  if (!mSbAvailable) {
    return EFI_UNSUPPORTED;
  }

  return gRT->SetVariable (
                L"AppleSecureBootFailureReason",
                &gAppleSecureBootVariableGuid,
                EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                sizeof (Reason),
                &Reason
                );
}

/**
  Retrieves the current Secure Boot Kernel failure reason.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Reason  On output, the current Kernel failure reason.

  @retval EFI_SUCCESS            The current failure reason has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbGetKernelFailureReason (
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Reason
  )
{
  UINTN DataSize;
  UINT8 FailReason;

  if (Reason == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FailReason = 0;

  DataSize = sizeof (FailReason);
  gRT->GetVariable (
         L"AppleSecureBootKernelFailureReason",
         &gAppleSecureBootVariableGuid,
         NULL,
         &DataSize,
         &FailReason
         );

  *Reason = FailReason;
  return EFI_SUCCESS;
}

/**
  Sets the Secure Boot Kernel failure reason.

  @param[in] This    A pointer to the current protocol instance.
  @param[in] Reason  The Kernel failure reason to set.

  @retval EFI_SUCCESS      The failure reason has been set successfully.
  @retval EFI_UNSUPPORTED  Secure Boot is currently unavailable.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbSetKernelFailureReason (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN UINT8                       Reason
  )
{
  if (!mSbAvailable) {
    return EFI_UNSUPPORTED;
  }

  return gRT->SetVariable (
                L"AppleSecureBootKernelFailureReason",
                &gAppleSecureBootVariableGuid,
                EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                sizeof (Reason),
                &Reason
                );
}

/**
  Retrieves the current Secure Boot policy.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Policy  On output, the current Secure Boot policy.

  @retval EFI_SUCCESS            The current policy has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.
  @retval EFI_NOT_FOUND          The current policy could not be retrieved.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbGetPolicy (
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Policy
  )
{
  if (Policy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mSbAvailable) {
    *Policy = 0;
    return EFI_SUCCESS;
  }

  *Policy = mSbPolicy;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalVerifyImg4Worker (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN CONST VOID                  *ImageBuffer,
  IN UINTN                       ImageSize,
  IN CONST VOID                  *ManifestBuffer,
  IN UINTN                       ManifestSize,
  IN UINT32                      ObjType,
  IN BOOLEAN                     SetFailureReason,
  IN UINT8                       SbPolicy
  )
{
  STATIC APPLE_IMG4_VERIFICATION_PROTOCOL *Img4Verify = NULL;

  EFI_STATUS Status;

  ASSERT (ImageBuffer != NULL);
  ASSERT (ImageSize != 0);
  ASSERT (ManifestBuffer != NULL);
  ASSERT (ManifestSize > 0);

  if (Img4Verify == NULL) {
    Status = gBS->LocateProtocol (
                    &gAppleImg4VerificationProtocolGuid,
                    NULL,
                    (VOID **)&Img4Verify
                    );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }
  //
  // Apple Secure Boot Policy matches SB Mode so far.
  //
  if (SbPolicy != AppleImg4SbModeMedium && SbPolicy != AppleImg4SbModeFull) {
    return EFI_LOAD_ERROR;
  }

  Status = Img4Verify->Verify (
                         Img4Verify,
                         ObjType,
                         ImageBuffer,
                         ImageSize,
                         SbPolicy,
                         ManifestBuffer,
                         ManifestSize,
                         NULL,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    return EFI_SECURITY_VIOLATION;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
InternalVerifyImg4ByPathWorker (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN UINT32                      ObjType,
  IN BOOLEAN                     SetFailureReason,
  IN UINT8                       SbPolicy
  )
{
  EFI_STATUS                      Status;
  BOOLEAN                         Result;

  UINTN                           ImagePathSize;
  UINTN                           ManifestPathSize;
  CHAR16                          *Path;
  CHAR16                          *ManifestSuffix;

  EFI_HANDLE                      Device;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  EFI_FILE_PROTOCOL               *Root;

  VOID                            *ImageBuffer;
  UINT32                          ImageSize;
  VOID                            *ManifestBuffer;
  UINT32                          ManifestSize;

  UINT64                          Ecid;

  STATIC CONST UINTN ManifestSuffixMaxSize =
    (((ARRAY_SIZE (mSbHardwareModel) - 1 + (2 * sizeof (Ecid))) * sizeof (CHAR16))
      + L_STR_SIZE_NT (L"...im4m"));

  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &DevicePath,
                  &Device
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NO_MEDIA;
  }

  ImagePathSize = OcFileDevicePathFullNameSize (DevicePath);
  if (ImagePathSize == 0) {
    return EFI_NO_MEDIA;
  }

  Result = OcOverflowAddUN (
             ImagePathSize,
             ManifestSuffixMaxSize,
             &ManifestPathSize
             );
  if (Result) {
    return EFI_NOT_FOUND;
  }

  Path = AllocatePool (ManifestPathSize);
  if (Path == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->HandleProtocol (
                  Device,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NO_MEDIA;
  }

  Status = FileSystem->OpenVolume (FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return EFI_NO_MEDIA;
  }

  OcFileDevicePathFullName (
    Path,
    (FILEPATH_DEVICE_PATH *)DevicePath,
    ImagePathSize
    );

  ImageBuffer = InternalReadFile (Root, Path, &ImageSize);
  if (ImageBuffer == NULL) {
    Root->Close (Root);
    return EFI_NO_MEDIA;
  }

  Result = mSbHardwareModel[0] != '\0';
  if (Result) {
    ManifestSuffix = &Path[(ImagePathSize / sizeof (*Path)) - 1];
    if (SbPolicy == AppleImg4SbModeMedium) {
      UnicodeSPrint (
        ManifestSuffix,
        ManifestSuffixMaxSize,
        L".%a.im4m",
        mSbHardwareModel
        );
    } else if (SbPolicy == AppleImg4SbModeFull) {
      Result = InternalGetApEcid (&Ecid);
      if (Result) {
        UnicodeSPrint (
          ManifestSuffix,
          ManifestSuffixMaxSize,
          L".%a.%LX.im4m",
          mSbHardwareModel,
          Ecid
          );
      }
    } else {
      Result = FALSE;
    }
  }
  if (!Result) {
    Root->Close (Root);
    FreePool (ImageBuffer);
    return EFI_LOAD_ERROR;
  }

  ManifestBuffer = InternalReadFile (Root, Path, &ManifestSize);

  Root->Close (Root);

  if (ManifestBuffer == NULL) {
    FreePool (ImageBuffer);
    return EFI_NOT_FOUND;
  }

  Status = InternalVerifyImg4Worker (
             This,
             ImageBuffer,
             ImageSize,
             ManifestBuffer,
             ManifestSize,
             ObjType,
             SetFailureReason,
             SbPolicy
             );

  FreePool (ImageBuffer);
  FreePool (ManifestBuffer);

  return Status;
}

/**
  Verify the signature of the file at DevicePath via the matching IMG4
  Manifest.

  @param[in] This              A pointer to the current protocol instance.
  @param[in] DevicePath        The device path to the image to validate.
  @param[in] ObjType           The IMG4 object type to validate against.
  @param[in] SetFailureReason  Whether to set the failure reason.

  @retval EFI_SUCCESS             The file at DevicePath is correctly signed.
  @retval EFI_LOAD_ERROR          The current policy is invalid.
  @retval EFI_INVALID_PARAMETER   One or more required parameters are NULL.
  @retval EFI_UNSUPPORTED         Secure Boot is currently unavailable or
                                  disabled.
  @retval EFI_OUT_OF_RESOURCES    Not enough resources are available.
  @retval EFI_NO_MEDIA            The file at DevicePath could not be read.
  @retval EFI_NOT_FOUND           The file's IMG4 Manifest could not be found.
  @retval EFI_SECURITY_VIOLATION  The file's signature is invalid.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbVerifyImg4ByPath (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN UINT32                      ObjType,
  IN BOOLEAN                     SetFailureReason
  )
{
  EFI_STATUS Status;
  UINT8      SbPolicy;
  UINT8      Reason;

  if (!mSbAvailable) {
    return EFI_UNSUPPORTED;
  }

  AppleSbGetPolicy (This, &SbPolicy);
  if (SbPolicy == 0 || DevicePath == NULL) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = InternalVerifyImg4ByPathWorker (
               This,
               DevicePath,
               ObjType,
               SetFailureReason,
               SbPolicy
               );
  }

  if (SetFailureReason) {
    Reason = InternalImg4GetFailureReason (This, SbPolicy, Status);
    AppleSbSetFailureReason (This, Reason);
  }

  return Status;
}

/**
  Verify the signature of ImageBuffer against ObjType within a IMG4 Manifest.

  @param[in] This              The pointer to the current protocol instance.
  @param[in] ImageBuffer       The buffer to validate.
  @param[in] ImageSize         The size, in bytes, of ImageBuffer.
  @param[in] ManifestBuffer    The buffer of the IMG4 Manifest.
  @param[in] ManifestSize      The size, in bytes, of ManifestBuffer.
  @param[in] ObjType           The IMG4 object type to validate against.
  @param[in] SetFailureReason  Whether to set the failure reason.

  @retval EFI_SUCCESS             ImageBuffer is correctly signed.
  @retval EFI_LOAD_ERROR          The current policy is invalid.
  @retval EFI_INVALID_PARAMETER   One or more required parameters are NULL.
  @retval EFI_UNSUPPORTED         Secure Boot is currently unavailable or
                                  disabled.
  @retval EFI_OUT_OF_RESOURCES    Not enough resources are available.
  @retval EFI_SECURITY_VIOLATION  ImageBuffer's signature is invalid.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbVerifyImg4 (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN CONST VOID                  *ImageBuffer,
  IN UINTN                       ImageSize,
  IN CONST VOID                  *ManifestBuffer,
  IN UINTN                       ManifestSize,
  IN UINT32                      ObjType,
  IN BOOLEAN                     SetFailureReason
  )
{
  EFI_STATUS Status;
  UINT8      SbPolicy;
  UINT8      Reason;

  if (!mSbAvailable) {
    return EFI_UNSUPPORTED;
  }

  AppleSbGetPolicy (This, &SbPolicy);
  if (SbPolicy == 0) {
    Status = EFI_UNSUPPORTED;
  } else if (ImageBuffer == NULL || ImageSize == 0) {
    Status = EFI_INVALID_PARAMETER;
  } else if (ManifestBuffer == NULL || ManifestSize == 0) {
    Status = EFI_NOT_FOUND;
  } else {
    Status = InternalVerifyImg4Worker (
               This,
               ImageBuffer,
               ImageSize,
               ManifestBuffer,
               ManifestSize,
               ObjType,
               SetFailureReason,
               SbPolicy
               );
  }

  if (SetFailureReason) {
    Reason = InternalImg4GetFailureReason (This, SbPolicy, Status);
    if (ObjType == APPLE_SB_OBJ_KERNEL
     || ObjType == APPLE_SB_OBJ_KERNEL_DEBUG) {
      AppleSbSetKernelFailureReason (This, Reason);
    } else {
      AppleSbSetFailureReason (This, Reason);
    }
  }

  return Status;
}

/**
  Verify the signature of the image at DebicePath against a Microsoft
  certificate chain.

  @param[in] This              The pointer to the current protocol instance.
  @param[in] DevicePath        The device path to the image to validate.
  @param[in] SetFailureReason  Whether to set the failure reason.

  @retval EFI_SUCCESS             The file at DevicePath is correctly signed.
  @retval EFI_LOAD_ERROR          The current policy is invalid.
  @retval EFI_INVALID_PARAMETER   One or more required parameters are NULL.
  @retval EFI_UNSUPPORTED         Secure Boot is currently unavailable or
                                  disabled.
  @retval EFI_OUT_OF_RESOURCES    Not enough resources are available.
  @retval EFI_NO_MEDIA            The file at DevicePath could not be read.
  @retval EFI_ACCESS DENIED       A suiting certificate could not be found.
  @retval EFI_SECURITY_VIOLATION  the file's signature is invalid.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbVerifyWindowsByPath (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN BOOLEAN                     SetFailureReason
  )
{
  EFI_STATUS Status;
  UINT8      WinPolicy;
  UINT8      Reason;

  if (!mSbAvailable) {
    return EFI_UNSUPPORTED;
  }

  Reason = 0x00;

  if (DevicePath == NULL) {
    Status = EFI_INVALID_PARAMETER;
    Reason = 0xFF;
  } else {
    Status = EFI_UNSUPPORTED;

    AppleSbGetWindowsPolicy (This, &WinPolicy);
    if (WinPolicy == 1) {
      //
      // We rely on UEFI Secure Boot to perform the verification.
      //
      Status = EFI_SUCCESS;
    } else if (WinPolicy != 0) {
      Status = EFI_LOAD_ERROR;
      Reason = 0xFF;
    }
  }

  if (SetFailureReason) {
    AppleSbSetWindowsFailureReason (This, Reason);
  }

  return Status;
}

/**
  Verify the signature of TargetBuffer against a Microsoft certificate chain.

  @param[in] This              The pointer to the current protocol instance.
  @param[in] TargetBuffer       The buffer to validate.
  @param[in] TargetSize        The size, in bytes, of TargetBuffer.
  @param[in] SetFailureReason  Whether to set the failure reason.

  @retval EFI_SUCCESS             ImageBuffer is correctly signed.
  @retval EFI_LOAD_ERROR          The current policy is invalid.
  @retval EFI_INVALID_PARAMETER   One or more required parameters are NULL.
  @retval EFI_UNSUPPORTED         Secure Boot is currently unavailable or
                                  disabled.
  @retval EFI_ACCESS DENIED       A suiting certificate could not be found.
  @retval EFI_SECURITY_VIOLATION  TargetBuffer's signature is invalid.

**/
STATIC
EFI_STATUS
EFIAPI
AppleSbVerifyWindows (
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN CONST VOID                  *TargetBuffer,
  IN UINTN                       TargetSize,
  IN BOOLEAN                     SetFailureReason
  )
{
  EFI_STATUS Status;
  UINT8      WinPolicy;
  UINT8      Reason;

  if (!mSbAvailable) {
    return EFI_UNSUPPORTED;
  }

  Reason = 0x00;

  if (TargetBuffer == NULL || TargetSize == 0) {
    Status = EFI_INVALID_PARAMETER;
    Reason = 0xFF;
  } else {
    Status = EFI_UNSUPPORTED;

    AppleSbGetWindowsPolicy (This, &WinPolicy);
    if (WinPolicy == 1) {
      //
      // We rely on UEFI Secure Boot to perform the verification.
      //
      Status = EFI_SUCCESS;
    } else if (WinPolicy != 0) {
      Status = EFI_LOAD_ERROR;
      Reason = 0xFF;
    }
  }

  if (SetFailureReason) {
    AppleSbSetWindowsFailureReason (This, Reason);
  }

  return Status;
}

APPLE_SECURE_BOOT_PROTOCOL *
OcAppleSecureBootInstallProtocol (
  IN BOOLEAN  Reinstall,
  IN UINT8    SbPolicy,
  IN UINT8    SbWinPolicy OPTIONAL,
  IN BOOLEAN  SbWinPolicyValid
  )
{
  STATIC APPLE_SECURE_BOOT_PROTOCOL SecureBoot = {
    APPLE_SECURE_BOOT_PROTOCOL_REVISION,
    AppleSbSetAvailability,
    AppleSbVerifyImg4ByPath,
    AppleSbVerifyImg4,
    AppleSbGetPolicy,
    AppleSbGetFailureReason,
    AppleSbSetFailureReason,
    AppleSbGetKernelFailureReason,
    AppleSbSetKernelFailureReason,
    AppleSbVerifyWindowsByPath,
    AppleSbVerifyWindows,
    AppleSbGetWindowsPolicy,
    AppleSbGetWindowsFailureReason,
    AppleSbSetWindowsFailureReason
  };

  EFI_STATUS                 Status;
  APPLE_SECURE_BOOT_PROTOCOL *Protocol;
  EFI_HANDLE                 Handle;
  UINTN                      DataSize;

  if (Reinstall) {
    Status = OcUninstallAllProtocolInstances (&gAppleSecureBootProtocolGuid);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCSB: Uninstall failed: %r\n", Status));
      return NULL;
    }
  } else {
    Status = gBS->LocateProtocol (
                    &gAppleSecureBootProtocolGuid,
                    NULL,
                    (VOID **)&Protocol
                    );
    if (!EFI_ERROR (Status)) {
      return Protocol;
    }
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gAppleSecureBootProtocolGuid,
                  (VOID **)&SecureBoot,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  mSbPolicy             = SbPolicy;
  mSbWindowsPolicy      = SbWinPolicy;
  mSbWindowsPolicyValid = SbWinPolicyValid;

  DataSize = sizeof (SbPolicy);
  gRT->SetVariable (
          L"AppleSecureBootPolicy",
          &gAppleSecureBootVariableGuid,
          EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
          DataSize,
          &SbPolicy
          );
  if (SbWinPolicyValid) {
    DataSize = sizeof (SbWinPolicy);
    gRT->SetVariable (
           L"AppleSecureBootWindowsPolicy",
           &gAppleSecureBootVariableGuid,
           EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           DataSize,
           &SbWinPolicy
           );
  }

  return &SecureBoot;
}
