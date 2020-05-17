/** @file
  Legacy Region Support

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
  Modified by dmazar with support for different chipsets and added newer ones.

**/

#include "LegacyRegion2.h"

//
// Current chipset's tables
//
UINT32                              mVendorDeviceId = 0;
STATIC PAM_REGISTER_VALUE           *mRegisterValues = NULL;
UINT8                               mPamPciBus = 0;
UINT8                               mPamPciDev = 0;
UINT8                               mPamPciFunc = 0;

//
// Intel 830 Chipset and similar
//

//
// 440 PAM map.
//
// PAM Range       Offset  Bits  Operation
// =============== ======  ====  ===============================================================
// 0xC0000-0xC3FFF  0x5a   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xC4000-0xC7FFF  0x5a   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xC8000-0xCBFFF  0x5b   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xCC000-0xCFFFF  0x5b   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD0000-0xD3FFF  0x5c   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD4000-0xD7FFF  0x5c   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD8000-0xDBFFF  0x5d   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xDC000-0xDFFFF  0x5d   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE0000-0xE3FFF  0x5e   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE4000-0xE7FFF  0x5e   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE8000-0xEBFFF  0x5f   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xEC000-0xEFFFF  0x5f   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xF0000-0xFFFFF  0x59   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
//
STATIC LEGACY_MEMORY_SECTION_INFO   mSectionArray[] = {
  {0xC0000, SIZE_16KB, FALSE, FALSE},
  {0xC4000, SIZE_16KB, FALSE, FALSE},
  {0xC8000, SIZE_16KB, FALSE, FALSE},
  {0xCC000, SIZE_16KB, FALSE, FALSE},
  {0xD0000, SIZE_16KB, FALSE, FALSE},
  {0xD4000, SIZE_16KB, FALSE, FALSE},
  {0xD8000, SIZE_16KB, FALSE, FALSE},
  {0xDC000, SIZE_16KB, FALSE, FALSE},
  {0xE0000, SIZE_16KB, FALSE, FALSE},
  {0xE4000, SIZE_16KB, FALSE, FALSE},
  {0xE8000, SIZE_16KB, FALSE, FALSE},
  {0xEC000, SIZE_16KB, FALSE, FALSE},
  {0xF0000, SIZE_64KB, FALSE, FALSE}
};

STATIC PAM_REGISTER_VALUE  mRegisterValues830[] = {
  {REG_PAM1_OFFSET_830, 0x01, 0x02},
  {REG_PAM1_OFFSET_830, 0x10, 0x20},
  {REG_PAM2_OFFSET_830, 0x01, 0x02},
  {REG_PAM2_OFFSET_830, 0x10, 0x20},
  {REG_PAM3_OFFSET_830, 0x01, 0x02},
  {REG_PAM3_OFFSET_830, 0x10, 0x20},
  {REG_PAM4_OFFSET_830, 0x01, 0x02},
  {REG_PAM4_OFFSET_830, 0x10, 0x20},
  {REG_PAM5_OFFSET_830, 0x01, 0x02},
  {REG_PAM5_OFFSET_830, 0x10, 0x20},
  {REG_PAM6_OFFSET_830, 0x01, 0x02},
  {REG_PAM6_OFFSET_830, 0x10, 0x20},
  {REG_PAM0_OFFSET_830, 0x10, 0x20}
};


//
// Intel 4 Series Chipset and similar
//

//
// PAM map.
//
// PAM Range       Offset  Bits  Operation
// =============== ======  ====  ===============================================================
// 0xC0000-0xC3FFF  0x91   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xC4000-0xC7FFF  0x91   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xC8000-0xCBFFF  0x92   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xCC000-0xCFFFF  0x92   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD0000-0xD3FFF  0x93   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD4000-0xD7FFF  0x93   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD8000-0xDBFFF  0x94   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xDC000-0xDFFFF  0x94   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE0000-0xE3FFF  0x95   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE4000-0xE7FFF  0x95   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE8000-0xEBFFF  0x96   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xEC000-0xEFFFF  0x96   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xF0000-0xFFFFF  0x90   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
//

