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

#include "NvramKeyInfo.h"
#include "ocvalidate.h"
#include "OcValidateLib.h"

#include <Library/BaseLib.h>
#include <Library/OcConsoleLib.h>

/**
  Callback function to verify whether one UEFI driver is duplicated in UEFI->Drivers.

  @param[in]  PrimaryDriver    Primary driver to be checked.
  @param[in]  SecondaryDriver  Secondary driver to be checked.

  @retval     TRUE             If PrimaryDriver and SecondaryDriver are duplicated.
**/
STATIC
BOOLEAN
UefiDriverHasDuplication (
  IN  CONST VOID  *PrimaryDriver,
  IN  CONST VOID  *SecondaryDriver
  )
{
  CONST OC_UEFI_DRIVER_ENTRY  *UefiPrimaryDriver;
  CONST OC_UEFI_DRIVER_ENTRY  *UefiSecondaryDriver;
  CONST CHAR8                 *UefiDriverPrimaryString;
  CONST CHAR8                 *UefiDriverSecondaryString;

  UefiPrimaryDriver         = *(CONST OC_UEFI_DRIVER_ENTRY **)PrimaryDriver;
  UefiSecondaryDriver       = *(CONST OC_UEFI_DRIVER_ENTRY **)SecondaryDriver;
  UefiDriverPrimaryString   = OC_BLOB_GET (&UefiPrimaryDriver->Path);
  UefiDriverSecondaryString = OC_BLOB_GET (&UefiSecondaryDriver->Path);

  return StringIsDuplicated ("UEFI->Drivers", UefiDriverPrimaryString, UefiDriverSecondaryString);
}

/**
  Callback function to verify whether one UEFI ReservedMemory entry overlaps the other,
  in terms of Address and Size.

  @param[in]  PrimaryEntry     Primary entry to be checked.
  @param[in]  SecondaryEntry   Secondary entry to be checked.

  @retval     TRUE             If PrimaryEntry and SecondaryEntry have overlapped Address and Size.
**/
STATIC
BOOLEAN
UefiReservedMemoryHasOverlap (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  CONST OC_UEFI_RSVD_ENTRY  *UefiReservedMemoryPrimaryEntry;
  CONST OC_UEFI_RSVD_ENTRY  *UefiReservedMemorySecondaryEntry;
  UINT64                    UefiReservedMemoryPrimaryAddress;
  UINT64                    UefiReservedMemoryPrimarySize;
  UINT64                    UefiReservedMemorySecondaryAddress;
  UINT64                    UefiReservedMemorySecondarySize;

  UefiReservedMemoryPrimaryEntry     = *(CONST OC_UEFI_RSVD_ENTRY **)PrimaryEntry;
  UefiReservedMemorySecondaryEntry   = *(CONST OC_UEFI_RSVD_ENTRY **)SecondaryEntry;
  UefiReservedMemoryPrimaryAddress   = UefiReservedMemoryPrimaryEntry->Address;
  UefiReservedMemoryPrimarySize      = UefiReservedMemoryPrimaryEntry->Size;
  UefiReservedMemorySecondaryAddress = UefiReservedMemorySecondaryEntry->Address;
  UefiReservedMemorySecondarySize    = UefiReservedMemorySecondaryEntry->Size;

  if (!UefiReservedMemoryPrimaryEntry->Enabled || !UefiReservedMemorySecondaryEntry->Enabled) {
    return FALSE;
  }

  if (  (UefiReservedMemoryPrimaryAddress < UefiReservedMemorySecondaryAddress + UefiReservedMemorySecondarySize)
     && (UefiReservedMemorySecondaryAddress < UefiReservedMemoryPrimaryAddress + UefiReservedMemoryPrimarySize))
  {
    DEBUG ((DEBUG_WARN, "UEFI->ReservedMemory: Entries have overlapped Address and Size "));
    return TRUE;
  }

  return FALSE;
}

