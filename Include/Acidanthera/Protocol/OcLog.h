/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_LOG_PROTOCOL_H
#define OC_LOG_PROTOCOL_H

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

///
/// Current supported log protocol revision.
///
#define OC_LOG_REVISION  0x01000A

///
/// The defines for the log flags.
///
#define OC_LOG_ENABLE       BIT0
#define OC_LOG_CONSOLE      BIT1
#define OC_LOG_DATA_HUB     BIT2
#define OC_LOG_SERIAL       BIT3
#define OC_LOG_VARIABLE     BIT4
#define OC_LOG_NONVOLATILE  BIT5
#define OC_LOG_FILE         BIT6
#define OC_LOG_ALL_BITS (\
  OC_LOG_ENABLE   | OC_LOG_CONSOLE     | \
  OC_LOG_DATA_HUB | OC_LOG_SERIAL      | \
  OC_LOG_VARIABLE | OC_LOG_NONVOLATILE | \
  OC_LOG_FILE)

typedef UINT32 OC_LOG_OPTIONS;

/**
  The GUID of the OC_LOG_PROTOCOL.
**/
#define OC_LOG_PROTOCOL_GUID      \
  { 0xDBB6008F, 0x89E4, 0x4272,   \
    { 0x98, 0x81, 0xCE, 0x3A, 0xFD, 0x97, 0x24, 0xD0 } }

/**
  The forward declaration for the protocol for the OC_LOG_PROTOCOL.
**/
typedef struct OC_LOG_PROTOCOL_ OC_LOG_PROTOCOL;

/**
  Add an entry to the log buffer

  @param[in] This          This protocol.
  @param[in] ErrorLevel    Debug level.
  @param[in] FormatString  String containing the output format.
  @param[in] Marker        Address of the VA_ARGS marker.

  @retval EFI_SUCCESS  The entry was successfully added.
**/
typedef
EFI_STATUS
(EFIAPI *OC_LOG_ADD_ENTRY) (
  IN OC_LOG_PROTOCOL  *This,
  IN UINTN            ErrorLevel,
  IN CONST CHAR8      *FormatString,
  IN VA_LIST          Marker
  );

/** 
  Reset the internal timers

  @param[in] This  This protocol.

  @retval EFI_SUCCESS  The timers were reset successfully.
**/
typedef
EFI_STATUS
(EFIAPI *OC_LOG_RESET_TIMERS) (
  IN OC_LOG_PROTOCOL  *This
  );

/**
  Retrieve pointer to the log buffer

  @param[in] This         This protocol.
  @param[in] OcLogBuffer  Address to store the buffer pointer.

  @retval EFI_SUCCESS  The timers were reset successfully.
**/
typedef
EFI_STATUS
(EFIAPI *OC_LOG_GET_LOG) (
  IN  OC_LOG_PROTOCOL  *This,
  OUT CHAR8            **OcLogBuffer
  );

/**
  Save the current log

  @param[in] This         This protocol.
  @param[in] NonVolatile  Variable.
  @param[in] FilePath     Filepath to save the log. OPTIONAL

  @retval EFI_SUCCESS  The log was saved successfully.
**/
typedef
EFI_STATUS
(EFIAPI *OC_LOG_SAVE_LOG) (
  IN OC_LOG_PROTOCOL           *This,
  IN UINT32                    NonVolatile OPTIONAL,
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath OPTIONAL
  );

/**
  The structure exposed by the OC_LOG_PROTOCOL.
**/
struct OC_LOG_PROTOCOL_ {
  UINT32                  Revision;     ///< The revision of the installed protocol.
  UINTN                   Reserved;     ///< Reserved for future extension.
  OC_LOG_ADD_ENTRY        AddEntry;     ///< A pointer to the AddEntry function.
  OC_LOG_GET_LOG          GetLog;       ///< A pointer to the GetLog function.
  OC_LOG_SAVE_LOG         SaveLog;      ///< A pointer to the SaveLog function.
  OC_LOG_RESET_TIMERS     ResetTimers;  ///< A pointer to the ResetTimers function.
  OC_LOG_OPTIONS          Options;      ///< The current options of the installed protocol.
  UINT32                  DisplayDelay; ///< The delay after visible onscreen message in microseconds.
  UINTN                   DisplayLevel; ///< The error level visible onscreen.
  UINTN                   HaltLevel;    ///< The error level causing CPU dead loop.
  EFI_FILE_PROTOCOL       *FileSystem;  ///< Log file system root, not owned.
  CHAR16                  *FilePath;    ///< Log file path.
};

/// A global variable storing the GUID of the OC_LOG_PROTOCOL.
extern EFI_GUID gOcLogProtocolGuid;

#endif // OC_LOG_PROTOCOL_H
