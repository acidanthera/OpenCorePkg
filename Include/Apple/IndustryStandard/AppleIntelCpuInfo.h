/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_INTEL_CPU_INFO_H
#define APPLE_INTEL_CPU_INFO_H

///
/// i386_cpu_info_t related definitions.
/// Keep in sync with XNU osfmk/i386/cpuid.h.
/// Last sync time: 4903.221.2.
///

/*
 *  CPU families (sysctl hw.cpufamily)
 *
 * These are meant to identify the CPU's marketing name - an
 * application can map these to (possibly) localized strings.
 * NB: the encodings of the CPU families are intentionally arbitrary.
 * There is no ordering, and you should never try to deduce whether
 * or not some feature is available based on the family.
 * Use feature flags (eg, hw.optional.altivec) to test for optional
 * functionality.
 */
#define CPUFAMILY_UNKNOWN           0
#define CPUFAMILY_INTEL_6_13        0xaa33392b
#define CPUFAMILY_INTEL_PENRYN      0x78ea4fbc
#define CPUFAMILY_INTEL_NEHALEM     0x6b5a4cd2
#define CPUFAMILY_INTEL_WESTMERE    0x573b5eec
#define CPUFAMILY_INTEL_SANDYBRIDGE 0x5490b78c
#define CPUFAMILY_INTEL_IVYBRIDGE   0x1f65e835
#define CPUFAMILY_INTEL_HASWELL     0x10b282dc
#define CPUFAMILY_INTEL_BROADWELL   0x582ed09c
#define CPUFAMILY_INTEL_SKYLAKE     0x37fc219f
#define CPUFAMILY_INTEL_KABYLAKE    0x0f817246

/* CPU type, integer_t */
typedef INT32 cpu_type_t;
typedef INT32 cpu_subtype_t;

/*
 * Cache ID descriptor structure, used to parse CPUID leaf 2.
 * Note: not used in kernel.
 */
typedef enum { Lnone, L1I, L1D, L2U, L3U, LCACHE_MAX } cache_type_t; 
typedef struct {
  UINT8         value;          /* Descriptor value */
  cache_type_t  type;           /* Cache type */
  UINT32        size;           /* Cache size */
  UINT32        linesize;       /* Cache line size */
  CONST CHAR8   *description;   /* Cache description */
} cpuid_cache_desc_t;  

/* Monitor/mwait Leaf: */
typedef struct {
  UINT32  linesize_min;
  UINT32  linesize_max;
  UINT32  extensions;
  UINT32  sub_Cstates;
} cpuid_mwait_leaf_t;

/* Thermal and Power Management Leaf: */
typedef struct {
  BOOLEAN sensor;
  BOOLEAN dynamic_acceleration;
  BOOLEAN invariant_APIC_timer;
  BOOLEAN core_power_limits;
  BOOLEAN fine_grain_clock_mod;
  BOOLEAN package_thermal_intr;
  UINT32  thresholds;
  BOOLEAN ACNT_MCNT;
  BOOLEAN hardware_feedback;
  BOOLEAN energy_policy;
} cpuid_thermal_leaf_t;


/* XSAVE Feature Leaf: */
typedef struct {
  UINT32  extended_state[4];  /* eax .. edx */
} cpuid_xsave_leaf_t;


/* Architectural Performance Monitoring Leaf: */
typedef struct {
  UINT8   version;
  UINT8   number;
  UINT8   width;
  UINT8   events_number;
  UINT32  events;
  UINT8   fixed_number;
  UINT8   fixed_width;
} cpuid_arch_perf_leaf_t;

/* The TSC to Core Crystal (RefCLK) Clock Information leaf */
typedef struct {
  UINT32  numerator;
  UINT32  denominator;
} cpuid_tsc_leaf_t;

/* Physical CPU info - this is exported out of the kernel (kexts), so be wary of changes */
typedef struct {
  CHAR8        cpuid_vendor[16];
  CHAR8        cpuid_brand_string[48];
  CONST CHAR8  *cpuid_model_string;

  cpu_type_t   cpuid_type; /* this is *not* a cpu_type_t in our <mach/machine.h> */
  UINT8        cpuid_family;
  UINT8        cpuid_model;
  UINT8        cpuid_extmodel;
  UINT8        cpuid_extfamily;
  UINT8        cpuid_stepping;
  UINT64       cpuid_features;
  UINT64       cpuid_extfeatures;
  UINT32       cpuid_signature;
  UINT8        cpuid_brand; 
  UINT8        cpuid_processor_flag;
  
  UINT32       cache_size[LCACHE_MAX];
  UINT32       cache_linesize;

  UINT8        cache_info[64];    /* list of cache descriptors */

  UINT32       cpuid_cores_per_package;
  UINT32       cpuid_logical_per_package;
  UINT32       cache_sharing[LCACHE_MAX];
  UINT32       cache_partitions[LCACHE_MAX];

  cpu_type_t              cpuid_cpu_type;     /* <mach/machine.h> */
  cpu_subtype_t           cpuid_cpu_subtype;    /* <mach/machine.h> */  

  /* Per-vendor info */
  cpuid_mwait_leaf_t      cpuid_mwait_leaf; 
#define cpuid_mwait_linesize_max  cpuid_mwait_leaf.linesize_max
#define cpuid_mwait_linesize_min  cpuid_mwait_leaf.linesize_min
#define cpuid_mwait_extensions    cpuid_mwait_leaf.extensions
#define cpuid_mwait_sub_Cstates   cpuid_mwait_leaf.sub_Cstates
  cpuid_thermal_leaf_t    cpuid_thermal_leaf;
  cpuid_arch_perf_leaf_t  cpuid_arch_perf_leaf;
  UINT32                  unused[4];      /* cpuid_xsave_leaf */

  /* Cache details: */
  UINT32  cpuid_cache_linesize;
  UINT32  cpuid_cache_L2_associativity;
  UINT32  cpuid_cache_size;

  /* Virtual and physical address aize: */
  UINT32  cpuid_address_bits_physical;
  UINT32  cpuid_address_bits_virtual;

  UINT32  cpuid_microcode_version;

  /* Numbers of tlbs per processor [i|d, small|large, level0|level1] */
  UINT32  cpuid_tlb[2][2][2];
#define TLB_INST  0
#define TLB_DATA  1
#define TLB_SMALL 0
#define TLB_LARGE 1
  UINT32  cpuid_stlb;

  UINT32  core_count;
  UINT32  thread_count;

  /* Max leaf ids available from CPUID */
  UINT32  cpuid_max_basic;
  UINT32  cpuid_max_ext;

  /* Family-specific info links */
  UINT32                  cpuid_cpufamily;
  cpuid_mwait_leaf_t      *cpuid_mwait_leafp; 
  cpuid_thermal_leaf_t    *cpuid_thermal_leafp;
  cpuid_arch_perf_leaf_t  *cpuid_arch_perf_leafp;
  cpuid_xsave_leaf_t      *cpuid_xsave_leafp;
  UINT64                  cpuid_leaf7_features;
  cpuid_tsc_leaf_t        cpuid_tsc_leaf;
  cpuid_xsave_leaf_t      cpuid_xsave_leaf[2];
} i386_cpu_info_t;

typedef i386_cpu_info_t APPLE_INTEL_CPU_INFO;

#endif // APPLE_INTEL_CPU_INFO_H
