/** @file
  Copyright (C) 2018, Downlod-Fritz. All rights reserved.
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "BootCompatInternal.h"

#include <Guid/AppleVariable.h>
#include <Guid/OcVariable.h>
#include <IndustryStandard/AppleHibernate.h>
#include <IndustryStandard/AppleCsrConfig.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcCryptoLib.h>
#include <Library/OcDeviceTreeLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcMemoryLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcRngLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  Obtain estimated kernel area start and end addresses for
  specified slide number.

  @param[in]  EstimatedKernelArea  Estimated kernel area size.
  @param[in]  HasSandyOrIvy        CPU type.
  @param[in]  Slide                Slide number.
  @param[out] StartAddr            Starting address.
  @param[out] EndAddr              Ending address (not inclusive).
**/
STATIC
VOID
GetSlideRangeForValue (
  IN  UINTN                EstimatedKernelArea,
  IN  BOOLEAN              HasSandyOrIvy,
  IN  UINT8                Slide,
  OUT UINTN                *StartAddr,
  OUT UINTN                *EndAddr
  )
{
  *StartAddr = Slide * SLIDE_GRANULARITY + KERNEL_BASE_PADDR;

  //
  // Skip ranges used by Intel HD 2000/3000.
  //
  if (Slide >= SLIDE_ERRATA_NUM && HasSandyOrIvy) {
    *StartAddr += SLIDE_ERRATA_SKIP_RANGE;
  }

  *EndAddr = *StartAddr + EstimatedKernelArea;
}

/**
  Generate more or less random slide value.

  @param[in]  SlideSupport  Slide support state.
**/
STATIC
UINT8
GenerateSlideValue (
  IN  SLIDE_SUPPORT_STATE  *SlideSupport
  )
{
  UINT32  Slide;

  //
  // Handle 0 slide case.
  //
  if (SlideSupport->ValidSlideCount == 1) {
    return SlideSupport->ValidSlides[0];
  }

  do {
    DivU64x32Remainder (GetPseudoRandomNumber64 (), SlideSupport->ValidSlideCount, &Slide);
  } while (SlideSupport->ValidSlides[Slide] == 0);

  return SlideSupport->ValidSlides[Slide];
}

