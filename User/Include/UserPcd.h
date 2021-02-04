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
#define __STDC_WANT_LIB_EXT1__ 0
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
#if defined(__GNUC__)
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
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderSupportArmThumb;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderForceLoadDebug;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderTolerantLoad;
extern BOOLEAN  _gPcd_FixedAtBuild_PcdImageLoaderSupportDebug;

#define _PCD_GET_MODE_32_PcdUefiLibMaxPrintBufferSize         _gPcd_FixedAtBuild_PcdUefiLibMaxPrintBufferSize
#define _PCD_GET_MODE_BOOL_PcdUgaConsumeSupport               _gPcd_FixedAtBuild_PcdUgaConsumeSupport
#define _PCD_GET_MODE_8_PcdDebugPropertyMask                  _gPcd_FixedAtBuild_PcdDebugPropertyMask
#define _PCD_GET_MODE_8_PcdDebugClearMemoryValue              _gPcd_FixedAtBuild_PcdDebugClearMemoryValue
#define _PCD_GET_MODE_32_PcdFixedDebugPrintErrorLevel         _gPcd_FixedAtBuild_PcdFixedDebugPrintErrorLevel
#define _PCD_GET_MODE_32_PcdDebugPrintErrorLevel              _gPcd_FixedAtBuild_PcdDebugPrintErrorLevel
#define _PCD_GET_MODE_32_PcdMaximumAsciiStringLength          _gPcd_FixedAtBuild_PcdMaximumAsciiStringLength
#define _PCD_GET_MODE_32_PcdMaximumUnicodeStringLength        _gPcd_FixedAtBuild_PcdMaximumUnicodeStringLength
#define _PCD_GET_MODE_32_PcdMaximumLinkedListLength           _gPcd_FixedAtBuild_PcdMaximumLinkedListLength
#define _PCD_GET_MODE_BOOL_PcdVerifyNodeInList                _gPcd_FixedAtBuild_PcdVerifyNodeInList
#define _PCD_GET_MODE_16_PcdOcCryptoAllowedRsaModuli          (512U | 256U)
#define _PCD_GET_MODE_16_PcdOcCryptoAllowedSigHashTypes  \
  ((1U << OcSigHashTypeSha256) | (1U << OcSigHashTypeSha384) | (1U << OcSigHashTypeSha512))
#define _PCD_GET_MODE_32_PcdCpuNumberOfReservedVariableMtrrs  _gPcd_FixedAtBuild_PcdCpuNumberOfReservedVariableMtrrs
//
// This will not be of any effect at userspace.
//
#define _PCD_GET_MODE_64_PcdPciExpressBaseAddress             0
#define _PCD_GET_MODE_64_PcdPciExpressBaseSize                0
#define _PCD_GET_MODE_32_PcdMaximumDevicePathNodeCount        _gPcd_FixedAtBuild_PcdMaximumDevicePathNodeCount
#define _PCD_GET_MODE_BOOL_PcdImageLoaderRtRelocAllowTargetMismatch _gPcd_FixedAtBuild_PcdImageLoaderRtRelocAllowTargetMismatch
#define _PCD_GET_MODE_BOOL_PcdImageLoaderHashProhibitOverlap        _gPcd_FixedAtBuild_PcdImageLoaderHashProhibitOverlap
#define _PCD_GET_MODE_BOOL_PcdImageLoaderLoadHeader                 _gPcd_FixedAtBuild_PcdImageLoaderLoadHeader
#define _PCD_GET_MODE_BOOL_PcdImageLoaderSupportArmThumb            _gPcd_FixedAtBuild_PcdImageLoaderSupportArmThumb
#define _PCD_GET_MODE_BOOL_PcdImageLoaderForceLoadDebug             _gPcd_FixedAtBuild_PcdImageLoaderForceLoadDebug
#define _PCD_GET_MODE_BOOL_PcdImageLoaderTolerantLoad               _gPcd_FixedAtBuild_PcdImageLoaderTolerantLoad
#define _PCD_GET_MODE_BOOL_PcdImageLoaderSupportDebug               _gPcd_FixedAtBuild_PcdImageLoaderSupportDebug

#endif // OC_USER_PCD_H
