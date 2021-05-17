/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_PROCESSOR_INFO_H
#define OC_PROCESSOR_INFO_H

// SandyBridge/IvyBridge bus clock is fixed at 100MHz

#define BRIDGE_BCLK  100

#define BASE_NHM_CLOCK_SOURCE  133333333ULL

//
// Skylake bus clock is fixed at 100MHz
// This constant is also known as BASE_ART_CLOCK_SOURCE in XNU
//
#define CLIENT_ART_CLOCK_SOURCE   24000000ULL
#define SERVER_ART_CLOCK_SOURCE   25000000ULL
#define ATOM_ART_CLOCK_SOURCE     19200000ULL

#define DEFAULT_ART_CLOCK_SOURCE  CLIENT_ART_CLOCK_SOURCE

#define MSR_PIC_MSG_CONTROL       0x2E
#define MSR_CORE_THREAD_COUNT     0x35

#define EFI_PLATFORM_INFORMATION                        0x000000CE
#define N_EFI_PLATFORM_INFO_MIN_RATIO                   40
#define B_EFI_PLATFORM_INFO_RATIO_MASK                  0xFF
#define N_EFI_PLATFORM_INFO_MAX_RATIO                   8
#define B_EFI_PLATFORM_INFO_TDC_TDP_LIMIT               (1 << 29)
#define N_EFI_PLATFORM_INFO_RATIO_LIMIT                 28
#define B_EFI_PLATFORM_INFO_RATIO_LIMIT                 (1 << 28)
#define B_EFI_PLATFORM_INFO_SMM_SAVE_CONTROL            (1 << 16)
#define N_EFI_PLATFORM_INFO_PROG_TCC_ACTIVATION_OFFSET  30
#define B_EFI_PLATFORM_INFO_PROG_TCC_ACTIVATION_OFFSET  (1 << 30)

//#define PLATFORM_INFO_SET_TDP

#define MSR_PMG_IO_CAPTURE_BASE           0xE4
#define MSR_IA32_EXT_CONFIG               0xEE
#define MSR_FEATURE_CONFIG                0x13C
#define MSR_FLEX_RATIO                    0x194
#define FLEX_RATIO_LOCK                   (1U << 20U)
#define FLEX_RATIO_EN                     (1U << 16U)
#define MSR_IA32_PERF_CONTROL             0x199
#define MSR_THERM2_CTL                    0x19D

#define MSR_IA32_MISC_ENABLES             0x1A0
#define TURBO_DISABLE_MASK                ((UINT64)1 << 38)
#define TURBO_MODE_DISABLE_BIT            38

#define MSR_TEMPERATURE_TARGET            0x1A2
#define MSR_MISC_PWR_MGMT                 0x1AA
#define MISC_PWR_MGMT_EIST_HW_DIS         (1 << 0)
#define MISC_PWR_MGMT_LOCK                (1 << 13)
#define MAX_RATIO_LIMIT_8C_OFFSET         56
#define MAX_RATIO_LIMIT_7C_OFFSET         48
#define MAX_RATIO_LIMIT_6C_OFFSET         40
#define MAX_RATIO_LIMIT_5C_OFFSET         32
#define MAX_RATIO_LIMIT_4C_OFFSET         24
#define MAX_RATIO_LIMIT_3C_OFFSET         16
#define MAX_RATIO_LIMIT_2C_OFFSET         8
#define MAX_RATIO_LIMIT_1C_OFFSET         0
#define MAX_RATIO_LIMIT_MASK              0xff
#define MSR_IA32_ENERGY_PERFORMANCE_BIAS  0x1B0
#define ENERGY_POLICY_PERFORMANCE         0
#define ENERGY_POLICY_NORMAL              6
#define ENERGY_POLICY_POWERSAVE           15
#define MSR_POWER_CTL                     0x1FC
#define MSR_LT_LOCK_MEMORY                0x2E7
#define MSR_IA32_CR_PAT                   0x277

// Sandy Bridge & JakeTown specific 'Running Average Power Limit' MSR's.
#define MSR_PP0_CURRENT_CONFIG            0x601
#define PP0_CURRENT_LIMIT                 (112 << 3) ///< 112 A
#define MSR_PP1_CURRENT_CONFIG            0x602
#define PP1_CURRENT_LIMIT                 (35 << 3)  ///< 35 A
#define MSR_PKG_POWER_SKU_UNIT            0x606

