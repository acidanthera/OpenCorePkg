/** @file
Copyright (C) 2014 - 2017, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_SMBIOS_H
#define APPLE_SMBIOS_H

#include <IndustryStandard/SmBios.h>

#pragma pack (1)

#define APPLE_SMBIOS_TYPE_FIRMWARE_INFORMATION  128
#define APPLE_SMBIOS_TYPE_MEMORY_SPD_DATA       130
#define APPLE_SMBIOS_TYPE_PROCESSOR_TYPE        131
#define APPLE_SMBIOS_TYPE_PROCESSOR_BUS_SPEED   132
#define APPLE_SMBIOS_TYPE_PLATFORM_FEATURE      133
#define APPLE_SMBIOS_TYPE_SMC_INFORMATION       134

#define APPLE_NUMBER_OF_FLASHMAP_ENTRIES  8

#define APPLE_SMBIOS_SMC_VERSION_SIZE     16

// APPLE_REGION_TYPE
enum {
  AppleRegionTypeReserved  = 0,
  AppleRegionTypeRecovery  = 1,
  AppleRegionTypeMain      = 2,
  AppleRegionTypeNvram     = 3,
  AppleRegionTypeConfig    = 4,
  AppleRegionTypeDiagvault = 5
};

typedef UINT8 APPLE_REGION_TYPE;

// APPLE_FIRMWARE_REGION_INFO
typedef PACKED struct {
  UINT32 StartAddress;
  UINT32 EndAddress;
} APPLE_FIRMWARE_REGION_INFO;

// APPLE_SMBIOS_TABLE_TYPE128
typedef PACKED struct {
  SMBIOS_STRUCTURE           Hdr;
  UINT8                      NumberOfRegions;
  UINT8                      Reserved[3];
  UINT32                     FirmwareFeatures;
  UINT32                     FirmwareFeaturesMask;
  APPLE_REGION_TYPE          RegionTypeMap[APPLE_NUMBER_OF_FLASHMAP_ENTRIES];
  APPLE_FIRMWARE_REGION_INFO FlashMap[APPLE_NUMBER_OF_FLASHMAP_ENTRIES];
  UINT32                     ExtendedFirmwareFeatures;
  UINT32                     ExtendedFirmwareFeaturesMask;
} APPLE_SMBIOS_TABLE_TYPE128;

// APPLE_SMBIOS_TABLE_TYPE130
typedef struct {
  SMBIOS_STRUCTURE Hdr;
  UINT16           MemoryDeviceHandle;
  UINT16           Offset;
  UINT16           Size;
  UINT16           Data[];
} APPLE_SMBIOS_TABLE_TYPE130;

//
// It might be that 0x01 was intended for Pentium, and 0x08 for Atom.
// Reference binaries: AppleSmBios.efi and AppleSystemInfo.framework
//
// typedef struct {
//   UINT32       MajorType;
//   UINT32       MinorType;
//   UINT32       NumberOfCores;
//   CONST CHAR8  *MarketingName;
//   CONST CHAR8  *TechnicalName;
// } PROCESSOR_INFO;
//
// The cpu types listed here originate from AppleSystemInfo.framework and are not
// complete per se. The match of a certain CPU is firstly done by MajorType and
// NumberOfCores, and MinorType does not need to match. For a more complete
// table refer to AppleProcessorType enum, which is built based on Mac dumps.
//
// <0201> 01 cores  Intel Core Solo                Intel Core Solo
// <0201> 02 cores  Intel Core Duo                 Intel Core Duo
// <0301> 01 cores  Intel Core 2 Solo              Intel Core 2 Solo
// <0301> 02 cores  Intel Core 2 Duo               Intel Core 2 Duo
// <0401> 01 cores  Single-Core Intel Xeon         Single-Core Intel Xeon
// <0401> 02 cores  Dual-Core Intel Xeon           Dual-Core Intel Xeon
// <0402> 01 cores  Single-Core Intel Xeon         Single-Core Intel Xeon
// <0402> 02 cores  Dual-Core Intel Xeon           Dual-Core Intel Xeon
// <0402> 03 cores  Unknown Intel Xeon             Unknown Intel Xeon
// <0402> 04 cores  Quad-Core Intel Xeon           Quad-Core Intel Xeon
// <0501> 01 cores  Single-Core Intel Xeon         Single-Core Intel Xeon
// <0501> 02 cores  Dual-Core Intel Xeon           Dual-Core Intel Xeon
// <0501> 03 cores  Unknown Intel Xeon             Unknown Intel Xeon
// <0501> 04 cores  Quad-Core Intel Xeon           Quad-Core Intel Xeon
// <0501> 06 cores  6-Core Intel Xeon              6-Core Intel Xeon
// <0601> 01 cores  Intel Core i5                  Intel Core i5
// <0601> 02 cores  Intel Core i5                  Intel Core i5
// <0601> 03 cores  Intel Core i5                  Intel Core i5
// <0601> 04 cores  Intel Core i5                  Intel Core i5
// <0601> 06 cores  Intel Core i5                  Intel Core i5
// <0701> 01 cores  Intel Core i7                  Intel Core i7
// <0701> 02 cores  Intel Core i7                  Intel Core i7
// <0701> 03 cores  Intel Core i7                  Intel Core i7
// <0701> 04 cores  Intel Core i7                  Intel Core i7
// <0701> 06 cores  Intel Core i7                  Intel Core i7
// <0901> 01 cores  Intel Core i3                  Intel Core i3
// <0901> 02 cores  Intel Core i3                  Intel Core i3
// <0901> 03 cores  Intel Core i3                  Intel Core i3
// <0901> 04 cores  Intel Core i3                  Intel Core i3
// <0A01> 04 cores  Quad-Core Intel Xeon E5        Quad-Core Intel Xeon E5
// <0A01> 06 cores  6-Core Intel Xeon E5           6-Core Intel Xeon E5
// <0A01> 08 cores  8-Core Intel Xeon E5           8-Core Intel Xeon E5
// <0A01> 10 cores  10-Core Intel Xeon E5          10-Core Intel Xeon E5
// <0A01> 12 cores  12-Core Intel Xeon E5          12-Core Intel Xeon E5
// <0B01> 01 cores  Intel Core M                   Intel Core M
// <0B01> 02 cores  Intel Core M                   Intel Core M
// <0B01> 03 cores  Intel Core M                   Intel Core M
// <0B01> 04 cores  Intel Core M                   Intel Core M
// <0C01> 01 cores  Intel Core m3                  Intel Core m3
// <0C01> 02 cores  Intel Core m3                  Intel Core m3
// <0C01> 03 cores  Intel Core m3                  Intel Core m3
// <0C01> 04 cores  Intel Core m3                  Intel Core m3
// <0D01> 01 cores  Intel Core m5                  Intel Core m5
// <0D01> 02 cores  Intel Core m5                  Intel Core m5
// <0D01> 03 cores  Intel Core m5                  Intel Core m5
// <0D01> 04 cores  Intel Core m5                  Intel Core m5
// <0E01> 01 cores  Intel Core m7                  Intel Core m7
// <0E01> 02 cores  Intel Core m7                  Intel Core m7
// <0E01> 03 cores  Intel Core m7                  Intel Core m7
// <0E01> 04 cores  Intel Core m7                  Intel Core m7
// <0F01> 08 cores  Intel Xeon W                   Intel Xeon W
// <0F01> 10 cores  Intel Xeon W                   Intel Xeon W
// <0F01> 14 cores  Intel Xeon W                   Intel Xeon W
// <0F01> 18 cores  Intel Xeon W                   Intel Xeon W
// <1001> 06 cores  Intel Core i9                  Intel Core i9
//
// Also see here for a list of CPUs used on Mac models:
// https://docs.google.com/spreadsheets/d/1x09b5-DGh8ozNwN5ZjAi7TMnOp4TDm6DbmrKu86i_bQ
//

//
// Apple Processor Type Information - Processor Types.
//
enum {
  AppleProcessorTypeUnknown         = 0x0000,
  AppleProcessorTypeCoreSolo        = 0x0201,
  AppleProcessorTypeCore2DuoType1   = 0x0301,
  AppleProcessorTypeCore2DuoType2   = 0x0302,
  AppleProcessorTypeXeonPenrynType1 = 0x0401, // may not be used
  AppleProcessorTypeXeonPenrynType2 = 0x0402,
  AppleProcessorTypeXeon            = 0x0501,
  AppleProcessorTypeXeonE5          = 0x0A01,
  
  AppleProcessorTypeCorei5Type1     = 0x0601,
  AppleProcessorTypeCorei7Type1     = 0x0701,
  AppleProcessorTypeCorei3Type1     = 0x0901,
  
  AppleProcessorTypeCorei5Type2     = 0x0602,
  AppleProcessorTypeCorei7Type2     = 0x0702,
  AppleProcessorTypeCorei3Type2     = 0x0902,
  
  AppleProcessorTypeCorei5Type3     = 0x0603,
  AppleProcessorTypeCorei7Type3     = 0x0703,
  AppleProcessorTypeCorei3Type3     = 0x0903,
  
  AppleProcessorTypeCorei5Type4     = 0x0604,
  AppleProcessorTypeCorei7Type4     = 0x0704,
  AppleProcessorTypeCorei3Type4     = 0x0904,
  
  AppleProcessorTypeCorei5Type5     = 0x0605, // NOTE: we are putting 0x0609 on IM191 (i5-8600), although it should be 0x0605.
  AppleProcessorTypeCorei7Type5     = 0x0705,
  AppleProcessorTypeCorei3Type5     = 0x0905,
  
  AppleProcessorTypeCorei5Type6     = 0x0606, // i5 5250U (iMac16,1, MacBookAir7,1 and 7,2), i5 5675R (iMac16,2), i5 5257U (MacBookPro12,1)
  AppleProcessorTypeCorei7Type6     = 0x0706, // ideal value for Broadwell i7, need confirmation
  AppleProcessorTypeCorei3Type6     = 0x0906, // ideal value for Broadwell i3, need confirmation
  
  // placeholder for i5Type7 (maybe 0x0607 ???)
  AppleProcessorTypeCorei7Type7     = 0x0707, // i7 6700HQ (MacBookPro13,3)
  // placeholder for i3Type7 (maybe 0x0907 ???)
  
  AppleProcessorTypeCorei5Type8     = 0x0608, // i5 8210Y (MacBookAir8,1)
  // placeholder for i7Type8 (maybe 0x0708 ???)
  // placeholder for i3Type8 (maybe 0x0908 ???)
  
  AppleProcessorTypeCorei5Type9     = 0x0609, // i5 8259U (MacBookPro15,2), i5 8500B (Macmini8,1)
  AppleProcessorTypeCorei7Type9     = 0x0709, // i7 8850H (MacBookPro15,1), i7 8700B (Macmini8,1)
  // placeholder for i3Type9 (maybe 0x0909 ???)
  
  AppleProcessorTypeCoreMType1      = 0x0B01, // may not be used
  AppleProcessorTypeCoreMType6      = 0x0B06, // M 5Y51 (MacBook8,1)

  AppleProcessorTypeCoreM3Type1     = 0x0C01, // may not be used
  AppleProcessorTypeCoreM3Type7     = 0x0C07, // m3-7Y32 (MacBook10,1)

  AppleProcessorTypeCoreM5Type1     = 0x0D01, // may not be used
  AppleProcessorTypeCoreM5Type7     = 0x0D07, // m5-6Y54 (MacBook9,1)

  AppleProcessorTypeCoreM7Type1     = 0x0E01, // may not be used
  AppleProcessorTypeCoreM7Type7     = 0x0E07, // might be used on Core m7 (SKL), need confirmation

  AppleProcessorTypeXeonW           = 0x0F01, // iMacPro1,1
  
  AppleProcessorTypeCorei9Type1     = 0x1001, // may not be used
  AppleProcessorTypeCorei9Type5     = 0x1005, // SKL-X i9, most likely to be invalid!
  AppleProcessorTypeCorei9Type9     = 0x1009  // ideal value for Coffee Lake i9, need confirmation
};

// APPLE_PROCESSOR_TYPE_CLASS

enum {
  AppleProcessorMajorUnknown     = 0x00,
  AppleProcessorMajorCore        = 0x02,
  AppleProcessorMajorCore2       = 0x03,
  AppleProcessorMajorXeonPenryn  = 0x04,
  AppleProcessorMajorXeonNehalem = 0x05,
  AppleProcessorMajorI5          = 0x06,
  AppleProcessorMajorI7          = 0x07,
  AppleProcessorMajorI3          = 0x09,
  AppleProcessorMajorI9          = 0x10,
  AppleProcessorMajorXeonE5      = 0x0A,
  AppleProcessorMajorM           = 0x0B,
  AppleProcessorMajorM3          = 0x0C,
  AppleProcessorMajorM5          = 0x0D,
  AppleProcessorMajorM7          = 0x0E,
  AppleProcessorMajorXeonW       = 0x0F
};

typedef struct {
  UINT8 MinorType;
  UINT8 MajorType;
} APPLE_PROCESSOR_TYPE_INFO;

typedef union {
  APPLE_PROCESSOR_TYPE_INFO  Detail;
  UINT16                     Type;
} APPLE_PROCESSOR_TYPE;

// APPLE_SMBIOS_TABLE_TYPE131
typedef PACKED struct {
  SMBIOS_STRUCTURE     Hdr;
  APPLE_PROCESSOR_TYPE ProcessorType;
  UINT8                Reserved[2];
} APPLE_SMBIOS_TABLE_TYPE131;

// APPLE_SMBIOS_TABLE_TYPE132
typedef struct {
  SMBIOS_STRUCTURE Hdr;
  UINT16           ProcessorBusSpeed;
} APPLE_SMBIOS_TABLE_TYPE132;

// APPLE_SMBIOS_TABLE_TYPE133
typedef struct {
  SMBIOS_STRUCTURE Hdr;
  UINT64           PlatformFeature;
} APPLE_SMBIOS_TABLE_TYPE133;

// APPLE_SMBIOS_TABLE_TYPE134
typedef PACKED struct {
  SMBIOS_STRUCTURE Hdr;
  UINT8            SmcVersion[APPLE_SMBIOS_SMC_VERSION_SIZE];
} APPLE_SMBIOS_TABLE_TYPE134;

// APPLE_SMBIOS_STRUCTURE_POINTER
typedef union {
  SMBIOS_STRUCTURE_POINTER   Standard;
  APPLE_SMBIOS_TABLE_TYPE128 *Type128;
  APPLE_SMBIOS_TABLE_TYPE130 *Type130;
  APPLE_SMBIOS_TABLE_TYPE131 *Type131;
  APPLE_SMBIOS_TABLE_TYPE132 *Type132;
  APPLE_SMBIOS_TABLE_TYPE133 *Type133;
  APPLE_SMBIOS_TABLE_TYPE134 *Type134;
  UINT8                      *Raw;
} APPLE_SMBIOS_STRUCTURE_POINTER;

#pragma pack ()

#endif // APPLE_SMBIOS_H
