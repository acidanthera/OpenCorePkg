/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiImageFormat.h

Abstract:

  This file defines the data structures that are architecturally defined for file
  images loaded via the FirmwareVolume protocol.  The Firmware Volume specification
  is the basis for these definitions.

--*/

#ifndef _EFI_IMAGE_FORMAT_H_
#define _EFI_IMAGE_FORMAT_H_

//
// pack all data structures since this is actually a binary format and we cannot
// allow internal padding in the data structures because of some compilerism..
//
#pragma pack(1)
//
// ////////////////////////////////////////////////////////////////////////////
//
// Architectural file types
//
typedef UINT8 EFI_FV_FILETYPE;

#define EFI_FV_FILETYPE_ALL                   0x00
#define EFI_FV_FILETYPE_RAW                   0x01
#define EFI_FV_FILETYPE_FREEFORM              0x02
#define EFI_FV_FILETYPE_SECURITY_CORE         0x03
#define EFI_FV_FILETYPE_PEI_CORE              0x04
#define EFI_FV_FILETYPE_DXE_CORE              0x05
#define EFI_FV_FILETYPE_PEIM                  0x06
#define EFI_FV_FILETYPE_DRIVER                0x07
#define EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER  0x08
#define EFI_FV_FILETYPE_APPLICATION           0x09
//
// File type 0x0A is reserved and should not be used
//
#define EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE 0x0B

//
// ////////////////////////////////////////////////////////////////////////////
//
// Section types
//
typedef UINT8 EFI_SECTION_TYPE;

//
// ************************************************************
// The section type EFI_SECTION_ALL is a psuedo type.  It is
// used as a wildcard when retrieving sections.  The section
// type EFI_SECTION_ALL matches all section types.
// ************************************************************
//
#define EFI_SECTION_ALL 0x00

//
// ************************************************************
// Encapsulation section Type values
// ************************************************************
//
#define EFI_SECTION_COMPRESSION   0x01
#define EFI_SECTION_GUID_DEFINED  0x02

//
// ************************************************************
// Leaf section Type values
// ************************************************************
//
#define EFI_SECTION_FIRST_LEAF_SECTION_TYPE 0x10

#define EFI_SECTION_PE32                    0x10
#define EFI_SECTION_PIC                     0x11
#define EFI_SECTION_TE                      0x12
#define EFI_SECTION_DXE_DEPEX               0x13
#define EFI_SECTION_VERSION                 0x14
#define EFI_SECTION_USER_INTERFACE          0x15
#define EFI_SECTION_COMPATIBILITY16         0x16
#define EFI_SECTION_FIRMWARE_VOLUME_IMAGE   0x17
#define EFI_SECTION_FREEFORM_SUBTYPE_GUID   0x18
#define EFI_SECTION_RAW                     0x19
#define EFI_SECTION_PEI_DEPEX               0x1B

#define EFI_SECTION_LAST_LEAF_SECTION_TYPE  0x1B
#define EFI_SECTION_LAST_SECTION_TYPE       0x1B

//
// ////////////////////////////////////////////////////////////////////////////
//
// Common section header
//
typedef struct {
  UINT8 Size[3];
  UINT8 Type;
} EFI_COMMON_SECTION_HEADER;

#define SECTION_SIZE(SectionHeaderPtr) \
    ((UINT32) (*((UINT32 *) ((EFI_COMMON_SECTION_HEADER *) SectionHeaderPtr)->Size) & 0x00ffffff))

//
// ////////////////////////////////////////////////////////////////////////////
//
// Compression section
//
//
// CompressionType values
//
#define EFI_NOT_COMPRESSED          0x00
#define EFI_STANDARD_COMPRESSION    0x01
#define EFI_CUSTOMIZED_COMPRESSION  0x02

typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
  UINT32                    UncompressedLength;
  UINT8                     CompressionType;
} EFI_COMPRESSION_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// GUID defined section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
  EFI_GUID                  SectionDefinitionGuid;
  UINT16                    DataOffset;
  UINT16                    Attributes;
} EFI_GUID_DEFINED_SECTION;