#define MSR_PKGC3_IRTL                    0x60A
#define MSR_PKGC6_IRTL                    0x60B
#define MSR_PKGC7_IRTL                    0x60C
#define IRTL_VALID                        (1 << 15)
#define IRTL_1_NS                         (0 << 10)
#define IRTL_32_NS                        (1 << 10)
#define IRTL_1024_NS                      (2 << 10)
#define IRTL_32768_NS                     (3 << 10)
#define IRTL_1048576_NS                   (4 << 10)
#define IRTL_33554432_NS                  (5 << 10)
#define IRTL_RESPONSE_MASK                (0x3ff)

// long duration in low dword, short duration in high dword
#define MSR_PKG_POWER_LIMIT               0x610
#define PKG_POWER_LIMIT_MASK              0x7fff
#define PKG_POWER_LIMIT_EN                (1 << 15)
#define PKG_POWER_LIMIT_CLAMP             (1 << 16)
#define PKG_POWER_LIMIT_TIME_SHIFT        17
#define PKG_POWER_LIMIT_TIME_MASK         0x7f

#define MSR_PKG_ENERGY_STATUS             0x611
#define MSR_PKG_PERF_STATUS               0x613
#define MSR_PKG_POWER_SKU                 0x614

// Sandy Bridge IA (Core) domain MSR's.
#define MSR_PP0_POWER_LIMIT               0x638
#define MSR_PP0_ENERGY_STATUS             0x639
#define MSR_PP0_POLICY                    0x63A
#define MSR_PP0_PERF_STATUS               0x63B

// Sandy Bridge Uncore (IGPU) domain MSR's (Not on JakeTown).
#define MSR_PP1_POWER_LIMIT               0x640
#define MSR_PP1_ENERGY_STATUS             0x641
#define MSR_PP1_POLICY                    0x642

// JakeTown only Memory MSR's.
#define MSR_DRAM_POWER_LIMIT              0x618
#define MSR_DRAM_ENERGY_STATUS            0x619
#define MSR_DRAM_PERF_STATUS              0x61B
#define MSR_DRAM_POWER_INFO               0x61C

/// x86 Page Address Translation
enum {
  PageAddressTranslationUncached            = 0,
  PageAddressTranslationWriteCombining      = 1,
  PageAddressTranslationWriteThrough        = 4,
  PageAddressTranslationWriteProtected      = 5,
  PageAddressTranslationWriteBack           = 6,
  /// Uncached, but can be overriden by MTRR
  PageAddressTranslationOverridableUncached = 7,
};

#define K8_FIDVID_STATUS   0xC0010042
#define K10_COFVID_STATUS  0xC0010071
#define K10_PSTATE_STATUS  0xC0010064

