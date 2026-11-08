//
//  RealtekConfig.c
//  OpenCorePkg
//
//  Created by laobamac on 2026/11/08.
//  Copyright (c) 2026 laobamac. All rights reserved.
//  SPDX-License-Identifier: BSD-3-Clause
//

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcConfigurationLib.h>
#include <Library/OcStringLib.h>

STATIC
BOOLEAN
InternalRealtekEntryActive (
  IN CONST CHAR8  *Arch,
  IN CONST CHAR8  *MinKernel,
  IN CONST CHAR8  *MaxKernel,
  IN UINT32       KernelVersion
  )
{
  UINT32  MinVersion;
  UINT32  MaxVersion;

  if (AsciiStrCmp (Arch, "i386") == 0) {
    return FALSE;
  }

  MinVersion = OcParseDarwinVersion (MinKernel);
  MaxVersion = OcParseDarwinVersion (MaxKernel);
  if (KernelVersion != 0) {
    return OcMatchDarwinVersion (KernelVersion, MinVersion, MaxVersion);
  }

  return (MinVersion < KERNEL_VERSION (KERNEL_VERSION_TAHOE + 1, 0, 0))
         && ((MaxVersion == 0) || (MaxVersion >= KERNEL_VERSION_TAHOE_MIN))
         && ((MaxVersion == 0) || (MinVersion <= MaxVersion));
}

STATIC
BOOLEAN
InternalRealtekIdentifier (
  IN CONST CHAR8  *Identifier
  )
{
  return (AsciiStrCmp (Identifier, "com.apple.iokit.IOSkywalkFamily") == 0)
         || (AsciiStrCmp (Identifier, "com.apple.driver.AppleEthernetRL") == 0);
}

STATIC
BOOLEAN
InternalRealtekPath (
  IN CONST CHAR8  *Path
  )
{
  CONST CHAR8         *End;
  UINTN               Index;
  UINTN               Length;
  STATIC CONST CHAR8  *Names[] = {
    "IOSkywalkFamily.kext", "IOSkywalkFamily",
    "AppleEthernetRL.kext", "AppleEthernetRL"
  };

  while (*Path != '\0') {
    End = Path;
    while ((*End != '\0') && (*End != '/') && (*End != '\\')) {
      ++End;
    }

    Length = (UINTN)(End - Path);
    for (Index = 0; Index < ARRAY_SIZE (Names); ++Index) {
      if ((Length == AsciiStrLen (Names[Index])) && (OcAsciiStrniCmp (Path, Names[Index], Length) == 0)) {
        return TRUE;
      }
    }

    Path = *End == '\0' ? End : End + 1;
  }

  return FALSE;
}

STATIC
BOOLEAN
InternalRealtekInjectedInfo (
  IN OC_KERNEL_ADD_ENTRY  *Entry
  )
{
  CHAR8         *Buffer;
  XML_DOCUMENT  *Document;
  XML_NODE      *Root;
  XML_NODE      *Value;
  CONST CHAR8   *Key;
  CONST CHAR8   *Identifier;
  UINT32        Index;
  BOOLEAN       Conflict;

  if ((Entry->PlistData == NULL) || (Entry->PlistDataSize == 0)) {
    return FALSE;
  }

  Buffer = AllocateCopyPool (Entry->PlistDataSize, Entry->PlistData);
  if (Buffer == NULL) {
    return TRUE;
  }

  Conflict = FALSE;
  Document = XmlDocumentParse (Buffer, Entry->PlistDataSize, FALSE);
  if (Document != NULL) {
    Root = PlistNodeCast (PlistDocumentRoot (Document), PLIST_NODE_TYPE_DICT);
    if (Root != NULL) {
      for (Index = 0; Index < PlistDictChildren (Root); ++Index) {
        Key = PlistKeyValue (PlistDictChild (Root, Index, &Value));
        if ((Key != NULL) && (AsciiStrCmp (Key, INFO_BUNDLE_IDENTIFIER_KEY) == 0)) {
          Value      = PlistNodeCast (Value, PLIST_NODE_TYPE_STRING);
          Identifier = Value != NULL ? XmlNodeContent (Value) : NULL;
          Conflict   = (Identifier != NULL) && InternalRealtekIdentifier (Identifier);
          break;
        }
      }
    }

    XmlDocumentFree (Document);
  }

  FreePool (Buffer);
  return Conflict;
}

