/** @file
  Boot Loader Spec / Grub2 blscfg module loader entry parser.

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
#include <Library/SortLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/OcBootEntry.h>

//
// Root.
//
#define ROOT_DIR    L"\\"
//
// Required where the BTRFS subvolume is /boot, as this looks like a
// normal directory within EFI. Note that scanning / and then /boot
// is how blscfg behaves by default too.
//
#define BOOT_DIR    L"\\boot"
//
// Don't add missing root= option unless this directory is present at root.
//
#define OSTREE_DIR  L"\\ostree"

//
// No leading slash so they can be relative to root or
// additional scan dir.
//
#define LOADER_ENTRIES_DIR    L"loader\\entries"
#define GRUB2_GRUB_CFG        L"grub2\\grub.cfg"
#define GRUB2_GRUBENV         L"grub2\\grubenv"
#define GRUB2_GRUBENV_SIZE    SIZE_1KB

#define BLSPEC_SUFFIX_CONF    L".conf"
#define BLSPEC_PREFIX_AUTO    L"auto-"

//
// Put a limit on entry name length, since the base part of the filename
// is used for the id, i.e. may be stored in BOOT#### entry in NVRAM.
// We then re-use the same filename length limit for vmlinuz and initrd files.
// Since we are using pure EFISTUB loading, initrd file paths have to be passed
// in kernel args (which have an unknown max. length of 256-4096 bytes).
//
#define MAX_LOADER_ENTRY_NAME_LEN           (127)
#define MAX_LOADER_ENTRY_FILE_INFO_SIZE     (                                         \
  SIZE_OF_EFI_FILE_INFO +                                                             \
  (MAX_LOADER_ENTRY_NAME_LEN + L_STR_LEN (BLSPEC_SUFFIX_CONF) + 1) * sizeof (CHAR16)  \
  )

//
// Might as well put an upper limit for some kind of sanity check.
// Typical files are ~350 bytes.
//
#define MAX_LOADER_ENTRY_FILE_SIZE        SIZE_4KB

STATIC
BOOLEAN
mIsGrub2;

/*
  loader entry processing states
*/
typedef enum ENTRY_PARSE_STATE_ {
  ENTRY_LEADING_SPACE,
  ENTRY_COMMENT,
  ENTRY_KEY,
  ENTRY_KEY_VALUE_SPACE,
  ENTRY_VALUE
} ENTRY_PARSE_STATE;

//
// First match, therefore Ubuntu should come after similar variants,
// and probably very short strings should come last in case they
// occur elsewhere in another kernel version string.
// NB: Should be kept in sync with Flavours.md.
//
STATIC
CHAR8 *
mLinuxVariants[] = {
  "Arch",
  "Astra",
  "CentOS",
  "Debian",
  "Deepin",
  "elementaryOS",
  "Endless",
  "Gentoo",
  "Fedora",
  "KDEneon",
  "Kali",
  "Mageia",
  "Manjaro",
  "Mint",
  "openSUSE",
  "Oracle",
  "PopOS",
  "RHEL",
  "Rocky",
  "Solus",
  "Lubuntu",
  "UbuntuMATE",
  "Xubuntu",
  "Ubuntu",
  "Void",
  "Zorin",
  "MX"
};

STATIC
CHAR8 *
ExtractVariantFrom (
  IN  CHAR8     *String
)
{
  UINTN       Index;
  CHAR8       *Variant;

  Variant = NULL;
  for (Index = 0; Index < ARRAY_SIZE (mLinuxVariants); Index++) {
    if (OcAsciiStriStr (String, mLinuxVariants[Index]) != NULL) {
      Variant = mLinuxVariants[Index];
      break;
    }
  }

  return Variant;
}

