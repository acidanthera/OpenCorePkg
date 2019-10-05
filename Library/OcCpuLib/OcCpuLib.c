/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <IndustryStandard/CpuId.h>
#include <IndustryStandard/GenericIch.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/AppleSmBios.h>

#include <Protocol/PciIo.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/OcCpuLib.h>
#include <Library/PciLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcTimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <ProcessorInfo.h>
#include <Register/Microcode.h>
#include <Register/Msr.h>
#include <Register/Intel/Msr/SandyBridgeMsr.h>
#include <Register/Intel/Msr/NehalemMsr.h>

//
// Tolerance within which we consider two frequency values to be roughly
// equivalent.
//
#define OC_CPU_FREQUENCY_TOLERANCE 50000000ULL // 50 Mhz

STATIC
UINT8
DetectAppleMajorType (
  IN  CONST CHAR8  *BrandString
  )
{
  CONST CHAR8  *BrandInfix;

  BrandInfix = AsciiStrStr (BrandString, "Core");
  if (BrandInfix != NULL) {
    while ((*BrandInfix != ' ') && (*BrandInfix != '\0')) {
      ++BrandInfix;
    }

    while (*BrandInfix == ' ') {
      ++BrandInfix;
    }

    if (AsciiStrnCmp (BrandInfix, "i7", L_STR_LEN ("i7")) == 0) {
      return AppleProcessorMajorI7;
    }
    if (AsciiStrnCmp (BrandInfix, "i5", L_STR_LEN ("i5")) == 0) {
      return AppleProcessorMajorI5;
    }
    if (AsciiStrnCmp (BrandInfix, "i3", L_STR_LEN ("i3")) == 0) {
      return AppleProcessorMajorI3;
    }
    if (AsciiStrnCmp (BrandInfix, "i9", L_STR_LEN ("i9")) == 0) {
      return AppleProcessorMajorI9;
    }
    if (AsciiStrnCmp (BrandInfix, "m3", L_STR_LEN ("m3")) == 0) {
      return AppleProcessorMajorM3;
    }
    if (AsciiStrnCmp (BrandInfix, "m5", L_STR_LEN ("m5")) == 0) {
      return AppleProcessorMajorM5;
    }
    if (AsciiStrnCmp (BrandInfix, "m7", L_STR_LEN ("m7")) == 0) {
      return AppleProcessorMajorM7;
    }
    if (AsciiStrnCmp (BrandInfix, "M", L_STR_LEN ("M")) == 0) {
      return AppleProcessorMajorM;
    }
    if (AsciiStrnCmp (BrandInfix, "Duo", L_STR_LEN ("Duo")) == 0) {
      return AppleProcessorMajorCore2;
    }
    if (AsciiStrnCmp (BrandInfix, "Quad", L_STR_LEN ("Quad")) == 0) {
      return AppleProcessorMajorXeonPenryn;
    }
    return AppleProcessorMajorCore;
  }

  BrandInfix = AsciiStrStr (BrandString, "Xeon");
  if (BrandInfix != NULL) {
    while ((*BrandInfix != ' ') && (*BrandInfix != '\0')) {
      ++BrandInfix;
    }

    while (*BrandInfix == ' ') {
      ++BrandInfix;
    }

    //
    // Support Xeon Scalable chips: Xeon(R) Gold 6136 CPU
    //
    if (AsciiStrnCmp (BrandInfix, "Bronze", L_STR_LEN ("Bronze")) == 0 ||
        AsciiStrnCmp (BrandInfix, "Silver", L_STR_LEN ("Silver")) == 0 ||
        AsciiStrnCmp (BrandInfix, "Gold", L_STR_LEN ("Gold")) == 0 ||
        AsciiStrnCmp (BrandInfix, "Platinum", L_STR_LEN ("Platinum")) == 0) {
      // Treat Xeon Scalable chips as their closest relatives, Xeon W
      return AppleProcessorMajorXeonW;
    }

    //
    // Support both variants: Xeon(R) E5-1234 and Xeon(R) CPU E5-1234
    //
    if (AsciiStrnCmp (BrandInfix, "CPU", L_STR_LEN ("CPU")) == 0) {
      BrandInfix += L_STR_LEN ("CPU");
      while (*BrandInfix == ' ') {
        ++BrandInfix;
      }
    }

    if (AsciiStrnCmp (BrandInfix, "E5", L_STR_LEN ("E5")) == 0) {
      return AppleProcessorMajorXeonE5;
    }
    if (AsciiStrnCmp (BrandInfix, "W", L_STR_LEN ("W")) == 0) {
      return AppleProcessorMajorXeonW;
    }
    return AppleProcessorMajorXeonNehalem;
  }

  return AppleProcessorMajorUnknown;
}