/**
  Decide on whether to use custom slide based on memory map analysis.
  This additionally logs the decision through standard services.

  @param[in,out]  SlideSupport      Slide support state.
  @param[in]      FallbackSlide     Fallback slide number with largest area.
  @param[in]      MaxAvailableSize  Maximum available contiguous area.

  @retval  TRUE in case custom slide is to be used.
**/
STATIC
BOOLEAN
ShouldUseCustomSlideOffsetDecision (
  IN OUT SLIDE_SUPPORT_STATE  *SlideSupport,
  IN     UINT8                FallbackSlide,
  IN     UINT64               MaxAvailableSize
  )
{
  UINTN  Index;
  UINTN  NumEntries;
  CHAR8  SlideList[256];
  CHAR8  Temp[32];

  //
  // All slides are available.
  //
  if (SlideSupport->ValidSlideCount == TOTAL_SLIDE_NUM) {
    DEBUG ((
      DEBUG_INFO,
      "OCABC: All slides are usable! You can disable ProvideCustomSlide!\n"
      ));
    return FALSE;
  }

  //
  // No slides are available, fallback to largest.
  //
  if (SlideSupport->ValidSlideCount == 0) {
    DEBUG ((
      DEBUG_INFO,
      "OCABC: No slide values are usable! Falling back to %u with 0x%08LX bytes!\n",
      (UINT32) FallbackSlide,
      MaxAvailableSize
      ));
    SlideSupport->ValidSlides[SlideSupport->ValidSlideCount++] = (UINT8) FallbackSlide;
    return TRUE;
  }

  //
  // Not all slides are available and thus we have to pass a custom slide
  // value through boot-args to boot reliably.
  //
  // Pretty-print valid slides as ranges.
  // For example, 1, 2, 3, 4, 5 will become 1-5.
  //
  DEBUG ((
    DEBUG_INFO,
    "OCABC: Only %u/%u slide values are usable!\n",
    (UINT32) SlideSupport->ValidSlideCount,
    (UINT32) TOTAL_SLIDE_NUM
    ));

  SlideList[0] = '\0';

  NumEntries = 0;
  for (Index = 0; Index <= SlideSupport->ValidSlideCount; ++Index) {
    if (Index == 0) {
      AsciiSPrint (
        Temp,
        sizeof (Temp),
        "Valid slides - %d",
        SlideSupport->ValidSlides[Index]
        );
      AsciiStrCatS (SlideList, sizeof (SlideList), Temp);
    } else if (Index == SlideSupport->ValidSlideCount
      || SlideSupport->ValidSlides[Index - 1] + 1 != SlideSupport->ValidSlides[Index]) {

      if (NumEntries == 1) {
        AsciiSPrint (
          Temp,
          sizeof (Temp),
          ", %d",
          SlideSupport->ValidSlides[Index - 1]
          );
        AsciiStrCatS (SlideList, sizeof (SlideList), Temp);
      } else if (NumEntries > 1) {
        AsciiSPrint (
          Temp,
          sizeof (Temp),
          "-%d",
          SlideSupport->ValidSlides[Index - 1]
          );
        AsciiStrCatS (SlideList, sizeof (SlideList), Temp);
      }

      if (Index != SlideSupport->ValidSlideCount) {
        AsciiSPrint (
          Temp,
          sizeof (Temp),
          ", %d",
          SlideSupport->ValidSlides[Index]
          );
        AsciiStrCatS (SlideList, sizeof (SlideList), Temp);
      }

      NumEntries = 0;
    } else {
      NumEntries++;
    }
  }

  DEBUG ((DEBUG_INFO, "OCABC: %a\n", SlideList));

  return TRUE;
}

