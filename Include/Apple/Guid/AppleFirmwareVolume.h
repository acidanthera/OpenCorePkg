/** @file
Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_FIRMWARE_VOLUME_H
#define APPLE_FIRMWARE_VOLUME_H

// APPLE_IMAGE_LIST_GUID
/// The Apple Image List GUID.
#define APPLE_IMAGE_LIST_GUID    \
  { 0x0E93C52B, 0x4B73, 0x5C32,  \
    { 0x86, 0xD5, 0x69, 0x25, 0x0A, 0x0B, 0xA7, 0x23 } }

// APPLE_ARROW_CURSOR_IMAGE_GUID
/// The Apple Arrow Cursor Image GUID.
#define APPLE_ARROW_CURSOR_IMAGE_GUID  \
  { 0x1A10742F, 0xFA80, 0x4B79,        \
    { 0x9D, 0xA6, 0x35, 0x70, 0x58, 0xCC, 0x39, 0x7B } }

// APPLE_ARROW_CURSOR_2X_IMAGE_GUID
/// The Apple HiDPI Arrow Cursor Image GUID.
#define APPLE_ARROW_CURSOR_2X_IMAGE_GUID  \
  { 0x133D55B5, 0x8852, 0x57AC,           \
    { 0xA8, 0x42, 0xF9, 0xEE, 0xBF, 0x84, 0x0C, 0x99 } }

// APPLE_BACK_BUTTON_SMALL_IMAGE_GUID
#define APPLE_BACK_BUTTON_SMALL_IMAGE_GUID  \
  { 0x63FA7900, 0x6DD2, 0x4BB3,             \
    { 0x99, 0x76, 0x87, 0x0F, 0xE2, 0x7A, 0x53, 0xC2 } }

// APPLE_BATTERY_STATE_0_IMAGE_GUID
#define APPLE_BATTERY_STATE_0_IMAGE_GUID  \
  { 0x6ECEFFFD, 0x614D, 0x452E,           \
    { 0xA8, 0x1D, 0x25, 0xE5, 0x6B, 0x0D, 0xEF, 0x98 } }

// APPLE_BATTERY_STATE_1_IMAGE_GUID
#define APPLE_BATTERY_STATE_1_IMAGE_GUID  \
  { 0x23D1280D, 0x43F0, 0x4713,           \
    { 0x90, 0xB2, 0x0E, 0x5E, 0x42, 0x21, 0xAF, 0x4C } }

// APPLE_BATTERY_STATE_2_IMAGE_GUID
#define APPLE_BATTERY_STATE_2_IMAGE_GUID  \
  { 0x53531469, 0x558E, 0x4AF1,           \
    { 0x80, 0x3A, 0xF9, 0x66, 0xF2, 0x7C, 0x57, 0x3B } }

// APPLE_BATTERY_STATE_3_IMAGE_GUID
#define APPLE_BATTERY_STATE_3_IMAGE_GUID  \
  { 0x3BBCB209, 0x26C8, 0x4BA9,           \
    { 0xAD, 0x25, 0xB9, 0x5B, 0x45, 0xA0, 0x4D, 0x26 } }

// APPLE_BATTERY_STATE_4_IMAGE_GUID
#define APPLE_BATTERY_STATE_4_IMAGE_GUID  \
  { 0xED8DCDD5, 0xD037, 0x4B1F,           \
    { 0x98, 0xDD, 0xBD, 0xFD, 0xAD, 0x4D, 0xD7, 0xDD } }

// APPLE_BATTERY_STATE_5_IMAGE_GUID
#define APPLE_BATTERY_STATE_5_IMAGE_GUID  \
  { 0x637E0BA6, 0xC5BB, 0x41B7,           \
    { 0xA2, 0x3B, 0x3A, 0x65, 0xCF, 0xC3, 0xE9, 0xDB } }

// APPLE_BATTERY_STATE_6_IMAGE_GUID
#define APPLE_BATTERY_STATE_6_IMAGE_GUID  \
  { 0x7A627E16, 0x679D, 0x4814,           \
    { 0x8F, 0x82, 0xEE, 0xAF, 0x38, 0x81, 0xF0, 0x98 } }

// APPLE_BEGIN_BOOT_BUTTON_IMAGE_GUID
#define APPLE_BEGIN_BOOT_BUTTON_IMAGE_GUID  \
  { 0xE8A59290, 0xA2AF, 0x4099,             \
    { 0xB0, 0xAF, 0x32, 0x3F, 0xF9, 0xB7, 0xAB, 0x41 } }

// APPLE_BEGIN_STICKY_BOOT_BUTTON_IMAGE_GUID
#define APPLE_BEGIN_STICKY_BOOT_BUTTON_IMAGE_GUID  \
  { 0xB4339807, 0x7CAC, 0x49BA,                    \
    { 0x9F, 0xB7, 0x62, 0x31, 0xC6, 0x22, 0xF2, 0x70 } }

// APPLE_CONTINUE_BUTTON_SMALL_IMAGE_GUID
#define APPLE_CONTINUE_BUTTON_SMALL_IMAGE_GUID  \
  { 0x728CAE6C, 0x1FFC, 0x449B,                 \
    { 0x86, 0x81, 0xBB, 0x2A, 0x62, 0x1E, 0x00, 0x22 } }

// APPLE_BOOT_NAME_LABEL_IMAGE_GUID
#define APPLE_BOOT_NAME_LABEL_IMAGE_GUID  \
  { 0xC0512F00, 0x0181, 0x48C0,           \
    { 0x8B, 0x71, 0x90, 0x50, 0x4B, 0x8F, 0x99, 0x1E } }

// APPLE_BROKEN_BOOT_IMAGE_GUID
#define APPLE_BROKEN_BOOT_IMAGE_GUID  \
  { 0x6776572C, 0xFE56, 0x42CA,       \
    { 0x9B, 0x93, 0x3D, 0x09, 0x60, 0xE7, 0x58, 0x3A } }

// APPLE_FIREWIRE_HD_IMAGE_GUID
#define APPLE_FIREWIRE_HD_IMAGE_GUID  \
  { 0x410C1D0C, 0x656F, 0x4769,       \
    { 0x8D, 0xFB, 0x90, 0xF9, 0xA0, 0x30, 0x3E, 0x9F } }

// APPLE_GENERIC_CD_IMAGE_GUID
#define APPLE_GENERIC_CD_IMAGE_GUID  \
  { 0x21A05FD5, 0xDB4A, 0x4CFC,      \
    { 0xB8, 0x4B, 0xEB, 0x0D, 0xBB, 0x56, 0x99, 0x34 } }

// APPLE_GENERIC_EXTERNAL_HD_IMAGE_GUID
#define APPLE_GENERIC_EXTERNAL_HD_IMAGE_GUID  \
  { 0xD872AEFA, 0x7C5F, 0x4C66,               \
    { 0x88, 0x36, 0xAA, 0x57, 0xEF, 0xF0, 0xD9, 0xF8 } }

// APPLE_INTERNAL_HD_IMAGE_GUID
#define APPLE_INTERNAL_HD_IMAGE_GUID  \
  { 0x809FBBFD, 0x127A, 0x4249,       \
    { 0x88, 0xBC, 0xFD, 0x0E, 0x76, 0x7F, 0x4F, 0xFD } }

// APPLE_NETBOOT_IMAGE_GUID
#define APPLE_NETBOOT_IMAGE_GUID  \
  { 0x13ECD928, 0x87AB, 0x4460,   \
    { 0xBB, 0xE0, 0xB5, 0x20, 0xF9, 0xEB, 0x1D, 0x32 } }

// APPLE_NETWORK_RECOVERY_IMAGE_GUID
#define APPLE_NETWORK_RECOVERY_IMAGE_GUID  \
  { 0x6F92E393, 0x03C0, 0x427B,            \
    { 0xBB, 0xEB, 0x4E, 0xF8, 0x07, 0xB5, 0x5B, 0xD8 } }

// APPLE_NETWORK_VOLUME_IMAGE_GUID
#define APPLE_NETWORK_VOLUME_IMAGE_GUID  \
  { 0xE6F930E0, 0xBAE5, 0x40E6,          \
    { 0x98, 0xC9, 0x4C, 0xD2, 0x29, 0x82, 0x78, 0xE7 } }

// APPLE_PASSWORD_LOCK_IMAGE_GUID
#define APPLE_PASSWORD_LOCK_IMAGE_GUID  \
  { 0xBB1A3984, 0xD171, 0x4003,         \
    { 0x90, 0x94, 0x46, 0xAF, 0x86, 0x6B, 0x45, 0xA2 } }

// APPLE_SD_IMAGE_GUID
#define APPLE_SD_IMAGE_GUID      \
  { 0x5B6DAB96, 0x195D, 0x4D24,  \
    { 0x97, 0x27, 0xA7, 0xD0, 0xE9, 0x36, 0x65, 0xC6 } }

// APPLE_SELECTED_IMAGE_GUID
#define APPLE_SELECTED_IMAGE_GUID  \
  { 0xA0AAFF71, 0x35DA, 0x41EE,    \
    { 0x86, 0x3F, 0xA2, 0x4F, 0x42, 0x9E, 0x59, 0xE4 } }

// APPLE_USB_HD_IMAGE_GUID
#define APPLE_USB_HD_IMAGE_GUID  \
  { 0x1BFC532E, 0xF48A, 0x4EBE,  \
    { 0xB2, 0xFB, 0x2B, 0x28, 0x6D, 0x70, 0xA6, 0xEB } }

// APPLE_WIRELESS_SMALL_IMAGE_GUID
#define APPLE_WIRELESS_SMALL_IMAGE_GUID  \
  { 0x2F08C089, 0x2073, 0x4BD9,          \
    { 0x9E, 0x7E, 0x30, 0x8A, 0x18, 0x32, 0x7B, 0x53 } }

// APPLE_LOGO_IMAGE_GUID
#define APPLE_LOGO_IMAGE_GUID  \
{ 0x7914C493, 0xF439, 0x4C6C,  \
  { 0xAB, 0x23, 0x7F, 0x72, 0x15, 0x0E, 0x72, 0xD4 } }

// APPLE_PASSWORD_EMPTY_IMAGE_GUID
#define APPLE_PASSWORD_EMPTY_IMAGE_GUID  \
  { 0x8F98528C, 0xF736, 0x4A84,          \
    { 0xAA, 0xA3, 0x37, 0x6A, 0x8E, 0x43, 0xBF, 0x51 } }

// APPLE_PASSWORD_FILL_IMAGE_GUID
#define APPLE_PASSWORD_FILL_IMAGE_GUID  \
  { 0x71F3B066, 0x936A, 0x4C84,         \
    { 0x92, 0x28, 0x23, 0x23, 0x0F, 0xD4, 0x7C, 0x79 } }

// APPLE_PASSWORD_PROCEED_IMAGE_GUID
#define APPLE_PASSWORD_PROCEED_IMAGE_GUID  \
  { 0x689CDA29, 0x29A8, 0x42F6,            \
    { 0x93, 0xFC, 0x46, 0xBA, 0x5F, 0x18, 0x06, 0x51 } }

// APPLE_LOGO_1394_IMAGE_GUID
#define APPLE_LOGO_1394_IMAGE_GUID  \
  { 0xF2C1819D, 0x10F5, 0x4223,     \
    { 0x92, 0x36, 0x9B, 0x4E, 0xBF, 0x1B, 0x9A, 0xE7 } }

// APPLE_LOGO_THUNDERBOLT_IMAGE_GUID
#define APPLE_LOGO_THUNDERBOLT_IMAGE_GUID  \
  { 0xE646C3A8, 0xC7E2, 0x4DC2,            \
    { 0xA7, 0xF2, 0xE3, 0x2A, 0x27, 0x0B, 0x0B, 0x26 } }

// APPLE_CLOCK_IMAGE_GUID
#define APPLE_CLOCK_IMAGE_GUID   \
  { 0x224FBFE4, 0xADB6, 0x4DF2,  \
    { 0xB8, 0x35, 0x60, 0x21, 0x82, 0xAE, 0xEF, 0x20 } }

// APPLE_ERROR_GLOBE_BORDER_IMAGE_GUID
#define APPLE_ERROR_GLOBE_BORDER_IMAGE_GUID  \
  { 0x022218B8, 0xFE5E, 0x4EBC,              \
    { 0xBC, 0x96, 0x74, 0x05, 0x8A, 0x4E, 0x7E, 0x83 } }

// APPLE_ERROR_GLOBE_TITLE_IMAGE_GUID
#define APPLE_ERROR_GLOBE_TITLE_IMAGE_GUID  \
  { 0xAD0D149F, 0xBA67, 0x4E0B,             \
    { 0xA6, 0xA2, 0x4E, 0x88, 0x53, 0x67, 0x3E, 0xA5 } }

// APPLE_ERROR_TRIANGLE_IMAGE_GUID
#define APPLE_ERROR_TRIANGLE_IMAGE_GUID  \
  { 0x290B026F, 0x6905, 0x4612,          \
    { 0xBA, 0x0F, 0xF6, 0x35, 0xDD, 0xE3, 0x52, 0x85 } }

// APPLE_GLOBE_BORDER_IMAGE_GUID
#define APPLE_GLOBE_BORDER_IMAGE_GUID  \
  { 0x6E66DAE5, 0x4108, 0x40B5,        \
    { 0x89, 0xA9, 0xC6, 0x10, 0x3F, 0x06, 0x39, 0xEC } }

// APPLE_GLOBE_MASK_IMAGE_GUID
#define APPLE_GLOBE_MASK_IMAGE_GUID  \
  { 0xFC788727, 0xC2D0, 0x469C,      \
    { 0xBD, 0x03, 0x5A, 0xEA, 0x03, 0x32, 0x3C, 0x67 } }

// gAppleImageListGuid
extern EFI_GUID gAppleImageListGuid;

// gAppleArrowCursorImageGuid
extern EFI_GUID gAppleArrowCursorImageGuid;

// gAppleArrowCursor2xImageGuid
extern EFI_GUID gAppleArrowCursor2xImageGuid;

// gAppleBackButtonSmallImageGuid
extern EFI_GUID gAppleBackButtonSmallImageGuid;

// gAppleBatteryState0ImageGuid
extern EFI_GUID gAppleBatteryState0ImageGuid;

// gAppleBatteryState1ImageGuid
extern EFI_GUID gAppleBatteryState1ImageGuid;

// gAppleBatteryState2ImageGuid
extern EFI_GUID gAppleBatteryState2ImageGuid;

// gAppleBatteryState3ImageGuid
extern EFI_GUID gAppleBatteryState3ImageGuid;

// gAppleBatteryState4ImageGuid
extern EFI_GUID gAppleBatteryState4ImageGuid;

// gAppleBatteryState5ImageGuid
extern EFI_GUID gAppleBatteryState5ImageGuid;

// gAppleBatteryState6ImageGuid
extern EFI_GUID gAppleBatteryState6ImageGuid;

// gAppleBeginBootButtonImageGuid
extern EFI_GUID gAppleBeginBootButtonImageGuid;

// gAppleBeginStickyBootButtonImageGuid
extern EFI_GUID gAppleBeginStickyBootButtonImageGuid;

// gAppleContinueButtonSmallImageGuid
extern EFI_GUID gAppleContinueButtonSmallImageGuid;

// gAppleBootNameLabelImageGuid
extern EFI_GUID gAppleBootNameLabelImageGuid;

// gAppleBrokenBootImageGuid
extern EFI_GUID gAppleBrokenBootImageGuid;

// gAppleFireWireHDImageGuid
extern EFI_GUID gAppleFireWireHDImageGuid;

// gAppleGenericCDImageGuid
extern EFI_GUID gAppleGenericCDImageGuid;

// gAppleGenericExternalHDImageGuid
extern EFI_GUID gAppleGenericExternalHDImageGuid;

// gAppleInternalHDImageGuid
extern EFI_GUID gAppleInternalHDImageGuid;

// gAppleNetBootImageGuid
extern EFI_GUID gAppleNetBootImageGuid;

// gAppleNetworkRecoveryImageGuid
extern EFI_GUID gAppleNetworkRecoveryImageGuid;

// gAppleNetworkVolumeImageGuid
extern EFI_GUID gAppleNetworkVolumeImageGuid;

// gApplePasswordLockImageGuid
extern EFI_GUID gApplePasswordLockImageGuid;

// gAppleSDImageGuid
extern EFI_GUID gAppleSDImageGuid;

// gAppleSelectedImageGuid
extern EFI_GUID gAppleSelectedImageGuid;

// gAppleUsbHDImageGuid
extern EFI_GUID gAppleUsbHDImageGuid;

// gAppleWirelessSmallImageGuid
extern EFI_GUID gAppleWirelessSmallImageGuid;

// gAppleLogoImageGuid
extern EFI_GUID gAppleLogoImageGuid;

// gApplePasswordEmptyImageGuid
extern EFI_GUID gApplePasswordEmptyImageGuid;

// gApplePasswordFillImageGuid
extern EFI_GUID gApplePasswordFillImageGuid;

// gApplePasswordProceedImageGuid
extern EFI_GUID gApplePasswordProceedImageGuid;

// gAppleLogo1394ImageGuid
extern EFI_GUID gAppleLogo1394ImageGuid;

// gAppleLogoThunderboltImageGuid
extern EFI_GUID gAppleLogoThunderboltImageGuid;

// gAppleClockImageGuid
extern EFI_GUID gAppleClockImageGuid;

// gAppleErrorGlobeBorderImageGuid
extern EFI_GUID gAppleErrorGlobeBorderImageGuid;

// gAppleErrorGlobeTitleImageGuid
extern EFI_GUID gAppleErrorGlobeTitleImageGuid;

// gAppleErrorTriangleImageGuid
extern EFI_GUID gAppleErrorTriangleImageGuid;

// gAppleGlobeBorderImageGuid
extern EFI_GUID gAppleGlobeBorderImageGuid;

// gAppleGlobeMaskImageGuid
extern EFI_GUID gAppleGlobeMaskImageGuid;

#endif // APPLE_FIRMWARE_VOLUME_H
