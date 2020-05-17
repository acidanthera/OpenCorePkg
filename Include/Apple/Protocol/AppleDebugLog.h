/** @file
  Apple Debug Log protocol.

Copyright (C) 2020, vit9696.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DEBUG_LOG_H
#define APPLE_DEBUG_LOG_H

/**
  Apple Debug Log protocol GUID.
  DDFA34FB-FE1F-48EA-B213-FB4A4CD57BE3
**/
#define APPLE_DEBUG_LOG_PROTOCOL_GUID   \
  { 0xDDFA34FB, 0xFE1F, 0x48EA,         \
    { 0xB2, 0x13, 0xFB, 0x4A, 0x4C, 0xD5, 0x7B, 0xE3 } }

/**
  Current supported revision.
**/
#define APPLE_DEBUG_LOG_PROTOCOL_REVISION     0x10000

/**
  Maximum logfile size.
**/
#define APPLE_DEBUG_LOG_PROTOCOL_FILESIZE     BASE_2MB

/**
  Logfile name on EFI system partition (indices 1~8).
**/
#define APPLE_DEBUG_LOG_PROTOCOL_FILENAME     L"\\EFI\\APPLE\\LOG\\BOOT-%u.LOG"

/**
  Legacy boot.efi logfile.
**/
#define APPLE_DEBUG_LOG_PROTOCOL_BOOTLOG      L"\\BOOTLOG"

/**
  Legacy previous boot.efi logfile.
**/
#define APPLE_DEBUG_LOG_PROTOCOL_BOOTLOG_OLD  L"\\BOOTLOG.OLD"

/**
  Apple Debug Log protocol structure forward declaration.
**/
typedef struct APPLE_DEBUG_LOG_PROTOCOL_ APPLE_DEBUG_LOG_PROTOCOL;

/**
  Send debug message to the protocol.

  @param[in]  Message   ASCII message to log.

  @retval EFI_SUCCESS on success.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_DEBUG_LOG_PRINT) (
  IN CONST CHAR8               *Message
  );

/**
  Extract characters from the log buffer.

  @param[in,out]  Position      Starting position for extraction.
  @param[in,out]  BufferSize    Extraction buffer size.
  @param[out]     Buffer        Extraction buffer, optional.
  @param[out]     LostCharacters          Amount of characters that did not fit the buffer, optional.

  - Position is automatically updated to point to the end of the buffer if provided
    value is too large.
  - Position is automatically updated to point after extracted characters if Buffer is
    not NULL.

  @retval EFI_INVALID_PARAMETER  if Position or BufferSize are NULL.
  @retval EFI_SUCCESS            on successful extraction.
  @retval EFI_SUCCESS            on reported size in BufferSize if Buffer is NULL.
  @retval EFI_END_OF_FILE        on empty buffer.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_DEBUG_LOG_EXTRACT_BUFFER) (
  IN OUT UINT32            *Position,
  IN OUT UINTN             *BufferSize,
     OUT CHAR8             *Buffer          OPTIONAL,
     OUT UINT32            *LostCharacters  OPTIONAL
  );

/**
  Save debug log to 1st APPLE_DEBUG_LOG_PROTOCOL_FILENAME
  on logging partition. Saving debug log includes extracting
  all previously unsaved characters from the debug log buffer.

  On first call this function performs log rotation:
  - 8th APPLE_DEBUG_LOG_PROTOCOL_FILENAME is removed.
  - APPLE_DEBUG_LOG_PROTOCOL_FILENAMEs from 1st to 7th are renamed to 2nd to 8th.

  Truncated character amount is reflected in the log before writing to file.

  @retval EFI_SUCCESS      on success.
  @retval EFI_UNSUPPORTED  if current TPL is above TPL_CALLBACK.
  @retval EFI_NOT_FOUND    on write failure.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_DEBUG_LOG_WRITE_FILES) (
  VOID
  );

/**
  Performs a one-time operation of setting up an event for
  EfiSimpleFileSystem protocol installation and triggering
  its handler for all currently present file systems.

  Event handler is supposed to handle all ESP partitions:
  - Set modification time of APPLE_DEBUG_LOG_PROTOCOL_FILENAME
    to current if it is before 2000 year.
  - Remove APPLE_DEBUG_LOG_PROTOCOL_FILENAME created or modified
    more than a month ago.
  - Remove APPLE_DEBUG_LOG_PROTOCOL_BOOTLOG and
    APPLE_DEBUG_LOG_PROTOCOL_BOOTLOG_OLD files if present.
  - Update logging partition to current handled partition.
**/
typedef
VOID
(EFIAPI *APPLE_DEBUG_LOG_SETUP_FILES) (
  VOID
  );

/**
  Apple debug log protocol.
**/
struct APPLE_DEBUG_LOG_PROTOCOL_ {
  UINTN                           Revision;
  APPLE_DEBUG_LOG_PRINT           Print;
  APPLE_DEBUG_LOG_EXTRACT_BUFFER  ExtractBuffer;
  APPLE_DEBUG_LOG_WRITE_FILES     WriteFiles;
  APPLE_DEBUG_LOG_SETUP_FILES     SetupFiles;
};

extern EFI_GUID gAppleDebugLogProtocolGuid;

#endif // APPLE_DEBUG_LOG_H
