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
  0xC7, 0x87, 0x88, 0x01, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, // mov dword ptr [rdi+188h], 0AAAAAAAAh
  0xB8, 0xAA, 0xAA, 0xAA, 0xAA,                               // mov eax, 0AAAAAAAAh
  0xC3                                                        // retn
};

#pragma pack(push, 1)

typedef struct {
  UINT8   Code1[3];
  UINT32  Signature;
  UINT8   Code2[3];
  UINT8   Stepping;
  UINT8   Code3[2];
  UINT32  Type;
  UINT8   Family;
  UINT8   Model;
  UINT8   ExtModel;
  UINT8   ExtFamily;
  UINT8   Code4[6];
  UINT64  Features;
  UINT8   Code5[10];
  UINT32  LogicalPerPkg;
  UINT8   Code6[6];
  UINT32  AppleFamily1;
  UINT8   Code7;
  UINT32  AppleFamily2;
  UINT8   Code8;
} INTERNAL_CPUID_FN_PATCH;

STATIC_ASSERT (
  sizeof (INTERNAL_CPUID_FN_PATCH) == sizeof (mKernelCpuidReplaceDbg),
  "Check your CPUID patch layout"
  );

typedef struct {
  UINT8   EaxCmd;
  UINT32  EaxVal;
  UINT8   EbxCmd;
  UINT32  EbxVal;
  UINT8   EcxCmd;
  UINT32  EcxVal;
  UINT8   EdxCmd;
  UINT32  EdxVal;
} INTERNAL_CPUID_PATCH;

