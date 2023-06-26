/** @file
  Copyright (c) 2020, PMheart. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef OC_USER_PCD_H
#define OC_USER_PCD_H

//
// Workaround RSIZE_MAX redeclaration in stdint.h and PrintLibInternal.c.
//
#ifdef __STDC_WANT_LIB_EXT1__
  #undef __STDC_WANT_LIB_EXT1__
#endif
#define __STDC_WANT_LIB_EXT1__  0
#include <stdint.h>

//
// Workaround for NULL redeclaration.
//
#include <stddef.h>
#ifdef NULL
  #undef NULL
#endif

//
// Workaround overrides to symbol visibility in ProcessorBind.h
// breaking linkage with C standard library with GCC/BFD.
//
#if defined (__GNUC__)
  #pragma GCC visibility push(default)
  #include <Base.h>
  #pragma GCC visibility pop
#endif

#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/OcCryptoLib.h>

extern UINT32   _gPcd_FixedAtBuild_PcdUefiLibMaxPrintBufferSize;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdUgaConsumeSupport;
extern UINT8    _gPcd_FixedAtBuild_PcdDebugPropertyMask;
extern UINT8    _gPcd_FixedAtBuild_PcdDebugRaisePropertyMask;
extern UINT8    _gPcd_FixedAtBuild_PcdDebugClearMemoryValue;
extern UINT32   _gPcd_FixedAtBuild_PcdFixedDebugPrintErrorLevel;
extern UINT32   _gPcd_FixedAtBuild_PcdDebugPrintErrorLevel;
extern UINT32   _gPcd_FixedAtBuild_PcdMaximumAsciiStringLength;
extern UINT32   _gPcd_FixedAtBuild_PcdMaximumUnicodeStringLength;
extern UINT32   _gPcd_FixedAtBuild_PcdMaximumLinkedListLength;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdVerifyNodeInList;
extern UINT32   _gPcd_FixedAtBuild_PcdCpuNumberOfReservedVariableMtrrs;
extern UINT32   _gPcd_FixedAtBuild_PcdMaximumDevicePathNodeCount;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderRtRelocAllowTargetMismatch;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderHashProhibitOverlap;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderLoadHeader;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderDebugSupport;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderAllowMisalignedOffset;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderRemoveXForWX;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderWXorX;
extern UINT32   _gPcd_FixedAtBuild_PcdImageLoaderAlignmentPolicy;
extern UINT32   _gPcd_FixedAtBuild_PcdImageLoaderRelocTypePolicy;
extern UINT8    _gPcd_FixedAtBuild_PcdUefiVariableDefaultLang[4];
extern UINT8    _gPcd_FixedAtBuild_PcdUefiVariableDefaultPlatformLang[6];
extern BOOLEAN  _gPcd_FeatureFlag_PcdFatReadOnlyMode;
extern UINT32   _gPcd_BinaryPatch_PcdSerialRegisterStride;
extern UINT8    _gPcd_FixedAtBuild_PcdUefiImageFormatSupportNonFv;
extern UINT8    _gPcd_FixedAtBuild_PcdUefiImageFormatSupportFv;

#define _PCD_GET_MODE_32_PcdUefiLibMaxPrintBufferSize   _gPcd_FixedAtBuild_PcdUefiLibMaxPrintBufferSize
#define _PCD_GET_MODE_BOOL_PcdUgaConsumeSupport         _gPcd_FixedAtBuild_PcdUgaConsumeSupport
#define _PCD_GET_MODE_8_PcdDebugPropertyMask            _gPcd_FixedAtBuild_PcdDebugPropertyMask
#define _PCD_GET_MODE_8_PcdDebugRaisePropertyMask       _gPcd_FixedAtBuild_PcdDebugRaisePropertyMask
#define _PCD_GET_MODE_8_PcdDebugClearMemoryValue        _gPcd_FixedAtBuild_PcdDebugClearMemoryValue
#define _PCD_GET_MODE_32_PcdFixedDebugPrintErrorLevel   _gPcd_FixedAtBuild_PcdFixedDebugPrintErrorLevel
#define _PCD_GET_MODE_32_PcdDebugPrintErrorLevel        _gPcd_FixedAtBuild_PcdDebugPrintErrorLevel
#define _PCD_GET_MODE_32_PcdMaximumAsciiStringLength    _gPcd_FixedAtBuild_PcdMaximumAsciiStringLength
#define _PCD_GET_MODE_32_PcdMaximumUnicodeStringLength  _gPcd_FixedAtBuild_PcdMaximumUnicodeStringLength
#define _PCD_GET_MODE_32_PcdMaximumLinkedListLength     _gPcd_FixedAtBuild_PcdMaximumLinkedListLength
#define _PCD_GET_MODE_BOOL_PcdVerifyNodeInList          _gPcd_FixedAtBuild_PcdVerifyNodeInList
#define _PCD_GET_MODE_16_PcdOcCryptoAllowedRsaModuli    (512U | 256U)
#define _PCD_GET_MODE_16_PcdOcCryptoAllowedSigHashTypes  \
  ((1U << OcSigHashTypeSha256) | (1U << OcSigHashTypeSha384) | (1U << OcSigHashTypeSha512))
#define _PCD_GET_MODE_32_PcdCpuNumberOfReservedVariableMtrrs  _gPcd_FixedAtBuild_PcdCpuNumberOfReservedVariableMtrrs
#define _PCD_GET_MODE_PTR_PcdUefiVariableDefaultLang          _gPcd_FixedAtBuild_PcdUefiVariableDefaultLang
#define _PCD_GET_MODE_PTR_PcdUefiVariableDefaultPlatformLang  _gPcd_FixedAtBuild_PcdUefiVariableDefaultPlatformLang
#define _PCD_GET_MODE_BOOL_PcdValidateOrderedCollection       ((BOOLEAN)0U)
#define _PCD_GET_MODE_BOOL_PcdFatReadOnlyMode                 _gPcd_FeatureFlag_PcdFatReadOnlyMode
#define _PCD_GET_MODE_32_PcdSerialRegisterStride              _gPcd_BinaryPatch_PcdSerialRegisterStride
//
// This will not be of any effect at userspace.
//
#define _PCD_GET_MODE_64_PcdPciExpressBaseAddress                    0
#define _PCD_GET_MODE_64_PcdPciExpressBaseSize                       0
#define _PCD_GET_MODE_32_PcdMaximumDevicePathNodeCount               _gPcd_FixedAtBuild_PcdMaximumDevicePathNodeCount
#define _PCD_GET_MODE_BOOL_PcdImageLoaderRtRelocAllowTargetMismatch  _gPcd_FixedAtBuild_PcdImageLoaderRtRelocAllowTargetMismatch
#define _PCD_GET_MODE_BOOL_PcdImageLoaderHashProhibitOverlap         _gPcd_FixedAtBuild_PcdImageLoaderHashProhibitOverlap
#define _PCD_GET_MODE_BOOL_PcdImageLoaderLoadHeader                  _gPcd_FixedAtBuild_PcdImageLoaderLoadHeader
#define _PCD_GET_MODE_BOOL_PcdImageLoaderDebugSupport                _gPcd_FixedAtBuild_PcdImageLoaderDebugSupport
#define _PCD_GET_MODE_BOOL_PcdImageLoaderAllowMisalignedOffset       _gPcd_FixedAtBuild_PcdImageLoaderAllowMisalignedOffset
#define _PCD_GET_MODE_BOOL_PcdImageLoaderRemoveXForWX                _gPcd_FixedAtBuild_PcdImageLoaderRemoveXForWX
#define _PCD_GET_MODE_BOOL_PcdImageLoaderWXorX                       _gPcd_FixedAtBuild_PcdImageLoaderWXorX
#define _PCD_GET_MODE_32_PcdImageLoaderAlignmentPolicy               _gPcd_FixedAtBuild_PcdImageLoaderAlignmentPolicy
#define _PCD_GET_MODE_32_PcdImageLoaderRelocTypePolicy               _gPcd_FixedAtBuild_PcdImageLoaderRelocTypePolicy
#define _PCD_GET_MODE_8_PcdUefiImageFormatSupportNonFv               _gPcd_FixedAtBuild_PcdUefiImageFormatSupportNonFv
#define _PCD_GET_MODE_8_PcdUefiImageFormatSupportFv                  _gPcd_FixedAtBuild_PcdUefiImageFormatSupportFv

#endif // OC_USER_PCD_H