#define CPU_MODEL_WILLAMETTE     0x01  ///< Willamette, Foster
#define CPU_MODEL_NORTHWOOD      0x02  ///< Northwood, Prestonia, Gallatin
#define CPU_MODEL_PRESCOTT       0x03  ///< Prescott, Nocona, Cranford, Potomac
#define CPU_MODEL_PRESCOTT_2M    0x04  ///< Prescott 2M, Smithfield, Irwindale, Paxville
#define CPU_MODEL_CEDAR_MILL     0x06  ///< Cedar Mill, Presler, Tusla, Dempsey
#define CPU_MODEL_BANIAS         0x09  ///< Banias
#define CPU_MODEL_DOTHAN         0x0D  ///< Dothan
#define CPU_MODEL_YONAH          0x0E  ///< Sossaman, Yonah
#define CPU_MODEL_MEROM          0x0F  ///< Allendale, Conroe, Kentsfield, Woodcrest, Clovertown, Tigerton, Merom
#define CPU_MODEL_PENRYN         0x17  ///< Wolfdale, Yorkfield, Harpertown, Penryn
#define CPU_MODEL_NEHALEM        0x1A  ///< Bloomfield, Nehalem-EP, Nehalem-WS, Gainestown
#define CPU_MODEL_BONNELL        0x1C  ///< Bonnell, Silverthorne, Diamondville, Pineview
#define CPU_MODEL_FIELDS         0x1E  ///< Lynnfield, Clarksfield, Jasper Forest
#define CPU_MODEL_DALES          0x1F  ///< Havendale, Auburndale
#define CPU_MODEL_NEHALEM_EX     0x2E  ///< Beckton
#define CPU_MODEL_DALES_32NM     0x25  ///< Clarkdale, Arrandale
#define CPU_MODEL_BONNELL_MID    0x26  ///< Bonnell, Lincroft
#define CPU_MODEL_WESTMERE       0x2C  ///< Gulftown, Westmere-EP, Westmere-WS
#define CPU_MODEL_WESTMERE_EX    0x2F
#define CPU_MODEL_SANDYBRIDGE    0x2A  ///< Sandy Bridge
#define CPU_MODEL_JAKETOWN       0x2D  ///< Sandy Bridge Xeon E5, Core i7 Extreme
#define CPU_MODEL_SALTWELL       0x36  ///< Saltwell, Cedarview
#define CPU_MODEL_SILVERMONT     0x37  ///< Bay Trail
#define CPU_MODEL_IVYBRIDGE      0x3A  ///< Ivy Bridge
#define CPU_MODEL_IVYBRIDGE_EP   0x3E
#define CPU_MODEL_CRYSTALWELL    0x46
#define CPU_MODEL_HASWELL        0x3C
#define CPU_MODEL_HASWELL_EP     0x3F  ///< Haswell MB
#define CPU_MODEL_HASWELL_ULT    0x45  ///< Haswell ULT
#define CPU_MODEL_BROADWELL      0x3D  ///< Broadwell
#define CPU_MODEL_BROADWELL_EP   0x4F  ///< Broadwell_EP
#define CPU_MODEL_BROADWELL_ULX  0x3D
#define CPU_MODEL_BROADWELL_ULT  0x3D
#define CPU_MODEL_BRYSTALWELL    0x47
#define CPU_MODEL_AIRMONT        0x4C  ///< CherryTrail / Braswell
#define CPU_MODEL_AVOTON         0x4D  ///< Avaton/Rangely
#define CPU_MODEL_SKYLAKE        0x4E  ///< Skylake-S
#define CPU_MODEL_SKYLAKE_ULT    0x4E
#define CPU_MODEL_SKYLAKE_ULX    0x4E
#define CPU_MODEL_SKYLAKE_DT     0x5E
#define CPU_MODEL_SKYLAKE_W      0x55
#define CPU_MODEL_GOLDMONT       0x5C  ///< Apollo Lake
#define CPU_MODEL_DENVERTON      0x5F  ///< Goldmont Microserver
#define CPU_MODEL_CANNONLAKE     0x66
#define CPU_MODEL_XEON_MILL      0x85  ///< Knights Mill
#define CPU_MODEL_KABYLAKE       0x8E  ///< Kabylake Dektop
#define CPU_MODEL_KABYLAKE_ULT   0x8E
#define CPU_MODEL_KABYLAKE_ULX   0x8E
#define CPU_MODEL_KABYLAKE_DT    0x9E
#define CPU_MODEL_COFFEELAKE     0x9E
#define CPU_MODEL_COFFEELAKE_ULT 0x9E
#define CPU_MODEL_COFFEELAKE_ULX 0x9E
#define CPU_MODEL_COFFEELAKE_DT  0x9E
#define CPU_MODEL_ICELAKE_Y      0x7D
#define CPU_MODEL_ICELAKE_U      0x7E
#define CPU_MODEL_ICELAKE_SP     0x9F /* Some variation of Ice Lake */
#define CPU_MODEL_COMETLAKE_S    0xA5 /* desktop CometLake */
#define CPU_MODEL_COMETLAKE_Y    0xA5 /* aka 10th generation Amber Lake Y */
#define CPU_MODEL_COMETLAKE_U    0xA6
#define CPU_MODEL_ROCKETLAKE_S   0xA7 /* desktop RocketLake */
#define CPU_MODEL_TIGERLAKE_U    0x8C

#define AMD_CPU_FAMILY          0xF
#define AMD_CPU_EXT_FAMILY_15H  0x6
#define AMD_CPU_EXT_FAMILY_16H  0x7
#define AMD_CPU_EXT_FAMILY_17H  0x8
#define AMD_CPU_EXT_FAMILY_19H  0xA

// CPU_P_STATE_COORDINATION
/// P-State Coordination
typedef enum {
  /// The OS Power Manager is responsible for coordinating the P-state among logical
  /// processors with dependencies and must initiate the transition on all of those Logical Processors.
  CpuPStateCoordinationSoftwareAll = 0xFC,

  /// The OS Power Manager is responsible for coordinating the P-state among logical
  /// processors with dependencies and may initiate the transition on any of those Logical Processors.
  CpuPStateCoordinationSoftwareAny = 0xFD,

  /// The processor hardware is responsible for coordinating the P-state among logical
  /// processors dependencies. The OS is responsible for keeping the P-state request up to date on all
  /// logical processors.
  CpuPStateCoordinationHardwareAll = 0xFE
} CPU_P_STATE_COORDINATION;

#endif // OC_PROCESSOR_INFO_H
