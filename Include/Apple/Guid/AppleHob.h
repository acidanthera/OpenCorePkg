/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_HOB_H
#define APPLE_HOB_H

// APPLE_DEBUG_MASK_HOB_GUID
#define APPLE_DEBUG_MASK_HOB_GUID  \
  { 0x59D1C24F, 0x50F1, 0x401A,    \
    { 0xB1, 0x01, 0xF3, 0x3E, 0x0D, 0xAE, 0xD4, 0x43 } }

// APPLE_FSB_FREQUENCY_PLATFORM_INFO_INDEX_HOB_GUID
#define APPLE_FSB_FREQUENCY_PLATFORM_INFO_INDEX_HOB_GUID  \
  { 0xEF56B861, 0x03CD, 0x4991,                           \
    { 0x99, 0xF2, 0x2A, 0xD3, 0x1B, 0xE8, 0x6B, 0x22 } }

// APPLE_SMC_MMIO_ADDRESS_HOB_GUID
#define APPLE_SMC_MMIO_ADDRESS_HOB_GUID  \
  { 0x2D450255, 0xBDE9, 0x4341,          \
    { 0x8C, 0x72, 0xF0, 0x77, 0x09, 0x59, 0x76, 0x04 } }

// APPLE_TSC_FREQUENCY_HOB_GUID
#define APPLE_TSC_FREQUENCY_HOB_GUID  \
  { 0x674ABEA3, 0x0FE5, 0x11E5,       \
    { 0x98, 0x8E, 0xB8, 0xE8, 0x56, 0x2C, 0xBA, 0xFA } }

// APPLE_HOB_1_GUID
#define APPLE_HOB_1_GUID         \
  { 0x908B63A8, 0xC7C8, 0x493A,  \
    { 0x80, 0x72, 0x9D, 0x58, 0xDB, 0xCF, 0x72, 0x4D } }

// APPLE_HOB_2_GUID
#define APPLE_HOB_2_GUID         \
  { 0xC78F061E, 0x0290, 0x4E4F,  \
    { 0x8D, 0xDC, 0x5B, 0xDA, 0xAC, 0x83, 0x7D, 0xE5 } }

// APPLE_HOB_3_GUID
#define APPLE_HOB_3_GUID         \
  { 0xB8E65062, 0xFB30, 0x4078,  \
    { 0xAB, 0xD3, 0xA9, 0x4E, 0x09, 0xCA, 0x9D, 0xE6 } }

// gAppleDebugMaskHobGuid
extern EFI_GUID gAppleDebugMaskHobGuid;

// gAppleFsbFrequencyPlatformInfoIndexHobGuid
extern EFI_GUID gAppleFsbFrequencyPlatformInfoIndexHobGuid;

// gAppleSmcMmioAddressHobGuid
extern EFI_GUID gAppleSmcMmioAddressHobGuid;

// gAppleTscFrequencyHobGuid
extern EFI_GUID gAppleTscFrequencyHobGuid;

// gAppleHob1Guid
extern EFI_GUID gAppleHob1Guid;

// gAppleHob1Guid
extern EFI_GUID gAppleHob2Guid;

// gAppleHob1Guid
extern EFI_GUID gAppleHob3Guid;

#endif //ifndef APPLE_HOB_H
