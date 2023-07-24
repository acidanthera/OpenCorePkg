/** @file
  Copyright 2006-2007 Advanced Micro Devices, Inc.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _ATOM_BIOS_H_
#define _ATOM_BIOS_H_

#pragma pack(1)                                       /* BIOS data must use byte aligment */

/*  Define offset to location of ROM header. */

#define OFFSET_ATOM_ROM_MAGIC                 0x0030
#define OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER  0x0048
#define OFFSET_TO_ATOM_ROM_IMAGE_SIZE         0x0002

#define ATOM_ROM_MAGIC_STR             " 761295520"
#define ATOM_ROM_HEADER_SIGNATURE_STR  "ATOM"

typedef struct _ATOM_COMMON_TABLE_HEADER {
  UINT16    usStructureSize;
  UINT8     ucTableFormatRevision;      /*Change it when the Parser is not backward compatible */
  UINT8     ucTableContentRevision;     /*Change it only when the table needs to change but the firmware */
  /*Image can't be updated, while Driver needs to carry the new table! */
} ATOM_COMMON_TABLE_HEADER;

typedef struct _ATOM_ROM_HEADER {
  ATOM_COMMON_TABLE_HEADER    sHeader;
  UINT8                       uaFirmWareSignature[4]; /*Signature to distinguish between Atombios and non-atombios,
                                                       + atombios should init it as "ATOM", don't change the position */
  UINT16                      usBiosRuntimeSegmentAddress;
  UINT16                      usProtectedModeInfoOffset;
  UINT16                      usConfigFilenameOffset;
  UINT16                      usCRC_BlockOffset;
  UINT16                      usBIOS_BootupMessageOffset;
  UINT16                      usInt10Offset;
  UINT16                      usPciBusDevInitCode;
  UINT16                      usIoBaseAddress;
  UINT16                      usSubsystemVendorID;
  UINT16                      usSubsystemID;
  UINT16                      usPCI_InfoOffset;
  UINT16                      usMasterCommandTableOffset; /*Offset for SW to get all command table offsets, Don't change the position */
  UINT16                      usMasterDataTableOffset;    /*Offset for SW to get all data table offsets, Don't change the position */
  UINT8                       ucExtendedFunctionCode;
  UINT8                       ucReserved;
} ATOM_ROM_HEADER;

/****************************************************************************/
// Structure used in Data.mtb
/****************************************************************************/
typedef struct _ATOM_MASTER_LIST_OF_DATA_TABLES {
  UINT16    UtilityPipeLine;                    // Offest for the utility to get parser info,Don't change this position!
  UINT16    MultimediaCapabilityInfo;           // Only used by MM Lib,latest version 1.1, not configuable from Bios, need to include the table to build Bios
  UINT16    MultimediaConfigInfo;               // Only used by MM Lib,latest version 2.1, not configuable from Bios, need to include the table to build Bios
  UINT16    StandardVESA_Timing;                // Only used by Bios
  UINT16    FirmwareInfo;                       // Shared by various SW components,latest version 1.4
  UINT16    DAC_Info;                           // Will be obsolete from R600
  UINT16    LVDS_Info;                          // Shared by various SW components,latest version 1.1
  UINT16    TMDS_Info;                          // Will be obsolete from R600
  UINT16    AnalogTV_Info;                      // Shared by various SW components,latest version 1.1
  UINT16    SupportedDevicesInfo;               // Will be obsolete from R600
  UINT16    GPIO_I2C_Info;                      // Shared by various SW components,latest version 1.2 will be used from R600
  UINT16    VRAM_UsageByFirmware;               // Shared by various SW components,latest version 1.3 will be used from R600
  UINT16    GPIO_Pin_LUT;                       // Shared by various SW components,latest version 1.1
  UINT16    VESA_ToInternalModeLUT;             // Only used by Bios
  UINT16    ComponentVideoInfo;                 // Shared by various SW components,latest version 2.1 will be used from R600
  UINT16    PowerPlayInfo;                      // Shared by various SW components,latest version 2.1,new design from R600
  UINT16    CompassionateData;                  // Will be obsolete from R600
  UINT16    SaveRestoreInfo;                    // Only used by Bios
  UINT16    PPLL_SS_Info;                       // Shared by various SW components,latest version 1.2, used to call SS_Info, change to new name because of int ASIC SS info
  UINT16    OemInfo;                            // Defined and used by external SW, should be obsolete soon
  UINT16    XTMDS_Info;                         // Will be obsolete from R600
  UINT16    MclkSS_Info;                        // Shared by various SW components,latest version 1.1, only enabled when ext SS chip is used
  UINT16    Object_Header;                      // Shared by various SW components,latest version 1.1
  UINT16    IndirectIOAccess;                   // Only used by Bios,this table position can't change at all!!
  UINT16    MC_InitParameter;                   // Only used by command table
  UINT16    ASIC_VDDC_Info;                     // Will be obsolete from R600
  UINT16    ASIC_InternalSS_Info;               // New tabel name from R600, used to be called "ASIC_MVDDC_Info"
  UINT16    TV_VideoMode;                       // Only used by command table
  UINT16    VRAM_Info;                          // Only used by command table, latest version 1.3
  UINT16    MemoryTrainingInfo;                 // Used for VBIOS and Diag utility for memory training purpose since R600. the new table rev start from 2.1
  UINT16    IntegratedSystemInfo;               // Shared by various SW components
  UINT16    ASIC_ProfilingInfo;                 // New table name from R600, used to be called "ASIC_VDDCI_Info" for pre-R600
  UINT16    VoltageObjectInfo;                  // Shared by various SW components, latest version 1.1
  UINT16    PowerSourceInfo;                    // Shared by various SW components, latest versoin 1.1
} ATOM_MASTER_LIST_OF_DATA_TABLES;

