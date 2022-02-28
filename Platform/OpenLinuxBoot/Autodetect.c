/** @file
  vmlinuz and initramfs/initrd autodetect.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LinuxBootInternal.h"

#include <Uefi.h>
#include <Guid/Gpt.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcFlexArrayLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/OcBootEntry.h>

#define GRUB_DEFAULT_FILE     L"\\etc\\default\\grub"
#define OS_RELEASE_FILE       L"\\etc\\os-release"
#define AUTODETECT_DIR        L"\\boot"
#define ROOT_DIR              L"\\"
#define ROOT_FS_FILE          L"\\bin\\sh"

STATIC
OC_FLEX_ARRAY
*mVmlinuzFiles;

STATIC
OC_FLEX_ARRAY
*mInitrdFiles;

STATIC
OC_FLEX_ARRAY
*mEtcOsReleaseOptions;

STATIC
CHAR8
*mEtcOsReleaseFileContents;

STATIC
CHAR8
*mPrettyName;

STATIC
CHAR8
*mDiskLabel;

STATIC
OC_FLEX_ARRAY
*mEtcDefaultGrubOptions;

STATIC
CHAR8
*mEtcDefaultGrubFileContents;

STATIC
OC_FLEX_ARRAY
*mPerPartuuidAutoOpts;

STATIC
CHAR16
*mCurrentPartuuidAutoOpts;

STATIC
CHAR16
*mCurrentPartuuidAutoOptsPlus;

STATIC
CHAR16
*mGlobalAutoOpts;

STATIC
CHAR16
*mGlobalAutoOptsPlus;

STATIC
EFI_STATUS
ProcessVmlinuzFile (
  EFI_FILE_HANDLE   Directory,
  EFI_FILE_INFO     *FileInfo,
  UINTN             FileInfoSize,
  VOID              *Context        OPTIONAL
  )
{
  CHAR16        *Dash;
  VMLINUZ_FILE  *VmlinuzFile;

  if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Do not use files without '-' in the name, i.e. we do not need and
  // do not try to use `vmlinuz` or `initrd` symlinks even if present
  // (and even though we can in fact specify them as filenames and boot
  // fine from them).
  //
  Dash = OcStrChr (FileInfo->FileName, L'-');
  if (Dash == NULL || Dash[1] == L'\0') {
    return EFI_NOT_FOUND;
  }

  if (StrnCmp (L"vmlinuz", FileInfo->FileName, L_STR_LEN (L"vmlinuz")) == 0) {
    VmlinuzFile = OcFlexArrayAddItem (mVmlinuzFiles);
  } else if (StrnCmp (L"init", FileInfo->FileName, L_STR_LEN (L"init")) == 0) {
    //
    // initrd* or initramfs*
    //
    VmlinuzFile = OcFlexArrayAddItem (mInitrdFiles);
  } else {
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
    "LNX: Found %s...\n",
    FileInfo->FileName
    ));

  if (VmlinuzFile == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  VmlinuzFile->FileName = AllocateCopyPool (StrSize (FileInfo->FileName), FileInfo->FileName);
  if (VmlinuzFile->FileName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  VmlinuzFile->Version = &VmlinuzFile->FileName[&Dash[1] - FileInfo->FileName];
  VmlinuzFile->StrLen = StrLen (FileInfo->FileName);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CreateAsciiRelativePath (
  CHAR8           **Dest,
  CHAR16          *DirectoryPath,
  UINTN           DirectoryPathLength,
  CHAR16          *FilePath,
  UINTN           FilePathLength
  )
{
  UINTN           Size;
  BOOLEAN         UseDir;

  UseDir = !(DirectoryPathLength == 1 && DirectoryPath[0] == L'\\');

  Size  = (UseDir ? DirectoryPathLength : 0) + FilePathLength + 2;
  *Dest = AllocatePool (Size);

  if (*Dest == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiSPrint (*Dest, Size, "%s\\%s", UseDir ? DirectoryPath : L"", FilePath);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CreateRootPartuuid (
  CHAR8           **Dest
  )
{
  UINTN           Length;
  UINTN           NumPrinted;

  Length = L_STR_LEN ("root=PARTUUID=") + GUID_STRING_LENGTH;

  *Dest = AllocatePool (Length + 1);
  if (*Dest == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NumPrinted = AsciiSPrint (*Dest, Length + 1, "%a%g", "root=PARTUUID=", gPartuuid);
  ASSERT (NumPrinted == Length);

  //
  // Value is case-sensitive and must be lower case.
  //
  OcAsciiToLower (&(*Dest)[L_STR_LEN ("root=PARTUUID=")]);

  return EFI_SUCCESS;
}

STATIC
VOID
AutodetectTitle (
  VOID
  )
{
  UINTN             Index;
  BOOLEAN           Found;

  if (mEtcOsReleaseOptions != NULL) {
    //
    // If neither are present, default title gets set later to "Linux".
    //
    Found = FALSE;
    for (Index = 0; Index < 2; Index++) {
      if (OcParsedVarsGetAsciiStr (
        mEtcOsReleaseOptions,
        Index == 0 ? "PRETTY_NAME" : "NAME",
        &mPrettyName
        )
        && mPrettyName != NULL) {
        Found = TRUE;
        break;
      }
    }

    if (Found) {
      DEBUG ((
        (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
        "LNX: Found distro %a\n",
        mPrettyName
        ));
    } else {
      DEBUG ((DEBUG_WARN, "LNX: Neither %a nor %a found in %s\n", "PRETTY_NAME", "NAME", OS_RELEASE_FILE));
    }
  }
}

STATIC
EFI_STATUS
LoadOsRelease (
  IN     CONST EFI_FILE_PROTOCOL        *RootDirectory,
  IN     CONST BOOLEAN                  IsStandaloneBoot
  )
{
  EFI_STATUS        Status;
  BOOLEAN           TryReading;

  mEtcOsReleaseOptions        = NULL;
  mEtcOsReleaseFileContents   = NULL;
  mPrettyName                 = NULL;

  TryReading = (mDiskLabel == NULL);

  DEBUG_CODE_BEGIN ();
  TryReading = TRUE;
  DEBUG_CODE_END ();

  if (IsStandaloneBoot) {
    TryReading = FALSE;
  }

  //
  // Load distro name from /etc/os-release.
  //
  if (TryReading) {
    mEtcOsReleaseFileContents = OcReadFileFromDirectory (RootDirectory, OS_RELEASE_FILE, NULL, 0);
    if (mEtcOsReleaseFileContents == NULL) {
      DEBUG ((DEBUG_WARN, "LNX: %s not found\n", OS_RELEASE_FILE));
    } else {
      DEBUG ((
        (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
        "LNX: Reading %s\n",
        OS_RELEASE_FILE
        ));
      Status = OcParseVars (mEtcOsReleaseFileContents, &mEtcOsReleaseOptions, FALSE);
      if (EFI_ERROR (Status)) {
        FreePool (mEtcOsReleaseFileContents);
        mEtcOsReleaseFileContents = NULL;
        DEBUG ((DEBUG_WARN, "LNX: Cannot parse %s - %r\n", OS_RELEASE_FILE, Status));
        return Status;
      }

      //
      // Do this early to give a nicer log entry order: distro name from os-release is logged
      // before reports about distro (e.g. before the error below if it is not GRUB-based).
      //
      AutodetectTitle ();
    }
  }

  if (mDiskLabel != NULL) {
    mPrettyName = mDiskLabel;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
LoadDefaultGrub (
  IN     CONST EFI_FILE_PROTOCOL        *RootDirectory,
  IN     CONST BOOLEAN                  IsStandaloneBoot
  )
{
  EFI_STATUS        Status;

  mEtcDefaultGrubOptions      = NULL;
  mEtcDefaultGrubFileContents = NULL;

  if (IsStandaloneBoot) {
    return EFI_SUCCESS;
  }

  //
  // Load kernel options from /etc/default/grub.
  //
  mEtcDefaultGrubFileContents = OcReadFileFromDirectory (RootDirectory, GRUB_DEFAULT_FILE, NULL, 0);
  if (mEtcDefaultGrubFileContents == NULL) {
    DEBUG ((DEBUG_INFO, "LNX: %s not found (bootloader is not GRUB?)\n", GRUB_DEFAULT_FILE));
  } else {
    DEBUG ((
      (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
      "LNX: Reading %s\n",
      GRUB_DEFAULT_FILE
      ));
    Status = OcParseVars (mEtcDefaultGrubFileContents, &mEtcDefaultGrubOptions, FALSE);
    if (EFI_ERROR (Status)) {
      FreePool (mEtcDefaultGrubFileContents);
      mEtcDefaultGrubFileContents = NULL;
      DEBUG ((DEBUG_WARN, "LNX: Cannot parse %s - %r\n", GRUB_DEFAULT_FILE, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
VOID
LoadAppleDiskLabel (
  IN     CONST EFI_FILE_PROTOCOL  *VmlinuzDirectory
  )
{
  mDiskLabel = NULL;

  if ((gPickerContext->PickerAttributes & OC_ATTR_USE_DISK_LABEL_FILE) != 0) {
    mDiskLabel = OcReadFileFromDirectory (VmlinuzDirectory, L".contentDetails", NULL, 0);
    if (mDiskLabel == NULL) {
      mDiskLabel = OcReadFileFromDirectory (VmlinuzDirectory, L".disk_label.contentDetails", NULL, 0);
    }

    if (mDiskLabel == NULL) {
      DEBUG ((DEBUG_INFO, "LNX: %s %s not present\n", L".contentDetails", L".disk_label.contentDetails"));
    } else {
      DEBUG ((DEBUG_INFO, "LNX: Found disk label '%a'\n", mDiskLabel));
    }
  }
}

STATIC
VOID
FreeEtcFiles (
  VOID
  )
{
  //
  // If non-null, refers to string inside mEtcOsReleaseFileContents or copy of mDiskLabel.
  //
  mPrettyName = NULL;

  if (mDiskLabel != NULL) {
    FreePool (mDiskLabel);
    mDiskLabel = NULL;
  }

  OcFlexArrayFree (&mEtcOsReleaseOptions);
  if (mEtcOsReleaseFileContents != NULL) {
    FreePool (mEtcOsReleaseFileContents);
    mEtcOsReleaseFileContents = NULL;
  }

  OcFlexArrayFree (&mEtcDefaultGrubOptions);
  if (mEtcDefaultGrubFileContents != NULL) {
    FreePool (mEtcDefaultGrubFileContents);
    mEtcDefaultGrubFileContents = NULL;
  }
}

STATIC
EFI_STATUS
InsertOption (
  IN     CONST UINTN              InsertIndex,
  IN           OC_FLEX_ARRAY      *Options,
  IN     CONST VOID               *Value,
  IN     CONST BOOLEAN            IsUnicode
  )
{
  EFI_STATUS                      Status;
  UINTN                           OptionsLength;
  UINTN                           CopiedLength;
  CHAR8                           **Option;

  if (IsUnicode) {
    OptionsLength = StrLen (Value);
  } else {
    OptionsLength = AsciiStrLen (Value);
  }

  if (OptionsLength > 0) {
    Option = OcFlexArrayInsertItem (Options, InsertIndex);
    if (Option == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (IsUnicode) {
      *Option = AllocatePool ((OptionsLength + 1) * sizeof (CHAR16));
      if (*Option == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = UnicodeStrnToAsciiStrS (Value, OptionsLength, *Option, OptionsLength + 1, &CopiedLength);
      ASSERT (!EFI_ERROR (Status));
      ASSERT (CopiedLength == OptionsLength);
    } else {
      *Option = AllocateCopyPool (OptionsLength + 1, Value);
      if (*Option == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
AddOption (
  IN           OC_FLEX_ARRAY      *Options,
  IN     CONST VOID               *Value,
  IN     CONST BOOLEAN            IsUnicode
  )
{
  return InsertOption (Options->Count, Options, Value, IsUnicode);
}

EFI_STATUS
InsertRootOption (
  IN           OC_FLEX_ARRAY      *Options
  )
{
  CHAR8             **NewOption;

  DEBUG ((
    (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
    "LNX: Creating \"root=PARTUUID=%g\"\n",
    gPartuuid
    ));

  NewOption = OcFlexArrayInsertItem (Options, 0);
  if (NewOption == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return CreateRootPartuuid (NewOption);
}

STATIC
VOID
GetCurrentPartuuidAutoOpts (
  VOID
  )
{
  UINTN             Index;
  AUTOOPTS          *AutoOpts;

  mCurrentPartuuidAutoOpts     = NULL;
  mCurrentPartuuidAutoOptsPlus = NULL;

  for (Index = 0; Index < mPerPartuuidAutoOpts->Count; Index++) {
    AutoOpts = OcFlexArrayItemAt (mPerPartuuidAutoOpts, Index);
    if (CompareMem (&gPartuuid, &AutoOpts->Guid, sizeof (EFI_GUID)) == 0) {
      if (AutoOpts->PlusOpts) {
        mCurrentPartuuidAutoOptsPlus = AutoOpts->Opts;
      } else {
        mCurrentPartuuidAutoOpts     = AutoOpts->Opts;
      }
    }
  }
}

EFI_STATUS
InternalPreloadAutoOpts (
  IN           OC_FLEX_ARRAY            *Options
  )
{
  EFI_STATUS        Status;
  UINTN             Index;
  OC_PARSED_VAR     *Option;
  AUTOOPTS          *AutoOpts;

  mGlobalAutoOpts     = NULL;
  mGlobalAutoOptsPlus = NULL;

  mPerPartuuidAutoOpts = OcFlexArrayInit (sizeof (AUTOOPTS), NULL);
  if (mPerPartuuidAutoOpts == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Options == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Look for autoopts.
  // Remember that although args are ASCII in the OC config file, they are
  // Unicode by the time they get passed as UEFI LoadOptions.
  //
  for (Index = 0; Index < Options->Count; Index++) {
    Option = OcFlexArrayItemAt (Options, Index);
    //
    // autoopts:{partuuid}[+]="...": user options for specified partuuid.
    //
    if (OcUnicodeStartsWith (Option->Unicode.Name, L"autoopts:", TRUE)) {
      if (Option->Unicode.Value == NULL) {
        DEBUG ((DEBUG_WARN, "LNX: Missing value for %s\n", Option->Unicode.Name));
        continue;
      }

      AutoOpts = OcFlexArrayAddItem (mPerPartuuidAutoOpts);

      Status = StrToGuid (&Option->Unicode.Name[L_STR_LEN (L"autoopts:")], &AutoOpts->Guid);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "LNX: Cannot parse partuuid from %s - %r\n", Option->Unicode.Name, Status));
        OcFlexArrayDiscardItem (mPerPartuuidAutoOpts, FALSE);
        continue;
      }

      AutoOpts->Opts = Option->Unicode.Value;
      AutoOpts->PlusOpts = OcUnicodeEndsWith (Option->Unicode.Name, L"+", FALSE);
    } else if (StrCmp (Option->Unicode.Name, L"autoopts") == 0) {
      if (Option->Unicode.Value == NULL) {
        DEBUG ((DEBUG_WARN, "LNX: Missing value for %s\n", Option->Unicode.Name));
        continue;
      }

      mGlobalAutoOpts = Option->Unicode.Value;
    } else if (StrCmp (Option->Unicode.Name, L"autoopts+") == 0) {
      if (Option->Unicode.Value == NULL) {
        DEBUG ((DEBUG_WARN, "LNX: Missing value for %s\n", Option->Unicode.Name));
        continue;
      }

      mGlobalAutoOptsPlus = Option->Unicode.Value;
    }
  }

  return EFI_SUCCESS;
}

//
// TODO: Options for rescue versions. Would it be better e.g. just to add "ro" and nothing else?
// However on some installs (e.g. where modules to load are specified in the kernel opts) this
// would not boot at all.
// Maybe upgrade to autoopts:{partuuid}r="...": user options for rescue kernels on specified partuuid?
//
STATIC
EFI_STATUS
AutodetectBootOptions (
  IN     CONST BOOLEAN                  IsRescue,
  IN     CONST BOOLEAN                  IsStandaloneBoot,
  IN           OC_FLEX_ARRAY            *Options
)
{
  EFI_STATUS        Status;
  UINTN             Index;
  UINTN             InsertIndex;
  CHAR8             *AsciiStrValue;
  CHAR8             *GrubVarName;
  BOOLEAN           FoundOptions;
  CHAR8             *AddRxOption;

  FoundOptions = FALSE;

  //
  // Do we have user-specified options for this partuuid?
  //
  if (mCurrentPartuuidAutoOpts) {
    DEBUG ((
      (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
      "LNX: Using autoopts:%g%a=\"%s\"\n",
      &gPartuuid,
      "",
      mCurrentPartuuidAutoOpts
      ));
    Status = AddOption (Options, mCurrentPartuuidAutoOpts, TRUE);
    return Status;
  }
  
  if (mCurrentPartuuidAutoOptsPlus) {
    DEBUG ((
      (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
      "LNX: Using autoopts:%g%a=\"%s\"\n",
      &gPartuuid,
      "+",
      mCurrentPartuuidAutoOptsPlus
      ));
    Status = AddOption (Options, mCurrentPartuuidAutoOptsPlus, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    FoundOptions = TRUE;
  }
  
  //
  // Don't use autoopts if partition specific autoopts:{partuuid} already found.
  //
  if (!FoundOptions && mGlobalAutoOpts) {
    DEBUG ((
      (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
      "LNX: Using autoopts%a=\"%s\"\n",
      "",
      mGlobalAutoOpts
      ));

    Status = AddOption (Options, mGlobalAutoOpts, TRUE);
    return Status;
  } else if (mGlobalAutoOptsPlus) {
    DEBUG ((
      (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
      "LNX: Using autoopts%a=\"%s\"\n",
      "+",
      mGlobalAutoOptsPlus
      ));

    Status = AddOption (Options, mGlobalAutoOptsPlus, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    FoundOptions = TRUE;
  }

  //
  // Should only have attempted to detect kernels on standalone boot partition if we had full user options.
  //
  ASSERT (!IsStandaloneBoot);

  //
  // Code only reaches here and below if has been nothing or only += options above.
  // Use options from GRUB default location.
  //
  if (mEtcDefaultGrubOptions != NULL) {
    //
    // Insert these after "ro" but before any user specified opts.
    //
    InsertIndex = 0;

    //
    // If both are present both should be added, standard grub scripts add them
    // in this order.
    // Rescue should only use GRUB_CMDLINE_LINUX so this is correct as
    // far as it goes; however note that rescue options are unfortunately not
    // normally stored here, but are generated in the depths of grub scripts.
    //
    for (Index = 0; Index < (IsRescue ? 1u : 2u); Index++) {
      if (Index == 0) {
        GrubVarName = "GRUB_CMDLINE_LINUX";
      } else {
        GrubVarName = "GRUB_CMDLINE_LINUX_DEFAULT";
      }
      if (OcParsedVarsGetAsciiStr (
        mEtcDefaultGrubOptions,
        GrubVarName,
        &AsciiStrValue
        )
        && AsciiStrValue != NULL) {

        DEBUG ((
          (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
          "LNX: Using %a=\"%a\"\n",
          GrubVarName,
          AsciiStrValue
          ));

        if (AsciiStrValue[0] != '\0') {
          Status = InsertOption (InsertIndex, Options, AsciiStrValue, FALSE);
          if (EFI_ERROR (Status)) {
            return Status;
          }
          //
          // Must not increment insert index if empty option.
          //
          InsertIndex++;
        }

        //
        // Empty string value is good enough for found: we are operating
        // from GRUB cfg files rather than pure guesswork.
        //
        FoundOptions = TRUE;
      }
    }
  }

  //
  // It might be valid to have no options for some kernels or distros, but at least
  // empty (not missing) user specified options or GRUB_CMDLINE_LINUX[_DEFAULT] needs
  // to be present in that case or we stop.
  //
  if (!FoundOptions) {
    DEBUG ((DEBUG_WARN, "LNX: No grub default or user defined options - aborting\n"));
    return EFI_INVALID_PARAMETER;
  }

#if !defined(LINUX_ALLOW_MBR)
  if (CompareGuid (&gPartuuid, &gEfiPartTypeUnusedGuid)) {
    Status = EFI_UNSUPPORTED;
    DEBUG ((DEBUG_WARN, "LNX: Cannot autodetect root on MBR partition - %r\n", Status));
    return Status;
  }
#endif

  //
  // Insert "root=PARTUUID=..." option, followed by "ro" if requested, only if we get to here.
  //
  Status = InsertRootOption (Options);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  InsertIndex = 1;

  AddRxOption = NULL;
  if ((gLinuxBootFlags & LINUX_BOOT_ADD_RW) != 0) {
    AddRxOption = "rw";
  } else if ((gLinuxBootFlags & LINUX_BOOT_ADD_RO) != 0) {
    AddRxOption = "ro";
  }
  if (AddRxOption != NULL) {
    DEBUG ((
      (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
      "LNX: Adding \"%a\"\n",
      AddRxOption
      ));
    Status = InsertOption (InsertIndex, Options, AddRxOption, FALSE);
  }

  return Status;
}

STATIC
EFI_STATUS
GenerateEntriesForVmlinuzFiles (
  IN     CHAR16                   *DirectoryPath,
  IN     CONST BOOLEAN            IsStandaloneBoot
  )
{
  EFI_STATUS                      Status;
  UINTN                           VmlinuzIndex;
  UINTN                           InitrdIndex;
  UINTN                           ShortestMatch;
  UINTN                           DirectoryPathLength;
  LOADER_ENTRY                    *Entry;
  VMLINUZ_FILE                    *VmlinuzFile;
  VMLINUZ_FILE                    *InitrdFile;
  VMLINUZ_FILE                    *InitrdMatch;
  CHAR8                           **Option;
  BOOLEAN                         IsRescue;

  ASSERT (DirectoryPath != NULL);
  ASSERT (mVmlinuzFiles->Count > 0);

  DirectoryPathLength = StrLen (DirectoryPath);

  for (VmlinuzIndex = 0; VmlinuzIndex < mVmlinuzFiles->Count; VmlinuzIndex++) {
    VmlinuzFile = OcFlexArrayItemAt (mVmlinuzFiles, VmlinuzIndex);

    IsRescue = FALSE;
    if (OcUnicodeStartsWith (VmlinuzFile->Version, L"0", FALSE)
      || StrStr (VmlinuzFile->Version, L"rescue") != NULL) {
      //
      // We might have to scan /boot/grb/grub.cfg as grub os-prober does if
      // we want to find rescue version options, or we need to find a way
      // for user to pass these in, since they are generated in the depths
      // of the grub scripts, and in typical distros are not present in
      // /etc/default/grub, even though it looks as if they could be.
      //
      IsRescue = TRUE;
      DEBUG ((DEBUG_INFO, "LNX: %s=rescue\n", VmlinuzFile->Version));
    }

    ShortestMatch = MAX_UINTN;
    InitrdMatch   = NULL;

    //
    // Find shortest init* filename containing the same version string.
    //
    for (InitrdIndex = 0; InitrdIndex < mInitrdFiles->Count; InitrdIndex++) {
      InitrdFile = OcFlexArrayItemAt (mInitrdFiles, InitrdIndex);
      if (InitrdFile->StrLen < ShortestMatch) {
        if (StrStr (InitrdFile->Version, VmlinuzFile->Version) != NULL) {
          InitrdMatch = InitrdFile;
          ShortestMatch = InitrdFile->StrLen;
        }
      }
    }

    Entry = InternalAllocateLoaderEntry ();
    if (Entry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Linux.
    //
    Status = CreateAsciiRelativePath (
      &Entry->Linux,
      DirectoryPath,
      DirectoryPathLength,
      VmlinuzFile->FileName,
      VmlinuzFile->StrLen
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Id and version from filename.
    //
    Status = InternalIdVersionFromFileName (Entry, VmlinuzFile->FileName);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Use title from .contentDetails or os-release file.
    //
    if (mPrettyName != NULL) {
      Entry->Title = AllocateCopyPool (AsciiStrSize (mPrettyName), mPrettyName);
      if (Entry->Title == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    //
    // Initrd.
    //
    if (InitrdMatch == NULL) {
      //
      // Note that where initrd was required user will see clear (and safe, i.e. will
      // not just boot incorrectly) warning from Linux kernel as well.
      //
      DEBUG ((DEBUG_WARN, "LNX: No matching initrd/initramfs file found for %a\n", Entry->Linux));
    } else {
      Option = OcFlexArrayAddItem (Entry->Initrds);
      if (Option == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      Status = CreateAsciiRelativePath (
        Option,
        DirectoryPath,
        DirectoryPathLength,
        InitrdMatch->FileName,
        InitrdMatch->StrLen
        );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    //
    // Add all options.
    //
    Status = AutodetectBootOptions (IsRescue, IsStandaloneBoot, Entry->Options);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
AutodetectLinuxAtDirectory (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  IN   EFI_FILE_PROTOCOL        *VmlinuzDirectory,
  IN   CHAR16                   *AutodetectDir,
  IN   CONST BOOLEAN            IsStandaloneBoot,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                      Status;
  EFI_FILE_PROTOCOL               *RootFsFile;

  mVmlinuzFiles = NULL;
  mInitrdFiles  = NULL;

  Status = EFI_SUCCESS;

  if (!IsStandaloneBoot) {
    Status = OcSafeFileOpen (RootDirectory, &RootFsFile, ROOT_FS_FILE, EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR (Status)) {
      Status = OcEnsureDirectoryFile (RootFsFile, FALSE);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "LNX: %s found but not a %a - %r\n", ROOT_FS_FILE, "file", Status));
      }
      RootFsFile->Close (RootFsFile);
    }
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "LNX: AutodetectLinux not root fs - %r\n", Status));
    }
  }

  if (!EFI_ERROR (Status)) {
    mVmlinuzFiles = OcFlexArrayInit (sizeof (VMLINUZ_FILE), OcFlexArrayFreePointerItem);
    if (mVmlinuzFiles == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  if (!EFI_ERROR (Status)) {
    mInitrdFiles = OcFlexArrayInit (sizeof (VMLINUZ_FILE), OcFlexArrayFreePointerItem);
    if (mInitrdFiles == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Place vmlinuz* and init* files into arrays.
  //
  if (!EFI_ERROR (Status)) {
    Status = OcScanDirectory (VmlinuzDirectory, ProcessVmlinuzFile, NULL);

    if (!EFI_ERROR (Status) && mVmlinuzFiles->Count == 0) {
      //
      // If initrd files but no vmlinuz files are present in autodetect dir.
      //
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status)) {
    gLoaderEntries = OcFlexArrayInit (
      sizeof (LOADER_ENTRY),
      (OC_FLEX_ARRAY_FREE_ITEM)InternalFreeLoaderEntry
      );
    if (gLoaderEntries == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      LoadAppleDiskLabel (VmlinuzDirectory);
      Status = LoadOsRelease (RootDirectory, IsStandaloneBoot);
      if (!EFI_ERROR (Status)) {
        Status = LoadDefaultGrub (RootDirectory, IsStandaloneBoot);
      }
      if (!EFI_ERROR (Status)) {
        Status = GenerateEntriesForVmlinuzFiles (AutodetectDir, IsStandaloneBoot);
      }
      FreeEtcFiles ();
    }

    if (!EFI_ERROR (Status)) {
      Status = InternalConvertLoaderEntriesToBootEntries (
        RootDirectory,
        Entries,
        NumEntries
      );
    }

    OcFlexArrayFree (&gLoaderEntries);
  }

  if (mVmlinuzFiles != NULL) {
    OcFlexArrayFree (&mVmlinuzFiles);
  }

  if (mInitrdFiles != NULL) {
    OcFlexArrayFree (&mInitrdFiles);
  }

  return Status;
}

STATIC
EFI_STATUS
DoAutodetectLinux (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  IN   EFI_FILE_PROTOCOL        *VmlinuzDirectory,
  IN   CHAR16                   *AutodetectDir,
  IN   CONST BOOLEAN            IsStandaloneBoot,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                      Status;

  Status = AutodetectLinuxAtDirectory (RootDirectory, VmlinuzDirectory, AutodetectDir, IsStandaloneBoot, Entries, NumEntries);

  DEBUG ((
    (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) ? DEBUG_WARN : DEBUG_INFO,
    "LNX: AutodetectLinux %s - %r\n",
    AutodetectDir,
    Status
    ));

  return Status;
}

EFI_STATUS
InternalAutodetectLinux (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                      Status;
  EFI_FILE_PROTOCOL               *VmlinuzDirectory;

  GetCurrentPartuuidAutoOpts ();

  //
  // Autodetect at /boot if present.
  //
  Status = OcSafeFileOpen (RootDirectory, &VmlinuzDirectory, AUTODETECT_DIR, EFI_FILE_MODE_READ, 0);
  if (!EFI_ERROR (Status)) {
    Status = DoAutodetectLinux (RootDirectory, VmlinuzDirectory, AUTODETECT_DIR, FALSE, Entries, NumEntries);
    VmlinuzDirectory->Close (VmlinuzDirectory);
  }

  //
  // Try to autodetect kernels at / only if detecting at /boot failed and we have full user-specified
  // options, since we can't autodetect any options on standalone /boot (unless we parse grub.cfg for
  // boot entries, and even then not all standalone boot setups have it).
  //
  if (EFI_ERROR (Status)) {
    if (mCurrentPartuuidAutoOpts != NULL || (mGlobalAutoOpts != NULL && mCurrentPartuuidAutoOptsPlus == NULL)) {
      Status = DoAutodetectLinux (RootDirectory, RootDirectory, ROOT_DIR, TRUE, Entries, NumEntries);
    } else {
      DEBUG ((
        (gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
        "LNX: Not trying to autodetect kernel on possible standalone /boot partition without full autoopts\n"
        ));
    }
  }

  return Status;
}
