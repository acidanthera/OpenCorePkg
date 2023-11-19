/** @file
  CPUID kernel patches.

Copyright (c) 2018-2020, vit9696, Goldfish64. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <IndustryStandard/AppleIntelCpuInfo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/PrintLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiLib.h>

//
// CPUID EAX=0x2 Cache descriptor values
//
STATIC
CONST struct {
  UINT8           value;
  cache_type_t    type;
  UINT32          size;
  UINT8           associativity;
  UINT8           linesize;
  UINT8           partitions;
} mCpuCacheDescriptorValues[] = {
  { 0x06, L1I, SIZE_8KB,               4,  32, 1 },
  { 0x08, L1I, SIZE_16KB,              4,  32, 1 },
  { 0x0A, L1D, SIZE_8KB,               2,  32, 1 },
  { 0x0C, L1D, SIZE_16KB,              4,  32, 1 },
  { 0x0E, L1D, 24*SIZE_1KB,            6,  64, 1 },
  { 0x22, L3U, SIZE_512KB,             4,  64, 2 },
  { 0x23, L3U, SIZE_1MB,               8,  64, 2 },
  { 0x25, L3U, SIZE_2MB,               8,  64, 2 },
  { 0x29, L3U, SIZE_4MB,               8,  64, 2 },
  { 0x2C, L1D, SIZE_32KB,              8,  64, 2 },
  { 0x30, L1I, SIZE_32KB,              8,  64, 1 },
  { 0x41, L2U, SIZE_128KB,             4,  32, 1 },
  { 0x42, L2U, SIZE_256KB,             4,  32, 1 },
  { 0x43, L2U, SIZE_512KB,             4,  32, 1 },
  { 0x44, L2U, SIZE_1MB,               4,  32, 1 },
  { 0x45, L2U, SIZE_2MB,               4,  32, 1 },
  { 0x46, L3U, SIZE_4MB,               4,  64, 1 },
  { 0x47, L3U, SIZE_8MB,               8,  64, 1 },
  { 0x48, L2U, 3*SIZE_1MB,             12, 64, 1 },
  { 0x49, L2U, SIZE_4MB,               16, 64, 1 }, // for Xeons family Fh model 6h it's L3U instead
  { 0x4A, L3U, 6*SIZE_1MB,             12, 64, 1 },
  { 0x4B, L3U, SIZE_8MB,               16, 64, 1 },
  { 0x4C, L3U, 12*SIZE_1MB,            12, 64, 1 },
  { 0x4D, L3U, SIZE_16MB,              16, 64, 1 },
  { 0x4E, L2U, 6*SIZE_1MB,             24, 64, 1 },
  { 0x60, L1D, SIZE_16KB,              8,  64, 1 },
  { 0x66, L1D, SIZE_8KB,               4,  64, 1 },
  { 0x67, L1D, SIZE_16KB,              4,  64, 1 },
  { 0x68, L1D, SIZE_32KB,              4,  64, 1 },
  { 0x78, L2U, SIZE_1MB,               4,  64, 1 },
  { 0x79, L2U, SIZE_128KB,             8,  64, 2 },
  { 0x7A, L2U, SIZE_256KB,             8,  64, 2 },
  { 0x7B, L2U, SIZE_512KB,             8,  64, 2 },
  { 0x7C, L2U, SIZE_1MB,               8,  64, 2 },
  { 0x7D, L2U, SIZE_2MB,               8,  64, 1 },
  { 0x7F, L2U, SIZE_512KB,             2,  64, 1 },
  { 0x80, L2U, SIZE_512KB,             8,  64, 1 },
  { 0x82, L2U, SIZE_256KB,             8,  32, 1 },
  { 0x83, L2U, SIZE_512KB,             8,  32, 1 },
  { 0x84, L2U, SIZE_1MB,               8,  32, 1 },
  { 0x85, L2U, SIZE_2MB,               8,  32, 1 },
  { 0x86, L2U, SIZE_512KB,             4,  64, 1 },
  { 0x87, L2U, SIZE_1MB,               8,  64, 1 },
  { 0xD0, L3U, SIZE_512KB,             4,  64, 1 },
  { 0xD1, L3U, SIZE_1MB,               4,  64, 1 },
  { 0xD2, L3U, SIZE_2MB,               4,  64, 1 },
  { 0xD6, L3U, SIZE_1MB,               8,  64, 1 },
  { 0xD7, L3U, SIZE_2MB,               8,  64, 1 },
  { 0xD8, L3U, SIZE_4MB,               8,  64, 1 },
  { 0xDC, L3U, (UINT32)(1.5*SIZE_1MB), 12, 64, 1 },
  { 0xDD, L3U, 3*SIZE_1MB,             12, 64, 1 },
  { 0xDE, L3U, 6*SIZE_1MB,             12, 64, 1 },
  { 0xE2, L3U, SIZE_2MB,               16, 64, 1 },
  { 0xE3, L3U, SIZE_4MB,               16, 64, 1 },
  { 0xE4, L3U, SIZE_8MB,               16, 64, 1 },
  { 0xEA, L3U, 12*SIZE_1MB,            24, 64, 1 },
  { 0xEB, L3U, 18*SIZE_1MB,            24, 64, 1 },
  { 0xEC, L3U, 24*SIZE_1MB,            24, 64, 1 }
};

STATIC
CONST UINT8
  mKernelCpuIdFindRelNew[] = {
  0xB9, 0x8B, 0x00, 0x00, 0x00, // mov ecx, 8Bh
  0x31, 0xC0,                   // xor eax, eax
  0x31, 0xD2,                   // xor edx, edx
  0x0F, 0x30,                   // wrmsr
  0xB8, 0x01, 0x00, 0x00, 0x00, // mov eax, 1
  0x31, 0xDB,                   // xor ebx, ebx
  0x31, 0xC9,                   // xor ecx, ecx
  0x31, 0xD2,                   // xor edx, edx
  0x0F, 0xA2                    // cpuid
};

STATIC
CONST UINT8
  mKernelCpuIdFindRelOld[] = {
  0xB9, 0x8B, 0x00, 0x00, 0x00, // mov ecx, 8Bh
  0x31, 0xD2,                   // xor edx, edx
  0x0F, 0x30,                   // wrmsr
  0xB8, 0x01, 0x00, 0x00, 0x00, // mov eax, 1
  0x31, 0xDB,                   // xor ebx, ebx
  0x31, 0xC9,                   // xor ecx, ecx
  0x31, 0xD2,                   // xor edx, edx
  0x0F, 0xA2                    // cpuid
};

STATIC
CONST UINT8
  mKernelCpuidFindMcRel[] = {
  0xB9, 0x8B, 0x00, 0x00, 0x00, // mov ecx, 8Bh
  0x0F, 0x32                    // rdmsr
};

/**
  cpu->cpuid_signature           = 0x11111111;
  cpu->cpuid_stepping            = 0x22;
  cpu->cpuid_model               = 0x33;
  cpu->cpuid_family              = 0x44;
  cpu->cpuid_type                = 0x55555555;
  cpu->cpuid_extmodel            = 0x66;
  cpu->cpuid_extfamily           = 0x77;
  cpu->cpuid_features            = 0x8888888888888888;
  cpu->cpuid_logical_per_package = 0x99999999;
  cpu->cpuid_cpufamily           = 0xAAAAAAAA;
  return 0xAAAAAAAA;
**/

STATIC
CONST UINT8
  mKernelCpuidReplaceDbg[] = {
  0xC7, 0x47, 0x68, 0x11, 0x11, 0x11, 0x11,                   // mov dword ptr [rdi+68h], 11111111h
  0xC6, 0x47, 0x50, 0x22,                                     // mov byte ptr [rdi+50h], 22h
  0x48, 0xB8, 0x55, 0x55, 0x55, 0x55, 0x44, 0x33, 0x66, 0x77, // mov rax, 7766334455555555h
  0x48, 0x89, 0x47, 0x48,                                     // mov [rdi+48h], rax
  0x48, 0xB8, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, // mov rax, 8888888888888888h
  0x48, 0x89, 0x47, 0x58,                                     // mov [rdi+58h], rax
  0xC7, 0x87, 0xCC, 0x00, 0x00, 0x00, 0x99, 0x99, 0x99, 0x99, // mov dword ptr [rdi+0CCh], 99999999h
  0xC7, 0x87, 0x88, 0x01, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, // mov dword ptr [rdi+188h], AAAAAAAAh
  0xB8, 0xAA, 0xAA, 0xAA, 0xAA,                               // mov eax, AAAAAAAAh
  0xC3                                                        // retn
};

#pragma pack(push, 1)

typedef struct {
  UINT8     Code1[3];
  UINT32    Signature;
  UINT8     Code2[3];
  UINT8     Stepping;
  UINT8     Code3[2];
  UINT32    Type;
  UINT8     Family;
  UINT8     Model;
  UINT8     ExtModel;
  UINT8     ExtFamily;
  UINT8     Code4[6];
  UINT64    Features;
  UINT8     Code5[10];
  UINT32    LogicalPerPkg;
  UINT8     Code6[6];
  UINT32    AppleFamily1;
  UINT8     Code7;
  UINT32    AppleFamily2;
  UINT8     Code8;
} INTERNAL_CPUID_FN_PATCH;

STATIC_ASSERT (
  sizeof (INTERNAL_CPUID_FN_PATCH) == sizeof (mKernelCpuidReplaceDbg),
  "Check your CPUID patch layout"
  );

typedef struct {
  UINT8     EaxCmd;
  UINT32    EaxVal;
  UINT8     EbxCmd;
  UINT32    EbxVal;
  UINT8     EcxCmd;
  UINT32    EcxVal;
  UINT8     EdxCmd;
  UINT32    EdxVal;
} INTERNAL_CPUID_PATCH;

typedef struct {
  UINT8     EdxCmd;
  UINT32    EdxVal;
} INTERNAL_MICROCODE_PATCH;

#pragma pack(pop)