//
// Bit values for Attributes
//
#define EFI_GUIDED_SECTION_PROCESSING_REQUIRED  0x01
#define EFI_GUIDED_SECTION_AUTH_STATUS_VALID    0x02

//
// Bit values for AuthenticationStatus
//
#define EFI_AGGREGATE_AUTH_STATUS_PLATFORM_OVERRIDE 0x000001
#define EFI_AGGREGATE_AUTH_STATUS_IMAGE_SIGNED      0x000002
#define EFI_AGGREGATE_AUTH_STATUS_NOT_TESTED        0x000004
#define EFI_AGGREGATE_AUTH_STATUS_TEST_FAILED       0x000008
#define EFI_AGGREGATE_AUTH_STATUS_ALL               0x00000f

#define EFI_LOCAL_AUTH_STATUS_PLATFORM_OVERRIDE     0x010000
#define EFI_LOCAL_AUTH_STATUS_IMAGE_SIGNED          0x020000
#define EFI_LOCAL_AUTH_STATUS_NOT_TESTED            0x040000
#define EFI_LOCAL_AUTH_STATUS_TEST_FAILED           0x080000
#define EFI_LOCAL_AUTH_STATUS_ALL                   0x0f0000

//
// ////////////////////////////////////////////////////////////////////////////
//
// PE32+ section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
} EFI_PE32_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// PIC section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
} EFI_PIC_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// PEIM header section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
} EFI_PEIM_HEADER_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// DEPEX section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
} EFI_DEPEX_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// Version section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
  UINT16                    BuildNumber;
  INT16                     VersionString[1];
} EFI_VERSION_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// User interface section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
  INT16                     FileNameString[1];
} EFI_USER_INTERFACE_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// Code16 section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
} EFI_CODE16_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// Firmware Volume Image section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
} EFI_FIRMWARE_VOLUME_IMAGE_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// Freeform subtype GUID section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
  EFI_GUID                  SubTypeGuid;
} EFI_FREEFORM_SUBTYPE_GUID_SECTION;

//
// ////////////////////////////////////////////////////////////////////////////
//
// Raw section
//
typedef struct {
  EFI_COMMON_SECTION_HEADER CommonHeader;
} EFI_RAW_SECTION;

//
// undo the pragma from the beginning...
//
#pragma pack()

typedef union {
  EFI_COMMON_SECTION_HEADER         *CommonHeader;
  EFI_COMPRESSION_SECTION           *CompressionSection;
  EFI_GUID_DEFINED_SECTION          *GuidDefinedSection;
  EFI_PE32_SECTION                  *Pe32Section;
  EFI_PIC_SECTION                   *PicSection;
  EFI_PEIM_HEADER_SECTION           *PeimHeaderSection;
  EFI_DEPEX_SECTION                 *DependencySection;
  EFI_VERSION_SECTION               *VersionSection;
  EFI_USER_INTERFACE_SECTION        *UISection;
  EFI_CODE16_SECTION                *Code16Section;
  EFI_FIRMWARE_VOLUME_IMAGE_SECTION *FVImageSection;
  EFI_FREEFORM_SUBTYPE_GUID_SECTION *FreeformSubtypeSection;
  EFI_RAW_SECTION                   *RawSection;
} EFI_FILE_SECTION_POINTER;

//
// EFI_FV_ATTRIBUTES bit definitions
//
typedef UINT64  EFI_FV_ATTRIBUTES;
typedef UINT32  EFI_FV_FILE_ATTRIBUTES;
typedef UINT32  EFI_FV_WRITE_POLICY;


typedef struct {
  EFI_GUID                *NameGuid;
  EFI_FV_FILETYPE         Type;
  EFI_FV_FILE_ATTRIBUTES  FileAttributes;
  VOID                    *Buffer;
  UINT32                  BufferSize;
} EFI_FV_WRITE_FILE_DATA;


#endif