STATIC
UINT16
DetectAppleProcessorType (
  IN UINT8  Model,
  IN UINT8  Stepping,
  IN UINT8  AppleMajorType
  )
{
  switch (Model) {
    //
    // Yonah: https://en.wikipedia.org/wiki/Yonah_(microprocessor)#Models_and_brand_names
    //
    // Used by Apple:
    //   Core Duo, Core Solo
    //
    // NOT used by Apple:
    //   Pentium, Celeron
    //
    // All 0x0201.
    //
    case CPU_MODEL_DOTHAN: // 0x0D
    case CPU_MODEL_YONAH:  // 0x0E
      // IM41  (T2400/T2500), MM11 (Solo T1200 / Duo T2300/T2400),
      // MBP11 (L2400/T2400/T2500/T2600), MBP12 (T2600),
      // MB11  (T2400/T2500)
      return AppleProcessorTypeCoreSolo; // 0x0201

    //
    // Merom:  https://en.wikipedia.org/wiki/Merom_(microprocessor)#Variants
    // Penryn: https://en.wikipedia.org/wiki/Penryn_(microprocessor)#Variants
    //
    // Used by Apple:
    //   Core 2 Extreme, Core 2 Duo (Merom),
    //   Core 2 Duo,                (Penryn),
    //   certain Clovertown (Merom) / Harpertown (Penryn) based models
    //
    // Not used by Apple:
    //   Merom:  Core 2 Solo, Pentium, Celeron M, Celeron
    //   Penryn: Core 2 Extreme, Core 2 Quad, Core 2 Solo, Pentium, Celeron
    //
    case CPU_MODEL_MEROM:  // 0x0F
    case CPU_MODEL_PENRYN: // 0x17
      if (AppleMajorType == AppleProcessorMajorCore2) {
        // TODO: add check for models above. (by changing the following "if (0)")
        if (0) {
          // ONLY MBA31 (SU9400/SU9600) and MBA32 (SL9400/SL9600)
          return AppleProcessorTypeCore2DuoType2; // 0x0302
        }
        // IM51 (T7200), IM61 (T7400), IM71 (T7300), IM81 (E8435), IM101 (E7600),
        // MM21 (unknown), MM31 (P7350),
        // MBP21 (T7600), MBP22 (unknown), MBP31 (T7700), MBP41 (T8300), MBP71 (P8600),
        // MBP51 (P8600), MBP52 (T9600), MBP53 (P8800), MBP54 (P8700), MBP55 (P7550),
        // MBA11 (P7500), MBA21 (SL9600),
        // MB21 (unknown), MB31 (T7500), MB41 (T8300), MB51 (P8600), MB52 (P7450), MB61 (P7550), MB71 (P8600)
        return AppleProcessorTypeCore2DuoType1; // 0x0301
      }
      if (AppleMajorType == AppleProcessorMajorXeonPenryn) {
        // MP21 (2x X5365), MP31 (2x E5462) - 0x0402
        // FIXME: check when 0x0401 will be used.
        return AppleProcessorTypeXeonPenrynType2; // 0x0402
      }
      // here stands for models not used by Apple (Merom/Penryn), putting 0x0301 as lowest
      return AppleProcessorTypeCore2DuoType1;   // 0x0301

    //
    // Nehalem:  https://en.wikipedia.org/wiki/Nehalem_(microarchitecture)#Server_and_desktop_processors
    // Westmere: https://en.wikipedia.org/wiki/Westmere_(microarchitecture)#Server_/_Desktop_processors
    //
    // Used by Apple:
    //   Gainestown (Xeon), Bloomfield (Xeon), Lynnfield (i5/i7)                   [Nehalem]
    //   Gulftown (aka Westmere-EP, Xeon), Clarkdale (i3/i5), Arrandale (i5/i7)    [Westmere]
    //
    // Not used by Apple:
    //   Beckton (Xeon), Jasper Forest (Xeon), Clarksfield (i7), Pentium, Celeron [Nehalem]
    //   Westmere-EX (Xeon E7), Pentium, Celeron                                  [Westmere]
    //
    case CPU_MODEL_NEHALEM:     // 0x1A
    case CPU_MODEL_NEHALEM_EX:  // 0x2E, not used by Apple
    case CPU_MODEL_FIELDS:      // 0x1E, Lynnfield, Clarksfield (part of Nehalem)
    case CPU_MODEL_WESTMERE:    // 0x2C
    case CPU_MODEL_WESTMERE_EX: // 0x2F, not used by Apple
    case CPU_MODEL_DALES_32NM:  // 0x25, Clarkdale, Arrandale (part of Westmere)
      if (AppleMajorType == AppleProcessorMajorXeonNehalem) {
        // MP41 & Xserve31 (2x E5520, CPU_MODEL_NEHALEM), MP51 (2x X5670, CPU_MODEL_WESTMERE)
        return AppleProcessorTypeXeon;        // 0x0501
      }
      if (AppleMajorType == AppleProcessorMajorI3) {
        // IM112 (i3-540, 0x0901, CPU_MODEL_DALES_32NM)
        return AppleProcessorTypeCorei3Type1; // 0x0901
      }
      if (AppleMajorType == AppleProcessorMajorI5) {
        // FIXME: no idea what it is on IM112 (i5-680)
        // MBP61, i5-640M, 0x0602, CPU_MODEL_DALES_32NM
        return AppleProcessorTypeCorei5Type2; // 0x0602
      }
      if (AppleMajorType == AppleProcessorMajorI7) {
        // FIXME: used by Apple, no idea what to use, assuming 0x0702 for now (based off 0x0602 on i5)
        return AppleProcessorTypeCorei7Type2; // 0x0702
      }
      // here stands for Pentium and Celeron (Nehalem/Westmere), not used by Apple at all.
      // putting 0x0901 (i3) as lowest
      return AppleProcessorTypeCorei3Type1; // 0x0901

    //
    // Sandy Bridge:   https://en.wikipedia.org/wiki/Sandy_Bridge#List_of_Sandy_Bridge_processors
    // Sandy Bridge-E: https://en.wikipedia.org/wiki/Sandy_Bridge-E#Overview
    //
    // Used by Apple:
    //   Core i5/i7 / i3 (see NOTE below)
    //
    // NOTE: There seems to be one more i3-2100 used on IM121 (EDU),
    //       assuming it exists for now.
    //
    // Not used by Apple:
    //   Xeon v1 (E5/E3),
    //   SNB-E based Core i7 (and Extreme): 3970X, 3960X, 3930K, 3820,
    //   Pentium, Celeron
    //
    case CPU_MODEL_SANDYBRIDGE: // 0x2A
    case CPU_MODEL_JAKETOWN:    // 0x2D, SNB-E, not used by Apple
      if (AppleMajorType == AppleProcessorMajorI3) {
        // FIXME: used by Apple on iMac12,1 (EDU, i3-2100), not confirmed yet
        return AppleProcessorTypeCorei3Type3;   // 0x0903
      }
      if (AppleMajorType == AppleProcessorMajorI5) {
        // NOTE: two values are used here. (0x0602 and 0x0603)
        // TODO: how to classify them. (by changing "if (0)")
        if (0) {
          // MM51 (i5-2415M), MM52 (i5-2520M), MBA41 (i5-2467M), MBA42 (i5-2557M)
          return AppleProcessorTypeCorei5Type2; // 0x0602
        }
        // IM121 (i5-2400S), MBP81 (i5-2415M)
        return AppleProcessorTypeCorei5Type3; // 0x0603
      }
      if (AppleMajorType == AppleProcessorMajorI7) {
        // IM122 (i7-2600), MBP82 (i7-2675QM), MBP83 (i7-2820QM)
        //
        // FIXME: will those i7 not used by Apple (see above), be identified as AppleProcessorMajorI7?
        return AppleProcessorTypeCorei7Type3;   // 0x0703
      }
      if (AppleMajorType == AppleProcessorMajorXeonE5) { // see TODO below
        // for Sandy Xeon E5, not used by Apple
        // FIXME: is AppleProcessorMajorXeonE5, which seems to be for IVY-E only, compatible with SNB-E too?
        // TODO: write some decent code to check SNB-E based Xeon E5.
        return AppleProcessorTypeXeonE5;        // 0x0A01
      }
      if (AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Sandy Xeon E3
        // NOTE: Xeon E3 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for SNB based Xeon E3
        return AppleProcessorTypeXeon;          // 0x0501
      }
      // here stands for Pentium and Celeron (Sandy), not used by Apple at all.
      // putting 0x0903 (i3) as lowest
      return AppleProcessorTypeCorei3Type3;   // 0x0903

    //
    // Ivy Bridge:   https://en.wikipedia.org/wiki/Ivy_Bridge_(microarchitecture)#List_of_Ivy_Bridge_processors
    // Ivy Bridge-E: https://en.wikipedia.org/wiki/Ivy_Bridge_(microarchitecture)#Models_and_steppings_2
    //
    // Used by Apple:
    //   Core i5/i7 / i3 (see NOTE below),
    //   Xeon E5 v2
    //
    // NOTE: There seems to be an iMac13,1 (EDU version), or rather iMac13,3, with CPU i3-3225,
    //       assuming it exists for now.
    //
    // Not used by Apple:
    //   Xeon v2 (E7/E3),
    //   IVY-E based Core i7 (and Extreme): 4960X, 4930K, 4820K,
    //   Pentium, Celeron
    //
    case CPU_MODEL_IVYBRIDGE:    // 0x3A
    case CPU_MODEL_IVYBRIDGE_EP: // 0x3E
      if (AppleMajorType == AppleProcessorMajorXeonE5) {
        // MP61 (E5-1620 v2)
        return AppleProcessorTypeXeonE5;      // 0x0A01
      }
      if (AppleMajorType == AppleProcessorMajorI5) {
        // IM131 (i5-3470S), IM132  (i5-3470S),
        // MBP92 (i5-3210M), MBP102 (i5-3210M)
        // MBA51 (i6-3317U), MBA52  (i5-3427U)
        return AppleProcessorTypeCorei5Type4; // 0x0604
      }
      if (AppleMajorType == AppleProcessorMajorI7) {
        // MM62  (i7-3615QM),
        // MBP91 (i7-3615QM), MBP101 (i7-3820QM)
        //
        // FIXME: will those i7 not used by Apple (see above), be identified as AppleProcessorMajorI7?
        return AppleProcessorTypeCorei7Type4; // 0x0704
      }
      if (AppleMajorType == AppleProcessorMajorI3) {
        // FIXME: used by Apple (if iMac13,3 were existent, i3-3225), not confirmed yet
        // assuming it exists for now
        return AppleProcessorTypeCorei3Type4; // 0x0904
      }
      if (AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Ivy/Ivy-E E3/E7, not used by Apple
        // NOTE: Xeon E3/E7 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for IVY based Xeon E3/E7
        return AppleProcessorTypeXeon;        // 0x0501
      }
      // here stands for Pentium and Celeron (Ivy), not used by Apple at all.
      // putting 0x0904 (i3) as lowest.
      return AppleProcessorTypeCorei3Type4; // 0x0904

    //
    // Haswell:   https://en.wikipedia.org/wiki/Haswell_(microarchitecture)#List_of_Haswell_processors
    // Haswell-E: basically the same page.
    //
    // Used by Apple:
    //   Core i5/i7
    //
    // Not used by Apple:
    //   Xeon v3 (E7/E5/E3),
    //   Core i3,
    //   Haswell-E based Core i7 Extreme: 5960X, 5930K, 5820K,
    //   Pentium, Celeron
    //
    case CPU_MODEL_HASWELL:     // 0x3C
    case CPU_MODEL_HASWELL_EP:  // 0x3F
    case CPU_MODEL_HASWELL_ULT: // 0x45
      if (AppleMajorType == AppleProcessorMajorI5) {
        // IM141 (i5-4570R), IM142 (i5-4670), IM151 (i5-4690),
        // MM71  (i5-4260U),
        // MBA62 (i5-4250U)
        return AppleProcessorTypeCorei5Type5; // 0x0605
      }
      if (AppleMajorType == AppleProcessorMajorI7) {
        // MBP112 (i7-4770HQ), MBP113 (i7-4850HQ)
        //
        // FIXME: will those i7 not used by Apple (see above), be identified as AppleProcessorMajorI7?
        return AppleProcessorTypeCorei7Type5; // 0x0705
      }
      if (AppleMajorType ==  AppleProcessorMajorI3) {
        // for i3, not used by Apple, just for showing i3 in "About This Mac".
        return AppleProcessorTypeCorei3Type5; // 0x0905
      }
      if (AppleMajorType == AppleProcessorMajorXeonE5) { // see TODO below
        // for Haswell-E Xeon E5, not used by Apple
        // FIXME: is AppleProcessorMajorXeonE5, which seems to be for IVY-E only, compatible with Haswell-E too?
        // TODO: write some decent code to check Haswell-E based Xeon E5.
        return AppleProcessorTypeXeonE5;      // 0x0A01
      }
      if (AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Haswell/Haswell-E E3/E7, not used by Apple
        // NOTE: Xeon E3/E7 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for Haswell/Haswell-E based Xeon E3/E7
        return AppleProcessorTypeXeon;        // 0x0501
      }
      // here stands for Pentium and Celeron (Haswell), not used by Apple at all.
      // putting 0x0905 (i3) as lowest.
      return AppleProcessorTypeCorei3Type5; // 0x0905

    //
    // Broadwell:   https://en.wikipedia.org/wiki/Broadwell_(microarchitecture)#List_of_Broadwell_processors
    // Broadwell-E: https://en.wikipedia.org/wiki/Broadwell_(microarchitecture)#"Broadwell-E"_HEDT_(14_nm)
    //
    // NOTE: support table for BDW-E is missing in XNU, thus a CPUID patch might be needed. (See Clover FakeCPUID)
    //
    // Used by Apple:
    //   Core i5/i7, Core M
    //
    // Not used by Apple:
    //   Broadwell-E: i7 6950X/6900K/6850K/6800K,
    //   Xeon v4 (E5/E3),
    //   Core i3,
    //   Pentium, Celeron
    //
    case CPU_MODEL_BROADWELL:     // 0x3D
    case CPU_MODEL_CRYSTALWELL:   // 0x46
    case CPU_MODEL_BRYSTALWELL:   // 0x47
      if (AppleMajorType == AppleProcessorMajorM) {
        // MB81 (M 5Y51)
        return AppleProcessorTypeCoreMType6;   // 0x0B06
      }
      if (AppleMajorType == AppleProcessorMajorI5) {
        // IM161  (i5-5250U), IM162 (i5-5675R),
        // MBP121 (i5-5257U),
        // MBA71  (i5-5250U), MBA72 (unknown)
        return AppleProcessorTypeCorei5Type6; // 0x0606
      }
      if (AppleMajorType == AppleProcessorMajorI7) {
        // FIXME: 0x0706 is just an ideal value for i7, waiting for confirmation
        // FIXME: will those i7 not used by Apple (see above), be identified as AppleProcessorMajorI7?
        return AppleProcessorTypeCorei7Type6; // 0x0706
      }
      if (AppleMajorType == AppleProcessorMajorI3) {
        // for i3, not used by Apple, just for showing i3 in "About This Mac".
        // FIXME: 0x0906 is just an ideal value for i3, waiting for confirmation
        return AppleProcessorTypeCorei3Type6; // 0x0906
      }
      if (AppleMajorType == AppleProcessorMajorXeonE5) { // see TODO below
        // for Broadwell-E Xeon E5, not used by Apple
        // FIXME: is AppleProcessorMajorXeonE5, which seems to be for IVY-E only, compatible with Broadwell-E too?
        // TODO: write some decent code to check Broadwell-E based Xeon E5.
        return AppleProcessorTypeXeonE5;      // 0x0A01
      }
      if (AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Broadwell E3, not used by Apple
        // NOTE: Xeon E3 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for Broadwell based Xeon E3
        return AppleProcessorTypeXeon;        // 0x0501
      }
      // here stands for Pentium and Celeron (Broadwell), not used by Apple at all.
      // putting 0x0906 (i3) as lowest.
      return AppleProcessorTypeCorei3Type5; // 0x0906

    //
    // Skylake: https://en.wikipedia.org/wiki/Skylake_(microarchitecture)#List_of_Skylake_processor_models
    //
    // Used by Apple:
    //   Xeon W, Core m3, m5, m7, i5, i7
    //
    // Not used by Apple:
    //   Core i3,
    //   all high-end models (Core i9, i7 Extreme): see https://en.wikipedia.org/wiki/Skylake_(microarchitecture)#High-end_desktop_processors
    //   Xeon E3 v5, Xeon Scalable
    //   Pentium, Celeron
    //
    case CPU_MODEL_SKYLAKE:     // 0x4E
    case CPU_MODEL_SKYLAKE_DT:  // 0x5E
    case CPU_MODEL_SKYLAKE_W:   // 0x55, also SKL-X and SKL-SP
      if (AppleMajorType == AppleProcessorMajorXeonW) {
        // IMP11 (Xeon W 2140B)
        return AppleProcessorTypeXeonW;       // 0x0F01
      }
      if (AppleMajorType == AppleProcessorMajorM3) {
        // FIXME: we dont have any m3 (Skylake) dump!
        // using an ideal value (0x0C07), which is used on MB101 (m3-7Y32)
        return AppleProcessorTypeCoreM3Type7; // 0x0C07
      }
      if (AppleMajorType == AppleProcessorMajorM5) {
        // MB91 (m5 6Y54)
        return AppleProcessorTypeCoreM5Type7; // 0x0D07
      }
      if (AppleMajorType == AppleProcessorMajorM7) {
        // FIXME: we dont have any m7 (Skylake) dump!
        // using an ideal value (0x0E07)
        return AppleProcessorTypeCoreM7Type7; // 0x0E07
      }
      if (AppleMajorType == AppleProcessorMajorI5) {
        return AppleProcessorTypeCorei5Type5; // 0x0605
      }
      if (AppleMajorType == AppleProcessorMajorI7) {
        // FIXME: used by Apple, but not sure what to use...
        // 0x0707 is used on MBP133 (i7-6700HQ),
        // 0x0705 is not confirmed, just an ideal one comparing to 0x0605 (AppleProcessorTypeCorei5Type5)
        // using 0x0705 for now
        return AppleProcessorTypeCorei7Type5; // 0x0705
      }
      if (AppleMajorType == AppleProcessorMajorI3) {
        // for i3, not used by Apple, just for showing i3 in "About This Mac".
        return AppleProcessorTypeCorei3Type5; // 0x0905
      }
      if (AppleMajorType == AppleProcessorMajorI9) {
        // for i9 (SKL-X), not used by Apple, just for showing i9 in "About This Mac".
        // FIXME: i9 was not introdced in this era but later (MBP151, Coffee Lake),
        //        will AppleProcessorMajorI9 work here?
        // NOTE: using a mostly invalid value 0x1005 for now...
        return AppleProcessorTypeCorei9Type5; // 0x1005
      }
      if (AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Skylake E3 (there's no E5/E7 on Skylake), not used by Apple
        // NOTE: Xeon E3 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for Skylake based Xeon E3
        return AppleProcessorTypeXeon;        // 0x0501
      }
      // here stands for Pentium and Celeron (Skylake), not used by Apple at all.
      // putting 0x0905 (i3) as lowest.
      return AppleProcessorTypeCorei3Type5; // 0x0905

    //
    // Kaby Lake:   https://en.wikipedia.org/wiki/Kaby_Lake#List_of_7th_generation_Kaby_Lake_processors
    // Coffee Lake: https://en.wikipedia.org/wiki/Coffee_Lake#List_of_8th_generation_Coffee_Lake_processors
    //
    // Used by Apple:
    //   Core m3    [Kaby],
    //   Core i5/i7 [Kaby/Coffee],
    //   Core i9    [Coffee],
    //
    // Not used by Apple:
    //   Core i3    [Kaby/Coffee],
    //   Xeon E3 v6 [Kaby],
    //   Xeon E     [Coffee],
    //   Pentium, Celeron
    //
    case CPU_MODEL_KABYLAKE:       // 0x8E
    case CPU_MODEL_COFFEELAKE:     // 0x9E
      if (AppleMajorType == AppleProcessorMajorM3) {
        // MB101 (m3 7Y32)
        return AppleProcessorTypeCoreM3Type7; // 0x0C07
      }
      if (AppleMajorType == AppleProcessorMajorI5) {
        // Kaby has 0x9 stepping, and Coffee use 0xA / 0xB stepping.
        if (Stepping == 9) {
          // IM181 (i5-7360U), IM182  (i5-7400), IM183 (i5-7600), IM191 (i5-8600) [NOTE 1]
          // MBP141 (i5-7360U), MBP142 (i5-7267U)
          //
          // NOTE 1: IM191 is Coffee and thus 0x0609 will be used, TODO.
          return AppleProcessorTypeCorei5Type5; // 0x0605
        }
        // MM81 (i5-8500B)
        // MBP152 (i5-8259U)
        return AppleProcessorTypeCorei5Type9; // 0x0609
      }
      if (AppleMajorType == AppleProcessorMajorI7) {
        // Kaby has 0x9 stepping, and Coffee use 0xA / 0xB stepping.
        if (Stepping == 9) {
          // FIXME: used by Apple, but not sure what to use...
          // 0x0709 is used on MBP151 (i7-8850H),
          // 0x0705 is not confirmed, just an ideal one comparing to 0x0605 (AppleProcessorTypeCorei5Type5)
          // using 0x0705 for now
          return AppleProcessorTypeCorei7Type5; // 0x0705
        }
        // MM81 (i7-8700B)
        return AppleProcessorTypeCorei7Type9; // 0x0709
      }
      if (AppleMajorType == AppleProcessorMajorI9) {
        // FIXME: find a dump from MBP151 with i9-8950HK,
        // for now using an ideal value (0x1009), comparing to 0x0709 (used on MBP151, i7-8850H and MM81, i7-8700B)
        return AppleProcessorTypeCorei9Type9; // 0x1009
      }
      if (AppleMajorType == AppleProcessorMajorI3) {
        // FIXME: find a dump from MM71 with i3...
        // for now using an idea value (0x0905)
        return AppleProcessorTypeCorei3Type5; // 0x0905
      }
      if (AppleMajorType == AppleProcessorMajorXeonNehalem) { // see TODO below
        // for Kaby Lake/Coffee Lake E3 (there's no E5/E7 on either), not used by Apple
        // NOTE: Xeon E3 is not used by Apple at all and should be somehow treated as i7,
        //       but here we'd like to show Xeon in "About This Mac".
        // TODO: CPU major type check for KBL/CFL based Xeon E3
        return AppleProcessorTypeXeon;        // 0x0501
      }
      // here stands for Pentium and Celeron (KBL/CFL), not used by Apple at all.
      // putting 0x0905 (i3) as lowest.
      return AppleProcessorTypeCorei3Type5; // 0x0905

    default:
      // NOTE: by default it is really unknown, but we fallback
      return AppleProcessorTypeCorei5Type5; // 0x0605
  }
}

UINTN
OcGetPmTimerAddr (
  OUT CONST CHAR8 **Type  OPTIONAL
  )
{
  UINTN   TimerAddr;
  UINT32  CpuVendor;

  TimerAddr = 0;

  if (Type != NULL) {
    *Type = "Failure";
  }

  //
  // Intel timer support.
  // Here we obtain the address of 24-bit or 32-bit PM1_TMR.
  // TODO: I believe that there is little reason to enforce our timer lib to calculate
  // CPU frequency through ACPI PM timer on modern Intel CPUs. Starting from Skylake
  // we have crystal clock, which allows us to get quite reliable values. Perhaps
  // this code should be put to OcCpuLib, and the best available source is to be used.
  //
  if (PciRead16 (PCI_ICH_LPC_ADDRESS (0)) == V_ICH_PCI_VENDOR_ID) {
    //
    // On legacy platforms PM1_TMR can be found in ACPI I/O space.
    // 1. For platforms prior to Intel Skylake (Sunrisepoint PCH) iTCO watchdog
    //    resources reside in LPC device (D31:F0).
    // 2. For platforms from Intel Skylake till Intel Kaby Lake inclusive they reside in
    //    PMC controller (D31:F2).
    // Checking whether ACPI I/O space is enabled is done via ACPI_CNTL register bit 0.
    //
    // On modern platforms, starting from Intel Coffee Lake, the space is roughly the same,
    // but it is referred to as PMC I/O space, and the addressing is done through BAR2.
    // In addition to that on B360 and friends PMC controller may be just missing.
    //
    if ((PciRead8 (PCI_ICH_LPC_ADDRESS (R_ICH_LPC_ACPI_CNTL)) & B_ICH_LPC_ACPI_CNTL_ACPI_EN) != 0) {
      TimerAddr = (PciRead16 (PCI_ICH_LPC_ADDRESS (R_ICH_LPC_ACPI_BASE)) & B_ICH_LPC_ACPI_BASE_BAR) + R_ACPI_PM1_TMR;
      if (Type != NULL) {
        *Type = "LPC";
      }
    } else if (PciRead16 (PCI_ICH_PMC_ADDRESS (0)) == V_ICH_PCI_VENDOR_ID) {
      if ((PciRead8 (PCI_ICH_PMC_ADDRESS (R_ICH_PMC_ACPI_CNTL)) & B_ICH_PMC_ACPI_CNTL_ACPI_EN) != 0) {
        TimerAddr = (PciRead16 (PCI_ICH_PMC_ADDRESS (R_ICH_PMC_ACPI_BASE)) & B_ICH_PMC_ACPI_BASE_BAR) + R_ACPI_PM1_TMR;
        if (Type != NULL) {
          *Type = "PMC ACPI";
        }
      } else if ((PciRead16 (PCI_ICH_PMC_ADDRESS (R_ICH_PMC_BAR2_BASE)) & B_ICH_PMC_BAR2_BASE_BAR_EN) != 0) {
        TimerAddr = (PciRead16 (PCI_ICH_PMC_ADDRESS (R_ICH_PMC_BAR2_BASE)) & B_ICH_PMC_BAR2_BASE_BAR) + R_ACPI_PM1_TMR;
        if (Type != NULL) {
          *Type = "PMC BAR2";
        }
      } else if (Type != NULL) {
        *Type = "Invalid INTEL PMC";
      }
    } else if (Type != NULL) {
      //
      // This is currently the case for Z390 and B360 boards.
      //
      *Type = "Unknown INTEL";
    }
  }

  //
  // AMD timer support.
  //
  if (TimerAddr == 0) {
    //
    // In an ideal world I believe we should detect AMD SMBus controller...
    //
    CpuVendor = 0;
    AsmCpuid (CPUID_SIGNATURE, NULL, &CpuVendor, NULL, NULL);

    if (CpuVendor == CPUID_VENDOR_AMD) {
      TimerAddr = MmioRead32 (
        R_AMD_ACPI_MMIO_BASE + R_AMD_ACPI_MMIO_PMIO_BASE + R_AMD_ACPI_PM_TMR_BLOCK
        );
      if (Type != NULL) {
        *Type = "AMD";
      }
    }
  }

  return TimerAddr;
}

/**
  Calculate the TSC frequency via PM timer

  @param[in] Recalculate  Do not re-use previously cached information.

  @retval  The calculated TSC frequency.
**/
STATIC
UINT64
OcCalculateTSCFromPMTimer (
  IN BOOLEAN  Recalculate
  )
{
  //
  // Cache the result to speed up multiple calls. For example, we might need
  // this frequency on module entry to initialise a TimerLib instance, and at
  // a later point in time to gather CPU information.
  //
  STATIC UINT64 TSCFrequency = 0;

  UINTN    TimerAddr;
  UINT64   Tsc0;
  UINT64   Tsc1;
  UINT32   AcpiTick0;
  UINT32   AcpiTick1;
  UINT32   AcpiTicksDelta;
  UINT32   AcpiTicksTarget;
  UINT32   TimerResolution;
  EFI_TPL  PrevTpl;
  
  if (Recalculate) {
    TSCFrequency = 0;
  }

  if (TSCFrequency == 0) {
    TimerAddr       = OcGetPmTimerAddr (NULL);
    TimerResolution = 10;

    if (TimerAddr != 0) {
      //
      // Check that timer is advancing (it does not on some virtual machines).
      //
      AcpiTick0 = IoRead32 (TimerAddr);
      gBS->Stall (500);
      AcpiTick1 = IoRead32 (TimerAddr);

      if (AcpiTick0 != AcpiTick1) {
        //
        // ACPI PM timers are usually of 24-bit length, but there are some less common cases of 32-bit length also.
        // When the maximal number is reached, it overflows.
        // The code below can handle overflow with AcpiTicksTarget of up to 24-bit size,
        // on both available sizes of ACPI PM Timers (24-bit and 32-bit).
        //
        // 357954 clocks of ACPI timer (100ms)
        //
        AcpiTicksTarget = V_ACPI_TMR_FREQUENCY / TimerResolution;

        //
        // Disable all events to ensure that nobody interrupts us.
        //
        PrevTpl   = gBS->RaiseTPL (TPL_HIGH_LEVEL);

        AcpiTick0 = IoRead32 (TimerAddr);
        Tsc0      = AsmReadTsc ();

        do {
          CpuPause ();

          //
          // Check how many AcpiTicks have passed since we started.
          //
          AcpiTick1 = IoRead32 (TimerAddr);

          if (AcpiTick0 <= AcpiTick1) {
            //
            // No overflow.
            //
            AcpiTicksDelta = AcpiTick1 - AcpiTick0;
          } else if (AcpiTick0 - AcpiTick1 <= 0x00FFFFFF) {
            //
            // Overflow, 24-bit timer.
            //
            AcpiTicksDelta = 0x00FFFFFF - AcpiTick0 + AcpiTick1;
          } else {
            //
            // Overflow, 32-bit timer.
            //
            AcpiTicksDelta = MAX_UINT32 - AcpiTick0 + AcpiTick1;
          }

          //
          // Keep checking AcpiTicks until target is reached.
          //
        } while (AcpiTicksDelta < AcpiTicksTarget);

        Tsc1 = AsmReadTsc ();

        //
        // On some systems we may end up waiting for notably longer than 100ms,
        // despite disabling all events. Divide by actual time passed as suggested
        // by asava's Clover patch r2668.
        //
        TSCFrequency = DivU64x32 (
          MultU64x32 (Tsc1 - Tsc0, V_ACPI_TMR_FREQUENCY), AcpiTicksDelta
          );

        //
        // Restore to normal TPL.
        //
        gBS->RestoreTPL (PrevTpl);
      }
    }

    DEBUG ((DEBUG_VERBOSE, "TscFrequency %lld\n", TSCFrequency));
  }

  return TSCFrequency;
}

/**
  Calculate the ART frequency and derieve the CPU frequency for Intel CPUs

  @param[out] CPUFrequency  The derieved CPU frequency.
  @param[in]  Recalculate   Do not re-use previously cached information.

  @retval  The calculated ART frequency.
**/
STATIC
UINT64
OcCalcluateARTFrequencyIntel (
  OUT UINT64   *CPUFrequency,
  IN  BOOLEAN  Recalculate
  )
{
  //
  // Cache the result to speed up multiple calls. For example, we might need
  // this frequency on module entry to initialise a TimerLib instance, and at
  // a later point in time to gather CPU information.
  //
  STATIC UINT64 ARTFrequency        = 0;
  STATIC UINT64 CPUFrequencyFromART = 0;

  UINT32                                            MaxId;
  UINT32                                            CpuVendor;

  UINT32                                            CpuidDenominatorEax;
  UINT32                                            CpuidNumeratorEbx;
  UINT32                                            CpuidARTFrequencyEcx;
  CPUID_PROCESSOR_FREQUENCY_EAX                     CpuidFrequencyEax;
  UINT64                                            TscAdjust;
  UINT64                                            CPUFrequencyFromTSC;
  CPUID_VERSION_INFO_EAX                            CpuidVerEax;
  UINT8                                             Model;

  if (Recalculate) {
    ARTFrequency        = 0;
    CPUFrequencyFromART = 0;
  }

  if (ARTFrequency == 0) {
    //
    // Get vendor CPUID 0x00000000
    //
    AsmCpuid (CPUID_SIGNATURE, &MaxId, &CpuVendor, NULL, NULL);
    //
    // Determine our core crystal clock frequency
    //
    if (CpuVendor == CPUID_VENDOR_INTEL && MaxId >= CPUID_TIME_STAMP_COUNTER) {
      TscAdjust = AsmReadMsr64 (MSR_IA32_TSC_ADJUST);
      DEBUG ((DEBUG_INFO, "OCCPU: TSC Adjust %Lu\n", TscAdjust));

      AsmCpuid (
        CPUID_TIME_STAMP_COUNTER,
        &CpuidDenominatorEax,
        &CpuidNumeratorEbx,
        &CpuidARTFrequencyEcx,
        NULL
        );
      if (CpuidARTFrequencyEcx > 0) {
        ARTFrequency = CpuidARTFrequencyEcx;
        DEBUG ((DEBUG_INFO, "OCCPU: Queried Core Crystal Clock Frequency %11LuHz\n", ARTFrequency));
      } else {
        AsmCpuid (CPUID_VERSION_INFO, &CpuidVerEax.Uint32, NULL, NULL, NULL);
        Model = (UINT8) CpuidVerEax.Bits.Model | (UINT8) (CpuidVerEax.Bits.ExtendedModelId << 4U);
        //
        // Fall back to identifying ART frequency based on known models
        //
        switch (Model) {
          case CPU_MODEL_SKYLAKE:
          case CPU_MODEL_SKYLAKE_DT:
          case CPU_MODEL_KABYLAKE:
          case CPU_MODEL_KABYLAKE_DT:
            ARTFrequency = CLIENT_ART_CLOCK_SOURCE; // 24 Mhz
            break;
          case CPU_MODEL_DENVERTON:
            ARTFrequency = SERVER_ART_CLOCK_SOURCE; // 25 Mhz
            break;
          case CPU_MODEL_GOLDMONT:
            ARTFrequency = ATOM_ART_CLOCK_SOURCE; // 19.2 Mhz
            break;
        }
        if (ARTFrequency > 0) {
          DEBUG ((DEBUG_INFO, "OCCPU: Known Model Core Crystal Clock Frequency %11LuHz\n", ARTFrequency));
        }
      }

      if (CpuidDenominatorEax > 0 && CpuidNumeratorEbx > 0) {
        //
        // Some Intel chips don't report their core crystal clock frequency.
        // Calculate it by dividing the TSC frequency by the TSC ratio.
        //
        if (ARTFrequency == 0 && MaxId >= CPUID_PROCESSOR_FREQUENCY) {
          CPUFrequencyFromTSC = OcCalculateTSCFromPMTimer (Recalculate);
          ARTFrequency = MultThenDivU64x64x32(
            CPUFrequencyFromTSC,
            CpuidDenominatorEax,
            CpuidNumeratorEbx,
            NULL
            );
          if (ARTFrequency > 0ULL) {
            DEBUG ((
              DEBUG_INFO,
              "OCCPU: Core Crystal Clock Frequency from TSC %11LuHz = %11LuHz * %u / %u\n",
              ARTFrequency,
              CPUFrequencyFromTSC,
              CpuidDenominatorEax,
              CpuidNumeratorEbx
              ));
            //
            // Use the reported CPU frequency rather than deriving it from ARTFrequency
            //
            AsmCpuid (CPUID_PROCESSOR_FREQUENCY, &CpuidFrequencyEax.Uint32, NULL, NULL, NULL);
            CPUFrequencyFromART = MultU64x32 (CpuidFrequencyEax.Bits.ProcessorBaseFrequency, 1000000);
          }
        }

        //
        // If we still can't determine the core crystal clock frequency, assume
        // it's 24 Mhz like most Intel chips to date.
        //
        if (ARTFrequency == 0ULL) {
          ARTFrequency = DEFAULT_ART_CLOCK_SOURCE;
          DEBUG ((DEBUG_INFO, "OCCPU: Fallback Core Crystal Clock Frequency %11LuHz\n", ARTFrequency));
        }

        ASSERT (ARTFrequency > 0ULL);
        if (CPUFrequencyFromART == 0ULL) {
          CPUFrequencyFromART = MultThenDivU64x64x32 (
            ARTFrequency,
            CpuidNumeratorEbx,
            CpuidDenominatorEax,
            NULL
            );
        }
        ASSERT (CPUFrequencyFromART > 0ULL);
        DEBUG ((
          DEBUG_INFO,
          "OCCPU: CPUFrequencyFromART %11LuHz %5LuMHz = %Lu * %u / %u\n",
          CPUFrequencyFromART,
          DivU64x32 (CPUFrequencyFromART, 1000000),
          ARTFrequency,
          CpuidNumeratorEbx,
          CpuidDenominatorEax
          ));
      }
    }
  }

  *CPUFrequency = CPUFrequencyFromART;
  return ARTFrequency;
}

UINT64
OcGetTSCFrequency (
  VOID
  )
{
  UINT64 CPUFrequency;
  //
  // For Intel platforms (the vendor check is covered by the callee), prefer
  // the CPU Frequency derieved from the ART, as the PM timer might not be
  // available (e.g. 300 series chipsets).
  // TODO: For AMD, the base clock can be determined from P-registers.
  //
  OcCalcluateARTFrequencyIntel (&CPUFrequency, FALSE);
  if (CPUFrequency == 0) {
    CPUFrequency = OcCalculateTSCFromPMTimer (FALSE);
  }
  //
  // For all known models with an invariant TSC, its frequency is equal to the
  // CPU's specified base clock.
  //
  return CPUFrequency;
}

VOID
ScanIntelProcessor (
  IN OUT OC_CPU_INFO  *Cpu
  )
{
  UINT64                                            Msr;
  CPUID_CACHE_PARAMS_EAX                            CpuidCacheEax;
  CPUID_CACHE_PARAMS_EBX                            CpuidCacheEbx;
  UINT8                                             AppleMajorType;
  MSR_SANDY_BRIDGE_PKG_CST_CONFIG_CONTROL_REGISTER  PkgCstConfigControl;
  MSR_IA32_PERF_STATUS_REGISTER                     PerfStatus;
  MSR_NEHALEM_PLATFORM_INFO_REGISTER                PlatformInfo;
  MSR_NEHALEM_TURBO_RATIO_LIMIT_REGISTER            TurboLimit;
  UINT16                                            CoreCount;
  CONST CHAR8                                       *TimerSourceType;
  UINTN                                             TimerAddr;
  BOOLEAN                                           Recalculate;

  AppleMajorType = DetectAppleMajorType (Cpu->BrandString);
  Cpu->AppleProcessorType = DetectAppleProcessorType (Cpu->Model, Cpu->Stepping, AppleMajorType);

  DEBUG ((DEBUG_INFO, "OCCPU: Detected Apple Processor Type: %02X -> %04X\n", AppleMajorType, Cpu->AppleProcessorType));

  if ((Cpu->Family != 0x06 || Cpu->Model < 0x0c)
    && (Cpu->Family != 0x0f || Cpu->Model < 0x03)) {
    return;
  }

  if (Cpu->Model >= CPU_MODEL_SANDYBRIDGE) {
    PkgCstConfigControl.Uint64 = AsmReadMsr64 (MSR_SANDY_BRIDGE_PKG_CST_CONFIG_CONTROL);
    Cpu->CstConfigLock = PkgCstConfigControl.Bits.CFGLock == 1;
  }

  //
  // TODO: this may not be accurate on some older processors.
  //
  if (Cpu->Model >= CPU_MODEL_NEHALEM) {
    PerfStatus.Uint64 = AsmReadMsr64 (MSR_IA32_PERF_STATUS);
    Cpu->CurBusRatio = (UINT8) (PerfStatus.Bits.State >> 8U);
    PlatformInfo.Uint64 = AsmReadMsr64 (MSR_NEHALEM_PLATFORM_INFO);
    Cpu->MinBusRatio = (UINT8) PlatformInfo.Bits.MaximumEfficiencyRatio;
    Cpu->MaxBusRatio = (UINT8) PlatformInfo.Bits.MaximumNonTurboRatio;
  } else if (Cpu->Model >= CPU_MODEL_PENRYN) {
    PerfStatus.Uint64 = AsmReadMsr64 (MSR_IA32_PERF_STATUS);
    Cpu->MaxBusRatio = (UINT8) (PerfStatus.Uint64 >> 8U) & 0x1FU;
    //
    // Undocumented values:
    // Non-integer bus ratio for the max-multi.
    // Non-integer bus ratio for the current-multi.
    //
    // MaxBusRatioDiv = (UINT8)(PerfStatus.Uint64 >> 46U) & 0x01U;
    // CurrDiv = (UINT8)(PerfStatus.Uint64 >> 14U) & 0x01U;
    //
  }

  if (Cpu->Model >= CPU_MODEL_NEHALEM
    && Cpu->Model != CPU_MODEL_NEHALEM_EX
    && Cpu->Model != CPU_MODEL_WESTMERE_EX) {
    TurboLimit.Uint64 = AsmReadMsr64 (MSR_NEHALEM_TURBO_RATIO_LIMIT);
    Cpu->TurboBusRatio1 = (UINT8) TurboLimit.Bits.Maximum1C;
    Cpu->TurboBusRatio2 = (UINT8) TurboLimit.Bits.Maximum2C;
    Cpu->TurboBusRatio3 = (UINT8) TurboLimit.Bits.Maximum3C;
    Cpu->TurboBusRatio4 = (UINT8) TurboLimit.Bits.Maximum4C;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCCPU: Ratio Min %d Max %d Current %d Turbo %d %d %d %d\n",
    Cpu->MinBusRatio,
    Cpu->MaxBusRatio,
    Cpu->CurBusRatio,
    Cpu->TurboBusRatio1,
    Cpu->TurboBusRatio2,
    Cpu->TurboBusRatio3,
    Cpu->TurboBusRatio4
    ));

  //
  // For logging purposes (the first call to these functions might happen
  // before logging is fully initialised), do not use the cached results in
  // DEBUG builds.
  //
  Recalculate = FALSE;

  DEBUG_CODE_BEGIN ();
  Recalculate = TRUE;
  DEBUG_CODE_END ();

  //
  // Calculate the Tsc frequency
  //
  DEBUG_CODE_BEGIN ();
  TimerAddr = OcGetPmTimerAddr (&TimerSourceType);
  DEBUG ((DEBUG_INFO, "OCCPU: Timer address is %Lx from %a\n", (UINT64) TimerAddr, TimerSourceType));
  DEBUG_CODE_END ();
  Cpu->CPUFrequencyFromTSC = OcCalculateTSCFromPMTimer (Recalculate);

  //
  // Determine our core crystal clock frequency
  //
  Cpu->ARTFrequency = OcCalcluateARTFrequencyIntel (&Cpu->CPUFrequencyFromART, Recalculate);

  //
  // Calculate CPU frequency based on ART if present, otherwise TSC
  //
  Cpu->CPUFrequency = Cpu->CPUFrequencyFromART > 0 ? Cpu->CPUFrequencyFromART : Cpu->CPUFrequencyFromTSC;

  //
  // Verify that our two CPU frequency calculations do not differ substantially.
  //
  if (Cpu->CPUFrequencyFromART > 0 && Cpu->CPUFrequencyFromTSC > 0
    && ABS((INT64) Cpu->CPUFrequencyFromART - (INT64) Cpu->CPUFrequencyFromTSC) > OC_CPU_FREQUENCY_TOLERANCE) {
    DEBUG ((
      DEBUG_WARN,
      "OCCPU: ART based CPU frequency differs substantially from TSC: %11LuHz != %11LuHz\n",
      Cpu->CPUFrequencyFromART,
      Cpu->CPUFrequencyFromTSC
      ));
  }

  //
  // There may be some quirks with virtual CPUs (VMware is fine).
  // Formerly we checked Cpu->MinBusRatio > 0, but we have no MinBusRatio on Penryn.
  //
  if (Cpu->CPUFrequency > 0 && Cpu->MaxBusRatio > Cpu->MinBusRatio) {
    Cpu->FSBFrequency = DivU64x32 (Cpu->CPUFrequency, Cpu->MaxBusRatio);
  } else {
    //
    // TODO: It seems to be possible that CPU frequency == 0 here...
    //
    Cpu->FSBFrequency = 100000000; // 100 Mhz
  }

  //
  // Calculate number of cores
  //
  if (Cpu->MaxId >= CPUID_CACHE_PARAMS && Cpu->Model <= CPU_MODEL_PENRYN) {
    AsmCpuidEx (CPUID_CACHE_PARAMS, 0, &CpuidCacheEax.Uint32, &CpuidCacheEbx.Uint32, NULL, NULL);
    if (CpuidCacheEax.Bits.CacheType != CPUID_CACHE_PARAMS_CACHE_TYPE_NULL) {
      CoreCount = (UINT16)GetPowerOfTwo32 (CpuidCacheEax.Bits.MaximumAddressableIdsForProcessorCores + 1);
      if (CoreCount < CpuidCacheEax.Bits.MaximumAddressableIdsForProcessorCores + 1) {
        CoreCount *= 2;
      }
      Cpu->CoreCount   = CoreCount;
      //
      // We should not be blindly relying on Cpu->Features & CPUID_FEATURE_HTT.
      // On Penryn CPUs it is set even without Hyper Threading.
      //
      if (Cpu->ThreadCount < Cpu->CoreCount) {
        Cpu->ThreadCount = Cpu->CoreCount;
      }
    }
  } else if (Cpu->Model == CPU_MODEL_WESTMERE) {
    Msr = AsmReadMsr64 (MSR_CORE_THREAD_COUNT);
    Cpu->CoreCount   = (UINT16)BitFieldRead64 (Msr, 16, 19);
    Cpu->ThreadCount = (UINT16)BitFieldRead64 (Msr, 0,  15);
  } else {
    Msr = AsmReadMsr64 (MSR_CORE_THREAD_COUNT);
    Cpu->CoreCount   = (UINT16)BitFieldRead64 (Msr, 16, 31);
    Cpu->ThreadCount = (UINT16)BitFieldRead64 (Msr, 0,  15);
  }

  if (Cpu->CoreCount == 0) {
    Cpu->CoreCount = 1;
  }

  if (Cpu->ThreadCount == 0) {
    Cpu->ThreadCount = 1;
  }

  //
  // TODO: handle package count...
  //
}

