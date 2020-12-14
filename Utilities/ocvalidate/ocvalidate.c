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
#include <Library/DevicePathLib.h>

#include <Library/OcTemplateLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcConfigurationLib.h>

#include <File.h>
#include <sys/time.h>

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
  CONST CHAR8  *SuffixDot;

  //
  // Find the last dot by reverse order.
  //
  SuffixDot = OcAsciiStrrChr (FileName, '.');

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
AsciiStringHasAllLegalCharacter (
  IN  CONST CHAR8  *String
  )
{
  UINTN  Index;

  for (Index = 0; Index < AsciiStrLen (String); ++Index) {
    //
    // Skip allowed characters (0-9, A-Z, a-z, '_', '-', '.', '/', and '\').
    //
    if (IsAsciiNumber (String[Index])
      || IsAsciiAlpha (String[Index])
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
AsciiStringHasAllPrintableCharacter (
  IN  CONST CHAR8  *String
  )
{
  UINTN  Index;

  for (Index = 0; Index < AsciiStrLen (String); ++Index) {
    //
    // Unprintable characters matched.
    //
    if (IsAsciiPrint (String[Index]) == 0) {
      return FALSE;
    }
  }

  return TRUE;
}

STATIC
UINT32
ReportError (
  IN  CONST CHAR8  *FuncName,
  IN  UINT32       ErrorCount
  )
{
  if (ErrorCount != 0) {
    DEBUG ((DEBUG_WARN, "%a returns %u %a!\n", FuncName, ErrorCount, ErrorCount > 1 ? "errors" : "error"));
  } else {
    DEBUG ((DEBUG_INFO, "%a returns no errors!\n", FuncName));
  }

  return ErrorCount;
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
  BOOLEAN         IsAddEnabled;

  DEBUG ((DEBUG_INFO, "config loaded into ACPI checker!\n"));

  ErrorCount    = 0;
  UserAcpi      = Config->Acpi;
  HasCustomDSDT = FALSE;
  IsAddEnabled  = FALSE;

  for (Index = 0; Index < UserAcpi.Add.Count; ++Index) {
    Path         = OC_BLOB_GET (&UserAcpi.Add.Values[Index]->Path);
    Comment      = OC_BLOB_GET (&UserAcpi.Add.Values[Index]->Comment);
    IsAddEnabled = UserAcpi.Add.Values[Index]->Enabled;

    if (!AsciiStringHasAllLegalCharacter (Path)) {
      DEBUG ((DEBUG_WARN, "ACPI->Add[%u]->Path contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiStringHasAllPrintableCharacter (Comment)) {
      DEBUG ((DEBUG_WARN, "ACPI->Add[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (AsciiStriCmp (GetFilenameSuffix (Path), "dsl") == 0) {
      DEBUG ((DEBUG_WARN, "ACPI->Add[%u]->Path has .dsl suffix!\n", Index));
      ++ErrorCount;
    }

    if (OcAsciiStriStr (Path, "DSDT") != NULL && IsAddEnabled) {
      HasCustomDSDT = TRUE;
    }
  }

  for (Index = 0; Index < UserAcpi.Delete.Count; ++Index) {
    Comment = OC_BLOB_GET (&UserAcpi.Delete.Values[Index]->Comment);

    if (!AsciiStringHasAllPrintableCharacter (Comment)) {
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

    if (!AsciiStringHasAllPrintableCharacter (Comment)) {
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

  return ReportError (__func__, ErrorCount);
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
  OC_UEFI_CONFIG    UserUefi;
  CONST CHAR8       *Comment;
  CONST CHAR8       *Arch;
  CONST CHAR8       *Identifier;
  CONST CHAR8       *Driver;
  UINT32            FindSize;
  UINT32            ReplaceSize;
  UINT32            MaskSize;
  UINT32            ReplaceMaskSize;
  UINT8             MaxSlide;
  BOOLEAN           IsMmioWhitelistEnabled;
  BOOLEAN           ShouldEnableDevirtualiseMmio;
  BOOLEAN           IsAllowRelocationBlockEnabled;
  BOOLEAN           IsProvideCustomSlideEnabled;
  BOOLEAN           IsAvoidRuntimeDefragEnabled;
  BOOLEAN           IsEnableSafeModeSlideEnabled;
  BOOLEAN           IsDisableVariableWriteEnabled;
  BOOLEAN           IsEnableWriteUnprotectorEnabled;
  BOOLEAN           IsRebuildAppleMemoryMapEnabled;
  BOOLEAN           IsSyncRuntimePermissionsEnabled;
  BOOLEAN           HasOpenRuntimeEfiDriver;
  
  DEBUG ((DEBUG_INFO, "config loaded into Booter checker!\n"));

  ErrorCount                      = 0;
  UserBooter                      = Config->Booter;
  UserUefi                        = Config->Uefi;
  IsMmioWhitelistEnabled          = FALSE;
  ShouldEnableDevirtualiseMmio    = FALSE;
  IsAllowRelocationBlockEnabled   = UserBooter.Quirks.AllowRelocationBlock;
  IsProvideCustomSlideEnabled     = UserBooter.Quirks.ProvideCustomSlide;
  IsAvoidRuntimeDefragEnabled     = UserBooter.Quirks.AvoidRuntimeDefrag;
  IsEnableSafeModeSlideEnabled    = UserBooter.Quirks.EnableSafeModeSlide;
  IsDisableVariableWriteEnabled   = UserBooter.Quirks.DisableVariableWrite;
  IsEnableWriteUnprotectorEnabled = UserBooter.Quirks.EnableWriteUnprotector;
  IsRebuildAppleMemoryMapEnabled  = UserBooter.Quirks.RebuildAppleMemoryMap;
  IsSyncRuntimePermissionsEnabled = UserBooter.Quirks.SyncRuntimePermissions;
  HasOpenRuntimeEfiDriver         = FALSE;
  MaxSlide                        = UserBooter.Quirks.ProvideMaxSlide;
  
  for (Index = 0; Index < UserBooter.MmioWhitelist.Count; ++Index) {
    Comment                = OC_BLOB_GET (&UserBooter.MmioWhitelist.Values[Index]->Comment);
    IsMmioWhitelistEnabled = UserBooter.MmioWhitelist.Values[Index]->Enabled;

    if (!AsciiStringHasAllPrintableCharacter (Comment)) {
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

    if (!AsciiStringHasAllPrintableCharacter (Comment)) {
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
      && AsciiStriCmp (GetFilenameSuffix (Identifier), "efi") != 0) {
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

  for (Index = 0; Index < UserUefi.Drivers.Count; ++Index) {
    Driver = OC_BLOB_GET (UserUefi.Drivers.Values[Index]);

    if (AsciiStrCmp (Driver, "OpenRuntime.efi") == 0) {
      HasOpenRuntimeEfiDriver = TRUE;
    }
  }

  if (ShouldEnableDevirtualiseMmio && !UserBooter.Quirks.DevirtualiseMmio) {
    DEBUG ((DEBUG_WARN, "There are enabled entries under Booter->MmioWhitelist, but DevirtualiseMmio is not enabled!\n"));
    ++ErrorCount;
  }
  if (IsAllowRelocationBlockEnabled) {
    if (!IsProvideCustomSlideEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->AllowRelocationBlock is enabled, but ProvideCustomSlide is not enabled altogether!\n"));
      ++ErrorCount;
    }

    if (!IsAvoidRuntimeDefragEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->AllowRelocationBlock is enabled, and AvoidRuntimeDefrag is highly recommended to be enabled as well\n"));
      //
      // This is not an error but a suggestion.
      //
    }
  }
  if (!IsProvideCustomSlideEnabled) {
    if (IsEnableSafeModeSlideEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->EnableSafeModeSlide is enabled, but ProvideCustomSlide is not enabled altogether!\n"));
      ++ErrorCount;
    }
    if (MaxSlide > 0) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->ProvideMaxSlide is set to %u, but ProvideCustomSlide is not enabled altogether!\n", MaxSlide));
      ++ErrorCount;
    }
  }
  if (!HasOpenRuntimeEfiDriver) {
    if (IsDisableVariableWriteEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->DisableVariableWrite is enabled, but OpenRuntime.efi is not loaded at UEFI->Drivers!\n"));
      ++ErrorCount;
    }
    if (IsEnableWriteUnprotectorEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->EnableWriteUnprotector is enabled, but OpenRuntime.efi is not loaded at UEFI->Drivers!\n"));
      ++ErrorCount;
    }
  }
  if (IsRebuildAppleMemoryMapEnabled) {
    if (IsEnableWriteUnprotectorEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->EnableWriteUnprotector and RebuildAppleMemoryMap cannot be enabled simultaneously!\n"));
      ++ErrorCount;
    }
    if (!IsSyncRuntimePermissionsEnabled) {
      DEBUG ((DEBUG_WARN, "Booter->Quirks->RebuildAppleMemoryMap is enabled, and SyncRuntimePermissions is highly recommended to be enabled as well\n"));
      //
      // This is not an error but a suggestion.
      //
    }
  }

  return ReportError (__func__, ErrorCount);
}

STATIC
UINT32
CheckDeviceProperties (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32                    ErrorCount;
  UINT32                    DeviceIndex;
  OC_DEV_PROP_CONFIG        UserDevProp;
  CONST CHAR8               *AsciiDevicePath;
  CONST CHAR16              *UnicodeDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DEBUG ((DEBUG_INFO, "config loaded into DeviceProperties checker!\n"));

  ErrorCount  = 0;
  UserDevProp = Config->DeviceProperties;

  for (DeviceIndex = 0; DeviceIndex < UserDevProp.Delete.Count; ++DeviceIndex) {
    AsciiDevicePath   = OC_BLOB_GET (UserDevProp.Delete.Keys[DeviceIndex]);
    UnicodeDevicePath = AsciiStrCopyToUnicode (AsciiDevicePath, 0);
    DevicePath        = NULL;

    //
    // Should we sanitise AsciiDevicePath first? 
    //

    if (UnicodeDevicePath != NULL) {
      DevicePath = ConvertTextToDevicePath (UnicodeDevicePath);
      FreePool ((VOID *) UnicodeDevicePath);
      // FreePool (UnicodeDevicePath);
    }

    if (DevicePath == NULL) {
      DEBUG ((DEBUG_WARN, "DeviceProperties->Delete[%u] is borked!\n", DeviceIndex));
      ++ErrorCount;
    }

    FreePool (DevicePath);
  }

  return ReportError (__func__, ErrorCount);
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

  return ReportError (__func__, ErrorCount);
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

  return ReportError (__func__, ErrorCount);
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

  return ReportError (__func__, ErrorCount);
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

  return ReportError (__func__, ErrorCount);
}

STATIC
UINT32
CheckUEFI (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32            ErrorCount;
  UINT32            Index;
  OC_UEFI_CONFIG    UserUefi;
  OC_MISC_CONFIG    UserMisc;
  CONST CHAR8       *Driver;
  BOOLEAN           HasOpenRuntimeEfiDriver;
  BOOLEAN           IsRequestBootVarRoutingEnabled;
  BOOLEAN           IsDeduplicateBootOrderEnabled;

  DEBUG ((DEBUG_INFO, "config loaded into UEFI checker!\n"));

  ErrorCount                     = 0;
  UserUefi                       = Config->Uefi;
  UserMisc                       = Config->Misc;
  HasOpenRuntimeEfiDriver        = FALSE;
  IsRequestBootVarRoutingEnabled = UserUefi.Quirks.RequestBootVarRouting;
  IsDeduplicateBootOrderEnabled  = UserUefi.Quirks.DeduplicateBootOrder;

  if (UserUefi.Apfs.EnableJumpstart
    && (UserMisc.Security.ScanPolicy != 0 && (UserMisc.Security.ScanPolicy & OC_SCAN_ALLOW_FS_APFS) == 0)) { ///< FIXME: Can ScanPolicy be 0 to be failsafe?
    DEBUG ((DEBUG_WARN, "UEFI->APFS->EnableJumpstart is enabled, but Misc->Security->ScanPolicy does not allow APFS scanning!\n"));
    ++ErrorCount;
  }

  for (Index = 0; Index < UserUefi.Drivers.Count; ++Index) {
    Driver = OC_BLOB_GET (UserUefi.Drivers.Values[Index]);

    if (AsciiStrCmp (Driver, "OpenRuntime.efi") == 0) {
      HasOpenRuntimeEfiDriver = TRUE;
    }
  }

  if (IsRequestBootVarRoutingEnabled && !HasOpenRuntimeEfiDriver) {
    DEBUG ((DEBUG_WARN, "UEFI->Quirks->RequestBootVarRouting is enabled, but OpenRuntime.efi is not loaded at UEFI->Drivers!\n"));
    ++ErrorCount;
  }
  if (IsDeduplicateBootOrderEnabled && !IsRequestBootVarRoutingEnabled) {
    DEBUG ((DEBUG_WARN, "UEFI->Quirks->DeduplicateBootOrder is enabled, but RequestBootVarRouting is not enabled altogether!\n"));
    ++ErrorCount;
  }

  return ReportError (__func__, ErrorCount);
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
    ErrorCount += ConfigCheckers[Index] (Config);
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
