/** @file
  The Apple Secure Boot protocol provides services to verify a trusted image's
  integrity.

Copyright (C) 2019, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_SECURE_BOOT_H
#define APPLE_SECURE_BOOT_H

#define APPLE_SECURE_BOOT_PROTOCOL_REVISION  4U

#define APPLE_SECURE_BOOT_PROTOCOL_GUID  \
 { 0x24B73556, 0x2197, 0x4702,           \
   { 0x82, 0xA8, 0x3E, 0x13, 0x37, 0xDA, 0xFB, 0xF2 } }

typedef struct APPLE_SECURE_BOOT_PROTOCOL_ APPLE_SECURE_BOOT_PROTOCOL;

#define APPLE_SB_OBJ_TYPE(A, B, C, D)  \
  (UINT32)(                            \
    ((A) << 24UL)                      \
  | ((B) << 16UL)                      \
  | ((C) << 8UL)                       \
  | ((D) << 0UL)                       \
  )

//
// The keys are iterated in the order in which they are defined here in
// AppleBds to validate any loaded image.
//
#define APPLE_SB_OBJ_EFIBOOT        APPLE_SB_OBJ_TYPE ('e', 'f', 'i', 'b')
#define APPLE_SB_OBJ_EFIBOOT_DEBUG  APPLE_SB_OBJ_TYPE ('e', 'f', 'b', 'd')
#define APPLE_SB_OBJ_EFIBOOT_BASE   APPLE_SB_OBJ_TYPE ('e', 'f', 'b', 'b')

#define APPLE_SB_OBJ_MUPD  APPLE_SB_OBJ_TYPE ('m', 'u', 'p', 'd')
#define APPLE_SB_OBJ_HPMU  APPLE_SB_OBJ_TYPE ('h', 'p', 'm', 'u')
#define APPLE_SB_OBJ_THOU  APPLE_SB_OBJ_TYPE ('t', 'h', 'o', 'u')
#define APPLE_SB_OBJ_GPUU  APPLE_SB_OBJ_TYPE ('g', 'p', 'u', 'u')
#define APPLE_SB_OBJ_ETHU  APPLE_SB_OBJ_TYPE ('e', 't', 'h', 'u')
#define APPLE_SB_OBJ_SDFU  APPLE_SB_OBJ_TYPE ('s', 'd', 'f', 'u')
#define APPLE_SB_OBJ_DTHU  APPLE_SB_OBJ_TYPE ('d', 't', 'h', 'u')

//
// The keys are iterated in the order in which they are defined here in
// boot.efi to validate the loaded kernel image.
//
#define APPLE_SB_OBJ_KERNEL        APPLE_SB_OBJ_TYPE ('m', 'k', 'r', 'n')
#define APPLE_SB_OBJ_KERNEL_DEBUG  APPLE_SB_OBJ_TYPE ('m', 'k', 'r', 'd')

/**
  Sets the Secure Boot availability state.

  @param[in] This       A pointer to the current protocol instance.
  @param[in] Available  The new availability status for Secure Boot.

**/
typedef
VOID
(EFIAPI *APPLE_SB_SET_AVAILABILITY)(
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN BOOLEAN                     Available
  );

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
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_VERIFY_IMG4_BY_PATH)(
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN UINT32                      ObjType,
  IN BOOLEAN                     SetFailureReason
  );