/**
  Return cached decision or perform memory map analysis to decide
  whether to use custom slide for reliable kernel booting or not.

  @param[in,out]  SlideSupport      Slide support state.
  @param[in]      GetMemoryMap      Function to get current memory map for analysis. optional.
  @param[in]      FilterMap         Function to filter returned memory map, optional.
  @param[in]      FilterMapContext  Filter map context, optional.

  @retval  TRUE in case custom slide is to be used.
**/
STATIC
BOOLEAN
ShouldUseCustomSlideOffset (
  IN OUT SLIDE_SUPPORT_STATE   *SlideSupport,
  IN     EFI_GET_MEMORY_MAP    GetMemoryMap       OPTIONAL,
  IN     OC_MEMORY_FILTER      FilterMap          OPTIONAL,
  IN     VOID                  *FilterMapContext  OPTIONAL
  )
{
  EFI_PHYSICAL_ADDRESS   AllocatedMapPages;
  UINTN                  MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  EFI_MEMORY_DESCRIPTOR  *Desc;
  UINTN                  MapKey;
  EFI_STATUS             Status;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  OC_CPU_GENERATION      CpuGeneration;
  UINTN                  Index;
  UINTN                  Slide;
  UINTN                  NumEntries;
  UINT64                 MaxAvailableSize;
  UINT8                  FallbackSlide;
  BOOLEAN                Supported;
  UINTN                  StartAddr;
  UINTN                  EndAddr;
  EFI_PHYSICAL_ADDRESS   DescEndAddr;
  UINT64                 AvailableSize;

  MaxAvailableSize = 0;
  FallbackSlide    = 0;

  if (SlideSupport->HasMemoryMapAnalysis) {
    return SlideSupport->ValidSlideCount > 0
      && SlideSupport->ValidSlideCount < TOTAL_SLIDE_NUM;
  }

  AllocatedMapPages = BASE_4GB;
  Status = OcGetCurrentMemoryMapAlloc (
    &MemoryMapSize,
    &MemoryMap,
    &MapKey,
    &DescriptorSize,
    &DescriptorVersion,
    GetMemoryMap,
    &AllocatedMapPages
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCABC: Failed to obtain memory map for KASLR - %r\n", Status));
    return FALSE;
  }

  if (FilterMap != NULL) {
    FilterMap (FilterMapContext, MemoryMapSize, MemoryMap, DescriptorSize);
  }

  CpuGeneration = OcCpuGetGeneration ();
  SlideSupport->HasSandyOrIvy = CpuGeneration == OcCpuGenerationSandyBridge ||
                                CpuGeneration == OcCpuGenerationIvyBridge;

  SlideSupport->EstimatedKernelArea = (UINTN) EFI_PAGES_TO_SIZE (
    OcCountRuntimePages (MemoryMapSize, MemoryMap, DescriptorSize, NULL)
    ) + ESTIMATED_KERNEL_SIZE;

  //
  // At this point we have a memory map that we could use to
  // determine what slide values are allowed.
  //
  NumEntries = MemoryMapSize / DescriptorSize;

  //
  // Reset valid slides to zero and find actually working ones.
  //
  SlideSupport->ValidSlideCount = 0;

  for (Slide = 0; Slide < TOTAL_SLIDE_NUM; ++Slide) {
    Desc      = MemoryMap;
    Supported = TRUE;

    GetSlideRangeForValue (
      SlideSupport->EstimatedKernelArea,
      SlideSupport->HasSandyOrIvy,
      (UINT8) Slide,
      &StartAddr,
      &EndAddr
      );

    AvailableSize = 0;

    for (Index = 0; Index < NumEntries; ++Index) {
      if (Desc->NumberOfPages == 0) {
        continue;
      }

      DescEndAddr = LAST_DESCRIPTOR_ADDR (Desc) + 1;

      if ((Desc->PhysicalStart < EndAddr) && (DescEndAddr > StartAddr)) {
        //
        // The memory overlaps with the slide region.
        //
        if (Desc->Type != EfiConventionalMemory) {
          //
          // The memory is unusable atm.
          //
          Supported = FALSE;
          break;
        } else {
          //
          // The memory will be available for the kernel.
          //
          AvailableSize += EFI_PAGES_TO_SIZE (Desc->NumberOfPages);

          if (Desc->PhysicalStart < StartAddr) {
            //
            // The region starts before the slide region.
            // Subtract the memory that is located before the slide region.
            //
            AvailableSize -= (StartAddr - Desc->PhysicalStart);
          }

          if (DescEndAddr > EndAddr) {
            //
            // The region ends after the slide region.
            // Subtract the memory that is located after the slide region.
            //
            AvailableSize -= (DescEndAddr - EndAddr);
          }
        }
      }

      Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
    }

    if (AvailableSize > MaxAvailableSize) {
      MaxAvailableSize = AvailableSize;
      FallbackSlide    = (UINT8) Slide;
    }

    //
    // Stop evalutating slides after exceeding ProvideMaxSlide, may break when
    // no slides are available.
    //
    if (SlideSupport->ProvideMaxSlide > 0 && Slide > SlideSupport->ProvideMaxSlide) {
      break;
    }

    if ((StartAddr + AvailableSize) != EndAddr) {
      //
      // The slide region is not continuous.
      //
      Supported = FALSE;
    }

    if (Supported) {
      SlideSupport->ValidSlides[SlideSupport->ValidSlideCount++] = (UINT8) Slide;
    }
  }

  //
  // Okay, we are done.
  //

  SlideSupport->HasMemoryMapAnalysis = TRUE;

  gBS->FreePages (
    (EFI_PHYSICAL_ADDRESS)(UINTN) MemoryMap,
    (UINTN) AllocatedMapPages
    );

  return ShouldUseCustomSlideOffsetDecision (
    SlideSupport,
    FallbackSlide,
    MaxAvailableSize
    );
}

