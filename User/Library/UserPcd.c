/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <UserPcd.h>
#include <Library/DebugLib.h>

#define _PCD_VALUE_PcdUefiLibMaxPrintBufferSize         320U
#define _PCD_VALUE_PcdUgaConsumeSupport                 ((BOOLEAN)1U)
#define _PCD_VALUE_PcdDebugPropertyMask                 (\
  DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED \
  | DEBUG_PROPERTY_DEBUG_PRINT_ENABLED \
  | DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED )
#define _PCD_VALUE_PcdDebugRaisePropertyMask            DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED
#define _PCD_VALUE_PcdDebugClearMemoryValue             0xAFU
#define _PCD_VALUE_PcdFixedDebugPrintErrorLevel         0x80000002U
#define _PCD_VALUE_PcdDebugPrintErrorLevel              0x80000002U
#define _PCD_VALUE_PcdMaximumAsciiStringLength          0U
#define _PCD_VALUE_PcdMaximumUnicodeStringLength        1000000U
#define _PCD_VALUE_PcdMaximumLinkedListLength           1000000U
#define _PCD_VALUE_PcdVerifyNodeInList                  ((BOOLEAN)0U)
#define _PCD_VALUE_PcdCpuNumberOfReservedVariableMtrrs  0x2U
#define _PCD_VALUE_PcdMaximumDevicePathNodeCount        0U
#define _PCD_VALUE_PcdFatReadOnlyMode                   ((BOOLEAN)1U)

UINT32   _gPcd_FixedAtBuild_PcdUefiLibMaxPrintBufferSize             = _PCD_VALUE_PcdUefiLibMaxPrintBufferSize;
BOOLEAN  _gPcd_FixedAtBuild_PcdUgaConsumeSupport                     = _PCD_VALUE_PcdUgaConsumeSupport;
UINT8    _gPcd_FixedAtBuild_PcdDebugPropertyMask                     = _PCD_VALUE_PcdDebugPropertyMask;
UINT8    _gPcd_FixedAtBuild_PcdDebugRaisePropertyMask                = _PCD_VALUE_PcdDebugRaisePropertyMask;
UINT8    _gPcd_FixedAtBuild_PcdDebugClearMemoryValue                 = _PCD_VALUE_PcdDebugClearMemoryValue;
UINT32   _gPcd_FixedAtBuild_PcdFixedDebugPrintErrorLevel             = _PCD_VALUE_PcdFixedDebugPrintErrorLevel;
UINT32   _gPcd_FixedAtBuild_PcdDebugPrintErrorLevel                  = _PCD_VALUE_PcdDebugPrintErrorLevel;
UINT32   _gPcd_FixedAtBuild_PcdMaximumAsciiStringLength              = _PCD_VALUE_PcdMaximumAsciiStringLength;
UINT32   _gPcd_FixedAtBuild_PcdMaximumUnicodeStringLength            = _PCD_VALUE_PcdMaximumUnicodeStringLength;
UINT32   _gPcd_FixedAtBuild_PcdMaximumLinkedListLength               = _PCD_VALUE_PcdMaximumLinkedListLength;
BOOLEAN  _gPcd_FixedAtBuild_PcdVerifyNodeInList                      = _PCD_VALUE_PcdVerifyNodeInList;
UINT32   _gPcd_FixedAtBuild_PcdCpuNumberOfReservedVariableMtrrs      = _PCD_VALUE_PcdCpuNumberOfReservedVariableMtrrs;
UINT32   _gPcd_FixedAtBuild_PcdMaximumDevicePathNodeCount            = _PCD_VALUE_PcdMaximumDevicePathNodeCount;
BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderRtRelocAllowTargetMismatch = FALSE;
BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderHashProhibitOverlap        = TRUE;
BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderLoadHeader                 = TRUE;
BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderDebugSupport               = TRUE;
BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderAllowMisalignedOffset      = FALSE;
BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderRemoveXForWX               = FALSE;
BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderWXorX                      = TRUE;
UINT32   _gPcd_FixedAtBuild_PcdImageLoaderAlignmentPolicy            = 0xFFFFFFFF;
UINT32   _gPcd_FixedAtBuild_PcdImageLoaderRelocTypePolicy            = 0x00;
BOOLEAN  _gPcd_FeatureFlag_PcdFatReadOnlyMode                        = _PCD_VALUE_PcdFatReadOnlyMode;
UINT32   _gPcd_BinaryPatch_PcdSerialRegisterStride                   = 0;
UINT8    _gPcd_FixedAtBuild_PcdUefiImageFormatSupportNonFv           = 0x00;
UINT8    _gPcd_FixedAtBuild_PcdUefiImageFormatSupportFv              = 0x01;