/**
  Verify the signature of ImageBuffer against Type within a IMG4 Manifest.

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
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_VERIFY_IMG4)(
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN CONST VOID                  *ImageBuffer,
  IN UINTN                       ImageSize,
  IN CONST VOID                  *ManifestBuffer,
  IN UINTN                       ManifestSize,
  IN UINT32                      ObjType,
  IN BOOLEAN                     SetFailureReason
  );

/**
  Retrieves the current Secure Boot policy.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Policy  On output, the current Secure Boot policy.

  @retval EFI_SUCCESS            The current policy has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.
  @retval EFI_NOT_FOUND          The current policy could not be retrieved.

**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_GET_POLICY)(
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Policy
  );

/**
  Retrieves the current Secure Boot failure reason.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Reason  On output, the current failure reason.

  @retval EFI_SUCCESS            The current failure reason has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.

**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_GET_FAILURE_REASON)(
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Reason
  );

/**
  Sets the Secure Boot failure reason.

  @param[in] This    A pointer to the current protocol instance.
  @param[in] Reason  The failure reason to set.

  @retval EFI_SUCCESS      The failure reason has been set successfully.
  @retval EFI_UNSUPPORTED  Secure Boot is currently unavailable.

**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_SET_FAILURE_REASON)(
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN UINT8                       Reason
  );

/**
  Retrieves the current Secure Boot Kernel failure reason.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Reason  On output, the current Kernel failure reason.

  @retval EFI_SUCCESS            The current failure reason has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.

**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_GET_KERNEL_FAILURE_REASON)(
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Reason
  );

/**
  Sets the Secure Boot Kernel failure reason.

  @param[in] This    A pointer to the current protocol instance.
  @param[in] Reason  The Kernel failure reason to set.

  @retval EFI_SUCCESS      The failure reason has been set successfully.
  @retval EFI_UNSUPPORTED  Secure Boot is currently unavailable.

**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_SET_KERNEL_FAILURE_REASON)(
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN UINT8                       Reason
  );

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
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_VERIFY_WINDOWS_BY_PATH)(
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN BOOLEAN                     SetFailureReason
  );

/**
  Verify the signature of ImageBuffer against a Microsoft certificate chain.

  @param[in] This              The pointer to the current protocol instance.
  @param[in] ImageBuffer       The buffer to validate.
  @param[in] ImageSize         The size, in bytes, of ImageBuffer.
  @param[in] SetFailureReason  Whether to set the failure reason.

  @retval EFI_SUCCESS             ImageBuffer is correctly signed.
  @retval EFI_LOAD_ERROR          The current policy is invalid.
  @retval EFI_INVALID_PARAMETER   One or more required parameters are NULL.
  @retval EFI_UNSUPPORTED         Secure Boot is currently unavailable or
                                  disabled.
  @retval EFI_ACCESS DENIED       A suiting certificate could not be found.
  @retval EFI_SECURITY_VIOLATION  ImageBuffer's signature is invalid.

**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_VERIFY_WINDOWS)(
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN CONST VOID                  *ImageBuffer,
  IN UINTN                       ImageSize,
  IN BOOLEAN                     SetFailureReason
  );

/**
  Retrieves the current Secure Boot Windows policy.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Policy  On output, the current Secure Boot Windows policy.

  @retval EFI_SUCCESS            The current policy has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.
  @retval EFI_NOT_FOUND          The current policy could not be retrieved.

**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_GET_WINDOWS_POLICY)(
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Policy
  );

/**
  Retrieves the current Secure Boot Windows failure reason.

  @param[in]  This    A pointer to the current protocol instance.
  @param[out] Reason  On output, the current Windows failure reason.

  @retval EFI_SUCCESS            The current failure reason has been returned.
  @retval EFI_INVALID_PARAMETER  One or more required parameters are NULL.

**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_GET_WINDOWS_FAILURE_REASON)(
  IN  APPLE_SECURE_BOOT_PROTOCOL  *This,
  OUT UINT8                       *Reason
  );

/**
  Sets the Secure Boot Windows failure reason.

  @param[in] This    A pointer to the current protocol instance.
  @param[in] Reason  The Windows failure reason to set.

  @retval EFI_SUCCESS      The failure reason has been set successfully.
  @retval EFI_UNSUPPORTED  Secure Boot is currently unavailable.

**/
typedef
EFI_STATUS
(EFIAPI *APPLE_SB_SET_WINDOWS_FAILURE_REASON)(
  IN APPLE_SECURE_BOOT_PROTOCOL  *This,
  IN UINT8                       Reason
  );

struct APPLE_SECURE_BOOT_PROTOCOL_ {
  UINTN                               Revision;
  APPLE_SB_SET_AVAILABILITY           SetAvailability;
  APPLE_SB_VERIFY_IMG4_BY_PATH        VerifyImg4ByPath;
  APPLE_SB_VERIFY_IMG4                VerifyImg4;
  APPLE_SB_GET_POLICY                 GetPolicy;
  APPLE_SB_GET_FAILURE_REASON         GetFailureReason;
  APPLE_SB_SET_FAILURE_REASON         SetFailureReason;
  APPLE_SB_GET_KERNEL_FAILURE_REASON  GetKernelFailureReason;
  APPLE_SB_SET_KERNEL_FAILURE_REASON  SetKernelFailureReason;
  APPLE_SB_VERIFY_WINDOWS_BY_PATH     VerifyWindowsByPath;
  APPLE_SB_VERIFY_WINDOWS             VerifyWindows;
  APPLE_SB_GET_WINDOWS_POLICY         GetWindowsPolicy;
  APPLE_SB_GET_WINDOWS_FAILURE_REASON GetWindowsFailureReason;
  APPLE_SB_SET_WINDOWS_FAILURE_REASON SetWindowsFailureReason;
};

extern EFI_GUID gAppleSecureBootProtocolGuid;

#endif // APPLE_SECURE_BOOT_H