/**
  UEFI GetVariable override specific to csr-active-config.
  See caller for more details.

  @param[in,out]  SlideSupport  Slide support state.
  @param[in]      GetVariable   Original UEFI GetVariable service.
  @param[in]      VariableName  GetVariable variable name argument.
  @param[in]      VendorGuid    GetVariable vendor GUID argument.
  @param[out]     Attributes    GetVariable attributes argument.
  @param[in,out]  DataSize      GetVariable data size argument.
  @param[out]     Data          GetVariable data argument.

  @retval GetVariable status code.
**/
STATIC
EFI_STATUS
GetVariableCsrActiveConfig (
  IN OUT SLIDE_SUPPORT_STATE  *SlideSupport,
  IN     EFI_GET_VARIABLE     GetVariable,
  IN     CHAR16               *VariableName,
  IN     EFI_GUID             *VendorGuid,
  OUT    UINT32               *Attributes  OPTIONAL,
  IN OUT UINTN                *DataSize,
  OUT    VOID                 *Data
  )
{
  EFI_STATUS  Status;
  UINT32      *Config;

  //
  // If we were asked for the size, just return it right away.
  //
  if (Data == NULL || *DataSize < sizeof (UINT32)) {
    *DataSize = sizeof (UINT32);
    return EFI_BUFFER_TOO_SMALL;
  }

  Config = (UINT32 *) Data;

  //
  // Otherwise call the original function.
  //
  Status = GetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCABC: GetVariable csr-active-config - %r\n", Status));

    *Config = 0;
    Status = EFI_SUCCESS;
    if (Attributes != NULL) {
      *Attributes =
        EFI_VARIABLE_BOOTSERVICE_ACCESS |
        EFI_VARIABLE_RUNTIME_ACCESS |
        EFI_VARIABLE_NON_VOLATILE;
    }
  }

  //
  // We must unrestrict NVRAM from SIP or slide=X will not be supported.
  //
  SlideSupport->CsrActiveConfig    = *Config;
  SlideSupport->HasCsrActiveConfig = TRUE;
  *Config |= CSR_ALLOW_UNRESTRICTED_NVRAM;

  return Status;
}

/**
  UEFI GetVariable override specific to boot-args.
  See caller for more details.

  @param[in,out]  SlideSupport  Slide support state.
  @param[in]      GetVariable   Original UEFI GetVariable service.
  @param[in]      VariableName  GetVariable variable name argument.
  @param[in]      VendorGuid    GetVariable vendor GUID argument.
  @param[out]     Attributes    GetVariable attributes argument.
  @param[in,out]  DataSize      GetVariable data size argument.
  @param[out]     Data          GetVariable data argument.

  @retval GetVariable status code.
**/
STATIC
EFI_STATUS
GetVariableBootArgs (
  IN OUT SLIDE_SUPPORT_STATE  *SlideSupport,
  IN     EFI_GET_VARIABLE     GetVariable,
  IN     CHAR16               *VariableName,
  IN     EFI_GUID             *VendorGuid,
  OUT    UINT32               *Attributes  OPTIONAL,
  IN OUT UINTN                *DataSize,
  OUT    VOID                 *Data
  )
{
  EFI_STATUS  Status;
  UINTN       StoredBootArgsSize;
  UINT8       Slide;
  CHAR8       SlideArgument[10];
  UINTN       SlideArgumentLength;

  StoredBootArgsSize  = BOOT_LINE_LENGTH;
  SlideArgumentLength = ARRAY_SIZE (SlideArgument) - 1;

  if (!SlideSupport->HasBootArgs) {
    Slide  = GenerateSlideValue (SlideSupport);

    //
    // boot-args normally arrives non-null terminated.
    //
    Status = GetVariable (
      VariableName,
      VendorGuid,
      Attributes,
      &StoredBootArgsSize,
      SlideSupport->BootArgs
      );
    if (EFI_ERROR (Status)) {
      SlideSupport->BootArgs[0] = '\0';
    }

    //
    // Note, the point is to always pass 3 characters to avoid side attacks on value length.
    // boot.efi always reads in decimal, so 008 and 8 are equivalent.
    //
    AsciiSPrint (SlideArgument, ARRAY_SIZE (SlideArgument), "slide=%-03d", Slide);

    if (!OcAppendArgumentToCmd (NULL, SlideSupport->BootArgs, SlideArgument, SlideArgumentLength)) {
      //
      // Broken boot-args, try to overwrite.
      //
      AsciiStrnCpyS (
        SlideSupport->BootArgs,
        SlideArgumentLength + 1,
        SlideArgument,
        SlideArgumentLength + 1
        );
    }

    SlideSupport->BootArgsSize = AsciiStrLen (SlideSupport->BootArgs);
    SlideSupport->HasBootArgs  = TRUE;
  }

  if (Attributes) {
    *Attributes =
      EFI_VARIABLE_BOOTSERVICE_ACCESS |
      EFI_VARIABLE_RUNTIME_ACCESS |
      EFI_VARIABLE_NON_VOLATILE;
  }

  if (*DataSize >= SlideSupport->BootArgsSize && Data != NULL) {
    CopyMem (
      Data,
      SlideSupport->BootArgs,
      SlideSupport->BootArgsSize
      );
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_BUFFER_TOO_SMALL;
  }

  *DataSize = SlideSupport->BootArgsSize;

  return Status;
}

