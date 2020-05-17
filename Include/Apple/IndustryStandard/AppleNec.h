/** @file
Copyright (C) 2014 - 2016, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_NEC_H
#define APPLE_NEC_H

// https://en.wikipedia.org/wiki/Apple_Remote#Technical_details

// APPLE_REMOTE_VENDOR
#define APPLE_REMOTE_VENDOR  0x043F

// APPLE_REMOTE_TABLE
enum {
  AppleRemoteCommandTable = 0x00,
  AppleRemoteKeyTable     = 0x0E
};

// APPLE_REMOTE_COMMAND
enum {
  AppleRemoteCommandPairing            = 0x01,
  AppleRemoteCommandFactoryDefaults    = 0x02,
  AppleRemoteCommandOriginalLowBattery = 0x03,
  AppleRemoteCommandCurrentLowBattery  = 0x07
};

// APPLE_REMOTE_COMMAND
typedef UINT16 APPLE_REMOTE_COMMAND;

// APPLE_REMOTE_KEY
enum {
  AppleRemoteKeyMenu         = 0x01,
  AppleRemoteKeyCenter       = 0x02,  ///< Play/Pause/Select on the old remote.
  AppleRemoteKeyRight        = 0x03,
  AppleRemoteKeyLeft         = 0x04,
  AppleRemoteKeyVolumeUp     = 0x05,
  AppleRemoteKeyVolumeDown   = 0x06,
  AppleRemoteKeyPlayUp       = 0x07,
  AppleRemoteKeyPlayDown     = 0x08,
  AppleRemoteKeyPlayNext     = 0x09,
  AppleRemoteKeyPlayPrevious = 0x0A,
  AppleRemoteKeyMenuUp       = 0x0B,
  AppleRemoteKeyMenuDown     = 0x0C,
  AppleRemoteKeyMenuPlay     = 0x0D,
  AppleRemoteKeyMenuNext     = 0x0E,
  AppleRemoteKeyMenuPrevious = 0x0F,
  AppleRemoteKeySelect       = 0x2E,  ///< Select on the new remote.
  AppleRemoteKeyPlay         = 0x2F   ///< Play/Pause on the new remote.
};

#pragma pack (1)

// APPLE_REMOTE_DATA_PACKAGE
typedef PACKED struct {
  /// This is always 0x43f and can be used to identify an Apple Remote
  UINT8  Vendor[11];

  /// 0x0 for the pairing and other commands, 0xe for the different buttons
  UINT8  CommandPage[5];

  /// A unique device ID, used to allow pairing of a remote to a specific
  /// device.  It can be changed with the pairing command.
  UINT64 DeviceId;

  /// Actual command for the Command Page
  UINT8  Command[7];

  /// All 32 bits added together have to equal 1
  UINT8  Checksum;
} APPLE_REMOTE_DATA_PACKAGE;

#pragma pack ()

#endif // APPLE_NEC_H
