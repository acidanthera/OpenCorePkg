/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef CPUID_H
#define CPUID_H

#include <Register/Cpuid.h>

#define CPUID_L2_CACHE_FEATURE  0x80000006

// Feature Flag Values Reported in the EDX Register

#define CPUID_FEATURE_FPU     BIT0   ///< Floating point unit on-chip
#define CPUID_FEATURE_VME     BIT1   ///< Virtual Mode Extension
#define CPUID_FEATURE_DE      BIT2   ///< Debugging Extension
#define CPUID_FEATURE_PSE     BIT3   ///< Page Size Extension
#define CPUID_FEATURE_TSC     BIT4   ///< Time Stamp Counter
#define CPUID_FEATURE_MSR     BIT5   ///< Model Specific Registers
#define CPUID_FEATURE_PAE     BIT6   ///< Physical Address Extension
#define CPUID_FEATURE_MCE     BIT7   ///< Machine Check Exception
#define CPUID_FEATURE_CX8     BIT8   ///< CMPXCHG8B
#define CPUID_FEATURE_APIC    BIT9   ///< On-chip APIC
#define CPUID_FEATURE_SEP     BIT11  ///< Fast System Call
#define CPUID_FEATURE_MTRR    BIT12  ///< Memory Type Range Register
#define CPUID_FEATURE_PGE     BIT13  ///< Page Global Enable
#define CPUID_FEATURE_MCA     BIT14  ///< Machine Check Architecture
#define CPUID_FEATURE_CMOV    BIT15  ///< Conditional Move Instruction
#define CPUID_FEATURE_PAT     BIT16  ///< Page Attribute Table
#define CPUID_FEATURE_PSE36   BIT17  ///< 36-bit Page Size Extension
#define CPUID_FEATURE_PSN     BIT18  ///< Processor Serial Number
#define CPUID_FEATURE_CLFSH   BIT19  ///< CLFLUSH Instruction Supported
#define CPUID_FEATURE_RESV20  BIT20  ///< Reserved
#define CPUID_FEATURE_DS      BIT21  ///< Debug Store
#define CPUID_FEATURE_ACPI    BIT22  ///< Thermal Monitor and Clock Control
#define CPUID_FEATURE_MMX     BIT23  ///< MMX Supported
#define CPUID_FEATURE_FXSR    BIT24  ///< Fast Floating Point Save/Restore
#define CPUID_FEATURE_SSE     BIT25  ///< Streaming SIMD Extensions
#define CPUID_FEATURE_SSE2    BIT26  ///< Streaming SIMD Extensions 2
#define CPUID_FEATURE_SS      BIT27  ///< Self-Snoop
#define CPUID_FEATURE_HTT     BIT28  ///< Hyper-Threading Technology
#define CPUID_FEATURE_TM      BIT29  ///< Thermal Monitor (TM1)
#define CPUID_FEATURE_IA64    BIT30  ///< Itanium Family Emulating IA-32
#define CPUID_FEATURE_PBE     BIT31  ///< Pending Break Enable

// Feature Flag Values Reported in the ECX Register