/**
  Erases customised slide value from everywhere accessible
  for security purposes.

  @param[in,out]  SlideSupport  Slide support state.
  @param[in,out]  BootArgs      Apple kernel boot arguments.
**/
STATIC
VOID
HideSlideFromOs (
  IN OUT SLIDE_SUPPORT_STATE   *SlideSupport,
  IN OUT OC_BOOT_ARGUMENTS     *BootArgs
  )
{
  EFI_STATUS  Status;
  DTEntry     Chosen;
  CHAR8       *ArgsStr;
  UINT32      ArgsSize;

  //
  // First, there is a BootArgs entry for XNU.
  //
  OcRemoveArgumentFromCmd (BootArgs->CommandLine, "slide=");

  //
  // Second, there is a DT entry.
  //
  DTInit ((VOID *)(UINTN) (*BootArgs->DeviceTreeP), BootArgs->DeviceTreeLength);
  Status = DTLookupEntry (NULL, "/chosen", &Chosen);
  if (!EFI_ERROR (Status)) {
    Status = DTGetProperty (Chosen, "boot-args", (VOID **)&ArgsStr, &ArgsSize);
    if (!EFI_ERROR (Status) && ArgsSize > 0) {
      OcRemoveArgumentFromCmd (ArgsStr, "slide=");
    }
  }

  //
  // Third, clean the boot args just in case.
  //
  SlideSupport->ValidSlideCount = 0;
  SlideSupport->BootArgsSize    = 0;
  SecureZeroMem (SlideSupport->ValidSlides, sizeof (SlideSupport->ValidSlides));
  SecureZeroMem (SlideSupport->BootArgs, sizeof (SlideSupport->BootArgs));
}

