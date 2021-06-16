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
#include <Protocol/ApplePlatformInfoDatabase.h>

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
  Obtain protocol.
  If not obtained returns NULL, and optionally adds log message
  "[CallerName] cannot get protocol [ProtocolName] - %r".

  @param[in]  Protocol      Protocol to search for.
  @param[in]  ErrorLevel    The error level of the debug log message to print if protocol not found.
                            Send zero to generate no log message (caller becomes reponsible).
  @param[in]  CallerName    The caller name for the error message; should always be provided
                            if ErrorLevel is non-zero; will work, but with less useful log output,
                            if ommitted in that case.
  @param[in]  ProtocolName  The protocol name for the error message; optional, protocol GUID will
                            be used as protocol name in error message when required, otherwise.

  @return     Protocol instance, or NULL if not found.
**/
VOID *
OcGetProtocol (
  IN  EFI_GUID      *Protocol,
  IN  UINTN         ErrorLevel,
  IN  CONST CHAR8   *CallerName     OPTIONAL,
  IN  CONST CHAR8   *ProtocolName   OPTIONAL
  );

/**
  Run and execute image file from buffer.

  @param[in]  DevicePath    Image device path, optional.
  @param[in]  Buffer        Buffer with image data, optional when DP is given.
  @param[in]  BufferSize    Image data size in the buffer.
  @param[out] ImageHandle   Loaded image handle for drivers, optional.
  @param[out] OptionalData  Data that is passed to the loaded image, optional.
**/
EFI_STATUS
OcLoadAndRunImage (
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath   OPTIONAL,
  IN   VOID                      *Buffer       OPTIONAL,
  IN   UINTN                     BufferSize,
  OUT  EFI_HANDLE                *ImageHandle  OPTIONAL,
  IN   CHAR16                    *OptionalData OPTIONAL
  );

/**
  Read first data from Apple Platform Info protocol.

  @param[in]      PlatformInfo  Apple Platform Info protocol.
  @param[in]      DataGuid      Resource GUID identifier.
  @param[in,out]  Size          Maximum size allowed, updated to actual size on success.
  @param[out]     Data          Data read from Apple Platform Info protocol.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcReadApplePlatformFirstData (
  IN      APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *PlatformInfo,
  IN      EFI_GUID                               *DataGuid,
  IN OUT  UINT32                                 *Size,
     OUT  VOID                                   *Data
  );

/**
  Read first data from Apple Platform Info protocol allocating memory from pool.

  @param[in]   PlatformInfo  Apple Platform Info protocol.
  @param[in]   DataGuid      Resource GUID identifier.
  @param[out]  Size          Size of the entry.
  @param[out]  Data          Data read from Apple Platform Info protocol allocated from pool.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcReadApplePlatformFirstDataAlloc (
  IN   APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *PlatformInfo,
  IN   EFI_GUID                               *DataGuid,
  OUT  UINT32                                 *Size,
  OUT  VOID                                   **Data
  );

/**
  Read data from Apple Platform Info protocol.

  @param[in]      PlatformInfo  Apple Platform Info protocol.
  @param[in]      DataGuid      Resource GUID identifier.
  @param[in]      HobGuid       Hob GUID identifier.
  @param[in,out]  Size          Maximum size allowed, updated to actual size on success.
  @param[out]     Data          Data read from Apple Platform Info protocol.

  @retval EFI_SUCCESS on success.
**/
EFI_STATUS
OcReadApplePlatformData (
  IN      APPLE_PLATFORM_INFO_DATABASE_PROTOCOL  *PlatformInfo,
  IN      EFI_GUID                               *DataGuid,
  IN      EFI_GUID                               *HobGuid,
  IN OUT  UINT32                                 *Size,
     OUT  VOID                                   *Data
  );

/**
  Performs ConIn keyboard input flush.
**/
VOID
OcConsoleFlush (
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