#define CPUID_FEATURE_SSE3       BIT32  ///< Streaming SIMD extensions 3
#define CPUID_FEATURE_PCLMULQDQ  BIT33  ///< PCLMULQDQ Instruction
#define CPUID_FEATURE_DTES64     BIT34  ///< 64-Bit Debug Store
#define CPUID_FEATURE_MONITOR    BIT35  ///< MONITOR/MWAIT
#define CPUID_FEATURE_DSCPL      BIT36  ///< CPL Qualified Debug Store
#define CPUID_FEATURE_VMX        BIT37  ///< Virtual Machine Extensions (VMX)
#define CPUID_FEATURE_SMX        BIT38  ///< Safer Mode Extensions (SMX)
#define CPUID_FEATURE_EST        BIT39  ///< Enhanced Intel SpeedStep (GV3)
#define CPUID_FEATURE_TM2        BIT40  ///< Thermal Monitor 2
#define CPUID_FEATURE_SSSE3      BIT41  ///< Supplemental SSE3 Instructions
#define CPUID_FEATURE_CID        BIT42  ///< L1 Context ID
#define CPUID_FEATURE_SEGLIM64   BIT43  ///< 64-bit segment limit checking
#define CPUID_FEATURE_RESVH12    BIT44  ///< Reserved
#define CPUID_FEATURE_CX16       BIT45  ///< CMPXCHG16B Instruction
#define CPUID_FEATURE_xTPR       BIT46  ///< Task Priority Update Control
#define CPUID_FEATURE_PDCM       BIT47  ///< Perfmon/Debug Capability MSR
#define CPUID_FEATURE_RESVH16    BIT48  ///< Reserved
#define CPUID_FEATURE_PCID       BIT49  ///< ASID-PCID support
#define CPUID_FEATURE_DCA        BIT50  ///< Direct Cache Access
#define CPUID_FEATURE_SSE4_1     BIT51  ///< Streaming SIMD Extensions 4.1
#define CPUID_FEATURE_SSE4_2     BIT52  ///< Streaming SIMD Extensions 4.1
#define CPUID_FEATURE_xAPIC      BIT53  ///< Extended xAPIC Support
#define CPUID_FEATURE_MOVBE      BIT54  ///< MOVBE Instruction
#define CPUID_FEATURE_POPCNT     BIT55  ///< POPCNT Instruction
#define CPUID_FEATURE_TSCTMR     BIT56  ///< TSC deadline timer
#define CPUID_FEATURE_AES        BIT57  ///< AES instructions
#define CPUID_FEATURE_XSAVE      BIT58  ///< XSAVE/XSTOR States
#define CPUID_FEATURE_OSXSAVE    BIT59  ///< OS Has Enabled XSETBV/XGETBV
#define CPUID_FEATURE_AVX1_0     BIT60  ///< AVX 1.0 instructions
#define CPUID_FEATURE_RDRAND     BIT61  ///< RDRAND instruction
#define CPUID_FEATURE_F16C       BIT62  ///< Float16 convert instructions
#define CPUID_FEATURE_VMM        BIT63  ///< VMM (Hypervisor) present

// The CPUID_EXTFEATURE_XXX values define 64-bit values
// returned in %ecx:%edx to a CPUID request with %eax of 0x80000001:

#define CPUID_EXTFEATURE_SYSCALL BIT11  ///< SYSCALL/sysret

#define CPUID_EXTFEATURE_XD      BIT20  ///< eXecute Disable
#define CPUID_EXTFEATURE_1GBPAGE BIT21  ///< 1GB pages

#define CPUID_EXTFEATURE_RDTSCP  BIT27  ///< RDTSCP

#define CPUID_EXTFEATURE_EM64T   BIT29  ///< Extended Mem 64 Technology

#define CPUID_EXTFEATURE_LAHF    BIT32  ///< LAFH/SAHF instructions


// The CPUID_EXTFEATURE_XXX values define 64-bit values
// returned in %ecx:%edx to a CPUID request with %eax of 0x80000007:

#define CPUID_EXTFEATURE_TSCI    BIT8  ///< TSC Invariant

// When the EAX register contains a value of 2, the CPUID instruction loads
// the EAX, EBX, ECX, and EDX registers with descriptors that indicate the
// processor's cache and TLB characteristics.

// CPUID_CACHE_SIZE
/// Number of 8-bit descriptor values
#define CPUID_CACHE_SIZE  16  

