/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "ocvalidate.h"

UINT32
CheckBooter (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32            ErrorCount;
  UINT32            Index;
  OC_BOOTER_CONFIG  *UserBooter;
  OC_UEFI_CONFIG    *UserUefi;
  CONST CHAR8       *Comment;
  CONST CHAR8       *Arch;
  CONST CHAR8       *Identifier;
  CONST CHAR8       *Driver;
  CONST UINT8       *Find;
  UINT32            FindSize;
  CONST UINT8       *Replace;
  UINT32            ReplaceSize;
  CONST UINT8       *Mask;
  UINT32            MaskSize;
  CONST UINT8       *ReplaceMask;
  UINT32            ReplaceMaskSize;
  UINT8             MaxSlide;
  BOOLEAN           IsMmioWhitelistEnabled;
  BOOLEAN           ShouldEnableDevirtualiseMmio;
  BOOLEAN           IsDevirtualiseMmioEnabled;
  BOOLEAN           IsAllowRelocationBlockEnabled;
  BOOLEAN           IsProvideCustomSlideEnabled;
  BOOLEAN           IsEnableSafeModeSlideEnabled;
  BOOLEAN           IsDisableVariableWriteEnabled;
  BOOLEAN           IsEnableWriteUnprotectorEnabled;
  BOOLEAN           HasOpenRuntimeEfiDriver;
  
  DEBUG ((DEBUG_VERBOSE, "config loaded into Booter checker!\n"));

  ErrorCount                      = 0;
  UserBooter                      = &Config->Booter;
  UserUefi                        = &Config->Uefi;
  IsMmioWhitelistEnabled          = FALSE;
  ShouldEnableDevirtualiseMmio    = FALSE;
  IsDevirtualiseMmioEnabled       = UserBooter->Quirks.DevirtualiseMmio;
  IsAllowRelocationBlockEnabled   = UserBooter->Quirks.AllowRelocationBlock;
  IsProvideCustomSlideEnabled     = UserBooter->Quirks.ProvideCustomSlide;
  IsEnableSafeModeSlideEnabled    = UserBooter->Quirks.EnableSafeModeSlide;
  IsDisableVariableWriteEnabled   = UserBooter->Quirks.DisableVariableWrite;
  IsEnableWriteUnprotectorEnabled = UserBooter->Quirks.EnableWriteUnprotector;
  HasOpenRuntimeEfiDriver         = FALSE;
  MaxSlide                        = UserBooter->Quirks.ProvideMaxSlide;
  
  for (Index = 0; Index < UserBooter->MmioWhitelist.Count; ++Index) {
    Comment                = OC_BLOB_GET (&UserBooter->MmioWhitelist.Values[Index]->Comment);
    IsMmioWhitelistEnabled = UserBooter->MmioWhitelist.Values[Index]->Enabled;

    //
    // Sanitise strings.
    //
    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Booter->MmioWhitelist[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (IsMmioWhitelistEnabled) {
      ShouldEnableDevirtualiseMmio = TRUE;
    }
  }

  for (Index = 0; Index < UserBooter->Patch.Count; ++Index) {
    Comment         = OC_BLOB_GET (&UserBooter->Patch.Values[Index]->Comment);
    Arch            = OC_BLOB_GET (&UserBooter->Patch.Values[Index]->Arch);
    Identifier      = OC_BLOB_GET (&UserBooter->Patch.Values[Index]->Identifier);
    Find            = OC_BLOB_GET (&UserBooter->Patch.Values[Index]->Find);
    FindSize        = UserBooter->Patch.Values[Index]->Find.Size;
    Replace         = OC_BLOB_GET (&UserBooter->Patch.Values[Index]->Replace);
    ReplaceSize     = UserBooter->Patch.Values[Index]->Replace.Size;
    Mask            = OC_BLOB_GET (&UserBooter->Patch.Values[Index]->Mask);
    MaskSize        = UserBooter->Patch.Values[Index]->Mask.Size;
    ReplaceMask     = OC_BLOB_GET (&UserBooter->Patch.Values[Index]->ReplaceMask);
    ReplaceMaskSize = UserBooter->Patch.Values[Index]->ReplaceMask.Size;

    //
    // Sanitise strings.
    //
    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Booter->Patch[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiArchIsLegal (Arch)) {
      DEBUG ((DEBUG_WARN, "Booter->Patch[%u]->Arch is borked (Can only be Any, i386, and x86_64)!\n", Index));
      ++ErrorCount;
    }
    if (!AsciiIdentifierIsLegal (Identifier)) {
      DEBUG ((DEBUG_WARN, "Booter->Patch[%u]->Identifier contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (Identifier[0] != '\0'
      && AsciiStrCmp (Identifier, "Any") != 0
      && AsciiStrCmp (Identifier, "Apple") != 0
      && !AsciiFileNameHasSuffix (Identifier, "efi")) {
      DEBUG ((
        DEBUG_WARN,
        "Booter->Patch[%u]->Identifier has illegal value: %a (Can only be Empty string/Any, Apple, or a specified bootloader with .efi sufffix)\n",
        Index,
        Identifier
        ));
      ++ErrorCount;
    }

    //
    // Checks for size.
    //
    if (FindSize != ReplaceSize) {
      DEBUG ((
        DEBUG_WARN,
        "Booter->Patch[%u] has different Find and Replace size (%u vs %u)!\n",
        Index,
        FindSize,
        ReplaceSize
        ));
      ++ErrorCount;
    }
    if (MaskSize > 0) {
      if (MaskSize != FindSize) {
        DEBUG ((
          DEBUG_WARN,
          "Booter->Patch[%u] has Mask set but its size is different from Find (%u vs %u)!\n",
          Index,
          MaskSize,
          FindSize
          ));
        ++ErrorCount;
      } else {
        if (!DataHasProperMasking (Find, Mask, FindSize)) {
          DEBUG ((DEBUG_WARN, "Booter->Patch[%u]->Find requires Mask to be active for corresponding bits!\n", Index));
          ++ErrorCount;
        }
      }
    }
    if (ReplaceMaskSize > 0) {
      if (ReplaceMaskSize != ReplaceSize) {
        DEBUG ((
          DEBUG_WARN,
          "Booter->Patch[%u] has ReplaceMask set but its size is different from Replace (%u vs %u)!\n",
          Index,
          ReplaceMaskSize,
          ReplaceSize
          ));
        ++ErrorCount;
      } else {
        if (!DataHasProperMasking (Replace, ReplaceMask, ReplaceSize)) {
          DEBUG ((DEBUG_WARN, "Booter->Patch[%u]->Replace requires ReplaceMask to be active for corresponding bits!\n", Index));
          ++ErrorCount;
        }
      }
    }
  }

  for (Index = 0; Index < UserUefi->Drivers.Count; ++Index) {
    Driver = OC_BLOB_GET (UserUefi->Drivers.Values[Index]);

    //
    // Sanitise strings.
    //
    if (!AsciiUefiDriverIsLegal (Driver)) {
      DEBUG ((DEBUG_WARN, "UEFI->Drivers[%u] contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (AsciiStrCmp (Driver, "OpenRuntime.efi") == 0) {
      HasOpenRuntimeEfiDriver = TRUE;
    }
  }

  if (ShouldEnableDevirtualiseMmio && !IsDevirtualiseMmioEnabled) {
    DEBUG ((DEBUG_WARN, "There are enabled entries under Booter->MmioWhitelist, but DevirtualiseMmio is not enabled!\n"));
    ++ErrorCount;
  }
  if (!HasOpenRuntimeEfiDriver) {
    if (IsProvideCustomSlideEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->ProvideCustomSlide is enabled, but OpenRuntime.efi is not loaded at UEFI->Drivers!\n"));
      ++ErrorCount;
    }
    if (IsDisableVariableWriteEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->DisableVariableWrite is enabled, but OpenRuntime.efi is not loaded at UEFI->Drivers!\n"));
      ++ErrorCount;
    }
    if (IsEnableWriteUnprotectorEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->EnableWriteUnprotector is enabled, but OpenRuntime.efi is not loaded at UEFI->Drivers!\n"));
      ++ErrorCount;
    }
  }
  if (!IsProvideCustomSlideEnabled) {
    if (IsAllowRelocationBlockEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->AllowRelocationBlock is enabled, but ProvideCustomSlide is not enabled altogether!\n"));
      ++ErrorCount;
    }
    if (IsEnableSafeModeSlideEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->EnableSafeModeSlide is enabled, but ProvideCustomSlide is not enabled altogether!\n"));
      ++ErrorCount;
    }
    if (MaxSlide > 0) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->ProvideMaxSlide is set to %u, but ProvideCustomSlide is not enabled altogether!\n", MaxSlide));
      ++ErrorCount;
    }
  }

  return ReportError (__func__, ErrorCount);
}