VOID
AppleSlideUnlockForSafeMode (
  IN OUT UINT8  *ImageBase,
  IN     UINTN  ImageSize
  )
{
  //
  // boot.efi performs the following check:
  // if (State & (BOOT_MODE_SAFE | BOOT_MODE_ASLR)) == (BOOT_MODE_SAFE | BOOT_MODE_ASLR)) {
  //   * Disable KASLR *
  // }
  // We do not care about the asm it will use for it, but we could assume that the constants
  // will be used twice and their location will be very close to each other.
  //
  // BOOT_MODE_SAFE | BOOT_MODE_ASLR constant is 0x4001 in hex.
  // It has not changed since its appearance, so is most likely safe to look for.
  // Furthermore, since boot.efi state mask uses higher bits, it is safe to assume that
  // the comparison will be at least 32-bit.
  //
  //
  // The new way patch is a workaround for 10.13.5 and newer, where the code got finally changed.
  // if (State & BOOT_MODE_SAFE) {
  //   ReportFeature(FEATURE_BOOT_MODE_SAFE);
  //   if (State & BOOT_MODE_ASLR) {
  //     * Disable KASLR *
  //   }
  // }
  //

  //
  // This is a reasonable maximum distance to expect between the instructions.
  //
  STATIC CONST UINTN MaxDist         = 0x10;
  STATIC CONST UINT8 SearchSeqNew[]  = {0xF6, 0xC4, 0x40, 0x75};
  STATIC CONST UINT8 SearchSeqNew2[] = {0x0F, 0xBA, 0xE0, 0x0E, 0x72};
  STATIC CONST UINT8 SearchSeq[]     = {0x01, 0x40, 0x00, 0x00};

  UINT8       *StartOff;
  UINT8       *EndOff;
  UINTN       FirstOff;
  UINTN       SecondOff;
  UINTN       SearchSeqNewSize;
  BOOLEAN     NewWay;


  StartOff = ImageBase;
  EndOff   = StartOff + ImageSize - sizeof (SearchSeq) - MaxDist;

  FirstOff  = 0;
  SecondOff = 0;
  NewWay    = FALSE;

  do {
    while (StartOff + FirstOff <= EndOff) {
      if (StartOff + FirstOff <= EndOff - 1
       && CompareMem (StartOff + FirstOff, SearchSeqNew2, sizeof (SearchSeqNew2)) == 0) {
        SearchSeqNewSize = sizeof (SearchSeqNew2);
        NewWay = TRUE;
        break;
      } else if (CompareMem (StartOff + FirstOff, SearchSeqNew, sizeof (SearchSeqNew)) == 0) {
        SearchSeqNewSize = sizeof (SearchSeqNew);
        NewWay = TRUE;
        break;
      } else if (CompareMem (StartOff + FirstOff, SearchSeq, sizeof (SearchSeq)) == 0) {
        break;
      }
      FirstOff++;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "OCABC: Found first %d at off %X\n",
      (UINT32) NewWay,
      (UINT32) FirstOff
      ));

    if (StartOff + FirstOff > EndOff) {
      DEBUG ((
        DEBUG_INFO,
        "OCABC: Failed to find first BOOT_MODE_SAFE | BOOT_MODE_ASLR sequence\n"
        ));
      break;
    }

    if (NewWay) {
      //
      // Here we just patch the comparison code and the check by straight nopping.
      //
      DEBUG ((DEBUG_VERBOSE, "OCABC: Patching new safe mode aslr check...\n"));
      SetMem (StartOff + FirstOff, SearchSeqNewSize + 1, 0x90);
      return;
    }

    SecondOff = FirstOff + sizeof (SearchSeq);

    while (
      StartOff + SecondOff <= EndOff && FirstOff + MaxDist >= SecondOff &&
      CompareMem (StartOff + SecondOff, SearchSeq, sizeof (SearchSeq))) {
      SecondOff++;
    }

    DEBUG ((DEBUG_VERBOSE, "OCABC: Found second at off %X\n", (UINT32) SecondOff));

    if (FirstOff + MaxDist < SecondOff) {
      DEBUG ((DEBUG_VERBOSE, "OCABC: Trying next match...\n"));
      SecondOff = 0;
      FirstOff += sizeof (SearchSeq);
    }
  } while (SecondOff == 0);

  if (SecondOff != 0) {
    //
    // Here we use 0xFFFFFFFF constant as a replacement value.
    // Since the state values are contradictive (e.g. safe & single at the same time)
    // We are allowed to use this instead of to simulate if (false).
    //
    DEBUG ((DEBUG_VERBOSE, "OCABC: Patching safe mode aslr check...\n"));
    SetMem (StartOff + FirstOff, sizeof (SearchSeq), 0xFF);
    SetMem (StartOff + SecondOff, sizeof (SearchSeq), 0xFF);
  }
}