enum {
  CpuIdCacheNull           = 0x00,  ///< NULL
  CpuIdCacheItlb4K_32_4    = 0x01,  ///< Inst TLB: 4K pages, 32 ents, 4-way
  CpuIdCacheItlb4M_2       = 0x02,  ///< Inst TLB: 4M pages, 2 ents
  CpuIdCacheDtlb4K_64_4    = 0x03,  ///< Data TLB: 4K pages, 64 ents, 4-way
  CpuIdCacheDtlb4M_8_4     = 0x04,  ///< Data TLB: 4M pages, 8 ents, 4-way
  CpuIdCacheDtlb4M_32_4    = 0x05,  ///< Data TLB: 4M pages, 32 ents, 4-way
  CpuIdCacheL1I_8K         = 0x06,  ///< Icache: 8K
  CpuIdCacheL1I_16K        = 0x08,  ///< Icache: 16K
  CpuIdCacheL1I_32K        = 0x09,  ///< Icache: 32K, 4-way, 64 bytes
  CpuIdCacheL1D_8K         = 0x0A,  ///< Dcache: 8K
  CpuIdCacheL1D_16K        = 0x0C,  ///< Dcache: 16K
  CpuIdCacheL1D_16K_4_32   = 0x0D,  ///< Dcache: 16K, 4-way, 64 byte, ECC
  CpuIdCacheL2_256K_8_64   = 0x21,  ///< L2: 256K, 8-way, 64 bytes
  CpuIdCacheL3_512K        = 0x22,  ///< L3: 512K
  CpuIdCacheL3_1M          = 0x23,  ///< L3: 1M
  CpuIdCacheL3_2M          = 0x25,  ///< L3: 2M
  CpuIdCacheL3_4M          = 0x29,  ///< L3: 4M
  CpuIdCacheL1D_32K_8      = 0x2C,  ///< Dcache: 32K, 8-way, 64 byte
  CpuIdCacheL1I_32K_8      = 0x30,  ///< Icache: 32K, 8-way
  CpuIdCacheL2_128K_S4     = 0x39,  ///< L2: 128K, 4-way, sectored, 64B
  CpuIdCacheL2_192K_S6     = 0x3A,  ///< L2: 192K, 6-way, sectored, 64B
  CpuIdCacheL2_128K_S2     = 0x3B,  ///< L2: 128K, 2-way, sectored, 64B
  CpuIdCacheL2_256K_S4     = 0x3C,  ///< L2: 256K, 4-way, sectored, 64B
  CpuIdCacheL2_384K_S6     = 0x3D,  ///< L2: 384K, 6-way, sectored, 64B
  CpuIdCacheL2_512K_S4     = 0x3E,  ///< L2: 512K, 4-way, sectored, 64B
  CpuIdCacheNoCache        = 0x40,  ///< No 2nd level or 3rd-level cache
  CpuIdCacheL2_128K        = 0x41,  ///< L2: 128K
  CpuIdCacheL2_256K        = 0x42,  ///< L2: 256K
  CpuIdCacheL2_512K        = 0x43,  ///< L2: 512K
  CpuIdCacheL2_1M_4        = 0x44,  ///< L2: 1M, 4-way
  CpuIdCacheL2_2M_4        = 0x45,  ///< L2: 2M, 4-way
  CpuIdCacheL3_4M_4_64     = 0x46,  ///< L3:  4M,  4-way, 64 bytes
  CpuIdCacheL3_8M_8_64     = 0x47,  ///< L3:  8M,  8-way, 64 bytes*/
  CpuIdCacheL2_3M_12_64    = 0x48,  ///< L3:  3M,  8-way, 64 bytes*/
  CpuIdCacheL2_4M_16_64    = 0x49,  ///< L2:  4M, 16-way, 64 bytes
  CpuIdCacheL2_6M_12_64    = 0x4A,  ///< L2:  6M, 12-way, 64 bytes
  CpuIdCacheL2_8M_16_64    = 0x4B,  ///< L2:  8M, 16-way, 64 bytes
  CpuIdCacheL2_12M_12_64   = 0x4C,  ///< L2: 12M, 12-way, 64 bytes
  CpuIdCacheL2_16M_16_64   = 0x4D,  ///< L2: 16M, 16-way, 64 bytes
  CpuIdCacheL2_6M_24_64    = 0x4E,  ///< L2:  6M, 24-way, 64 bytes
  CpuIdCacheItlb64         = 0x50,  ///< Inst TLB: 64 entries
  CpuIdCacheItlb128        = 0x51,  ///< Inst TLB: 128 entries
  CpuIdCacheItlb256        = 0x52,  ///< Inst TLB: 256 entries
  CpuIdCacheItlb4M2M_7     = 0x55,  ///< Inst TLB: 4M/2M, 7 entries
  CpuIdCacheDtlb4M_16_4    = 0x56,  ///< Data TLB: 4M, 16 entries, 4-way
  CpuIdCacheDtlb4K_16_4    = 0x57,  ///< Data TLB: 4K, 16 entries, 4-way
  CpuIdCacheDtlb4M2M_32_4  = 0x5A,  ///< Data TLB: 4M/2M, 32 entries
  CpuIdCacheDtlb64         = 0x5B,  ///< Data TLB: 64 entries
  CpuIdCacheDtlb128        = 0x5C,  ///< Data TLB: 128 entries
  CpuIdCacheDtlb256        = 0x5D,  ///< Data TLB: 256 entries
  CpuIdCacheL1D_16K_8_64   = 0x60,  ///< Data cache: 16K, 8-way, 64 bytes
  CpuIdCacheL1D_8K_4_64    = 0x66,  ///< Data cache:  8K, 4-way, 64 bytes
  CpuIdCacheL1D_16K_4_64   = 0x67,  ///< Data cache: 16K, 4-way, 64 bytes
  CpuIdCacheL1D_32K_4_64   = 0x68,  ///< Data cache: 32K, 4-way, 64 bytes
  CpuIdCacheTRACE_12K_8    = 0x70,  ///< Trace cache 12K-uop, 8-way
  CpuIdCacheTRACE_16K_8    = 0x71,  ///< Trace cache 16K-uop, 8-way
  CpuIdCacheTRACE_32K_8    = 0x72,  ///< Trace cache 32K-uop, 8-way
  CpuIdCacheTRACE_64K_8    = 0x73,  ///< Trace cache 64K-uop, 8-way
  CpuIdCacheL2_1M_4_64     = 0x78,  ///< L2:   1M, 4-way, 64 bytes
  CpuIdCacheL2_128K_8_64_2 = 0x79,  ///< L2: 128K, 8-way, 64b, 2 lines/sec
  CpuIdCacheL2_256K_8_64_2 = 0x7A,  ///< L2: 256K, 8-way, 64b, 2 lines/sec
  CpuIdCacheL2_512K_8_64_2 = 0x7B,  ///< L2: 512K, 8-way, 64b, 2 lines/sec
  CpuIdCacheL2_1M_8_64_2   = 0x7C,  ///< L2:   1M, 8-way, 64b, 2 lines/sec
  CpuIdCacheL2_2M_8_64     = 0x7D,  ///< L2:   2M, 8-way, 64 bytes
  CpuIdCacheL2_512K_2_64   = 0x7F,  ///< L2: 512K, 2-way, 64 bytes
  CpuIdCacheL2_256K_8_32   = 0x82,  ///< L2: 256K, 8-way, 32 bytes
  CpuIdCacheL2_512K_8_32   = 0x83,  ///< L2: 512K, 8-way, 32 bytes
  CpuIdCacheL2_1M_8_32     = 0x84,  ///< L2:   1M, 8-way, 32 bytes
  CpuIdCacheL2_2M_8_32     = 0x85,  ///< L2:   2M, 8-way, 32 bytes
  CpuIdCacheL2_512K_4_64   = 0x86,  ///< L2: 512K, 4-way, 64 bytes
  CpuIdCacheL2_1M_8_64     = 0x87,  ///< L2:   1M, 8-way, 64 bytes
  CpuIdCacheItlb4K_128_4   = 0xB0,  ///< ITLB: 4KB, 128 entries, 4-way
  CpuIdCacheItlb4M_4_4     = 0xB1,  ///< ITLB: 4MB, 4 entries, 4-way, or
  CpuIdCacheItlb2M_8_4     = 0xB1,  ///< ITLB: 2MB, 8 entries, 4-way, or
  CpuIdCacheItlb4M_8       = 0xB1,  ///< ITLB: 4MB, 8 entries
  CpuIdCacheItlb4K_64_4    = 0xB2,  ///< ITLB: 4KB, 64 entries, 4-way
  CpuIdCacheDtlb4K_128_4   = 0xB3,  ///< DTLB: 4KB, 128 entries, 4-way
  CpuIdCacheDtlb4K_256_4   = 0xB4,  ///< DTLB: 4KB, 256 entries, 4-way
  CpuIdCache2TLB_4K_512_4  = 0xCA,  ///< 2nd-level TLB: 4KB, 512, 4-way
  CpuIdCacheL3_512K_4_64   = 0xD0,  ///< L3: 512KB, 4-way, 64 bytes
  CpuIdCacheL3_1M_4_64     = 0xD1,  ///< L3:    1M, 4-way, 64 bytes
  CpuIdCacheL3_2M_4_64     = 0xD2,  ///< L3:    2M, 4-way, 64 bytes
  CpuIdCacheL3_1M_8_64     = 0xD6,  ///< L3:    1M, 8-way, 64 bytes
  CpuIdCacheL3_2M_8_64     = 0xD7,  ///< L3:    2M, 8-way, 64 bytes
  CpuIdCacheL3_4M_8_64     = 0xD8,  ///< L3:    4M, 8-way, 64 bytes
  CpuIdCacheL3_1M5_12_64   = 0xDC,  ///< L3:  1.5M, 12-way, 64 bytes
  CpuIdCacheL3_3M_12_64    = 0xDD,  ///< L3:    3M, 12-way, 64 bytes
  CpuIdCacheL3_6M_12_64    = 0xDE,  ///< L3:    6M, 12-way, 64 bytes
  CpuIdCacheL3_2M_16_64    = 0xE2,  ///< L3:    2M, 16-way, 64 bytes
  CpuIdCacheL3_4M_16_64    = 0xE3,  ///< L3:    4M, 16-way, 64 bytes
  CpuIdCacheL3_8M_16_64    = 0xE4,  ///< L3:    8M, 16-way, 64 bytes
  CpuIdCachePrefetch64     = 0xF0,  ///< 64-Byte Prefetching
  CpuIdCachePrefetch128    = 0xF1,  ///< 128-Byte Prefetching
};

#define CPUID_VENDOR_INTEL  0x756E6547
#define CPUID_VENDOR_AMD    0x68747541

#endif // CPUID_H