typedef struct _ATOM_MASTER_DATA_TABLE {
  ATOM_COMMON_TABLE_HEADER           sHeader;
  ATOM_MASTER_LIST_OF_DATA_TABLES    ListOfDataTables;
} ATOM_MASTER_DATA_TABLE;

typedef union _ATOM_MODE_MISC_INFO_ACCESS {
  UINT16    usAccess;
} ATOM_MODE_MISC_INFO_ACCESS;

/****************************************************************************/
// Structure used in StandardVESA_TimingTable
//                   AnalogTV_InfoTable
//                   ComponentVideoInfoTable
/****************************************************************************/
typedef struct _ATOM_MODE_TIMING {
  UINT16                        usCRTC_H_Total;
  UINT16                        usCRTC_H_Disp;
  UINT16                        usCRTC_H_SyncStart;
  UINT16                        usCRTC_H_SyncWidth;
  UINT16                        usCRTC_V_Total;
  UINT16                        usCRTC_V_Disp;
  UINT16                        usCRTC_V_SyncStart;
  UINT16                        usCRTC_V_SyncWidth;
  UINT16                        usPixelClock;                                    // in 10Khz unit
  ATOM_MODE_MISC_INFO_ACCESS    susModeMiscInfo;
  UINT16                        usCRTC_OverscanRight;
  UINT16                        usCRTC_OverscanLeft;
  UINT16                        usCRTC_OverscanBottom;
  UINT16                        usCRTC_OverscanTop;
  UINT16                        usReserve;
  UINT8                         ucInternalModeNumber;
  UINT8                         ucRefreshRate;
} ATOM_MODE_TIMING;

typedef struct _ATOM_DTD_FORMAT {
  UINT16                        usPixClk;
  UINT16                        usHActive;
  UINT16                        usHBlanking_Time;
  UINT16                        usVActive;
  UINT16                        usVBlanking_Time;
  UINT16                        usHSyncOffset;
  UINT16                        usHSyncWidth;
  UINT16                        usVSyncOffset;
  UINT16                        usVSyncWidth;
  UINT16                        usImageHSize;
  UINT16                        usImageVSize;
  UINT8                         ucHBorder;
  UINT8                         ucVBorder;
  ATOM_MODE_MISC_INFO_ACCESS    susModeMiscInfo;
  UINT8                         ucInternalModeNumber;
  UINT8                         ucRefreshRate;
} ATOM_DTD_FORMAT;

typedef struct _ATOM_LVDS_INFO_V12 {
  ATOM_COMMON_TABLE_HEADER    sHeader;
  ATOM_DTD_FORMAT             sLCDTiming;
  UINT16                      usExtInfoTableOffset;
  UINT16                      usSupportedRefreshRate;   // Refer to panel info table in ATOMBIOS extension Spec.
  UINT16                      usOffDelayInMs;
  UINT8                       ucPowerSequenceDigOntoDEin10Ms;
  UINT8                       ucPowerSequenceDEtoBLOnin10Ms;
  UINT8                       ucLVDS_Misc;          // Bit0:{=0:single, =1:dual},Bit1 {=0:666RGB, =1:888RGB},Bit2:3:{Grey level}
  // Bit4:{=0:LDI format for RGB888, =1 FPDI format for RGB888}
  // Bit5:{=0:Spatial Dithering disabled;1 Spatial Dithering enabled}
  // Bit6:{=0:Temporal Dithering disabled;1 Temporal Dithering enabled}
  UINT8                       ucPanelDefaultRefreshRate;
  UINT8                       ucPanelIdentification;
  UINT8                       ucSS_Id;
  UINT16                      usLCDVenderID;
  UINT16                      usLCDProductID;
  UINT8                       ucLCDPanel_SpecialHandlingCap;
  UINT8                       ucPanelInfoSize;  //  start from ATOM_DTD_FORMAT to end of panel info, include ExtInfoTable
  UINT8                       ucReserved[2];
} ATOM_LVDS_INFO_V12;

typedef struct _ATOM_STANDARD_VESA_TIMING {
  ATOM_COMMON_TABLE_HEADER    sHeader;
  char                        *aModeTimings;                // 16 is not the real array number, just for initial allocation
} ATOM_STANDARD_VESA_TIMING;

#pragma pack()

#endif
