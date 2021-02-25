/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_MISC_LIB_H
#define OC_MISC_LIB_H

#include <Uefi.h>
#include <Library/OcStringLib.h>

/**
  The size, in Bits, of one Byte.
**/
#define OC_CHAR_BIT  8

/**
  Convert seconds to microseconds for use in e.g. gBS->Stall.
**/
#define SECONDS_TO_MICROSECONDS(x) ((x)*1000000)

BOOLEAN
FindPattern (
  IN CONST UINT8   *Pattern,
  IN CONST UINT8   *PatternMask OPTIONAL,
  IN CONST UINT32  PatternSize,
  IN CONST UINT8   *Data,
  IN UINT32        DataSize,
  IN UINT32        *DataOff
  );

UINT32
ApplyPatch (
  IN CONST UINT8   *Pattern,
  IN CONST UINT8   *PatternMask OPTIONAL,
  IN CONST UINT32  PatternSize,
  IN CONST UINT8   *Replace,
  IN CONST UINT8   *ReplaceMask OPTIONAL,
  IN UINT8         *Data,
  IN UINT32        DataSize,
  IN UINT32        Count,
  IN UINT32        Skip
  );

/**
  Obtain application arguments.

  @param[out]  Argc   Argument count.
  @param[out]  Argv   Argument list.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
GetArguments (
  OUT UINTN   *Argc,
  OUT CHAR16  ***Argv
  );

/**
  Uninstall all protocols with the specified GUID.

  @param[in] Protocol    The published unique identifier of the protocol. It is the caller's responsibility to pass in
                         a valid GUID.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcUninstallAllProtocolInstances (
  EFI_GUID  *Protocol
  );

/**
  Handle protocol on handle and fallback to any protocol when missing.

  @param[in]  Handle        Handle to search for protocol.
  @param[in]  Protocol      Protocol to search for.
  @param[out] Interface     Protocol interface if found.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcHandleProtocolFallback (
  IN  EFI_HANDLE  Handle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  );

/**
  Count instances found under a specific protocol.

  @param[in]  Protocol      Protocol to search for.

  @return     Number of instances found.
**/
UINTN
OcCountProtocolInstances (
  IN EFI_GUID  *Protocol
  );

/**
  Run and execute image file from buffer.

  @param[in]  DevicePath   Image device path, optional.
  @param[in]  Buffer       Buffer with image data, optional when DP is given.
  @param[in]  BufferSize   Image data size in the buffer.
  @param[out] ImageHandle  Loaded image handle for drivers, optional.
**/
EFI_STATUS
OcLoadAndRunImage (
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath  OPTIONAL,
  IN   VOID                      *Buffer      OPTIONAL,
  IN   UINTN                     BufferSize,
  OUT  EFI_HANDLE                *ImageHandle OPTIONAL
  );

/**
  Release UEFI ownership from USB controllers at booting.
**/
EFI_STATUS
ReleaseUsbOwnership (
  VOID
  );

/**
  Perform cold reboot directly bypassing UEFI services. Does not return.
  Supposed to work in any modern physical or virtual environment.
**/
VOID
DirectResetCold (
  VOID
  );

/**
  Internal worker macro that calls DebugPrint().

  This macro calls DebugPrint(), passing in the filename, line number, an
  expression that failed the comparison with expected value,
  the expected value and the actual value.

  @param  Expression  Integer expression that evaluated to value different from Value (should be convertible to INTN)
  @param  ExpectedValue  Expected value of the expression (should be convertible to INTN)

**/
#define _ASSERT_EQUALS(Expression, ExpectedValue)              \
  DebugPrint(                                          \
    DEBUG_ERROR,                                       \
    "ASSERT %a(%d): %a (expected: %d, actual: %d)\n",  \
    __FILE__,                                          \
    __LINE__,                                          \
    #Expression,                                       \
    (INTN)(ExpectedValue),                                     \
    (INTN)(Expression))

/**
  Macro that calls DebugAssert() if the value of an expression differs from the expected value.

  If MDEPKG_NDEBUG is not defined and the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED
  bit of PcdDebugProperyMask is set, then this macro evaluates the integer
  expression specified by Expression.  If the value of Expression differs from ExpectedValue, then
  DebugPrint() is called passing in the source filename, source line number,
  Expression, its value and ExpectedValue; then ASSERT(FALSE) is called to
  cause a breakpoint, deadloop or no-op depending on PcdDebugProperyMask.

  @param  Expression  Integer expression (should be convertible to INTN).
  @param  ExpectedValue  Expected value (should be convertible to INTN).

**/
#if !defined(MDEPKG_NDEBUG)
  #define ASSERT_EQUALS(Expression, ExpectedValue)    \
    do {                                      \
      if (DebugAssertEnabled ()) {            \
        if ((Expression) != (ExpectedValue)) {        \
          _ASSERT_EQUALS (Expression, ExpectedValue); \
          ASSERT(FALSE);                      \
          ANALYZER_UNREACHABLE ();            \
        }                                     \
      }                                       \
    } while (FALSE)
#else
  #define ASSERT_EQUALS(Expression, ExpectedValue)
#endif

#endif // OC_MISC_LIB_H
