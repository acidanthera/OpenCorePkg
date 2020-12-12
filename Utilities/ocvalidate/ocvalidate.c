/** @file
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/OcTemplateLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcConfigurationLib.h>

#include <File.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>

/*
 for fuzzing (TODO):
 clang-mp-7.0 -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../MdePkg/Include/ -include ../Include/Base.h ConfigValidty.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c ../../Library/OcConfigurationLib/OcConfigurationLib.c -o ConfigValidty
 rm -rf DICT fuzz*.log ; mkdir DICT ; cp ConfigValidty.plist DICT ; ./ConfigValidty -jobs=4 DICT

 rm -rf ConfigValidty.dSYM DICT fuzz*.log ConfigValidty
*/

//
// TODO: Doc functions and add comments where necessary.
//

typedef
UINT32
(*CONFIG_CHECK) (
  IN  OC_GLOBAL_CONFIG  *Config
  );

STATIC
INT64
GetCurrentTimestamp (
  VOID
  )
{
  struct timeval te;

  //
  // Get current time.
  //
  gettimeofday (&te, NULL);
  //
  // Return milliseconds.
  //
  return te.tv_sec * 1000LL + te.tv_usec / 1000LL;
}

STATIC
CHAR8 *
GetFilenameSuffix (
  IN  CONST CHAR8  *FileName
  )
{
  //
  // Find the last dot by reverse order.
  //
  CONST CHAR8  *SuffixDot;

  SuffixDot = strrchr (FileName, '.');

  //
  // In some weird cases the filename can be crazily like
  // "a.suffix." (note that '.') or "..." (extremely abnormal).
  // We do not support such.
  //
  if (!SuffixDot || SuffixDot == FileName) {
    return (CHAR8 *) "";
  }

  //
  // In most normal cases, return whatever following the dot.
  //
  return (CHAR8 *) SuffixDot + 1;
}

STATIC
BOOLEAN
StringHasAllLegalCharacter (
  IN  CONST CHAR8  *String
  )
{
  // only 0-9, A-Z, a-z, '_', '-', '.', '/', and '\'
  // are accepted.
  UINTN  Index;

  for (Index = 0; Index < AsciiStrLen (String); ++Index) {
    //
    // Skip allowed characters.
    //
    if (isdigit (String[Index])
      || isalpha (String[Index])
      || String[Index] == '_'
      || String[Index] == '-'
      || String[Index] == '.'
      || String[Index] == '/'
      || String[Index] == '\\') {
      continue;
    }

    //
    // Disallowed characters matched.
    //
    return FALSE;
  }

  return TRUE;
}

STATIC
BOOLEAN
StringHasAllPrintableCharacter (
  IN  CONST CHAR8  *String
  )
{
  UINTN  Index;

  for (Index = 0; Index < AsciiStrLen (String); ++Index) {
    //
    // Unprintable characters matched.
    //
    if (isprint (String[Index]) == 0) {
      return FALSE;
    }
  }

  return TRUE;
}