UINT32
OcKernelCheckRealtekEthernetConfig (
  IN OC_GLOBAL_CONFIG  *Config,
  IN UINT32            KernelVersion
  )
{
  UINT32                 Errors;
  UINT32                 Index;
  UINT32                 Kind;
  UINT32                 Count;
  OC_KERNEL_BLOCK_ENTRY  *Block;
  OC_KERNEL_PATCH_ENTRY  *Patch;
  OC_KERNEL_ADD_ENTRY    *Entry;

  if (  !Config->Kernel.Quirks.ForceRealtekEthernet
     || (  (KernelVersion != 0)
        && !OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_TAHOE_MIN, KERNEL_VERSION (KERNEL_VERSION_TAHOE + 1, 0, 0) - 1)))
  {
    return 0;
  }

  Errors = 0;
  for (Index = 0; Index < Config->Kernel.Block.Count; ++Index) {
    Block = Config->Kernel.Block.Values[Index];
    if (  Block->Enabled
       && InternalRealtekIdentifier (OC_BLOB_GET (&Block->Identifier))
       && InternalRealtekEntryActive (
            OC_BLOB_GET (&Block->Arch),
            OC_BLOB_GET (&Block->MinKernel),
            OC_BLOB_GET (&Block->MaxKernel),
            KernelVersion
            ))
    {
      DEBUG ((DEBUG_WARN, "OC: ForceRealtekEthernet conflicts with Kernel->Block[%u] (%a)\n", Index, OC_BLOB_GET (&Block->Identifier)));
      ++Errors;
    }
  }

  for (Index = 0; Index < Config->Kernel.Patch.Count; ++Index) {
    Patch = Config->Kernel.Patch.Values[Index];
    if (  Patch->Enabled
       && InternalRealtekIdentifier (OC_BLOB_GET (&Patch->Identifier))
       && InternalRealtekEntryActive (
            OC_BLOB_GET (&Patch->Arch),
            OC_BLOB_GET (&Patch->MinKernel),
            OC_BLOB_GET (&Patch->MaxKernel),
            KernelVersion
            ))
    {
      DEBUG ((DEBUG_WARN, "OC: ForceRealtekEthernet conflicts with Kernel->Patch[%u] (%a)\n", Index, OC_BLOB_GET (&Patch->Identifier)));
      ++Errors;
    }
  }

  for (Kind = 0; Kind < 2; ++Kind) {
    Count = Kind == 0 ? Config->Kernel.Add.Count : Config->Kernel.Force.Count;
    for (Index = 0; Index < Count; ++Index) {
      Entry = Kind == 0 ? Config->Kernel.Add.Values[Index] : Config->Kernel.Force.Values[Index];
      if (  Entry->Enabled
         && InternalRealtekEntryActive (
              OC_BLOB_GET (&Entry->Arch),
              OC_BLOB_GET (&Entry->MinKernel),
              OC_BLOB_GET (&Entry->MaxKernel),
              KernelVersion
              )
         && (  InternalRealtekIdentifier (OC_BLOB_GET (&Entry->Identifier))
            || InternalRealtekPath (OC_BLOB_GET (&Entry->BundlePath))
            || InternalRealtekPath (OC_BLOB_GET (&Entry->ExecutablePath))
            || InternalRealtekInjectedInfo (Entry)))
      {
        DEBUG ((DEBUG_WARN, "OC: ForceRealtekEthernet conflicts with Kernel->%a[%u] (%a)\n", Kind == 0 ? "Add" : "Force", Index, OC_BLOB_GET (&Entry->BundlePath)));
        ++Errors;
      }
    }
  }

  return Errors;
}
