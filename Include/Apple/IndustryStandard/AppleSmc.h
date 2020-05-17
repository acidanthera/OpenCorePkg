/** @file
Copyright (C) 2014 - 2016, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef APPLE_SMC_H
#define APPLE_SMC_H

//
// SMC uses Big Endian byte order to store keys.
// For some reason AppleSmcIo protocol in UEFI takes little endian keys.
// As this header is used by both UEFI and Kernel VirtualSMC parts,
// we define SMC_MAKE_IDENTIFIER to produce Little Endian keys in UEFI (EFIAPI),
// and Big Endian keys in all other places.
//
// NB: This code assumes Little Endian host byte order, which so far is the
// only supported byte order in UEFI.
//
#ifdef EFIAPI
#define SMC_MAKE_IDENTIFIER(A, B, C, D)  \
  ((UINT32)(((UINT32)(A) << 24U) | ((UINT32)(B) << 16U) | ((UINT32)(C) << 8U) | (UINT32)(D)))
#else
#define SMC_MAKE_IDENTIFIER(A, B, C, D)  \
  ((UINT32)(((UINT32)(D) << 24U) | ((UINT32)(C) << 16U) | ((UINT32)(B) << 8U) | (UINT32)(A)))
#endif

// PMIO

#define SMC_PORT_BASE            0x0300
#define SMC_PORT_LENGTH          0x0020

#define SMC_PORT_OFFSET_DATA     0x00
#define SMC_PORT_OFFSET_COMMAND  0x04
#define SMC_PORT_OFFSET_STATUS   SMC_PORT_OFFSET_COMMAND
#define SMC_PORT_OFFSET_RESULT   0x1E
#define SMC_PORT_OFFSET_EVENT    0x1F

// MMIO

#define SMC_MMIO_BASE_ADDRESS  0xFEF00000
#define SMC_MMIO_LENGTH        0x00010000

#define SMC_MMIO_DATA_VARIABLE  0x00
#define SMC_MMIO_DATA_FIXED     0x78

// MMIO offsets

#define SMC_MMIO_OFFSET_KEY             0x00
#define SMC_MMIO_OFFSET_KEY_TYPE        SMC_MMIO_OFFSET_KEY
#define SMC_MMIO_OFFSET_SMC_MODE        SMC_MMIO_OFFSET_KEY
#define SMC_MMIO_OFFSET_DATA_SIZE       0x05
#define SMC_MMIO_OFFSET_KEY_ATTRIBUTES  0x06
#define SMC_MMIO_OFFSET_COMMAND         0x07
#define SMC_MMIO_OFFSET_RESULT          SMC_MMIO_OFFSET_COMMAND
#define SMC_MMIO_OFFSET_LOG             0x08

// Read addresses

#define SMC_MMIO_READ_KEY  \
  (SMC_MMIO_DATA_VARIABLE + SMC_MMIO_OFFSET_KEY)

#define SMC_MMIO_READ_KEY_TYPE  \
  (SMC_MMIO_DATA_VARIABLE + SMC_MMIO_OFFSET_KEY_TYPE)

#define SMC_MMIO_READ_DATA_SIZE  \
  (SMC_MMIO_DATA_VARIABLE + SMC_MMIO_OFFSET_DATA_SIZE)

#define SMC_MMIO_READ_KEY_ATTRIBUTES  \
  (SMC_MMIO_DATA_VARIABLE + SMC_MMIO_OFFSET_KEY_ATTRIBUTES)

#define SMC_MMIO_READ_LOG  \
  (SMC_MMIO_DATA_FIXED + SMC_MMIO_OFFSET_LOG)

#define SMC_MMIO_READ_RESULT  \
  (SMC_MMIO_DATA_FIXED + SMC_MMIO_OFFSET_RESULT)

#define SMC_MMIO_READ_EVENT_STATUS 0x4000
#define SMC_MMIO_READ_UNKNOWN1     0x4004
#define SMC_MMIO_READ_KEY_STATUS   0x4005

// Write addresses

#define SMC_MMIO_WRITE_MODE  \
  (SMC_MMIO_DATA_VARIABLE + SMC_MMIO_OFFSET_SMC_MODE)

#define SMC_MMIO_WRITE_KEY  \
  (SMC_MMIO_DATA_FIXED + SMC_MMIO_OFFSET_KEY)

#define SMC_MMIO_WRITE_INDEX  \
  (SMC_MMIO_DATA_FIXED + SMC_MMIO_OFFSET_KEY_TYPE)

#define SMC_MMIO_WRITE_DATA_SIZE  \
  (SMC_MMIO_DATA_FIXED + SMC_MMIO_OFFSET_DATA_SIZE)

#define SMC_MMIO_WRITE_KEY_ATTRIBUTES  \
  (SMC_MMIO_DATA_FIXED + SMC_MMIO_OFFSET_KEY_ATTRIBUTES)

#define SMC_MMIO_WRITE_COMMAND  \
  (SMC_MMIO_DATA_FIXED + SMC_MMIO_OFFSET_COMMAND)

typedef UINT32 SMC_ADDRESS;

// Modes

#define SMC_MODE_APPCODE  'A'
#define SMC_MODE_UPDATE   'U'
#define SMC_MODE_BASE     'B'

// SMC_MODE
typedef CHAR8 *SMC_MODE;

enum {
  SmcResetModeMaster  = 0,
  SmcResetModeAppCode = 1,
  SmcResetModeUpdate  = 2,
  SmcResetModeBase    = 3
};

typedef UINT8 SMC_RESET_MODE;

enum {
  SmcFlashTypeAppCode = 1,
  SmcFlashTypeBase    = 2,
  SmcFlashTypeUpdate  = 3,
  SmcFlashTypeEpm     = 4
};

typedef UINT8 SMC_FLASH_TYPE;

enum {
  SmcFlashModeAppCode = SmcResetModeMaster,
  SmcFlashModeUpdate  = SmcResetModeBase,
  SmcFlashModeBase    = SmcResetModeUpdate,
  SmcFlashModeEpm     = SmcResetModeMaster
};

typedef UINT8 SMC_FLASH_MODE;

// Commands

enum {
  SmcCmdReadValue            = 0x10,
  SmcCmdWriteValue           = 0x11,
  SmcCmdGetKeyFromIndex      = 0x12,
  SmcCmdGetKeyInfo           = 0x13,
  SmcCmdReset                = 0x14,
  SmcCmdWriteValueAtIndex    = 0x15,
  SmcCmdReadValueAtIndex     = 0x16,
  SmcCmdGetSramAddress       = 0x17,
  SmcCmdReadPKey             = 0x20, // response based on payload submitted
  SmcCmdUnknown1             = 0x77,
  SmcCmdFlashWrite           = 0xF1,
  SmcCmdFlashAuth            = 0xF2,
  SmcCmdFlashType            = 0xF4,
  SmcCmdFlashWriteMoreData   = 0xF5, // write more data than available at once
  SmcCmdFlashAuthMoreData    = 0xF6  // auth more data than available at once
};

typedef UINT8 SMC_COMMAND;

// Reports

#define SMC_STATUS_AWAITING_DATA  BIT0  ///< Ready to read data.
#define SMC_STATUS_IB_CLOSED      BIT1  /// A write is pending.
#define SMC_STATUS_BUSY           BIT2  ///< Busy processing a command.
#define SMC_STATUS_GOT_COMMAND    BIT3  ///< The last input was a command.
#define SMC_STATUS_UKN_0x16       BIT4
#define SMC_STATUS_KEY_DONE       BIT5
#define SMC_STATUS_READY          BIT6  // Ready to work
#define SMC_STATUS_UKN_0x80       BIT7  // error

// SMC_STATUS
typedef UINT8 SMC_STATUS;

enum {
  SmcSuccess               = 0,
  SmcError                 = 1,

  SmcCommCollision         = 128,
  SmcSpuriousData          = 129,
  SmcBadCommand            = 130,
  SmcBadParameter          = 131,
  SmcNotFound              = 132,
  SmcNotReadable           = 133,
  SmcNotWritable           = 134,
  SmcKeySizeMismatch       = 135,
  SmcFramingError          = 136,
  SmcBadArgumentError      = 137,

  SmcTimeoutError          = 183,
  SmcKeyIndexRangeError    = 184,

  SmcBadFunctionParameter  = 192,
  SmcEventBufferWrongOrder = 196,
  SmcEventBufferReadError  = 197,
  SmcDeviceAccessError     = 199,
  SmcUnsupportedFeature    = 203,
  SmcSmbAccessError        = 204,

  SmcInvalidSize           = 206
};

#define SMC_ERROR(a) (((UINTN)(a)) > 0)

#define EFI_STATUS_FROM_SMC_RESULT(x)  \
  ((((UINTN)(x)) == SmcSuccess) ? EFI_SUCCESS : EFIERR ((UINTN)(x)))

#define EFI_SMC_SUCCESS                   SmcSuccess
#define EFI_SMC_ERROR                     EFIERR (SmcError)

#define EFI_SMC_COMM_COLLISION            EFIERR (SmcCommCollision)
#define EFI_SMC_SPURIOUS_DATA             EFIERR (SmcSpuriousData)
#define EFI_SMC_BAD_COMMAND               EFIERR (SmcBadCommand)
#define EFI_SMC_BAD_PARAMETER             EFIERR (SmcBadParameter)
#define EFI_SMC_NOT_FOUND                 EFIERR (SmcNotFound)
#define EFI_SMC_NOT_READABLE              EFIERR (SmcNotReadable)
#define EFI_SMC_NOT_WRITABLE              EFIERR (SmcNotWritable)
#define EFI_SMC_KEY_MISMATCH              EFIERR (SmcKeySizeMismatch)
#define EFI_SMC_FRAMING_ERROR             EFIERR (SmcFramingError)
#define EFI_SMC_BAD_ARGUMENT_ERROR        EFIERR (SmcBadArgumentError)

#define EFI_SMC_TIMEOUT_ERROR             EFIERR (SmcTimeoutError)
#define EFI_SMC_KEY_INDEX_RANGE_ERROR     EFIERR (SmcKeyIndexRangeError)

#define EFI_SMC_BAD_FUNCTION_PARAMETER    EFIERR (SmcBadFunctionParameter)
#define EFI_SMC_EVENT_BUFFER_WRONG_ORDER  EFIERR (SmcEventBufferWrongOrder)
#define EFI_SMC_EVENT_BUFFER_READ_ERROR   EFIERR (SmcEventBufferReadError)
#define EFI_SMC_DEVICE_ACCESS_ERROR       EFIERR (SmcDeviceAccessError)
#define EFI_SMC_UNSUPPORTED_FEATURE       EFIERR (SmcUnsupportedFeature)
#define EFI_SMB_ACCESS_ERROR              EFIERR (SmcSmbAccessError)

#define EFI_SMC_INVALID_SIZE              EFIERR (SmcInvalidSize)

// SMC_RESULT
typedef UINT8 SMC_RESULT;

// Key Types

#define SMC_MAKE_KEY_TYPE(A, B, C, D) SMC_MAKE_IDENTIFIER ((A), (B), (C), (D))

enum {
  SmcKeyTypeCh8s   = SMC_MAKE_KEY_TYPE ('c', 'h', '8', '*'),
  SmcKeyTypeChar   = SMC_MAKE_KEY_TYPE ('c', 'h', 'a', 'r'),
  SmcKeyTypeFloat  = SMC_MAKE_KEY_TYPE ('f', 'l', 't', ' '),
  SmcKeyTypeFlag   = SMC_MAKE_KEY_TYPE ('f', 'l', 'a', 'g'),
  SmcKeyTypeFp1f   = SMC_MAKE_KEY_TYPE ('f', 'p', '1', 'f'),
  SmcKeyTypeFp2e   = SMC_MAKE_KEY_TYPE ('f', 'p', '2', 'e'),
  SmcKeyTypeFp3d   = SMC_MAKE_KEY_TYPE ('f', 'p', '3', 'd'),
  SmcKeyTypeFp4c   = SMC_MAKE_KEY_TYPE ('f', 'p', '4', 'c'),
  SmcKeyTypeFp5b   = SMC_MAKE_KEY_TYPE ('f', 'p', '5', 'b'),
  SmcKeyTypeFp6a   = SMC_MAKE_KEY_TYPE ('f', 'p', '6', 'a'),
  SmcKeyTypeFp79   = SMC_MAKE_KEY_TYPE ('f', 'p', '7', '9'),
  SmcKeyTypeFp88   = SMC_MAKE_KEY_TYPE ('f', 'p', '8', '8'),
  SmcKeyTypeFp97   = SMC_MAKE_KEY_TYPE ('f', 'p', '9', '7'),
  SmcKeyTypeFpa6   = SMC_MAKE_KEY_TYPE ('f', 'p', 'a', '6'),
  SmcKeyTypeFpb5   = SMC_MAKE_KEY_TYPE ('f', 'p', 'b', '5'),
  SmcKeyTypeFpc4   = SMC_MAKE_KEY_TYPE ('f', 'p', 'c', '4'),
  SmcKeyTypeFpd3   = SMC_MAKE_KEY_TYPE ('f', 'p', 'd', '3'),
  SmcKeyTypeFpe2   = SMC_MAKE_KEY_TYPE ('f', 'p', 'e', '2'),
  SmcKeyTypeFpf1   = SMC_MAKE_KEY_TYPE ('f', 'p', 'f', '1'),
  SmcKeyTypeHex    = SMC_MAKE_KEY_TYPE ('h', 'e', 'x', '_'),
  SmcKeyTypeIoft   = SMC_MAKE_KEY_TYPE ('i', 'o', 'f', 't'),
  SmcKeyTypeSint8  = SMC_MAKE_KEY_TYPE ('s', 'i', '8', ' '),
  SmcKeyTypeSint16 = SMC_MAKE_KEY_TYPE ('s', 'i', '1', '6'),
  SmcKeyTypeSint32 = SMC_MAKE_KEY_TYPE ('s', 'i', '3', '2'),
  SmcKeyTypeSint64 = SMC_MAKE_KEY_TYPE ('s', 'i', '6', '4'),
  SmcKeyTypeSp1e   = SMC_MAKE_KEY_TYPE ('s', 'p', '1', 'e'),
  SmcKeyTypeSp2d   = SMC_MAKE_KEY_TYPE ('s', 'p', '2', 'd'),
  SmcKeyTypeSp3c   = SMC_MAKE_KEY_TYPE ('s', 'p', '3', 'c'),
  SmcKeyTypeSp4b   = SMC_MAKE_KEY_TYPE ('s', 'p', '4', 'b'),
  SmcKeyTypeSp5a   = SMC_MAKE_KEY_TYPE ('s', 'p', '5', 'a'),
  SmcKeyTypeSp69   = SMC_MAKE_KEY_TYPE ('s', 'p', '6', '9'),
  SmcKeyTypeSp78   = SMC_MAKE_KEY_TYPE ('s', 'p', '7', '8'),
  SmcKeyTypeSp87   = SMC_MAKE_KEY_TYPE ('s', 'p', '8', '7'),
  SmcKeyTypeSp96   = SMC_MAKE_KEY_TYPE ('s', 'p', '9', '6'),
  SmcKeyTypeSpa5   = SMC_MAKE_KEY_TYPE ('s', 'p', 'a', '5'),
  SmcKeyTypeSpb4   = SMC_MAKE_KEY_TYPE ('s', 'p', 'b', '4'),
  SmcKeyTypeSpc3   = SMC_MAKE_KEY_TYPE ('s', 'p', 'c', '3'),
  SmcKeyTypeSpd2   = SMC_MAKE_KEY_TYPE ('s', 'p', 'd', '2'),
  SmcKeyTypeSpe1   = SMC_MAKE_KEY_TYPE ('s', 'p', 'e', '1'),
  SmcKeyTypeSpf0   = SMC_MAKE_KEY_TYPE ('s', 'p', 'f', '0'),
  SmcKeyTypeUint8z = SMC_MAKE_KEY_TYPE ('u', 'i', '8', '\0'),
  SmcKeyTypeUint8  = SMC_MAKE_KEY_TYPE ('u', 'i', '8', ' '),
  SmcKeyTypeUint8s = SMC_MAKE_KEY_TYPE ('u', 'i', '8', '*'),
  SmcKeyTypeUint16 = SMC_MAKE_KEY_TYPE ('u', 'i', '1', '6'),
  SmcKeyTypeUint32 = SMC_MAKE_KEY_TYPE ('u', 'i', '3', '2'),
  SmcKeyTypeUint64 = SMC_MAKE_KEY_TYPE ('u', 'i', '6', '4'),
  SmcKeyTypeAla    = SMC_MAKE_KEY_TYPE ('{', 'a', 'l', 'a'),
  SmcKeyTypeAlc    = SMC_MAKE_KEY_TYPE ('{', 'a', 'l', 'c'),
  SmcKeyTypeAli    = SMC_MAKE_KEY_TYPE ('{', 'a', 'l', 'i'),
  SmcKeyTypeAlp    = SMC_MAKE_KEY_TYPE ('{', 'a', 'l', 'p'),
  SmcKeyTypeAlr    = SMC_MAKE_KEY_TYPE ('{', 'a', 'l', 'r'),
  SmcKeyTypeAlt    = SMC_MAKE_KEY_TYPE ('{', 'a', 'l', 't'),
  SmcKeyTypeAlv    = SMC_MAKE_KEY_TYPE ('{', 'a', 'l', 'v'),
  SmcKeyTypeClc    = SMC_MAKE_KEY_TYPE ('{', 'c', 'l', 'c'),
  SmcKeyTypeClh    = SMC_MAKE_KEY_TYPE ('{', 'c', 'l', 'h'),
  SmcKeyTypeFds    = SMC_MAKE_KEY_TYPE ('{', 'f', 'd', 's'),
  SmcKeyTypeHdi    = SMC_MAKE_KEY_TYPE ('{', 'h', 'd', 'i'),
  SmcKeyTypeJst    = SMC_MAKE_KEY_TYPE ('{', 'j', 's', 't'),
  SmcKeyTypeLia    = SMC_MAKE_KEY_TYPE ('{', 'l', 'i', 'a'),
  SmcKeyTypeLic    = SMC_MAKE_KEY_TYPE ('{', 'l', 'i', 'c'),
  SmcKeyTypeLim    = SMC_MAKE_KEY_TYPE ('{', 'l', 'i', 'm'),
  SmcKeyTypeLkb    = SMC_MAKE_KEY_TYPE ('{', 'l', 'k', 'b'),
  SmcKeyTypeLks    = SMC_MAKE_KEY_TYPE ('{', 'l', 'k', 's'),
  SmcKeyTypeLsc    = SMC_MAKE_KEY_TYPE ('{', 'l', 's', 'c'),
  SmcKeyTypeLsd    = SMC_MAKE_KEY_TYPE ('{', 'l', 's', 'd'),
  SmcKeyTypeLsf    = SMC_MAKE_KEY_TYPE ('{', 'l', 's', 'f'),
  SmcKeyTypeLso    = SMC_MAKE_KEY_TYPE ('{', 'l', 's', 'o'),
  SmcKeyTypeMss    = SMC_MAKE_KEY_TYPE ('{', 'm', 's', 's'),
  SmcKeyTypePwm    = SMC_MAKE_KEY_TYPE ('{', 'p', 'w', 'm'),
  SmcKeyTypeRev    = SMC_MAKE_KEY_TYPE ('{', 'r', 'e', 'v')
};

// SMC_KEY_TYPE
typedef UINT32 SMC_KEY_TYPE;

// Key Attributes

#define SMC_KEY_ATTRIBUTE_PRIVATE_WRITE   BIT0
#define SMC_KEY_ATTRIBUTE_PRIVATE_READ    BIT1
#define SMC_KEY_ATTRIBUTE_ATOMIC          BIT2
#define SMC_KEY_ATTRIBUTE_CONST           BIT3
#define SMC_KEY_ATTRIBUTE_FUNCTION        BIT4
#define SMC_KEY_ATTRIBUTE_UKN_0x20        BIT5
#define SMC_KEY_ATTRIBUTE_WRITE           BIT6
#define SMC_KEY_ATTRIBUTE_READ            BIT7

// SMC_KEY_ATTRIBUTES
typedef UINT8 SMC_KEY_ATTRIBUTES;


// Data

#define SMC_MAX_DATA_SIZE  (SMC_MMIO_DATA_FIXED - SMC_MMIO_DATA_VARIABLE)

typedef UINT8 SMC_DATA;
typedef UINT8 SMC_DATA_SIZE;

// Keys

// SMC_KEY_IS_VALID_CHAR
#define SMC_KEY_IS_VALID_CHAR(x) (((x) >= 0x20) && ((x) <= 0x7E))

// SMC_MAKE_KEY
#define SMC_MAKE_KEY(A, B, C, D) SMC_MAKE_IDENTIFIER ((A), (B), (C), (D))

#define SMC_KEY_NUM      SMC_MAKE_KEY ('$', 'N', 'u', 'm')
#define SMC_KEY_ADR      SMC_MAKE_KEY ('$', 'A', 'd', 'r')
#define SMC_KEY_LDKN     SMC_MAKE_KEY ('L', 'D', 'K', 'N')
#define SMC_KEY_HBKP     SMC_MAKE_KEY ('H', 'B', 'K', 'P')
#define SMC_KEY_KEY      SMC_MAKE_KEY ('#', 'K', 'E', 'Y')
#define SMC_KEY_RMde     SMC_MAKE_KEY ('R', 'M', 'd', 'e')
#define SMC_KEY_BRSC     SMC_MAKE_KEY ('B', 'R', 'S', 'C')
#define SMC_KEY_MSLD     SMC_MAKE_KEY ('M', 'S', 'L', 'D')
#define SMC_KEY_BATP     SMC_MAKE_KEY ('B', 'A', 'T', 'P')
#define SMC_KEY_BBIN     SMC_MAKE_KEY ('B', 'B', 'I', 'N')

typedef UINT32 SMC_KEY;
typedef UINT32 SMC_KEY_INDEX;

typedef UINT8 SMC_DEVICE_INDEX;

// Flash data

// SMC_FLASH_SIZE_MAX
#define SMC_FLASH_SIZE_MAX  0x0800

// SMC_FLASH_SIZE
typedef UINT16 SMC_FLASH_SIZE;

// Events

enum {
  SmcEventALSChange             = 0x2A,
  SmcEventShutdownImminent      = 0x40,
  SmcEventBridgeOSPanic         = 0x41,
  SmcEventLogMessage            = 0x4C,
  SmcEventKeyDone               = 0x4B,
  SmcEventPThermalLevelChanged  = 0x54,
  SmcEventCallPlatformFunction  = 0x55,
  SmcEventSMSDrop               = 0x60,
  SmcEventUnknown6A             = 0x6A, // Bug??
  SmcEventSMSOrientation        = 0x69,
  SmcEventSMSShock              = 0x6F,
  SmcEventSystemStateNotify     = 0x70,
  SmcEventPowerStateNotify      = 0x71,
  SmcEventHidEventNotify        = 0x72,
  SmcEventPLimitChange          = 0x80,
  SmcEventPCIeReady             = 0x83, // Not certain
};

// SmcEventSystemStateNotify subtypes, not always certain
// Mostly from bridgeOS kernelcache and ramrod.
// Check SMCRegisterForSubTypeNotification in libSMC.dylib.
enum {
  SmcSystemStateNotifyMacOsPanicCause            = 4,  // Name unclear
  SmcSystemStateNotifyPrepareForS0               = 6,
  SmcSystemStateNotifyMacOsPanicDone             = 10,
  SmcSystemStateNotifyRestart                    = 15,
  SmcSystemStateNotifyMacEfiFirmwareUpdated      = 16,
  SmcSystemStateNotifyQuiesceDevices             = 17,
  SmcSystemStateNotifyResumeDevices              = 18,
  SmcSystemStateNotifyGPUPanelPowerOn            = 19,
};

// SmcSystemStateNotifyMacOsPanicCause values, received after PanicDone
enum {
  SmcSystemStateNotifyPanicUnknown               = 0,
  SmcSystemStateNotifyPanicMacOSPanic            = 1,
  SmcSystemStateNotifyPanicMacOSWatchdog         = 2,
  SmcSystemStateNotifyPanicX86StraightS5Shutdown = 3,
  SmcSystemStateNotifyPanicX86GlobalReset        = 4,
  SmcSystemStateNotifyPanicX86CpuCATERR          = 5,
  SmcSystemStateNotifyPanicACPIPanic             = 6,
  SmcSystemStateNotifyPanicMacEFI                = 7,
};

// SMC_EVENT_CODE
typedef UINT8 SMC_EVENT_CODE;

// Log

#define SMC_MAX_LOG_SIZE  0x80

typedef UINT8 SMC_LOG;
typedef UINT8 SMC_LOG_SIZE;

// Hard drive encryption

#define SMC_HBKP_SIZE  0x20

#endif // APPLE_SMC_H