//
// Skips comment lines, and lines with key but no value
// EFI_LOAD_ERROR - File has invalid chars
//
STATIC
EFI_STATUS
GetLoaderEntryLine (
  IN     CONST CHAR16           *FileName,
  IN OUT       CHAR8            *Content,
  IN OUT       UINTN            *Pos,
     OUT       CHAR8            **Key,
     OUT       CHAR8            **Value
  )
{
  ENTRY_PARSE_STATE     State;
  CHAR8                 *LastSpace;
  CHAR8                 Ch;
  BOOLEAN               IsComplete;

  *Key        = NULL;
  *Value      = NULL;
  State       = ENTRY_LEADING_SPACE;
  IsComplete  = FALSE;

  do {
    Ch = Content[*Pos];
    
    if (!(Ch == '\0' || Ch == '\t' || Ch == '\n' || (Ch >= 20 && Ch < 128))) {
     DEBUG ((DEBUG_WARN, "LNX: Invalid char 0x%x in %s\n", Ch, FileName));
     return EFI_INVALID_PARAMETER;
    }

    switch (State) {
      case ENTRY_LEADING_SPACE:
        if (Ch == '\n' || Ch == '\0') {
          //
          // Skip empty line
          //
        } else if (Ch == ' ' || Ch == '\t') {
        } else if (Ch == '#') {
          State = ENTRY_COMMENT;
        } else {
          *Key = &Content[*Pos];
          State = ENTRY_KEY;
        }
        break;

      case ENTRY_COMMENT:
        if (Ch == '\n') {
          State = ENTRY_LEADING_SPACE;
        }
        break;

      case ENTRY_KEY:
        if (Ch == '\n' || Ch == '\0') {
          //
          // No value, skip line
          //
        } else if (Ch == ' ' || Ch == '\t') {
          Content[*Pos] = '\0';
          State = ENTRY_KEY_VALUE_SPACE;
        }
        break;

      case ENTRY_KEY_VALUE_SPACE:
        if (Ch == '\n' || Ch == '\0') {
          //
          // No value, skip line
          //
        } else if (Ch == ' ' || Ch == '\t') {
        } else {
          *Value = &Content[*Pos];
          State = ENTRY_VALUE;
          LastSpace = NULL;
        }
        break;

      case ENTRY_VALUE:
        if (Ch == '\n' || Ch == '\0') {
          if (LastSpace != NULL) {
            *LastSpace = '\0';
          } else {
            Content[*Pos] = '\0';
          }
          IsComplete = TRUE;
        } else if (Ch == ' ' || Ch == '\t' ) {
          LastSpace = &Content[*Pos];
        } else {
          LastSpace = NULL;
        }
        break;

      default:
        ASSERT (FALSE);
        return EFI_INVALID_PARAMETER;
    }
    if (Ch != '\0') {
      ++(*Pos);
    }
  }
  while (Ch != '\0' && !IsComplete);

  if (!IsComplete) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

LOADER_ENTRY *
InternalAllocateLoaderEntry (
  VOID
  )
{
  LOADER_ENTRY      *Entry;

  Entry = OcFlexArrayAddItem (gLoaderEntries);

  if (Entry != NULL) {
    Entry->Options = OcFlexArrayInit (sizeof (CHAR8 *), OcFlexArrayFreePointerItem);
    if (Entry->Options == NULL) {
      InternalFreeLoaderEntry (Entry);
      Entry = NULL;
    }
  }

  if (Entry != NULL) {
    Entry->Initrds = OcFlexArrayInit (sizeof (CHAR8 *), OcFlexArrayFreePointerItem);
    if (Entry->Initrds == NULL) {
      InternalFreeLoaderEntry (Entry);
      Entry = NULL;
    }
  }

  return Entry;
}

VOID
InternalFreeLoaderEntry (
  LOADER_ENTRY    *Entry
  )
{
  ASSERT (Entry != NULL);

  if (Entry == NULL) {
    return;
  }

  if (Entry->Title) {
    FreePool (Entry->Title);
  }
  if (Entry->Version) {
    FreePool (Entry->Version);
  }
  if (Entry->Linux) {
    FreePool (Entry->Linux);
  }
  OcFlexArrayFree (&Entry->Initrds);
  OcFlexArrayFree (&Entry->Options);
  if (Entry->OcId != NULL) {
    FreePool (Entry->OcId);
  }
  if (Entry->OcFlavour != NULL) {
    FreePool (Entry->OcFlavour);
  }
}

STATIC
EFI_STATUS
EntryCopySingleValue (
  IN     CONST BOOLEAN  Grub2,
  IN OUT       CHAR8    **Target,
  IN     CONST CHAR8    *Value
  )
{
  if (!Grub2 || *Target == NULL) {
    if (*Target != NULL) {
      FreePool (*Target);
    }
    *Target = AllocateCopyPool (AsciiStrSize (Value), Value);
    if (*Target == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EntryCopyMultipleValue (
  IN     CONST BOOLEAN              Grub2,
  IN           OC_FLEX_ARRAY        *Array,
  IN     CONST CHAR8                *Value
  )
{
  VOID          **NewItem;

  if (Grub2 && Array->Items != NULL) {
    return EFI_SUCCESS;
  }

  NewItem = OcFlexArrayAddItem (Array);
  if (NewItem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *NewItem = AllocateCopyPool (AsciiStrSize (Value), Value);
  if (*NewItem ==  NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
InternalProcessLoaderEntryFile (
  IN     CONST CHAR16           *FileName,
  IN OUT       CHAR8            *Content,
     OUT       LOADER_ENTRY     *Entry,
  IN     CONST BOOLEAN          Grub2
  )
{
  EFI_STATUS    Status;
  UINTN         Pos;
  CHAR8         *Key;
  CHAR8         *Value;

  ASSERT (Content != NULL);

  Status = EFI_SUCCESS;
  Pos = 0;

  //
  // Grub2 blscfg module uses first only (even for options,
  // which should allow more than one according to BL Spec).
  // systemd-boot uses multiple (concatenated with a space)
  // for options, and last only for everything which should
  // have only one.
  // Both use multiple for initrd.
  //
  while (TRUE) {
    Status = GetLoaderEntryLine(FileName, Content, &Pos, &Key, &Value);
    if (Status == EFI_NOT_FOUND) {
      break;
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (AsciiStrCmp (Key, "title") == 0) {
      Status = EntryCopySingleValue (Grub2, &Entry->Title, Value);
    } else if (AsciiStrCmp (Key, "version") == 0) {
      Status = EntryCopySingleValue (Grub2, &Entry->Version, Value);
    } else if (AsciiStrCmp (Key, "linux") == 0) {
      Status = EntryCopySingleValue (Grub2, &Entry->Linux, Value);
    } else if (AsciiStrCmp (Key, "options") == 0) {
      Status = EntryCopyMultipleValue (Grub2, Entry->Options, Value);
    } else if (AsciiStrCmp (Key, "initrd") == 0) {
      Status = EntryCopyMultipleValue (FALSE, Entry->Initrds, Value);
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ExpandReplaceOptions (
  IN OUT LOADER_ENTRY *Entry
  )
{
  EFI_STATUS          Status;
  CHAR8               **Options;
  CHAR8               *NewOptions;
  GRUB_VAR            *DefaultOptionsVar;

  if (Entry->Options->Count > 0) {
    //
    // Grub2 blscfg takes the first only.
    //
    ASSERT (Entry->Options->Count == 1);
    Status = InternalExpandGrubVarsForArray (Entry->Options);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    //
    // This is what grub2 blscfg does if there are no options.
    //
    DefaultOptionsVar = InternalGetGrubVar ("default_kernelopts");
    if (DefaultOptionsVar != NULL) {
      if (DefaultOptionsVar->Errors != 0) {
        DEBUG ((DEBUG_WARN, "LNX: Unusable grub var $%a - 0x%x\n", "default_kernelopts", DefaultOptionsVar->Errors));
        return EFI_INVALID_PARAMETER;
      }

      DEBUG ((DEBUG_INFO, "LNX: Using $%a\n", "default_kernelopts"));

      //
      // Blscfg expands $default_kernelopts, and we expand it even
      // if !HasVars since we need the string to be realloced.
      //
      Status = InternalExpandGrubVars (DefaultOptionsVar->Value, &NewOptions);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      Options = OcFlexArrayAddItem (Entry->Options);
      if (Options == NULL) {
        FreePool (NewOptions);
        return EFI_OUT_OF_RESOURCES;
      }
      *Options = NewOptions;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DoFilterLoaderEntry (
  EFI_FILE_HANDLE   Directory,
  EFI_FILE_INFO     *FileInfo,
  UINTN             FileInfoSize,
  VOID              *Context        OPTIONAL
  )
{
  if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) != 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Skip ".*" and case sensitive "auto-*" files,
  // follows systemd-boot logic.
  //
  if (FileInfo->FileName[0] == L'.' ||
    StrnCmp (FileInfo->FileName, BLSPEC_PREFIX_AUTO, L_STR_LEN (BLSPEC_PREFIX_AUTO)) == 0) {
    return EFI_NOT_FOUND;
  }

  if (!OcUnicodeEndsWith (FileInfo->FileName, BLSPEC_SUFFIX_CONF, TRUE)) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

//
// Works for vmlinuz-{version} and {machine-id}-{version}.conf.
// If blspec/blscfg style then we want .conf files with the same machine-id
// to count as alternate kernels of the same install; but there might be
// different installs, and the same kernel might be on different installs;
// so {machine-id} is good as the shared id (and it would not be sufficient
// to just use {version} when generating a non-shared id).
// If autodetect, we assume there is only one install in the directory,
// so {vmlinuz} is good as the shared id.
//
EFI_STATUS
InternalIdVersionFromFileName (
  IN OUT LOADER_ENTRY  *Entry,
  IN     CHAR16        *FileName
  )
{
  EFI_STATUS          Status;
  UINTN               NumCopied;

  CHAR16              *Split;
  CHAR16              *IdEnd;
  CHAR16              *VersionStart;
  CHAR16              *VersionEnd;
  CHAR8               *OcId;
  CHAR8               *Version;

  ASSERT (Entry    != NULL);
  ASSERT (FileName != NULL);
  
  Split = NULL;

  Split = OcStrChr (FileName, L'-');

  VersionEnd = &FileName[StrLen (FileName)];
  if (OcUnicodeEndsWith (FileName, BLSPEC_SUFFIX_CONF, TRUE)) {
    VersionEnd -= L_STR_LEN (BLSPEC_SUFFIX_CONF);
  }

  //
  // Will not happen with sane filenames.
  //
  if (Split != NULL && (Split == FileName || VersionEnd == Split + 1)) {
    Split = NULL;
  }

  if (Split == NULL) {
    VersionStart = FileName;
    IdEnd = VersionEnd;
  } else {
    VersionStart = Split + 1;

    if ((gLinuxBootFlags & LINUX_BOOT_USE_LATEST) != 0) {
      IdEnd = Split;
    } else {
      IdEnd = VersionEnd;
    }
  }

  OcId = AllocatePool (IdEnd - FileName + 1);
  if (OcId == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UnicodeStrnToAsciiStrS (FileName, IdEnd - FileName, OcId, IdEnd - FileName + 1, &NumCopied);
  ASSERT (!EFI_ERROR (Status));
  ASSERT (NumCopied == (UINTN) (IdEnd - FileName));

  Entry->OcId = OcId;

  if (Entry->Version == NULL) {
    Version = AllocatePool (VersionEnd - VersionStart + 1);
    if (Version == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = UnicodeStrnToAsciiStrS (VersionStart, VersionEnd - VersionStart, Version, VersionEnd - VersionStart + 1, &NumCopied);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (NumCopied == (UINTN) (VersionEnd - VersionStart));

    Entry->Version = Version;
  }

  return EFI_SUCCESS;
}

//
// We do not need to warn about missing files: OC warns about
// missing bootable file (vmlinuz) and kernel stops and warns
// about missing initrd. However, some linuxes (e.g. Endless)
// make the filepaths specified in an entry relative to /boot,
// not relative to / as it should be, so we have to check which
// it is. Only for this reason, we end up warning here if we
// can't find it at all.
//
STATIC
EFI_STATUS
FindLoaderFile (
  IN     CONST EFI_FILE_HANDLE  Directory,
  IN     CONST CHAR16           *DirName,
  IN OUT       CHAR8            **FileName
  )
{
  EFI_STATUS        Status;
  EFI_FILE_PROTOCOL *File;
  UINTN             FileNameLen;
  UINTN             DirNameLen;
  CHAR16            *Path;
  UINTN             MaxPathSize;

  ASSERT (DirName != NULL);
  ASSERT (FileName != NULL);
  ASSERT (*FileName != NULL);

  FileNameLen = AsciiStrLen (*FileName);
  DirNameLen = StrLen (DirName);

  MaxPathSize = (DirNameLen + FileNameLen + 1) * sizeof (CHAR16);

  Path = AllocatePool (MaxPathSize);
  if (Path == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeSPrintAsciiFormat (Path, MaxPathSize, "%a", *FileName);
  UnicodeUefiSlashes (Path);

  Status = OcSafeFileOpen (Directory, &File, Path, EFI_FILE_MODE_READ, 0);
  if (!EFI_ERROR (Status)) {
    File->Close (File);
    FreePool (Path);
    return Status;
  }

  if (DirNameLen <= 1) {
    DEBUG ((DEBUG_WARN, "LNX: %a not found - %r\n", *FileName, Status));
    FreePool (Path);
    return Status;
  }

  UnicodeSPrintAsciiFormat (Path, MaxPathSize, "%s%a", DirName, *FileName);
  UnicodeUefiSlashes (Path);

  Status = OcSafeFileOpen (Directory, &File, Path, EFI_FILE_MODE_READ, 0);
  if (!EFI_ERROR (Status)) {
    //
    // Found at 'wrong' location - re-use allocated path
    //
    File->Close (File);
    AsciiSPrint ((CHAR8 *)Path, MaxPathSize, "%s%a", DirName, *FileName);
    AsciiUnixSlashes ((CHAR8 *)Path);
    FreePool (*FileName);
    *FileName = (CHAR8 *)Path;
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_WARN, "LNX: %a not found (searched %s and %s) - %r\n", *FileName, ROOT_DIR, DirName, Status));
  FreePool (Path);
  return Status;
}

STATIC
BOOLEAN
HasRootOption (
  IN           OC_FLEX_ARRAY      *Options
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  CHAR8                 **Option;
  CHAR8                 *OptionCopy;
  OC_FLEX_ARRAY         *ParsedVars;
  BOOLEAN               HasRoot;

  ASSERT (Options != NULL);

  //
  // TRUE on errors: do not attempt to add root.
  //
  for (Index = 0; Index < Options->Count; Index++) {
    Option = OcFlexArrayItemAt (Options, Index);
    ASSERT (Option != NULL);
    ASSERT (*Option != NULL);

    OptionCopy = AllocateCopyPool (AsciiStrSize (*Option), *Option);
    if (OptionCopy == NULL) {
      return TRUE;
    }

    Status = OcParseVars (OptionCopy, &ParsedVars, FALSE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "LNX: Error parsing Options[%u]=<%a> - %r\n", Index, *Option, Status));
      FreePool (OptionCopy);
      return TRUE;
    }

    HasRoot = OcHasParsedVar (ParsedVars, "root", FALSE);

    FreePool (ParsedVars);
    FreePool (OptionCopy);

    if (HasRoot) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
BOOLEAN
HasOstreeDir (
  EFI_FILE_HANDLE   Directory
  )
{
  EFI_STATUS            Status;
  EFI_FILE_PROTOCOL     *OstreeFile;

  Status = OcSafeFileOpen (Directory, &OstreeFile, OSTREE_DIR, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = OcEnsureDirectoryFile (OstreeFile, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LNX: %s found but not a %a - %r\n", OSTREE_DIR, "directory", Status));
  }
  OstreeFile->Close (OstreeFile);

  return !EFI_ERROR(Status);
}

STATIC
EFI_STATUS
DoProcessLoaderEntry (
  EFI_FILE_HANDLE   Directory,
  EFI_FILE_INFO     *FileInfo,
  UINTN             FileInfoSize,
  VOID              *Context        OPTIONAL
  )
{
  EFI_STATUS            Status;
  CHAR8                 *Content;
  LOADER_ENTRY          *Entry;
  CHAR16                SaveChar;
  CHAR16                *DirName;
  CHAR8                 **Initrd;
  UINTN                 Index;

  ASSERT (Context != NULL);
  DirName = Context;

  if (FileInfoSize > MAX_LOADER_ENTRY_FILE_INFO_SIZE) {
    SaveChar = FileInfo->FileName[MAX_LOADER_ENTRY_NAME_LEN];
    FileInfo->FileName[MAX_LOADER_ENTRY_NAME_LEN] = CHAR_NULL;
    DEBUG ((DEBUG_WARN, "LNX: Entry filename overlong: %s...\n", FileInfo->FileName));
    FileInfo->FileName[MAX_LOADER_ENTRY_NAME_LEN] = SaveChar;
    return EFI_NOT_FOUND;
  }

  if (FileInfo->FileSize > MAX_LOADER_ENTRY_FILE_SIZE) {
    DEBUG ((DEBUG_WARN, "LNX: Entry file size overlong: %s\n", FileInfo->FileName));
    return EFI_NOT_FOUND;
  }

  DEBUG (((gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
    "LNX: Reading %s...\n", FileInfo->FileName));

  Content = OcReadFileFromDirectory (Directory, FileInfo->FileName, NULL, 0);
  if (Content == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry = InternalAllocateLoaderEntry ();
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InternalProcessLoaderEntryFile (FileInfo->FileName, Content, Entry, mIsGrub2);
  if (EFI_ERROR (Status)) {
    OcFlexArrayDiscardItem (gLoaderEntries, TRUE);
    return Status;
  }

  if (Entry->Linux == NULL) {
    DEBUG (((gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
      "LNX: No linux line, ignoring\n"));
    OcFlexArrayDiscardItem (gLoaderEntries, TRUE);
    return EFI_NOT_FOUND;
  }

  Status = FindLoaderFile (Directory, DirName, &Entry->Linux);
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < Entry->Initrds->Count; Index++) {
      Initrd = OcFlexArrayItemAt (Entry->Initrds, Index);
      Status = FindLoaderFile (Directory, DirName, Initrd);
      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }
  if (EFI_ERROR (Status)) {
    OcFlexArrayDiscardItem (gLoaderEntries, TRUE);
    return Status;
  }

  if (mIsGrub2) {
    Status = ExpandReplaceOptions (Entry);
    if (EFI_ERROR (Status)) {
      OcFlexArrayDiscardItem (gLoaderEntries, TRUE);
      return Status;
    }
  }

  //
  // Need to understand other reasons to apply this fix (if any),
  // so not automatically applying unless we recognise the layout.
  //
  if ((gLinuxBootFlags & LINUX_BOOT_ALLOW_CONF_AUTO_ROOT) != 0
    && !HasRootOption (Entry->Options)) {
    if (!HasOstreeDir (Directory)) {
      DEBUG ((DEBUG_WARN, "LNX: Missing root option, %s %afound - %afixing\n", OSTREE_DIR, "not ", "not "));
    } else {
      DEBUG ((DEBUG_INFO, "LNX: Missing root option, %s %afound - %afixing\n", OSTREE_DIR, "", ""));
      Status = InsertRootOption (Entry->Options);
      if (EFI_ERROR (Status)) {
        OcFlexArrayDiscardItem (gLoaderEntries, TRUE);
        return Status;
      }
    }
  }

  //
  // Id, and version if not already set within .conf data, from filename.
  //
  Status = InternalIdVersionFromFileName (Entry, FileInfo->FileName);
  if (EFI_ERROR (Status)) {
    OcFlexArrayDiscardItem (gLoaderEntries, TRUE);
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ProcessLoaderEntry (
  EFI_FILE_HANDLE   Directory,
  EFI_FILE_INFO     *FileInfo,
  UINTN             FileInfoSize,
  VOID              *Context        OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = DoFilterLoaderEntry (Directory, FileInfo, FileInfoSize, Context);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DoProcessLoaderEntry (Directory, FileInfo, FileInfoSize, Context);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "LNX: Error processing %s - %r\n", FileInfo->FileName, Status));

    //
    // Continue to use earlier or later .conf files which we can process -
    // more chance of a way to boot into Linux for end user.
    //
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ScanLoaderEntriesAtDirectory (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  IN   CHAR16                   *DirName,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                      Status;
  EFI_FILE_PROTOCOL               *EntriesDirectory;
  CHAR8                           *GrubCfg;
  CHAR8                           *GrubEnv;
  GRUB_VAR                        *EarlyInitrdVar;

  Status = OcSafeFileOpen (RootDirectory, &EntriesDirectory, LOADER_ENTRIES_DIR, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status              = EFI_SUCCESS;
  GrubEnv             = NULL;
  mIsGrub2            = FALSE;
  gLoaderEntries      = NULL;

  //
  // Only treat as GRUB2 if grub2/grub.cfg exists.
  //
  GrubCfg   = OcReadFileFromDirectory (RootDirectory, GRUB2_GRUB_CFG, NULL, 0);
  if (GrubCfg == NULL) {
    DEBUG ((DEBUG_INFO, "LNX: %s not found\n", GRUB2_GRUB_CFG));
  } else {
    mIsGrub2  = TRUE;
    Status = InternalInitGrubVars ();
    if (!EFI_ERROR (Status)) {
      //
      // Read grubenv first, since vars in grub.cfg should overwrite it.
      //
      GrubEnv = OcReadFileFromDirectory (RootDirectory, GRUB2_GRUBENV, NULL, GRUB2_GRUBENV_SIZE);
      if (GrubEnv == NULL) {
        DEBUG ((DEBUG_WARN, "LNX: %s not found\n", GRUB2_GRUBENV));
      } else {
        DEBUG (((gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
          "LNX: Reading %s\n", GRUB2_GRUBENV));
        Status = InternalProcessGrubEnv (GrubEnv, GRUB2_GRUBENV_SIZE);
      }
      if (!EFI_ERROR (Status)) {
        DEBUG (((gLinuxBootFlags & LINUX_BOOT_LOG_VERBOSE) == 0 ? DEBUG_VERBOSE : DEBUG_INFO,
          "LNX: Reading %s\n", GRUB2_GRUB_CFG));
        Status = InternalProcessGrubCfg (GrubCfg);
      }
    }
  }

  //
  // If we are grub2 and $early_initrd exists, then warn and halt (blscfg logic is to use it).
  // Would not be hard to implement if required. This is a space separated list of filenames
  // to use first as initrds.
  // Note: they are filenames only, so (following how blscfg module does it) we need to prepend
  // the path of either another (the first) initrd or the (first) vmlinuz.
  //
  if (!EFI_ERROR (Status)) {
    if (mIsGrub2) {
      EarlyInitrdVar = InternalGetGrubVar ("early_initrd");
      if (EarlyInitrdVar != NULL &&
        EarlyInitrdVar->Value != NULL &&
        EarlyInitrdVar->Value[0] != '\0'
        ) {
        DEBUG ((DEBUG_INFO, "LNX: grub var $%a is present but currently unsupported - aborting\n", "early_initrd"));
        Status = EFI_INVALID_PARAMETER;
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    gLoaderEntries = OcFlexArrayInit (sizeof (LOADER_ENTRY), (OC_FLEX_ARRAY_FREE_ITEM) InternalFreeLoaderEntry);
    if (gLoaderEntries == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      Status = OcScanDirectory (EntriesDirectory, ProcessLoaderEntry, DirName);
    }

    if (!EFI_ERROR (Status)) {
      ASSERT (gLoaderEntries->Count > 0);

      Status = InternalConvertLoaderEntriesToBootEntries (
        RootDirectory,
        Entries,
        NumEntries
      );
    }

    OcFlexArrayFree (&gLoaderEntries);
  }

  InternalFreeGrubVars ();

  if (GrubEnv != NULL) {
    FreePool (GrubEnv);
  }
  if (GrubCfg != NULL) {
    FreePool (GrubCfg);
  }

  EntriesDirectory->Close (EntriesDirectory);

  return Status;
}

STATIC
EFI_STATUS
DoScanLoaderEntries (
  IN   EFI_FILE_PROTOCOL        *Directory,
  IN   CHAR16                   *DirName,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                      Status;

  Status = ScanLoaderEntriesAtDirectory (Directory, DirName, Entries, NumEntries);

  DEBUG ((
    (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) ? DEBUG_WARN : DEBUG_INFO,
    "LNX: ScanLoaderEntries %s - %r\n",
    DirName,
    Status
    ));
    
  return Status;
}

EFI_STATUS
InternalScanLoaderEntries (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                    Status;
  EFI_FILE_PROTOCOL             *AdditionalScanDirectory;

  Status = DoScanLoaderEntries (RootDirectory, ROOT_DIR, Entries, NumEntries);
  if (EFI_ERROR (Status)) {
    Status = OcSafeFileOpen (RootDirectory, &AdditionalScanDirectory, BOOT_DIR, EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR (Status)) {
      Status = DoScanLoaderEntries (AdditionalScanDirectory, BOOT_DIR, Entries, NumEntries);
      AdditionalScanDirectory->Close (AdditionalScanDirectory);
    }
  }
  
  return Status;
}

STATIC
EFI_STATUS
DoConvertLoaderEntriesToBootEntries (
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     OptionsIndex;
  LOADER_ENTRY              *Entry;
  OC_FLEX_ARRAY             *PickerEntries;
  OC_PICKER_ENTRY           *PickerEntry;
  OC_STRING_BUFFER          *StringBuffer;
  CHAR8                     **Options;
  UINTN                     OptionsLength;

  StringBuffer = NULL;

  PickerEntries = OcFlexArrayInit (sizeof (OC_PICKER_ENTRY), (OC_FLEX_ARRAY_FREE_ITEM) InternalFreePickerEntry);
  if (PickerEntries == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_SUCCESS;
  for (Index = 0; Index < gLoaderEntries->Count; Index++) {
    Entry = OcFlexArrayItemAt (gLoaderEntries, Index);

    PickerEntry = OcFlexArrayAddItem (PickerEntries);
    if (PickerEntry == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    PickerEntry->Path = AllocateCopyPool (AsciiStrSize (Entry->Linux), Entry->Linux);
    if (PickerEntry->Path == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    //
    // Arguments.
    //
    StringBuffer = OcAsciiStringBufferInit ();
    if (StringBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    for (OptionsIndex = 0; OptionsIndex < Entry->Initrds->Count; OptionsIndex++) {
      Options = OcFlexArrayItemAt (Entry->Initrds, OptionsIndex);
      Status = OcAsciiStringBufferAppend (StringBuffer, "initrd=");
      if (EFI_ERROR (Status)) {
        break;
      }
      Status = OcAsciiStringBufferAppend (StringBuffer, *Options);
      if (EFI_ERROR (Status)) {
        break;
      }
      Status = OcAsciiStringBufferAppend (StringBuffer, " ");
      if (EFI_ERROR (Status)) {
        break;
      }
    }

    if (EFI_ERROR (Status)) {
      break;
    }

    for (OptionsIndex = 0; OptionsIndex < Entry->Options->Count; OptionsIndex++) {
      Options = OcFlexArrayItemAt (Entry->Options, OptionsIndex);
      OptionsLength = AsciiStrLen (*Options);
      ASSERT (OptionsLength != 0);
      if (OptionsLength == 0) {
        continue;
      }
      if ((*Options)[OptionsLength - 1] == ' ') {
        --OptionsLength;
      }
      Status = OcAsciiStringBufferAppendN (StringBuffer, *Options, OptionsLength);
      if (EFI_ERROR (Status)) {
        break;
      }
      if (OptionsIndex < Entry->Options->Count - 1) {
        Status = OcAsciiStringBufferAppend (StringBuffer, " ");
        if (EFI_ERROR (Status)) {
          break;
        }
      }
    }

    if (EFI_ERROR (Status)) {
      break;
    }

    PickerEntry->Arguments = OcAsciiStringBufferFreeContainer (&StringBuffer);

    //
    // Name.
    //
    StringBuffer = OcAsciiStringBufferInit ();
    if (StringBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }
    if ((gLinuxBootFlags & LINUX_BOOT_ADD_DEBUG_INFO) != 0) {
      //
      // Show file system type and enough of PARTUUID to help distinguish one partition from another.
      //
      Status = OcAsciiStringBufferSPrint (StringBuffer, "%a-%08x", gFileSystemType, gPartuuid.Data1);
      if (EFI_ERROR (Status)) {
        break;
      }

      OcAsciiToLower (&StringBuffer->String[StringBuffer->StringLength] - sizeof (gPartuuid.Data1) * 2);
      
      Status = OcAsciiStringBufferAppend (StringBuffer, ": ");
      if (EFI_ERROR (Status)) {
        break;
      }
    }
    Status = OcAsciiStringBufferAppend (StringBuffer, Entry->Title);
    if (EFI_ERROR (Status)) {
      break;
    }
    PickerEntry->Name = OcAsciiStringBufferFreeContainer (&StringBuffer);

    ASSERT (Entry->OcFlavour != NULL);
    PickerEntry->Flavour = AllocateCopyPool (AsciiStrSize (Entry->OcFlavour), Entry->OcFlavour);
    if (PickerEntry->Flavour == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    ASSERT (Entry->OcId != NULL);
    PickerEntry->Id = AllocateCopyPool (AsciiStrSize (Entry->OcId), Entry->OcId);
    if (PickerEntry->Id == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    PickerEntry->Auxiliary = Entry->OcAuxiliary;

    PickerEntry->RealPath  = TRUE;
    PickerEntry->TextMode  = FALSE;
    PickerEntry->Tool      = FALSE;
  }

  if (EFI_ERROR (Status)) {
    if (StringBuffer != NULL) {
      OcAsciiStringBufferFree (&StringBuffer);
    }
    OcFlexArrayFree (&PickerEntries);
  } else {
    ASSERT (StringBuffer == NULL);
    OcFlexArrayFreeContainer (&PickerEntries, (VOID **) Entries, NumEntries);
  }

  return Status;
}

STATIC
EFI_STATUS
AppendVersion (
  LOADER_ENTRY      *Entry
  )
{
  EFI_STATUS        Status;
  OC_STRING_BUFFER  *StringBuffer;

  ASSERT (Entry->Title != NULL);

  if (Entry->Version == NULL || AsciiStrStr (Entry->Title, Entry->Version) != NULL) {
    return EFI_SUCCESS;
  }

  StringBuffer = OcAsciiStringBufferInit ();

  Status = OcAsciiStringBufferSPrint (StringBuffer, "%a (%a)", Entry->Title, Entry->Version);

  if (!EFI_ERROR (Status)) {
    FreePool (Entry->Title);
    Entry->Title = OcAsciiStringBufferFreeContainer (&StringBuffer);
  }

  return Status;
}

STATIC
EFI_STATUS
AppendVersions (
  VOID
  )
{
  EFI_STATUS          Status;
  UINTN               Index;
  LOADER_ENTRY        *Entry;

  if (gPickerContext->HideAuxiliary && (gLinuxBootFlags & LINUX_BOOT_USE_LATEST) != 0) {
    return EFI_SUCCESS;
  }

  for (Index = 0; Index < gLoaderEntries->Count; Index++) {
    Entry = OcFlexArrayItemAt (gLoaderEntries, Index);
    
    Status = AppendVersion (Entry);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DisambiguateDuplicates (
  VOID
  )
{
  UINTN               Index;
  UINTN               MatchIndex;
  LOADER_ENTRY        *Entry;
  LOADER_ENTRY        *MatchEntry;

  //
  // This check should not be strictly necessary, since there shouldn't
  // (normally) be any duplicate ids if this flag is not set.
  //
  if ((gLinuxBootFlags & LINUX_BOOT_USE_LATEST) == 0) {
    return EFI_SUCCESS;
  }

  for (Index = 0; Index < gLoaderEntries->Count - 1; Index++) {
    Entry = OcFlexArrayItemAt (gLoaderEntries, Index);

    if (Entry->DuplicateIdScanned) {
      continue;
    }

    for (MatchIndex = Index + 1; MatchIndex < gLoaderEntries->Count; MatchIndex++) {
      MatchEntry = OcFlexArrayItemAt (gLoaderEntries, MatchIndex);

      if (!MatchEntry->DuplicateIdScanned) {
        if (AsciiStrCmp (Entry->OcId, MatchEntry->OcId) == 0) {
          //
          // Everything but the first id duplicate becomes auxiliary.
          //
          MatchEntry->OcAuxiliary = TRUE;
          MatchEntry->DuplicateIdScanned = TRUE;
        }
      }
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EntryApplyDefaults (
  IN  EFI_FILE_PROTOCOL   *RootDirectory,
  IN  LOADER_ENTRY        *Entry
  )
{
  CHAR8               *Variant;
  UINTN               Length;
  UINTN               NumPrinted;

  Variant = NULL;

  if (Entry->Title != NULL) {
    Variant = ExtractVariantFrom (Entry->Title);
  }

  ASSERT (Entry->OcFlavour == NULL);

  if (Variant != NULL) {
    Length = AsciiStrLen (Variant) + L_STR_LEN ("Linux") + 1;

    Entry->OcFlavour = AllocatePool (Length + 1);
    if (Entry->OcFlavour == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    NumPrinted = AsciiSPrint (Entry->OcFlavour, Length + 1, "%a:%a", Variant, "Linux");
    ASSERT (NumPrinted == Length);
  } else {
    Variant = "Linux";
    Entry->OcFlavour = AllocateCopyPool (AsciiStrSize (Variant), Variant);
    if (Entry->OcFlavour == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  if (Entry->Title == NULL) {
    Entry->Title = AllocateCopyPool (AsciiStrSize (Variant), Variant);
    if (Entry->Title == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ApplyDefaults (
  IN   EFI_FILE_PROTOCOL        *RootDirectory
  )
{
  EFI_STATUS          Status;
  UINTN               Index;
  LOADER_ENTRY        *Entry;

  Status = EFI_SUCCESS;

  for (Index = 0; Index < gLoaderEntries->Count; Index++) {
    Entry = OcFlexArrayItemAt (gLoaderEntries, Index);

    Status = EntryApplyDefaults (RootDirectory, Entry);
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  return Status;
}

//
// Also use for loader entries generated by autodetect.
//
EFI_STATUS
InternalConvertLoaderEntriesToBootEntries (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS      Status;

  Status = EFI_SUCCESS;

  //
  // Sort entries by version descending.
  //
  PerformQuickSort (gLoaderEntries->Items, gLoaderEntries->Count, gLoaderEntries->ItemSize, InternalReverseVersionCompare);

  //
  // Autodetect good title and flavour, if needed.
  //
  Status = ApplyDefaults (RootDirectory);

  //
  // Always append version to titles when !HideAuxiliary.
  // We previously implemented a pure-BLSpec process of disambiguating by appending version
  // only if the title is ambiguous with others on the same partition, but it is more natural
  // to do it always.
  //
  if (!EFI_ERROR (Status)) {
    Status = AppendVersions ();
  }

  //
  // Make duplicates by id after the first into auxiliary entries.
  //
  if (!EFI_ERROR (Status)) {
    Status = DisambiguateDuplicates ();
  }

  if (!EFI_ERROR (Status)) {
    Status = DoConvertLoaderEntriesToBootEntries (
      Entries,
      NumEntries
    );
  }

  return Status;
}