STATIC
EFI_STATUS
PatchKernelCpuIdLegacy (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     UINT32           KernelVersion,
  IN     OC_CPU_INFO      *CpuInfo,
  IN     UINT32           *Data,
  IN     UINT32           *DataMask,
  IN     UINT8            *Start,
  IN     UINT8            *Last
  )
{
  EFI_STATUS  Status;

  UINT8                 *Record;
  UINT8                 *BlockClearFunc;
  UINT8                 *StartPointer;
  UINT8                 *EndPointer;
  UINT32                StructAddr;
  UINT8                 *Location;
  UINT8                 *LocationEnd;
  UINT8                 *LocationSnow32;
  UINT8                 *LocationTigerTsc;
  UINT8                 *LocationTigerTscEnd;
  UINT32                Signature[3];
  BOOLEAN               IsTiger;
  BOOLEAN               IsTigerOld;
  BOOLEAN               IsLeopard;
  BOOLEAN               IsSnow;
  BOOLEAN               IsLion;
  UINT32                Index;
  UINT32                MaxExt;
  INT32                 Delta;
  INTERNAL_CPUID_PATCH  Patch;

  //
  // XNU 8.10.1 and older require an additional patch to _tsc_init.
  //
  IsTiger    = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_TIGER_MIN, KERNEL_VERSION_TIGER_MAX);
  IsTigerOld = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_TIGER_MIN, KERNEL_VERSION (8, 10, 1));
  IsLeopard  = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LEOPARD_MIN, KERNEL_VERSION_LEOPARD_MAX);
  IsSnow     = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_SNOW_LEOPARD_MIN, KERNEL_VERSION_SNOW_LEOPARD_MAX);
  IsLion     = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LION_MIN, KERNEL_VERSION_LION_MAX);
  StructAddr = 0;

  LocationSnow32      = NULL;
  LocationTigerTsc    = NULL;
  LocationTigerTscEnd = NULL;

  //
  // Locate _cpuid_set_info or _cpuid_get_info.
  // _cpuid_set_info is also patched in 10.4, and is located below _cpuid_get_info.
  //
  Status = PatcherGetSymbolAddress (Patcher, IsTiger ? "_cpuid_get_info" : "_cpuid_set_info", (UINT8 **)&Record);
  if (EFI_ERROR (Status) || (Record >= Last)) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate _cpuid_%a_info (%p) - %r\n", IsTiger ? "get" : "set", Record, Status));
    return EFI_NOT_FOUND;
  }

  //
  // Start of patch area.
  //
  // The existing code here will be replaced with a hardcoded vendor string (CPUID (0)).
  // We'll also hardcode the CPUID (1) data here, jumped to from the next patch location area.
  //
  // 10.4 is in _cpuid_get_info.
  // Others are in _cpuid_set_info.
  //
  // 10.4 has the struct in esi.
  // 10.5 and 32-bit 10.6 have the struct hardcoded. This can be
  //   pulled right below the call to _blkclr, as below.
  // 64-bit 10.6 has the struct in rdi, a bit before lea rsi... below.
  // 32-bit 10.7 has the struct hardcoded, and can be pulled right
  // before the call to _bzero
  //
  STATIC CONST UINT8  mKernelCpuidFindPatchTigerStart[4] = {
    0x8D, 0x45, 0xEC,               // lea eax, dword [...]
    0xC7                            // mov from mov dword [...], 4
  };

  STATIC CONST UINT8  mKernelCpuidFindPatchLeoSnowLionStruct32[2] = {
    0x00,                           // 0 from end of mov dword [...], [struct address]
    0xE8                            // call from call _blkclr or _bzero
  };

  STATIC CONST UINT8  mKernelCpuidFindPatchLeoSnowLionStart32[5] = {
    0x04, 0x00, 0x00, 0x00,         // 4 from mov dword [...], 4
    0xC7                            // mov...
  };

  STATIC CONST UINT8  mKernelCpuidFindPatchSnowStart64[8] = {
    0xBA, 0x04, 0x00, 0x00, 0x00,  // mov edx, 4
    0x48, 0x8D, 0x35               // lea rsi, ...
  };

  if (Patcher->Is32Bit) {
    if (IsTiger) {
      for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (  (Record[0] == mKernelCpuidFindPatchTigerStart[0])
           && (Record[1] == mKernelCpuidFindPatchTigerStart[1])
           && (Record[2] == mKernelCpuidFindPatchTigerStart[2])
           && (Record[3] == mKernelCpuidFindPatchTigerStart[3]))
        {
          break;
        }
      }
    } else {
      //
      // We need to get the address of _blkclr or _bzero first for proper matching.
      //
      Status = PatcherGetSymbolAddress (Patcher, IsLion ? "_bzero" : "_blkclr", (UINT8 **)&BlockClearFunc);
      if (EFI_ERROR (Status) || (Record >= Last)) {
        DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate %a (%p) - %r\n", IsLion ? "_bzero" : "_blkclr", Record, Status));
        return EFI_NOT_FOUND;
      }

      //
      // Get struct address first.
      //
      for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (  (Record[0] == mKernelCpuidFindPatchLeoSnowLionStruct32[0])
           && (Record[1] == mKernelCpuidFindPatchLeoSnowLionStruct32[1])
           && (*((INT32 *)&Record[2]) == (INT32)(BlockClearFunc - (Record + sizeof (mKernelCpuidFindPatchLeoSnowLionStruct32) + sizeof (UINT32)))))
        {
          break;
        }
      }

      if (Index >= EFI_PAGE_SIZE) {
        return EFI_NOT_FOUND;
      }

      StructAddr = *((UINT32 *)(Record - 3));

      for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (  (Record[0] == mKernelCpuidFindPatchLeoSnowLionStart32[0])
           && (Record[1] == mKernelCpuidFindPatchLeoSnowLionStart32[1])
           && (Record[2] == mKernelCpuidFindPatchLeoSnowLionStart32[2])
           && (Record[3] == mKernelCpuidFindPatchLeoSnowLionStart32[3])
           && (Record[4] == mKernelCpuidFindPatchLeoSnowLionStart32[4]))
        {
          break;
        }
      }
    }
  } else {
    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindPatchSnowStart64[0])
         && (Record[1] == mKernelCpuidFindPatchSnowStart64[1])
         && (Record[2] == mKernelCpuidFindPatchSnowStart64[2])
         && (Record[3] == mKernelCpuidFindPatchSnowStart64[3])
         && (Record[4] == mKernelCpuidFindPatchSnowStart64[4])
         && (Record[5] == mKernelCpuidFindPatchSnowStart64[5])
         && (Record[6] == mKernelCpuidFindPatchSnowStart64[6])
         && (Record[7] == mKernelCpuidFindPatchSnowStart64[7]))
      {
        break;
      }
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  if (Patcher->Is32Bit) {
    StartPointer = IsTiger ? Record : Record - 4;
  } else {
    StartPointer = Record + sizeof (mKernelCpuidFindPatchSnowStart64) + sizeof (UINT32);
  }

  //
  // End of patch area.
  //
  STATIC CONST UINT8  mKernelCpuidFindPatchTigerEnd[4] = {
    0x00,                           // 0 from mov byte [...], 0
    0x31, 0xDB,                     // xor ebx, ebx
    0x8B                            // mov ...
  };

  STATIC UINT8   mKernelCpuidFindPatchSnowLionEnd32[7] = {
    0xC6, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0x00    // mov byte [struct_addr], 0
  };
  STATIC UINT32  *mKernelCpuidFindPatchSnowLionEndPtr32 = (UINT32 *)&mKernelCpuidFindPatchSnowLionEnd32[2];

  *mKernelCpuidFindPatchSnowLionEndPtr32 = StructAddr + sizeof (Signature[0]) * 3;

  STATIC CONST UINT8  mKernelCpuidFindPatchLeoEnd1[5] = {
    0xB8, 0x00, 0x00, 0x00, 0x80    // mov eax/edx, 80000000h
  };
  STATIC CONST UINT8  mKernelCpuidFindPatchLeoEnd1Mask = 0xFD;

  if (IsTiger) {
    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindPatchTigerEnd[0])
         && (Record[1] == mKernelCpuidFindPatchTigerEnd[1])
         && (Record[2] == mKernelCpuidFindPatchTigerEnd[2])
         && (Record[3] == mKernelCpuidFindPatchTigerEnd[3]))
      {
        break;
      }
    }
  } else if ((IsSnow || IsLion) && Patcher->Is32Bit) {
    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindPatchSnowLionEnd32[0])
         && (Record[1] == mKernelCpuidFindPatchSnowLionEnd32[1])
         && (Record[2] == mKernelCpuidFindPatchSnowLionEnd32[2])
         && (Record[3] == mKernelCpuidFindPatchSnowLionEnd32[3])
         && (Record[4] == mKernelCpuidFindPatchSnowLionEnd32[4])
         && (Record[5] == mKernelCpuidFindPatchSnowLionEnd32[5])
         && (Record[6] == mKernelCpuidFindPatchSnowLionEnd32[6]))
      {
        break;
      }
    }
  } else {
    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  ((Record[0] & mKernelCpuidFindPatchLeoEnd1Mask) == mKernelCpuidFindPatchLeoEnd1[0])
         && (Record[1] == mKernelCpuidFindPatchLeoEnd1[1])
         && (Record[2] == mKernelCpuidFindPatchLeoEnd1[2])
         && (Record[3] == mKernelCpuidFindPatchLeoEnd1[3])
         && (Record[4] == mKernelCpuidFindPatchLeoEnd1[4]))
      {
        break;
      }
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  if (IsSnow && !Patcher->Is32Bit) {
    STATIC CONST UINT8  mKernelCpuidFindPatchLeoEnd2[3] = {
      0x0F, 0xA2,                   // cpuid
      0x89                          // mov ...
    };

    for ( ; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindPatchLeoEnd2[0])
         && (Record[1] == mKernelCpuidFindPatchLeoEnd2[1])
         && (Record[2] == mKernelCpuidFindPatchLeoEnd2[2]))
      {
        break;
      }
    }

    if (Index >= EFI_PAGE_SIZE) {
      return EFI_NOT_FOUND;
    }
  }

  EndPointer = IsTiger ? Record - 3 : Record;

  //
  // Start of CPUID location.
  //
  // We'll replace this with a call to the previous patched section to
  // populate the CPUID (1) info.
  //
  // 32-bit 10.6 has two different mov eax, 0x1 pairs for 32-bit and 64-bit.
  // The first is patched out with nops, and we'll use the second for the jmp.
  //
  // 32-bit 10.7 has a call to a function that is very similar to 32-bit 10.6, and calls
  // cpuid in 64-bit mode if supported. We can simply replace this call with one to the patched area.
  //
  STATIC CONST UINT8  mKernelCpuidFindLocLeoTigerStart[7] = {
    0xB9, 0x01, 0x00, 0x00, 0x00,       // mov ecx, 1
    0x89, 0xC8                          // mov eax, ecx
  };

  STATIC CONST UINT8  mKernelCpuidFindLocSnowStart[5] = {
    0xB8, 0x01, 0x00, 0x00, 0x00        // mov eax, 1
  };

  STATIC CONST UINT8  mKernelCpuidFindLocSnowStart32[6] = {
    0xE8, 0xFF, 0xFF, 0xFF, 0xFF,       // call _cpuid64
    0xEB                                // jmp ...
  };

  STATIC CONST UINT8  mKernelCpuidFindLocLionStart32[8] = {
    0xB9, 0x01, 0x00, 0x00, 0x00,       // mov ecx, 1
    0x8D, 0x55, 0xD0                    // lea edx, dword [...]
  };

  if (IsTiger || IsLeopard) {
    for ( ; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindLocLeoTigerStart[0])
         && (Record[1] == mKernelCpuidFindLocLeoTigerStart[1])
         && (Record[2] == mKernelCpuidFindLocLeoTigerStart[2])
         && (Record[3] == mKernelCpuidFindLocLeoTigerStart[3])
         && (Record[4] == mKernelCpuidFindLocLeoTigerStart[4])
         && (Record[5] == mKernelCpuidFindLocLeoTigerStart[5])
         && (Record[6] == mKernelCpuidFindLocLeoTigerStart[6]))
      {
        break;
      }
    }
  } else if (IsSnow) {
    for ( ; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindLocSnowStart[0])
         && (Record[1] == mKernelCpuidFindLocSnowStart[1])
         && (Record[2] == mKernelCpuidFindLocSnowStart[2])
         && (Record[3] == mKernelCpuidFindLocSnowStart[3])
         && (Record[4] == mKernelCpuidFindLocSnowStart[4]))
      {
        break;
      }
    }

    if (Index >= EFI_PAGE_SIZE) {
      return EFI_NOT_FOUND;
    }

    //
    // Find where call _cpuid64 is located.
    // We'll then look for the second mov eax, 0x1 after that.
    //
    if (Patcher->Is32Bit) {
      for ( ; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (  (Record[0] == mKernelCpuidFindLocSnowStart32[0])
           && (Record[5] == mKernelCpuidFindLocSnowStart32[5]))
        {
          break;
        }
      }

      if (Index >= EFI_PAGE_SIZE) {
        return EFI_NOT_FOUND;
      }

      LocationSnow32 = Record;

      for ( ; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (  (Record[0] == mKernelCpuidFindLocSnowStart[0])
           && (Record[1] == mKernelCpuidFindLocSnowStart[1])
           && (Record[2] == mKernelCpuidFindLocSnowStart[2])
           && (Record[3] == mKernelCpuidFindLocSnowStart[3])
           && (Record[4] == mKernelCpuidFindLocSnowStart[4]))
        {
          break;
        }
      }
    }
  } else {
    for ( ; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindLocLionStart32[0])
         && (Record[1] == mKernelCpuidFindLocLionStart32[1])
         && (Record[2] == mKernelCpuidFindLocLionStart32[2])
         && (Record[3] == mKernelCpuidFindLocLionStart32[3])
         && (Record[4] == mKernelCpuidFindLocLionStart32[4])
         && (Record[5] == mKernelCpuidFindLocLionStart32[5])
         && (Record[6] == mKernelCpuidFindLocLionStart32[6])
         && (Record[7] == mKernelCpuidFindLocLionStart32[7]))
      {
        break;
      }
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  Location = IsLion ? Record + sizeof (mKernelCpuidFindLocLionStart32) : Record;

  //
  // End of CPUID location. Not applicable to 10.7.
  //
  STATIC CONST UINT8  mKernelCpuidFindLegacyLocEnd[3] = {
    0x0F, 0xA2,                   // cpuid
    0x89                          // mov ...
  };

  if (!IsLion) {
    for ( ; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindLegacyLocEnd[0])
         && (Record[1] == mKernelCpuidFindLegacyLocEnd[1])
         && (Record[2] == mKernelCpuidFindLegacyLocEnd[2]))
      {
        break;
      }
    }

    if (Index >= EFI_PAGE_SIZE) {
      return EFI_NOT_FOUND;
    }

    LocationEnd = Record + 2;
  } else {
    LocationEnd = Location;
  }

  //
  // Locate _tsc_init on 10.4, as there is a CPUID (1) call that needs to be patched.
  // This only applies to XNU 8.10.1 and older. Some recovery versions of
  // 10.4.10 have a newer XNU 8.10.3 kernel with code changes to _tsc_init.
  //
  // It's possible 8.10.2 may require the patch, but there are no sources or kernels
  // available to verify.
  //
  if (IsTigerOld) {
    Status = PatcherGetSymbolAddress (Patcher, "_tsc_init", (UINT8 **)&Record);
    if (EFI_ERROR (Status) || (Record >= Last)) {
      DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate _tsc_init (%p) - %r\n", Record, Status));
      return EFI_NOT_FOUND;
    }

    //
    // Start of _tsc_init CPUID location.
    //
    // We'll replace this with a call to the previous patched section to
    // populate the CPUID (1) info.
    //
    STATIC CONST UINT8  mKernelCpuidFindTscLocTigerStart[7] = {
      0xBA, 0x01, 0x00, 0x00, 0x00,       // mov edx, 1
      0x89, 0xD0                          // mov eax, edx
    };

    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindTscLocTigerStart[0])
         && (Record[1] == mKernelCpuidFindTscLocTigerStart[1])
         && (Record[2] == mKernelCpuidFindTscLocTigerStart[2])
         && (Record[3] == mKernelCpuidFindTscLocTigerStart[3])
         && (Record[4] == mKernelCpuidFindTscLocTigerStart[4])
         && (Record[5] == mKernelCpuidFindTscLocTigerStart[5])
         && (Record[6] == mKernelCpuidFindTscLocTigerStart[6]))
      {
        break;
      }
    }

    if (Index >= EFI_PAGE_SIZE) {
      return EFI_NOT_FOUND;
    }

    LocationTigerTsc = Record;

    //
    // End of _tsc_init CPUID location.
    //
    for ( ; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindLegacyLocEnd[0])
         && (Record[1] == mKernelCpuidFindLegacyLocEnd[1])
         && (Record[2] == mKernelCpuidFindLegacyLocEnd[2]))
      {
        break;
      }
    }

    if (Index >= EFI_PAGE_SIZE) {
      return EFI_NOT_FOUND;
    }

    LocationTigerTscEnd = Record + 2;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCAK: Legacy CPUID patch %p:%p, loc %p:%p, tsc loc %p:%p, struct @ 0x%X\n",
    StartPointer - Start,
    EndPointer - Start,
    Location - Start,
    LocationEnd - Start,
    IsTigerOld ? LocationTigerTsc - Start : 0,
    IsTigerOld ? LocationTigerTscEnd - Start : 0,
    StructAddr
    ));

  //
  // Free 2 more bytes in the end by assigning EAX directly.
  //
  if (IsSnow && !Patcher->Is32Bit) {
    AsmCpuid (0x80000000, &MaxExt, NULL, NULL, NULL);
    EndPointer[0] = 0xB8;
    CopyMem (&EndPointer[1], &MaxExt, sizeof (MaxExt));
  }

  //
  // Short-write CPU signature to create space for CPUID (1) patch.
  //
  // 29 bytes.
  //
  AsmCpuid (0, NULL, &Signature[0], &Signature[2], &Signature[1]);
  for (Index = 0; Index < 3; ++Index) {
    //
    // mov eax, <signature>
    //
    *StartPointer++ = 0xB8;
    CopyMem (StartPointer, &Signature[Index], sizeof (Signature[0]));
    StartPointer += sizeof (Signature[0]);

    if (IsLeopard || ((IsSnow || IsLion) && Patcher->Is32Bit)) {
      //
      // mov dword [struct_addr], eax
      //
      *StartPointer++ = 0xA3;
      CopyMem (StartPointer, &StructAddr, sizeof (StructAddr));
      StartPointer += sizeof (StructAddr);
      StructAddr   += sizeof (Signature[0]);
    } else if (IsTiger) {
      //
      // mov [esi+offset], eax
      //
      *StartPointer++ = 0x89;
      *StartPointer++ = 0x46;
      *StartPointer++ = (UINT8)(Index * sizeof (Signature[0]));
    } else {
      //
      // mov [rsi], eax
      //
      *StartPointer++ = 0x89;
      *StartPointer++ = 0x06;

      if (Index < 2) {
        //
        // add rsi, 4
        //
        *StartPointer++ = 0x48;
        *StartPointer++ = 0x83;
        *StartPointer++ = 0xC6;
        *StartPointer++ = 0x04;
      }
    }
  }

  //
  // Ensure that we still have room, which is within 2-byte jmp (127)
  // and has at least 2-byte jmp and patch + 1-byte ret.
  //
  if (  (StartPointer >= EndPointer)
     || (EndPointer - StartPointer > 128)
     || ((UINTN)(EndPointer - StartPointer) < sizeof (INTERNAL_CPUID_PATCH) + 3))
  {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Short jmp to EndPointer.
  //
  StartPointer[0] = 0xEB;
  StartPointer[1] = (UINT8)(EndPointer - StartPointer - 2);
  StartPointer   += 2;

  //
  // Workaround incorrect OSXSAVE handling, see below.
  //
  if (  (CpuInfo->CpuidVerEcx.Bits.XSAVE != 0)
     && (CpuInfo->CpuidVerEcx.Bits.OSXSAVE == 0)
     && (CpuInfo->CpuidVerEcx.Bits.AVX != 0))
  {
    CpuInfo->CpuidVerEcx.Bits.OSXSAVE = 1;
  }

  //
  // NOP out call _cpuid64 in 32-bit 10.6.
  //
  if (IsSnow && Patcher->Is32Bit) {
    while (LocationSnow32 < LocationEnd) {
      *LocationSnow32++ = 0x90;
    }
  }

  //
  // Call from location to patch area.
  //
  Delta       = (INT32)(StartPointer - (Location + 5));
  *Location++ = 0xE8;
  CopyMem (Location, &Delta, sizeof (Delta));
  Location += sizeof (Delta);
  while (Location < LocationEnd) {
    *Location++ = 0x90;
  }

  //
  // In 10.4, we need to replace a call to CPUID (1) with a call to
  // the patch area like above in _tsc_init.
  //
  if (IsTigerOld) {
    Delta               = (INT32)(StartPointer - (LocationTigerTsc + 5));
    *LocationTigerTsc++ = 0xE8;
    CopyMem (LocationTigerTsc, &Delta, sizeof (Delta));
    LocationTigerTsc += sizeof (Delta);
    while (LocationTigerTsc < LocationTigerTscEnd) {
      *LocationTigerTsc++ = 0x90;
    }
  }

  //
  // Write virtualised CPUID.
  // 20 bytes.
  // 10.7 requires the registers to be copied to a memory location in EDX, so we'll use ESI instead.
  //
  Patch.EaxCmd = 0xB8;
  Patch.EaxVal = (Data[0] & DataMask[0]) | (CpuInfo->CpuidVerEax.Uint32 & ~DataMask[0]);
  Patch.EbxCmd = 0xBB;
  Patch.EbxVal = (Data[1] & DataMask[1]) | (CpuInfo->CpuidVerEbx.Uint32 & ~DataMask[1]);
  Patch.EcxCmd = 0xB9;
  Patch.EcxVal = (Data[2] & DataMask[2]) | (CpuInfo->CpuidVerEcx.Uint32 & ~DataMask[2]);
  Patch.EdxCmd = IsLion ? 0xBE : 0xBA;
  Patch.EdxVal = (Data[3] & DataMask[3]) | (CpuInfo->CpuidVerEdx.Uint32 & ~DataMask[3]);
  CopyMem (StartPointer, &Patch, sizeof (Patch));
  StartPointer += sizeof (Patch);

  //
  // Under 10.7, we need to copy registers to memory in EDX.
  //
  if (IsLion) {
    //
    // mov [edx], eax
    //
    *StartPointer++ = 0x89;
    *StartPointer++ = 0x02;
    //
    // mov [edx+4], ebx
    //
    *StartPointer++ = 0x89;
    *StartPointer++ = 0x5A;
    *StartPointer++ = 0x04;
    //
    // mov [edx+8], ecx
    //
    *StartPointer++ = 0x89;
    *StartPointer++ = 0x4A;
    *StartPointer++ = 0x08;
    //
    // mov [edx+12], esi
    //
    *StartPointer++ = 0x89;
    *StartPointer++ = 0x72;
    *StartPointer++ = 0x0C;
  }

  //
  // Return to the end of location.
  //
  *StartPointer++ = 0xC3;

  DEBUG ((DEBUG_INFO, "OCAK: [OK] Legacy CPUID patch completed @ %p\n", StartPointer - Start));
  return EFI_SUCCESS;
}