VOID
ScanAmdProcessor (
  IN OUT OC_CPU_INFO  *Cpu
  )
{
  UINT32  CpuidEbx;
  UINT32  CpuidEcx;
  UINT64  CofVid;
  UINT64  CoreFrequencyID;
  UINT64  CoreDivisorID;
  UINT64  Divisor;
  BOOLEAN Recalculate;

  //
  // For logging purposes (the first call to these functions might happen
  // before logging is fully initialised), do not use the cached results in
  // DEBUG builds.
  //
  Recalculate = FALSE;

  DEBUG_CODE_BEGIN ();
  Recalculate = TRUE;
  DEBUG_CODE_END ();

  //
  // Faking an Intel Core i5 Processor.
  // This value is purely cosmetic, but it makes sense to fake something
  // that is somewhat representative of the kind of Processor that's actually
  // in the system
  //
  Cpu->AppleProcessorType = AppleProcessorTypeCorei5Type5;
  //
  // get TSC Frequency calculated in OcTimerLib
  // FIXME(1): This code assumes the CPU operates in P0.  Either ensure it does
  //           and raise the mode on demand, or adapt the logic to consider
  //           both the operating and the nominal frequency, latter for
  //           the invariant TSC.
  //
  Cpu->CPUFrequencyFromTSC = OcCalculateTSCFromPMTimer (Recalculate);
  Cpu->CPUFrequency = Cpu->CPUFrequencyFromTSC;
  //
  // Get core and thread count from CPUID
  //
  if (Cpu->MaxExtId >= 0x80000008) {
    AsmCpuid (0x80000008, NULL, NULL, &CpuidEcx, NULL);
    Cpu->ThreadCount = (UINT16) (BitFieldRead32 (CpuidEcx, 0, 7) + 1);
  }

  if (Cpu->Family == AMD_CPU_FAMILY) {
    Divisor = 0;

    switch (Cpu->ExtFamily) {
      case AMD_CPU_EXT_FAMILY_17H:
        CofVid           = AsmReadMsr64 (K10_PSTATE_STATUS);
        CoreFrequencyID  = BitFieldRead64 (CofVid, 0, 7);
        CoreDivisorID    = BitFieldRead64 (CofVid, 8, 13);
        Cpu->MaxBusRatio = (UINT8) (CoreFrequencyID / CoreDivisorID * 2);
        //
        // Get core count from CPUID
        //
        if (Cpu->MaxExtId >= 0x8000001E) {
          AsmCpuid (0x8000001E, NULL, &CpuidEbx, NULL, NULL);
          Cpu->CoreCount =
            (UINT16) DivU64x32 (
              Cpu->ThreadCount,
              (BitFieldRead32 (CpuidEbx, 8, 15) + 1)
            );
        }
        break;
      case AMD_CPU_EXT_FAMILY_15H:
      case AMD_CPU_EXT_FAMILY_16H:
        // FIXME: Please refer to FIXME(1) for the MSR used here.
        CofVid           = AsmReadMsr64 (K10_COFVID_STATUS);
        CoreFrequencyID  = BitFieldRead64 (CofVid, 0, 5);
        CoreDivisorID    = BitFieldRead64 (CofVid, 6, 8);
        Divisor          = LShiftU64 (1, CoreDivisorID);
        //
        // BKDG for AMD Family 15h Models 10h-1Fh Processors (42300 Rev 3.12)
        // Core current operating frequency in MHz. CoreCOF = 100 *
        // (MSRC001_00[6B:64][CpuFid] + 10h) / (2 ^ MSRC001_00[6B:64][CpuDid]).
        //
        Cpu->MaxBusRatio = (UINT8)((CoreFrequencyID + 0x10) / Divisor);
        //
        // AMD 15h and 16h CPUs don't support hyperthreading,
        // so the core count is equal to the thread count
        //
        Cpu->CoreCount = Cpu->ThreadCount;
        break;
      default:
        return;
    }

    DEBUG ((
      DEBUG_INFO,
      "OCCPU: FID %Lu DID %Lu Divisor %Lu MaxBR %u\n",
      CoreFrequencyID,
      CoreDivisorID,
      Divisor,
      Cpu->MaxBusRatio
      ));

    //
    // CPUPM is not supported on AMD, meaning the current
    // and minimum bus ratio are equal to the maximum bus ratio
    //
    Cpu->CurBusRatio = Cpu->MaxBusRatio;
    Cpu->MinBusRatio = Cpu->MaxBusRatio;

    Cpu->FSBFrequency = DivU64x32 (Cpu->CPUFrequency, Cpu->MaxBusRatio);
  }
}