STATIC PAM_REGISTER_VALUE  mRegisterValuesS4[] = {
    {REG_PAM1_OFFSET_S4, 0x01, 0x02},
    {REG_PAM1_OFFSET_S4, 0x10, 0x20},
    {REG_PAM2_OFFSET_S4, 0x01, 0x02},
    {REG_PAM2_OFFSET_S4, 0x10, 0x20},
    {REG_PAM3_OFFSET_S4, 0x01, 0x02},
    {REG_PAM3_OFFSET_S4, 0x10, 0x20},
    {REG_PAM4_OFFSET_S4, 0x01, 0x02},
    {REG_PAM4_OFFSET_S4, 0x10, 0x20},
    {REG_PAM5_OFFSET_S4, 0x01, 0x02},
    {REG_PAM5_OFFSET_S4, 0x10, 0x20},
    {REG_PAM6_OFFSET_S4, 0x01, 0x02},
    {REG_PAM6_OFFSET_S4, 0x10, 0x20},
    {REG_PAM0_OFFSET_S4, 0x10, 0x20}
};

//
// Core processors
//

//
// PAM map.
//
// PAM Range       Offset  Bits  Operation
// =============== ======  ====  ===============================================================
// 0xC0000-0xC3FFF  0x81   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xC4000-0xC7FFF  0x81   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xC8000-0xCBFFF  0x82   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xCC000-0xCFFFF  0x82   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD0000-0xD3FFF  0x83   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD4000-0xD7FFF  0x83   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xD8000-0xDBFFF  0x84   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xDC000-0xDFFFF  0x84   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE0000-0xE3FFF  0x85   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE4000-0xE7FFF  0x85   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xE8000-0xEBFFF  0x86   1:0   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xEC000-0xEFFFF  0x86   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
// 0xF0000-0xFFFFF  0x80   5:4   00 = DRAM Disabled, 01= Read Only, 10 = Write Only, 11 = Normal
//

STATIC PAM_REGISTER_VALUE  mRegisterValuesCP[] = {
  {REG_PAM1_OFFSET_CP, 0x01, 0x02},
  {REG_PAM1_OFFSET_CP, 0x10, 0x20},
  {REG_PAM2_OFFSET_CP, 0x01, 0x02},
  {REG_PAM2_OFFSET_CP, 0x10, 0x20},
  {REG_PAM3_OFFSET_CP, 0x01, 0x02},
  {REG_PAM3_OFFSET_CP, 0x10, 0x20},
  {REG_PAM4_OFFSET_CP, 0x01, 0x02},
  {REG_PAM4_OFFSET_CP, 0x10, 0x20},
  {REG_PAM5_OFFSET_CP, 0x01, 0x02},
  {REG_PAM5_OFFSET_CP, 0x10, 0x20},
  {REG_PAM6_OFFSET_CP, 0x01, 0x02},
  {REG_PAM6_OFFSET_CP, 0x10, 0x20},
  {REG_PAM0_OFFSET_CP, 0x10, 0x20}
};

STATIC PAM_REGISTER_VALUE  mRegisterValuesNH[] = {
  {REG_PAM1_OFFSET_NH, 0x01, 0x02},
  {REG_PAM1_OFFSET_NH, 0x10, 0x20},
  {REG_PAM2_OFFSET_NH, 0x01, 0x02},
  {REG_PAM2_OFFSET_NH, 0x10, 0x20},
  {REG_PAM3_OFFSET_NH, 0x01, 0x02},
  {REG_PAM3_OFFSET_NH, 0x10, 0x20},
  {REG_PAM4_OFFSET_NH, 0x01, 0x02},
  {REG_PAM4_OFFSET_NH, 0x10, 0x20},
  {REG_PAM5_OFFSET_NH, 0x01, 0x02},
  {REG_PAM5_OFFSET_NH, 0x10, 0x20},
  {REG_PAM6_OFFSET_NH, 0x01, 0x02},
  {REG_PAM6_OFFSET_NH, 0x10, 0x20},
  {REG_PAM0_OFFSET_NH, 0x10, 0x20}
};

//
// NForce chipset
//

