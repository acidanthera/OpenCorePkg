/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_DATA_HUB_H
#define APPLE_DATA_HUB_H

//
// Hack to avoid the need of framework headers.
//
typedef UINT16  STRING_REF;

#include <Guid/DataHubRecords.h>

// APPLE_PLATFORM_PRODUCER_NAME_GUID
#define APPLE_PLATFORM_PRODUCER_NAME_GUID  \
  { 0x64517CC8, 0x6561, 0x4051,            \
    { 0xB0, 0x3C, 0x59, 0x64, 0xB6, 0x0F, 0x4C, 0x7A } }

// APPLE_ROM_PRODUCER_NAME_GUID
#define APPLE_ROM_PRODUCER_NAME_GUID  \
  { 0xA38DA1AC, 0xA626, 0x4E18,       \
    { 0x93, 0x88, 0x14, 0xB0, 0xE8, 0x2A, 0x54, 0x04 } }

// APPLE_ROM_DATA_RECORD_GUID
#define APPLE_ROM_DATA_RECORD_GUID  \
  { 0x8CBDD607, 0xCAB4, 0x43A4,     \
    { 0x97, 0x8B, 0xAB, 0x8D, 0xEF, 0x11, 0x06, 0x1C } }

// APPLE_SYSTEM_SERIAL_NUMBER_DATA_RECORD_GUID
#define APPLE_SYSTEM_SERIAL_NUMBER_DATA_RECORD_GUID  \
  { 0x4BAA44C3, 0x9D4D, 0x46A6,                      \
    { 0x99, 0x13, 0xAE, 0xF9, 0x0D, 0x3C, 0x0C, 0xB1 } }

// APPLE_SYSTEM_ID_DATA_RECORD_GUID
#define APPLE_SYSTEM_ID_DATA_RECORD_GUID  \
  { 0x1485AFA4, 0xF000, 0x4E3E,           \
    { 0x81, 0xB4, 0xA7, 0xEE, 0x10, 0x4D, 0x5E, 0x30 } }

// APPLE_MODEL_DATA_RECORD_GUID
#define APPLE_MODEL_DATA_RECORD_GUID  \
  { 0xFA6AE23D, 0x09BE, 0x40A0,       \
    { 0xAF, 0xDE, 0x06, 0x37, 0x85, 0x94, 0x26, 0xC8 } }

// APPLE_DEVICE_PATHS_SUPPORTED_DATA_RECORD_GUID
#define APPLE_DEVICE_PATHS_SUPPORTED_DATA_RECORD_GUID  \
  { 0x5BB91CF7, 0xD816, 0x404B,                        \
    { 0x86, 0x72, 0x68, 0xF2, 0x7F, 0x78, 0x31, 0xDC } }

// APPLE_MACHINE_PERSONALITY_DATA_RECORD_GUID
#define APPLE_MACHINE_PERSONALITY_DATA_RECORD_GUID  \
  { 0x2B6C7ADE, 0xC5DA, 0x474B,                     \
    { 0xBA, 0x42, 0x06, 0xBD, 0xDD, 0x4E, 0x34, 0x97 } }

// APPLE_BOARD_ID_DATA_RECORD_GUID
#define APPLE_BOARD_ID_DATA_RECORD_GUID  \
  { 0xB459BF16, 0x14ED, 0x5131,          \
    { 0x92, 0xB4, 0x5E, 0x19, 0xF0, 0x5B, 0xC0, 0xAD } }

// APPLE_BOARD_REVISION_DATA_RECORD_GUID
#define APPLE_BOARD_REVISION_DATA_RECORD_GUID  \
  { 0x5F6B002A, 0xD39E, 0x57D0,                \
    { 0x82, 0xC1, 0x7C, 0x72, 0x18, 0x95, 0xBD, 0x62 } }

// APPLE_INITIAL_TSC_FREQUENCY_DATA_RECORD_GUID
#define APPLE_INITIAL_TSC_FREQUENCY_DATA_RECORD_GUID  \
  { 0x581BC734, 0xF9B5, 0x4A4A,                       \
    { 0x8C, 0xED, 0x25, 0x85, 0xDA, 0x1D, 0xE5, 0x08 } }

// APPLE_STARTUP_POWER_EVENTS_DATA_RECORD_GUID
#define APPLE_STARTUP_POWER_EVENTS_DATA_RECORD_GUID  \
  { 0x972057CF, 0x7145, 0x4C8A,                      \
    { 0x83, 0x0E, 0x3E, 0xCE, 0x8A, 0xC9, 0xB1, 0xF4 } }

// APPLE_COPROCESSOR_VERSION_DATA_RECORD_GUID
#define APPLE_COPROCESSOR_VERSION_DATA_RECORD_GUID  \
  { 0xE1AF3A96, 0x2783, 0x4C5B,                     \
    { 0xA1, 0x06, 0x36, 0x01, 0xF5, 0x85, 0x51, 0x05 } }

#define APPLE_SUBCLASS_VERSION   0x0100
#define APPLE_SUBCLASS_INSTANCE  EFI_SUBCLASS_INSTANCE_NON_APPLICABLE

// APPLE_PLATFORM_DATA_RECORD
typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER Header;
  UINT32                    KeySize;
  UINT32                    ValueSize;
  UINT8                     Data[];
// CHAR16                    Key[];
// UINT8                     Value[];
} APPLE_PLATFORM_DATA_RECORD;

// APPLE_ROM_RECORD
typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER Header;
  UINT8                     Reserved1;
  UINT64                    Rom;
  UINT8                     Reserved2[3];
} APPLE_ROM_RECORD;

// gApplePlatformProducerNameGuid
extern EFI_GUID gApplePlatformProducerNameGuid;

// gAppleRomProducerNameGuid
extern EFI_GUID gAppleRomProducerNameGuid;

// gAppleRomDataRecordGuid
extern EFI_GUID gAppleRomDataRecordGuid;

// gAppleSystemSerialNumbrDataRecordGuid
extern EFI_GUID gAppleSystemSerialNumbrDataRecordGuid;

// gAppleSystemIdDataRecordGuid
extern EFI_GUID gAppleSystemIdDataRecordGuid;

// gAppleModelDataRecordGuid
extern EFI_GUID gAppleModelDataRecordGuid;

// gAppleDevicePathsSupportedDataRecordGuid
extern EFI_GUID gAppleDevicePathsSupportedDataRecordGuid;

// gAppleMachinePersonalityDataRecordGuid
extern EFI_GUID gAppleMachinePersonalityDataRecordGuid;

// gAppleBoardIdDataRecordGuid
extern EFI_GUID gAppleBoardIdDataRecordGuid;

// gAppleBoardRevisionDataRecordGuid
extern EFI_GUID gAppleBoardRevisionDataRecordGuid;

// gAppleInitialTscDataRecordGuid
extern EFI_GUID gAppleInitialTscDataRecordGuid;

// gAppleStartupPowerEventsDataRecordGuid
extern EFI_GUID gAppleStartupPowerEventsDataRecordGuid;

// gAppleCoprocessorVersionDataRecordGuid
extern EFI_GUID gAppleCoprocessorVersionDataRecordGuid;

#endif // APPLE_DATA_HUB_H
