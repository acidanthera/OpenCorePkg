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
// Required where the BTRFS subvolume is /boot, as this looks like a
// normal directory within EFI. Note that scanning / and then /boot
// is how blscfg behaves by default too.
//
#define ADDITIONAL_SCAN_DIR   L"boot"

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

  Entry = AllocateZeroPool (sizeof (LOADER_ENTRY));

  if (Entry != NULL) {
    Entry->Options = OcFlexArrayInit (sizeof (CHAR8 *), OcFlexArrayFreePointerItem);
    if (Entry->Options == NULL) {
      InternalFreeLoaderEntry (&Entry);
    }
  }

  if (Entry != NULL) {
    Entry->Initrds = OcFlexArrayInit (sizeof (CHAR8 *), OcFlexArrayFreePointerItem);
    if (Entry->Initrds == NULL) {
      InternalFreeLoaderEntry (&Entry);
    }
  }

  return Entry;
}

VOID
InternalFreeLoaderEntry (
  LOADER_ENTRY    **Entry
  )
{
  ASSERT (Entry != NULL);
  ASSERT (*Entry != NULL);

  if (Entry != NULL && *Entry != NULL) {
    if ((*Entry)->Title) {
      FreePool ((*Entry)->Title);
    }
    if ((*Entry)->Version) {
      FreePool ((*Entry)->Version);
    }
    if ((*Entry)->Linux) {
      FreePool ((*Entry)->Linux);
    }
    OcFlexArrayFree (&(*Entry)->Initrds);
    OcFlexArrayFree (&(*Entry)->Options);
    if ((*Entry)->OcId != NULL) {
      FreePool ((*Entry)->OcId);
    }
    if ((*Entry)->OcFlavour != NULL) {
      FreePool ((*Entry)->OcFlavour);
    }

    FreePool (*Entry);

    *Entry = NULL;
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
     OUT       LOADER_ENTRY     **Entry,
  IN     CONST BOOLEAN          Grub2
  )
{
  EFI_STATUS    Status;
  UINTN         Pos;
  CHAR8         *Key;
  CHAR8         *Value;

  ASSERT (Content != NULL);

  *Entry = InternalAllocateLoaderEntry ();
  if (*Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

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
      InternalFreeLoaderEntry (Entry);
      return Status;
    }

    if (AsciiStrCmp (Key, "title") == 0) {
      Status = EntryCopySingleValue (Grub2, &(*Entry)->Title, Value);
    } else if (AsciiStrCmp (Key, "version") == 0) {
      Status = EntryCopySingleValue (Grub2, &(*Entry)->Version, Value);
    } else if (AsciiStrCmp (Key, "linux") == 0) {
      Status = EntryCopySingleValue (Grub2, &(*Entry)->Linux, Value);
    } else if (AsciiStrCmp (Key, "options") == 0) {
      Status = EntryCopyMultipleValue (Grub2, (*Entry)->Options, Value);
    } else if (AsciiStrCmp (Key, "initrd") == 0) {
      Status = EntryCopyMultipleValue (FALSE, (*Entry)->Initrds, Value);
    }

    if (EFI_ERROR (Status)) {
      InternalFreeLoaderEntry (Entry);
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
STATIC
CHAR8 *
OcIdFromFileName (
  CHAR16      *FileName
  )
{
  EFI_STATUS          Status;
  UINTN               NumCopied;

  CHAR16              *IdEnd;
  CHAR8               *OcId;

  ASSERT (FileName != NULL);
  
  IdEnd   = NULL;

  //
  // Shared id; intended to pick up machine-id from standard .conf file
  // naming, or just the word vmlinuz from standard kernel file naming.
  //
  if ((gLinuxBootFlags & LINUX_BOOT_USE_LATEST) != 0) {
    IdEnd = OcStrChr (FileName, L'-');
  }

  //
  // Non-shared id, or no '-' found in filename above.
  //
  if (IdEnd == NULL) {
    IdEnd = &FileName[StrLen (FileName)];
    if (OcUnicodeEndsWith (FileName, L".conf", TRUE)) {
      IdEnd -= L_STR_LEN (L".conf");
    }
  }

  OcId = AllocatePool (IdEnd - FileName + 1);

  if (OcId != NULL) {
    Status = UnicodeStrnToAsciiStrS (FileName, IdEnd - FileName, OcId, IdEnd - FileName + 1, &NumCopied);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (NumCopied == (UINTN)(IdEnd - FileName));
  }

  return OcId;
}

NAMED_LOADER_ENTRY *
InternalCreateNamedLoaderEntry (
  IN  LOADER_ENTRY        *Entry,
  IN  CHAR16              *FileName
  )
{
  NAMED_LOADER_ENTRY      *NamedEntry;
  CHAR8                   *OcId;

  OcId = OcIdFromFileName (FileName);
  if (OcId == NULL) {
    return NULL;
  }

  NamedEntry = OcFlexArrayAddItem (gNamedLoaderEntries);
  if (NamedEntry == NULL) {
    FreePool (OcId);
    return NULL;
  }

  Entry->OcId = OcId;

  NamedEntry->Entry = Entry;
  NamedEntry->FileName = FileName;

  return NamedEntry;
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
  NAMED_LOADER_ENTRY    *NamedEntry;
  CHAR16                *FileName;

  //
  // Doesn't really matter if we return EFI_SUCCESS or EFI_NOT_FOUND in these initial checks;
  // anything else will terminate directory processing.
  //
  if (FileInfoSize > MAX_LOADER_ENTRY_FILE_INFO_SIZE) {
    DEBUG ((DEBUG_WARN, "LNX: Entry %a %s overlong...\n", "filename", FileInfo->FileName));
    return EFI_SUCCESS;
  }

  if (FileInfo->FileSize > MAX_LOADER_ENTRY_FILE_SIZE) {
    DEBUG ((DEBUG_WARN, "LNX: Entry %a %s overlong...\n", "file size", FileInfo->FileName));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "LNX: Parsing %s...\n", FileInfo->FileName));

  Content = OcReadFileFromDirectory (Directory, FileInfo->FileName, NULL, 0);
  if (Content == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InternalProcessLoaderEntryFile (FileInfo->FileName, Content, &Entry, mIsGrub2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Entry->Linux == NULL) {
    InternalFreeLoaderEntry (&Entry);
    DEBUG ((DEBUG_INFO, "LNX: No linux line, ignoring\n"));
  } else {
    if (mIsGrub2) {
      Status = ExpandReplaceOptions (Entry);
      if (EFI_ERROR (Status)) {
        InternalFreeLoaderEntry (&Entry);
        return Status;
      }
    }

    FileName =  AllocateCopyPool (StrSize (FileInfo->FileName), FileInfo->FileName);
    if (FileName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    NamedEntry = InternalCreateNamedLoaderEntry (Entry, FileName);
    if (NamedEntry == NULL) {
      FreePool (FileName);
      InternalFreeLoaderEntry (&Entry);
      return EFI_OUT_OF_RESOURCES;
    }
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
InternalScanLoaderEntries (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
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
  gNamedLoaderEntries = NULL;

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
        DEBUG ((DEBUG_INFO, "LNX: Reading %s\n", GRUB2_GRUBENV));
        Status = InternalProcessGrubEnv (GrubEnv, GRUB2_GRUBENV_SIZE);
      }
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "LNX: Reading %s\n", GRUB2_GRUB_CFG));
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
    gNamedLoaderEntries = OcFlexArrayInit (sizeof (NAMED_LOADER_ENTRY), (OC_FLEX_ARRAY_FREE_ITEM) InternalFreeNamedLoaderEntry);
    if (gNamedLoaderEntries == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      Status = OcScanDirectory (EntriesDirectory, ProcessLoaderEntry, NULL);
    }

    if (!EFI_ERROR (Status)) {
      Status = InternalConvertNamedLoaderEntriesToBootEntries (
        RootDirectory,
        Entries,
        NumEntries
      );
    }

    OcFlexArrayFree (&gNamedLoaderEntries);
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

EFI_STATUS
ScanLoaderEntries (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      TempStatus;
  EFI_FILE_PROTOCOL               *AdditionalScanDirectory;

  Status = InternalScanLoaderEntries (RootDirectory, Entries, NumEntries);
  if (!EFI_ERROR (Status)) {
    return Status;
  }
  if (Status != EFI_NOT_FOUND) {
    DEBUG ((DEBUG_WARN, "LNX: ScanLoaderEntries @root - %r\n", Status));
  }

  TempStatus = OcSafeFileOpen (RootDirectory, &AdditionalScanDirectory, ADDITIONAL_SCAN_DIR, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (TempStatus)) {
    return Status;
  }

  Status = InternalScanLoaderEntries (AdditionalScanDirectory, Entries, NumEntries);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "LNX: ScanLoaderEntries @%s - %r\n", ADDITIONAL_SCAN_DIR, Status));
    return Status;
  }
  if (Status != EFI_NOT_FOUND) {
    DEBUG ((DEBUG_WARN, "LNX: ScanLoaderEntries @%s - %r\n", ADDITIONAL_SCAN_DIR, Status));
  }
  return Status;
}

VOID
InternalFreeNamedLoaderEntry (
  NAMED_LOADER_ENTRY    *Entry
  )
{
  ASSERT (Entry != NULL);
  ASSERT (Entry->Entry != NULL);

  if (Entry != NULL) {
    if (Entry->FileName != NULL) {
      FreePool (Entry->FileName);
      Entry->FileName = NULL;
    }
    
    if (Entry->Entry != NULL) {
      InternalFreeLoaderEntry (&(Entry->Entry));
    }
  }
}

STATIC
EFI_STATUS
DoConvertNamedLoaderEntriesToBootEntries (
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  UINTN                     OptionsIndex;
  NAMED_LOADER_ENTRY        *NamedEntry;
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
  for (Index = 0; Index < gNamedLoaderEntries->Count; Index++) {
    NamedEntry = OcFlexArrayItemAt (gNamedLoaderEntries, Index);
    Entry = NamedEntry->Entry;

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

    PickerEntry->RealPath = TRUE;
    PickerEntry->TextMode = FALSE;
    PickerEntry->Tool = FALSE;
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
MakeUnique (
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
MakeAllUnique (
  VOID
  )
{
  EFI_STATUS          Status;
  UINTN               Index;
  NAMED_LOADER_ENTRY  *NamedEntry;

  if (gPickerContext->HideAuxiliary) {
    return EFI_SUCCESS;
  }

  for (Index = 0; Index < gNamedLoaderEntries->Count; Index++) {
    NamedEntry = OcFlexArrayItemAt (gNamedLoaderEntries, Index);
    
    Status = MakeUnique (NamedEntry->Entry);
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
  NAMED_LOADER_ENTRY  *NamedEntry;
  NAMED_LOADER_ENTRY  *MatchEntry;

  //
  // This check should not be strictly necessary, since there shouldn't
  // (normally) be any duplicate ids if this flag is not set.
  //
  if ((gLinuxBootFlags & LINUX_BOOT_USE_LATEST) == 0) {
    return EFI_SUCCESS;
  }

  for (Index = 0; Index < gNamedLoaderEntries->Count - 1; Index++) {
    NamedEntry = OcFlexArrayItemAt (gNamedLoaderEntries, Index);

    if ((NamedEntry->DuplicateFlags & DUPLICATE_ID_SCANNED) != 0) {
      continue;
    }

    for (MatchIndex = Index + 1; MatchIndex < gNamedLoaderEntries->Count; MatchIndex++) {
      MatchEntry = OcFlexArrayItemAt (gNamedLoaderEntries, MatchIndex);

      if ((MatchEntry->DuplicateFlags & DUPLICATE_ID_SCANNED) == 0) {
        if (AsciiStrCmp (NamedEntry->Entry->OcId, MatchEntry->Entry->OcId) == 0) {
          //
          // Everything but the first id duplicate becomes auxiliary.
          //
          MatchEntry->Entry->OcAuxiliary = TRUE;
          MatchEntry->DuplicateFlags |= DUPLICATE_ID_SCANNED;
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
  NAMED_LOADER_ENTRY  *NamedEntry;

  Status = EFI_SUCCESS;

  for (Index = 0; Index < gNamedLoaderEntries->Count; Index++) {
    NamedEntry = OcFlexArrayItemAt (gNamedLoaderEntries, Index);

    Status = EntryApplyDefaults (RootDirectory, NamedEntry->Entry);
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
InternalConvertNamedLoaderEntriesToBootEntries (
  IN   EFI_FILE_PROTOCOL        *RootDirectory,
  OUT  OC_PICKER_ENTRY          **Entries,
  OUT  UINTN                    *NumEntries
  )
{
  EFI_STATUS      Status;

  Status = EFI_SUCCESS;

  //
  // Sort entries by filename descending.
  //
  PerformQuickSort (gNamedLoaderEntries->Items, gNamedLoaderEntries->Count, gNamedLoaderEntries->ItemSize, OcReverseStringCompare);

  //
  // Fill out any missing data, including from kernel file version string if needed.
  //
  Status = ApplyDefaults (RootDirectory);

  //
  // Append version to titles when !HideAuxiliary.
  // We previously implemented a pure-BLSpec process of disambiguating only if the
  // title is ambiguous with others on the same partition, but this works better.
  //
  if (!EFI_ERROR (Status)) {
    Status = MakeAllUnique ();
  }

  //
  // Make duplicates by id after the first into auxiliary entries.
  //
  if (!EFI_ERROR (Status)) {
    Status = DisambiguateDuplicates ();
  }

  if (!EFI_ERROR (Status)) {
    Status = DoConvertNamedLoaderEntriesToBootEntries (
      Entries,
      NumEntries
    );
  }

  return Status;
}