EFI_STATUS
PatchKernelCpuId (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     OC_CPU_INFO      *CpuInfo,
  IN     UINT32           *Data,
  IN     UINT32           *DataMask,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS                Status;
  UINT8                     *CpuidSetInfo;
  UINT8                     *Record;
  UINT8                     *Start;
  UINT8                     *Last;
  UINT32                    Index;
  UINT32                    FoundSize;
  INTERNAL_CPUID_PATCH      *CpuidPatch;
  INTERNAL_MICROCODE_PATCH  *McPatch;
  INTERNAL_CPUID_FN_PATCH   *FnPatch;
  CPUID_VERSION_INFO_EAX    Eax;
  CPUID_VERSION_INFO_EBX    Ebx;
  CPUID_VERSION_INFO_ECX    Ecx;
  CPUID_VERSION_INFO_EDX    Edx;
  BOOLEAN                   FoundReleaseKernel;

  ASSERT (Patcher != NULL);

  STATIC_ASSERT (
    sizeof (mKernelCpuIdFindRelNew) > sizeof (mKernelCpuIdFindRelOld),
    "Kernel CPUID patch seems wrong"
    );

  ASSERT (
    mKernelCpuIdFindRelNew[0] == mKernelCpuIdFindRelOld[0]
         && mKernelCpuIdFindRelNew[1] == mKernelCpuIdFindRelOld[1]
         && mKernelCpuIdFindRelNew[2] == mKernelCpuIdFindRelOld[2]
         && mKernelCpuIdFindRelNew[3] == mKernelCpuIdFindRelOld[3]
    );

  Start = ((UINT8 *)MachoGetMachHeader (&Patcher->MachContext));
  Last  = Start + MachoGetInnerSize (&Patcher->MachContext) - EFI_PAGE_SIZE * 2 - sizeof (mKernelCpuIdFindRelNew);

  //
  // Do legacy patching for 32-bit 10.7, and 10.6 and older.
  //
  if (  (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LION_MIN, KERNEL_VERSION_LION_MAX) && Patcher->Is32Bit)
     || OcMatchDarwinVersion (KernelVersion, 0, KERNEL_VERSION_SNOW_LEOPARD_MAX))
  {
    Status = PatchKernelCpuIdLegacy (Patcher, KernelVersion, CpuInfo, Data, DataMask, Start, Last);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to patch legacy CPUID - %r\n", Status));
    }

    return Status;
  }

  Status = PatcherGetSymbolAddress (Patcher, "_cpuid_set_info", (UINT8 **)&CpuidSetInfo);
  if (EFI_ERROR (Status) || (CpuidSetInfo >= Last)) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate _cpuid_set_info (%p) - %r\n", CpuidSetInfo, Status));
    return EFI_NOT_FOUND;
  }

  Record    = CpuidSetInfo;
  FoundSize = 0;

  for (Index = 0; Index < EFI_PAGE_SIZE; ++Index, ++Record) {
    if (  (Record[0] == mKernelCpuIdFindRelNew[0])
       && (Record[1] == mKernelCpuIdFindRelNew[1])
       && (Record[2] == mKernelCpuIdFindRelNew[2])
       && (Record[3] == mKernelCpuIdFindRelNew[3]))
    {
      if (CompareMem (Record, mKernelCpuIdFindRelNew, sizeof (mKernelCpuIdFindRelNew)) == 0) {
        FoundSize = sizeof (mKernelCpuIdFindRelNew);
        break;
      } else if (CompareMem (Record, mKernelCpuIdFindRelOld, sizeof (mKernelCpuIdFindRelOld)) == 0) {
        FoundSize = sizeof (mKernelCpuIdFindRelOld);
        break;
      }
    }
  }

  FoundReleaseKernel = FoundSize > 0;

  //
  // When patching the release kernel we do not allow reevaluating CPUID information,
  // which is used to report OSXSAVE availability. This causes issues with some programs,
  // like Docker using Hypervisor.framework, which rely on sysctl to track CPU feature.
  //
  // To workaround this we make sure to always report OSXSAVE bit when it is available
  // regardless of the reevaluation performed by init_fpu in XNU.
  //
  // REF: https://github.com/acidanthera/bugtracker/issues/1035
  //
  if (  FoundReleaseKernel
     && (CpuInfo->CpuidVerEcx.Bits.XSAVE != 0)
     && (CpuInfo->CpuidVerEcx.Bits.OSXSAVE == 0)
     && (CpuInfo->CpuidVerEcx.Bits.AVX != 0))
  {
    CpuInfo->CpuidVerEcx.Bits.OSXSAVE = 1;
  }

  Eax.Uint32 = (Data[0] & DataMask[0]) | (CpuInfo->CpuidVerEax.Uint32 & ~DataMask[0]);
  Ebx.Uint32 = (Data[1] & DataMask[1]) | (CpuInfo->CpuidVerEbx.Uint32 & ~DataMask[1]);
  Ecx.Uint32 = (Data[2] & DataMask[2]) | (CpuInfo->CpuidVerEcx.Uint32 & ~DataMask[2]);
  Edx.Uint32 = (Data[3] & DataMask[3]) | (CpuInfo->CpuidVerEdx.Uint32 & ~DataMask[3]);

  if (FoundReleaseKernel) {
    CpuidPatch         = (INTERNAL_CPUID_PATCH *)Record;
    CpuidPatch->EaxCmd = 0xB8;
    CpuidPatch->EaxVal = Eax.Uint32;
    CpuidPatch->EbxCmd = 0xBB;
    CpuidPatch->EbxVal = Ebx.Uint32;
    CpuidPatch->EcxCmd = 0xB9;
    CpuidPatch->EcxVal = Ecx.Uint32;
    CpuidPatch->EdxCmd = 0xBA;
    CpuidPatch->EdxVal = Edx.Uint32;
    SetMem (
      Record + sizeof (INTERNAL_CPUID_PATCH),
      FoundSize - sizeof (INTERNAL_CPUID_PATCH),
      0x90
      );
    Record += FoundSize;

    for (Index = 0; Index < EFI_PAGE_SIZE - sizeof (mKernelCpuidFindMcRel); ++Index, ++Record) {
      if (CompareMem (Record, mKernelCpuidFindMcRel, sizeof (mKernelCpuidFindMcRel)) == 0) {
        McPatch         = (INTERNAL_MICROCODE_PATCH *)Record;
        McPatch->EdxCmd = 0xBA;
        McPatch->EdxVal = CpuInfo->MicrocodeRevision;
        SetMem (
          Record + sizeof (INTERNAL_MICROCODE_PATCH),
          sizeof (mKernelCpuidFindMcRel) - sizeof (INTERNAL_MICROCODE_PATCH),
          0x90
          );

        DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success CPUID release\n"));
        return EFI_SUCCESS;
      }
    }
  } else {
    //
    // Handle debug kernel here...
    //
    Status = PatcherGetSymbolAddress (Patcher, "_cpuid_set_cpufamily", (UINT8 **)&Record);
    if (EFI_ERROR (Status) || (Record >= Last)) {
      DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate _cpuid_set_cpufamily (%p) - %r\n", Record, Status));
      return EFI_NOT_FOUND;
    }

    CopyMem (Record, mKernelCpuidReplaceDbg, sizeof (mKernelCpuidReplaceDbg));
    FnPatch            = (INTERNAL_CPUID_FN_PATCH *)Record;
    FnPatch->Signature = Eax.Uint32;
    FnPatch->Stepping  = (UINT8)Eax.Bits.SteppingId;
    FnPatch->ExtModel  = (UINT8)Eax.Bits.ExtendedModelId;
    FnPatch->Model     = (UINT8)Eax.Bits.Model | (UINT8)(Eax.Bits.ExtendedModelId << 4U);
    FnPatch->Family    = (UINT8)Eax.Bits.FamilyId;
    FnPatch->Type      = (UINT8)Eax.Bits.ProcessorType;
    FnPatch->ExtFamily = (UINT8)Eax.Bits.ExtendedFamilyId;
    FnPatch->Features  = LShiftU64 (Ecx.Uint32, 32) | (UINT64)Edx.Uint32;
    if (FnPatch->Features & CPUID_FEATURE_HTT) {
      FnPatch->LogicalPerPkg = (UINT16)Ebx.Bits.MaximumAddressableIdsForLogicalProcessors;
    } else {
      FnPatch->LogicalPerPkg = 1;
    }

    FnPatch->AppleFamily1 = FnPatch->AppleFamily2 = OcCpuModelToAppleFamily (Eax);

    DEBUG ((DEBUG_INFO, "OCAK: [OK] Patch success CPUID debug\n"));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to find either CPUID patch (%u)\n", FoundSize));

  return EFI_UNSUPPORTED;
}