STATIC
UINT32
CheckACPI (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32          ErrorCount;
  UINT32          Index;
  OC_ACPI_CONFIG  UserAcpi;
  CONST CHAR8     *Path;
  CONST CHAR8     *Comment;
  UINT32          FindSize;
  UINT32          ReplaceSize;
  UINT32          MaskSize;
  UINT32          ReplaceMaskSize;
  BOOLEAN         HasCustomDSDT;

  DEBUG ((DEBUG_INFO, "config loaded into ACPI checker!\n"));

  ErrorCount    = 0;
  UserAcpi      = Config->Acpi;
  HasCustomDSDT = FALSE;

  for (Index = 0; Index < UserAcpi.Add.Count; ++Index) {
    Path    = OC_BLOB_GET (&UserAcpi.Add.Values[Index]->Path);
    Comment = OC_BLOB_GET (&UserAcpi.Add.Values[Index]->Comment);

    if (!StringHasAllLegalCharacter (Path)) {
      DEBUG ((DEBUG_WARN, "ACPI->Add[%u]->Path contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!StringHasAllPrintableCharacter (Comment)) {
      DEBUG ((DEBUG_WARN, "ACPI->Add[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (AsciiStrCmp (GetFilenameSuffix (Path), "dsl") == 0) { ///< TODO: case insensitive check maybe?
      DEBUG ((DEBUG_WARN, "ACPI->Add[%u]->Path has .dsl suffix!\n", Index));
      ++ErrorCount;
    }

    if (OcAsciiStriStr (Path, "DSDT") == 0) {
      HasCustomDSDT = TRUE;
    }
  }

  for (Index = 0; Index < UserAcpi.Delete.Count; ++Index) {
    Comment = OC_BLOB_GET (&UserAcpi.Delete.Values[Index]->Comment);

    if (!StringHasAllPrintableCharacter (Comment)) {
      DEBUG ((DEBUG_WARN, "ACPI->Delete[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    //
    // Size of OemTableId and TableSignature cannot be checked,
    // as serialisation kills it.
    //
  }

  for (Index = 0; Index < UserAcpi.Patch.Count; ++Index) {
    Comment         = OC_BLOB_GET (&UserAcpi.Patch.Values[Index]->Comment);
    FindSize        = UserAcpi.Patch.Values[Index]->Find.Size;
    ReplaceSize     = UserAcpi.Patch.Values[Index]->Replace.Size;
    MaskSize        = UserAcpi.Patch.Values[Index]->Mask.Size;
    ReplaceMaskSize = UserAcpi.Patch.Values[Index]->ReplaceMask.Size;

    if (!StringHasAllPrintableCharacter (Comment)) {
      DEBUG ((DEBUG_WARN, "ACPI->Patch[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    //
    // Size of OemTableId and TableSignature cannot be checked,
    // as serialisation kills it.
    //

    if (FindSize != ReplaceSize) {
      DEBUG ((
        DEBUG_WARN,
        "ACPI->Patch[%u] has different Find and Replace size (%u vs %u)!\n",
        Index,
        FindSize,
        ReplaceSize
        ));
      ++ErrorCount;
    }
    if (MaskSize > 0 && MaskSize != FindSize) {
      DEBUG ((DEBUG_WARN,
        "ACPI->Patch[%u] has Mask set but its size is different from Find/Replace (%u vs %u)!\n",
        Index,
        MaskSize,
        FindSize
        ));
      ++ErrorCount;
    }
    if (ReplaceMaskSize > 0 && ReplaceMaskSize != FindSize) {
      DEBUG ((
        DEBUG_WARN,
        "ACPI->Patch[%u] has ReplaceMask set but its size is different from Find/Replace (%u vs %u)!\n",
        Index,
        ReplaceMaskSize,
        FindSize
        ));
      ++ErrorCount;
    }
  }

  if (HasCustomDSDT && !UserAcpi.Quirks.RebaseRegions) {
    DEBUG ((DEBUG_WARN, "ACPI->Quirks->RebaseRegions is not enabled when there is a custom DSDT table!\n"));
    ++ErrorCount;
  }

  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  }
  return ErrorCount;
}

STATIC
UINT32
CheckBooter (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32            ErrorCount;
  UINT32            Index;
  OC_BOOTER_CONFIG  UserBooter;
  CONST CHAR8       *Comment;
  CONST CHAR8       *Arch;
  CONST CHAR8       *Identifier;
  UINT32            FindSize;
  UINT32            ReplaceSize;
  UINT32            MaskSize;
  UINT32            ReplaceMaskSize;
  BOOLEAN           IsMmioWhitelistEnabled;
  BOOLEAN           ShouldEnableDevirtualiseMmio;
  BOOLEAN           IsAllowRelocationBlockEnabled;
  BOOLEAN           IsProvideCustomSlideEnabled;
  BOOLEAN           IsAvoidRuntimeDefragEnabled;
  BOOLEAN           IsEnableSafeModeSlideEnabled;
  
  DEBUG ((DEBUG_INFO, "config loaded into Booter checker!\n"));

  ErrorCount                    = 0;
  UserBooter                    = Config->Booter;
  IsMmioWhitelistEnabled        = FALSE;
  ShouldEnableDevirtualiseMmio  = FALSE;
  IsAllowRelocationBlockEnabled = UserBooter.Quirks.AllowRelocationBlock;
  IsProvideCustomSlideEnabled   = UserBooter.Quirks.ProvideCustomSlide;
  IsAvoidRuntimeDefragEnabled   = UserBooter.Quirks.AvoidRuntimeDefrag;
  IsEnableSafeModeSlideEnabled  = UserBooter.Quirks.EnableSafeModeSlide;
  
  for (Index = 0; Index < UserBooter.MmioWhitelist.Count; ++Index) {
    Comment                = OC_BLOB_GET (&UserBooter.MmioWhitelist.Values[Index]->Comment);
    IsMmioWhitelistEnabled = UserBooter.MmioWhitelist.Values[Index]->Enabled;

    if (!StringHasAllPrintableCharacter (Comment)) {
      DEBUG ((DEBUG_WARN, "Booter->MmioWhitelist[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (IsMmioWhitelistEnabled) {
      ShouldEnableDevirtualiseMmio = TRUE;
    }
  }

  for (Index = 0; Index < UserBooter.Patch.Count; ++Index) {
    Comment         = OC_BLOB_GET (&UserBooter.Patch.Values[Index]->Comment);
    Arch            = OC_BLOB_GET (&UserBooter.Patch.Values[Index]->Arch);
    Identifier      = OC_BLOB_GET (&UserBooter.Patch.Values[Index]->Identifier);
    FindSize        = UserBooter.Patch.Values[Index]->Find.Size;
    ReplaceSize     = UserBooter.Patch.Values[Index]->Replace.Size;
    MaskSize        = UserBooter.Patch.Values[Index]->Mask.Size;
    ReplaceMaskSize = UserBooter.Patch.Values[Index]->ReplaceMask.Size;

    if (!StringHasAllPrintableCharacter (Comment)) {
      DEBUG ((DEBUG_WARN, "Booter->Patch[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (Arch[0] != '\0'
      && AsciiStrCmp (Arch, "Any") != 0
      && AsciiStrCmp (Arch, "i386") != 0
      && AsciiStrCmp (Arch, "x86_64") != 0) {
      DEBUG ((
        DEBUG_WARN,
        "Booter->Patch[%u]->Arch has illegal value: %a (Can only be Empty string/Any, i386, and x86_64)\n",
        Index,
        Arch
        ));
      ++ErrorCount;
    }

    if (Identifier[0] != '\0'
      && AsciiStrCmp (Identifier, "Any") != 0
      && AsciiStrCmp (Identifier, "Apple") != 0
      && AsciiStrCmp (GetFilenameSuffix (Identifier), "efi") != 0) { ///< TODO: case insensitive maybe?
      DEBUG ((
        DEBUG_WARN,
        "Booter->Patch[%u]->Identifier has illegal value: %a (Can only be Empty string/Any, Apple, or a specified bootloader with .efi sufffix)\n",
        Index,
        Identifier
        ));
      ++ErrorCount;
    }

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
    if (MaskSize > 0 && MaskSize != FindSize) {
      DEBUG ((
        DEBUG_WARN,
        "Booter->Patch[%u] has Mask set but its size is different from Find/Replace (%u vs %u)!\n",
        Index,
        MaskSize,
        FindSize
        ));
      ++ErrorCount;
    }
    if (ReplaceMaskSize > 0 && ReplaceMaskSize != FindSize) {
      DEBUG ((
        DEBUG_WARN,
        "Booter->Patch[%u] has ReplaceMask set but its size is different from Find/Replace (%u vs %u)!\n",
        Index,
        ReplaceMaskSize,
        FindSize
        ));
      ++ErrorCount;
    }
  }

  if (ShouldEnableDevirtualiseMmio && !UserBooter.Quirks.DevirtualiseMmio) {
    DEBUG ((DEBUG_WARN, "There are enabled entries under Booter->MmioWhitelist, but DevirtualiseMmio is not enabled!\n"));
    ++ErrorCount;
  }
  if (IsAllowRelocationBlockEnabled && (!IsProvideCustomSlideEnabled || !IsAvoidRuntimeDefragEnabled)) {
    DEBUG ((
      DEBUG_WARN,
      "Booter->Quirks->AllowRelocationBlock is enabled, but ProvideCustomSlide (%a) and AvoidRuntimeDefrag (%a) are not enabled altogether!\n",
      IsProvideCustomSlideEnabled ? "enabled" : "disabled",
      IsAvoidRuntimeDefragEnabled ? "enabled" : "disabled"
      ));
    ++ErrorCount;
  }
  if (IsEnableSafeModeSlideEnabled && !IsProvideCustomSlideEnabled) {
    DEBUG ((DEBUG_WARN, "Booter->Quirks->EnableSafeModeSlide is enabled, but ProvideCustomSlide is not enabled altogether!\n",));
    ++ErrorCount;
  }

  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  }
  return ErrorCount;
}

STATIC
UINT32
CheckDeviceProperties (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32 ErrorCount;

  DEBUG ((DEBUG_INFO, "config loaded into DeviceProperties checker!\n"));

  ErrorCount = 0;

  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  }
  return ErrorCount;
}

STATIC
UINT32
CheckKernel (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32 ErrorCount;

  DEBUG ((DEBUG_INFO, "config loaded into Kernel checker!\n"));

  ErrorCount = 0;

  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  }
  return ErrorCount;
}

STATIC
UINT32
CheckMisc (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32 ErrorCount;

  DEBUG ((DEBUG_INFO, "config loaded into Misc checker!\n"));

  ErrorCount = 0;

  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  }
  return ErrorCount;
}

STATIC
UINT32
CheckNVRAM (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32 ErrorCount;

  DEBUG ((DEBUG_INFO, "config loaded into NVRAM checker!\n"));

  ErrorCount = 0;

  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  }
  return ErrorCount;
}

STATIC
UINT32
CheckPlatformInfo (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32 ErrorCount;

  DEBUG ((DEBUG_INFO, "config loaded into PlatformInfo checker!\n"));

  ErrorCount = 0;

  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  }
  return ErrorCount;
}

STATIC
UINT32
CheckUEFI (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32 ErrorCount = 0;

  DEBUG ((DEBUG_INFO, "config loaded into UEFI checker!\n"));

  ErrorCount = 0;

  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", __func__, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  }
  return ErrorCount;
}

STATIC
UINT32
CheckConfig (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32  ErrorCount;
  UINTN   Index;
  UINTN   ConfigCheckersSize;

  ErrorCount = 0;

  STATIC CONFIG_CHECK ConfigCheckers[] = {
    &CheckACPI,
    &CheckBooter,
    &CheckDeviceProperties,
    &CheckKernel,
    &CheckMisc,
    &CheckNVRAM,
    &CheckPlatformInfo,
    &CheckUEFI
  };
  ConfigCheckersSize = sizeof (ConfigCheckers) / sizeof (ConfigCheckers[0]);

  for (Index = 0; Index < ConfigCheckersSize; ++Index) {
    ErrorCount |= ConfigCheckers[Index] (Config);
  }

  return ErrorCount;
}

int main(int argc, const char *argv[]) {
  UINT8              *ConfigFileBuffer;
  UINT32             ConfigFileSize;
  CONST CHAR8        *ConfigFileName;
  INT64              ExecTimeStart;
  OC_GLOBAL_CONFIG   Config;
  EFI_STATUS         Status;
  UINT32             ErrorCount;

  PcdGet8  (PcdDebugPropertyMask)         |= DEBUG_PROPERTY_DEBUG_CODE_ENABLED;
  PcdGet32 (PcdFixedDebugPrintErrorLevel) |= DEBUG_INFO;
  PcdGet32 (PcdDebugPrintErrorLevel)      |= DEBUG_INFO;

  ConfigFileName   = argc > 1 ? argv[1] : "config.plist";
  ConfigFileBuffer = readFile (ConfigFileName, &ConfigFileSize);
  if (ConfigFileBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to read %a\n", ConfigFileName));
    return -1;
  }

  ExecTimeStart = GetCurrentTimestamp ();

  Status   = OcConfigurationInit (&Config, ConfigFileBuffer, ConfigFileSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Invalid config\n"));
    return -1;
  }

  ErrorCount = CheckConfig (&Config);
  if (ErrorCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "Done checking %a in %llu ms\n",
      ConfigFileName,
      GetCurrentTimestamp () - ExecTimeStart
      ));
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "Done checking %a in %llu ms, but it has %u %a to be fixed\n",
      ConfigFileName,
      GetCurrentTimestamp () - ExecTimeStart,
      ErrorCount,
      ErrorCount > 1 ? "errors" : "error"
      ));
  }

  OcConfigurationFree (&Config);
  free (ConfigFileBuffer);

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  VOID *NewData = AllocatePool (Size);
  if (NewData) {
    CopyMem (NewData, Data, Size);
    OC_GLOBAL_CONFIG   Config;
    OcConfigurationInit (&Config, NewData, Size);
    OcConfigurationFree (&Config);
    FreePool (NewData);
  }
  return 0;
}