STATIC PAM_REGISTER_VALUE  mRegisterValuesNV[] = {
  {REG_PAM1_OFFSET_NV, 0x01, 0x02},
  {REG_PAM1_OFFSET_NV, 0x10, 0x20},
  {REG_PAM2_OFFSET_NV, 0x01, 0x02},
  {REG_PAM2_OFFSET_NV, 0x10, 0x20},
  {REG_PAM3_OFFSET_NV, 0x01, 0x02},
  {REG_PAM3_OFFSET_NV, 0x10, 0x20},
  {REG_PAM4_OFFSET_NV, 0x01, 0x02},
  {REG_PAM4_OFFSET_NV, 0x10, 0x20},
  {REG_PAM5_OFFSET_NV, 0x01, 0x02},
  {REG_PAM5_OFFSET_NV, 0x10, 0x20},
  {REG_PAM6_OFFSET_NV, 0x01, 0x02},
  {REG_PAM6_OFFSET_NV, 0x10, 0x20},
  {REG_PAM0_OFFSET_NV, 0x10, 0x20}
};

//
// Handle used to install the Legacy Region Protocol
//
STATIC EFI_HANDLE  mHandle = NULL;

//
// Instance of the Legacy Region Protocol to install into the handle database
//
STATIC EFI_LEGACY_REGION2_PROTOCOL  mLegacyRegion2 = {
  LegacyRegion2Decode,
  LegacyRegion2Lock,
  LegacyRegion2BootLock,
  LegacyRegion2Unlock,
  LegacyRegionGetInfo
};