STATIC
BOOLEAN
ValidateReservedMemoryType (
  IN  CONST CHAR8  *Type
  )
{
  UINTN               Index;
  STATIC CONST CHAR8  *AllowedType[] = {
    "Reserved",          "LoaderCode",    "LoaderData",     "BootServiceCode",         "BootServiceData",
    "RuntimeCode",       "RuntimeData",   "Available",      "Persistent",              "UnusableMemory",
    "ACPIReclaimMemory", "ACPIMemoryNVS", "MemoryMappedIO", "MemoryMappedIOPortSpace", "PalCode"
  };

  for (Index = 0; Index < ARRAY_SIZE (AllowedType); ++Index) {
    if (AsciiStrCmp (Type, AllowedType[Index]) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
UINT32
CheckUefiAPFS (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32   ErrorCount;
  BOOLEAN  IsEnableJumpstartEnabled;
  UINT32   ScanPolicy;

  ErrorCount = 0;

  //
  // If FS restrictions is enabled but APFS FS scanning is disabled, it is an error.
  //
  IsEnableJumpstartEnabled = Config->Uefi.Apfs.EnableJumpstart;
  ScanPolicy               = Config->Misc.Security.ScanPolicy;
  if (  IsEnableJumpstartEnabled
     && ((ScanPolicy & OC_SCAN_FILE_SYSTEM_LOCK) != 0)
     && ((ScanPolicy & OC_SCAN_ALLOW_FS_APFS) == 0))
  {
    DEBUG ((DEBUG_WARN, "UEFI->APFS->EnableJumpstart is enabled, but Misc->Security->ScanPolicy does not allow APFS scanning!\n"));
    ++ErrorCount;
  }

  return ErrorCount;
}

STATIC
UINT32
CheckUefiAppleInput (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  CONST CHAR8  *AppleEvent;

  ErrorCount = 0;

  AppleEvent = OC_BLOB_GET (&Config->Uefi.AppleInput.AppleEvent);
  if (  (AsciiStrCmp (AppleEvent, "Auto") != 0)
     && (AsciiStrCmp (AppleEvent, "Builtin") != 0)
     && (AsciiStrCmp (AppleEvent, "OEM") != 0))
  {
    DEBUG ((DEBUG_WARN, "UEFI->AppleInput->AppleEvent is illegal (Can only be Auto, Builtin, OEM)!\n"));
    ++ErrorCount;
  }

  if (Config->Uefi.Input.KeySupport && Config->Uefi.AppleInput.CustomDelays) {
    if (  (Config->Uefi.AppleInput.KeyInitialDelay != 0)
       && (Config->Uefi.AppleInput.KeyInitialDelay < Config->Uefi.Input.KeyForgetThreshold))
    {
      DEBUG ((DEBUG_WARN, "KeyInitialDelay is enabled in KeySupport mode, is non-zero and is less than the KeyForgetThreshold value (will result in uncontrolled key repeats)!\n"));
      ++ErrorCount;
    }

    if (Config->Uefi.AppleInput.KeySubsequentDelay < Config->Uefi.Input.KeyForgetThreshold) {
      DEBUG ((DEBUG_WARN, "KeySubsequentDelay is enabled in KeySupport mode and is less than the KeyForgetThreshold value (will result in uncontrolled key repeats)!\n"));
      ++ErrorCount;
    }
  }

  return ErrorCount;
}

STATIC
UINT32
CheckUefiGain (
  INT8 Gain,
  CHAR8 *GainName,
  INT8 GainAbove, OPTIONAL
  CHAR8   *GainAboveName  OPTIONAL
  )
{
  UINT32  ErrorCount;

  ErrorCount = 0;

  //
  // Cannot check these as they are already truncated to INT8 before we can validate them.
  // TODO: (?) Add check during DEBUG parsing that specified values fit into what they will be cast to.
  //
 #if 0
  if (Gain < -128) {
    DEBUG ((DEBUG_WARN, "UEFI->Audio->%a must be greater than or equal to -128!\n", GainName));
    ++ErrorCount;
  } else if (Gain > 127) {
    DEBUG ((DEBUG_WARN, "UEFI->Audio->%a must be less than or equal to 127!\n", GainName));
    ++ErrorCount;
  }

 #endif

  if ((GainAboveName != NULL) && (Gain > GainAbove)) {
    DEBUG ((DEBUG_WARN, "UEFI->Audio->%a must be less than or equal to UEFI->Audio->%a!\n", GainName, GainAboveName));
    ++ErrorCount;
  }

  return ErrorCount;
}

STATIC
UINT32
CheckUefiAudio (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  BOOLEAN      IsAudioSupportEnabled;
  UINT64       AudioOutMask;
  CONST CHAR8  *AsciiAudioDevicePath;
  CONST CHAR8  *AsciiPlayChime;

  ErrorCount = 0;

  IsAudioSupportEnabled = Config->Uefi.Audio.AudioSupport;
  AudioOutMask          = Config->Uefi.Audio.AudioOutMask;
  AsciiAudioDevicePath  = OC_BLOB_GET (&Config->Uefi.Audio.AudioDevice);
  AsciiPlayChime        = OC_BLOB_GET (&Config->Uefi.Audio.PlayChime);
  if (IsAudioSupportEnabled) {
    if (AudioOutMask == 0) {
      DEBUG ((DEBUG_WARN, "UEFI->Audio->AudioOutMask is zero when AudioSupport is enabled, no sound will play!\n"));
      ++ErrorCount;
    }

    ErrorCount += CheckUefiGain (
                    Config->Uefi.Audio.MaximumGain,
                    "MaximumGain",
                    0,
                    NULL
                    );

    // No operational reason for MinimumAssistGain <= MaximumGain, but is safer to ensure non-deafening sound levels.
    ErrorCount += CheckUefiGain (
                    Config->Uefi.Audio.MinimumAssistGain,
                    "MinimumAssistGain",
                    Config->Uefi.Audio.MaximumGain,
                    "MaximumGain"
                    );

    ErrorCount += CheckUefiGain (
                    Config->Uefi.Audio.MinimumAudibleGain,
                    "MinimumAudibleGain",
                    Config->Uefi.Audio.MinimumAssistGain,
                    "MinimumAssistGain"
                    );

    ErrorCount += CheckUefiGain (
                    Config->Uefi.Audio.MinimumAudibleGain,
                    "MinimumAudibleGain",
                    Config->Uefi.Audio.MaximumGain,
                    "MaximumGain"
                    );

    if (!AsciiDevicePathIsLegal (AsciiAudioDevicePath)) {
      DEBUG ((DEBUG_WARN, "UEFI->Audio->AudioDevice is borked! Please check the information above!\n"));
      ++ErrorCount;
    }

    if (AsciiPlayChime[0] == '\0') {
      DEBUG ((DEBUG_WARN, "UEFI->Audio->PlayChime cannot be empty when AudioSupport is enabled!\n"));
      ++ErrorCount;
    } else if (  (AsciiStrCmp (AsciiPlayChime, "Auto") != 0)
              && (AsciiStrCmp (AsciiPlayChime, "Enabled") != 0)
              && (AsciiStrCmp (AsciiPlayChime, "Disabled") != 0))
    {
      DEBUG ((DEBUG_WARN, "UEFI->Audio->PlayChime is borked (Can only be Auto, Enabled, or Disabled)!\n"));
      ++ErrorCount;
    }
  }

  return ErrorCount;
}

STATIC
UINT32
CheckUefiDrivers (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32                ErrorCount;
  UINT32                Index;
  OC_UEFI_DRIVER_ENTRY  *DriverEntry;
  CONST CHAR8           *Comment;
  CONST CHAR8           *Driver;
  UINTN                 DriverSumSize;
  BOOLEAN               HasOpenRuntimeEfiDriver;
  BOOLEAN               IsOpenRuntimeLoadEarly;
  UINT32                IndexOpenRuntimeEfiDriver;
  BOOLEAN               HasOpenUsbKbDxeEfiDriver;
  UINT32                IndexOpenUsbKbDxeEfiDriver;
  BOOLEAN               HasPs2KeyboardDxeEfiDriver;
  BOOLEAN               IndexPs2KeyboardDxeEfiDriver;
  BOOLEAN               HasHfsEfiDriver;
  UINT32                IndexHfsEfiDriver;
  BOOLEAN               HasAudioDxeEfiDriver;
  UINT32                IndexAudioDxeEfiDriver;
  BOOLEAN               IsRequestBootVarRoutingEnabled;
  BOOLEAN               IsKeySupportEnabled;
  BOOLEAN               IsConnectDriversEnabled;
  BOOLEAN               HasOpenVariableRuntimeDxeEfiDriver;
  UINT32                IndexOpenVariableRuntimeDxeEfiDriver;

  ErrorCount = 0;

  HasOpenRuntimeEfiDriver              = FALSE;
  IndexOpenRuntimeEfiDriver            = 0;
  HasOpenUsbKbDxeEfiDriver             = FALSE;
  IndexOpenUsbKbDxeEfiDriver           = 0;
  HasPs2KeyboardDxeEfiDriver           = FALSE;
  IndexPs2KeyboardDxeEfiDriver         = 0;
  HasHfsEfiDriver                      = FALSE;
  IndexHfsEfiDriver                    = 0;
  HasAudioDxeEfiDriver                 = FALSE;
  IndexAudioDxeEfiDriver               = 0;
  HasOpenVariableRuntimeDxeEfiDriver   = FALSE;
  IndexOpenVariableRuntimeDxeEfiDriver = 0;
  IsOpenRuntimeLoadEarly               = FALSE;
  for (Index = 0; Index < Config->Uefi.Drivers.Count; ++Index) {
    DriverEntry = Config->Uefi.Drivers.Values[Index];
    Comment     = OC_BLOB_GET (&DriverEntry->Comment);
    Driver      = OC_BLOB_GET (&DriverEntry->Path);

    //
    // Check the length of path relative to OC directory.
    //
    DriverSumSize = L_STR_LEN (OPEN_CORE_UEFI_DRIVER_PATH) + AsciiStrSize (Driver);
    if (DriverSumSize > OC_STORAGE_SAFE_PATH_MAX) {
      DEBUG ((
        DEBUG_WARN,
        "UEFI->Drivers[%u] (length %u) is too long (should not exceed %u)!\n",
        Index,
        AsciiStrLen (Driver),
        OC_STORAGE_SAFE_PATH_MAX - L_STR_LEN (OPEN_CORE_UEFI_DRIVER_PATH)
        ));
      ++ErrorCount;
    }

    //
    // Sanitise strings.
    //
    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "UEFI->Drivers[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiUefiDriverIsLegal (Driver, Index)) {
      ++ErrorCount;
      continue;
    }

    if (!DriverEntry->Enabled) {
      continue;
    }

    if (AsciiStrCmp (Driver, "OpenVariableRuntimeDxe.efi") == 0) {
      HasOpenVariableRuntimeDxeEfiDriver   = TRUE;
      IndexOpenVariableRuntimeDxeEfiDriver = Index;

      if (!DriverEntry->LoadEarly) {
        DEBUG ((DEBUG_WARN, "OpenVariableRuntimeDxe at UEFI->Drivers[%u] must have LoadEarly set to TRUE!\n", Index));
        ++ErrorCount;
      }
    }

    //
    // For all drivers but OpenVariableRuntimeDxe.efi and OpenRuntime.efi, LoadEarly must be FALSE.
    //
    if ((AsciiStrCmp (Driver, "OpenVariableRuntimeDxe.efi") != 0) && (AsciiStrCmp (Driver, "OpenRuntime.efi") != 0) && DriverEntry->LoadEarly) {
      DEBUG ((DEBUG_WARN, "%a at UEFI->Drivers[%u] must have LoadEarly set to FALSE!\n", Driver, Index));
      ++ErrorCount;
    }

    if (AsciiStrCmp (Driver, "OpenRuntime.efi") == 0) {
      HasOpenRuntimeEfiDriver   = TRUE;
      IndexOpenRuntimeEfiDriver = Index;
      IsOpenRuntimeLoadEarly    = DriverEntry->LoadEarly;
    }

    if (AsciiStrCmp (Driver, "OpenUsbKbDxe.efi") == 0) {
      HasOpenUsbKbDxeEfiDriver   = TRUE;
      IndexOpenUsbKbDxeEfiDriver = Index;
    }

    if (AsciiStrCmp (Driver, "Ps2KeyboardDxe.efi") == 0) {
      HasPs2KeyboardDxeEfiDriver   = TRUE;
      IndexPs2KeyboardDxeEfiDriver = Index;
    }

    //
    // There are several HFS Plus drivers, including HfsPlus, VboxHfs, etc.
    // Here only "hfs" (case-insensitive) is matched.
    //
    if (OcAsciiStriStr (Driver, "hfs") != NULL) {
      HasHfsEfiDriver   = TRUE;
      IndexHfsEfiDriver = Index;
    }

    if (AsciiStrCmp (Driver, "AudioDxe.efi") == 0) {
      HasAudioDxeEfiDriver   = TRUE;
      IndexAudioDxeEfiDriver = Index;
    }
  }

  //
  // Check duplicated Drivers.
  //
  ErrorCount += FindArrayDuplication (
                  Config->Uefi.Drivers.Values,
                  Config->Uefi.Drivers.Count,
                  sizeof (Config->Uefi.Drivers.Values[0]),
                  UefiDriverHasDuplication
                  );

  if (HasOpenRuntimeEfiDriver) {
    if (HasOpenVariableRuntimeDxeEfiDriver) {
      if (!IsOpenRuntimeLoadEarly) {
        DEBUG ((
          DEBUG_WARN,
          "OpenRuntime.efi at UEFI->Drivers[%u] should have its LoadEarly set to TRUE when OpenVariableRuntimeDxe.efi at UEFI->Drivers[%u] is in use!\n",
          IndexOpenRuntimeEfiDriver,
          IndexOpenVariableRuntimeDxeEfiDriver
          ));
        ++ErrorCount;
      }

      if (IndexOpenVariableRuntimeDxeEfiDriver >= IndexOpenRuntimeEfiDriver) {
        DEBUG ((
          DEBUG_WARN,
          "OpenRuntime.efi (currently at UEFI->Drivers[%u]) should be placed after OpenVariableRuntimeDxe.efi (currently at UEFI->Drivers[%u])!\n",
          IndexOpenRuntimeEfiDriver,
          IndexOpenVariableRuntimeDxeEfiDriver
          ));
        ++ErrorCount;
      }
    } else {
      if (IsOpenRuntimeLoadEarly) {
        DEBUG ((
          DEBUG_WARN,
          "OpenRuntime.efi at UEFI->Drivers[%u] should have its LoadEarly set to FALSE unless OpenVariableRuntimeDxe.efi is in use!\n",
          IndexOpenRuntimeEfiDriver
          ));
        ++ErrorCount;
      }
    }
  }

  IsRequestBootVarRoutingEnabled = Config->Uefi.Quirks.RequestBootVarRouting;
  if (IsRequestBootVarRoutingEnabled) {
    if (!HasOpenRuntimeEfiDriver) {
      DEBUG ((DEBUG_WARN, "UEFI->Quirks->RequestBootVarRouting is enabled, but OpenRuntime.efi is not loaded at UEFI->Drivers!\n"));
      ++ErrorCount;
    }
  }

  IsKeySupportEnabled = Config->Uefi.Input.KeySupport;
  if (IsKeySupportEnabled) {
    if (HasOpenUsbKbDxeEfiDriver) {
      DEBUG ((DEBUG_WARN, "OpenUsbKbDxe.efi at UEFI->Drivers[%u] should NEVER be used together with UEFI->Input->KeySupport!\n", IndexOpenUsbKbDxeEfiDriver));
      ++ErrorCount;
    }
  } else {
    if (HasPs2KeyboardDxeEfiDriver) {
      DEBUG ((DEBUG_WARN, "UEFI->Input->KeySupport should be enabled when Ps2KeyboardDxe.efi is in use!\n"));
      ++ErrorCount;
    }
  }

  if (HasOpenUsbKbDxeEfiDriver && HasPs2KeyboardDxeEfiDriver) {
    DEBUG ((
      DEBUG_WARN,
      "OpenUsbKbDxe.efi at UEFI->Drivers[%u], and Ps2KeyboardDxe.efi at UEFI->Drivers[%u], should NEVER co-exist!\n",
      IndexOpenUsbKbDxeEfiDriver,
      IndexPs2KeyboardDxeEfiDriver
      ));
    ++ErrorCount;
  }

  IsConnectDriversEnabled = Config->Uefi.ConnectDrivers;
  if (!IsConnectDriversEnabled) {
    if (HasHfsEfiDriver) {
      DEBUG ((DEBUG_WARN, "HFS+ filesystem driver is loaded at UEFI->Drivers[%u], but UEFI->ConnectDrivers is not enabled!\n", IndexHfsEfiDriver));
      ++ErrorCount;
    }

    if (HasAudioDxeEfiDriver) {
      DEBUG ((DEBUG_WARN, "AudioDevice.efi is loaded at UEFI->Drivers[%u], but UEFI->ConnectDrivers is not enabled!\n", IndexAudioDxeEfiDriver));
      ++ErrorCount;
    }
  }

  return ErrorCount;
}

STATIC
UINT32
CheckUefiInput (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  BOOLEAN      IsPointerSupportEnabled;
  CONST CHAR8  *PointerSupportMode;
  CONST CHAR8  *KeySupportMode;

  ErrorCount = 0;

  IsPointerSupportEnabled = Config->Uefi.Input.PointerSupport;
  PointerSupportMode      = OC_BLOB_GET (&Config->Uefi.Input.PointerSupportMode);
  if (IsPointerSupportEnabled && (AsciiStrCmp (PointerSupportMode, "ASUS") != 0)) {
    DEBUG ((DEBUG_WARN, "UEFI->Input->PointerSupport is enabled, but PointerSupportMode is not ASUS!\n"));
    ++ErrorCount;
  }

  KeySupportMode = OC_BLOB_GET (&Config->Uefi.Input.KeySupportMode);
  if (  (AsciiStrCmp (KeySupportMode, "Auto") != 0)
     && (AsciiStrCmp (KeySupportMode, "V1") != 0)
     && (AsciiStrCmp (KeySupportMode, "V2") != 0)
     && (AsciiStrCmp (KeySupportMode, "AMI") != 0))
  {
    DEBUG ((DEBUG_WARN, "UEFI->Input->KeySupportMode is illegal (Can only be Auto, V1, V2, AMI)!\n"));
    ++ErrorCount;
  }

  return ErrorCount;
}

STATIC
UINT32
CheckUefiOutput (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  CONST CHAR8  *InitialMode;
  CONST CHAR8  *TextRenderer;
  CONST CHAR8  *GopPassThrough;
  BOOLEAN      IsTextRendererSystem;
  BOOLEAN      IsClearScreenOnModeSwitchEnabled;
  BOOLEAN      IsIgnoreTextInGraphicsEnabled;
  BOOLEAN      IsReplaceTabWithSpaceEnabled;
  BOOLEAN      IsSanitiseClearScreenEnabled;
  CONST CHAR8  *ConsoleFont;
  CONST CHAR8  *ConsoleMode;
  CONST CHAR8  *Resolution;
  UINT32       UserWidth;
  UINT32       UserHeight;
  UINT32       UserBpp;
  BOOLEAN      UserSetMax;
  INT8         UIScale;
  BOOLEAN      HasUefiOutputUIScale;

  ErrorCount           = 0;
  IsTextRendererSystem = FALSE;
  HasUefiOutputUIScale = FALSE;

  //
  // Sanitise strings.
  //
  InitialMode = OC_BLOB_GET (&Config->Uefi.Output.InitialMode);
  if (  (AsciiStrCmp (InitialMode, "Auto") != 0)
     && (AsciiStrCmp (InitialMode, "Text") != 0)
     && (AsciiStrCmp (InitialMode, "Graphics") != 0))
  {
    DEBUG ((DEBUG_WARN, "UEFI->Output->InitialMode is illegal (Can only be Auto, Text, or Graphics)!\n"));
    ++ErrorCount;
  }

  TextRenderer = OC_BLOB_GET (&Config->Uefi.Output.TextRenderer);
  if (  (AsciiStrCmp (TextRenderer, "BuiltinGraphics") != 0)
     && (AsciiStrCmp (TextRenderer, "BuiltinText") != 0)
     && (AsciiStrCmp (TextRenderer, "SystemGraphics") != 0)
     && (AsciiStrCmp (TextRenderer, "SystemText") != 0)
     && (AsciiStrCmp (TextRenderer, "SystemGeneric") != 0))
  {
    DEBUG ((DEBUG_WARN, "UEFI->Output->TextRenderer is illegal (Can only be BuiltinGraphics, BuiltinText, SystemGraphics, SystemText, or SystemGeneric)!\n"));
    ++ErrorCount;
  } else if (AsciiStrnCmp (TextRenderer, "System", L_STR_LEN ("System")) == 0) {
    //
    // Check whether TextRenderer has System prefix.
    //
    IsTextRendererSystem = TRUE;
  }

  if (IsTextRendererSystem) {
    ConsoleFont = OC_BLOB_GET (&Config->Uefi.Output.ConsoleFont);
    if (ConsoleFont[0] != '\0') {
      DEBUG ((DEBUG_WARN, "UEFI->Output->ConsoleFont is specified on non-Builtin TextRenderer (currently %a)!\n", TextRenderer));
      ++ErrorCount;
    }
  } else {
    IsClearScreenOnModeSwitchEnabled = Config->Uefi.Output.ClearScreenOnModeSwitch;
    if (IsClearScreenOnModeSwitchEnabled) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->ClearScreenOnModeSwitch is enabled on non-System TextRenderer (currently %a)!\n", TextRenderer));
      ++ErrorCount;
    }

    IsIgnoreTextInGraphicsEnabled = Config->Uefi.Output.IgnoreTextInGraphics;
    if (IsIgnoreTextInGraphicsEnabled) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->IgnoreTextInGraphics is enabled on non-System TextRenderer (currently %a)!\n", TextRenderer));
      ++ErrorCount;
    }

    IsReplaceTabWithSpaceEnabled = Config->Uefi.Output.ReplaceTabWithSpace;
    if (IsReplaceTabWithSpaceEnabled) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->ReplaceTabWithSpace is enabled on non-System TextRenderer (currently %a)!\n", TextRenderer));
      ++ErrorCount;
    }

    IsSanitiseClearScreenEnabled = Config->Uefi.Output.SanitiseClearScreen;
    if (IsSanitiseClearScreenEnabled) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->SanitiseClearScreen is enabled on non-System TextRenderer (currently %a)!\n", TextRenderer));
      ++ErrorCount;
    }
  }

  GopPassThrough = OC_BLOB_GET (&Config->Uefi.Output.GopPassThrough);
  if (  (AsciiStrCmp (GopPassThrough, "Enabled") != 0)
     && (AsciiStrCmp (GopPassThrough, "Disabled") != 0)
     && (AsciiStrCmp (GopPassThrough, "Apple") != 0))
  {
    DEBUG ((DEBUG_WARN, "UEFI->Output->GopPassThrough is illegal (Can only be Enabled, Disabled, Apple)!\n"));
    ++ErrorCount;
  }

  //
  // Parse Output->ConsoleMode by calling OpenCore libraries.
  //
  ConsoleMode = OC_BLOB_GET (&Config->Uefi.Output.ConsoleMode);
  OcParseConsoleMode (
    ConsoleMode,
    &UserWidth,
    &UserHeight,
    &UserSetMax
    );
  if (  (ConsoleMode[0] != '\0')
     && !UserSetMax)
  {
    if ((UserWidth == 0) || (UserHeight == 0)) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->ConsoleMode is borked, please check documentation!\n"));
      ++ErrorCount;
    } else if ((UserWidth < 80) || (UserHeight < 25)) {
      DEBUG ((DEBUG_WARN, "UEFI->Output->ConsoleMode is below minumum supported console text resolution of 80x25, please fix!\n"));
      ++ErrorCount;
    }
  }

  //
  // Parse Output->Resolution by calling OpenCore libraries.
  //
  Resolution = OC_BLOB_GET (&Config->Uefi.Output.Resolution);
  OcParseScreenResolution (
    Resolution,
    &UserWidth,
    &UserHeight,
    &UserBpp,
    &UserSetMax
    );
  if (  (Resolution[0] != '\0')
     && !UserSetMax
     && ((UserWidth == 0) || (UserHeight == 0)))
  {
    DEBUG ((DEBUG_WARN, "UEFI->Output->Resolution is borked, please check documentation!\n"));
    ++ErrorCount;
  }

  UIScale = Config->Uefi.Output.UIScale;
  if ((UIScale < -1) || (UIScale > 2)) {
    DEBUG ((DEBUG_WARN, "UEFI->Output->UIScale is borked (Can only be between -1 and 2)!\n"));
    ++ErrorCount;
  } else if (UIScale != -1) {
    HasUefiOutputUIScale = TRUE;
  }

  if (HasUefiOutputUIScale && mHasNvramUIScale) {
    DEBUG ((DEBUG_WARN, "UIScale is set under both NVRAM and UEFI->Output!\n"));
    ++ErrorCount;
  }

  return ErrorCount;
}