typedef struct {
  UINT8   EdxCmd;
  UINT32  EdxVal;
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
  EFI_STATUS            Status;

  UINT8                 *Record;
  UINT8                 *BlockClearFunc;
  UINT8                 *StartPointer;
  UINT8                 *EndPointer;
  UINT32                StructAddr;
  UINT8                 *Location;
  UINT8                 *LocationEnd;
  UINT8                 *LocationTsc;
  UINT8                 *LocationTscEnd;
  UINT8                 *LocationSnow32;
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
  IsTiger     = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_TIGER_MIN, KERNEL_VERSION_TIGER_MAX);
  IsTigerOld  = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_TIGER_MIN, KERNEL_VERSION (8, 10, 1));
  IsLeopard   = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LEOPARD_MIN, KERNEL_VERSION_LEOPARD_MAX);
  IsSnow      = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_SNOW_LEOPARD_MIN, KERNEL_VERSION_SNOW_LEOPARD_MAX);
  IsLion      = OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LION_MIN, KERNEL_VERSION_LION_MAX);
  StructAddr  = 0;
  
  LocationSnow32  = NULL;
  LocationTsc     = NULL;
  LocationTscEnd  = NULL;

  //
  // Locate _cpuid_set_info or _cpuid_get_info.
  // _cpuid_set_info is also patched in 10.4, and is located below _cpuid_get_info.
  //
  Status = PatcherGetSymbolAddress (Patcher, IsTiger ? "_cpuid_get_info" : "_cpuid_set_info", (UINT8 **) &Record);
  if (EFI_ERROR (Status) || Record >= Last) {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _cpuid_%a_info (%p) - %r\n", IsTiger ? "get" : "set", Record, Status));
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
  STATIC CONST UINT8 mKernelCpuidFindPatchTigerStart[4] = {
    0x8D, 0x45, 0xEC,               // lea eax, dword [...]
    0xC7                            // mov from mov dword [...], 4
  };

  STATIC CONST UINT8 mKernelCpuidFindPatchLeoSnowLionStruct32[2] = {
    0x00,                           // 0 from end of mov dword [...], [struct address]
    0xE8                            // call from call _blkclr or _bzero
  };

  STATIC CONST UINT8 mKernelCpuidFindPatchLeoSnowLionStart32[5] = {
    0x04, 0x00, 0x00, 0x00,         // 4 from mov dword [...], 4
    0xC7                            // mov...
  };

  STATIC CONST UINT8 mKernelCpuidFindPatchSnowStart64[8] = {
    0xBA, 0x04, 0x00, 0x00, 0x00,  // mov edx, 4
    0x48, 0x8D, 0x35               // lea rsi, ...
  };

  if (Patcher->Is32Bit) {
    if (IsTiger) {
      for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (Record[0] == mKernelCpuidFindPatchTigerStart[0]
          && Record[1] == mKernelCpuidFindPatchTigerStart[1]
          && Record[2] == mKernelCpuidFindPatchTigerStart[2]
          && Record[3] == mKernelCpuidFindPatchTigerStart[3]) {
          break;
        }
      }
    } else {
      //
      // We need to get the address of _blkclr or _bzero first for proper matching.
      //
      Status = PatcherGetSymbolAddress (Patcher, IsLion ? "_bzero" : "_blkclr", (UINT8 **) &BlockClearFunc);
      if (EFI_ERROR (Status) || Record >= Last) {
        DEBUG ((DEBUG_WARN, "OCAK: Failed to locate %a (%p) - %r\n", IsLion ? "_bzero" : "_blkclr", Record, Status));
        return EFI_NOT_FOUND;
      }

      //
      // Get struct address first.
      //
      for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (Record[0] == mKernelCpuidFindPatchLeoSnowLionStruct32[0]
          && Record[1] == mKernelCpuidFindPatchLeoSnowLionStruct32[1]
          && *((INT32 *) &Record[2]) == (INT32) (BlockClearFunc - (Record + sizeof (mKernelCpuidFindPatchLeoSnowLionStruct32) + sizeof (UINT32)))) {
          break;
        }
      }

      if (Index >= EFI_PAGE_SIZE) {
        return EFI_NOT_FOUND;
      }

      StructAddr = *((UINT32 *) (Record - 3));

      for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (Record[0] == mKernelCpuidFindPatchLeoSnowLionStart32[0]
          && Record[1] == mKernelCpuidFindPatchLeoSnowLionStart32[1]
          && Record[2] == mKernelCpuidFindPatchLeoSnowLionStart32[2]
          && Record[3] == mKernelCpuidFindPatchLeoSnowLionStart32[3]
          && Record[4] == mKernelCpuidFindPatchLeoSnowLionStart32[4]) {
          break;
        }
      }
    }
  } else {
    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindPatchSnowStart64[0]
        && Record[1] == mKernelCpuidFindPatchSnowStart64[1]
        && Record[2] == mKernelCpuidFindPatchSnowStart64[2]
        && Record[3] == mKernelCpuidFindPatchSnowStart64[3]
        && Record[4] == mKernelCpuidFindPatchSnowStart64[4]
        && Record[5] == mKernelCpuidFindPatchSnowStart64[5]
        && Record[6] == mKernelCpuidFindPatchSnowStart64[6]
        && Record[7] == mKernelCpuidFindPatchSnowStart64[7]) {
        break;
      }
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  if (Patcher->Is32Bit) {
    StartPointer = IsTiger ? Record : Record - 4 ;
  } else {
    StartPointer = Record + sizeof (mKernelCpuidFindPatchSnowStart64) + sizeof (UINT32);
  }

  //
  // End of patch area.
  //
  STATIC CONST UINT8 mKernelCpuidFindPatchTigerEnd[4] = {
    0x00,                           // 0 from mov byte [...], 0
    0x31, 0xDB,                     // xor ebx, ebx
    0x8B                            // mov ...
  };

  STATIC UINT8 mKernelCpuidFindPatchSnowLionEnd32[7] = {
    0xC6, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0x00    // mov byte [struct_addr], 0
  };
  STATIC UINT32 *mKernelCpuidFindPatchSnowLionEndPtr32 = (UINT32 *) &mKernelCpuidFindPatchSnowLionEnd32[2];
  *mKernelCpuidFindPatchSnowLionEndPtr32 = StructAddr + sizeof (Signature[0]) * 3;

  STATIC CONST UINT8 mKernelCpuidFindPatchLeoEnd1[5] = {
    0xB8, 0x00, 0x00, 0x00, 0x80    // mov eax/edx, 80000000h
  };
  STATIC CONST UINT8 mKernelCpuidFindPatchLeoEnd1Mask = 0xFD;

  if (IsTiger) {
    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindPatchTigerEnd[0]
        && Record[1] == mKernelCpuidFindPatchTigerEnd[1]
        && Record[2] == mKernelCpuidFindPatchTigerEnd[2]
        && Record[3] == mKernelCpuidFindPatchTigerEnd[3]) {
        break;
      }
    }
  } else if ((IsSnow || IsLion) && Patcher->Is32Bit) {
    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindPatchSnowLionEnd32[0]
        && Record[1] == mKernelCpuidFindPatchSnowLionEnd32[1]
        && Record[2] == mKernelCpuidFindPatchSnowLionEnd32[2]
        && Record[3] == mKernelCpuidFindPatchSnowLionEnd32[3]
        && Record[4] == mKernelCpuidFindPatchSnowLionEnd32[4]
        && Record[5] == mKernelCpuidFindPatchSnowLionEnd32[5]
        && Record[6] == mKernelCpuidFindPatchSnowLionEnd32[6]) {
        break;
      }
    }
  } else {
    for (Index = 0; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if ((Record[0] & mKernelCpuidFindPatchLeoEnd1Mask) == mKernelCpuidFindPatchLeoEnd1[0]
        && Record[1] == mKernelCpuidFindPatchLeoEnd1[1]
        && Record[2] == mKernelCpuidFindPatchLeoEnd1[2]
        && Record[3] == mKernelCpuidFindPatchLeoEnd1[3]
        && Record[4] == mKernelCpuidFindPatchLeoEnd1[4]) {
        break;
      }
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  if (IsSnow && !Patcher->Is32Bit) {
    STATIC CONST UINT8 mKernelCpuidFindPatchLeoEnd2[3] = {
      0x0F, 0xA2,                   // cpuid
      0x89                          // mov ...
    };

    for (; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindPatchLeoEnd2[0]
        && Record[1] == mKernelCpuidFindPatchLeoEnd2[1]
        && Record[2] == mKernelCpuidFindPatchLeoEnd2[2]) {
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
  STATIC CONST UINT8 mKernelCpuidFindLocLeoTigerStart[7] = {
    0xB9, 0x01, 0x00, 0x00, 0x00,       // mov ecx, 1
    0x89, 0xC8                          // mov eax, ecx
  };

  STATIC CONST UINT8 mKernelCpuidFindLocSnowStart[5] = {
    0xB8, 0x01, 0x00, 0x00, 0x00        // mov eax, 1
  };

  STATIC CONST UINT8 mKernelCpuidFindLocSnowStart32[6] = {
    0xE8, 0xFF, 0xFF, 0xFF, 0xFF,       // call _cpuid64
    0xEB                                // jmp ...
  };

  STATIC CONST UINT8 mKernelCpuidFindLocLionStart32[8] = {
    0xB9, 0x01, 0x00, 0x00, 0x00,       // mov ecx, 1
    0x8D, 0x55, 0xD0                    // lea edx, dword [...]
  };

  if (IsTiger || IsLeopard) {
    for (; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindLocLeoTigerStart[0]
        && Record[1] == mKernelCpuidFindLocLeoTigerStart[1]
        && Record[2] == mKernelCpuidFindLocLeoTigerStart[2]
        && Record[3] == mKernelCpuidFindLocLeoTigerStart[3]
        && Record[4] == mKernelCpuidFindLocLeoTigerStart[4]
        && Record[5] == mKernelCpuidFindLocLeoTigerStart[5]
        && Record[6] == mKernelCpuidFindLocLeoTigerStart[6]) {
        break;
      }
    }
  } else if (IsSnow) {
    for (; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindLocSnowStart[0]
        && Record[1] == mKernelCpuidFindLocSnowStart[1]
        && Record[2] == mKernelCpuidFindLocSnowStart[2]
        && Record[3] == mKernelCpuidFindLocSnowStart[3]
        && Record[4] == mKernelCpuidFindLocSnowStart[4]) {
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
      for (; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (Record[0] == mKernelCpuidFindLocSnowStart32[0]
          && Record[5] == mKernelCpuidFindLocSnowStart32[5]) {
          break;
        }
      }

      if (Index >= EFI_PAGE_SIZE) {
        return EFI_NOT_FOUND;
      }

      LocationSnow32 = Record;

      for (; Index < EFI_PAGE_SIZE; Index++, Record++) {
        if (Record[0] == mKernelCpuidFindLocSnowStart[0]
          && Record[1] == mKernelCpuidFindLocSnowStart[1]
          && Record[2] == mKernelCpuidFindLocSnowStart[2]
          && Record[3] == mKernelCpuidFindLocSnowStart[3]
          && Record[4] == mKernelCpuidFindLocSnowStart[4]) {
          break;
        }
      }
    }
  } else {
    for (; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindLocLionStart32[0]
        && Record[1] == mKernelCpuidFindLocLionStart32[1]
        && Record[2] == mKernelCpuidFindLocLionStart32[2]
        && Record[3] == mKernelCpuidFindLocLionStart32[3]
        && Record[4] == mKernelCpuidFindLocLionStart32[4]
        && Record[5] == mKernelCpuidFindLocLionStart32[5]
        && Record[6] == mKernelCpuidFindLocLionStart32[6]
        && Record[7] == mKernelCpuidFindLocLionStart32[7]) {
        break;
      }
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  Location = IsLion ? Record + sizeof (mKernelCpuidFindLocLionStart32): Record;

  //
  // End of CPUID location. Not applicable to 10.7.
  //
  STATIC CONST UINT8 mKernelCpuidFindLegacyLocEnd[3] = {
    0x0F, 0xA2,                   // cpuid
    0x89                          // mov ...
  };

  if (!IsLion) {
    for (; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindLegacyLocEnd[0]
        && Record[1] == mKernelCpuidFindLegacyLocEnd[1]
        && Record[2] == mKernelCpuidFindLegacyLocEnd[2]) {
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
  // It's possible 8.10.2 may require the patch, but there is no sources or kernels
  // available to verify.
  //
  if (IsTigerOld) {
    Status = PatcherGetSymbolAddress (Patcher, "_tsc_init", (UINT8 **) &Record);
    if (EFI_ERROR (Status) || Record >= Last) {
      DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _tsc_init (%p) - %r\n", Record, Status));
      return EFI_NOT_FOUND;
    }

    //
    // Start of _tsc_init CPUID location.
    //
    // We'll replace this with a call to the previous patched section to
    // populate the CPUID (1) info.
    //
    STATIC CONST UINT8 mKernelCpuidFindTscLocTigerStart[7] = {
      0xBA, 0x01, 0x00, 0x00, 0x00,       // mov edx, 1
      0x89, 0xD0                          // mov eax, edx
    };

    for (; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindTscLocTigerStart[0]
        && Record[1] == mKernelCpuidFindTscLocTigerStart[1]
        && Record[2] == mKernelCpuidFindTscLocTigerStart[2]
        && Record[3] == mKernelCpuidFindTscLocTigerStart[3]
        && Record[4] == mKernelCpuidFindTscLocTigerStart[4]
        && Record[5] == mKernelCpuidFindTscLocTigerStart[5]
        && Record[6] == mKernelCpuidFindTscLocTigerStart[6]) {
        break;
      }
    }

    if (Index >= EFI_PAGE_SIZE) {
      return EFI_NOT_FOUND;
    }

    LocationTsc = Record;

    //
    // End of _tsc_init CPUID location.
    //
    for (; Index < EFI_PAGE_SIZE; Index++, Record++) {
      if (Record[0] == mKernelCpuidFindLegacyLocEnd[0]
        && Record[1] == mKernelCpuidFindLegacyLocEnd[1]
        && Record[2] == mKernelCpuidFindLegacyLocEnd[2]) {
        break;
      }
    }

    if (Index >= EFI_PAGE_SIZE) {
      return EFI_NOT_FOUND;
    }

    LocationTscEnd = Record + 2;
  }

  DEBUG ((
    DEBUG_INFO,
    "OCAK: Legacy CPUID patch %p:%p, loc %p:%p, tsc loc %p:%p struct @ 0x%X\n",
    StartPointer - Start,
    EndPointer - Start,
    Location - Start,
    LocationEnd - Start,
    IsTigerOld ? LocationTsc - Start : 0,
    IsTigerOld ? LocationTscEnd - Start : 0,
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
        *StartPointer++ = (UINT8) (Index * sizeof (Signature[0]));
      
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
  if (StartPointer >= EndPointer
    || EndPointer - StartPointer > 128
    || (UINTN) (EndPointer - StartPointer) < sizeof (INTERNAL_CPUID_PATCH) + 3) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Short jmp to EndPointer.
  //
  StartPointer[0] = 0xEB;
  StartPointer[1] = (UINT8) (EndPointer - StartPointer - 2);
  StartPointer += 2;

  //
  // Workaround incorrect OSXSAVE handling, see below.
  //
  if (CpuInfo->CpuidVerEcx.Bits.XSAVE != 0
    && CpuInfo->CpuidVerEcx.Bits.OSXSAVE == 0
    && CpuInfo->CpuidVerEcx.Bits.AVX != 0) {
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
  Delta = (INT32) (StartPointer - (Location + 5));
  *Location++ = 0xE8;
  CopyMem (Location, &Delta, sizeof (Delta));
  Location   += sizeof (Delta);
  while (Location < LocationEnd) {
    *Location++ = 0x90;
  }

  //
  // In 10.4, we need to replace a call to CPUID (1) with a call to
  // the patch area like above in _tsc_init.
  //
  if (IsTigerOld) {
    Delta = (INT32) (StartPointer - (LocationTsc + 5));
    *LocationTsc++ = 0xE8;
    CopyMem (LocationTsc, &Delta, sizeof (Delta));
    LocationTsc   += sizeof (Delta);
    while (LocationTsc < LocationTscEnd) {
      *LocationTsc++ = 0x90;
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

  DEBUG ((DEBUG_INFO, "OCAK: Legacy CPUID patch completed @ %p\n", StartPointer - Start));
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

  ASSERT (mKernelCpuIdFindRelNew[0] == mKernelCpuIdFindRelOld[0]
    && mKernelCpuIdFindRelNew[1] == mKernelCpuIdFindRelOld[1]
    && mKernelCpuIdFindRelNew[2] == mKernelCpuIdFindRelOld[2]
    && mKernelCpuIdFindRelNew[3] == mKernelCpuIdFindRelOld[3]
    );

  Start = ((UINT8 *) MachoGetMachHeader (&Patcher->MachContext));
  Last  = Start + MachoGetFileSize (&Patcher->MachContext) - EFI_PAGE_SIZE * 2 - sizeof (mKernelCpuIdFindRelNew);

  //
  // Do legacy patching for 32-bit 10.7, and 10.6 and older.
  //
  if ((OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LION_MIN, KERNEL_VERSION_LION_MAX) && Patcher->Is32Bit)
    || OcMatchDarwinVersion (KernelVersion, 0, KERNEL_VERSION_SNOW_LEOPARD_MAX)) {
    Status = PatchKernelCpuIdLegacy (Patcher, KernelVersion, CpuInfo, Data, DataMask, Start, Last);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCAK: Failed to patch legacy CPUID - %r\n", Status));
    }
    return Status;
  }

  Status = PatcherGetSymbolAddress (Patcher, "_cpuid_set_info", (UINT8 **) &CpuidSetInfo);
  if (EFI_ERROR (Status) || CpuidSetInfo >= Last) {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _cpuid_set_info (%p) - %r\n", CpuidSetInfo, Status));
    return EFI_NOT_FOUND;
  }

  Record = CpuidSetInfo;
  FoundSize = 0;

  for (Index = 0; Index < EFI_PAGE_SIZE; ++Index, ++Record) {
    if (Record[0] == mKernelCpuIdFindRelNew[0]
      && Record[1] == mKernelCpuIdFindRelNew[1]
      && Record[2] == mKernelCpuIdFindRelNew[2]
      && Record[3] == mKernelCpuIdFindRelNew[3]) {

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
  if (FoundReleaseKernel
    && CpuInfo->CpuidVerEcx.Bits.XSAVE != 0
    && CpuInfo->CpuidVerEcx.Bits.OSXSAVE == 0
    && CpuInfo->CpuidVerEcx.Bits.AVX != 0) {
    CpuInfo->CpuidVerEcx.Bits.OSXSAVE = 1;
  }

  Eax.Uint32 = (Data[0] & DataMask[0]) | (CpuInfo->CpuidVerEax.Uint32 & ~DataMask[0]);
  Ebx.Uint32 = (Data[1] & DataMask[1]) | (CpuInfo->CpuidVerEbx.Uint32 & ~DataMask[1]);
  Ecx.Uint32 = (Data[2] & DataMask[2]) | (CpuInfo->CpuidVerEcx.Uint32 & ~DataMask[2]);
  Edx.Uint32 = (Data[3] & DataMask[3]) | (CpuInfo->CpuidVerEdx.Uint32 & ~DataMask[3]);

  if (FoundReleaseKernel) {
    CpuidPatch         = (INTERNAL_CPUID_PATCH *) Record;
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
        McPatch         = (INTERNAL_MICROCODE_PATCH *) Record;
        McPatch->EdxCmd = 0xBA;
        McPatch->EdxVal = CpuInfo->MicrocodeRevision;
        SetMem (
          Record + sizeof (INTERNAL_MICROCODE_PATCH),
          sizeof (mKernelCpuidFindMcRel) - sizeof (INTERNAL_MICROCODE_PATCH),
          0x90
          );
        return EFI_SUCCESS;
      }
    }
  } else {
    //
    // Handle debug kernel here...
    //
    Status = PatcherGetSymbolAddress (Patcher, "_cpuid_set_cpufamily", (UINT8 **) &Record);
    if (EFI_ERROR (Status) || Record >= Last) {
      DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _cpuid_set_cpufamily (%p) - %r\n", Record, Status));
      return EFI_NOT_FOUND;
    }

    CopyMem (Record, mKernelCpuidReplaceDbg, sizeof (mKernelCpuidReplaceDbg));
    FnPatch = (INTERNAL_CPUID_FN_PATCH *) Record;
    FnPatch->Signature     = Eax.Uint32;
    FnPatch->Stepping      = (UINT8) Eax.Bits.SteppingId;
    FnPatch->ExtModel      = (UINT8) Eax.Bits.ExtendedModelId;
    FnPatch->Model         = (UINT8) Eax.Bits.Model | (UINT8) (Eax.Bits.ExtendedModelId << 4U);
    FnPatch->Family        = (UINT8) Eax.Bits.FamilyId;
    FnPatch->Type          = (UINT8) Eax.Bits.ProcessorType;
    FnPatch->ExtFamily     = (UINT8) Eax.Bits.ExtendedFamilyId;
    FnPatch->Features      = LShiftU64 (Ecx.Uint32, 32) | (UINT64) Edx.Uint32;
    if (FnPatch->Features & CPUID_FEATURE_HTT) {
      FnPatch->LogicalPerPkg = (UINT16) Ebx.Bits.MaximumAddressableIdsForLogicalProcessors;
    } else {
      FnPatch->LogicalPerPkg = 1;
    }

    FnPatch->AppleFamily1 = FnPatch->AppleFamily2 = OcCpuModelToAppleFamily (Eax);

    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_WARN, "OCAK: Failed to find either CPUID patch (%u)\n", FoundSize));

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

#define CURRENT_CPU_INFO_CORE_COUNT_OFFSET 4

STATIC
UINT8
mProvideCurrentCpuInfoZeroMsrThreadCoreCountFind[] = {
  // or rcx, rdx
  0x48, 0x09, 0xD1,
  // mov ecx, 0x10001
  0xB9, 0x01, 0x00, 0x01, 0x00
};

STATIC
UINT8
mProvideCurrentCpuInfoZeroMsrThreadCoreCountReplace[] = {
  // or rcx, rdx
  0x48, 0x09, 0xD1,
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
UINT8* PatchMovVar (
  IN OUT  UINT8             *Location,
  IN      UINT8             *Start,
  IN      MACH_SECTION_ANY  *DataSection,
  IN      MACH_SECTION_ANY  *TextSection,
  IN      BOOLEAN           Is32Bit,
  IN      UINT8             *Var,
  IN      UINT64            Value
  )
{
  INT32   Delta;
  UINT64  LocationAddr;
  UINT64  VarAddr64;
  UINT32  VarAddr32;
  UINT32  ValueLower;
  UINT32  ValueUpper;

  if (Is32Bit) {
    ValueLower = (UINT32) Value;
    ValueUpper = (UINT32) (Value >> 32);

    //
    // mov [var], value lower
    //
    VarAddr32 = (UINT32) ((Var - Start) + DataSection->Section32.Address - DataSection->Section32.Offset);
    *Location++ = 0xC7;
    *Location++ = 0x05;
    CopyMem (Location, &VarAddr32, sizeof (VarAddr32));
    Location += sizeof (VarAddr32);
    CopyMem (Location, &ValueLower, sizeof (ValueLower));
    Location += sizeof (ValueLower);

    //
    // mov [var+4], value upper
    //
    VarAddr32 += sizeof (UINT32);
    *Location++ = 0xC7;
    *Location++ = 0x05;
    CopyMem (Location, &VarAddr32, sizeof (VarAddr32));
    Location += sizeof (VarAddr32);
    CopyMem (Location, &ValueUpper, sizeof (ValueUpper));
    Location += sizeof (ValueUpper);

  } else {
    //
    // mov rax, value
    //
    *Location++ = 0x48;
    *Location++ = 0xB8;
    CopyMem (Location, &Value, sizeof (Value));
    Location += sizeof (Value);

    LocationAddr = (Location - Start) + TextSection->Section64.Address - TextSection->Section64.Offset;
    VarAddr64    = (Var - Start) + DataSection->Section64.Address - DataSection->Section64.Offset;

    //
    // mov [var], rax
    //
    Delta = (INT32) (VarAddr64 - (LocationAddr + 7));
    *Location++ = 0x48;
    *Location++ = 0x89;
    *Location++ = 0x05;
    CopyMem (Location, &Delta, sizeof (Delta));
    Location += sizeof (Delta);
  }

  return Location;
}

EFI_STATUS
PatchProvideCurrentCpuInfo (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     OC_CPU_INFO      *CpuInfo,
  IN     UINT32           KernelVersion
  )
{
  EFI_STATUS        Status;

  UINT8             *Start;
  MACH_SECTION_ANY  *DataSection;
  MACH_SECTION_ANY  *TextSection;

  INT32             Delta;
  UINT64            LocationAddr;
  UINT64            VarAddr64;
  //UINT32            VarAddr32;

  UINT8             *TscInitFunc;
  UINT8             *TmrCvtFunc;

  UINT8             *BusFreq;
  UINT8             *BusFCvtt2n;
  UINT8             *BusFCvtn2t;
  UINT8             *TscFreq;
  UINT8             *TscFCvtt2n;
  UINT8             *TscFCvtn2t;
  UINT8             *TscGranularity;
  UINT8             *Bus2Tsc;

  UINT8             *TscLocation;

  UINT64            busFreqValue;
  UINT64            busFCvtt2nValue;
  UINT64            busFCvtn2tValue;
  UINT64            tscFreqValue;
  UINT64            tscFCvtt2nValue;
  UINT64            tscFCvtn2tValue;
  UINT64            tscGranularityValue;

  UINT32            msrCoreThreadCount;

  ASSERT (Patcher != NULL);

  Start = ((UINT8 *) MachoGetMachHeader (&Patcher->MachContext));

  //
  // 10.6 and below has variables in __DATA/__data instead of __DATA/__common
  //
  if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_LION_MIN, 0)) {
    DataSection = MachoGetSegmentSectionByName (&Patcher->MachContext, "__DATA", "__common");
  } else {
    DataSection = MachoGetSegmentSectionByName (&Patcher->MachContext, "__DATA", "__data");
  }
  TextSection = MachoGetSegmentSectionByName (&Patcher->MachContext, "__TEXT", "__text");

  //
  // Pull required symbols.
  //
  Status = EFI_SUCCESS;
  Status |= PatcherGetSymbolAddress (Patcher, "_tsc_init",        (UINT8 **) &TscInitFunc);
  Status |= PatcherGetSymbolAddress (Patcher, "_tmrCvt",          (UINT8 **) &TmrCvtFunc);

  Status |= PatcherGetSymbolAddress (Patcher, "_busFreq",         (UINT8 **) &BusFreq);
  Status |= PatcherGetSymbolAddress (Patcher, "_busFCvtt2n",      (UINT8 **) &BusFCvtt2n);
  Status |= PatcherGetSymbolAddress (Patcher, "_busFCvtn2t",      (UINT8 **) &BusFCvtn2t);
  Status |= PatcherGetSymbolAddress (Patcher, "_tscFreq",         (UINT8 **) &TscFreq);
  Status |= PatcherGetSymbolAddress (Patcher, "_tscFCvtt2n",      (UINT8 **) &TscFCvtt2n);
  Status |= PatcherGetSymbolAddress (Patcher, "_tscFCvtn2t",      (UINT8 **) &TscFCvtn2t);
  Status |= PatcherGetSymbolAddress (Patcher, "_tscGranularity",  (UINT8 **) &TscGranularity);
  Status |= PatcherGetSymbolAddress (Patcher, "_bus2tsc",         (UINT8 **) &Bus2Tsc);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate one or more TSC symbols - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  //
  // Perform TSC and FSB calculations. This is traditionally done in tsc.c in XNU.
  //  
  busFreqValue = CpuInfo->FSBFrequency;
  busFCvtt2nValue = DivU64x64Remainder ((1000000000ULL << 32), busFreqValue, NULL);
  busFCvtn2tValue = DivU64x64Remainder (0xFFFFFFFFFFFFFFFFULL, busFCvtt2nValue, NULL);

  tscFreqValue = CpuInfo->CPUFrequency;
  tscFCvtt2nValue = DivU64x64Remainder ((1000000000ULL << 32), tscFreqValue, NULL);
  tscFCvtn2tValue = DivU64x64Remainder (0xFFFFFFFFFFFFFFFFULL, tscFCvtt2nValue, NULL);

  tscGranularityValue = DivU64x64Remainder (tscFreqValue, busFreqValue, NULL);

  DEBUG ((DEBUG_INFO, "OCAK: BusFreq = %LuHz, BusFCvtt2n = %Lu, BusFCvtn2t = %Lu\n", busFreqValue, busFCvtt2nValue, busFCvtn2tValue));
  DEBUG ((DEBUG_INFO, "OCAK: TscFreq = %LuHz, TscFCvtt2n = %Lu, TscFCvtn2t = %Lu\n", tscFreqValue, tscFCvtt2nValue, tscFCvtn2tValue));

  //
  // Patch _tsc_init with above values.
  //
  TscLocation = TscInitFunc;
  
  TscLocation = PatchMovVar (TscLocation, Start, DataSection, TextSection, Patcher->Is32Bit, BusFreq, busFreqValue);
  TscLocation = PatchMovVar (TscLocation, Start, DataSection, TextSection, Patcher->Is32Bit, BusFCvtt2n, busFCvtt2nValue);
  TscLocation = PatchMovVar (TscLocation, Start, DataSection, TextSection, Patcher->Is32Bit, BusFCvtn2t, busFCvtn2tValue);
  TscLocation = PatchMovVar (TscLocation, Start, DataSection, TextSection, Patcher->Is32Bit, TscFreq, tscFreqValue);
  TscLocation = PatchMovVar (TscLocation, Start, DataSection, TextSection, Patcher->Is32Bit, TscFCvtt2n, tscFCvtt2nValue);
  TscLocation = PatchMovVar (TscLocation, Start, DataSection, TextSection, Patcher->Is32Bit, TscFCvtn2t, tscFCvtn2tValue);
  TscLocation = PatchMovVar (TscLocation, Start, DataSection, TextSection, Patcher->Is32Bit, TscGranularity, tscGranularityValue);
  TscLocation = PatchMovVar (TscLocation, Start, DataSection, TextSection, Patcher->Is32Bit, BusFreq, busFreqValue);

  if (Patcher->Is32Bit) {
    // TODO
  } else {
    //
    // mov rdi, FSB freq
    //
    *TscLocation++ = 0x48;
    *TscLocation++ = 0xBF;
    CopyMem (TscLocation, &busFreqValue, sizeof (busFreqValue));
    TscLocation += sizeof (busFreqValue);

    //
    // mov rsi, TSC freq
    //
    *TscLocation++ = 0x48;
    *TscLocation++ = 0xBE;
    CopyMem (TscLocation, &tscFreqValue, sizeof (tscFreqValue));
    TscLocation += sizeof (tscFreqValue);

    //
    // call _tmrCvt
    //
    Delta = (INT32) (TmrCvtFunc - (TscLocation + 5));
    *TscLocation++ = 0xE8;
    CopyMem (TscLocation, &Delta, sizeof (Delta));
    TscLocation += sizeof (Delta);

    //
    // mov [_bus2tsc], rax
    //
    LocationAddr = (TscLocation - Start) + TextSection->Section64.Address - TextSection->Section64.Offset;
    VarAddr64    = (Bus2Tsc - Start) + DataSection->Section64.Address - DataSection->Section64.Offset;
    Delta        = (INT32) (VarAddr64 - (LocationAddr + 7));

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
        DEBUG ((DEBUG_INFO, "OCAK: Failed to find CPU MSR 0x35 default value patch - %r\n", Status));
      }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping CPU MSR 0x35 default value patch on %u\n", KernelVersion));
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
      DEBUG ((DEBUG_INFO, "OCAK: Failed to find CPU topology validation patch - %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping CPU topology validation patch on %u\n", KernelVersion));
  }

  return EFI_SUCCESS;
}
