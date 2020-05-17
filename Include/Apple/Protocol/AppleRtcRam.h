/** @file
  Apple RTC RAM.

Copyright (C) 2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_RTC_RAM_PROTOCOL_H
#define APPLE_RTC_RAM_PROTOCOL_H

#include <IndustryStandard/AppleRtc.h>

/**
  Installed by RTC driver.
  E121EC07-9C42-45EE-B0B6-FFF8EF03C521
  Found in AppleRtcRam (CC54F583-3F9E-4AB0-9F7C-D2C7ED1C87A5).

  Note, there is a sibling driver in PEI:
  13CFE225-E07B-40F3-9703-EBE99318766E
  Found in AppleRtcRamPeim (1B99796D-2A26-437E-BEE0-014F0EBBECE1).
**/

#define APPLE_RTC_RAM_PROTOCOL_GUID \
  { 0xE121EC07, 0x9C42, 0x45EE,     \
     { 0xB0, 0xB6, 0xFF, 0xF8, 0xEF, 0x03, 0xC5, 0x21 } }

typedef struct APPLE_RTC_RAM_PROTOCOL_ APPLE_RTC_RAM_PROTOCOL;

/**
  Obtain available RTC memory in bytes.

  @param[in]  This   Apple RTC RAM protocol instance.

  @retval 256 under normal circumstances.
**/
typedef
UINTN
(EFIAPI *APPLE_RTC_RAM_GET_AVAILABLE_MEMORY) (
  IN  APPLE_RTC_RAM_PROTOCOL  *This
  );

/**
  Read memory from RTC.

  @param[in]  This        Apple RTC RAM protocol instance.
  @param[out] Buffer      Destination buffer to read to.
  @param[in]  BufferSize  The amount of memory to read in bytes.
  @param[in]  Address     The starting RTC offset to read from.

  The implementation should respect data consistency:
  - If APPLE_RTC_BG_COLOR_ADDR ^ APPLE_RTC_BG_COMPLEMENT_ADDR != 0xFF
    APPLE_RTC_BG_COLOR_GRAY should be returned for APPLE_RTC_BG_COLOR_ADDR.

  @retval EFI_INVALID_PARAMETER  when Buffer is NULL.
  @retval EFI_INVALID_PARAMETER  when BufferSize is 0.
  @retval EFI_INVALID_PARAMETER  when the requested memory is out of range.
  @retval EFI_ACCESS_DENIED      when an RTC I/O operating is in progress.
  @retval EFI_TIMEOUT            when RTC device is not ready.
  @retval EFI_SUCCESS            on success.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_RTC_RAM_READ_MEMORY) (
  IN  APPLE_RTC_RAM_PROTOCOL  *This,
  OUT UINT8                   *Buffer,
  IN  UINTN                   BufferSize,
  IN  UINTN                   Address
  );

/**
  Write memory to RTC.

  @param[in]  This        Apple RTC RAM protocol instance.
  @param[in]  Buffer      Source buffer to write to RTC.
  @param[in]  BufferSize  The amount of memory to write in bytes.
  @param[in]  Address     The starting RTC offset to write to.

  The implementation should maintain data consistency:
  - APPLE_RTC_BG_COMPLEMENT_ADDR should be set to ~APPLE_RTC_BG_COLOR_ADDR.
  - APPLE_RTC_CORE_CHECKSUM_ADDR1 / APPLE_RTC_CORE_CHECKSUM_ADDR2 should
    be recalculated if necessary.
  - APPLE_RTC_MAIN_CHECKSUM_ADDR1 / APPLE_RTC_MAIN_CHECKSUM_ADDR2 should
    be recalculated if necessary.

  @retval EFI_INVALID_PARAMETER  when Buffer is NULL.
  @retval EFI_INVALID_PARAMETER  when BufferSize is 0.
  @retval EFI_INVALID_PARAMETER  when the requested memory is out of range.
  @retval EFI_INVALID_PARAMETER  when trying to update system memory
                                 before APPLE_RTC_CHECKSUM_START.
  @retval EFI_OUT_OF_RESOURCES   when a memory allocation error happened.
  @retval EFI_ACCESS_DENIED      when an RTC I/O operating is in progress.
  @retval EFI_TIMEOUT            when RTC device is not ready.
  @retval EFI_SUCCESS            on success.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_RTC_RAM_WRITE_MEMORY) (
  IN  APPLE_RTC_RAM_PROTOCOL  *This,
  IN  CONST UINT8             *Buffer,
  IN  UINTN                   BufferSize,
  IN  UINTN                   Address
  );

/**
  Reset RTC memory to default values. For uses like CMD+OPT+P+R.

  The implementation should maintain data consistency:
  - Core memory as defined by APPLE_RTC_CORE_SIZE should not be changed.
  - Preserve APPLE_RTC_RESERVED_ADDR area.
  - Preserve APPLE_RTC_FIRMWARE_57_ADDR address.

  @param[in]  This   Apple RTC RAM protocol instance.

  @retval EFI_OUT_OF_RESOURCES   when a memory allocation error happened.
  @retval EFI_ACCESS_DENIED      when an RTC I/O operating is in progress.
  @retval EFI_TIMEOUT            when RTC device is not ready.
  @retval EFI_SUCCESS            on success.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_RTC_RAM_RESET_MEMORY) (
  IN  APPLE_RTC_RAM_PROTOCOL  *This
  );

/**
  Apple RTC protocol structure.
**/
struct APPLE_RTC_RAM_PROTOCOL_ {
  APPLE_RTC_RAM_GET_AVAILABLE_MEMORY  GetAvailableMemory;
  APPLE_RTC_RAM_READ_MEMORY           ReadMemory;
  APPLE_RTC_RAM_WRITE_MEMORY          WriteMemory;
  APPLE_RTC_RAM_RESET_MEMORY          ResetMemory;
};

extern EFI_GUID gAppleRtcRamProtocolGuid;

#endif // APPLE_RTC_RAM_PROTOCOL_H