/** Scan the processor and fill the cpu info structure with results

  @param[in] Cpu  A pointer to the cpu info structure to fill with results

  @retval EFI_SUCCESS  The scan was completed successfully.
**/
VOID
OcCpuScanProcessor (
  IN OUT OC_CPU_INFO  *Cpu
  )
{
  UINT32                  CpuidEax;
  UINT32                  CpuidEbx;
  UINT32                  CpuidEcx;
  UINT32                  CpuidEdx;

  ASSERT (Cpu != NULL);

  ZeroMem (Cpu, sizeof (*Cpu));

  //
  // Get vendor CPUID 0x00000000
  //
  AsmCpuid (CPUID_SIGNATURE, &CpuidEax, &Cpu->Vendor[0], &Cpu->Vendor[2], &Cpu->Vendor[1]);

  Cpu->MaxId = CpuidEax;

  //
  // Get extended CPUID 0x80000000
  //
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &CpuidEax, &CpuidEbx, &CpuidEcx, &CpuidEdx);

  Cpu->MaxExtId = CpuidEax;

  //
  // Get brand string CPUID 0x80000002 - 0x80000004
  //
  if (Cpu->MaxExtId >= CPUID_BRAND_STRING3) {
    //
    // The brandstring 48 bytes max, guaranteed NULL terminated.
    //
    UINT32  *BrandString = (UINT32 *) Cpu->BrandString;

    AsmCpuid (
      CPUID_BRAND_STRING1,
      BrandString,
      (BrandString + 1),
      (BrandString + 2),
      (BrandString + 3)
      );

    AsmCpuid (
      CPUID_BRAND_STRING2,
      (BrandString + 4),
      (BrandString + 5),
      (BrandString + 6),
      (BrandString + 7)
      );

    AsmCpuid (
      CPUID_BRAND_STRING3,
      (BrandString + 8),
      (BrandString + 9),
      (BrandString + 10),
      (BrandString + 11)
      );
  }

  Cpu->PackageCount = 1;
  Cpu->CoreCount    = 1;
  Cpu->ThreadCount  = 1;

  //
  // Get processor signature and decode
  //
  if (Cpu->MaxId >= CPUID_VERSION_INFO) {
    //
    // Intel SDM requires us issuing CPUID 1 read during microcode
    // version read, so let's do it here for simplicity.
    //

    if (Cpu->Vendor[0] == CPUID_VENDOR_INTEL) {
      AsmWriteMsr64 (MSR_IA32_BIOS_SIGN_ID, 0);
    }

    AsmCpuid (
      CPUID_VERSION_INFO,
      &Cpu->CpuidVerEax.Uint32,
      &Cpu->CpuidVerEbx.Uint32,
      &Cpu->CpuidVerEcx.Uint32,
      &Cpu->CpuidVerEdx.Uint32
      );

    if (Cpu->Vendor[0] == CPUID_VENDOR_INTEL) {
      Cpu->MicrocodeRevision = AsmReadMsr32 (MSR_IA32_BIOS_SIGN_ID);
    }

    Cpu->Signature = Cpu->CpuidVerEax.Uint32;
    Cpu->Stepping  = (UINT8) Cpu->CpuidVerEax.Bits.SteppingId;
    Cpu->ExtModel  = (UINT8) Cpu->CpuidVerEax.Bits.ExtendedModelId;
    Cpu->Model     = (UINT8) Cpu->CpuidVerEax.Bits.Model | (UINT8) (Cpu->CpuidVerEax.Bits.ExtendedModelId << 4U);
    Cpu->Family    = (UINT8) Cpu->CpuidVerEax.Bits.FamilyId;
    Cpu->Type      = (UINT8) Cpu->CpuidVerEax.Bits.ProcessorType;
    Cpu->ExtFamily = (UINT8) Cpu->CpuidVerEax.Bits.ExtendedFamilyId;
    Cpu->Brand     = (UINT8) Cpu->CpuidVerEbx.Bits.BrandIndex;
    Cpu->Features  = (((UINT64) Cpu->CpuidVerEcx.Uint32) << 32ULL) | Cpu->CpuidVerEdx.Uint32;
    if (Cpu->Features & CPUID_FEATURE_HTT) {
      Cpu->ThreadCount = (UINT16) Cpu->CpuidVerEbx.Bits.MaximumAddressableIdsForLogicalProcessors;
    }
  }

  DEBUG ((DEBUG_INFO, "OCCPU: %a %a\n", "Found", Cpu->BrandString));

  DEBUG ((
    DEBUG_INFO,
    "OCCPU: Signature %0X Stepping %0X Model %0X Family %0X Type %0X ExtModel %0X ExtFamily %0X\n",
    Cpu->Signature,
    Cpu->Stepping,
    Cpu->Model,
    Cpu->Family,
    Cpu->Type,
    Cpu->ExtModel,
    Cpu->ExtFamily
    ));

  if (Cpu->Vendor[0] == CPUID_VENDOR_INTEL) {
    ScanIntelProcessor (Cpu);
  } else if (Cpu->Vendor[0] == CPUID_VENDOR_AMD) {
    ScanAmdProcessor (Cpu);
  } else {
    DEBUG ((DEBUG_WARN, "Found unsupported CPU vendor: %0X", Cpu->Vendor[0]));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCCPU: CPUFrequencyFromTSC %11LuHz %5LuMHz\n",
    Cpu->CPUFrequencyFromTSC,
    DivU64x32 (Cpu->CPUFrequencyFromTSC, 1000000)
    ));

  DEBUG ((
    DEBUG_INFO,
    "OCCPU: CPUFrequency %11LuHz %5LuMHz\n",
    Cpu->CPUFrequency,
    DivU64x32 (Cpu->CPUFrequency, 1000000)
    ));

  DEBUG ((
    DEBUG_INFO,
    "OCCPU: FSBFrequency %11LuHz %5LuMHz\n",
    Cpu->FSBFrequency,
    DivU64x32 (Cpu->FSBFrequency, 1000000)
    ));

  DEBUG ((
    DEBUG_INFO,
    "OCCPU: Pkg %u Cores %u Threads %u\n",
    Cpu->PackageCount,
    Cpu->CoreCount,
    Cpu->ThreadCount
    ));
}

