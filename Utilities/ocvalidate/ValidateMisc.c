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
#include "OcValidateLib.h"
#include "KextInfo.h"

#include <Library/BaseLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConfigurationLib.h>
#include <Protocol/OcLog.h>

/**
  Callback function to verify whether Arguments and Path are duplicated in Misc->Entries.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
MiscEntriesHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  //
  // NOTE: Entries and Tools share the same constructor.
  //
  CONST OC_MISC_TOOLS_ENTRY  *MiscEntriesPrimaryEntry;
  CONST OC_MISC_TOOLS_ENTRY  *MiscEntriesSecondaryEntry;
  CONST CHAR8                *MiscEntriesPrimaryArgumentsString;
  CONST CHAR8                *MiscEntriesSecondaryArgumentsString;
  CONST CHAR8                *MiscEntriesPrimaryPathString;
  CONST CHAR8                *MiscEntriesSecondaryPathString;

  MiscEntriesPrimaryEntry             = *(CONST OC_MISC_TOOLS_ENTRY **)PrimaryEntry;
  MiscEntriesSecondaryEntry           = *(CONST OC_MISC_TOOLS_ENTRY **)SecondaryEntry;
  MiscEntriesPrimaryArgumentsString   = OC_BLOB_GET (&MiscEntriesPrimaryEntry->Arguments);
  MiscEntriesSecondaryArgumentsString = OC_BLOB_GET (&MiscEntriesSecondaryEntry->Arguments);
  MiscEntriesPrimaryPathString        = OC_BLOB_GET (&MiscEntriesPrimaryEntry->Path);
  MiscEntriesSecondaryPathString      = OC_BLOB_GET (&MiscEntriesSecondaryEntry->Path);

  if (!MiscEntriesPrimaryEntry->Enabled || !MiscEntriesSecondaryEntry->Enabled) {
    return FALSE;
  }

  if (  (AsciiStrCmp (MiscEntriesPrimaryArgumentsString, MiscEntriesSecondaryArgumentsString) == 0)
     && (AsciiStrCmp (MiscEntriesPrimaryPathString, MiscEntriesSecondaryPathString) == 0))
  {
    DEBUG ((DEBUG_WARN, "Misc->Entries->Arguments: %a is duplicated ", MiscEntriesPrimaryPathString));
    return TRUE;
  }

  return FALSE;
}

/**
  Callback function to verify whether Arguments and Path are duplicated in Misc->Tools.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
MiscToolsHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  CONST OC_MISC_TOOLS_ENTRY  *MiscToolsPrimaryEntry;
  CONST OC_MISC_TOOLS_ENTRY  *MiscToolsSecondaryEntry;
  CONST CHAR8                *MiscToolsPrimaryArgumentsString;
  CONST CHAR8                *MiscToolsSecondaryArgumentsString;
  CONST CHAR8                *MiscToolsPrimaryPathString;
  CONST CHAR8                *MiscToolsSecondaryPathString;
  BOOLEAN                    MiscToolsPrimaryFullNvramAccess;
  BOOLEAN                    MiscToolsSecondaryFullNvramAccess;

  MiscToolsPrimaryEntry             = *(CONST OC_MISC_TOOLS_ENTRY **)PrimaryEntry;
  MiscToolsSecondaryEntry           = *(CONST OC_MISC_TOOLS_ENTRY **)SecondaryEntry;
  MiscToolsPrimaryArgumentsString   = OC_BLOB_GET (&MiscToolsPrimaryEntry->Arguments);
  MiscToolsSecondaryArgumentsString = OC_BLOB_GET (&MiscToolsSecondaryEntry->Arguments);
  MiscToolsPrimaryPathString        = OC_BLOB_GET (&MiscToolsPrimaryEntry->Path);
  MiscToolsSecondaryPathString      = OC_BLOB_GET (&MiscToolsSecondaryEntry->Path);
  MiscToolsPrimaryFullNvramAccess   = MiscToolsPrimaryEntry->FullNvramAccess;
  MiscToolsSecondaryFullNvramAccess = MiscToolsSecondaryEntry->FullNvramAccess;

  if (!MiscToolsPrimaryEntry->Enabled || !MiscToolsSecondaryEntry->Enabled) {
    return FALSE;
  }

  if (  (AsciiStrCmp (MiscToolsPrimaryArgumentsString, MiscToolsSecondaryArgumentsString) == 0)
     && (AsciiStrCmp (MiscToolsPrimaryPathString, MiscToolsSecondaryPathString) == 0)
     && (MiscToolsPrimaryFullNvramAccess == MiscToolsSecondaryFullNvramAccess))
  {
    DEBUG ((DEBUG_WARN, "Misc->Tools->Path: %a is duplicated ", MiscToolsPrimaryPathString));
    return TRUE;
  }

  return FALSE;
}

/**
  Validate if SecureBootModel has allowed value.

  @param[in]  SecureBootModel  SecureBootModel retrieved from user config.

  @retval     TRUE             If SecureBootModel is valid.
**/
STATIC
BOOLEAN
ValidateSecureBootModel (
  IN  CONST CHAR8  *SecureBootModel
  )
{
  UINTN               Index;
  STATIC CONST CHAR8  *AllowedSecureBootModel[] = {
    "Default", "Disabled",
    "j137",    "j680",    "j132",   "j174",  "j140k",
    "j780",    "j213",    "j140a",  "j152f", "j160",
    "j230k",   "j214k",   "j223",   "j215",  "j185", "j185f",
    "x86legacy"
  };

  for (Index = 0; Index < ARRAY_SIZE (AllowedSecureBootModel); ++Index) {
    if (AsciiStrCmp (SecureBootModel, AllowedSecureBootModel[Index]) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
UINT32
CheckBlessOverride (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32              ErrorCount;
  UINT32              Index;
  UINTN               Index2;
  CONST CHAR8         *BlessOverrideEntry;
  STATIC CONST CHAR8  *DisallowedBlessOverrideValues[] = {
    "\\EFI\\Microsoft\\Boot\\bootmgfw.efi",
    "\\System\\Library\\CoreServices\\boot.efi",
  };

  ErrorCount = 0;

  for (Index = 0; Index < Config->Misc.BlessOverride.Count; ++Index) {
    BlessOverrideEntry = OC_BLOB_GET (Config->Misc.BlessOverride.Values[Index]);

    //
    // &DisallowedBlessOverrideValues[][1] means no first '\\'.
    //
    for (Index2 = 0; Index2 < ARRAY_SIZE (DisallowedBlessOverrideValues); ++Index2) {
      if (  (AsciiStrCmp (BlessOverrideEntry, DisallowedBlessOverrideValues[Index2]) == 0)
         || (AsciiStrCmp (BlessOverrideEntry, &DisallowedBlessOverrideValues[Index2][1]) == 0))
      {
        DEBUG ((DEBUG_WARN, "Misc->BlessOverride: %a is redundant!\n", BlessOverrideEntry));
        ++ErrorCount;
      }
    }
  }

  return ErrorCount;
}

STATIC
UINT32
ValidateInstanceIdentifier (
  IN CONST CHAR8  *InstanceIdentifier
  )
{
  UINT32  ErrorCount;
  CHAR8   InstanceIdentifierCopy[OC_MAX_INSTANCE_IDENTIFIER_SIZE];
  UINTN   Length;

  ErrorCount = 0;

  if (AsciiStrSize (InstanceIdentifier) > OC_MAX_INSTANCE_IDENTIFIER_SIZE) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->InstanceIdentifier cannot be longer than %d bytes!\n", OC_MAX_INSTANCE_IDENTIFIER_SIZE));
    ++ErrorCount;
  } else {
    if (AsciiStrStr (InstanceIdentifier, ",") != NULL) {
      DEBUG ((DEBUG_WARN, "Misc->Boot->InstanceIdentifier cannot contain comma (,)!\n"));
      ++ErrorCount;
    }

    //
    // Illegal chars
    //
    Length = AsciiStrLen (InstanceIdentifier);
    AsciiStrnCpyS (InstanceIdentifierCopy, OC_MAX_INSTANCE_IDENTIFIER_SIZE, InstanceIdentifier, Length);
    AsciiFilterString (InstanceIdentifierCopy, TRUE);
    if (OcAsciiStrniCmp (InstanceIdentifierCopy, InstanceIdentifier, Length) != 0) {
      DEBUG ((DEBUG_WARN, "Misc->Boot->InstanceIdentifier cannot contain CR, LF, TAB or any other non-ASCII characters!\n"));
      ++ErrorCount;
    }
  }

  return ErrorCount;
}

STATIC
UINT32
CheckMiscBoot (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32                ErrorCount;
  UINT32                ConsoleAttributes;
  CONST CHAR8           *HibernateMode;
  UINT32                PickerAttributes;
  UINT32                Index;
  OC_UEFI_DRIVER_ENTRY  *DriverEntry;
  CONST CHAR8           *Driver;
  BOOLEAN               HasOpenCanopyEfiDriver;
  CONST CHAR8           *PickerMode;
  CONST CHAR8           *PickerVariant;
  CONST CHAR8           *InstanceIdentifier;
  UINTN                 PVSumSize;
  UINTN                 PVPathFixedSize;
  BOOLEAN               IsPickerAudioAssistEnabled;
  BOOLEAN               IsAudioSupportEnabled;
  CONST CHAR8           *LauncherOption;
  CONST CHAR8           *LauncherPath;

  ErrorCount = 0;

  ConsoleAttributes = Config->Misc.Boot.ConsoleAttributes;
  if ((ConsoleAttributes & ~0x7FU) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->ConsoleAttributes has unknown bits set!\n"));
    ++ErrorCount;
  }

  HibernateMode = OC_BLOB_GET (&Config->Misc.Boot.HibernateMode);
  if (  (AsciiStrCmp (HibernateMode, "None") != 0)
     && (AsciiStrCmp (HibernateMode, "Auto") != 0)
     && (AsciiStrCmp (HibernateMode, "RTC") != 0)
     && (AsciiStrCmp (HibernateMode, "NVRAM") != 0))
  {
    DEBUG ((DEBUG_WARN, "Misc->Boot->HibernateMode is borked (Can only be None, Auto, RTC, or NVRAM)!\n"));
    ++ErrorCount;
  }

  PickerAttributes = Config->Misc.Boot.PickerAttributes;
  if ((PickerAttributes & ~OC_ATTR_ALL_BITS) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->PickerAttributes has unknown bits set!\n"));
    ++ErrorCount;
  }

  HasOpenCanopyEfiDriver = FALSE;
  for (Index = 0; Index < Config->Uefi.Drivers.Count; ++Index) {
    DriverEntry = Config->Uefi.Drivers.Values[Index];
    Driver      = OC_BLOB_GET (&DriverEntry->Path);

    if (DriverEntry->Enabled && (AsciiStrCmp (Driver, "OpenCanopy.efi") == 0)) {
      HasOpenCanopyEfiDriver = TRUE;
    }
  }

  PickerMode = OC_BLOB_GET (&Config->Misc.Boot.PickerMode);
  if (  (AsciiStrCmp (PickerMode, "Builtin") != 0)
     && (AsciiStrCmp (PickerMode, "External") != 0)
     && (AsciiStrCmp (PickerMode, "Apple") != 0))
  {
    DEBUG ((DEBUG_WARN, "Misc->Boot->PickerMode is borked (Can only be Builtin, External, or Apple)!\n"));
    ++ErrorCount;
  } else if (HasOpenCanopyEfiDriver && (AsciiStrCmp (PickerMode, "External") != 0)) {
    DEBUG ((DEBUG_WARN, "OpenCanopy.efi is loaded at UEFI->Drivers, but Misc->Boot->PickerMode is not set to External!\n"));
    ++ErrorCount;
  }

  PickerVariant = OC_BLOB_GET (&Config->Misc.Boot.PickerVariant);
  if (PickerVariant[0] == '\0') {
    DEBUG ((DEBUG_WARN, "Misc->Boot->PickerVariant cannot be empty!\n"));
    ++ErrorCount;
  }

  InstanceIdentifier = OC_BLOB_GET (&Config->Misc.Boot.InstanceIdentifier);
  ErrorCount        += ValidateInstanceIdentifier (InstanceIdentifier);

  //
  // Check the length of path relative to OC directory.
  //
  // There is one missing '\\' after the concatenation of PickerVariant and ExtAppleRecv10_15.icns (which has the longest length). Append one.
  //
  PVPathFixedSize = L_STR_LEN (OPEN_CORE_IMAGE_PATH) + 1 + L_STR_SIZE ("ExtAppleRecv10_15.icns");
  PVSumSize       = PVPathFixedSize + AsciiStrLen (PickerVariant);
  if (PVSumSize > OC_STORAGE_SAFE_PATH_MAX) {
    DEBUG ((
      DEBUG_WARN,
      "Misc->Boot->PickerVariant (length %u) is too long (should not exceed %u)!\n",
      AsciiStrLen (PickerVariant),
      OC_STORAGE_SAFE_PATH_MAX - PVPathFixedSize
      ));
    ++ErrorCount;
  }

  IsPickerAudioAssistEnabled = Config->Misc.Boot.PickerAudioAssist;
  IsAudioSupportEnabled      = Config->Uefi.Audio.AudioSupport;
  if (IsPickerAudioAssistEnabled && !IsAudioSupportEnabled) {
    DEBUG ((DEBUG_WARN, "Misc->Boot->PickerAudioAssist is enabled, but UEFI->Audio->AudioSupport is not enabled altogether!\n"));
    ++ErrorCount;
  }

  LauncherOption = OC_BLOB_GET (&Config->Misc.Boot.LauncherOption);
  if (  (AsciiStrCmp (LauncherOption, "Disabled") != 0)
     && (AsciiStrCmp (LauncherOption, "Full") != 0)
     && (AsciiStrCmp (LauncherOption, "Short") != 0)
     && (AsciiStrCmp (LauncherOption, "System") != 0))
  {
    DEBUG ((DEBUG_WARN, "Misc->Boot->LauncherOption is borked (Can only be Disabled, Full, Short, or System)!\n"));
    ++ErrorCount;
  }

  LauncherPath = OC_BLOB_GET (&Config->Misc.Boot.LauncherPath);
  if (LauncherPath[0] == '\0') {
    DEBUG ((DEBUG_WARN, "Misc->Boot->LauncherPath cannot be empty!\n"));
    ++ErrorCount;
  }

  return ErrorCount;
}

STATIC
UINT32
CheckMiscDebug (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32  ErrorCount;
  UINT64  DisplayLevel;
  UINT64  AllowedDisplayLevel;
  UINT64  HaltLevel;
  UINT64  AllowedHaltLevel;
  UINT32  Target;

  ErrorCount = 0;

  //
  // FIXME: Check whether DisplayLevel only supports values within AllowedDisplayLevel, or all possible levels in DebugLib.h?
  //
  DisplayLevel        = Config->Misc.Debug.DisplayLevel;
  AllowedDisplayLevel = DEBUG_WARN | DEBUG_INFO | DEBUG_VERBOSE | DEBUG_ERROR;
  if ((DisplayLevel & ~AllowedDisplayLevel) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Debug->DisplayLevel has unknown bits set!\n"));
    ++ErrorCount;
  }

  HaltLevel        = DisplayLevel;
  AllowedHaltLevel = AllowedDisplayLevel;
  if ((HaltLevel & ~AllowedHaltLevel) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->HaltLevel has unknown bits set!\n"));
    ++ErrorCount;
  }

  Target = Config->Misc.Debug.Target;
  if ((Target & ~OC_LOG_ALL_BITS) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Debug->Target has unknown bits set!\n"));
    ++ErrorCount;
  }

  return ErrorCount;
}

STATIC
UINT32
ValidateFlavour (
  IN CHAR8        *EntryType,
  IN UINT32       Index,
  IN CONST CHAR8  *Flavour
  )
{
  UINT32       ErrorCount;
  CHAR8        FlavourCopy[OC_MAX_CONTENT_FLAVOUR_SIZE];
  UINTN        Length;
  CONST CHAR8  *Start;
  CONST CHAR8  *End;

  ErrorCount = 0;

  if ((Flavour == NULL) || (*Flavour == '\0')) {
    DEBUG ((DEBUG_WARN, "Misc->%a[%u]->Flavour cannot be empty (use \"Auto\")!\n", EntryType, Index));
    ++ErrorCount;
  } else if (AsciiStrSize (Flavour) > OC_MAX_CONTENT_FLAVOUR_SIZE) {
    DEBUG ((DEBUG_WARN, "Misc->%a[%u]->Flavour cannot be longer than %d bytes!\n", EntryType, Index, OC_MAX_CONTENT_FLAVOUR_SIZE));
    ++ErrorCount;
  } else {
    //
    // Illegal chars
    //
    Length = AsciiStrLen (Flavour);
    AsciiStrnCpyS (FlavourCopy, OC_MAX_CONTENT_FLAVOUR_SIZE, Flavour, Length);
    AsciiFilterString (FlavourCopy, TRUE);
    if (OcAsciiStrniCmp (FlavourCopy, Flavour, Length) != 0) {
      DEBUG ((DEBUG_WARN, "Flavour names within Misc->%a[%u]->Flavour cannot contain CR, LF, TAB or any other non-ASCII characters!\n", EntryType, Index));
      ++ErrorCount;
    }

    //
    // Per-name tests
    //
    End = Flavour - 1;
    do {
      for (Start = ++End; *End != '\0' && *End != ':'; ++End) {
      }

      if (Start == End) {
        DEBUG ((DEBUG_WARN, "Flavour names within Misc->%a[%u]->Flavour cannot be empty!\n", EntryType, Index));
        ++ErrorCount;
      } else {
        AsciiStrnCpyS (FlavourCopy, OC_MAX_CONTENT_FLAVOUR_SIZE, Start, End - Start);
        if (OcAsciiStartsWith (FlavourCopy, "Ext", TRUE)) {
          DEBUG ((DEBUG_WARN, "Flavour names within Misc->%a[%u]->Flavour cannot begin with \"Ext\"!\n", EntryType, Index));
          ++ErrorCount;
        }
      }
    } while (*End != '\0');
  }

  return ErrorCount;
}

STATIC
UINT32
CheckMiscEntries (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32        ErrorCount;
  UINT32        Index;
  CONST CHAR8   *Arguments;
  CONST CHAR8   *Comment;
  CONST CHAR8   *AsciiName;
  CONST CHAR16  *UnicodeName;
  CONST CHAR8   *Path;
  CONST CHAR8   *Flavour;

  ErrorCount = 0;

  for (Index = 0; Index < Config->Misc.Entries.Count; ++Index) {
    Arguments = OC_BLOB_GET (&Config->Misc.Entries.Values[Index]->Arguments);
    Comment   = OC_BLOB_GET (&Config->Misc.Entries.Values[Index]->Comment);
    AsciiName = OC_BLOB_GET (&Config->Misc.Entries.Values[Index]->Name);
    Path      = OC_BLOB_GET (&Config->Misc.Entries.Values[Index]->Path);
    Flavour   = OC_BLOB_GET (&Config->Misc.Entries.Values[Index]->Flavour);

    //
    // Sanitise strings.
    //
    // NOTE: As Arguments takes identical requirements of Comment,
    //       we use Comment sanitiser here.
    //
    if (!AsciiCommentIsLegal (Arguments)) {
      DEBUG ((DEBUG_WARN, "Misc->Entries[%u]->Arguments contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Misc->Entries[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    UnicodeName = AsciiStrCopyToUnicode (AsciiName, 0);
    if (UnicodeName != NULL) {
      if (!UnicodeIsFilteredString (UnicodeName, TRUE)) {
        DEBUG ((DEBUG_WARN, "Misc->Entries[%u]->Name contains illegal character!\n", Index));
        ++ErrorCount;
      }

      FreePool ((VOID *)UnicodeName);
    }

    //
    // FIXME: Properly sanitise Path.
    //
    if (!AsciiCommentIsLegal (Path)) {
      DEBUG ((DEBUG_WARN, "Misc->Entries[%u]->Path contains illegal character!\n", Index));
      ++ErrorCount;
    }

    ErrorCount += ValidateFlavour ("Entries", Index, Flavour);
  }

  //
  // Check duplicated entries in Entries.
  //
  ErrorCount += FindArrayDuplication (
                  Config->Misc.Entries.Values,
                  Config->Misc.Entries.Count,
                  sizeof (Config->Misc.Entries.Values[0]),
                  MiscEntriesHasDuplication
                  );

  return ErrorCount;
}

STATIC
UINT32
CheckMiscSecurity (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  UINT32       Index;
  BOOLEAN      IsAuthRestartEnabled;
  BOOLEAN      HasVSMCKext;
  CONST CHAR8  *AsciiDmgLoading;
  UINT32       ExposeSensitiveData;
  CONST CHAR8  *AsciiVault;
  UINT32       ScanPolicy;
  UINT32       AllowedScanPolicy;
  CONST CHAR8  *SecureBootModel;

  ErrorCount = 0;

  HasVSMCKext = FALSE;
  for (Index = 0; Index < Config->Kernel.Add.Count; ++Index) {
    if (AsciiStrCmp (OC_BLOB_GET (&Config->Kernel.Add.Values[Index]->BundlePath), mKextInfo[INDEX_KEXT_VSMC].KextBundlePath) == 0) {
      HasVSMCKext = TRUE;
    }
  }

  IsAuthRestartEnabled = Config->Misc.Security.AuthRestart;
  if (IsAuthRestartEnabled && !HasVSMCKext) {
    DEBUG ((DEBUG_WARN, "Misc->Security->AuthRestart is enabled, but VirtualSMC is not loaded at Kernel->Add!\n"));
    ++ErrorCount;
  }

  AsciiDmgLoading = OC_BLOB_GET (&Config->Misc.Security.DmgLoading);
  if (  (AsciiStrCmp (AsciiDmgLoading, "Disabled") != 0)
     && (AsciiStrCmp (AsciiDmgLoading, "Signed") != 0)
     && (AsciiStrCmp (AsciiDmgLoading, "Any") != 0))
  {
    DEBUG ((DEBUG_WARN, "Misc->Security->DmgLoading is borked (Can only be Disabled, Signed, or Any)!\n"));
    ++ErrorCount;
  }

  ExposeSensitiveData = Config->Misc.Security.ExposeSensitiveData;
  if ((ExposeSensitiveData & ~OCS_EXPOSE_ALL_BITS) != 0) {
    DEBUG ((DEBUG_WARN, "Misc->Security->ExposeSensitiveData has unknown bits set!\n"));
    ++ErrorCount;
  }

  AsciiVault = OC_BLOB_GET (&Config->Misc.Security.Vault);
  if (  (AsciiStrCmp (AsciiVault, "Optional") != 0)
     && (AsciiStrCmp (AsciiVault, "Basic") != 0)
     && (AsciiStrCmp (AsciiVault, "Secure") != 0))
  {
    DEBUG ((DEBUG_WARN, "Misc->Security->Vault is borked (Can only be Optional, Basic, or Secure)!\n"));
    ++ErrorCount;
  }

  ScanPolicy        = Config->Misc.Security.ScanPolicy;
  AllowedScanPolicy = OC_SCAN_FILE_SYSTEM_LOCK | OC_SCAN_DEVICE_LOCK | OC_SCAN_DEVICE_BITS | OC_SCAN_FILE_SYSTEM_BITS;
  //
  // ScanPolicy can be zero (failsafe value), skipping such.
  //
  if (ScanPolicy != 0) {
    if ((ScanPolicy & ~AllowedScanPolicy) != 0) {
      DEBUG ((DEBUG_WARN, "Misc->Security->ScanPolicy has unknown bits set!\n"));
      ++ErrorCount;
    }

    if (((ScanPolicy & OC_SCAN_FILE_SYSTEM_BITS) != 0) && ((ScanPolicy & OC_SCAN_FILE_SYSTEM_LOCK) == 0)) {
      DEBUG ((DEBUG_WARN, "Misc->Security->ScanPolicy requests scanning filesystem, but OC_SCAN_FILE_SYSTEM_LOCK (bit 0) is not set!\n"));
      ++ErrorCount;
    }

    if (((ScanPolicy & OC_SCAN_DEVICE_BITS) != 0) && ((ScanPolicy & OC_SCAN_DEVICE_LOCK) == 0)) {
      DEBUG ((DEBUG_WARN, "Misc->Security->ScanPolicy requests scanning devices, but OC_SCAN_DEVICE_LOCK (bit 1) is not set!\n"));
      ++ErrorCount;
    }
  }

  //
  // Validate SecureBootModel.
  //
  SecureBootModel = OC_BLOB_GET (&Config->Misc.Security.SecureBootModel);
  if (!ValidateSecureBootModel (SecureBootModel)) {
    DEBUG ((DEBUG_WARN, "Misc->Security->SecureBootModel is borked!\n"));
    ++ErrorCount;
  }

  if (  !(  (AsciiStrCmp (AsciiDmgLoading, "Disabled") == 0)
         || (AsciiStrCmp (AsciiDmgLoading, "Signed") == 0)
         || (AsciiDmgLoading[0] == '\0')) ///< Default is "Signed", and assume default will always be secure.
     && (AsciiStrCmp (SecureBootModel, "Disabled") != 0))
  {
    DEBUG ((DEBUG_WARN, "Misc->Security->DmgLoading must be Disabled or Signed unless Misc->Security->SecureBootModel is Disabled!\n"));
    ++ErrorCount;
  }

  return ErrorCount;
}

STATIC
BOOLEAN
ValidateBaudRate (
  IN  UINT32  BaudRate
  )
{
  UINTN  Index;

  //
  // Reference:
  // https://github.com/acidanthera/audk/blob/bb1bba3d776733c41dbfa2d1dc0fe234819a79f2/MdeModulePkg/MdeModulePkg.dec#L1223
  //
  STATIC CONST UINT32  AllowedBaudRate[] = {
    921600U, 460800U, 230400U, 115200U,
    57600U,  38400U,  19200U,  9600U,  7200U,
    4800U,   3600U,   2400U,   2000U,  1800U,
    1200U,   600U,    300U,    150U,   134U,
    110U,    75U,     50U
  };

  for (Index = 0; Index < ARRAY_SIZE (AllowedBaudRate); ++Index) {
    if (BaudRate == AllowedBaudRate[Index]) {
      return TRUE;
    }
  }

  DEBUG ((DEBUG_WARN, "Misc->Serial->BaudRate is borked!\n"));
  DEBUG ((DEBUG_WARN, "Accepted BaudRate values:\n"));
  for (Index = 0; Index < ARRAY_SIZE (AllowedBaudRate); ++Index) {
    DEBUG ((DEBUG_WARN, "%u, ", AllowedBaudRate[Index]));
    if ((Index != 0) && (Index % 5 == 0)) {
      DEBUG ((DEBUG_WARN, "\n"));
    }
  }

  DEBUG ((DEBUG_WARN, "\n"));
  return FALSE;
}

STATIC
UINT32
CheckMiscSerial (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32       ErrorCount;
  UINT32       RegisterAccessWidth;
  UINT32       BaudRate;
  CONST UINT8  *PciDeviceInfo;
  UINT32       PciDeviceInfoSize;

  ErrorCount = 0;

  //
  // Reference:
  // https://github.com/acidanthera/audk/blob/bb1bba3d776733c41dbfa2d1dc0fe234819a79f2/MdeModulePkg/MdeModulePkg.dec#L1199-L1200
  //
  RegisterAccessWidth = Config->Misc.Serial.Custom.RegisterAccessWidth;
  if ((RegisterAccessWidth != 8U) && (RegisterAccessWidth != 32U)) {
    DEBUG ((DEBUG_WARN, "Misc->Serial->RegisterAccessWidth can only be 8 or 32!\n"));
    ++ErrorCount;
  }

  BaudRate = Config->Misc.Serial.Custom.BaudRate;
  if (!ValidateBaudRate (BaudRate)) {
    ++ErrorCount;
  }

  //
  // Reference:
  // https://github.com/acidanthera/audk/blob/bb1bba3d776733c41dbfa2d1dc0fe234819a79f2/MdeModulePkg/MdeModulePkg.dec#L1393
  //
  PciDeviceInfo     = OC_BLOB_GET (&Config->Misc.Serial.Custom.PciDeviceInfo);
  PciDeviceInfoSize = Config->Misc.Serial.Custom.PciDeviceInfo.Size;
  if (PciDeviceInfoSize > OC_SERIAL_PCI_DEVICE_INFO_MAX_SIZE) {
    DEBUG ((DEBUG_WARN, "Size of Misc->Serial->PciDeviceInfo cannot exceed %u!\n", OC_SERIAL_PCI_DEVICE_INFO_MAX_SIZE));
    ++ErrorCount;
  } else if (PciDeviceInfoSize == 0) {
    DEBUG ((DEBUG_WARN, "Misc->Serial->PciDeviceInfo cannot be empty (use 0xFF instead)!\n"));
    ++ErrorCount;
  } else {
    if (PciDeviceInfo[PciDeviceInfoSize - 1] != 0xFFU) {
      DEBUG ((DEBUG_WARN, "Last byte of Misc->Serial->PciDeviceInfo must be 0xFF!\n"));
      ++ErrorCount;
    }

    if ((PciDeviceInfoSize - 1) % 4 != 0) {
      DEBUG ((DEBUG_WARN, "Misc->Serial->PciDeviceInfo must be divisible by 4 excluding the last 0xFF!\n"));
      ++ErrorCount;
    }
  }

  return ErrorCount;
}

STATIC
UINT32
CheckMiscTools (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32        ErrorCount;
  UINT32        Index;
  CONST CHAR8   *Arguments;
  CONST CHAR8   *Comment;
  CONST CHAR8   *AsciiName;
  CONST CHAR16  *UnicodeName;
  CONST CHAR8   *Path;
  CONST CHAR8   *Flavour;

  ErrorCount = 0;

  for (Index = 0; Index < Config->Misc.Tools.Count; ++Index) {
    Arguments = OC_BLOB_GET (&Config->Misc.Tools.Values[Index]->Arguments);
    Comment   = OC_BLOB_GET (&Config->Misc.Tools.Values[Index]->Comment);
    AsciiName = OC_BLOB_GET (&Config->Misc.Tools.Values[Index]->Name);
    Path      = OC_BLOB_GET (&Config->Misc.Tools.Values[Index]->Path);
    Flavour   = OC_BLOB_GET (&Config->Misc.Tools.Values[Index]->Flavour);

    //
    // Sanitise strings.
    //
    // NOTE: As Arguments takes identical requirements of Comment,
    //       we use Comment sanitiser here.
    //
    if (!AsciiCommentIsLegal (Arguments)) {
      DEBUG ((DEBUG_WARN, "Misc->Tools[%u]->Arguments contains illegal character!\n", Index));
      ++ErrorCount;
    }

    if (!AsciiCommentIsLegal (Comment)) {
      DEBUG ((DEBUG_WARN, "Misc->Tools[%u]->Comment contains illegal character!\n", Index));
      ++ErrorCount;
    }

    //
    // Check the length of path relative to OC directory.
    //
    if (L_STR_LEN (OPEN_CORE_TOOL_PATH) + AsciiStrSize (Path) > OC_STORAGE_SAFE_PATH_MAX) {
      DEBUG ((DEBUG_WARN, "Misc->Tools[%u]->Path is too long (should not exceed %u)!\n", Index, OC_STORAGE_SAFE_PATH_MAX));
      ++ErrorCount;
    }

    UnicodeName = AsciiStrCopyToUnicode (AsciiName, 0);
    if (UnicodeName != NULL) {
      if (!UnicodeIsFilteredString (UnicodeName, TRUE)) {
        DEBUG ((DEBUG_WARN, "Misc->Tools[%u]->Name contains illegal character!\n", Index));
        ++ErrorCount;
      }

      FreePool ((VOID *)UnicodeName);
    }

    //
    // FIXME: Properly sanitise Path.
    //
    if (!AsciiCommentIsLegal (Path)) {
      DEBUG ((DEBUG_WARN, "Misc->Tools[%u]->Path contains illegal character!\n", Index));
      ++ErrorCount;
    }

    ErrorCount += ValidateFlavour ("Tools", Index, Flavour);
  }

  //
  // Check duplicated entries in Tools.
  //
  ErrorCount += FindArrayDuplication (
                  Config->Misc.Tools.Values,
                  Config->Misc.Tools.Count,
                  sizeof (Config->Misc.Tools.Values[0]),
                  MiscToolsHasDuplication
                  );

  return ErrorCount;
}

UINT32
CheckMisc (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32               ErrorCount;
  UINTN                Index;
  STATIC CONFIG_CHECK  MiscCheckers[] = {
    &CheckBlessOverride,
    &CheckMiscBoot,
    &CheckMiscDebug,
    &CheckMiscEntries,
    &CheckMiscSecurity,
    &CheckMiscSerial,
    &CheckMiscTools
  };

  DEBUG ((DEBUG_VERBOSE, "config loaded into %a!\n", __func__));

  ErrorCount = 0;

  for (Index = 0; Index < ARRAY_SIZE (MiscCheckers); ++Index) {
    ErrorCount += MiscCheckers[Index](Config);
  }

  return ReportError (__func__, ErrorCount);
}
