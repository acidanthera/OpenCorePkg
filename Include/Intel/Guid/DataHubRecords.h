/** @file
  DataHubRecord.h includes all data hub subclass GUID definitions.

  This file includes all data hub sub class defitions from
  Cache subclass specification 0.9, DataHub SubClass specification 0.9, Memory SubClass Spec 0.9,
  Processor Subclass specification 0.9, and Misc SubClass specification 0.9.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _DATAHUB_RECORDS_GUID_H_
#define _DATAHUB_RECORDS_GUID_H_

//
// The include is required to retrieve type EFI_EXP_BASE10_DATA
//
#include <Guid/StatusCodeDataTypeId.h>

#define EFI_PROCESSOR_SUBCLASS_GUID \
  { 0x26fdeb7e, 0xb8af, 0x4ccf, {0xaa, 0x97, 0x02, 0x63, 0x3c, 0xe4, 0x8c, 0xa7 } }

extern  EFI_GUID gEfiProcessorSubClassGuid;


#define EFI_CACHE_SUBCLASS_GUID \
  { 0x7f0013a7, 0xdc79, 0x4b22, {0x80, 0x99, 0x11, 0xf7, 0x5f, 0xdc, 0x82, 0x9d } }

extern  EFI_GUID gEfiCacheSubClassGuid;

///
/// The memory subclass belongs to the data class and is identified as the memory
/// subclass by the GUID.
///
#define EFI_MEMORY_SUBCLASS_GUID \
  {0x4E8F4EBB, 0x64B9, 0x4e05, {0x9B, 0x18, 0x4C, 0xFE, 0x49, 0x23, 0x50, 0x97} }

extern  EFI_GUID  gEfiMemorySubClassGuid;

#define EFI_MISC_SUBCLASS_GUID \
  { 0x772484B2, 0x7482, 0x4b91, {0x9F, 0x9A, 0xAD, 0x43, 0xF8, 0x1C, 0x58, 0x81 } }

extern  EFI_GUID  gEfiMiscSubClassGuid;


///
/// Inconsistent with specification here:
/// In ProcSubclass specification 0.9, the value is 0x0100.
/// Keep it unchanged from the perspective of binary consistency.
///
#define EFI_PROCESSOR_SUBCLASS_VERSION    0x00010000

#pragma pack(1)

typedef struct _USB_PORT_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH              PciRootBridgeDevicePath;
  PCI_DEVICE_PATH                   PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} USB_PORT_DEVICE_PATH;

//
// IDE
//
typedef struct _IDE_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH              PciRootBridgeDevicePath;
  PCI_DEVICE_PATH                   PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} IDE_DEVICE_PATH;

//
// RMC Connector
//
typedef struct _RMC_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH              PciRootBridgeDevicePath;
  PCI_DEVICE_PATH                   PciBridgeDevicePath;
  PCI_DEVICE_PATH                   PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} RMC_CONN_DEVICE_PATH;

//
// RIDE
//
typedef struct _RIDE_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH              PciRootBridgeDevicePath;
  PCI_DEVICE_PATH                   PciBridgeDevicePath;
  PCI_DEVICE_PATH                   PciBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} RIDE_DEVICE_PATH;

//
// Gigabit NIC
//
typedef struct _GB_NIC_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH              PciRootBridgeDevicePath;
  PCI_DEVICE_PATH                   PciBridgeDevicePath;
  PCI_DEVICE_PATH                   PciXBridgeDevicePath;
  PCI_DEVICE_PATH                   PciXBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} GB_NIC_DEVICE_PATH;

//
// P/S2 Connector
//
typedef struct _PS2_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH              PciRootBridgeDevicePath;
  PCI_DEVICE_PATH                   LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH              LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} PS2_CONN_DEVICE_PATH;

//
// Serial Port Connector
//
typedef struct _SERIAL_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH              PciRootBridgeDevicePath;
  PCI_DEVICE_PATH                   LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH              LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} SERIAL_CONN_DEVICE_PATH;

//
// Parallel Port Connector
//
typedef struct _PARALLEL_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH              PciRootBridgeDevicePath;
  PCI_DEVICE_PATH                   LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH              LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} PARALLEL_CONN_DEVICE_PATH;

//
// Floopy Connector
//
typedef struct _FLOOPY_CONN_DEVICE_PATH {
  ACPI_HID_DEVICE_PATH              PciRootBridgeDevicePath;
  PCI_DEVICE_PATH                   LpcBridgeDevicePath;
  ACPI_HID_DEVICE_PATH              LpcBusDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          EndDevicePath;
} FLOOPY_CONN_DEVICE_PATH;

///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, this data structure and corrsponding fields are NOT defined.
/// It's implementation-specific to simplify the code logic.
///
typedef union _EFI_MISC_PORT_DEVICE_PATH {
  USB_PORT_DEVICE_PATH              UsbDevicePath;
  IDE_DEVICE_PATH                   IdeDevicePath;
  RMC_CONN_DEVICE_PATH              RmcConnDevicePath;
  RIDE_DEVICE_PATH                  RideDevicePath;
  GB_NIC_DEVICE_PATH                GbNicDevicePath;
  PS2_CONN_DEVICE_PATH              Ps2ConnDevicePath;
  SERIAL_CONN_DEVICE_PATH           SerialConnDevicePath;
  PARALLEL_CONN_DEVICE_PATH         ParallelConnDevicePath;
  FLOOPY_CONN_DEVICE_PATH           FloppyConnDevicePath;
} EFI_MISC_PORT_DEVICE_PATH;

#pragma pack()

///
/// String Token Definition
///
/// Inconsistent with specification here:
/// The macro isn't defined by any specification.
/// Keep it unchanged for backward compatibility.
///
#define EFI_STRING_TOKEN          UINT16

///
/// Each data record that is a member of some subclass starts with a standard
/// header of type EFI_SUBCLASS_TYPE1_HEADER.
/// This header is only a guideline and applicable only to a data
/// subclass that is producing SMBIOS data records. A subclass can start with a
/// different header if needed.
///
typedef struct {
  ///
  /// The version of the specification to which a specific subclass data record adheres.
  ///
  UINT32                            Version;
  ///
  /// The size in bytes of this data class header.
  ///
  UINT32                            HeaderSize;
  ///
  /// The instance number of the subclass with the same ProducerName. This number is
  /// applicable in cases where multiple subclass instances that were produced by the same
  /// driver exist in the system. This entry is 1 based; 0 means Reserved and -1 means Not
  /// Applicable. All data consumer drivers should be able to handle all the possible values
  /// of Instance, including Not Applicable and Reserved.
  ///
  UINT16                            Instance;
  ///
  /// The instance number of the RecordType for the same Instance. This number is
  /// applicable in cases where multiple instances of the RecordType exist for a specific
  /// Instance. This entry is 1 based; 0 means Reserved and -1 means Not Applicable.
  /// All data consumer drivers should be able to handle all the possible values of
  /// SubInstance, including Not Applicable and Reserved.
  ///
  UINT16                            SubInstance;
  ///
  /// The record number for the data record being specified. The numbering scheme and
  /// definition is defined in the specific subclass specification.
  ///
  UINT32                            RecordType;
} EFI_SUBCLASS_TYPE1_HEADER;

///
/// This structure is used to link data records in the same subclasses. A data record is
/// defined as a link to another data record in the same subclass using this structure.
///
typedef struct {
  ///
  /// An EFI_GUID that identifies the component that produced this data record. Type
  /// EFI_GUID is defined in InstallProtocolInterface() in the EFI 1.10 Specification.
  ///
  EFI_GUID                          ProducerName;
  ///
  /// The instance number of the subclass with the same ProducerName. This number is
  /// applicable in cases where multiple subclass instances that were produced by the same
  /// driver exist in the system. This entry is 1 based; 0 means Reserved and -1 means Not
  /// Applicable. All data consumer drivers should be able to handle all the possible values
  /// of Instance, including Not Applicable and Reserved.
  ///
  UINT16                            Instance;
  /// The instance number of the RecordType for the same Instance. This number is
  /// applicable in cases where multiple instances of the RecordType exist for a specific
  /// Instance. This entry is 1 based; 0 means Reserved and -1 means Not Applicable.
  /// All data consumer drivers should be able to handle all the possible values of
  /// SubInstance, including Not Applicable and Reserved.
  UINT16                            SubInstance;
} EFI_INTER_LINK_DATA;

//
// EXP data
//
///
/// This macro provides a calculation for base-10 representations. Value and Exponent are each
/// INT16. It is signed to cover negative values and is 16 bits wide (15 bits for data and 1 bit
/// for the sign).
///
typedef struct {
  ///
  /// The INT16 number by which to multiply the base-10 representation.
  ///
  UINT16                            Value;
  ///
  /// The INT16 number by which to raise the base-10 calculation.
  ///
  UINT16                            Exponent;
} EFI_EXP_BASE2_DATA;

typedef EFI_EXP_BASE10_DATA        EFI_PROCESSOR_MAX_CORE_FREQUENCY_DATA;
typedef EFI_EXP_BASE10_DATA        EFI_PROCESSOR_MAX_FSB_FREQUENCY_DATA;
typedef EFI_EXP_BASE10_DATA        EFI_PROCESSOR_CORE_FREQUENCY_DATA;

///
/// This data record refers to the list of frequencies that the processor core supports. The list of
/// supported frequencies is determined by the firmware based on hardware capabilities--for example,
/// it could be a common subset of all processors and the chipset. The unit of measurement of this data
/// record is in Hertz. For asynchronous processors, the content of this data record is zero.
/// The list is terminated by -1 in the Value field of the last element. A Value field of zero means
/// that the processor/driver supports automatic frequency selection.
///
/// Inconsistent with specification here:
/// According to MiscSubclass 0.9 specification, it should be a pointer since it refers to a list of frequencies.
///
typedef EFI_EXP_BASE10_DATA        *EFI_PROCESSOR_CORE_FREQUENCY_LIST_DATA;

///
/// This data record refers to the list of supported frequencies of the processor external bus. The list of
/// supported frequencies is determined by the firmware based on hardware capabilities--for example,
/// it could be a common subset of all processors and the chipset. The unit of measurement of this data
/// record is in Hertz. For asynchronous processors, the content of this data record is NULL.
/// The list is terminated by -1 in the Value field of the last element. A Value field of zero means
/// that the processor/driver supports automatic frequency selection.
///
typedef EFI_EXP_BASE10_DATA        *EFI_PROCESSOR_FSB_FREQUENCY_LIST_DATA;
typedef EFI_EXP_BASE10_DATA        EFI_PROCESSOR_FSB_FREQUENCY_DATA;
typedef STRING_REF                 EFI_PROCESSOR_VERSION_DATA;
typedef STRING_REF                 EFI_PROCESSOR_MANUFACTURER_DATA;
typedef STRING_REF                 EFI_PROCESSOR_SERIAL_NUMBER_DATA;
typedef STRING_REF                 EFI_PROCESSOR_ASSET_TAG_DATA;
typedef STRING_REF                 EFI_PROCESSOR_PART_NUMBER_DATA;

typedef struct {
  UINT32                            ProcessorSteppingId:4;
  UINT32                            ProcessorModel:     4;
  UINT32                            ProcessorFamily:    4;
  UINT32                            ProcessorType:      2;
  UINT32                            ProcessorReserved1: 2;
  UINT32                            ProcessorXModel:    4;
  UINT32                            ProcessorXFamily:   8;
  UINT32                            ProcessorReserved2: 4;
} EFI_PROCESSOR_SIGNATURE;


///
/// Inconsistent with specification here:
/// The name of third field in ProcSubClass specification 0.9 is LogicalProcessorCount.
/// Keep it unchanged for backward compatibility.
///
typedef struct {
  UINT32                            ProcessorBrandIndex    :8;
  UINT32                            ProcessorClflush       :8;
  UINT32                            ProcessorReserved      :8;
  UINT32                            ProcessorDfltApicId    :8;
} EFI_PROCESSOR_MISC_INFO;

typedef struct {
  UINT32                            ProcessorFpu:       1;
  UINT32                            ProcessorVme:       1;
  UINT32                            ProcessorDe:        1;
  UINT32                            ProcessorPse:       1;
  UINT32                            ProcessorTsc:       1;
  UINT32                            ProcessorMsr:       1;
  UINT32                            ProcessorPae:       1;
  UINT32                            ProcessorMce:       1;
  UINT32                            ProcessorCx8:       1;
  UINT32                            ProcessorApic:      1;
  UINT32                            ProcessorReserved1: 1;
  UINT32                            ProcessorSep:       1;
  UINT32                            ProcessorMtrr:      1;
  UINT32                            ProcessorPge:       1;
  UINT32                            ProcessorMca:       1;
  UINT32                            ProcessorCmov:      1;
  UINT32                            ProcessorPat:       1;
  UINT32                            ProcessorPse36:     1;
  UINT32                            ProcessorPsn:       1;
  UINT32                            ProcessorClfsh:     1;
  UINT32                            ProcessorReserved2: 1;
  UINT32                            ProcessorDs:        1;
  UINT32                            ProcessorAcpi:      1;
  UINT32                            ProcessorMmx:       1;
  UINT32                            ProcessorFxsr:      1;
  UINT32                            ProcessorSse:       1;
  UINT32                            ProcessorSse2:      1;
  UINT32                            ProcessorSs:        1;
  UINT32                            ProcessorReserved3: 1;
  UINT32                            ProcessorTm:        1;
  UINT32                            ProcessorReserved4: 2;
} EFI_PROCESSOR_FEATURE_FLAGS;

///
/// This data record refers to the unique ID that identifies a set of processors. This data record is 16
/// bytes in length. The data in this structure is processor specific and reserved values can be defined
/// for future use. The consumer of this data should not make any assumption and should use this data
/// with respect to the processor family defined in the Family record number.
///
typedef struct {
  ///
  /// Identifies the processor.
  ///
  EFI_PROCESSOR_SIGNATURE           Signature;
  ///
  /// Provides additional processor information.
  ///
  EFI_PROCESSOR_MISC_INFO           MiscInfo;
  ///
  /// Reserved for future use.
  ///
  UINT32                            Reserved;
  ///
  /// Provides additional processor information.
  ///
  EFI_PROCESSOR_FEATURE_FLAGS       FeatureFlags;
} EFI_PROCESSOR_ID_DATA;

///
/// This data record refers to the general classification of the processor. This data record is 4 bytes in
/// length.
///
typedef enum {
  EfiProcessorOther    = 1,
  EfiProcessorUnknown  = 2,
  EfiCentralProcessor  = 3,
  EfiMathProcessor     = 4,
  EfiDspProcessor      = 5,
  EfiVideoProcessor    = 6
} EFI_PROCESSOR_TYPE_DATA;

///
/// This data record refers to the family of the processor as defined by the DMTF.
/// This data record is 4 bytes in length.
///
typedef enum {
  EfiProcessorFamilyOther                  = 0x01,
  EfiProcessorFamilyUnknown                = 0x02,
  EfiProcessorFamily8086                   = 0x03,
  EfiProcessorFamily80286                  = 0x04,
  EfiProcessorFamilyIntel386               = 0x05,
  EfiProcessorFamilyIntel486               = 0x06,
  EfiProcessorFamily8087                   = 0x07,
  EfiProcessorFamily80287                  = 0x08,
  EfiProcessorFamily80387                  = 0x09,
  EfiProcessorFamily80487                  = 0x0A,
  EfiProcessorFamilyPentium                = 0x0B,
  EfiProcessorFamilyPentiumPro             = 0x0C,
  EfiProcessorFamilyPentiumII              = 0x0D,
  EfiProcessorFamilyPentiumMMX             = 0x0E,
  EfiProcessorFamilyCeleron                = 0x0F,
  EfiProcessorFamilyPentiumIIXeon          = 0x10,
  EfiProcessorFamilyPentiumIII             = 0x11,
  EfiProcessorFamilyM1                     = 0x12,
  EfiProcessorFamilyM2                     = 0x13,
  EfiProcessorFamilyM1Reserved2            = 0x14,
  EfiProcessorFamilyM1Reserved3            = 0x15,
  EfiProcessorFamilyM1Reserved4            = 0x16,
  EfiProcessorFamilyM1Reserved5            = 0x17,
  EfiProcessorFamilyAmdDuron               = 0x18,
  EfiProcessorFamilyK5                     = 0x19,
  EfiProcessorFamilyK6                     = 0x1A,
  EfiProcessorFamilyK6_2                   = 0x1B,
  EfiProcessorFamilyK6_3                   = 0x1C,
  EfiProcessorFamilyAmdAthlon              = 0x1D,
  EfiProcessorFamilyAmd29000               = 0x1E,
  EfiProcessorFamilyK6_2Plus               = 0x1F,
  EfiProcessorFamilyPowerPC                = 0x20,
  EfiProcessorFamilyPowerPC601             = 0x21,
  EfiProcessorFamilyPowerPC603             = 0x22,
  EfiProcessorFamilyPowerPC603Plus         = 0x23,
  EfiProcessorFamilyPowerPC604             = 0x24,
  EfiProcessorFamilyPowerPC620             = 0x25,
  EfiProcessorFamilyPowerPCx704            = 0x26,
  EfiProcessorFamilyPowerPC750             = 0x27,
  EfiProcessorFamilyAlpha3                 = 0x30,
  EfiProcessorFamilyAlpha21064             = 0x31,
  EfiProcessorFamilyAlpha21066             = 0x32,
  EfiProcessorFamilyAlpha21164             = 0x33,
  EfiProcessorFamilyAlpha21164PC           = 0x34,
  EfiProcessorFamilyAlpha21164a            = 0x35,
  EfiProcessorFamilyAlpha21264             = 0x36,
  EfiProcessorFamilyAlpha21364             = 0x37,
  EfiProcessorFamilyMips                   = 0x40,
  EfiProcessorFamilyMIPSR4000              = 0x41,
  EfiProcessorFamilyMIPSR4200              = 0x42,
  EfiProcessorFamilyMIPSR4400              = 0x43,
  EfiProcessorFamilyMIPSR4600              = 0x44,
  EfiProcessorFamilyMIPSR10000             = 0x45,
  EfiProcessorFamilySparc                  = 0x50,
  EfiProcessorFamilySuperSparc             = 0x51,
  EfiProcessorFamilymicroSparcII           = 0x52,
  EfiProcessorFamilymicroSparcIIep         = 0x53,
  EfiProcessorFamilyUltraSparc             = 0x54,
  EfiProcessorFamilyUltraSparcII           = 0x55,
  EfiProcessorFamilyUltraSparcIIi          = 0x56,
  EfiProcessorFamilyUltraSparcIII          = 0x57,
  ///
  /// Inconsistent with specification here:
  /// This field in ProcSubClass specification 0.9 is defined as EfiProcessorFamilyUltraSparcIIi.
  /// Change it to EfiProcessorFamilyUltraSparcIIIi to avoid build break.
  ///
  EfiProcessorFamilyUltraSparcIIIi         = 0x58,
  EfiProcessorFamily68040                  = 0x60,
  EfiProcessorFamily68xxx                  = 0x61,
  EfiProcessorFamily68000                  = 0x62,
  EfiProcessorFamily68010                  = 0x63,
  EfiProcessorFamily68020                  = 0x64,
  EfiProcessorFamily68030                  = 0x65,
  EfiProcessorFamilyHobbit                 = 0x70,
  EfiProcessorFamilyCrusoeTM5000           = 0x78,
  EfiProcessorFamilyCrusoeTM3000           = 0x79,
  EfiProcessorFamilyEfficeonTM8000         = 0x7A,
  EfiProcessorFamilyWeitek                 = 0x80,
  EfiProcessorFamilyItanium                = 0x82,
  EfiProcessorFamilyAmdAthlon64            = 0x83,
  EfiProcessorFamilyAmdOpteron             = 0x84,
  EfiProcessorFamilyAmdSempron             = 0x85,
  EfiProcessorFamilyAmdTurion64Mobile      = 0x86,
  EfiProcessorFamilyDualCoreAmdOpteron     = 0x87,
  EfiProcessorFamilyAmdAthlon64X2DualCore  = 0x88,
  EfiProcessorFamilyAmdTurion64X2Mobile    = 0x89,
  EfiProcessorFamilyPARISC                 = 0x90,
  EfiProcessorFamilyPaRisc8500             = 0x91,
  EfiProcessorFamilyPaRisc8000             = 0x92,
  EfiProcessorFamilyPaRisc7300LC           = 0x93,
  EfiProcessorFamilyPaRisc7200             = 0x94,
  EfiProcessorFamilyPaRisc7100LC           = 0x95,
  EfiProcessorFamilyPaRisc7100             = 0x96,
  EfiProcessorFamilyV30                    = 0xA0,
  EfiProcessorFamilyPentiumIIIXeon         = 0xB0,
  EfiProcessorFamilyPentiumIIISpeedStep    = 0xB1,
  EfiProcessorFamilyPentium4               = 0xB2,
  EfiProcessorFamilyIntelXeon              = 0xB3,
  EfiProcessorFamilyAS400                  = 0xB4,
  EfiProcessorFamilyIntelXeonMP            = 0xB5,
  EfiProcessorFamilyAMDAthlonXP            = 0xB6,
  EfiProcessorFamilyAMDAthlonMP            = 0xB7,
  EfiProcessorFamilyIntelItanium2          = 0xB8,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyIntelPentiumM          = 0xB9,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyIntelCeleronD          = 0xBA,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyIntelPentiumD          = 0xBB,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyIntelPentiumEx         = 0xBC,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyIntelCoreSolo          = 0xBD,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyReserved               = 0xBE,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyIntelCore2             = 0xBF,
  EfiProcessorFamilyIBM390                 = 0xC8,
  EfiProcessorFamilyG4                     = 0xC9,
  EfiProcessorFamilyG5                     = 0xCA,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification  0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyG6                     = 0xCB,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyzArchitectur           = 0xCC,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyViaC7M                 = 0xD2,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyViaC7D                 = 0xD3,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyViaC7                  = 0xD4,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyViaEden                = 0xD5,
  EfiProcessorFamilyi860                   = 0xFA,
  EfiProcessorFamilyi960                   = 0xFB,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyIndicatorFamily2       = 0xFE,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorFamilyReserved1              = 0xFF
} EFI_PROCESSOR_FAMILY_DATA;

typedef enum {
  EfiProcessorFamilySh3           = 0x104,
  EfiProcessorFamilySh4           = 0x105,
  EfiProcessorFamilyArm           = 0x118,
  EfiProcessorFamilyStrongArm     = 0x119,
  EfiProcessorFamily6x86          = 0x12C,
  EfiProcessorFamilyMediaGx       = 0x12D,
  EfiProcessorFamilyMii           = 0x12E,
  EfiProcessorFamilyWinChip       = 0x140,
  EfiProcessorFamilyDsp           = 0x15E,
  EfiProcessorFamilyVideo         = 0x1F4
} EFI_PROCESSOR_FAMILY2_DATA;

///
/// This data record refers to the core voltage of the processor being defined. The unit of measurement
/// of this data record is in volts.
///
typedef EFI_EXP_BASE10_DATA         EFI_PROCESSOR_VOLTAGE_DATA;

///
/// This data record refers to the base address of the APIC of the processor being defined. This data
/// record is a physical address location.
///
typedef EFI_PHYSICAL_ADDRESS        EFI_PROCESSOR_APIC_BASE_ADDRESS_DATA;

///
/// This data record refers to the ID of the APIC of the processor being defined. This data record is a
/// 4-byte entry.
///
typedef UINT32                      EFI_PROCESSOR_APIC_ID_DATA;

///
/// This data record refers to the version number of the APIC of the processor being defined. This data
/// record is a 4-byte entry.
///
typedef UINT32                      EFI_PROCESSOR_APIC_VERSION_NUMBER_DATA;

typedef enum {
  EfiProcessorIa32Microcode    = 1,
  EfiProcessorIpfPalAMicrocode = 2,
  EfiProcessorIpfPalBMicrocode = 3
} EFI_PROCESSOR_MICROCODE_TYPE;

///
/// This data record refers to the revision of the processor microcode that is loaded in the processor.
/// This data record is a 4-byte entry.
///
typedef struct {
  ///
  /// Identifies what type of microcode the data is.
  ///
  EFI_PROCESSOR_MICROCODE_TYPE      ProcessorMicrocodeType;
  ///
  /// Indicates the revision number of this microcode.
  ///
  UINT32                            ProcessorMicrocodeRevisionNumber;
} EFI_PROCESSOR_MICROCODE_REVISION_DATA;

///
/// This data record refers to the status of the processor.
///
typedef struct {
  UINT32       CpuStatus                 :3; ///< Indicates the status of the processor.
  UINT32       Reserved1                 :3; ///< Reserved for future use. Should be set to zero.
  UINT32       SocketPopulated           :1; ///< Indicates if the processor is socketed or not.
  UINT32       Reserved2                 :1; ///< Reserved for future use. Should be set to zero.
  UINT32       ApicEnable                :1; ///< Indicates if the APIC is enabled or not.
  UINT32       BootApplicationProcessor  :1; ///< Indicates if this processor is the boot processor.
  UINT32       Reserved3                 :22;///< Reserved for future use. Should be set to zero.
} EFI_PROCESSOR_STATUS_DATA;

typedef enum {
  EfiCpuStatusUnknown        = 0,
  EfiCpuStatusEnabled        = 1,
  EfiCpuStatusDisabledByUser = 2,
  EfiCpuStatusDisabledbyBios = 3,
  EfiCpuStatusIdle           = 4,
  EfiCpuStatusOther          = 7
} EFI_CPU_STATUS;

typedef enum {
  EfiProcessorSocketOther            = 1,
  EfiProcessorSocketUnknown          = 2,
  EfiProcessorSocketDaughterBoard    = 3,
  EfiProcessorSocketZIF              = 4,
  EfiProcessorSocketReplacePiggyBack = 5,
  EfiProcessorSocketNone             = 6,
  EfiProcessorSocketLIF              = 7,
  EfiProcessorSocketSlot1            = 8,
  EfiProcessorSocketSlot2            = 9,
  EfiProcessorSocket370Pin           = 0xA,
  EfiProcessorSocketSlotA            = 0xB,
  EfiProcessorSocketSlotM            = 0xC,
  EfiProcessorSocket423              = 0xD,
  EfiProcessorSocketA462             = 0xE,
  EfiProcessorSocket478              = 0xF,
  EfiProcessorSocket754              = 0x10,
  EfiProcessorSocket940              = 0x11,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorSocket939              = 0x12,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorSocketmPGA604          = 0x13,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorSocketLGA771           = 0x14,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in ProcSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiProcessorSocketLGA775           = 0x15

} EFI_PROCESSOR_SOCKET_TYPE_DATA;

typedef STRING_REF                  EFI_PROCESSOR_SOCKET_NAME_DATA;

///
/// Inconsistent with specification here:
/// In ProcSubclass specification 0.9, the naming is EFI_PROCESSOR_CACHE_ASSOCIATION_DATA.
/// Keep it unchanged for backward compatibilty.
///
typedef EFI_INTER_LINK_DATA         EFI_CACHE_ASSOCIATION_DATA;

///
/// This data record refers to the health status of the processor.
///
/// Inconsistent with specification here:
/// In ProcSubclass specification 0.9, the naming is EFI_PROCESSOR_HEALTH_STATUS_DATA.
/// Keep it unchanged for backward compatibilty.
///
typedef enum {
  EfiProcessorHealthy        = 1,
  EfiProcessorPerfRestricted = 2,
  EfiProcessorFuncRestricted = 3
} EFI_PROCESSOR_HEALTH_STATUS;

///
/// This data record refers to the package number of this processor. Multiple logical processors can
/// exist in a system and each logical processor can be correlated to the physical processor using this
/// record type.
///
typedef UINTN                       EFI_PROCESSOR_PACKAGE_NUMBER_DATA;

typedef UINT8                       EFI_PROCESSOR_CORE_COUNT_DATA;
typedef UINT8                       EFI_PROCESSOR_ENABLED_CORE_COUNT_DATA;
typedef UINT8                       EFI_PROCESSOR_THREAD_COUNT_DATA;

typedef struct {
  UINT16  Reserved              :1;
  UINT16  Unknown               :1;
  UINT16  Capable64Bit          :1;
  UINT16  Reserved2             :13;
} EFI_PROCESSOR_CHARACTERISTICS_DATA;

///
/// Inconsistent with specification here:
/// In ProcSubclass specification 0.9, the enumeration type data structure is NOT defined.
/// The equivalent in specification is
///      #define EFI_PROCESSOR_FREQUENCY_RECORD_NUMBER           0x00000001
///      #define EFI_PROCESSOR_BUS_FREQUENCY_RECORD_NUMBER       0x00000002
///      #define EFI_PROCESSOR_VERSION_RECORD_NUMBER             0x00000003
///      #define EFI_PROCESSOR_MANUFACTURER_RECORD_NUMBER        0x00000004
///      #define EFI_PROCESSOR_SERIAL_NUMBER_RECORD_NUMBER       0x00000005
///      #define EFI_PROCESSOR_ID_RECORD_NUMBER                  0x00000006
///      #define EFI_PROCESSOR_TYPE_RECORD_NUMBER                0x00000007
///      #define EFI_PROCESSOR_FAMILY_RECORD_NUMBER              0x00000008
///      #define EFI_PROCESSOR_VOLTAGE_RECORD_NUMBER             0x00000009
///      #define EFI_PROCESSOR_APIC_BASE_ADDRESS_RECORD_NUMBER   0x0000000A
///      #define EFI_PROCESSOR_APIC_ID_RECORD_NUMBER             0x0000000B
///      #define EFI_PROCESSOR_APIC_VER_NUMBER_RECORD_NUMBER     0x0000000C
///      #define EFI_PROCESSOR_MICROCODE_REVISION_RECORD_NUMBER  0x0000000D
///      #define EFI_PROCESSOR_STATUS_RECORD_NUMBER              0x0000000E
///      #define EFI_PROCESSOR_SOCKET_TYPE_RECORD_NUMBER         0x0000000F
///      #define EFI_PROCESSOR_SOCKET_NAME_RECORD_NUMBER         0x00000010
///      #define EFI_PROCESSOR_CACHE_ASSOCIATION_RECORD_NUMBER   0x00000011
///      #define EFI_PROCESSOR_MAX_FREQUENCY_RECORD_NUMBER       0x00000012
///      #define EFI_PROCESSOR_ASSET_TAG_RECORD_NUMBER           0x00000013
///      #define EFI_PROCESSOR_MAX_FSB_FREQUENCY_RECORD_NUMBER   0x00000014
///      #define EFI_PROCESSOR_PACKAGE_NUMBER_RECORD_NUMBER      0x00000015
///      #define EFI_PROCESSOR_FREQUENCY_LIST_RECORD_NUMBER      0x00000016
///      #define EFI_PROCESSOR_FSB_FREQUENCY_LIST_RECORD_NUMBER  0x00000017
///      #define EFI_PROCESSOR_HEALTH_STATUS_RECORD_NUMBER       0x00000018
///
/// Keep the definition unchanged for backward compatibility.
typedef enum {
  ProcessorCoreFrequencyRecordType     = 1,
  ProcessorFsbFrequencyRecordType      = 2,
  ProcessorVersionRecordType           = 3,
  ProcessorManufacturerRecordType      = 4,
  ProcessorSerialNumberRecordType      = 5,
  ProcessorIdRecordType                = 6,
  ProcessorTypeRecordType              = 7,
  ProcessorFamilyRecordType            = 8,
  ProcessorVoltageRecordType           = 9,
  ProcessorApicBaseAddressRecordType   = 10,
  ProcessorApicIdRecordType            = 11,
  ProcessorApicVersionNumberRecordType = 12,
  CpuUcodeRevisionDataRecordType       = 13,
  ProcessorStatusRecordType            = 14,
  ProcessorSocketTypeRecordType        = 15,
  ProcessorSocketNameRecordType        = 16,
  CacheAssociationRecordType           = 17,
  ProcessorMaxCoreFrequencyRecordType  = 18,
  ProcessorAssetTagRecordType          = 19,
  ProcessorMaxFsbFrequencyRecordType   = 20,
  ProcessorPackageNumberRecordType     = 21,
  ProcessorCoreFrequencyListRecordType = 22,
  ProcessorFsbFrequencyListRecordType  = 23,
  ProcessorHealthStatusRecordType      = 24,
  ProcessorCoreCountRecordType         = 25,
  ProcessorEnabledCoreCountRecordType  = 26,
  ProcessorThreadCountRecordType       = 27,
  ProcessorCharacteristicsRecordType   = 28,
  ProcessorFamily2RecordType           = 29,
  ProcessorPartNumberRecordType        = 30,
} EFI_CPU_VARIABLE_RECORD_TYPE;

///
/// Inconsistent with specification here:
/// In ProcSubclass specification 0.9, the union type data structure is NOT defined.
/// It's implementation-specific to simplify the code logic.
///
typedef union {
  EFI_PROCESSOR_CORE_FREQUENCY_LIST_DATA  ProcessorCoreFrequencyList;
  EFI_PROCESSOR_FSB_FREQUENCY_LIST_DATA   ProcessorFsbFrequencyList;
  EFI_PROCESSOR_SERIAL_NUMBER_DATA        ProcessorSerialNumber;
  EFI_PROCESSOR_CORE_FREQUENCY_DATA       ProcessorCoreFrequency;
  EFI_PROCESSOR_FSB_FREQUENCY_DATA        ProcessorFsbFrequency;
  EFI_PROCESSOR_MAX_CORE_FREQUENCY_DATA   ProcessorMaxCoreFrequency;
  EFI_PROCESSOR_MAX_FSB_FREQUENCY_DATA    ProcessorMaxFsbFrequency;
  EFI_PROCESSOR_VERSION_DATA              ProcessorVersion;
  EFI_PROCESSOR_MANUFACTURER_DATA         ProcessorManufacturer;
  EFI_PROCESSOR_ID_DATA                   ProcessorId;
  EFI_PROCESSOR_TYPE_DATA                 ProcessorType;
  EFI_PROCESSOR_FAMILY_DATA               ProcessorFamily;
  EFI_PROCESSOR_VOLTAGE_DATA              ProcessorVoltage;
  EFI_PROCESSOR_APIC_BASE_ADDRESS_DATA    ProcessorApicBase;
  EFI_PROCESSOR_APIC_ID_DATA              ProcessorApicId;
  EFI_PROCESSOR_APIC_VERSION_NUMBER_DATA  ProcessorApicVersionNumber;
  EFI_PROCESSOR_MICROCODE_REVISION_DATA   CpuUcodeRevisionData;
  EFI_PROCESSOR_STATUS_DATA               ProcessorStatus;
  EFI_PROCESSOR_SOCKET_TYPE_DATA          ProcessorSocketType;
  EFI_PROCESSOR_SOCKET_NAME_DATA          ProcessorSocketName;
  EFI_PROCESSOR_ASSET_TAG_DATA            ProcessorAssetTag;
  EFI_PROCESSOR_PART_NUMBER_DATA          ProcessorPartNumber;
  EFI_PROCESSOR_HEALTH_STATUS             ProcessorHealthStatus;
  EFI_PROCESSOR_PACKAGE_NUMBER_DATA       ProcessorPackageNumber;
  EFI_PROCESSOR_CORE_COUNT_DATA           ProcessorCoreCount;
  EFI_PROCESSOR_ENABLED_CORE_COUNT_DATA   ProcessorEnabledCoreCount;
  EFI_PROCESSOR_THREAD_COUNT_DATA         ProcessorThreadCount;
  EFI_PROCESSOR_CHARACTERISTICS_DATA      ProcessorCharacteristics;
  EFI_PROCESSOR_FAMILY2_DATA              ProcessorFamily2;
} EFI_CPU_VARIABLE_RECORD;

typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER         DataRecordHeader;
  EFI_CPU_VARIABLE_RECORD           VariableRecord;
} EFI_CPU_DATA_RECORD;

#define EFI_CACHE_SUBCLASS_VERSION    0x00010000

typedef EFI_EXP_BASE2_DATA          EFI_CACHE_SIZE_DATA;
///
/// Inconsistent with specification here:
/// In CacheSubclass specification 0.9, the naming is EFI_CACHE_MAXIMUM_SIZE_DATA.
/// Keep it unchanged for backward compatibilty.
///
typedef EFI_EXP_BASE2_DATA          EFI_MAXIMUM_CACHE_SIZE_DATA;
typedef EFI_EXP_BASE10_DATA         EFI_CACHE_SPEED_DATA;
typedef STRING_REF                  EFI_CACHE_SOCKET_DATA;

typedef struct {
  UINT32                            Other         :1;
  UINT32                            Unknown       :1;
  UINT32                            NonBurst      :1;
  UINT32                            Burst         :1;
  UINT32                            PipelineBurst :1;
  ///
  /// Inconsistent between CacheSubclass 0.9 and SMBIOS specifications here:
  /// In CacheSubclass specification 0.9, the sequence of Asynchronous and Synchronous fileds
  /// are opposite to SMBIOS specification.
  ///
  UINT32                            Asynchronous  :1;
  UINT32                            Synchronous   :1;
  UINT32                            Reserved      :25;
} EFI_CACHE_SRAM_TYPE_DATA;

typedef EFI_CACHE_SRAM_TYPE_DATA    EFI_CACHE_SRAM_INSTALL_DATA;

typedef enum {
  EfiCacheErrorOther     = 1,
  EfiCacheErrorUnknown   = 2,
  EfiCacheErrorNone      = 3,
  EfiCacheErrorParity    = 4,
  EfiCacheErrorSingleBit = 5,
  EfiCacheErrorMultiBit  = 6
} EFI_CACHE_ERROR_TYPE_DATA;

typedef enum {
  EfiCacheTypeOther       = 1,
  EfiCacheTypeUnknown     = 2,
  EfiCacheTypeInstruction = 3,
  EfiCacheTypeData        = 4,
  EfiCacheTypeUnified     = 5
} EFI_CACHE_TYPE_DATA;

typedef enum {
  EfiCacheAssociativityOther        = 1,
  EfiCacheAssociativityUnknown      = 2,
  EfiCacheAssociativityDirectMapped = 3,
  EfiCacheAssociativity2Way         = 4,
  EfiCacheAssociativity4Way         = 5,
  EfiCacheAssociativityFully        = 6,
  EfiCacheAssociativity8Way         = 7,
  EfiCacheAssociativity16Way        = 8
} EFI_CACHE_ASSOCIATIVITY_DATA;

///
/// Inconsistent with specification here:
/// In CacheSubclass 0.9 specification. It defines the field type as UINT16.
/// In fact, it should be UINT32 type because it refers to a 32bit width data.
///
typedef struct {
  UINT32                            Level           :3;
  UINT32                            Socketed        :1;
  UINT32                            Reserved2       :1;
  UINT32                            Location        :2;
  UINT32                            Enable          :1;
  UINT32                            OperationalMode :2;
  UINT32                            Reserved1       :22;
} EFI_CACHE_CONFIGURATION_DATA;

#define EFI_CACHE_L1            1
#define EFI_CACHE_L2            2
#define EFI_CACHE_L3            3
#define EFI_CACHE_L4            4
#define EFI_CACHE_LMAX          EFI_CACHE_L4

#define EFI_CACHE_SOCKETED      1
#define EFI_CACHE_NOT_SOCKETED  0

typedef enum {
  EfiCacheInternal = 0,
  EfiCacheExternal = 1,
  EfiCacheReserved = 2,
  EfiCacheUnknown  = 3
} EFI_CACHE_LOCATION;

#define EFI_CACHE_ENABLED       1
#define EFI_CACHE_DISABLED      0

typedef enum {
  EfiCacheWriteThrough = 0,
  EfiCacheWriteBack    = 1,
  EfiCacheDynamicMode  = 2,
  EfiCacheUnknownMode  = 3
} EFI_CACHE_OPERATIONAL_MODE;


///
/// Inconsistent with specification here:
/// In CacheSubclass specification 0.9, the enumeration type data structure is NOT defined.
/// The equivalent in specification is
///      #define EFI_CACHE_SIZE_RECORD_NUMBER                    0x00000001
///      #define EFI_CACHE_MAXIMUM_SIZE_RECORD_NUMBER            0x00000002
///      #define EFI_CACHE_SPEED_RECORD_NUMBER                   0x00000003
///      #define EFI_CACHE_SOCKET_RECORD_NUMBER                  0x00000004
///      #define EFI_CACHE_SRAM_SUPPORT_RECORD_NUMBER            0x00000005
///      #define EFI_CACHE_SRAM_INSTALL_RECORD_NUMBER            0x00000006
///      #define EFI_CACHE_ERROR_SUPPORT_RECORD_NUMBER           0x00000007
///      #define EFI_CACHE_TYPE_RECORD_NUMBER                    0x00000008
///      #define EFI_CACHE_ASSOCIATIVITY_RECORD_NUMBER           0x00000009
///      #define EFI_CACHE_CONFIGURATION_RECORD_NUMBER           0x0000000A
/// Keep the definition unchanged for backward compatibility.
///
typedef enum {
  CacheSizeRecordType              = 1,
  MaximumSizeCacheRecordType       = 2,
  CacheSpeedRecordType             = 3,
  CacheSocketRecordType            = 4,
  CacheSramTypeRecordType          = 5,
  CacheInstalledSramTypeRecordType = 6,
  CacheErrorTypeRecordType         = 7,
  CacheTypeRecordType              = 8,
  CacheAssociativityRecordType     = 9,
  CacheConfigRecordType            = 10
} EFI_CACHE_VARIABLE_RECORD_TYPE;

///
/// Inconsistent with specification here:
/// In CacheSubclass specification 0.9, the union type data structure is NOT defined.
/// It's implementation-specific to simplify the code logic.
///
typedef union {
  EFI_CACHE_SIZE_DATA                         CacheSize;
  EFI_MAXIMUM_CACHE_SIZE_DATA                 MaximumCacheSize;
  EFI_CACHE_SPEED_DATA                        CacheSpeed;
  EFI_CACHE_SOCKET_DATA                       CacheSocket;
  EFI_CACHE_SRAM_TYPE_DATA                    CacheSramType;
  EFI_CACHE_SRAM_TYPE_DATA                    CacheInstalledSramType;
  EFI_CACHE_ERROR_TYPE_DATA                   CacheErrorType;
  EFI_CACHE_TYPE_DATA                         CacheType;
  EFI_CACHE_ASSOCIATIVITY_DATA                CacheAssociativity;
  EFI_CACHE_CONFIGURATION_DATA                CacheConfig;
  EFI_CACHE_ASSOCIATION_DATA                  CacheAssociation;
} EFI_CACHE_VARIABLE_RECORD;

typedef struct {
   EFI_SUBCLASS_TYPE1_HEADER        DataRecordHeader;
   EFI_CACHE_VARIABLE_RECORD        VariableRecord;
} EFI_CACHE_DATA_RECORD;

#define EFI_MEMORY_SUBCLASS_VERSION     0x0100
#define EFI_MEMORY_SIZE_RECORD_NUMBER   0x00000001

typedef enum _EFI_MEMORY_REGION_TYPE {
  EfiMemoryRegionMemory             = 0x01,
  EfiMemoryRegionReserved           = 0x02,
  EfiMemoryRegionAcpi               = 0x03,
  EfiMemoryRegionNvs                = 0x04
} EFI_MEMORY_REGION_TYPE;

///
/// This data record refers to the size of a memory region. The regions that are
/// described can refer to physical memory, memory-mapped I/O, or reserved BIOS memory regions.
/// The unit of measurement of this data record is in bytes.
///
typedef struct {
  ///
  /// A zero-based value that indicates which processor(s) can access the memory region.
  /// A value of 0xFFFF indicates the region is accessible by all processors.
  ///
  UINT32                            ProcessorNumber;
  ///
  /// A zero-based value that indicates the starting bus that can access the memory region.
  ///
  UINT16                            StartBusNumber;
  ///
  /// A zero-based value that indicates the ending bus that can access the memory region.
  /// A value of 0xFF for a PCI system indicates the region is accessible by all buses and
  /// is global in scope. An example of the EndBusNumber not being 0xFF is a system
  /// with two or more peer-to-host PCI bridges.
  ///
  UINT16                            EndBusNumber;
  ///
  /// The type of memory region from the operating system's point of view.
  /// MemoryRegionType values are equivalent to the legacy INT 15 AX = E820 BIOS
  /// command values.
  ///
  EFI_MEMORY_REGION_TYPE            MemoryRegionType;
  ///
  /// The size of the memory region in bytes.
  ///
  EFI_EXP_BASE2_DATA                MemorySize;
  ///
  /// The starting physical address of the memory region.
  ///
  EFI_PHYSICAL_ADDRESS              MemoryStartAddress;
} EFI_MEMORY_SIZE_DATA;


#define EFI_MEMORY_ARRAY_LOCATION_RECORD_NUMBER    0x00000002

typedef enum _EFI_MEMORY_ARRAY_LOCATION {
  EfiMemoryArrayLocationOther                 = 0x01,
  EfiMemoryArrayLocationUnknown               = 0x02,
  EfiMemoryArrayLocationSystemBoard           = 0x03,
  EfiMemoryArrayLocationIsaAddonCard          = 0x04,
  EfiMemoryArrayLocationEisaAddonCard         = 0x05,
  EfiMemoryArrayLocationPciAddonCard          = 0x06,
  EfiMemoryArrayLocationMcaAddonCard          = 0x07,
  EfiMemoryArrayLocationPcmciaAddonCard       = 0x08,
  EfiMemoryArrayLocationProprietaryAddonCard  = 0x09,
  EfiMemoryArrayLocationNuBus                 = 0x0A,
  EfiMemoryArrayLocationPc98C20AddonCard      = 0xA0,
  EfiMemoryArrayLocationPc98C24AddonCard      = 0xA1,
  EfiMemoryArrayLocationPc98EAddonCard        = 0xA2,
  EfiMemoryArrayLocationPc98LocalBusAddonCard = 0xA3
} EFI_MEMORY_ARRAY_LOCATION;

typedef enum _EFI_MEMORY_ARRAY_USE {
  EfiMemoryArrayUseOther                      = 0x01,
  EfiMemoryArrayUseUnknown                    = 0x02,
  EfiMemoryArrayUseSystemMemory               = 0x03,
  EfiMemoryArrayUseVideoMemory                = 0x04,
  EfiMemoryArrayUseFlashMemory                = 0x05,
  EfiMemoryArrayUseNonVolatileRam             = 0x06,
  EfiMemoryArrayUseCacheMemory                = 0x07
} EFI_MEMORY_ARRAY_USE;

typedef enum _EFI_MEMORY_ERROR_CORRECTION {
  EfiMemoryErrorCorrectionOther               = 0x01,
  EfiMemoryErrorCorrectionUnknown             = 0x02,
  EfiMemoryErrorCorrectionNone                = 0x03,
  EfiMemoryErrorCorrectionParity              = 0x04,
  EfiMemoryErrorCorrectionSingleBitEcc        = 0x05,
  EfiMemoryErrorCorrectionMultiBitEcc         = 0x06,
  EfiMemoryErrorCorrectionCrc                 = 0x07
} EFI_MEMORY_ERROR_CORRECTION;

///
/// This data record refers to the physical memory array. This data record is a structure.
/// The type definition structure for EFI_MEMORY_ARRAY_LOCATION_DATA is in SMBIOS 2.3.4:
/// - Table 3.3.17.1, Type 16, Offset 0x4
/// - Table 3.3.17.2, Type 16, Offset 0x5
/// - Table 3.3.17.3, Type 16, with the following offsets:
///     -- Offset 0x6
///     -- Offset 0x7
///     -- Offset 0xB
///     -- Offset 0xD
///
typedef struct {
  ///
  /// The physical location of the memory array.
  ///
  EFI_MEMORY_ARRAY_LOCATION         MemoryArrayLocation;
  ///
  /// The memory array usage.
  ///
  EFI_MEMORY_ARRAY_USE              MemoryArrayUse;
  ///
  /// The primary error correction or detection supported by this memory array.
  ///
  EFI_MEMORY_ERROR_CORRECTION       MemoryErrorCorrection;
  ///
  /// The maximum memory capacity size in kilobytes. If capacity is unknown, then
  /// values of MaximumMemoryCapacity.Value = 0x00 and
  /// MaximumMemoryCapacity.Exponent = 0x8000 are used.
  ///
  EFI_EXP_BASE2_DATA                MaximumMemoryCapacity;
  ///
  /// The number of memory slots or sockets that are available for memory devices
  /// in this array.
  ///
  UINT16                            NumberMemoryDevices;
} EFI_MEMORY_ARRAY_LOCATION_DATA;


#define EFI_MEMORY_ARRAY_LINK_RECORD_NUMBER    0x00000003

typedef enum _EFI_MEMORY_FORM_FACTOR {
  EfiMemoryFormFactorOther                    = 0x01,
  EfiMemoryFormFactorUnknown                  = 0x02,
  EfiMemoryFormFactorSimm                     = 0x03,
  EfiMemoryFormFactorSip                      = 0x04,
  EfiMemoryFormFactorChip                     = 0x05,
  EfiMemoryFormFactorDip                      = 0x06,
  EfiMemoryFormFactorZip                      = 0x07,
  EfiMemoryFormFactorProprietaryCard          = 0x08,
  EfiMemoryFormFactorDimm                     = 0x09,
  EfiMemoryFormFactorTsop                     = 0x0A,
  EfiMemoryFormFactorRowOfChips               = 0x0B,
  EfiMemoryFormFactorRimm                     = 0x0C,
  EfiMemoryFormFactorSodimm                   = 0x0D,
  EfiMemoryFormFactorSrimm                    = 0x0E,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in MemSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiMemoryFormFactorFbDimm                   = 0x0F
} EFI_MEMORY_FORM_FACTOR;

typedef enum _EFI_MEMORY_ARRAY_TYPE {
  EfiMemoryTypeOther                          = 0x01,
  EfiMemoryTypeUnknown                        = 0x02,
  EfiMemoryTypeDram                           = 0x03,
  EfiMemoryTypeEdram                          = 0x04,
  EfiMemoryTypeVram                           = 0x05,
  EfiMemoryTypeSram                           = 0x06,
  EfiMemoryTypeRam                            = 0x07,
  EfiMemoryTypeRom                            = 0x08,
  EfiMemoryTypeFlash                          = 0x09,
  EfiMemoryTypeEeprom                         = 0x0A,
  EfiMemoryTypeFeprom                         = 0x0B,
  EfiMemoryTypeEprom                          = 0x0C,
  EfiMemoryTypeCdram                          = 0x0D,
  EfiMemoryType3Dram                          = 0x0E,
  EfiMemoryTypeSdram                          = 0x0F,
  EfiMemoryTypeSgram                          = 0x10,
  EfiMemoryTypeRdram                          = 0x11,
  EfiMemoryTypeDdr                            = 0x12,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in MemSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiMemoryTypeDdr2                           = 0x13,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in MemSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiMemoryTypeDdr2FbDimm                     = 0x14
} EFI_MEMORY_ARRAY_TYPE;

typedef struct {
  UINT32                            Reserved        :1;
  UINT32                            Other           :1;
  UINT32                            Unknown         :1;
  UINT32                            FastPaged       :1;
  UINT32                            StaticColumn    :1;
  UINT32                            PseudoStatic    :1;
  UINT32                            Rambus          :1;
  UINT32                            Synchronous     :1;
  UINT32                            Cmos            :1;
  UINT32                            Edo             :1;
  UINT32                            WindowDram      :1;
  UINT32                            CacheDram       :1;
  UINT32                            Nonvolatile     :1;
  UINT32                            Reserved1       :19;
} EFI_MEMORY_TYPE_DETAIL;

typedef enum {
  EfiMemoryStateEnabled      = 0,
  EfiMemoryStateUnknown      = 1,
  EfiMemoryStateUnsupported  = 2,
  EfiMemoryStateError        = 3,
  EfiMemoryStateAbsent       = 4,
  EfiMemoryStateDisabled     = 5,
  ///
  /// Inconsistent with specification here:
  /// This field is NOT defined in MemSubClass specification 0.9. It's introduced for SMBIOS2.6 specification.
  ///
  EfiMemoryStatePartial      = 6
} EFI_MEMORY_STATE;

///
/// This data record describes a memory device. This data record is a structure.
/// The type definition structure for EFI_MEMORY_ARRAY_LINK_DATA is in SMBIOS 2.3.4.
///
typedef struct {
  ///
  /// A string that identifies the physically labeled socket or board position where the
  /// memory device is located.
  ///
  STRING_REF                        MemoryDeviceLocator;
  ///
  /// A string denoting the physically labeled bank where the memory device is located.
  ///
  STRING_REF                        MemoryBankLocator;
  ///
  /// A string denoting the memory manufacturer.
  ///
  STRING_REF                        MemoryManufacturer;
  ///
  /// A string denoting the serial number of the memory device.
  ///
  STRING_REF                        MemorySerialNumber;
  ///
  /// The asset tag of the memory device.
  ///
  STRING_REF                        MemoryAssetTag;
  ///
  /// A string denoting the part number of the memory device.
  ///
  STRING_REF                        MemoryPartNumber;
  ///
  /// A link to a memory array structure set.
  ///
  EFI_INTER_LINK_DATA               MemoryArrayLink;
  ///
  /// A link to a memory array structure set.
  ///
  EFI_INTER_LINK_DATA               MemorySubArrayLink;
  ///
  /// The total width in bits of this memory device. If there are no error correcting bits,
  /// then the total width equals the data width. If the width is unknown, then set the field
  /// to 0xFFFF.
  ///
  UINT16                            MemoryTotalWidth;
  ///
  /// The data width in bits of the memory device. A data width of 0x00 and a total width
  /// of 0x08 indicate that the device is used solely for error correction.
  ///
  UINT16                            MemoryDataWidth;
  ///
  /// The size in bytes of the memory device. A value of 0x00 denotes that no device is
  /// installed, while a value of all Fs denotes that the size is not known.
  ///
  EFI_EXP_BASE2_DATA                MemoryDeviceSize;
  ///
  /// The form factor of the memory device.
  ///
  EFI_MEMORY_FORM_FACTOR            MemoryFormFactor;
  ///
  /// A memory device set that must be populated with all devices of the same type and
  /// size. A value of 0x00 indicates that the device is not part of any set. A value of 0xFF
  /// indicates that the attribute is unknown. Any other value denotes the set number.
  ///
  UINT8                             MemoryDeviceSet;
  ///
  /// The memory type in the socket.
  ///
  EFI_MEMORY_ARRAY_TYPE             MemoryType;
  ///
  /// The memory type details.
  ///
  EFI_MEMORY_TYPE_DETAIL            MemoryTypeDetail;
  ///
  /// The memory speed in megahertz (MHz). A value of 0x00 denotes that
  /// the speed is unknown.
  /// Inconsistent with specification here:
  /// In MemSubclass specification 0.9, the naming is MemoryTypeSpeed.
  /// Keep it unchanged for backward compatibilty.
  ///
  EFI_EXP_BASE10_DATA               MemorySpeed;
  ///
  /// The memory state.
  ///
  EFI_MEMORY_STATE                  MemoryState;
} EFI_MEMORY_ARRAY_LINK_DATA;


#define EFI_MEMORY_ARRAY_START_ADDRESS_RECORD_NUMBER    0x00000004

///
/// This data record refers to a specified physical memory array associated with
/// a given memory range.
///
typedef struct {
  ///
  /// The starting physical address in bytes of memory mapped to a specified physical
  /// memory array.
  ///
  EFI_PHYSICAL_ADDRESS              MemoryArrayStartAddress;
  ///
  /// The last physical address in bytes of memory mapped to a specified physical memory
  /// array.
  ///
  EFI_PHYSICAL_ADDRESS              MemoryArrayEndAddress;
  ///
  /// See Physical Memory Array (Type 16) for physical memory array structures.
  ///
  EFI_INTER_LINK_DATA               PhysicalMemoryArrayLink;
  ///
  /// The number of memory devices that form a single row of memory for the address
  /// partition.
  ///
  UINT16                            MemoryArrayPartitionWidth;
} EFI_MEMORY_ARRAY_START_ADDRESS_DATA;


#define EFI_MEMORY_DEVICE_START_ADDRESS_RECORD_NUMBER    0x00000005

///
/// This data record refers to a physical memory device that is associated with
/// a given memory range.
///
typedef struct {
  ///
  /// The starting physical address that is associated with the device.
  ///
  EFI_PHYSICAL_ADDRESS              MemoryDeviceStartAddress;
  ///
  /// The ending physical address that is associated with the device.
  ///
  EFI_PHYSICAL_ADDRESS              MemoryDeviceEndAddress;
  ///
  /// A link to the memory device data structure.
  ///
  EFI_INTER_LINK_DATA               PhysicalMemoryDeviceLink;
  ///
  /// A link to the memory array data structure.
  ///
  EFI_INTER_LINK_DATA               PhysicalMemoryArrayLink;
  ///
  /// The position of the memory device in a row. A value of 0x00 is reserved and a value
  /// of 0xFF indicates that the position is unknown.
  ///
  UINT8                             MemoryDevicePartitionRowPosition;
  ///
  /// The position of the device in an interleave.
  ///
  UINT8                             MemoryDeviceInterleavePosition;
  ///
  /// The maximum number of consecutive rows from the device that are accessed in a
  /// single interleave transfer. A value of 0x00 indicates that the device is not interleaved
  /// and a value of 0xFF indicates that the interleave configuration is unknown.
  ///
  UINT8                             MemoryDeviceInterleaveDataDepth;
} EFI_MEMORY_DEVICE_START_ADDRESS_DATA;


//
//  Memory. Channel Device Type -  SMBIOS Type 37
//

#define EFI_MEMORY_CHANNEL_TYPE_RECORD_NUMBER    0x00000006

typedef enum _EFI_MEMORY_CHANNEL_TYPE {
  EfiMemoryChannelTypeOther                   = 1,
  EfiMemoryChannelTypeUnknown                 = 2,
  EfiMemoryChannelTypeRambus                  = 3,
  EfiMemoryChannelTypeSyncLink                = 4
} EFI_MEMORY_CHANNEL_TYPE;

///
/// This data record refers the type of memory that is associated with the channel. This data record is a
/// structure.
/// The type definition structure for EFI_MEMORY_CHANNEL_TYPE_DATA is in SMBIOS 2.3.4,
/// Table 3.3.38, Type 37, with the following offsets:
///   - Offset 0x4
///   - Offset 0x5
///   - Offset 0x6
///
typedef struct {
  ///
  /// The type of memory that is associated with the channel.
  ///
  EFI_MEMORY_CHANNEL_TYPE           MemoryChannelType;
  ///
  /// The maximum load that is supported by the channel.
  ///
  UINT8                             MemoryChannelMaximumLoad;
  ///
  /// The number of memory devices on this channel.
  ///
  UINT8                             MemoryChannelDeviceCount;
} EFI_MEMORY_CHANNEL_TYPE_DATA;

#define EFI_MEMORY_CHANNEL_DEVICE_RECORD_NUMBER    0x00000007

///
/// This data record refers to the memory device that is associated with the memory channel. This data
/// record is a structure.
/// The type definition structure for EFI_MEMORY_CHANNEL_DEVICE_DATA is in SMBIOS 2.3.4,
/// Table 3.3.38, Type 37, with the following offsets:
///   - Offset 0x7
///   - Offset 0x8
///
typedef struct {
  ///
  /// A number between one and MemoryChannelDeviceCount plus an arbitrary base.
  ///
  UINT8                             DeviceId;
  ///
  /// The Link of the associated memory device. See Memory Device (Type 17) for
  /// memory devices.
  ///
  EFI_INTER_LINK_DATA               DeviceLink;
  ///
  /// The number of load units that this device consumes.
  ///
  UINT8                             MemoryChannelDeviceLoad;
} EFI_MEMORY_CHANNEL_DEVICE_DATA;

//
//  Memory. Controller Information - SMBIOS Type 5
//
///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 5.
///
#define EFI_MEMORY_CONTROLLER_INFORMATION_RECORD_NUMBER    0x00000008

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 5.
///
typedef enum {
  EfiErrorDetectingMethodOther   = 1,
  EfiErrorDetectingMethodUnknown = 2,
  EfiErrorDetectingMethodNone    = 3,
  EfiErrorDetectingMethodParity  = 4,
  EfiErrorDetectingMethod32Ecc   = 5,
  EfiErrorDetectingMethod64Ecc   = 6,
  EfiErrorDetectingMethod128Ecc  = 7,
  EfiErrorDetectingMethodCrc     = 8
} EFI_MEMORY_ERROR_DETECT_METHOD_TYPE;

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 5.
///
typedef struct {
  UINT8                             Other                 :1;
  UINT8                             Unknown               :1;
  UINT8                             None                  :1;
  UINT8                             SingleBitErrorCorrect :1;
  UINT8                             DoubleBitErrorCorrect :1;
  UINT8                             ErrorScrubbing        :1;
  UINT8                             Reserved              :2;
} EFI_MEMORY_ERROR_CORRECT_CAPABILITY;

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 5.
///
typedef enum {
  EfiMemoryInterleaveOther      = 1,
  EfiMemoryInterleaveUnknown    = 2,
  EfiMemoryInterleaveOneWay     = 3,
  EfiMemoryInterleaveTwoWay     = 4,
  EfiMemoryInterleaveFourWay    = 5,
  EfiMemoryInterleaveEightWay   = 6,
  EfiMemoryInterleaveSixteenWay = 7
} EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE;

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 5.
///
typedef struct {
  UINT16                            Other    :1;
  UINT16                            Unknown  :1;
  UINT16                            SeventyNs:1;
  UINT16                            SixtyNs  :1;
  UINT16                            FiftyNs  :1;
  UINT16                            Reserved :11;
} EFI_MEMORY_SPEED_TYPE;

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 5.
///
typedef struct {
  UINT16                            Other       :1;
  UINT16                            Unknown     :1;
  UINT16                            Standard    :1;
  UINT16                            FastPageMode:1;
  UINT16                            EDO         :1;
  UINT16                            Parity      :1;
  UINT16                            ECC         :1;
  UINT16                            SIMM        :1;
  UINT16                            DIMM        :1;
  UINT16                            BurstEdo    :1;
  UINT16                            SDRAM       :1;
  UINT16                            Reserved    :5;
} EFI_MEMORY_SUPPORTED_TYPE;

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 5.
///
typedef struct {
  UINT8                             Five    :1;
  UINT8                             Three   :1;
  UINT8                             Two     :1;
  UINT8                             Reserved:5;
} EFI_MEMORY_MODULE_VOLTAGE_TYPE;

///
/// EFI_MEMORY_CONTROLLER_INFORMATION is obsolete
/// Use EFI_MEMORY_CONTROLLER_INFORMATION_DATA instead
///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 5.
///
typedef struct {
  EFI_MEMORY_ERROR_DETECT_METHOD_TYPE ErrorDetectingMethod;
  EFI_MEMORY_ERROR_CORRECT_CAPABILITY ErrorCorrectingCapability;
  EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE  MemorySupportedInterleave;
  EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE  MemoryCurrentInterleave;
  UINT8                               MaxMemoryModuleSize;
  EFI_MEMORY_SPEED_TYPE               MemorySpeedType;
  EFI_MEMORY_SUPPORTED_TYPE           MemorySupportedType;
  EFI_MEMORY_MODULE_VOLTAGE_TYPE      MemoryModuleVoltage;
  UINT8                               NumberofMemorySlot;
  EFI_MEMORY_ERROR_CORRECT_CAPABILITY EnabledCorrectingCapability;
  UINT16                              *MemoryModuleConfigHandles;
} EFI_MEMORY_CONTROLLER_INFORMATION;

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 5.
///
typedef struct {
  EFI_MEMORY_ERROR_DETECT_METHOD_TYPE   ErrorDetectingMethod;
  EFI_MEMORY_ERROR_CORRECT_CAPABILITY   ErrorCorrectingCapability;
  EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE    MemorySupportedInterleave;
  EFI_MEMORY_SUPPORT_INTERLEAVE_TYPE    MemoryCurrentInterleave;
  UINT8                                 MaxMemoryModuleSize;
  EFI_MEMORY_SPEED_TYPE                 MemorySpeedType;
  EFI_MEMORY_SUPPORTED_TYPE             MemorySupportedType;
  EFI_MEMORY_MODULE_VOLTAGE_TYPE        MemoryModuleVoltage;
  UINT8                                 NumberofMemorySlot;
  EFI_MEMORY_ERROR_CORRECT_CAPABILITY   EnabledCorrectingCapability;
  EFI_INTER_LINK_DATA                   MemoryModuleConfig[1];
} EFI_MEMORY_CONTROLLER_INFORMATION_DATA;

///
/// Memory. Error Information - SMBIOS Type 18
///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 18.
///
#define EFI_MEMORY_32BIT_ERROR_INFORMATION_RECORD_NUMBER    0x00000009
///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 18.
///
typedef enum {
  EfiMemoryErrorOther             = 1,
  EfiMemoryErrorUnknown           = 2,
  EfiMemoryErrorOk                = 3,
  EfiMemoryErrorBadRead           = 4,
  EfiMemoryErrorParity            = 5,
  EfiMemoryErrorSigleBit          = 6,
  EfiMemoryErrorDoubleBit         = 7,
  EfiMemoryErrorMultiBit          = 8,
  EfiMemoryErrorNibble            = 9,
  EfiMemoryErrorChecksum          = 10,
  EfiMemoryErrorCrc               = 11,
  EfiMemoryErrorCorrectSingleBit  = 12,
  EfiMemoryErrorCorrected         = 13,
  EfiMemoryErrorUnCorrectable     = 14
} EFI_MEMORY_ERROR_TYPE;
///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 18.
///
typedef enum {
  EfiMemoryGranularityOther               = 1,
  EfiMemoryGranularityOtherUnknown        = 2,
  EfiMemoryGranularityDeviceLevel         = 3,
  EfiMemoryGranularityMemPartitionLevel   = 4
} EFI_MEMORY_ERROR_GRANULARITY_TYPE;
///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 18.
///
typedef enum {
  EfiMemoryErrorOperationOther            = 1,
  EfiMemoryErrorOperationUnknown          = 2,
  EfiMemoryErrorOperationRead             = 3,
  EfiMemoryErrorOperationWrite            = 4,
  EfiMemoryErrorOperationPartialWrite     = 5
} EFI_MEMORY_ERROR_OPERATION_TYPE;
///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 18.
///
typedef struct {
  EFI_MEMORY_ERROR_TYPE               MemoryErrorType;
  EFI_MEMORY_ERROR_GRANULARITY_TYPE   MemoryErrorGranularity;
  EFI_MEMORY_ERROR_OPERATION_TYPE     MemoryErrorOperation;
  UINT32                              VendorSyndrome;
  UINT32                              MemoryArrayErrorAddress;
  UINT32                              DeviceErrorAddress;
  UINT32                              DeviceErrorResolution;
} EFI_MEMORY_32BIT_ERROR_INFORMATION;

///
/// Memory. Error Information - SMBIOS Type 33.
///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 33.
///
#define EFI_MEMORY_64BIT_ERROR_INFORMATION_RECORD_NUMBER    0x0000000A

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 type 33.
///
typedef struct {
  EFI_MEMORY_ERROR_TYPE             MemoryErrorType;
  EFI_MEMORY_ERROR_GRANULARITY_TYPE MemoryErrorGranularity;
  EFI_MEMORY_ERROR_OPERATION_TYPE   MemoryErrorOperation;
  UINT32                            VendorSyndrome;
  UINT64                            MemoryArrayErrorAddress;
  UINT64                            DeviceErrorAddress;
  UINT32                            DeviceErrorResolution;
} EFI_MEMORY_64BIT_ERROR_INFORMATION;

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It is implementation-specific to simplify the code logic.
///
typedef union _EFI_MEMORY_SUBCLASS_RECORDS {
  EFI_MEMORY_SIZE_DATA                 SizeData;
  EFI_MEMORY_ARRAY_LOCATION_DATA       ArrayLocationData;
  EFI_MEMORY_ARRAY_LINK_DATA           ArrayLink;
  EFI_MEMORY_ARRAY_START_ADDRESS_DATA  ArrayStartAddress;
  EFI_MEMORY_DEVICE_START_ADDRESS_DATA DeviceStartAddress;
  EFI_MEMORY_CHANNEL_TYPE_DATA         ChannelTypeData;
  EFI_MEMORY_CHANNEL_DEVICE_DATA       ChannelDeviceData;
  EFI_MEMORY_CONTROLLER_INFORMATION    MemoryControllerInfo;
  EFI_MEMORY_32BIT_ERROR_INFORMATION   Memory32bitErrorInfo;
  EFI_MEMORY_64BIT_ERROR_INFORMATION   Memory64bitErrorInfo;
} EFI_MEMORY_SUBCLASS_RECORDS;

typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER         Header;
  EFI_MEMORY_SUBCLASS_RECORDS       Record;
} EFI_MEMORY_SUBCLASS_DRIVER_DATA;

#define EFI_MISC_SUBCLASS_VERSION     0x0100

#pragma pack(1)

//
// Last PCI Bus Number
//
#define EFI_MISC_LAST_PCI_BUS_RECORD_NUMBER    0x00000001

typedef struct {
  UINT8                             LastPciBus;
} EFI_MISC_LAST_PCI_BUS_DATA;

//
// Misc. BIOS Vendor - SMBIOS Type 0
//
#define EFI_MISC_BIOS_VENDOR_RECORD_NUMBER    0x00000002

typedef struct {
  UINT64                            Reserved1                         :2;
  UINT64                            Unknown                           :1;
  UINT64                            BiosCharacteristicsNotSupported   :1;
  UINT64                            IsaIsSupported                    :1;
  UINT64                            McaIsSupported                    :1;
  UINT64                            EisaIsSupported                   :1;
  UINT64                            PciIsSupported                    :1;
  UINT64                            PcmciaIsSupported                 :1;
  UINT64                            PlugAndPlayIsSupported            :1;
  UINT64                            ApmIsSupported                    :1;
  UINT64                            BiosIsUpgradable                  :1;
  UINT64                            BiosShadowingAllowed              :1;
  UINT64                            VlVesaIsSupported                 :1;
  UINT64                            EscdSupportIsAvailable            :1;
  UINT64                            BootFromCdIsSupported             :1;
  UINT64                            SelectableBootIsSupported         :1;
  UINT64                            RomBiosIsSocketed                 :1;
  UINT64                            BootFromPcmciaIsSupported         :1;
  UINT64                            EDDSpecificationIsSupported       :1;
  UINT64                            JapaneseNecFloppyIsSupported      :1;
  UINT64                            JapaneseToshibaFloppyIsSupported  :1;
  UINT64                            Floppy525_360IsSupported          :1;
  UINT64                            Floppy525_12IsSupported           :1;
  UINT64                            Floppy35_720IsSupported           :1;
  UINT64                            Floppy35_288IsSupported           :1;
  UINT64                            PrintScreenIsSupported            :1;
  UINT64                            Keyboard8042IsSupported           :1;
  UINT64                            SerialIsSupported                 :1;
  UINT64                            PrinterIsSupported                :1;
  UINT64                            CgaMonoIsSupported                :1;
  UINT64                            NecPc98                           :1;
  UINT64                            AcpiIsSupported                   :1;
  UINT64                            UsbLegacyIsSupported              :1;
  UINT64                            AgpIsSupported                    :1;
  UINT64                            I20BootIsSupported                :1;
  UINT64                            Ls120BootIsSupported              :1;
  UINT64                            AtapiZipDriveBootIsSupported      :1;
  UINT64                            Boot1394IsSupported               :1;
  UINT64                            SmartBatteryIsSupported           :1;
  UINT64                            BiosBootSpecIsSupported           :1;
  UINT64                            FunctionKeyNetworkBootIsSupported :1;
  UINT64                            Reserved                          :22;
} EFI_MISC_BIOS_CHARACTERISTICS;

typedef struct {
  UINT64                            BiosReserved  :16;
  UINT64                            SystemReserved:16;
  UINT64                            Reserved      :32;
} EFI_MISC_BIOS_CHARACTERISTICS_EXTENSION;

typedef struct {
  STRING_REF                        BiosVendor;
  STRING_REF                        BiosVersion;
  STRING_REF                        BiosReleaseDate;
  EFI_PHYSICAL_ADDRESS              BiosStartingAddress;
  EFI_EXP_BASE2_DATA                BiosPhysicalDeviceSize;
  EFI_MISC_BIOS_CHARACTERISTICS     BiosCharacteristics1;
  EFI_MISC_BIOS_CHARACTERISTICS_EXTENSION
                                    BiosCharacteristics2;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, this data structure and corrsponding fields are NOT defined.
  /// It's introduced for SmBios 2.6 specification type 0.
  ///
  UINT8                             BiosMajorRelease;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, this data structure and corrsponding fields are NOT defined.
  /// It's introduced for SmBios 2.6 specification type 0.
  ///
  UINT8                             BiosMinorRelease;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, this data structure and corrsponding fields are NOT defined.
  /// It's introduced for SmBios 2.6 specification type 0.
  ///
  UINT8                             BiosEmbeddedFirmwareMajorRelease;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, this data structure and corrsponding fields are NOT defined.
  /// It's introduced for SmBios 2.6 specification type 0.
  ///
  UINT8                             BiosEmbeddedFirmwareMinorRelease;
} EFI_MISC_BIOS_VENDOR_DATA;

//
// Misc. System Manufacturer - SMBIOS Type 1
//
#define EFI_MISC_SYSTEM_MANUFACTURER_RECORD_NUMBER    0x00000003

typedef enum {
  EfiSystemWakeupTypeReserved        = 0,
  EfiSystemWakeupTypeOther           = 1,
  EfiSystemWakeupTypeUnknown         = 2,
  EfiSystemWakeupTypeApmTimer        = 3,
  EfiSystemWakeupTypeModemRing       = 4,
  EfiSystemWakeupTypeLanRemote       = 5,
  EfiSystemWakeupTypePowerSwitch     = 6,
  EfiSystemWakeupTypePciPme          = 7,
  EfiSystemWakeupTypeAcPowerRestored = 8
} EFI_MISC_SYSTEM_WAKEUP_TYPE;

typedef struct {
  STRING_REF                        SystemManufacturer;
  STRING_REF                        SystemProductName;
  STRING_REF                        SystemVersion;
  STRING_REF                        SystemSerialNumber;
  EFI_GUID                          SystemUuid;
  EFI_MISC_SYSTEM_WAKEUP_TYPE       SystemWakeupType;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, this data structure and corrsponding fields are NOT defined.
  /// It's introduced for SmBios 2.6 specification type 1.
  ///
  STRING_REF                        SystemSKUNumber;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, this data structure and corrsponding fields are NOT defined.
  /// It's introduced for SmBios 2.6 specification type 1.
  ///
  STRING_REF                        SystemFamily;
} EFI_MISC_SYSTEM_MANUFACTURER_DATA;

//
// Misc. Base Board Manufacturer - SMBIOS Type 2
//
#define EFI_MISC_BASE_BOARD_MANUFACTURER_RECORD_NUMBER    0x00000004

typedef struct {
  UINT32                            Motherboard           :1;
  UINT32                            RequiresDaughterCard  :1;
  UINT32                            Removable             :1;
  UINT32                            Replaceable           :1;
  UINT32                            HotSwappable          :1;
  UINT32                            Reserved              :27;
} EFI_BASE_BOARD_FEATURE_FLAGS;

typedef enum {
  EfiBaseBoardTypeUnknown                = 1,
  EfiBaseBoardTypeOther                  = 2,
  EfiBaseBoardTypeServerBlade            = 3,
  EfiBaseBoardTypeConnectivitySwitch     = 4,
  EfiBaseBoardTypeSystemManagementModule = 5,
  EfiBaseBoardTypeProcessorModule        = 6,
  EfiBaseBoardTypeIOModule               = 7,
  EfiBaseBoardTypeMemoryModule           = 8,
  EfiBaseBoardTypeDaughterBoard          = 9,
  EfiBaseBoardTypeMotherBoard            = 0xA,
  EfiBaseBoardTypeProcessorMemoryModule  = 0xB,
  EfiBaseBoardTypeProcessorIOModule      = 0xC,
  EfiBaseBoardTypeInterconnectBoard      = 0xD
} EFI_BASE_BOARD_TYPE;

typedef struct {
  STRING_REF                        BaseBoardManufacturer;
  STRING_REF                        BaseBoardProductName;
  STRING_REF                        BaseBoardVersion;
  STRING_REF                        BaseBoardSerialNumber;
  STRING_REF                        BaseBoardAssetTag;
  STRING_REF                        BaseBoardChassisLocation;
  EFI_BASE_BOARD_FEATURE_FLAGS      BaseBoardFeatureFlags;
  EFI_BASE_BOARD_TYPE               BaseBoardType;
  EFI_INTER_LINK_DATA               BaseBoardChassisLink;
  UINT32                            BaseBoardNumberLinks;
  EFI_INTER_LINK_DATA               LinkN;
} EFI_MISC_BASE_BOARD_MANUFACTURER_DATA;

//
// Misc. System/Chassis Enclosure - SMBIOS Type 3
//
#define EFI_MISC_CHASSIS_MANUFACTURER_RECORD_NUMBER    0x00000005

typedef enum {
  EfiMiscChassisTypeOther               = 0x1,
  EfiMiscChassisTypeUnknown             = 0x2,
  EfiMiscChassisTypeDeskTop             = 0x3,
  EfiMiscChassisTypeLowProfileDesktop   = 0x4,
  EfiMiscChassisTypePizzaBox            = 0x5,
  EfiMiscChassisTypeMiniTower           = 0x6,
  EfiMiscChassisTypeTower               = 0x7,
  EfiMiscChassisTypePortable            = 0x8,
  EfiMiscChassisTypeLapTop              = 0x9,
  EfiMiscChassisTypeNotebook            = 0xA,
  EfiMiscChassisTypeHandHeld            = 0xB,
  EfiMiscChassisTypeDockingStation      = 0xC,
  EfiMiscChassisTypeAllInOne            = 0xD,
  EfiMiscChassisTypeSubNotebook         = 0xE,
  EfiMiscChassisTypeSpaceSaving         = 0xF,
  EfiMiscChassisTypeLunchBox            = 0x10,
  EfiMiscChassisTypeMainServerChassis   = 0x11,
  EfiMiscChassisTypeExpansionChassis    = 0x12,
  EfiMiscChassisTypeSubChassis          = 0x13,
  EfiMiscChassisTypeBusExpansionChassis = 0x14,
  EfiMiscChassisTypePeripheralChassis   = 0x15,
  EfiMiscChassisTypeRaidChassis         = 0x16,
  EfiMiscChassisTypeRackMountChassis    = 0x17,
  EfiMiscChassisTypeSealedCasePc        = 0x18,
  EfiMiscChassisMultiSystemChassis      = 0x19
} EFI_MISC_CHASSIS_TYPE;

typedef struct {
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass 0.9 specification, it has the incorrect field name "EFI_MISC_CHASSIS_TYPE".
  /// Change it to "ChassisType" to pass build.
  ///
  UINT32                            ChassisType       :16;
  UINT32                            ChassisLockPresent:1;
  UINT32                            Reserved          :15;
} EFI_MISC_CHASSIS_STATUS;

typedef enum {
  EfiChassisStateOther           = 0x01,
  EfiChassisStateUnknown         = 0x02,
  EfiChassisStateSafe            = 0x03,
  EfiChassisStateWarning         = 0x04,
  EfiChassisStateCritical        = 0x05,
  EfiChassisStateNonRecoverable  = 0x06
} EFI_MISC_CHASSIS_STATE;

typedef enum {
  EfiChassisSecurityStatusOther                          = 0x01,
  EfiChassisSecurityStatusUnknown                        = 0x02,
  EfiChassisSecurityStatusNone                           = 0x03,
  EfiChassisSecurityStatusExternalInterfaceLockedOut     = 0x04,
  EfiChassisSecurityStatusExternalInterfaceLockedEnabled = 0x05
} EFI_MISC_CHASSIS_SECURITY_STATE;

typedef struct {
  UINT32                            RecordType :1;
  UINT32                            Type       :7;
  UINT32                            Reserved   :24;
} EFI_MISC_ELEMENT_TYPE;

typedef struct {
  EFI_MISC_ELEMENT_TYPE             ChassisElementType;
  EFI_INTER_LINK_DATA               ChassisElementStructure;
  EFI_BASE_BOARD_TYPE               ChassisBaseBoard;
  UINT32                            ChassisElementMinimum;
  UINT32                            ChassisElementMaximum;
} EFI_MISC_ELEMENTS;

typedef struct {
  STRING_REF                        ChassisManufacturer;
  STRING_REF                        ChassisVersion;
  STRING_REF                        ChassisSerialNumber;
  STRING_REF                        ChassisAssetTag;
  EFI_MISC_CHASSIS_STATUS           ChassisType;
  EFI_MISC_CHASSIS_STATE            ChassisBootupState;
  EFI_MISC_CHASSIS_STATE            ChassisPowerSupplyState;
  EFI_MISC_CHASSIS_STATE            ChassisThermalState;
  EFI_MISC_CHASSIS_SECURITY_STATE   ChassisSecurityState;
  UINT32                            ChassisOemDefined;
  UINT32                            ChassisHeight;
  UINT32                            ChassisNumberPowerCords;
  UINT32                            ChassisElementCount;
  UINT32                            ChassisElementRecordLength;
  EFI_MISC_ELEMENTS                 ChassisElements;
} EFI_MISC_CHASSIS_MANUFACTURER_DATA;

//
// Misc. Port Connector Information - SMBIOS Type 8
//
#define EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_RECORD_NUMBER    0x00000006

typedef enum {
  EfiPortConnectorTypeNone                   = 0x00,
  EfiPortConnectorTypeCentronics             = 0x01,
  EfiPortConnectorTypeMiniCentronics         = 0x02,
  EfiPortConnectorTypeProprietary            = 0x03,
  EfiPortConnectorTypeDB25Male               = 0x04,
  EfiPortConnectorTypeDB25Female             = 0x05,
  EfiPortConnectorTypeDB15Male               = 0x06,
  EfiPortConnectorTypeDB15Female             = 0x07,
  EfiPortConnectorTypeDB9Male                = 0x08,
  EfiPortConnectorTypeDB9Female              = 0x09,
  EfiPortConnectorTypeRJ11                   = 0x0A,
  EfiPortConnectorTypeRJ45                   = 0x0B,
  EfiPortConnectorType50PinMiniScsi          = 0x0C,
  EfiPortConnectorTypeMiniDin                = 0x0D,
  EfiPortConnectorTypeMicriDin               = 0x0E,
  EfiPortConnectorTypePS2                    = 0x0F,
  EfiPortConnectorTypeInfrared               = 0x10,
  EfiPortConnectorTypeHpHil                  = 0x11,
  EfiPortConnectorTypeUsb                    = 0x12,
  EfiPortConnectorTypeSsaScsi                = 0x13,
  EfiPortConnectorTypeCircularDin8Male       = 0x14,
  EfiPortConnectorTypeCircularDin8Female     = 0x15,
  EfiPortConnectorTypeOnboardIde             = 0x16,
  EfiPortConnectorTypeOnboardFloppy          = 0x17,
  EfiPortConnectorType9PinDualInline         = 0x18,
  EfiPortConnectorType25PinDualInline        = 0x19,
  EfiPortConnectorType50PinDualInline        = 0x1A,
  EfiPortConnectorType68PinDualInline        = 0x1B,
  EfiPortConnectorTypeOnboardSoundInput      = 0x1C,
  EfiPortConnectorTypeMiniCentronicsType14   = 0x1D,
  EfiPortConnectorTypeMiniCentronicsType26   = 0x1E,
  EfiPortConnectorTypeHeadPhoneMiniJack      = 0x1F,
  EfiPortConnectorTypeBNC                    = 0x20,
  EfiPortConnectorType1394                   = 0x21,
  EfiPortConnectorTypePC98                   = 0xA0,
  EfiPortConnectorTypePC98Hireso             = 0xA1,
  EfiPortConnectorTypePCH98                  = 0xA2,
  EfiPortConnectorTypePC98Note               = 0xA3,
  EfiPortConnectorTypePC98Full               = 0xA4,
  EfiPortConnectorTypeOther                  = 0xFF
} EFI_MISC_PORT_CONNECTOR_TYPE;

typedef enum {
  EfiPortTypeNone                      = 0x00,
  EfiPortTypeParallelXtAtCompatible    = 0x01,
  EfiPortTypeParallelPortPs2           = 0x02,
  EfiPortTypeParallelPortEcp           = 0x03,
  EfiPortTypeParallelPortEpp           = 0x04,
  EfiPortTypeParallelPortEcpEpp        = 0x05,
  EfiPortTypeSerialXtAtCompatible      = 0x06,
  EfiPortTypeSerial16450Compatible     = 0x07,
  EfiPortTypeSerial16550Compatible     = 0x08,
  EfiPortTypeSerial16550ACompatible    = 0x09,
  EfiPortTypeScsi                      = 0x0A,
  EfiPortTypeMidi                      = 0x0B,
  EfiPortTypeJoyStick                  = 0x0C,
  EfiPortTypeKeyboard                  = 0x0D,
  EfiPortTypeMouse                     = 0x0E,
  EfiPortTypeSsaScsi                   = 0x0F,
  EfiPortTypeUsb                       = 0x10,
  EfiPortTypeFireWire                  = 0x11,
  EfiPortTypePcmciaTypeI               = 0x12,
  EfiPortTypePcmciaTypeII              = 0x13,
  EfiPortTypePcmciaTypeIII             = 0x14,
  EfiPortTypeCardBus                   = 0x15,
  EfiPortTypeAccessBusPort             = 0x16,
  EfiPortTypeScsiII                    = 0x17,
  EfiPortTypeScsiWide                  = 0x18,
  EfiPortTypePC98                      = 0x19,
  EfiPortTypePC98Hireso                = 0x1A,
  EfiPortTypePCH98                     = 0x1B,
  EfiPortTypeVideoPort                 = 0x1C,
  EfiPortTypeAudioPort                 = 0x1D,
  EfiPortTypeModemPort                 = 0x1E,
  EfiPortTypeNetworkPort               = 0x1F,
  EfiPortType8251Compatible            = 0xA0,
  EfiPortType8251FifoCompatible        = 0xA1,
  EfiPortTypeOther                     = 0xFF
} EFI_MISC_PORT_TYPE;

typedef struct {
  STRING_REF                        PortInternalConnectorDesignator;
  STRING_REF                        PortExternalConnectorDesignator;
  EFI_MISC_PORT_CONNECTOR_TYPE      PortInternalConnectorType;
  EFI_MISC_PORT_CONNECTOR_TYPE      PortExternalConnectorType;
  EFI_MISC_PORT_TYPE                PortType;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, this type of field is defined as EFI_DEVICE_PATH_PROTOCOL,
  /// which causes the implementation some complexity. Keep it unchanged for backward
  /// compatibility.
  ///
  EFI_MISC_PORT_DEVICE_PATH         PortPath;
} EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA;

//
// Misc. System Slots - SMBIOS Type 9
//
#define EFI_MISC_SYSTEM_SLOT_DESIGNATION_RECORD_NUMBER    0x00000007

typedef enum {
  EfiSlotTypeOther                        = 0x01,
  EfiSlotTypeUnknown                      = 0x02,
  EfiSlotTypeIsa                          = 0x03,
  EfiSlotTypeMca                          = 0x04,
  EfiSlotTypeEisa                         = 0x05,
  EfiSlotTypePci                          = 0x06,
  EfiSlotTypePcmcia                       = 0x07,
  EfiSlotTypeVlVesa                       = 0x08,
  EfiSlotTypeProprietary                  = 0x09,
  EfiSlotTypeProcessorCardSlot            = 0x0A,
  EfiSlotTypeProprietaryMemoryCardSlot    = 0x0B,
  EfiSlotTypeIORiserCardSlot              = 0x0C,
  EfiSlotTypeNuBus                        = 0x0D,
  EfiSlotTypePci66MhzCapable              = 0x0E,
  EfiSlotTypeAgp                          = 0x0F,
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, its naming should be EfiSlotTypeAgp2X
  /// rather than EfiSlotTypeApg2X.
  ///
  EfiSlotTypeAgp2X                        = 0x10,
  EfiSlotTypeAgp4X                        = 0x11,
  EfiSlotTypePciX                         = 0x12,
  EfiSlotTypeAgp8x                        = 0x13,
  EfiSlotTypePC98C20                      = 0xA0,
  EfiSlotTypePC98C24                      = 0xA1,
  EfiSlotTypePC98E                        = 0xA2,
  EfiSlotTypePC98LocalBus                 = 0xA3,
  EfiSlotTypePC98Card                     = 0xA4,
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, these fields aren't defined.
  /// They're introduced for SmBios 2.6 specification type 9.
  ///
  EfiSlotTypePciExpress                   = 0xA5,
  EfiSlotTypePciExpressX1                 = 0xA6,
  EfiSlotTypePciExpressX2                 = 0xA7,
  EfiSlotTypePciExpressX4                 = 0xA8,
  EfiSlotTypePciExpressX8                 = 0xA9,
  EfiSlotTypePciExpressX16                = 0xAA
} EFI_MISC_SLOT_TYPE;

typedef enum {
  EfiSlotDataBusWidthOther      = 0x01,
  EfiSlotDataBusWidthUnknown    = 0x02,
  EfiSlotDataBusWidth8Bit       = 0x03,
  EfiSlotDataBusWidth16Bit      = 0x04,
  EfiSlotDataBusWidth32Bit      = 0x05,
  EfiSlotDataBusWidth64Bit      = 0x06,
  EfiSlotDataBusWidth128Bit     = 0x07,
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, these fields aren't defined.
  /// They're introduced for SmBios 2.6 specification type 9.
  ///
  EfiSlotDataBusWidth1xOrx1     = 0x8,
  EfiSlotDataBusWidth2xOrx2     = 0x9,
  EfiSlotDataBusWidth4xOrx4     = 0xA,
  EfiSlotDataBusWidth8xOrx8     = 0xB,
  EfiSlotDataBusWidth12xOrx12   = 0xC,
  EfiSlotDataBusWidth16xOrx16   = 0xD,
  EfiSlotDataBusWidth32xOrx32   = 0xE
} EFI_MISC_SLOT_DATA_BUS_WIDTH;

typedef enum {
  EfiSlotUsageOther     = 1,
  EfiSlotUsageUnknown   = 2,
  EfiSlotUsageAvailable = 3,
  EfiSlotUsageInUse     = 4
} EFI_MISC_SLOT_USAGE;

typedef enum {
  EfiSlotLengthOther   = 1,
  EfiSlotLengthUnknown = 2,
  EfiSlotLengthShort   = 3,
  EfiSlotLengthLong    = 4
} EFI_MISC_SLOT_LENGTH;

typedef struct {
  UINT32                            CharacteristicsUnknown  :1;
  UINT32                            Provides50Volts         :1;
  UINT32                            Provides33Volts         :1;
  UINT32                            SharedSlot              :1;
  UINT32                            PcCard16Supported       :1;
  UINT32                            CardBusSupported        :1;
  UINT32                            ZoomVideoSupported      :1;
  UINT32                            ModemRingResumeSupported:1;
  UINT32                            PmeSignalSupported      :1;
  UINT32                            HotPlugDevicesSupported :1;
  UINT32                            SmbusSignalSupported    :1;
  UINT32                            Reserved                :21;
} EFI_MISC_SLOT_CHARACTERISTICS;

typedef struct {
  STRING_REF                        SlotDesignation;
  EFI_MISC_SLOT_TYPE                SlotType;
  EFI_MISC_SLOT_DATA_BUS_WIDTH      SlotDataBusWidth;
  EFI_MISC_SLOT_USAGE               SlotUsage;
  EFI_MISC_SLOT_LENGTH              SlotLength;
  UINT16                            SlotId;
  EFI_MISC_SLOT_CHARACTERISTICS     SlotCharacteristics;
  EFI_DEVICE_PATH_PROTOCOL          SlotDevicePath;
} EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA;

//
// Misc. Onboard Device - SMBIOS Type 10
//
#define EFI_MISC_ONBOARD_DEVICE_RECORD_NUMBER    0x00000008

typedef enum {
  EfiOnBoardDeviceTypeOther          = 1,
  EfiOnBoardDeviceTypeUnknown        = 2,
  EfiOnBoardDeviceTypeVideo          = 3,
  EfiOnBoardDeviceTypeScsiController = 4,
  EfiOnBoardDeviceTypeEthernet       = 5,
  EfiOnBoardDeviceTypeTokenRing      = 6,
  EfiOnBoardDeviceTypeSound          = 7
} EFI_MISC_ONBOARD_DEVICE_TYPE;

typedef struct {
  UINT32                            DeviceType    :16;
  UINT32                            DeviceEnabled :1;
  UINT32                            Reserved      :15;
} EFI_MISC_ONBOARD_DEVICE_STATUS;

typedef struct {
  STRING_REF                        OnBoardDeviceDescription;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, the name is OnBoardDeviceType.
  /// Keep it unchanged for backward compatibilty.
  ///
  EFI_MISC_ONBOARD_DEVICE_STATUS    OnBoardDeviceStatus;
  EFI_DEVICE_PATH_PROTOCOL          OnBoardDevicePath;
} EFI_MISC_ONBOARD_DEVICE_DATA;

//
// Misc. BIOS Language Information - SMBIOS Type 11
//
#define EFI_MISC_OEM_STRING_RECORD_NUMBER    0x00000009

typedef struct {
  STRING_REF                        OemStringRef[1];
} EFI_MISC_OEM_STRING_DATA;

//
// Misc. System Options - SMBIOS Type 12
//
typedef struct {
  STRING_REF                        SystemOptionStringRef[1];
} EFI_MISC_SYSTEM_OPTION_STRING_DATA;

#define EFI_MISC_SYSTEM_OPTION_STRING_RECORD_NUMBER    0x0000000A

//
// Misc. Number of Installable Languages - SMBIOS Type 13
//
#define EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_RECORD_NUMBER    0x0000000B

typedef struct {
  UINT32                            AbbreviatedLanguageFormat :1;
  UINT32                            Reserved                  :31;
} EFI_MISC_LANGUAGE_FLAGS;

typedef struct {
  UINT16                            NumberOfInstallableLanguages;
  EFI_MISC_LANGUAGE_FLAGS           LanguageFlags;
  UINT16                            CurrentLanguageNumber;
} EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA;

//
// Misc. System Language String
//
#define EFI_MISC_SYSTEM_LANGUAGE_STRING_RECORD_NUMBER    0x0000000C

typedef struct {
  UINT16                            LanguageId;
  STRING_REF                        SystemLanguageString;
} EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA;

//
// Group Associations - SMBIOS Type 14
//
#define EFI_MISC_GROUP_NAME_RECORD_NUMBER    0x0000000D

typedef struct {
  STRING_REF                        GroupName;
  UINT16                            NumberGroupItems;
  UINT16                            GroupId;
} EFI_MISC_GROUP_NAME_DATA;

//
// Group Item Set Element
//
#define EFI_MISC_GROUP_ITEM_SET_RECORD_NUMBER    0x0000000E

typedef struct {
  EFI_GUID                          SubClass;
  EFI_INTER_LINK_DATA               GroupLink;
  UINT16                            GroupId;
  UINT16                            GroupElementId;
} EFI_MISC_GROUP_ITEM_SET_DATA;

//
//  Misc. Pointing Device Type - SMBIOS Type 21
//
#define EFI_MISC_POINTING_DEVICE_TYPE_RECORD_NUMBER    0x0000000F

typedef enum {
  EfiPointingDeviceTypeOther         = 0x01,
  EfiPointingDeviceTypeUnknown       = 0x02,
  EfiPointingDeviceTypeMouse         = 0x03,
  EfiPointingDeviceTypeTrackBall     = 0x04,
  EfiPointingDeviceTypeTrackPoint    = 0x05,
  EfiPointingDeviceTypeGlidePoint    = 0x06,
  EfiPointingDeviceTouchPad          = 0x07,
  EfiPointingDeviceTouchScreen       = 0x08,
  EfiPointingDeviceOpticalSensor     = 0x09
} EFI_MISC_POINTING_DEVICE_TYPE;

typedef enum {
  EfiPointingDeviceInterfaceOther              = 0x01,
  EfiPointingDeviceInterfaceUnknown            = 0x02,
  EfiPointingDeviceInterfaceSerial             = 0x03,
  EfiPointingDeviceInterfacePs2                = 0x04,
  EfiPointingDeviceInterfaceInfrared           = 0x05,
  EfiPointingDeviceInterfaceHpHil              = 0x06,
  EfiPointingDeviceInterfaceBusMouse           = 0x07,
  EfiPointingDeviceInterfaceADB                = 0x08,
  EfiPointingDeviceInterfaceBusMouseDB9        = 0xA0,
  EfiPointingDeviceInterfaceBusMouseMicroDin   = 0xA1,
  EfiPointingDeviceInterfaceUsb                = 0xA2
} EFI_MISC_POINTING_DEVICE_INTERFACE;

typedef struct {
  EFI_MISC_POINTING_DEVICE_TYPE       PointingDeviceType;
  EFI_MISC_POINTING_DEVICE_INTERFACE  PointingDeviceInterface;
  UINT16                              NumberPointingDeviceButtons;
  EFI_DEVICE_PATH_PROTOCOL            PointingDevicePath;
} EFI_MISC_POINTING_DEVICE_TYPE_DATA;

//
// Portable Battery - SMBIOS Type 22
//
///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the name is EFI_MISC_BATTERY_LOCATION_RECORD_NUMBER.
/// Keep it unchanged for backward compatibilty.
///
#define EFI_MISC_PORTABLE_BATTERY_RECORD_NUMBER   0x00000010

///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the structure name is EFI_MISC_BATTERY_DEVICE_CHEMISTRY.
/// And all field namings are also different with specification.
/// Keep it unchanged for backward compatibilty.
///
typedef enum {
  EfiPortableBatteryDeviceChemistryOther = 1,
  EfiPortableBatteryDeviceChemistryUnknown = 2,
  EfiPortableBatteryDeviceChemistryLeadAcid = 3,
  EfiPortableBatteryDeviceChemistryNickelCadmium = 4,
  EfiPortableBatteryDeviceChemistryNickelMetalHydride = 5,
  EfiPortableBatteryDeviceChemistryLithiumIon = 6,
  EfiPortableBatteryDeviceChemistryZincAir = 7,
  EfiPortableBatteryDeviceChemistryLithiumPolymer = 8
} EFI_MISC_PORTABLE_BATTERY_DEVICE_CHEMISTRY;

///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the structure name is EFI_MISC_BATTERY_LOCATION_DATA.
/// Also, the name and the order of the fields vary with specifications.
/// Keep it unchanged for backward compatibilty.
///
typedef struct {
  STRING_REF                        Location;
  STRING_REF                        Manufacturer;
  STRING_REF                        ManufactureDate;
  STRING_REF                        SerialNumber;
  STRING_REF                        DeviceName;
  EFI_MISC_PORTABLE_BATTERY_DEVICE_CHEMISTRY
                                    DeviceChemistry;
  UINT16                            DesignCapacity;
  UINT16                            DesignVoltage;
  STRING_REF                        SBDSVersionNumber;
  UINT8                             MaximumError;
  UINT16                            SBDSSerialNumber;
  UINT16                            SBDSManufactureDate;
  STRING_REF                        SBDSDeviceChemistry;
  UINT8                             DesignCapacityMultiplier;
  UINT32                            OEMSpecific;
  UINT8                             BatteryNumber; // Temporary
  BOOLEAN                           Valid; // Is entry valid - Temporary
} EFI_MISC_PORTABLE_BATTERY;


//
// Misc. Reset Capabilities - SMBIOS Type 23
//
#define EFI_MISC_RESET_CAPABILITIES_RECORD_NUMBER    0x00000011

typedef struct {
  UINT32                            Status              :1;
  UINT32                            BootOption          :2;
  UINT32                            BootOptionOnLimit   :2;
  UINT32                            WatchdogTimerPresent:1;
  UINT32                            Reserved            :26;
} EFI_MISC_RESET_CAPABILITIES_TYPE;

typedef struct {
  EFI_MISC_RESET_CAPABILITIES_TYPE  ResetCapabilities;
  UINT16                            ResetCount;
  UINT16                            ResetLimit;
  UINT16                            ResetTimerInterval;
  UINT16                            ResetTimeout;
} EFI_MISC_RESET_CAPABILITIES;

typedef struct {
    EFI_MISC_RESET_CAPABILITIES     ResetCapabilities;
    UINT16                          ResetCount;
    UINT16                          ResetLimit;
    UINT16                          ResetTimerInterval;
    UINT16                          ResetTimeout;
} EFI_MISC_RESET_CAPABILITIES_DATA;

//
// Misc. Hardware Security - SMBIOS Type 24
//
#define EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA_RECORD_NUMBER    0x00000012

///
/// Inconsistent with specification here:
/// The MiscSubclass specification 0.9 only mentions the possible value of each field in
/// EFI_MISC_HARDWARE_SECURITY_SETTINGS.
/// It's implementation-specific in order to to simplify the code logic.
///
typedef enum {
  EfiHardwareSecurityStatusDisabled       = 0,
  EfiHardwareSecurityStatusEnabled        = 1,
  EfiHardwareSecurityStatusNotImplemented = 2,
  EfiHardwareSecurityStatusUnknown        = 3
} EFI_MISC_HARDWARE_SECURITY_STATUS;

typedef struct {
  UINT32 FrontPanelResetStatus       :2;
  UINT32 AdministratorPasswordStatus :2;
  UINT32 KeyboardPasswordStatus      :2;
  UINT32 PowerOnPasswordStatus       :2;
  UINT32 Reserved                    :24;
} EFI_MISC_HARDWARE_SECURITY_SETTINGS;

typedef struct {
  EFI_MISC_HARDWARE_SECURITY_SETTINGS HardwareSecuritySettings;
} EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA;

//
// System Power Controls - SMBIOS Type 25
//
#define EFI_MISC_SCHEDULED_POWER_ON_MONTH_RECORD_NUMBER    0x00000013

typedef struct {
  UINT16                            ScheduledPoweronMonth;
  UINT16                            ScheduledPoweronDayOfMonth;
  UINT16                            ScheduledPoweronHour;
  UINT16                            ScheduledPoweronMinute;
  UINT16                            ScheduledPoweronSecond;
} EFI_MISC_SCHEDULED_POWER_ON_MONTH_DATA;

//
// Voltage Probe - SMBIOS Type 26
//
#define EFI_MISC_VOLTAGE_PROBE_DESCRIPTION_RECORD_NUMBER    0x00000014

typedef struct {
  UINT32                            VoltageProbeSite        :5;
  UINT32                            VoltageProbeStatus      :3;
  UINT32                            Reserved                :24;
} EFI_MISC_VOLTAGE_PROBE_LOCATION;

typedef struct {
  STRING_REF                        VoltageProbeDescription;
  EFI_MISC_VOLTAGE_PROBE_LOCATION   VoltageProbeLocation;
  EFI_EXP_BASE10_DATA               VoltageProbeMaximumValue;
  EFI_EXP_BASE10_DATA               VoltageProbeMinimumValue;
  EFI_EXP_BASE10_DATA               VoltageProbeResolution;
  EFI_EXP_BASE10_DATA               VoltageProbeTolerance;
  EFI_EXP_BASE10_DATA               VoltageProbeAccuracy;
  EFI_EXP_BASE10_DATA               VoltageProbeNominalValue;
  EFI_EXP_BASE10_DATA               MDLowerNoncriticalThreshold;
  EFI_EXP_BASE10_DATA               MDUpperNoncriticalThreshold;
  EFI_EXP_BASE10_DATA               MDLowerCriticalThreshold;
  EFI_EXP_BASE10_DATA               MDUpperCriticalThreshold;
  EFI_EXP_BASE10_DATA               MDLowerNonrecoverableThreshold;
  EFI_EXP_BASE10_DATA               MDUpperNonrecoverableThreshold;
  UINT32                            VoltageProbeOemDefined;
} EFI_MISC_VOLTAGE_PROBE_DESCRIPTION_DATA;

//
// Cooling Device - SMBIOS Type 27
//
#define EFI_MISC_COOLING_DEVICE_TEMP_LINK_RECORD_NUMBER    0x00000015

typedef struct {
  UINT32                            CoolingDevice       :5;
  UINT32                            CoolingDeviceStatus :3;
  UINT32                            Reserved            :24;
} EFI_MISC_COOLING_DEVICE_TYPE;

typedef struct {
  EFI_MISC_COOLING_DEVICE_TYPE      CoolingDeviceType;
  EFI_INTER_LINK_DATA               CoolingDeviceTemperatureLink;
  UINT8                             CoolingDeviceUnitGroup;
  UINT16                            CoolingDeviceNominalSpeed;
  UINT32                            CoolingDeviceOemDefined;
} EFI_MISC_COOLING_DEVICE_TEMP_LINK_DATA;

//
// Temperature Probe - SMBIOS Type 28
//
#define EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION_RECORD_NUMBER    0x00000016

typedef struct {
  UINT32                            TemperatureProbeSite   :5;
  UINT32                            TemperatureProbeStatus :3;
  UINT32                            Reserved               :24;
} EFI_MISC_TEMPERATURE_PROBE_LOCATION;

typedef struct {
  STRING_REF                        TemperatureProbeDescription;
  EFI_MISC_TEMPERATURE_PROBE_LOCATION
                                    TemperatureProbeLocation;
  ///
  /// Inconsistent with specification here:
  /// MiscSubclass 0.9 specification defines the fields type as EFI_EXP_BASE10_DATA.
  /// In fact, they should be UINT16 type because they refer to 16bit width data.
  /// Keeping this inconsistency for backward compatibility.
  ///
  UINT16                            TemperatureProbeMaximumValue;
  UINT16                            TemperatureProbeMinimumValue;
  UINT16                            TemperatureProbeResolution;
  UINT16                            TemperatureProbeTolerance;
  UINT16                            TemperatureProbeAccuracy;
  UINT16                            TemperatureProbeNominalValue;
  UINT16                            MDLowerNoncriticalThreshold;
  UINT16                            MDUpperNoncriticalThreshold;
  UINT16                            MDLowerCriticalThreshold;
  UINT16                            MDUpperCriticalThreshold;
  UINT16                            MDLowerNonrecoverableThreshold;
  UINT16                            MDUpperNonrecoverableThreshold;
  UINT32                            TemperatureProbeOemDefined;
} EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION_DATA;

//
// Electrical Current Probe - SMBIOS Type 29
//

#define EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION_RECORD_NUMBER    0x00000017

typedef struct {
  UINT32                            ElectricalCurrentProbeSite    :5;
  UINT32                            ElectricalCurrentProbeStatus  :3;
  UINT32                            Reserved                      :24;
} EFI_MISC_ELECTRICAL_CURRENT_PROBE_LOCATION;

typedef struct {
  STRING_REF                        ElectricalCurrentProbeDescription;
  EFI_MISC_ELECTRICAL_CURRENT_PROBE_LOCATION
                                    ElectricalCurrentProbeLocation;
  EFI_EXP_BASE10_DATA               ElectricalCurrentProbeMaximumValue;
  EFI_EXP_BASE10_DATA               ElectricalCurrentProbeMinimumValue;
  EFI_EXP_BASE10_DATA               ElectricalCurrentProbeResolution;
  EFI_EXP_BASE10_DATA               ElectricalCurrentProbeTolerance;
  EFI_EXP_BASE10_DATA               ElectricalCurrentProbeAccuracy;
  EFI_EXP_BASE10_DATA               ElectricalCurrentProbeNominalValue;
  EFI_EXP_BASE10_DATA               MDLowerNoncriticalThreshold;
  EFI_EXP_BASE10_DATA               MDUpperNoncriticalThreshold;
  EFI_EXP_BASE10_DATA               MDLowerCriticalThreshold;
  EFI_EXP_BASE10_DATA               MDUpperCriticalThreshold;
  EFI_EXP_BASE10_DATA               MDLowerNonrecoverableThreshold;
  EFI_EXP_BASE10_DATA               MDUpperNonrecoverableThreshold;
  UINT32                            ElectricalCurrentProbeOemDefined;
} EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION_DATA;

//
// Out-of-Band Remote Access - SMBIOS Type 30
//

#define EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION_RECORD_NUMBER    0x00000018

typedef struct  {
  UINT32                            InboundConnectionEnabled  :1;
  UINT32                            OutboundConnectionEnabled :1;
  UINT32                            Reserved                  :30;
} EFI_MISC_REMOTE_ACCESS_CONNECTIONS;

typedef struct {
  STRING_REF                         RemoteAccessManufacturerNameDescription;
  EFI_MISC_REMOTE_ACCESS_CONNECTIONS RemoteAccessConnections;
} EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION_DATA;

//
// Misc. BIS Entry Point - SMBIOS Type 31
//
#define EFI_MISC_BIS_ENTRY_POINT_RECORD_NUMBER    0x00000019

typedef struct {
  EFI_PHYSICAL_ADDRESS              BisEntryPoint;
} EFI_MISC_BIS_ENTRY_POINT_DATA;

//
// Misc. Boot Information - SMBIOS Type 32
//
#define EFI_MISC_BOOT_INFORMATION_STATUS_RECORD_NUMBER    0x0000001A

///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the structure name is EFI_MISC_BOOT_INFORMATION_STATUS_TYPE.
/// Keep it unchanged for backward compatibilty.
///
typedef enum {
  EfiBootInformationStatusNoError                  = 0x00,
  EfiBootInformationStatusNoBootableMedia          = 0x01,
  EfiBootInformationStatusNormalOSFailedLoading    = 0x02,
  EfiBootInformationStatusFirmwareDetectedFailure  = 0x03,
  EfiBootInformationStatusOSDetectedFailure        = 0x04,
  EfiBootInformationStatusUserRequestedBoot        = 0x05,
  EfiBootInformationStatusSystemSecurityViolation  = 0x06,
  EfiBootInformationStatusPreviousRequestedImage   = 0x07,
  EfiBootInformationStatusWatchdogTimerExpired     = 0x08,
  EfiBootInformationStatusStartReserved            = 0x09,
  EfiBootInformationStatusStartOemSpecific         = 0x80,
  EfiBootInformationStatusStartProductSpecific     = 0xC0
} EFI_MISC_BOOT_INFORMATION_STATUS_DATA_TYPE;

typedef struct {
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, the field name is EFI_MISC_BOOT_INFORMATION_STATUS_TYPE.
  /// Keep it unchanged for backward compatibilty.
  ///
  EFI_MISC_BOOT_INFORMATION_STATUS_DATA_TYPE BootInformationStatus;
  UINT8                                      BootInformationData[9];
} EFI_MISC_BOOT_INFORMATION_STATUS_DATA;

//
// Management Device - SMBIOS Type 34
//
#define EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION_RECORD_NUMBER    0x0000001B

typedef enum {
  EfiManagementDeviceTypeOther      = 0x01,
  EfiManagementDeviceTypeUnknown    = 0x02,
  EfiManagementDeviceTypeLm75       = 0x03,
  EfiManagementDeviceTypeLm78       = 0x04,
  EfiManagementDeviceTypeLm79       = 0x05,
  EfiManagementDeviceTypeLm80       = 0x06,
  EfiManagementDeviceTypeLm81       = 0x07,
  EfiManagementDeviceTypeAdm9240    = 0x08,
  EfiManagementDeviceTypeDs1780     = 0x09,
  EfiManagementDeviceTypeMaxim1617  = 0x0A,
  EfiManagementDeviceTypeGl518Sm    = 0x0B,
  EfiManagementDeviceTypeW83781D    = 0x0C,
  EfiManagementDeviceTypeHt82H791   = 0x0D
} EFI_MISC_MANAGEMENT_DEVICE_TYPE;

typedef enum {
  EfiManagementDeviceAddressTypeOther   = 1,
  EfiManagementDeviceAddressTypeUnknown = 2,
  EfiManagementDeviceAddressTypeIOPort  = 3,
  EfiManagementDeviceAddressTypeMemory  = 4,
  EfiManagementDeviceAddressTypeSmbus   = 5
} EFI_MISC_MANAGEMENT_DEVICE_ADDRESS_TYPE;

typedef struct {
  STRING_REF                        ManagementDeviceDescription;
  EFI_MISC_MANAGEMENT_DEVICE_TYPE   ManagementDeviceType;
  UINTN                             ManagementDeviceAddress;
  EFI_MISC_MANAGEMENT_DEVICE_ADDRESS_TYPE
                                    ManagementDeviceAddressType;
} EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION_DATA;

//
// Management Device Component - SMBIOS Type 35
//

#define EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION_RECORD_NUMBER    0x0000001C

typedef struct {
  STRING_REF                        ManagementDeviceComponentDescription;
  EFI_INTER_LINK_DATA               ManagementDeviceLink;
  EFI_INTER_LINK_DATA               ManagementDeviceComponentLink;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, this field is NOT defined.
  /// It's introduced for SmBios 2.6 specification type 35.
  ///
  EFI_INTER_LINK_DATA               ManagementDeviceThresholdLink;
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, this field is NOT defined.
  /// It's implementation-specific to simplify the code logic.
  ///
  UINT8                             ComponentType;
} EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION_DATA;

//
// IPMI Data Record - SMBIOS Type 38
//
typedef enum {
  EfiIpmiOther = 0,
  EfiIpmiKcs   = 1,
  EfiIpmiSmic  = 2,
  EfiIpmiBt    = 3
} EFI_MISC_IPMI_INTERFACE_TYPE;

typedef struct {
  UINT16                            IpmiSpecLeastSignificantDigit:4;
  UINT16                            IpmiSpecMostSignificantDigit: 4;
  UINT16                            Reserved:                     8;
} EFI_MISC_IPMI_SPECIFICATION_REVISION;

typedef struct {
  EFI_MISC_IPMI_INTERFACE_TYPE      IpmiInterfaceType;
  EFI_MISC_IPMI_SPECIFICATION_REVISION
                                    IpmiSpecificationRevision;
  UINT16                            IpmiI2CSlaveAddress;
  UINT16                            IpmiNvDeviceAddress;
  UINT64                            IpmiBaseAddress;
  EFI_DEVICE_PATH_PROTOCOL          IpmiDevicePath;
} EFI_MISC_IPMI_INTERFACE_TYPE_DATA;

#define EFI_MISC_IPMI_INTERFACE_TYPE_RECORD_NUMBER    0x0000001D
///
/// The definition above is *NOT* defined in MiscSubclass specifications 0.9.
/// It's defined for backward compatibility.
///
#define EFI_MISC_IPMI_INTERFACE_TYPE_DATA_RECORD_NUMBER EFI_MISC_IPMI_INTERFACE_TYPE_RECORD_NUMBER

///
/// System Power supply Record - SMBIOS Type 39
///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the type of all fields are UINT32.
/// Keep it unchanged for backward compatibilty.
///
typedef struct {
  UINT16                            PowerSupplyHotReplaceable:1;
  UINT16                            PowerSupplyPresent       :1;
  UINT16                            PowerSupplyUnplugged     :1;
  UINT16                            InputVoltageRangeSwitch  :4;
  UINT16                            PowerSupplyStatus        :3;
  UINT16                            PowerSupplyType          :4;
  UINT16                            Reserved                 :2;
} EFI_MISC_POWER_SUPPLY_CHARACTERISTICS;

///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the structure name is EFI_MISC_POWER_SUPPLY_UNIT_GROUP_DATA.
/// Keep it unchanged for backward compatibilty.
///
typedef struct {
  UINT16                                 PowerUnitGroup;
  STRING_REF                             PowerSupplyLocation;
  STRING_REF                             PowerSupplyDeviceName;
  STRING_REF                             PowerSupplyManufacturer;
  STRING_REF                             PowerSupplySerialNumber;
  STRING_REF                             PowerSupplyAssetTagNumber;
  STRING_REF                             PowerSupplyModelPartNumber;
  STRING_REF                             PowerSupplyRevisionLevel;
  UINT16                                 PowerSupplyMaxPowerCapacity;
  EFI_MISC_POWER_SUPPLY_CHARACTERISTICS  PowerSupplyCharacteristics;
  EFI_INTER_LINK_DATA                    PowerSupplyInputVoltageProbeLink;
  EFI_INTER_LINK_DATA                    PowerSupplyCoolingDeviceLink;
  EFI_INTER_LINK_DATA                    PowerSupplyInputCurrentProbeLink;
} EFI_MISC_SYSTEM_POWER_SUPPLY_DATA;

#define EFI_MISC_SYSTEM_POWER_SUPPLY_RECORD_NUMBER    0x0000001E

///
/// OEM Data Record - SMBIOS Type 0x80-0xFF
///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the structure name is EFI_SMBIOS_STRUCTURE_HDR.
/// Due to this, the structure is commonly used by vendors to construct SmBios type 0x80~0xFF table,
/// Keep it unchanged for backward compatibilty.
///
typedef struct {
  UINT8                             Type;
  UINT8                             Length;
  UINT16                            Handle;
} SMBIOS_STRUCTURE_HDR;

typedef struct {
  ///
  /// Inconsistent with specification here:
  /// In MiscSubclass specification 0.9, the field name is EFI_SMBIOS_STRUCTURE_HDR.
  /// Keep it unchanged for backward compatibilty.
  ///
  SMBIOS_STRUCTURE_HDR              Header;
  UINT8                             RawData[1];
} EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION_DATA;

#define EFI_MISC_SMBIOS_STRUCT_ENCAP_RECORD_NUMBER    0x0000001F

///
/// Misc. System Event Log  - SMBIOS Type 15
///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 specification type 15.
///
#define EFI_MISC_SYSTEM_EVENT_LOG_RECORD_NUMBER    0x00000020

///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 specification type 15.
///
typedef struct {
  UINT16                            LogAreaLength;
  UINT16                            LogHeaderStartOffset;
  UINT16                            LogDataStartOffset;
  UINT8                             AccessMethod;
  UINT8                             LogStatus;
  UINT32                            LogChangeToken;
  UINT32                            AccessMethodAddress;
  UINT8                             LogHeaderFormat;
  UINT8                             NumberOfSupportedLogType;
  UINT8                             LengthOfLogDescriptor;
} EFI_MISC_SYSTEM_EVENT_LOG_DATA;

//
// Access Method.
//  0x00~0x04:  as following definition
//  0x05~0x7f:  Available for future assignment.
//  0x80~0xff:  BIOS Vendor/OEM-specific.
//
#define ACCESS_INDEXIO_1INDEX8BIT_DATA8BIT    0x00
#define ACCESS_INDEXIO_2INDEX8BIT_DATA8BIT    0X01
#define ACCESS_INDEXIO_1INDEX16BIT_DATA8BIT   0X02
#define ACCESS_MEMORY_MAPPED                  0x03
#define ACCESS_GPNV                           0x04

///
/// Management Device Threshold Data Record - SMBIOS Type 36
///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 specification type 36.
///
#define EFI_MISC_MANAGEMENT_DEVICE_THRESHOLD_RECORD_NUMBER    0x00000021
///
/// Inconsistent with specification here:
/// In MiscSubclass specification 0.9, the following data structures are NOT defined.
/// It's introduced for SmBios 2.6 specification type 36.
///
typedef struct {
  UINT16                            LowerThresNonCritical;
  UINT16                            UpperThresNonCritical;
  UINT16                            LowerThresCritical;
  UINT16                            UpperThresCritical;
  UINT16                            LowerThresNonRecover;
  UINT16                            UpperThresNonRecover;
} EFI_MISC_MANAGEMENT_DEVICE_THRESHOLD;

//
// Declare the following strutures alias to use them more conviniently.
//
typedef EFI_MISC_LAST_PCI_BUS_DATA                        EFI_MISC_LAST_PCI_BUS;
typedef EFI_MISC_BIOS_VENDOR_DATA                         EFI_MISC_BIOS_VENDOR;
typedef EFI_MISC_SYSTEM_MANUFACTURER_DATA                 EFI_MISC_SYSTEM_MANUFACTURER;
typedef EFI_MISC_BASE_BOARD_MANUFACTURER_DATA             EFI_MISC_BASE_BOARD_MANUFACTURER;
typedef EFI_MISC_CHASSIS_MANUFACTURER_DATA                EFI_MISC_CHASSIS_MANUFACTURER;
typedef EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR;
typedef EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA             EFI_MISC_SYSTEM_SLOT_DESIGNATION;
typedef EFI_MISC_ONBOARD_DEVICE_DATA                      EFI_MISC_ONBOARD_DEVICE;
typedef EFI_MISC_POINTING_DEVICE_TYPE_DATA                EFI_MISC_ONBOARD_DEVICE_TYPE_DATA;
typedef EFI_MISC_OEM_STRING_DATA                          EFI_MISC_OEM_STRING;
typedef EFI_MISC_SYSTEM_OPTION_STRING_DATA                EFI_MISC_SYSTEM_OPTION_STRING;
typedef EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA     EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES;
typedef EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA              EFI_MISC_SYSTEM_LANGUAGE_STRING;
typedef EFI_MISC_SYSTEM_EVENT_LOG_DATA                    EFI_MISC_SYSTEM_EVENT_LOG;
typedef EFI_MISC_BIS_ENTRY_POINT_DATA                     EFI_MISC_BIS_ENTRY_POINT;
typedef EFI_MISC_BOOT_INFORMATION_STATUS_DATA             EFI_MISC_BOOT_INFORMATION_STATUS;
typedef EFI_MISC_SYSTEM_POWER_SUPPLY_DATA                 EFI_MISC_SYSTEM_POWER_SUPPLY;
typedef EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION_DATA         EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION;
typedef EFI_MISC_SCHEDULED_POWER_ON_MONTH_DATA            EFI_MISC_SCHEDULED_POWER_ON_MONTH;
typedef EFI_MISC_VOLTAGE_PROBE_DESCRIPTION_DATA           EFI_MISC_VOLTAGE_PROBE_DESCRIPTION;
typedef EFI_MISC_COOLING_DEVICE_TEMP_LINK_DATA            EFI_MISC_COOLING_DEVICE_TEMP_LINK;
typedef EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION_DATA       EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION;
typedef EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION_DATA
                                                          EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION;
typedef EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION_DATA       EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION;
typedef EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION_DATA EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION;
typedef EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION_DATA
                                                          EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION;

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It is implementation-specific to simplify the code logic.
///
typedef union {
  EFI_MISC_LAST_PCI_BUS_DATA                         LastPciBus;
  EFI_MISC_BIOS_VENDOR_DATA                          MiscBiosVendor;
  EFI_MISC_SYSTEM_MANUFACTURER_DATA                  MiscSystemManufacturer;
  EFI_MISC_BASE_BOARD_MANUFACTURER_DATA              MiscBaseBoardManufacturer;
  EFI_MISC_CHASSIS_MANUFACTURER_DATA                 MiscChassisManufacturer;
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA   MiscPortInternalConnectorDesignator;
  EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA              MiscSystemSlotDesignation;
  EFI_MISC_ONBOARD_DEVICE_DATA                       MiscOnboardDevice;
  EFI_MISC_OEM_STRING_DATA                           MiscOemString;
  EFI_MISC_SYSTEM_OPTION_STRING_DATA                 MiscOptionString;
  EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA      NumberOfInstallableLanguages;
  EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA               MiscSystemLanguageString;
  EFI_MISC_SYSTEM_EVENT_LOG_DATA                     MiscSystemEventLog;
  EFI_MISC_GROUP_NAME_DATA                           MiscGroupNameData;
  EFI_MISC_GROUP_ITEM_SET_DATA                       MiscGroupItemSetData;
  EFI_MISC_POINTING_DEVICE_TYPE_DATA                 MiscPointingDeviceTypeData;
  EFI_MISC_RESET_CAPABILITIES_DATA                   MiscResetCapablilitiesData;
  EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA           MiscHardwareSecuritySettingsData;
  EFI_MISC_SCHEDULED_POWER_ON_MONTH_DATA             MiscScheduledPowerOnMonthData;
  EFI_MISC_VOLTAGE_PROBE_DESCRIPTION_DATA            MiscVoltagePorbeDescriptionData;
  EFI_MISC_COOLING_DEVICE_TEMP_LINK_DATA             MiscCoolingDeviceTempLinkData;
  EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION_DATA        MiscTemperatureProbeDescriptionData;
  EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION_DATA MiscElectricalCurrentProbeDescriptionData;
  EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION_DATA
                                                     MiscRemoteAccessManufacturerDescriptionData;
  EFI_MISC_BIS_ENTRY_POINT_DATA                      MiscBisEntryPoint;
  EFI_MISC_BOOT_INFORMATION_STATUS_DATA              MiscBootInformationStatus;
  EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION_DATA        MiscMangementDeviceDescriptionData;
  EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION_DATA
                                                     MiscmangementDeviceComponentDescriptionData;
  EFI_MISC_IPMI_INTERFACE_TYPE_DATA                  MiscIpmiInterfaceTypeData;
  EFI_MISC_SYSTEM_POWER_SUPPLY_DATA                  MiscPowerSupplyInfo;
  EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION_DATA          MiscSmbiosStructEncapsulation;
  EFI_MISC_MANAGEMENT_DEVICE_THRESHOLD               MiscManagementDeviceThreshold;
} EFI_MISC_SUBCLASS_RECORDS;

///
/// Inconsistent with specification here:
/// In MemSubclass specification 0.9, the following data structures are NOT defined.
/// It is implementation-specific to simplify the code logic.
///
typedef struct {
  EFI_SUBCLASS_TYPE1_HEADER         Header;
  EFI_MISC_SUBCLASS_RECORDS         Record;
} EFI_MISC_SUBCLASS_DRIVER_DATA;
#pragma pack()

///
/// Inconsistent with specification here:
/// In DataHubSubclass specification 0.9 page 16, the following symbol is NOT defined.
/// But value is meaningful, 0 means Reserved.
///
#define EFI_SUBCLASS_INSTANCE_RESERVED       0
///
/// Inconsistent with specification here:
/// In DataHubSubclass specification 0.9 page 16, the following symbol is NOT defined.
/// But value is meaningful, -1 means Not Applicable.
///
#define EFI_SUBCLASS_INSTANCE_NON_APPLICABLE 0xFFFF

#endif