//
// FIXME: Just in case this can be of use...
//
STATIC
UINT32
CheckUefiQuirks (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32  ErrorCount;
  INT8    ResizeGpuBars;

  ErrorCount    = 0;
  ResizeGpuBars = Config->Uefi.Quirks.ResizeGpuBars;

  if ((ResizeGpuBars < -1) || (ResizeGpuBars > 19)) {
    DEBUG ((DEBUG_WARN, "UEFI->Quirks->ResizeGpuBars is borked (Can only be between -1 and 19)!\n"));
    ++ErrorCount;
  }

  return ErrorCount;
}

STATIC
UINT32
CheckUefiReservedMemory (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  UINT32       Index;
  CONST CHAR8  *AsciiReservedMemoryType;
  UINT64       ReservedMemoryAddress;
  UINT64       ReservedMemorySize;

  ErrorCount = 0;

  //
  // Validate ReservedMemory[N].
  //
  for (Index = 0; Index < Config->Uefi.ReservedMemory.Count; ++Index) {
    AsciiReservedMemoryType = OC_BLOB_GET (&Config->Uefi.ReservedMemory.Values[Index]->Type);
    ReservedMemoryAddress   = Config->Uefi.ReservedMemory.Values[Index]->Address;
    ReservedMemorySize      = Config->Uefi.ReservedMemory.Values[Index]->Size;

    if (!ValidateReservedMemoryType (AsciiReservedMemoryType)) {
      DEBUG ((DEBUG_WARN, "UEFI->ReservedMemory[%u]->Type is borked!\n", Index));
      ++ErrorCount;
    }

    if (ReservedMemoryAddress % EFI_PAGE_SIZE != 0) {
      DEBUG ((DEBUG_WARN, "UEFI->ReservedMemory[%u]->Address (%Lu) cannot be divided by page size!\n", Index, ReservedMemoryAddress));
      ++ErrorCount;
    }

    if (ReservedMemorySize == 0ULL) {
      DEBUG ((DEBUG_WARN, "UEFI->ReservedMemory[%u]->Size cannot be zero!\n", Index));
      ++ErrorCount;
    } else if (ReservedMemorySize % EFI_PAGE_SIZE != 0) {
      DEBUG ((DEBUG_WARN, "UEFI->ReservedMemory[%u]->Size (%Lu) cannot be divided by page size!\n", Index, ReservedMemorySize));
      ++ErrorCount;
    }
  }

  //
  // Now overlapping check amongst Address and Size.
  //
  ErrorCount += FindArrayDuplication (
                  Config->Uefi.ReservedMemory.Values,
                  Config->Uefi.ReservedMemory.Count,
                  sizeof (Config->Uefi.ReservedMemory.Values[0]),
                  UefiReservedMemoryHasOverlap
                  );

  return ErrorCount;
}

UINT32
CheckUefi (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32               ErrorCount;
  UINTN                Index;
  STATIC CONFIG_CHECK  UefiCheckers[] = {
    &CheckUefiAPFS,
    &CheckUefiAppleInput,
    &CheckUefiAudio,
    &CheckUefiDrivers,
    &CheckUefiInput,
    &CheckUefiOutput,
    &CheckUefiQuirks,
    &CheckUefiReservedMemory
  };

  DEBUG ((DEBUG_VERBOSE, "config loaded into %a!\n", __func__));

  ErrorCount = 0;

  for (Index = 0; Index < ARRAY_SIZE (UefiCheckers); ++Index) {
    ErrorCount += UefiCheckers[Index](Config);
  }

  return ReportError (__func__, ErrorCount);
}