EFI_STATUS
AppleSlideGetVariable (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat,
  IN     EFI_GET_VARIABLE      GetVariable,
  IN     EFI_GET_MEMORY_MAP    GetMemoryMap  OPTIONAL,
  IN     OC_MEMORY_FILTER      FilterMap     OPTIONAL,
  IN     VOID                  *FilterMapContext  OPTIONAL,
  IN     CHAR16                *VariableName,
  IN     EFI_GUID              *VendorGuid,
     OUT UINT32                *Attributes   OPTIONAL,
  IN OUT UINTN                 *DataSize,
     OUT VOID                  *Data
  )
{
  BootCompat->SlideSupport.ProvideMaxSlide =  BootCompat->Settings.ProvideMaxSlide;

  if (VariableName != NULL && VendorGuid != NULL && DataSize != NULL
    && CompareGuid (VendorGuid, &gAppleBootVariableGuid)) {

    if (StrCmp (VariableName, L"csr-active-config") == 0) {
      //
      // We override csr-active-config with CSR_ALLOW_UNRESTRICTED_NVRAM bit set
      // to allow one to pass a custom slide value even when SIP is on.
      // This original value of csr-active-config is returned to OS at XNU boot.
      // This allows SIP to be fully enabled in the operating system.
      //
      return GetVariableCsrActiveConfig (
        &BootCompat->SlideSupport,
        GetVariable,
        VariableName,
        VendorGuid,
        Attributes,
        DataSize,
        Data
        );
    } else if (StrCmp (VariableName, L"boot-args") == 0
      && !BootCompat->ServiceState.AppleCustomSlide
      && ShouldUseCustomSlideOffset (&BootCompat->SlideSupport, GetMemoryMap, FilterMap, FilterMapContext)) {
      //
      // When we cannot allow some KASLR values due to used address we generate
      // a random slide value among the valid options, which we we pass via boot-args.
      // See ShouldUseCustomSlideOffset for more details.
      //
      // We delay memory map analysis as much as we can, in case boot.efi or anything else allocates
      // stuff with gBS->AllocatePool and it overlaps with the kernel area.
      // Overriding AllocatePool with a custom allocator does not really improve the situation,
      // because on older boards allocated memory above BASE_4GB causes instant reboots, and
      // on the only (so far) problematic X99 and X299 we have no free region for our pool anyway.
      // In any case, the current APTIOFIX_SPECULATED_KERNEL_SIZE value appears to work reliably.
      //
      return GetVariableBootArgs (
        &BootCompat->SlideSupport,
        GetVariable,
        VariableName,
        VendorGuid,
        Attributes,
        DataSize,
        Data
        );
    }
  }

  return GetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
}

VOID
AppleSlideRestore (
  IN OUT BOOT_COMPAT_CONTEXT   *BootCompat,
  IN OUT OC_BOOT_ARGUMENTS     *BootArgs
  )
{
  SLIDE_SUPPORT_STATE  *SlideSupport;

  SlideSupport = &BootCompat->SlideSupport;

  //
  // Restore csr-active-config to a value it was before our slide=X alteration.
  //
  if (BootArgs->CsrActiveConfig != NULL && SlideSupport->HasCsrActiveConfig) {
    *BootArgs->CsrActiveConfig = SlideSupport->CsrActiveConfig;
  }

  //
  // Having slide=X values visible in the operating system defeats the purpose of KASLR.
  // Since our custom implementation works by passing random KASLR slide via boot-args,
  // this is especially important.
  //
  HideSlideFromOs (SlideSupport, BootArgs);
}
