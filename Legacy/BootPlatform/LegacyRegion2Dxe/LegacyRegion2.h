/** @file
  Legacy Region Support

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LEGACY_REGION_DXE_H_
#define _LEGACY_REGION_DXE_H_

#include <PiDxe.h>

#include <Protocol/LegacyRegion.h>
#include <Protocol/LegacyRegion2.h>

#include <IndustryStandard/Pci.h>

#include <Library/PciLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define PAM_PCI_BUS        0
#define PAM_PCI_DEV        0
#define PAM_PCI_FUNC       0

#define REG_PAM0_OFFSET_NH     0x40    // Programmable Attribute Map 0
#define REG_PAM1_OFFSET_NH     0x41    // Programmable Attribute Map 1
#define REG_PAM2_OFFSET_NH     0x42    // Programmable Attribute Map 2
#define REG_PAM3_OFFSET_NH     0x43    // Programmable Attribute Map 3
#define REG_PAM4_OFFSET_NH     0x44    // Programmable Attribute Map 4
#define REG_PAM5_OFFSET_NH     0x45    // Programmable Attribute Map 5
#define REG_PAM6_OFFSET_NH     0x46    // Programmable Attribute Map 6


#define REG_PAM0_OFFSET_830    0x59    // Programmable Attribute Map 0
#define REG_PAM1_OFFSET_830    0x5a    // Programmable Attribute Map 1
#define REG_PAM2_OFFSET_830    0x5b    // Programmable Attribute Map 2
#define REG_PAM3_OFFSET_830    0x5c    // Programmable Attribute Map 3
#define REG_PAM4_OFFSET_830    0x5d    // Programmable Attribute Map 4
#define REG_PAM5_OFFSET_830    0x5e    // Programmable Attribute Map 5
#define REG_PAM6_OFFSET_830    0x5f    // Programmable Attribute Map 6

#define REG_PAM0_OFFSET_S4     0x90    // Programmable Attribute Map 0
#define REG_PAM1_OFFSET_S4     0x91    // Programmable Attribute Map 1
#define REG_PAM2_OFFSET_S4     0x92    // Programmable Attribute Map 2
#define REG_PAM3_OFFSET_S4     0x93    // Programmable Attribute Map 3
#define REG_PAM4_OFFSET_S4     0x94    // Programmable Attribute Map 4
#define REG_PAM5_OFFSET_S4     0x95    // Programmable Attribute Map 5
#define REG_PAM6_OFFSET_S4     0x96    // Programmable Attribute Map 6

#define REG_PAM0_OFFSET_CP     0x80    // Programmable Attribute Map 0
#define REG_PAM1_OFFSET_CP     0x81    // Programmable Attribute Map 1
#define REG_PAM2_OFFSET_CP     0x82    // Programmable Attribute Map 2
#define REG_PAM3_OFFSET_CP     0x83    // Programmable Attribute Map 3
#define REG_PAM4_OFFSET_CP     0x84    // Programmable Attribute Map 4
#define REG_PAM5_OFFSET_CP     0x85    // Programmable Attribute Map 5
#define REG_PAM6_OFFSET_CP     0x86    // Programmable Attribute Map 6

#define REG_PAM0_OFFSET_NV     0xC0    // Programmable Attribute Map 0
#define REG_PAM1_OFFSET_NV     0xC1    // Programmable Attribute Map 1
#define REG_PAM2_OFFSET_NV     0xC2    // Programmable Attribute Map 2
#define REG_PAM3_OFFSET_NV     0xC3    // Programmable Attribute Map 3
#define REG_PAM4_OFFSET_NV     0xC4    // Programmable Attribute Map 4
#define REG_PAM5_OFFSET_NV     0xC5    // Programmable Attribute Map 5
#define REG_PAM6_OFFSET_NV     0xC6    // Programmable Attribute Map 6

#define PAM_BASE_ADDRESS   0xc0000
#define PAM_LIMIT_ADDRESS  BASE_1MB

//
// Describes Legacy Region blocks and status.
//
typedef struct {
  UINT32  Start;
  UINT32  Length;
  BOOLEAN ReadEnabled;
  BOOLEAN WriteEnabled;
} LEGACY_MEMORY_SECTION_INFO;

//
// Provides a map of the PAM registers and bits used to set Read/Write access.
//
typedef struct {
  UINT8   PAMRegOffset;
  UINT8   ReadEnableData;
  UINT8   WriteEnableData;
} PAM_REGISTER_VALUE;

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
  );

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
  );

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
  IN EFI_LEGACY_REGION2_PROTOCOL          *This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  OUT UINT32                              *Granularity
  );

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
  );

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
  );

#endif
