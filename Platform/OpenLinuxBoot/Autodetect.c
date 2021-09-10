/** @file
  vmlinuz and initramfs/initrd autodetect.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "LinuxBootInternal.h"

#include <Uefi.h>
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
*mPrettyName;

STATIC
OC_FLEX_ARRAY
*mEtcDefaultGrubOptions;

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

  DEBUG (((gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
    "LNX: Found %s...\n", FileInfo->FileName ));

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

  Size = DirectoryPathLength + FilePathLength + 2;
  *Dest = AllocatePool (Size);
  if (*Dest == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  AsciiSPrint (*Dest, Size, "%s\\%s", DirectoryPath, FilePath);
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
  CHAR8             *AsciiStrValue;
  BOOLEAN           Found;

  mPrettyName = NULL;
  if (mEtcOsReleaseOptions != NULL) {
    //
    // If neither are present, default title gets set later to "Linux".
    //
    Found = FALSE;
    for (Index = 0; Index < 2; Index++) {
      if (OcParsedVarsGetAsciiStr (
        mEtcOsReleaseOptions,
        Index == 0 ? "PRETTY_NAME" : "NAME",
        &AsciiStrValue
        ) &&
        AsciiStrValue != NULL) {
        mPrettyName = AsciiStrValue;
        Found = TRUE;
        break;
      }
    }

    if (Found) {
      DEBUG (((gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
        "LNX: Found distro %a\n", mPrettyName));
    } else {
      DEBUG ((DEBUG_WARN, "LNX: Neither %a nor %a found in %s\n", "PRETTY_NAME", "NAME", OS_RELEASE_FILE));
    }
  }
}

STATIC
EFI_STATUS
LoadEtcFiles (
  IN     CONST EFI_FILE_PROTOCOL        *RootDirectory
  )
{
  EFI_STATUS        Status;
  CHAR8             *Contents;

  Status = EFI_SUCCESS;

  mEtcOsReleaseOptions = NULL;

  //
  // Load distro name from /etc/os-release.
  //
  Contents = OcReadFileFromDirectory (RootDirectory, OS_RELEASE_FILE, NULL, 0);
  if (Contents == NULL) {
    DEBUG ((DEBUG_WARN, "LNX: %s not found\n", OS_RELEASE_FILE));
  } else {
    DEBUG (((gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
      "LNX: Reading %s\n", OS_RELEASE_FILE));
    Status = OcParseVars (Contents, &mEtcOsReleaseOptions, FALSE);
    if (EFI_ERROR (Status)) {
      FreePool (Contents);
      DEBUG ((DEBUG_WARN, "LNX: Cannot parse %s - %r\n", OS_RELEASE_FILE, Status));
      return Status;
    }

    //
    // Do this early purely to give a nicer log entry order - distro is named
    // before reports about it (esp. e.g. error below if it is not GRUB-based).
    //
    AutodetectTitle ();
  }

  //
  // Load kernel options from /etc/default/grub.
  //
  Contents   = OcReadFileFromDirectory (RootDirectory, GRUB_DEFAULT_FILE, NULL, 0);
  if (Contents == NULL) {
    DEBUG ((DEBUG_WARN, "LNX: %s not found (bootloader is not GRUB?)\n", GRUB_DEFAULT_FILE));
  } else {
    DEBUG (((gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
      "LNX: Reading %s\n", GRUB_DEFAULT_FILE));
    Status = OcParseVars (Contents, &mEtcDefaultGrubOptions, FALSE);
    if (EFI_ERROR (Status)) {
      FreePool (Contents);
      DEBUG ((DEBUG_WARN, "LNX: Cannot parse %s - %r\n", GRUB_DEFAULT_FILE, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
VOID
FreeEtcFiles (
  VOID
  )
{
  OcFlexArrayFree (&mEtcOsReleaseOptions);
  OcFlexArrayFree (&mEtcDefaultGrubOptions);
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

//
// TODO: Options for rescue versions. Would it be better e.g. just to add "ro" and nothing else?
// However on some installs (e.g. where modules to load are specified in the kernel opts) this
// would not boot at all.
// Maybe upgrade to partuuidopts:{partuuid}r="...": user options for rescue kernels on specified partuuid?
//
STATIC
EFI_STATUS
AutodetectBootOptions (
  IN     CONST BOOLEAN                  IsRescue,
  IN           OC_FLEX_ARRAY            *Options
)
{
  EFI_STATUS        Status;
  UINTN             Index;
  UINTN             InsertIndex;
  OC_PARSED_VAR     *Option;
  EFI_GUID          Guid;
  CHAR8             *AsciiStrValue;
  BOOLEAN           Found;

  if ((gLinuxBootFlags & LINUX_BOOT_ADD_RO) != 0) {
    DEBUG ((OC_TRACE_KERNEL_OPTS, "LNX: Adding \"ro\"\n"));
    Status = AddOption (Options, "ro", FALSE);
  }

  Found       = FALSE;
  InsertIndex = Options->Count;

  //
  // Look for user-specified options for this partuuid.
  // Remember that although args are ASCII in the OC config file, they are
  // Unicode by the time they get passed as UEFI LoadOptions.
  //
  for (Index = 0; Index < gParsedLoadOptions->Count; Index++) {
    Option = OcFlexArrayItemAt (gParsedLoadOptions, Index);
    //
    // partuuidopts:{partuuid}[+]="...": user options for specified partuuid.
    //
    if (OcUnicodeStartsWith (Option->Unicode.Name, L"partuuidopts:", TRUE)) {
      if (Option->Unicode.Value == NULL) {
        DEBUG ((DEBUG_WARN, "LNX: Missing value for %s\n", Option->Unicode.Name));
        continue;
      }

      Status = StrToGuid (&Option->Unicode.Name[L_STR_LEN(L"partuuidopts:")], &Guid);
      if (EFI_ERROR(Status)) {
        DEBUG ((DEBUG_WARN, "LNX: Cannot parse partuuid from %s - %r\n", Option->Unicode.Name, Status));
        continue;
      }
      
      if (CompareMem (&gPartuuid, &Guid, sizeof (EFI_GUID)) != 0) {
        DEBUG ((OC_TRACE_KERNEL_OPTS, "LNX: No match %g != %g\n", &gPartuuid, &Guid));
      } else {
        DEBUG ((OC_TRACE_KERNEL_OPTS, "LNX: Using partuuidopts=\"%s\"\n", Option->Unicode.Value));

        Status = AddOption (Options, Option->Unicode.Value, TRUE);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        //
        // partuuidopts:{partuuid}+="...": use user options in addition to detected options.
        //
        if (!OcUnicodeEndsWith (Option->Unicode.Name, L"+", FALSE)) {
          return EFI_SUCCESS;
        }

        Found = TRUE;
      }
    }
  }

  //
  // Use options from GRUB default location.
  //
  if (mEtcDefaultGrubOptions != NULL) {
    //
    // If both are present both should be added, standard grub scripts add them
    // in this order.
    // Rescue should only use GRUB_CMDLINE_LINUX so this is correct as
    // far as it goes; however note that rescue options are unfortunately not
    // normally stored here, but are generated in the depths of grub scripts.
    //
    for (Index = 0; Index < (IsRescue ? 1u : 2u); Index++) {
      if (OcParsedVarsGetAsciiStr (
        mEtcDefaultGrubOptions,
        Index == 0 ? "GRUB_CMDLINE_LINUX" : "GRUB_CMDLINE_LINUX_DEFAULT",
        &AsciiStrValue
        ) &&
        AsciiStrValue != NULL) {

        //
        // Insert these after "ro" but before "partuuidopts+".
        //
        if (AsciiStrValue[0] != '\0') {
          Status = InsertOption (InsertIndex, Options, AsciiStrValue, FALSE);
          if (EFI_ERROR (Status)) {
            return Status;
          }
          InsertIndex++;
        }

        //
        // Empty string value is good enough for found: we are operating
        // from GRUB cfg files rather than pure guesswork.
        //
        Found = TRUE;
      }
    }
  }

  //
  // Use global defaults, if user has defined any.
  //
  for (Index = 0; Index < gParsedLoadOptions->Count; Index++) {
    Option = OcFlexArrayItemAt (gParsedLoadOptions, Index);
    if (!Found && StrCmp (Option->Unicode.Name, L"autoopts") == 0) {
      if (Option->Unicode.Value == NULL) {
        DEBUG ((DEBUG_WARN, "LNX: Missing value for %s\n", Option->Unicode.Name));
        continue;
      }

      Status = AddOption (Options, Option->Unicode.Value, TRUE);
      return Status;
    } else if (StrCmp (Option->Unicode.Name, L"autoopts+") == 0) {
      if (Option->Unicode.Value == NULL) {
        DEBUG ((DEBUG_WARN, "LNX: Missing value for %s\n", Option->Unicode.Name));
        continue;
      }

      Status = AddOption (Options, Option->Unicode.Value, TRUE);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Found = TRUE;
    }
  }

  //
  // It might be valid to have no options except "ro", but at least empty
  // string "GRUB_CMDLINE_LINUX" needs to be present in that case or we stop.
  //
  if (!Found) {
    DEBUG ((DEBUG_WARN, "LNX: No grub default or user defined options - aborting\n"));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GenerateEntriesForVmlinuzFiles (
  IN     CHAR16                   *DirectoryPath
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
    Status = CreateAsciiRelativePath (&Entry->Linux, DirectoryPath, DirectoryPathLength, VmlinuzFile->FileName, VmlinuzFile->StrLen);
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
    // Use title from os-release file.
    //
    Entry->Title = AllocateCopyPool (AsciiStrSize (mPrettyName), mPrettyName);
    if (Entry->Title == NULL) {
      return EFI_OUT_OF_RESOURCES;
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
      Status = CreateAsciiRelativePath (Option, DirectoryPath, DirectoryPathLength, InitrdMatch->FileName, InitrdMatch->StrLen);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    //
    // root=PARTUUID=... option.
    //
    Option = OcFlexArrayAddItem (Entry->Options);
    if (Option == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    
    Status = CreateRootPartuuid (Option);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Remaining options.
    //
    Status = AutodetectBootOptions (IsRescue, Entry->Options);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalAutodetectLinux (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                      Status;
  EFI_FILE_PROTOCOL               *VmlinuzDirectory;
  EFI_FILE_PROTOCOL               *RootFsFile;

  //
  // For now we are only searching in /boot.
  // vmlinuz files in / should not require autodetect, as
  // they should be accompanied by /loader/entries (Fedora-style),
  // and vmlinuz files in /boot not accompanied by /loader/entries
  // is Debian-style, so it seems sensible to wait to see what
  // else there is rather than speculatively adding directories.
  //
  Status = OcSafeFileOpen (RootDirectory, &VmlinuzDirectory, AUTODETECT_DIR, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mVmlinuzFiles = NULL;
  mInitrdFiles  = NULL;

  Status = OcSafeFileOpen (RootDirectory, &RootFsFile, ROOT_FS_FILE, EFI_FILE_MODE_READ, 0);
  if (!EFI_ERROR (Status)) {
    Status = OcEnsureDirectoryFile (RootFsFile, FALSE);
    RootFsFile->Close (RootFsFile);
  }
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LNX: Does not appear to be root filesystem - %r\n", Status));
  }

  if (!EFI_ERROR (Status)){
    mVmlinuzFiles = OcFlexArrayInit (sizeof (VMLINUZ_FILE), OcFlexArrayFreePointerItem);
    if (mVmlinuzFiles == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  if (!EFI_ERROR (Status)){
    mInitrdFiles = OcFlexArrayInit (sizeof (VMLINUZ_FILE), OcFlexArrayFreePointerItem);
    if (mInitrdFiles == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Place vmlinuz* and init* files into arrays.
  //
  if (!EFI_ERROR (Status)){
    Status = OcScanDirectory (VmlinuzDirectory, ProcessVmlinuzFile, NULL);
  }

  if (!EFI_ERROR (Status)) {
    gLoaderEntries = OcFlexArrayInit (sizeof (LOADER_ENTRY), (OC_FLEX_ARRAY_FREE_ITEM) InternalFreeLoaderEntry);
    if (gLoaderEntries == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      Status = LoadEtcFiles (RootDirectory);
      if (!EFI_ERROR (Status)) {
        Status = GenerateEntriesForVmlinuzFiles (AUTODETECT_DIR);
      }
      FreeEtcFiles();
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

  VmlinuzDirectory->Close (VmlinuzDirectory);
  return Status;
}

EFI_STATUS
AutodetectLinux (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                      Status;

  Status = InternalAutodetectLinux (RootDirectory, Entries, NumEntries);
  
  DEBUG ((
    (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) ? DEBUG_WARN : DEBUG_INFO,
    "LNX: AutodetectLinux - %r\n",
    Status));

  return Status;
}