VOID
OcCpuCorrectFlexRatio (
  IN OC_CPU_INFO  *Cpu
  )
{
  UINT64  Msr;
  UINT64  FlexRatio;

  if (Cpu->Vendor[0] == CPUID_VENDOR_INTEL
    && Cpu->Model != CPU_MODEL_GOLDMONT
    && Cpu->Model != CPU_MODEL_AIRMONT
    && Cpu->Model != CPU_MODEL_AVOTON) {
    Msr = AsmReadMsr64 (MSR_FLEX_RATIO);
    if (Msr & FLEX_RATIO_EN) {
      FlexRatio = BitFieldRead64 (Msr, 8, 15);
      if (FlexRatio == 0) {
        //
        // Disable Flex Ratio if current value is 0.
        //
        AsmWriteMsr64 (MSR_FLEX_RATIO, Msr & ~((UINT64) FLEX_RATIO_EN));
      }
    }
  }
}

UINT32
OcCpuModelToAppleFamily (
  IN CPUID_VERSION_INFO_EAX   VersionEax
  )
{
  UINT8                   Model;

  if (VersionEax.Bits.FamilyId != 6) {
    return CPUFAMILY_UNKNOWN;
  }

  //
  // This MUST be 1 to 1 with Apple XNU kernel implemenation.
  //

  Model = (UINT8) VersionEax.Bits.Model | (UINT8) (VersionEax.Bits.ExtendedModelId << 4U);

  switch (Model) {
    case CPU_MODEL_PENRYN:
      return CPUFAMILY_INTEL_PENRYN;
    case CPU_MODEL_NEHALEM:
    case CPU_MODEL_FIELDS:
    case CPU_MODEL_DALES:
    case CPU_MODEL_NEHALEM_EX:
      return CPUFAMILY_INTEL_NEHALEM;
    case CPU_MODEL_DALES_32NM:
    case CPU_MODEL_WESTMERE:
    case CPU_MODEL_WESTMERE_EX:
      return CPUFAMILY_INTEL_WESTMERE;
    case CPU_MODEL_SANDYBRIDGE:
    case CPU_MODEL_JAKETOWN:
      return CPUFAMILY_INTEL_SANDYBRIDGE;
    case CPU_MODEL_IVYBRIDGE:
    case CPU_MODEL_IVYBRIDGE_EP:
      return CPUFAMILY_INTEL_IVYBRIDGE;
    case CPU_MODEL_HASWELL:
    case CPU_MODEL_HASWELL_EP:
    case CPU_MODEL_HASWELL_ULT:
    case CPU_MODEL_CRYSTALWELL:
      return CPUFAMILY_INTEL_HASWELL;
    case CPU_MODEL_BROADWELL:
    case CPU_MODEL_BRYSTALWELL:
      return CPUFAMILY_INTEL_BROADWELL;
    case CPU_MODEL_SKYLAKE:
    case CPU_MODEL_SKYLAKE_DT:
    case CPU_MODEL_SKYLAKE_W:
      return CPUFAMILY_INTEL_SKYLAKE;
    case CPU_MODEL_KABYLAKE:
    case CPU_MODEL_KABYLAKE_DT:
      return CPUFAMILY_INTEL_KABYLAKE;
    default:
      return CPUFAMILY_UNKNOWN;
  }
}

BOOLEAN
OcIsSandyOrIvy (
  VOID
  )
{
  CPU_MICROCODE_PROCESSOR_SIGNATURE  Sig;
  BOOLEAN                            SandyOrIvy;
  UINT32                             CpuFamily;
  UINT32                             CpuModel;

  Sig.Uint32 = 0;

  AsmCpuid (1, &Sig.Uint32, NULL, NULL, NULL);

  CpuFamily = Sig.Bits.Family;
  if (CpuFamily == 15) {
    CpuFamily += Sig.Bits.ExtendedFamily;
  }

  CpuModel = Sig.Bits.Model;
  if (CpuFamily == 15 || CpuFamily == 6) {
    CpuModel |= Sig.Bits.ExtendedModel << 4;
  }

  SandyOrIvy = CpuFamily == 6 && (CpuModel == 0x2A || CpuModel == 0x3A);

  DEBUG ((
    DEBUG_VERBOSE,
    "OCCPU: Discovered CpuFamily %d CpuModel %d SandyOrIvy %a\n",
    CpuFamily,
    CpuModel,
    SandyOrIvy ? "YES" : "NO"
    ));

  return SandyOrIvy;
}