STATIC
EFI_STATUS
LegacyRegionManipulationInternal (
  IN  UINT32                  Start,
  IN  UINT32                  Length,
  IN  BOOLEAN                 *ReadEnable,
  IN  BOOLEAN                 *WriteEnable,
  OUT UINT32                  *Granularity
  )
{
  UINT32                        EndAddress;
  UINTN                         Index;
  UINTN                         StartIndex;

  //
  // Validate input parameters.
  //
  if (Length == 0 || Granularity == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  EndAddress = Start + Length - 1;
  if ((Start < PAM_BASE_ADDRESS) || EndAddress > PAM_LIMIT_ADDRESS) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Loop to find the start PAM.
  //
  StartIndex = 0;
  for (Index = 0; Index < (sizeof(mSectionArray) / sizeof(mSectionArray[0])); Index++) {
    if ((Start >= mSectionArray[Index].Start) && (Start < (mSectionArray[Index].Start + mSectionArray[Index].Length))) {
      StartIndex = Index;
      break;
    }
  }
  ASSERT (Index < (sizeof(mSectionArray) / sizeof(mSectionArray[0])));

  //
  // Program PAM until end PAM is encountered
  //
  for (Index = StartIndex; Index < (sizeof(mSectionArray) / sizeof(mSectionArray[0])); Index++) {
    if (ReadEnable != NULL) {
      if (*ReadEnable) {
        PciOr8 (
          PCI_LIB_ADDRESS(mPamPciBus, mPamPciDev, mPamPciFunc, mRegisterValues[Index].PAMRegOffset),
          mRegisterValues[Index].ReadEnableData
          );
      } else {
        PciAnd8 (
          PCI_LIB_ADDRESS(mPamPciBus, mPamPciDev, mPamPciFunc, mRegisterValues[Index].PAMRegOffset),
          (UINT8) (~mRegisterValues[Index].ReadEnableData)
          );
      }
    }
    if (WriteEnable != NULL) {
      if (*WriteEnable) {
        PciOr8 (
          PCI_LIB_ADDRESS(mPamPciBus, mPamPciDev, mPamPciFunc, mRegisterValues[Index].PAMRegOffset),
          mRegisterValues[Index].WriteEnableData
          );
      } else {
        PciAnd8 (
          PCI_LIB_ADDRESS(mPamPciBus, mPamPciDev, mPamPciFunc, mRegisterValues[Index].PAMRegOffset),
          (UINT8) (~mRegisterValues[Index].WriteEnableData)
          );
      }
    }

    //
    // If the end PAM is encountered, record its length as granularity and jump out.
    //
    if ((EndAddress >= mSectionArray[Index].Start) && (EndAddress < (mSectionArray[Index].Start + mSectionArray[Index].Length))) {
      *Granularity = mSectionArray[Index].Length;
      break;
    }
  }
  ASSERT (Index < (sizeof(mSectionArray) / sizeof(mSectionArray[0])));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
LegacyRegionGetInfoInternal (
  OUT UINT32                        *DescriptorCount,
  OUT LEGACY_MEMORY_SECTION_INFO    **Descriptor
  )
{
  UINTN    Index;
  UINT8    PamValue;

  //
  // Check input parameters
  //
  if (DescriptorCount == NULL || Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Fill in current status of legacy region.
  //
  *DescriptorCount = (sizeof(mSectionArray) / sizeof(mSectionArray[0]));
  for (Index = 0; Index < *DescriptorCount; Index++) {
    PamValue = PciRead8 (PCI_LIB_ADDRESS(mPamPciBus, mPamPciDev, mPamPciFunc, mRegisterValues[Index].PAMRegOffset));
    mSectionArray[Index].ReadEnabled = FALSE;
    if ((PamValue & mRegisterValues[Index].ReadEnableData) != 0) {
      mSectionArray[Index].ReadEnabled = TRUE;
    }
    mSectionArray[Index].WriteEnabled = FALSE;
    if ((PamValue & mRegisterValues[Index].WriteEnableData) != 0) {
      mSectionArray[Index].WriteEnabled = TRUE;
    }
  }

  *Descriptor = mSectionArray;
  return EFI_SUCCESS;
}

/**
  Modify the hardware to allow (decode) or disallow (not decode) memory reads in a region.

  If the On parameter evaluates to TRUE, this function enables memory reads in the address range
  Start to (Start + Length - 1).
  If the On parameter evaluates to FALSE, this function disables memory reads in the address range
  Start to (Start + Length - 1).

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose attributes
                                should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address
                                was not aligned to a region's starting address or if the length
                                was greater than the number of bytes in the first region.
  @param  On[in]                Decode / Non-Decode flag.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2Decode (
  IN  EFI_LEGACY_REGION2_PROTOCOL  *This,
  IN  UINT32                       Start,
  IN  UINT32                       Length,
  OUT UINT32                       *Granularity,
  IN  BOOLEAN                      *On
  )
{
  return LegacyRegionManipulationInternal (Start, Length, On, NULL, Granularity);
}


/**
  Modify the hardware to disallow memory attribute changes in a region.

  This function makes the attributes of a region read only. Once a region is boot-locked with this
  function, the read and write attributes of that region cannot be changed until a power cycle has
  reset the boot-lock attribute. Calls to Decode(), Lock() and Unlock() will have no effect.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose
                                attributes should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address was
                                not aligned to a region's starting address or if the length was
                                greater than the number of bytes in the first region.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.
  @retval EFI_UNSUPPORTED       The chipset does not support locking the configuration registers in
                                a way that will not affect memory regions outside the legacy memory
                                region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2BootLock (
  IN  EFI_LEGACY_REGION2_PROTOCOL         *This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  OUT UINT32                              *Granularity
  )
{
  if ((Start < 0xC0000) || ((Start + Length - 1) > 0xFFFFF)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_UNSUPPORTED;
}


/**
  Modify the hardware to disallow memory writes in a region.

  This function changes the attributes of a memory range to not allow writes.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose
                                attributes should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address was
                                not aligned to a region's starting address or if the length was
                                greater than the number of bytes in the first region.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2Lock (
  IN  EFI_LEGACY_REGION2_PROTOCOL *This,
  IN  UINT32                      Start,
  IN  UINT32                      Length,
  OUT UINT32                      *Granularity
  )
{
  BOOLEAN  WriteEnable;

  WriteEnable = FALSE;
  return LegacyRegionManipulationInternal (Start, Length, NULL, &WriteEnable, Granularity);
}


/**
  Modify the hardware to allow memory writes in a region.

  This function changes the attributes of a memory range to allow writes.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  Start[in]             The beginning of the physical address of the region whose
                                attributes should be modified.
  @param  Length[in]            The number of bytes of memory whose attributes should be modified.
                                The actual number of bytes modified may be greater than the number
                                specified.
  @param  Granularity[out]      The number of bytes in the last region affected. This may be less
                                than the total number of bytes affected if the starting address was
                                not aligned to a region's starting address or if the length was
                                greater than the number of bytes in the first region.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegion2Unlock (
  IN  EFI_LEGACY_REGION2_PROTOCOL  *This,
  IN  UINT32                       Start,
  IN  UINT32                       Length,
  OUT UINT32                       *Granularity
  )
{
  BOOLEAN  WriteEnable;

  WriteEnable = TRUE;
  return LegacyRegionManipulationInternal (Start, Length, NULL, &WriteEnable, Granularity);
}

/**
  Get region information for the attributes of the Legacy Region.

  This function is used to discover the granularity of the attributes for the memory in the legacy
  region. Each attribute may have a different granularity and the granularity may not be the same
  for all memory ranges in the legacy region.

  @param  This[in]              Indicates the EFI_LEGACY_REGION_PROTOCOL instance.
  @param  DescriptorCount[out]  The number of region descriptor entries returned in the Descriptor
                                buffer.
  @param  Descriptor[out]       A pointer to a pointer used to return a buffer where the legacy
                                region information is deposited. This buffer will contain a list of
                                DescriptorCount number of region descriptors.  This function will
                                provide the memory for the buffer.

  @retval EFI_SUCCESS           The region's attributes were successfully modified.
  @retval EFI_INVALID_PARAMETER If Start or Length describe an address not in the Legacy Region.

**/
EFI_STATUS
EFIAPI
LegacyRegionGetInfo (
  IN  EFI_LEGACY_REGION2_PROTOCOL   *This,
  OUT UINT32                        *DescriptorCount,
  OUT EFI_LEGACY_REGION_DESCRIPTOR  **Descriptor
  )
{
  LEGACY_MEMORY_SECTION_INFO   *SectionInfo;
  UINT32                       SectionCount;
  EFI_LEGACY_REGION_DESCRIPTOR *DescriptorArray;
  UINTN                        Index;
  UINTN                        DescriptorIndex;

  //
  // Get section numbers and information
  //
  LegacyRegionGetInfoInternal (&SectionCount, &SectionInfo);

  //
  // Each section has 3 descriptors, corresponding to readability, writeability, and lock status.
  //
  DescriptorArray = AllocatePool (sizeof (EFI_LEGACY_REGION_DESCRIPTOR) * SectionCount * 3);
  if (DescriptorArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DescriptorIndex = 0;
  for (Index = 0; Index < SectionCount; Index++) {
    DescriptorArray[DescriptorIndex].Start       = SectionInfo[Index].Start;
    DescriptorArray[DescriptorIndex].Length      = SectionInfo[Index].Length;
    DescriptorArray[DescriptorIndex].Granularity = SectionInfo[Index].Length;
    if (SectionInfo[Index].ReadEnabled) {
      DescriptorArray[DescriptorIndex].Attribute   = LegacyRegionDecoded;
    } else {
      DescriptorArray[DescriptorIndex].Attribute   = LegacyRegionNotDecoded;
    }
    DescriptorIndex++;

    //
    // Create descriptor for writeability, according to lock status
    //
    DescriptorArray[DescriptorIndex].Start       = SectionInfo[Index].Start;
    DescriptorArray[DescriptorIndex].Length      = SectionInfo[Index].Length;
    DescriptorArray[DescriptorIndex].Granularity = SectionInfo[Index].Length;
    if (SectionInfo[Index].WriteEnabled) {
      DescriptorArray[DescriptorIndex].Attribute = LegacyRegionWriteEnabled;
    } else {
      DescriptorArray[DescriptorIndex].Attribute = LegacyRegionWriteDisabled;
    }
    DescriptorIndex++;

    //
    // Chipset does not support bootlock.
    //
    DescriptorArray[DescriptorIndex].Start       = SectionInfo[Index].Start;
    DescriptorArray[DescriptorIndex].Length      = SectionInfo[Index].Length;
    DescriptorArray[DescriptorIndex].Granularity = SectionInfo[Index].Length;
    DescriptorArray[DescriptorIndex].Attribute   = LegacyRegionNotLocked;
    DescriptorIndex++;
  }

  *DescriptorCount = (UINT32) DescriptorIndex;
  *Descriptor      = DescriptorArray;

  return EFI_SUCCESS;
}


/**
  Detects chipset and initialize PAM support tables
 
  @retval EFI_SUCCESS   Successfully initialized
 
**/
EFI_STATUS
DetectChipset (
  VOID
  )
{
  UINT16  VID = 0;
  UINT16  DID = 0;

  mRegisterValues = NULL;
  
  mVendorDeviceId = PciRead32 (PCI_LIB_ADDRESS(PAM_PCI_BUS, PAM_PCI_DEV, PAM_PCI_FUNC, 0));

  switch (mVendorDeviceId) {
    
    //
    // Intel 830 and similar
    // Copied from 915 resolution created by steve tomljenovic,
    // Resolution module by Evan Lojewski
    //
    case 0x35758086: // 830
    case 0x35808086: // 855GM
      /// Intel 830 and similar (PAM 0x59-0x5f).
      mRegisterValues = mRegisterValues830;
      break;

    case 0x25C08086: // 5000
    case 0x25D48086: // 5000V
    case 0x65C08086: // 5100
      /// Intel 5000 and similar (PAM 0x59-0x5f).
      mRegisterValues = mRegisterValues830;
      mPamPciDev = 16;
      break;
      
    //
    // Intel Series 4 and similar
    // Copied from 915 resolution created by steve tomljenovic,
    // Resolution module by Evan Lojewski
    //
    case 0x25608086: // 845G
    case 0x25708086: // 865G
    case 0x25808086: // 915G
    case 0x25908086: // 915GM

    case 0x27708086: // 945G
    case 0x27748086: // 955X
    case 0x277c8086: // 975X
    case 0x27a08086: // 945GM - Dell D430  Offset 090:  10 11 11 00 00
    case 0x27ac8086: // 945GME
    case 0x29208086: // G45
    case 0x29708086: // 946GZ
    case 0x29808086: // G965
    case 0x29908086: // Q965
    case 0x29a08086: // P965
    case 0x29b08086: // R845  
    case 0x29c08086: // G31/P35
    case 0x29d08086: // Q33
    case 0x29e08086: // X38/X48
    case 0x2a008086: // 965GM
    case 0x2a108086: // GME965/GLE960
    case 0x2a408086: // PM/GM45/47
    case 0x2e008086: // Eaglelake
    case 0x2e108086: // B43
    case 0x2e208086: // P45
    case 0x2e308086: // G41
    case 0x2e408086: // B43 Base
    case 0x2e908086: // B43 Soft Sku
    case 0x81008086: // 500
    case 0xA0008086: // 3150
      /// Intel Series 4 and similar (PAM 0x90-0x96).
      mRegisterValues = mRegisterValuesS4;
      break;
          
    //
    // Core processors
    // http://pci-ids.ucw.cz/read/PC/8086
    //
    case 0x01008086: // 2nd Generation Core Processor Family DRAM Controller
    case 0x01048086: // 2nd Generation Core Processor Family DRAM Controller
    case 0x01088086: // Xeon E3-1200 2nd Generation Core Processor Family DRAM Controller
    case 0x010c8086: // Xeon E3-1200 2nd Generation Core Processor Family DRAM Controller
      
    case 0x01508086: // 3rd Generation Core Processor Family DRAM Controller
    case 0x01548086: // 3rd Generation Core Processor Family DRAM Controller
    case 0x01588086: // 3rd Generation Core Processor Family DRAM Controller
    case 0x015c8086: // 3rd Generation Core Processor Family DRAM Controller
      
    case 0x01608086: // 3rd Generation Core Processor Family DRAM Controller
    case 0x01648086: // 3rd Generation Core Processor Family DRAM Controller

    case 0x0C008086: // 4rd Generation Core Processor Family DRAM Controller
    case 0x0C048086: // 4rd Generation M-Processor Series
    case 0x0C088086: // 4rd Generation Haswell Xeon
    case 0x0A048086: // 4rd Generation U-Processor Series
    case 0x0D048086: // 4rd Generation H-Processor Series (BGA) with GT3 Graphics
    case 0x16048086: // 5th Generation Core Processor Family DRAM Controller

    case 0x191f8086: // 6th Generation (Skylake) DRAM Controller (Z170X)
          
    case 0x0F008086: // Bay Trail Family DRAM Controller
      /// Next Generation Core processors (PAM 0x80-0x86).
      mRegisterValues = mRegisterValuesCP;
      break;

    //
    // 1st gen i7 - Nehalem
    //
    case 0x00408086: // Core Processor DRAM Controller
    case 0x00448086: // Core Processor DRAM Controller - Arrandale
    case 0x00488086: // Core Processor DRAM Controller
    case 0x00698086: // Core Processor DRAM Controller

    case 0xD1308086: // Xeon(R) CPU L3426 Processor DRAM Controller
    case 0xD1318086: // Core-i Processor DRAM Controller
    case 0xD1328086: // PM55 i7-720QM  DRAM Controller  
    case 0x34008086: // Core-i Processor DRAM Controller   
    case 0x34018086: // Core-i Processor DRAM Controller   
    case 0x34028086: // Core-i Processor DRAM Controller   
    case 0x34038086: // Core-i Processor DRAM Controller   
    case 0x34048086: // Core-i Processor DRAM Controller   
    case 0x34058086: // X58 Core-i Processor DRAM Controller   
    case 0x34068086: // Core-i Processor DRAM Controller   
    case 0x34078086: // Core-i Processor DRAM Controller   
      /// Core i7 processors (PAM 0x40-0x47).
      mRegisterValues = mRegisterValuesNH;
      mPamPciBus = 0xFF;
      for (mPamPciBus = 0xFF; mPamPciBus > 0x1F; mPamPciBus >>= 1) {
        VID = PciRead16 (PCI_LIB_ADDRESS(mPamPciBus, 0, 1, 0x00));
        if (VID != 0x8086) {
          continue;
        }
        DID = PciRead16 (PCI_LIB_ADDRESS(mPamPciBus, 0, 1, 0x02));
        if (DID > 0x2c00) {
          break;
        }
      } 
      if ((VID != 0x8086) || (DID < 0x2c00)) {
        //
        // Nehalem bus is not found, assume 0.
        //
        mPamPciBus = 0;
      } else {
        mPamPciFunc = 1;
      }
      break;
    case 0x3C008086: // Xeon E5 Processor 
      /// Xeon E5 processors (PAM 0x40-0x47).
      mRegisterValues = mRegisterValuesNH;
      mPamPciBus = PciRead8 (PCI_LIB_ADDRESS(0, 5, 0, 0x109));
      mPamPciDev = 12;
      mPamPciFunc = 6;
      break;

    case 0x0a8210de:
    case 0x0a8610de: 
      /// NForce MCP79 and similar (PAM 0xC0-0xC7).
      mRegisterValues = mRegisterValuesNV;
      break;      
      
    default:
      // Unknown chipset.
      break;
  }

  return mRegisterValues != NULL ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  Initialize Legacy Region support

  @retval EFI_SUCCESS   Successfully initialized

**/
EFI_STATUS
EFIAPI
LegacyRegion2Install (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Protocol;
  
  //
  // Check for presence of gEfiLegacyRegionProtocolGuid
  // and gEfiLegacyRegion2ProtocolGuid
  //
  Status = gBS->LocateProtocol (&gEfiLegacyRegionProtocolGuid, NULL, (VOID **) &Protocol);
  if (Status == EFI_SUCCESS) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->LocateProtocol (&gEfiLegacyRegion2ProtocolGuid, NULL, (VOID **) &Protocol);
  if (Status == EFI_SUCCESS) {
    return EFI_UNSUPPORTED;
  }
  
  Status = DetectChipset ();
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Install the Legacy Region Protocol on a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiLegacyRegion2ProtocolGuid,
                  &mLegacyRegion2,
                  NULL
                  );

  return Status;
}