STATIC
UINT8
  mProvideCurrentCpuInfoTopologyValidationReplace[] = {
  // ret
  0xC3
};

STATIC
PATCHER_GENERIC_PATCH
  mProvideCurrentCpuInfoTopologyValidationPatch = {
  .Comment     = DEBUG_POINTER ("ProvideCurrentCpuInfoTopologyValidation"),
  .Base        = "_x86_validate_topology",
  .Find        = NULL,
  .Mask        = NULL,
  .Replace     = mProvideCurrentCpuInfoTopologyValidationReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mProvideCurrentCpuInfoTopologyValidationReplace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

// Offset of value in below patch.
#define CURRENT_CPU_INFO_CORE_COUNT_OFFSET  1

STATIC
UINT8
  mProvideCurrentCpuInfoZeroMsrThreadCoreCountFind[] = {
  // mov ecx, 0x10001
  0xB9, 0x01, 0x00, 0x01, 0x00
};

STATIC
UINT8
  mProvideCurrentCpuInfoZeroMsrThreadCoreCountReplace[] = {
  // mov ecx, core/thread count
  0xB9, 0x00, 0x00, 0x00, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
  mProvideCurrentCpuInfoZeroMsrThreadCoreCountPatch = {
  .Comment     = DEBUG_POINTER ("ProvideCurrentCpuInfoZeroMsrThreadCoreCount"),
  .Base        = "_cpuid_set_info",
  .Find        = mProvideCurrentCpuInfoZeroMsrThreadCoreCountFind,
  .Mask        = NULL,
  .Replace     = mProvideCurrentCpuInfoZeroMsrThreadCoreCountReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mProvideCurrentCpuInfoZeroMsrThreadCoreCountFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
UINT8 *
PatchMovVar (
  IN OUT  UINT8    *Location,
  IN      BOOLEAN  Is32Bit,
  IN OUT  UINT64   *FuncAddr,
  IN      UINT64   VarSymAddr,
  IN      UINT64   Value
  )
{
  UINT8   *Start;
  INT32   Delta;
  UINT32  VarAddr32;
  UINT32  ValueLower;
  UINT32  ValueUpper;

  Start = Location;
  DEBUG ((DEBUG_VERBOSE, "OCAK: Current TSC func address: 0x%llX, variable address: 0x%llX\n", *FuncAddr, VarSymAddr));

  if (Is32Bit) {
    ValueLower = (UINT32)Value;
    ValueUpper = (UINT32)(Value >> 32);

    //
    // 32-bit uses absolute addressing
    //
    // mov [var], value lower
    //
    VarAddr32   = (UINT32)(VarSymAddr);
    *Location++ = 0xC7;
    *Location++ = 0x05;
    CopyMem (Location, &VarAddr32, sizeof (VarAddr32));
    Location += sizeof (VarAddr32);
    CopyMem (Location, &ValueLower, sizeof (ValueLower));
    Location += sizeof (ValueLower);

    //
    // mov [var+4], value upper
    //
    VarAddr32  += sizeof (UINT32);
    *Location++ = 0xC7;
    *Location++ = 0x05;
    CopyMem (Location, &VarAddr32, sizeof (VarAddr32));
    Location += sizeof (VarAddr32);
    CopyMem (Location, &ValueUpper, sizeof (ValueUpper));
    Location += sizeof (ValueUpper);

    *FuncAddr += (Location - Start);
  } else {
    //
    // mov rax, value
    //
    *Location++ = 0x48;
    *Location++ = 0xB8;
    CopyMem (Location, &Value, sizeof (Value));
    Location  += sizeof (Value);
    *FuncAddr += sizeof (Value) + 2;

    //
    // mov [var], rax
    //
    Delta = (INT32)(VarSymAddr - (*FuncAddr + 7));
    DEBUG ((DEBUG_VERBOSE, "OCAK: TSC func delta 0x%X\n", Delta));
    *Location++ = 0x48;
    *Location++ = 0x89;
    *Location++ = 0x05;
    CopyMem (Location, &Delta, sizeof (Delta));
    Location  += sizeof (Delta);
    *FuncAddr += sizeof (Delta) + 3;
  }

  return Location;
}

// 10.14:
// 44 89 E8                 mov        eax, r13d
// C1 E8 1A                 shr        eax, 0x1a
// FF C0                    inc        eax
// 89 05 68 60 98 00        mov        dword [dword_ffffff8000e4b1c8], eax

// 10.15
// 44 89 EA                 mov        edx, r13d
// C1 EA 1A                 shr        edx, 26
// FF C2                    inc        edx
// 89 15 2F 55 A2 00        mov        dword [dword_FFFFFF8000E551D8], edx

// 11.0.1
// 44 89 EA                 mov        edx, r13d
// C1 EA 1A                 shr        edx, 0x1a
// FF C2                    inc        edx
// 89 15 9F 9C A8 00        mov        dword [dword_ffffff8000e4b1d8], edx

// 12.0.1
// 44 89 EA                 mov        edx, r13d
// C1 EA 1A                 shr        edx, 0x1a
// 83 C2 01                 add        edx, 0x1
// 89 15 DD F9 AA 00        mov        dword [dword_ffffff8000e6e1d8], edx

STATIC
CONST UINT8
  mProvideCurrentCpuInfoTopologyCoreCountFind[] = {
  0xB9, 0x35, 0x00, 0x00, 0x00, 0x0F, 0x32
};

STATIC
UINT8
  mProvideCurrentCpuInfoTopologyCoreCountReplace[] = {
  0xB8, 0x00, 0x00, 0x00, 0x00, 0x31, 0xD2
};

STATIC
PATCHER_GENERIC_PATCH
  mProvideCurrentCpuInfoTopologyCoreCountPatch = {
  .Comment     = DEBUG_POINTER ("Set core count to thread count"),
  .Base        = "_cpuid_set_info",
  .Find        = mProvideCurrentCpuInfoTopologyCoreCountFind,
  .Mask        = NULL,
  .Replace     = mProvideCurrentCpuInfoTopologyCoreCountReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mProvideCurrentCpuInfoTopologyCoreCountReplace),
  .Count       = 2,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoTopologyCorePerPackageV1Find[] = {
  // mov  eax, r13d
  // shr  eax, 0x1a    <--- start
  // inc  eax
  // mov  dword[], eax
  0xC1, 0xE8, 0x1A, 0xFF, 0xC0, 0x89
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoTopologyCorePerPackageV1Replace[] = {
  // mov  eax, r13d
  // mov  eax, 0x80    <--- start
  // mov  dword[], eax
  0xB8, 0x80, 0x00, 0x00, 0x00, 0x89
};

STATIC
PATCHER_GENERIC_PATCH
  mProvideCurrentCpuInfoTopologyCorePerPackageV1Patch = {
  .Comment     = DEBUG_POINTER ("Set core per package count to 128 V1"),
  .Base        = NULL,
  .Find        = mProvideCurrentCpuInfoTopologyCorePerPackageV1Find,
  .Mask        = NULL,
  .Replace     = mProvideCurrentCpuInfoTopologyCorePerPackageV1Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mProvideCurrentCpuInfoTopologyCorePerPackageV1Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoTopologyCorePerPackageV1_5Find[] = {
  // mov  edx, r13d
  // shr  edx, 0x1a    <--- start
  // inc  edx
  // mov  dword[], edx
  0xC1, 0xEA, 0x1A, 0xFF, 0xC2, 0x89
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoTopologyCorePerPackageV1_5Replace[] = {
  // mov  edx, r13d
  // mov  edx, 0x80    <--- start
  // mov  dword[], edx
  0xBA, 0x80, 0x00, 0x00, 0x00, 0x89
};

STATIC
PATCHER_GENERIC_PATCH
  mProvideCurrentCpuInfoTopologyCorePerPackageV1_5Patch = {
  .Comment     = DEBUG_POINTER ("Set core per package count to 128 V1_5"),
  .Base        = NULL,
  .Find        = mProvideCurrentCpuInfoTopologyCorePerPackageV1_5Find,
  .Mask        = NULL,
  .Replace     = mProvideCurrentCpuInfoTopologyCorePerPackageV1_5Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mProvideCurrentCpuInfoTopologyCorePerPackageV1_5Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoTopologyCorePerPackageV2Find[] = {
  // mov  eax/edx, r13d
  // shr  eax/edx, 0x1a    <--- start
  // add  eax/edx, 1
  // mov  dword[], eax/edx
  0xC1, 0xE8 /* 0xEA */, 0x1A, 0x83, 0xC0 /* 0xC2 */, 0x01, 0x89
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoTopologyCorePerPackageV2Mask[] = {
  0xFF, 0xFD, 0xFF, 0xFF, 0xFD, 0xFF, 0xFF
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoTopologyCorePerPackageV2Replace[] = {
  // mov  eax/edx, r13d
  // shr  eax/edx, 0x1a    <--- start
  // nop
  // mov  al/dl, 128
  // mov  dword[], eax/edx
  0xC1, 0xE8 /* 0xEA */, 0x1A, 0x90, 0xB0 /* 0xB2 */, 0x80, 0x89
};

STATIC
PATCHER_GENERIC_PATCH
  mProvideCurrentCpuInfoTopologyCorePerPackageV2Patch = {
  .Comment     = DEBUG_POINTER ("Set core per package count to 128 V2"),
  .Base        = NULL,
  .Find        = mProvideCurrentCpuInfoTopologyCorePerPackageV2Find,
  .Mask        = mProvideCurrentCpuInfoTopologyCorePerPackageV2Mask,
  .Replace     = mProvideCurrentCpuInfoTopologyCorePerPackageV2Replace,
  .ReplaceMask = mProvideCurrentCpuInfoTopologyCorePerPackageV2Mask,
  .Size        = sizeof (mProvideCurrentCpuInfoTopologyCorePerPackageV2Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
EFI_STATUS
PatchProvideCurrentCpuInfoMSR35h (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     OC_CPU_INFO      *CpuInfo,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;
  UINT32      CoreThreadCount;
  BOOLEAN     IsAmpCpu;

  //
  // TODO: We can support older, just there is no real need.
  // Anyone can test/contribute as needed.
  //
  if (KernelVersion < KERNEL_VERSION_MOJAVE_MIN) {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Ignoring CPU INFO for AMP below macOS 10.14\n"));
    return EFI_SUCCESS;
  }

  IsAmpCpu = (CpuInfo->ThreadCount % CpuInfo->CoreCount) != 0;

  //
  // Provide real values for normal CPUs.
  // Provide Thread=Thread for AMP (ADL) CPUs.
  //
  if (IsAmpCpu) {
    CoreThreadCount =
      (((UINT32)CpuInfo->ThreadCount) << 16U) | ((UINT32)CpuInfo->ThreadCount);
  } else {
    CoreThreadCount =
      (((UINT32)CpuInfo->CoreCount) << 16U) | ((UINT32)CpuInfo->ThreadCount);
  }

  CopyMem (
    &mProvideCurrentCpuInfoTopologyCoreCountReplace[1],
    &CoreThreadCount,
    sizeof (UINT32)
    );

  Status = PatcherApplyGenericPatch (
             Patcher,
             &mProvideCurrentCpuInfoTopologyCoreCountPatch
             );

  DEBUG ((DEBUG_INFO, "OCAK: Patching MSR 35h to %08x - %r\n", CoreThreadCount, Status));

  if (EFI_ERROR (Status) || !IsAmpCpu) {
    return Status;
  }

  if (KernelVersion >= KERNEL_VERSION_MONTEREY_MIN) {
    Status = PatcherApplyGenericPatch (
               Patcher,
               &mProvideCurrentCpuInfoTopologyCorePerPackageV2Patch
               );
  } else {
    Status = PatcherApplyGenericPatch (
               Patcher,
               &mProvideCurrentCpuInfoTopologyCorePerPackageV1Patch
               );
  }

  if (EFI_ERROR (Status)) {
    Status = PatcherApplyGenericPatch (
               Patcher,
               &mProvideCurrentCpuInfoTopologyCorePerPackageV1_5Patch
               );
  }

  DEBUG ((DEBUG_INFO, "OCAK: Patching core per package count - %r\n", Status));

  return Status;
}

STATIC
CONST UINT8
  mProvideCurrentCpuInfoLeopardSysctlMibInitFind1[] = {
  0xE8, 0x00, 0x00, 0x00, 0x00,            // call _ml_cpu_cache_sharing / _ml_cpu_cache_size
  0xC7, 0x04, 0x24, 0x02, 0x00, 0x00, 0x00 // mov dword ptr [esp], 0x2/0x3
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoLeopardSysctlMibInitMask1[] = {
  0xFF, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFE, 0xFF,0xFF, 0xFF
};

STATIC
UINT8
  mProvideCurrentCpuInfoLeopardSysctlMibInitReplace1[] = {
  0xB8, 0x00, 0x00, 0x00, 0x00, // mov eax, (UINT32 value for func param 1 or 2)
  0x31, 0xD2,                   // xor edx, edx
  0x90,                         // nop
  0x90,                         // nop
  0x90,                         // nop
  0x90,                         // nop
  0x90                          // nop
};

STATIC
PATCHER_GENERIC_PATCH
  mProvideCurrentCpuInfoLeopardSysctlMibInitPatch1 = {
  .Comment     = DEBUG_POINTER ("_sysctl_mib_init Leopard patch 1"),
  .Base        = "_sysctl_mib_init",
  .Find        = mProvideCurrentCpuInfoLeopardSysctlMibInitFind1,
  .Mask        = mProvideCurrentCpuInfoLeopardSysctlMibInitMask1,
  .Replace     = mProvideCurrentCpuInfoLeopardSysctlMibInitReplace1,
  .ReplaceMask = NULL,
  .Size        = sizeof (mProvideCurrentCpuInfoLeopardSysctlMibInitReplace1),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoLeopardSysctlMibInitFind2[] = {
  0xE8, 0x00, 0x00, 0x00, 0x00,            // call _ml_cpu_cache_sharing / _ml_cpu_cache_size
  0xC7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00 // mov dword ptr [esp], 0x0
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoLeopardSysctlMibInitMask2[] = {
  0xFF, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF
};

STATIC
UINT8
  mProvideCurrentCpuInfoLeopardSysctlMibInitReplace2[] = {
  0xB8, 0x00, 0x00, 0x00, 0x00,            // mov eax, (UINT32 value for func param = 3)
  0xC7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00 // mov dword ptr [esp], 0x0
};

STATIC
PATCHER_GENERIC_PATCH
  mProvideCurrentCpuInfoLeopardSysctlMibInitPatch2 = {
  .Comment     = DEBUG_POINTER ("_sysctl_mib_init Leopard patch 2"),
  .Base        = "_sysctl_mib_init",
  .Find        = mProvideCurrentCpuInfoLeopardSysctlMibInitFind2,
  .Mask        = mProvideCurrentCpuInfoLeopardSysctlMibInitMask2,
  .Replace     = mProvideCurrentCpuInfoLeopardSysctlMibInitReplace2,
  .ReplaceMask = NULL,
  .Size        = sizeof (mProvideCurrentCpuInfoLeopardSysctlMibInitReplace2),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoSnowLeopardSysctlMibInitFind[] = {
  0xA3, 0x00, 0x00, 0x00, 0x00,             // mov dword [...], eax
  0x89, 0x15, 0x00, 0x00, 0x00, 0x00,       // mov dword [...], edx
  0xC7, 0x04, 0x24, 0x00, 0x00, 0x00, 0x00, // mov dword ptr [esp], 0x2/0x3
  0xE8, 0x00, 0x00, 0x00, 0x00              // call _ml_cpu_cache_sharing / _ml_cpu_cache_size
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoSnowLeopardSysctlMibInitFindMask[] = {
  0xFF, 0x00, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0x00, 0x00, 0x00,0x00,
  0xFF, 0xFF, 0xFF, 0x00, 0x00,0x00, 0x00,
  0xFF, 0x00, 0x00, 0x00, 0x00
};

STATIC
UINT8
  mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplace[] = {
  0xA3, 0x00, 0x00, 0x00, 0x00,       // mov dword [...], eax
  0x89, 0x15, 0x00, 0x00, 0x00, 0x00, // mov dword [...], edx
  0xB8, 0x00, 0x00, 0x00, 0x00,       // mov eax, (UINT32 value for func param 1 or 2)
  0x31, 0xD2,                         // xor edx, edx
  0x90,                               // nop
  0x90,                               // nop
  0x90,                               // nop
  0x90,                               // nop
  0x90                                // nop
};

STATIC
CONST UINT8
  mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplaceMask[] = {
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF,
  0xFF,
  0xFF,
  0xFF,
  0xFF,
  0xFF
};

STATIC
PATCHER_GENERIC_PATCH
  mProvideCurrentCpuInfoSnowLeopardSysctlMibInitPatch = {
  .Comment     = DEBUG_POINTER ("_sysctl_mib_init Snow Leopard patch"),
  .Base        = "_sysctl_mib_init",
  .Find        = mProvideCurrentCpuInfoSnowLeopardSysctlMibInitFind,
  .Mask        = mProvideCurrentCpuInfoSnowLeopardSysctlMibInitFindMask,
  .Replace     = mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplace,
  .ReplaceMask = mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplaceMask,
  .Size        = sizeof (mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

EFI_STATUS
PatchProvideCurrentCpuInfo (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     OC_CPU_INFO      *CpuInfo,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS  Status;

  INT32   Delta;
  UINT32  VarAddr32;
  UINT32  ValueLower;
  UINT32  ValueUpper;
  UINT8   *Record;
  UINT8   *Start;
  UINT8   *Last;
  UINT32  Index;
  UINT32  Index2;

  UINT8  *TscInitFunc;
  UINT8  *TmrCvtFunc;

  UINT64  TscInitFuncSymAddr;
  UINT64  BusFreqSymAddr;
  UINT64  BusFCvtt2nSymAddr;
  UINT64  BusFCvtn2tSymAddr;
  UINT64  TscFreqSymAddr;
  UINT64  TscFCvtt2nSymAddr;
  UINT64  TscFCvtn2tSymAddr;
  UINT64  TscGranularitySymAddr;
  UINT64  Bus2TscSymAddr;

  UINT8  *TscLocation;

  UINT64  busFreqValue;
  UINT64  busFCvtt2nValue;
  UINT64  busFCvtn2tValue;
  UINT64  tscFreqValue;
  UINT64  tscFCvtt2nValue;
  UINT64  tscFCvtn2tValue;
  UINT64  tscGranularityValue;

  BOOLEAN  IsLeaf4CacheSupported;

  BOOLEAN  IsTiger;
  BOOLEAN  IsLeopard;
  BOOLEAN  IsSnowLeopard;
  BOOLEAN  IsTigerCacheUnsupported;
  UINT8    *LocationTigerCache;
  UINT8    *LocationTigerCacheEnd;

  APPLE_INTEL_CPU_CACHE_TYPE  CacheType;
  CPUID_CACHE_PARAMS_EAX      CpuidCacheParamsEax;
  CPUID_CACHE_PARAMS_EBX      CpuidCacheParamsEbx;
  CPUID_CACHE_INFO_CACHE_TLB  CpuidCacheInfo[4];
  UINT8                       CpuidCacheDescriptors[64];
  UINT32                      CacheSets;
  UINT32                      CacheSizes[LCACHE_MAX];
  UINT32                      CacheSize;
  UINT32                      CacheLineSizes[LCACHE_MAX];
  UINT32                      CacheLineSize;
  UINT32                      CacheSharings[LCACHE_MAX];
  BOOLEAN                     CacheDescriptorFound;
  cache_type_t                CacheDescriptorType;
  UINT32                      CacheDescriptorSize;

  UINT32  msrCoreThreadCount;

  ASSERT (Patcher != NULL);

  LocationTigerCache    = NULL;
  LocationTigerCacheEnd = NULL;

  Start = ((UINT8 *)MachoGetMachHeader (&Patcher->MachContext));
  Last  = Start + MachoGetInnerSize (&Patcher->MachContext) - EFI_PAGE_SIZE * 2;

  IsLeaf4CacheSupported = CpuInfo->MaxId >= CPUID_CACHE_PARAMS;

  IsTiger       = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_TIGER_MIN, KERNEL_VERSION_TIGER_MAX);
  IsLeopard     = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LEOPARD_MIN, KERNEL_VERSION_LEOPARD_MAX);
  IsSnowLeopard = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_SNOW_LEOPARD_MIN, KERNEL_VERSION_SNOW_LEOPARD_MAX);

  //
  // 10.4 does not support pulling CPUID leaf 4 that may contain cache info instead of leaf 2.
  // On processors that support leaf 4, use that instead.
  //
  IsTigerCacheUnsupported = IsTiger && IsLeaf4CacheSupported;

  Status  = EFI_SUCCESS;
  Status |= PatchProvideCurrentCpuInfoMSR35h (Patcher, CpuInfo, KernelVersion);

  //
  // Pull required symbols.
  //
  Status |= PatcherGetSymbolAddressValue (Patcher, "_tsc_init", (UINT8 **)&TscInitFunc, &TscInitFuncSymAddr);
  Status |= PatcherGetSymbolAddress (Patcher, "_tmrCvt", (UINT8 **)&TmrCvtFunc);

  //
  // _busFreq only exists on 10.5 and higher.
  //
  if (!IsTiger) {
    Status |= PatcherGetSymbolValue (Patcher, "_busFreq", &BusFreqSymAddr);
  }

  Status |= PatcherGetSymbolValue (Patcher, "_busFCvtt2n", &BusFCvtt2nSymAddr);
  Status |= PatcherGetSymbolValue (Patcher, "_busFCvtn2t", &BusFCvtn2tSymAddr);
  Status |= PatcherGetSymbolValue (Patcher, "_tscFreq", &TscFreqSymAddr);
  Status |= PatcherGetSymbolValue (Patcher, "_tscFCvtt2n", &TscFCvtt2nSymAddr);
  Status |= PatcherGetSymbolValue (Patcher, "_tscFCvtn2t", &TscFCvtn2tSymAddr);
  Status |= PatcherGetSymbolValue (Patcher, "_tscGranularity", &TscGranularitySymAddr);
  Status |= PatcherGetSymbolValue (Patcher, "_bus2tsc", &Bus2TscSymAddr);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate one or more TSC symbols - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  //
  // Perform TSC and FSB calculations. This is traditionally done in tsc.c in XNU.
  //
  // For AMD Processors
  if ((CpuInfo->Family == 0xF) && ((CpuInfo->ExtFamily == 0x8) || (CpuInfo->ExtFamily == 0xA))) {
    DEBUG ((DEBUG_INFO, "OCAK: Setting FSB and TSC for Family 0x%x and ExtFamily 0x%x\n", (UINT16)CpuInfo->Family, (UINT16)CpuInfo->ExtFamily));
    busFreqValue    = CpuInfo->FSBFrequency;
    busFCvtt2nValue = DivU64x64Remainder ((1000000000ULL << 32), busFreqValue, NULL);
    busFCvtn2tValue = DivU64x64Remainder ((1000000000ULL << 32), busFCvtt2nValue, NULL);

    tscFreqValue    = CpuInfo->CPUFrequency;
    tscFCvtt2nValue = DivU64x64Remainder ((1000000000ULL << 32), tscFreqValue, NULL);
    tscFCvtn2tValue = DivU64x64Remainder ((1000000000ULL  << 32), tscFCvtt2nValue, NULL);
  }
  // For all other processors
  else {
    busFreqValue    = CpuInfo->FSBFrequency;
    busFCvtt2nValue = DivU64x64Remainder ((1000000000ULL << 32), busFreqValue, NULL);
    busFCvtn2tValue = DivU64x64Remainder (0xFFFFFFFFFFFFFFFFULL, busFCvtt2nValue, NULL);

    tscFreqValue    = CpuInfo->CPUFrequency;
    tscFCvtt2nValue = DivU64x64Remainder ((1000000000ULL << 32), tscFreqValue, NULL);
    tscFCvtn2tValue = DivU64x64Remainder (0xFFFFFFFFFFFFFFFFULL, tscFCvtt2nValue, NULL);
  }

  tscGranularityValue = DivU64x64Remainder (tscFreqValue, busFreqValue, NULL);

  DEBUG ((DEBUG_INFO, "OCAK: BusFreq = %LuHz, BusFCvtt2n = %Lu, BusFCvtn2t = %Lu\n", busFreqValue, busFCvtt2nValue, busFCvtn2tValue));
  DEBUG ((DEBUG_INFO, "OCAK: TscFreq = %LuHz, TscFCvtt2n = %Lu, TscFCvtn2t = %Lu\n", tscFreqValue, tscFCvtt2nValue, tscFCvtn2tValue));

  //
  // Patch _tsc_init with above values.
  //
  TscLocation = TscInitFunc;

  if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LEOPARD_MIN, 0)) {
    TscLocation = PatchMovVar (TscLocation, Patcher->Is32Bit, &TscInitFuncSymAddr, BusFreqSymAddr, busFreqValue);
  }

  TscLocation = PatchMovVar (TscLocation, Patcher->Is32Bit, &TscInitFuncSymAddr, BusFCvtt2nSymAddr, busFCvtt2nValue);
  TscLocation = PatchMovVar (TscLocation, Patcher->Is32Bit, &TscInitFuncSymAddr, BusFCvtn2tSymAddr, busFCvtn2tValue);
  TscLocation = PatchMovVar (TscLocation, Patcher->Is32Bit, &TscInitFuncSymAddr, TscFreqSymAddr, tscFreqValue);
  TscLocation = PatchMovVar (TscLocation, Patcher->Is32Bit, &TscInitFuncSymAddr, TscFCvtt2nSymAddr, tscFCvtt2nValue);
  TscLocation = PatchMovVar (TscLocation, Patcher->Is32Bit, &TscInitFuncSymAddr, TscFCvtn2tSymAddr, tscFCvtn2tValue);
  TscLocation = PatchMovVar (TscLocation, Patcher->Is32Bit, &TscInitFuncSymAddr, TscGranularitySymAddr, tscGranularityValue);

  if (Patcher->Is32Bit) {
    //
    // push ebp
    // move ebp, esp
    //
    *TscLocation++ = 0x55;
    *TscLocation++ = 0x89;
    *TscLocation++ = 0xE5;

    ValueLower = (UINT32)busFreqValue;
    ValueUpper = (UINT32)(busFreqValue >> 32);

    //
    // mov eax, FSB freq (lower)
    //
    *TscLocation++ = 0xB8;
    CopyMem (TscLocation, &ValueLower, sizeof (ValueLower));
    TscLocation += sizeof (ValueLower);

    //
    // push eax
    //
    *TscLocation++ = 0x50;

    //
    // mov eax, FSB freq (higher)
    //
    *TscLocation++ = 0xB8;
    CopyMem (TscLocation, &ValueUpper, sizeof (ValueUpper));
    TscLocation += sizeof (ValueUpper);

    //
    // push eax
    //
    *TscLocation++ = 0x50;

    ValueLower = (UINT32)tscFreqValue;
    ValueUpper = (UINT32)(tscFreqValue >> 32);

    //
    // mov eax, TSC freq (lower)
    //
    *TscLocation++ = 0xB8;
    CopyMem (TscLocation, &ValueLower, sizeof (ValueLower));
    TscLocation += sizeof (ValueLower);

    //
    // push eax
    //
    *TscLocation++ = 0x50;

    //
    // mov eax, TSC freq (higher)
    //
    *TscLocation++ = 0xB8;
    CopyMem (TscLocation, &ValueUpper, sizeof (ValueUpper));
    TscLocation += sizeof (ValueUpper);

    //
    // push eax
    //
    *TscLocation++ = 0x50;

    //
    // call _tmrCvt(busFCvtt2n, tscFCvtn2t)
    //
    Delta          = (INT32)(TmrCvtFunc - (TscLocation + 5));
    *TscLocation++ = 0xE8;
    CopyMem (TscLocation, &Delta, sizeof (Delta));
    TscLocation += sizeof (Delta);

    //
    // mov [_bus2tsc], eax
    //
    VarAddr32      = (UINT32)(Bus2TscSymAddr);
    *TscLocation++ = 0xA3;
    CopyMem (TscLocation, &VarAddr32, sizeof (VarAddr32));
    TscLocation += sizeof (VarAddr32);

    //
    // mov [_bus2tsc+4], edx
    //
    VarAddr32     += sizeof (UINT32);
    *TscLocation++ = 0x89;
    *TscLocation++ = 0x15;
    CopyMem (TscLocation, &VarAddr32, sizeof (VarAddr32));
    TscLocation += sizeof (VarAddr32);

    //
    // pop eax (x4)
    // leave
    //
    *TscLocation++ = 0x58;
    *TscLocation++ = 0x58;
    *TscLocation++ = 0x58;
    *TscLocation++ = 0x58;
    *TscLocation++ = 0xC9;
  } else {
    //
    // mov rdi, FSB freq
    //
    *TscLocation++ = 0x48;
    *TscLocation++ = 0xBF;
    CopyMem (TscLocation, &busFreqValue, sizeof (busFreqValue));
    TscLocation        += sizeof (busFreqValue);
    TscInitFuncSymAddr += sizeof (busFreqValue) + 2;

    //
    // mov rsi, TSC freq
    //
    *TscLocation++ = 0x48;
    *TscLocation++ = 0xBE;
    CopyMem (TscLocation, &tscFreqValue, sizeof (tscFreqValue));
    TscLocation        += sizeof (tscFreqValue);
    TscInitFuncSymAddr += sizeof (tscFreqValue) + 2;

    //
    // call _tmrCvt(busFCvtt2n, tscFCvtn2t)
    //
    Delta          = (INT32)(TmrCvtFunc - (TscLocation + 5));
    *TscLocation++ = 0xE8;
    CopyMem (TscLocation, &Delta, sizeof (Delta));
    TscLocation        += sizeof (Delta);
    TscInitFuncSymAddr += sizeof (Delta) + 1;

    //
    // mov [_bus2tsc], rax
    //
    Delta = (INT32)(Bus2TscSymAddr - (TscInitFuncSymAddr + 7));

    *TscLocation++ = 0x48;
    *TscLocation++ = 0x89;
    *TscLocation++ = 0x05;
    CopyMem (TscLocation, &Delta, sizeof (Delta));
    TscLocation += sizeof (Delta);
  }

  //
  // ret
  //
  *TscLocation++ = 0xC3;

  //
  // Find patch region for injecting CPU cache information
  // into set_intel_cache_info.
  //
  if (IsTigerCacheUnsupported) {
    Status = PatcherGetSymbolAddress (Patcher, "_cpuid_info", (UINT8 **)&Record);
    if (EFI_ERROR (Status) || (Record >= Last)) {
      DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to locate _cpuid_info (%p) - %r\n", Record, Status));
      return EFI_NOT_FOUND;
    }

    //
    // Start of patch area.
    // We'll use this area to populate info_p.
    //
    STATIC CONST UINT8  mKernelCpuidFindCacheLocTigerStart[9] = {
      0x8B, 0x45, 0x08,       // mov eax, dword [ebp+0x8] (*info_p argument)
      0x8B, 0x50, 0x74,       // mov edx, dword [eax+0x74]
      0x85, 0xD2,             // test edx, edx
      0x75                    // jne ...
    };

    //
    // End of patch area.
    //
    STATIC CONST UINT8  mKernelCpuidFindCacheLocTigerEnd[5] = {
      0x31, 0xC0,       // xor eax, eax
      0x0F, 0xA2,       // cpuid
      0x89              // mov ...
    };

    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindCacheLocTigerStart[0])
         && (Record[1] == mKernelCpuidFindCacheLocTigerStart[1])
         && (Record[2] == mKernelCpuidFindCacheLocTigerStart[2])
         && (Record[3] == mKernelCpuidFindCacheLocTigerStart[3])
         && (Record[4] == mKernelCpuidFindCacheLocTigerStart[4])
         && (Record[5] == mKernelCpuidFindCacheLocTigerStart[5])
         && (Record[6] == mKernelCpuidFindCacheLocTigerStart[6])
         && (Record[7] == mKernelCpuidFindCacheLocTigerStart[7])
         && (Record[8] == mKernelCpuidFindCacheLocTigerStart[8]))
      {
        break;
      }
    }

    if (Index >= EFI_PAGE_SIZE) {
      return EFI_NOT_FOUND;
    }

    LocationTigerCache = Record + 3;

    for ( ; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (  (Record[0] == mKernelCpuidFindCacheLocTigerEnd[0])
         && (Record[1] == mKernelCpuidFindCacheLocTigerEnd[1])
         && (Record[2] == mKernelCpuidFindCacheLocTigerEnd[2])
         && (Record[3] == mKernelCpuidFindCacheLocTigerEnd[3])
         && (Record[4] == mKernelCpuidFindCacheLocTigerEnd[4]))
      {
        break;
      }
    }

    if (Index >= EFI_PAGE_SIZE) {
      return EFI_NOT_FOUND;
    }

    LocationTigerCacheEnd = Record;

    DEBUG ((
      DEBUG_INFO,
      "OCAK: CPU info cache loc %p:%p\n",
      LocationTigerCache - Start,
      LocationTigerCacheEnd - Start
      ));

    ZeroMem (CacheLineSizes, sizeof (CacheLineSizes));

    //
    // Build CPU info struct
    //
    Index = 0;
    do {
      AsmCpuidEx (CPUID_CACHE_PARAMS, Index, &CpuidCacheParamsEax.Uint32, &CpuidCacheParamsEbx.Uint32, &CacheSets, NULL);
      if (CpuidCacheParamsEax.Bits.CacheType == 0) {
        break;
      }

      switch (CpuidCacheParamsEax.Bits.CacheLevel) {
        case 1:
          CacheType = (CpuidCacheParamsEax.Bits.CacheType == 1) ? L1D :
                      (CpuidCacheParamsEax.Bits.CacheType == 2) ? L1I :
                      Lnone;
          break;

        case 2:
          CacheType = (CpuidCacheParamsEax.Bits.CacheType == 3) ? L2U :
                      Lnone;
          break;

        case 3:
          CacheType = (CpuidCacheParamsEax.Bits.CacheType == 3) ? L3U :
                      Lnone;
          break;

        default:
          CacheType = Lnone;
      }

      if (CacheType != Lnone) {
        CacheSizes[CacheType]     = (CpuidCacheParamsEbx.Bits.LineSize + 1) * (CacheSets + 1) * (CpuidCacheParamsEbx.Bits.Ways + 1);
        CacheLineSizes[CacheType] = CpuidCacheParamsEbx.Bits.LineSize + 1;
      }

      Index++;
    } while (TRUE);

    if (CacheLineSizes[L2U]) {
      CacheLineSize = CacheLineSizes[L2U];
    } else if (CacheLineSizes[L1D]) {
      CacheLineSize = CacheLineSizes[L1D];
    } else {
      //
      // XNU would panic here.
      //
      DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Unable to determine CPU cache line size\n"));
      return EFI_UNSUPPORTED;
    }

    //
    // Populate cache sizes.
    // EAX = address of *info_p struct in stack; 0x68 = offset of cache size array in *info_p struct.
    //
    for (Index = L1I; Index < LCACHE_MAX; Index++) {
      *LocationTigerCache++ = 0xC7;
      *LocationTigerCache++ = 0x40;
      *LocationTigerCache++ = 0x68 + ((UINT8)(sizeof (UINT32) * Index));
      CopyMem (LocationTigerCache, &CacheSizes[Index], sizeof (CacheSizes[Index]));
      LocationTigerCache += sizeof (CacheSizes[Index]);
      DEBUG ((DEBUG_INFO, "OCAK: Cache size (L%u): %u bytes\n", (Index >= 3) ? (Index - 1) : 1, CacheSizes[Index]));
    }

    //
    // Populate cache line size.
    // 0x7C = offset of cache line size in *info_p struct.
    //
    *LocationTigerCache++ = 0xC7;
    *LocationTigerCache++ = 0x40;
    *LocationTigerCache++ = 0x7C;
    CopyMem (LocationTigerCache, &CacheLineSize, sizeof (CacheLineSize));
    LocationTigerCache += sizeof (CacheLineSize);
    DEBUG ((DEBUG_INFO, "OCAK: Cache line size: %u bytes\n", CacheLineSize));

    while (LocationTigerCache < LocationTigerCacheEnd) {
      *LocationTigerCache++ = 0x90;
    }
  }

  //
  // Patch sysctl reporting if leaf 0x4 is unsupported on 10.5 and 10.6.
  // This applies only to processors that are older than Prescott, and only on IA32.
  //
  if (Patcher->Is32Bit && !IsLeaf4CacheSupported && (IsLeopard || IsSnowLeopard)) {
    ZeroMem (CpuidCacheDescriptors, sizeof (CpuidCacheDescriptors));
    ZeroMem (CacheSizes, sizeof (CacheSizes));
    ZeroMem (CacheSharings, sizeof (CacheSharings));
    AsmCpuid (CPUID_CACHE_INFO, &CpuidCacheInfo[0].Uint32, &CpuidCacheInfo[1].Uint32, &CpuidCacheInfo[2].Uint32, &CpuidCacheInfo[3].Uint32);

    for (Index = 0; Index < 4; Index++) {
      if (CpuidCacheInfo[Index].Bits.NotValid) {
        continue;
      }

      ((UINT32 *)CpuidCacheDescriptors)[Index] = CpuidCacheInfo[Index].Uint32;
    }

    for (Index = 1; Index < CpuidCacheDescriptors[0]; Index++) {
      if ((Index * 16) > sizeof (CpuidCacheDescriptors)) {
        break;
      }

      AsmCpuid (CPUID_CACHE_INFO, &CpuidCacheInfo[0].Uint32, &CpuidCacheInfo[1].Uint32, &CpuidCacheInfo[2].Uint32, &CpuidCacheInfo[3].Uint32);

      for (Index2 = 0; Index2 < 4; Index2++) {
        if (CpuidCacheInfo[Index2].Bits.NotValid) {
          continue;
        }

        ((UINT32 *)CpuidCacheDescriptors)[sizeof (UINT32) * Index * Index2] = CpuidCacheInfo[Index2].Uint32;
      }
    }

    //
    // Get cache sizes.
    //
    for (Index = 0; Index < sizeof (CpuidCacheDescriptors); Index++) {
      CacheDescriptorFound = FALSE;
      for (Index2 = 0; Index2 < (sizeof (mCpuCacheDescriptorValues) / sizeof (mCpuCacheDescriptorValues[0])); Index2++) {
        if (mCpuCacheDescriptorValues[Index2].value == CpuidCacheDescriptors[Index]) {
          CacheDescriptorType  = mCpuCacheDescriptorValues[Index2].type;
          CacheDescriptorSize  = mCpuCacheDescriptorValues[Index2].size;
          CacheDescriptorFound = TRUE;
          break;
        }
      }

      if (!CacheDescriptorFound) {
        continue;
      }

      CacheSizes[CacheDescriptorType]    = CacheDescriptorSize;
      CacheSharings[CacheDescriptorType] = 1;
    }

    DEBUG ((DEBUG_INFO, "OCAK: Caches L1I: %u, L1D: %u, L2: %u, L3: %u\n", CacheSizes[L1I], CacheSizes[L1D], CacheSizes[L2U], CacheSizes[L3U]));

    if (IsLeopard) {
      //
      // Patch _ml_cpu_cache_sharing (0x1)
      //
      CacheSize = MAX (CacheSharings[L1I], CacheSharings[L1D]);
      CopyMem (
        &mProvideCurrentCpuInfoLeopardSysctlMibInitReplace1[1],
        &CacheSize,
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoLeopardSysctlMibInitPatch1
                  );

      //
      // Patch _ml_cpu_cache_sharing (0x2)
      //
      CopyMem (
        &mProvideCurrentCpuInfoLeopardSysctlMibInitReplace1[1],
        &CacheSharings[L2U],
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoLeopardSysctlMibInitPatch1
                  );

      //
      // Patch _ml_cpu_cache_sharing (0x3)
      //
      CopyMem (
        &mProvideCurrentCpuInfoLeopardSysctlMibInitReplace2[1],
        &CacheSharings[L3U],
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoLeopardSysctlMibInitPatch2
                  );

      //
      // Patch _ml_cpu_cache_size (0x1) - L1
      //
      CacheSize = MAX (CacheSizes[L1I], CacheSizes[L1D]);
      CopyMem (
        &mProvideCurrentCpuInfoLeopardSysctlMibInitReplace1[1],
        &CacheSize,
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoLeopardSysctlMibInitPatch1
                  );

      //
      // Patch _ml_cpu_cache_size (0x2) - L2
      //
      CopyMem (
        &mProvideCurrentCpuInfoLeopardSysctlMibInitReplace1[1],
        &CacheSizes[L2U],
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoLeopardSysctlMibInitPatch1
                  );

      //
      // Patch _ml_cpu_cache_size (0x3) - L3
      //
      CopyMem (
        &mProvideCurrentCpuInfoLeopardSysctlMibInitReplace2[1],
        &CacheSizes[L3U],
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoLeopardSysctlMibInitPatch2
                  );
    } else {
      //
      // Patch _ml_cpu_cache_sharing (0x1)
      //
      CacheSize = MAX (CacheSharings[L1I], CacheSharings[L1D]);
      CopyMem (
        &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplace[12],
        &CacheSize,
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitPatch
                  );

      //
      // Patch _ml_cpu_cache_sharing (0x2)
      //
      CopyMem (
        &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplace[12],
        &CacheSharings[L2U],
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitPatch
                  );

      //
      // Patch _ml_cpu_cache_sharing (0x3)
      //
      CopyMem (
        &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplace[12],
        &CacheSharings[L3U],
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitPatch
                  );

      //
      // Patch _ml_cpu_cache_size (0x1) - L1
      //
      CacheSize = MAX (CacheSizes[L1I], CacheSizes[L1D]);
      CopyMem (
        &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplace[12],
        &CacheSize,
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitPatch
                  );

      //
      // Patch _ml_cpu_cache_size (0x2) - L2
      //
      CopyMem (
        &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplace[12],
        &CacheSizes[L2U],
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitPatch
                  );

      //
      // Patch _ml_cpu_cache_size (0x3) - L3
      //
      CopyMem (
        &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitReplace[12],
        &CacheSizes[L3U],
        sizeof (UINT32)
        );
      Status |= PatcherApplyGenericPatch (
                  Patcher,
                  &mProvideCurrentCpuInfoSnowLeopardSysctlMibInitPatch
                  );
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCAK: [FAIL] Failed to patch or more areas in _sysctl_mib_init - %r\n", Status));
      return EFI_NOT_FOUND;
    }
  }

  //
  // Patch MSR 0x35 fallback value on 10.13 and above.
  //
  // This value is used if the MSR 0x35 is read as zero, typically on VMs or AMD processors.
  //
  if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, 0)) {
    //
    // 10.15 and above have two instances that need patching.
    //
    if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_CATALINA_MIN, 0)) {
      mProvideCurrentCpuInfoZeroMsrThreadCoreCountPatch.Count = 2;
    }

    msrCoreThreadCount = (CpuInfo->CoreCount << 16) | CpuInfo->ThreadCount;

    CopyMem (
      &mProvideCurrentCpuInfoZeroMsrThreadCoreCountReplace[CURRENT_CPU_INFO_CORE_COUNT_OFFSET],
      &msrCoreThreadCount,
      sizeof (msrCoreThreadCount)
      );

    Status = PatcherApplyGenericPatch (
               Patcher,
               &mProvideCurrentCpuInfoZeroMsrThreadCoreCountPatch
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to find CPU MSR 0x35 default value patch - %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping CPU MSR 0x35 default value patch on %u\n", KernelVersion));
  }

  //
  // Disable _x86_validate_topology on 10.13 and above.
  //
  if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, 0)) {
    Status = PatcherApplyGenericPatch (
               Patcher,
               &mProvideCurrentCpuInfoTopologyValidationPatch
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: [FAIL] Failed to find CPU topology validation patch - %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: [OK] Skipping CPU topology validation patch on %u\n", KernelVersion));
  }

  return EFI_SUCCESS;
}
