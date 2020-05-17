/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_SMC_IO_H
#define APPLE_SMC_IO_H

#include <IndustryStandard/AppleSmc.h>

// APPLE_SMC_IO_PROTOCOL_REVISION
#define APPLE_SMC_IO_PROTOCOL_REVISION  0x0000000033

// APPLE_SMC_IO_PROTOCOL_GUID
#define APPLE_SMC_IO_PROTOCOL_GUID                        \
  { 0x17407E5A, 0xAF6C, 0x4EE8,                           \
    { 0x98, 0xA8, 0x00, 0x21, 0x04, 0x53, 0xCD, 0xD9 } }

typedef struct APPLE_SMC_IO_PROTOCOL APPLE_SMC_IO_PROTOCOL;

// SMC_IO_SMC_READ_VALUE
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_READ_VALUE)(
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  IN  SMC_DATA_SIZE          Size,
  OUT SMC_DATA               *Value
  );

// SMC_IO_SMC_WRITE_VALUE
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_WRITE_VALUE)(
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  IN  SMC_DATA_SIZE          Size,
  IN  SMC_DATA               *Value
  );

// SMC_IO_SMC_GET_KEY_COUNT
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_GET_KEY_COUNT)(
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  OUT UINT32                 *Count
  );

// SMC_IO_SMC_MAKE_KEY
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_MAKE_KEY)(
  IN  CHAR8    *Name,
  OUT SMC_KEY  *Key
  );

// SMC_IO_SMC_GET_KEY_FROM_INDEX
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_GET_KEY_FROM_INDEX)(
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY_INDEX          Index,
  OUT SMC_KEY                *Key
  );

// SMC_IO_SMC_GET_KEY_INFO
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_GET_KEY_INFO)(
  IN  APPLE_SMC_IO_PROTOCOL  *This,
  IN  SMC_KEY                Key,
  OUT SMC_DATA_SIZE          *Size,
  OUT SMC_KEY_TYPE           *Type,
  OUT SMC_KEY_ATTRIBUTES     *Attributes
  );

// SMC_IO_SMC_RESET
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_RESET)(
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT32                 Mode
  );

// SMC_IO_SMC_FLASH_TYPE
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_FLASH_TYPE)(
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN SMC_FLASH_TYPE         Type
  );

// SMC_IO_SMC_FLASH_WRITE
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_FLASH_WRITE)(
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT32                 Unknown,
  IN SMC_FLASH_SIZE         Size,
  IN SMC_DATA               *Data
  );

// SMC_IO_SMC_FLASH_AUTH
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_FLASH_AUTH)(
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN SMC_FLASH_SIZE         Size,
  IN SMC_DATA               *Data
  );

// SMC_IO_SMC_UNSUPPORTED
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_UNSUPPORTED)(
  VOID
  );

// SMC_IO_SMC_UNKNOWN_1
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_UNKNOWN_1)(
  VOID
  );

// SMC_IO_SMC_UNKNOWN_2
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_UNKNOWN_2)(
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1,
  IN UINTN                  Ukn2
  );

// SMC_IO_SMC_UNKNOWN_3
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_UNKNOWN_3)(
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1,
  IN UINTN                  Ukn2
  );

// SMC_IO_SMC_UNKNOWN_4
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_UNKNOWN_4)(
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINTN                  Ukn1
  );

// SMC_IO_SMC_UNKNOWN_5
typedef
EFI_STATUS
(EFIAPI *SMC_IO_SMC_UNKNOWN_5)(
  IN APPLE_SMC_IO_PROTOCOL  *This,
  IN UINT8                  *Data
  );

// APPLE_SMC_IO_PROTOCOL
struct APPLE_SMC_IO_PROTOCOL {
  UINT64                        Revision;
  SMC_IO_SMC_READ_VALUE         SmcReadValue;
  SMC_IO_SMC_WRITE_VALUE        SmcWriteValue;
  SMC_IO_SMC_GET_KEY_COUNT      SmcGetKeyCount;
  SMC_IO_SMC_MAKE_KEY           SmcMakeKey;
  SMC_IO_SMC_GET_KEY_FROM_INDEX SmcGetKeyFromIndex;
  SMC_IO_SMC_GET_KEY_INFO       SmcGetKeyInfo;
  SMC_IO_SMC_RESET              SmcReset;
  SMC_IO_SMC_FLASH_TYPE         SmcFlashType;
  SMC_IO_SMC_UNSUPPORTED        SmcUnsupported;
  SMC_IO_SMC_FLASH_WRITE        SmcFlashWrite;
  SMC_IO_SMC_FLASH_AUTH         SmcFlashAuth;
  SMC_DEVICE_INDEX              Index;
  SMC_ADDRESS                   Address;
  BOOLEAN                       Mmio;
  SMC_IO_SMC_UNKNOWN_1          SmcUnknown1;
  SMC_IO_SMC_UNKNOWN_2          SmcUnknown2;
  SMC_IO_SMC_UNKNOWN_3          SmcUnknown3;
  SMC_IO_SMC_UNKNOWN_4          SmcUnknown4;
  SMC_IO_SMC_UNKNOWN_5          SmcUnknown5;
};

// gAppleSmcIoProtocolGuid
extern EFI_GUID gAppleSmcIoProtocolGuid;

#endif // APPLE_SMC_IO_H
