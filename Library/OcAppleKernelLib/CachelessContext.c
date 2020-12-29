/** @file
  Cacheless boot (S/L/E) support.

  Copyright (c) 2020, Goldfish64. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcVirtualFsLib.h>

#include "CachelessInternal.h"
#include "PrelinkedInternal.h"

STATIC
VOID
FreeBuiltInKext (
  IN BUILTIN_KEXT   *BuiltinKext
  )
{
  DEPEND_KEXT       *DependKext;
  LIST_ENTRY        *KextLink;

  if (BuiltinKext->PlistPath != NULL) {
    FreePool (BuiltinKext->PlistPath);
  }
  if (BuiltinKext->Identifier != NULL) {
    FreePool (BuiltinKext->Identifier);
  }
  if (BuiltinKext->BinaryFileName != NULL) {
    FreePool (BuiltinKext->BinaryFileName);
  }
  if (BuiltinKext->BinaryPath != NULL) {
    FreePool (BuiltinKext->BinaryPath);
  }

  while (!IsListEmpty (&BuiltinKext->Dependencies)) {
    KextLink = GetFirstNode (&BuiltinKext->Dependencies);
    DependKext = GET_DEPEND_KEXT_FROM_LINK (KextLink);
    RemoveEntryList (KextLink);

    if (DependKext->Identifier != NULL) {
      FreePool (DependKext->Identifier);
    }
    FreePool (DependKext);
  }

  FreePool (BuiltinKext);
}

STATIC
EFI_STATUS
AddKextDependency (
  IN OUT LIST_ENTRY           *Dependencies,
  IN     CONST CHAR8          *Identifier
  )
{
  DEPEND_KEXT       *DependKext;
  LIST_ENTRY        *KextLink;

  KextLink = GetFirstNode (Dependencies);
  while (!IsNull (Dependencies, KextLink)) {
    DependKext = GET_DEPEND_KEXT_FROM_LINK (KextLink);

    if (AsciiStrCmp (DependKext->Identifier, Identifier) == 0) {
      return EFI_SUCCESS;
    }

    KextLink = GetNextNode (Dependencies, KextLink);
  }

  DependKext = AllocateZeroPool (sizeof (*DependKext));
  if (DependKext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  DependKext->Signature = DEPEND_KEXT_SIGNATURE;
  DependKext->Identifier = AllocateCopyPool (AsciiStrSize (Identifier), Identifier);
  if (DependKext->Identifier == NULL) {
    FreePool (DependKext);
    return EFI_OUT_OF_RESOURCES;
  }

  InsertTailList (Dependencies, &DependKext->Link);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
AddKextDependencies (
  IN OUT LIST_ENTRY           *Dependencies,
  IN     XML_NODE             *InfoPlistLibraries
  )
{
  EFI_STATUS        Status;
  UINT32            ChildCount;
  UINT32            ChildIndex;
  CONST CHAR8       *ChildPlistKey;

  ChildCount = PlistDictChildren (InfoPlistLibraries);
  for (ChildIndex = 0; ChildIndex < ChildCount; ChildIndex++) {
    ChildPlistKey = PlistKeyValue (PlistDictChild (InfoPlistLibraries, ChildIndex, NULL));
    if (ChildPlistKey == NULL) {
      continue;
    }

    Status = AddKextDependency (Dependencies, ChildPlistKey);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ScanExtensions (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     EFI_FILE_PROTOCOL    *File,
  IN     CONST CHAR16         *FilePath,
  IN     BOOLEAN              ReadPlugins
  )
{
  EFI_STATUS          Status;
  EFI_FILE_PROTOCOL   *FileKext;
  EFI_FILE_PROTOCOL   *FilePlist;
  EFI_FILE_PROTOCOL   *FilePlugins;
  EFI_FILE_INFO       *FileInfo;
  UINTN               FileInfoSize;
  BOOLEAN             UseContents;

  CHAR8               *InfoPlist;
  UINT32              InfoPlistSize;
  XML_DOCUMENT        *InfoPlistDocument;
  XML_NODE            *InfoPlistRoot;
  XML_NODE            *InfoPlistValue;
  XML_NODE            *InfoPlistLibraries;
  CONST CHAR8         *TmpKeyValue;
  UINT32              FieldCount;
  UINT32              FieldIndex;

  BUILTIN_KEXT        *BuiltinKext;
  CHAR16              TmpPath[256];

  DEBUG ((DEBUG_INFO, "OCAK: Scanning %s...\n", FilePath));

  FileInfo = AllocatePool (SIZE_1KB);
  if (FileInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  File->SetPosition (File, 0);

  do {
    //
    // Apple's HFS+ driver does not adhere to the spec and will return zero for
    // EFI_BUFFER_TOO_SMALL. EFI_FILE_INFO structures larger than 1KB are
    // unrealistic as the filename is the only variable.
    //
    FileInfoSize = SIZE_1KB - sizeof (CHAR16);
    Status = File->Read (File, &FileInfoSize, FileInfo);
    if (EFI_ERROR (Status)) {
      FileKext->Close (FileKext);
      File->SetPosition (File, 0);
      FreePool (FileInfo);
      return Status;
    }

    if (FileInfoSize > 0) {
      if (OcUnicodeEndsWith (FileInfo->FileName, L".kext", FALSE)) {
        Status = File->Open (File, &FileKext, FileInfo->FileName, EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);
        if (EFI_ERROR (Status)) {
          continue;
        }

        //
        // Determine if Contents directory exists.
        // If not, we'll use the root of the kext.
        //
        Status      = FileKext->Open (FileKext, &FilePlist, L"Contents", EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);
        UseContents = !EFI_ERROR (Status);

        //
        // There are some kexts that do not have an Info.plist, but do have PlugIns.
        // This was observed in some versions of 10.4.
        //
        Status = FileKext->Open (
          FileKext,
          &FilePlist,
          UseContents ? L"Contents\\Info.plist" : L"Info.plist",
          EFI_FILE_MODE_READ,
          0
          );
        if (!EFI_ERROR (Status)) {
          //
          // Parse Info.plist.
          //
          Status = AllocateCopyFileData (FilePlist, (UINT8**)&InfoPlist, &InfoPlistSize);
          FilePlist->Close (FilePlist);
          if (EFI_ERROR (Status)) {
            FileKext->Close (FileKext);
            File->SetPosition (File, 0);
            FreePool (FileInfo);
            return Status;
          }

          InfoPlistDocument = XmlDocumentParse (InfoPlist, InfoPlistSize, FALSE);
          if (InfoPlistDocument == NULL) {
            FreePool (InfoPlist);
            FileKext->Close (FileKext);
            File->SetPosition (File, 0);
            FreePool (FileInfo);
            return EFI_INVALID_PARAMETER;
          }

          InfoPlistRoot = PlistNodeCast (PlistDocumentRoot (InfoPlistDocument), PLIST_NODE_TYPE_DICT);
          if (InfoPlistRoot == NULL) {
            XmlDocumentFree (InfoPlistDocument);
            FreePool (InfoPlist);
            FileKext->Close (FileKext);
            File->SetPosition (File, 0);
            FreePool (FileInfo);
            return EFI_INVALID_PARAMETER;
          }

          //
          // Add to built-in kexts list.
          //
          BuiltinKext = AllocateZeroPool (sizeof (*BuiltinKext));
          if (BuiltinKext == NULL) {
            XmlDocumentFree (InfoPlistDocument);
            FreePool (InfoPlist);
            FileKext->Close (FileKext);
            File->SetPosition (File, 0);
            FreePool (FileInfo);
            return EFI_OUT_OF_RESOURCES;
          }
          BuiltinKext->Signature = BUILTIN_KEXT_SIGNATURE;
          InitializeListHead (&BuiltinKext->Dependencies);

          //
          // Search for plist properties.
          //
          FieldCount = PlistDictChildren (InfoPlistRoot);
          for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
            TmpKeyValue = PlistKeyValue (PlistDictChild (InfoPlistRoot, FieldIndex, &InfoPlistValue));
            if (TmpKeyValue == NULL) {
              continue;
            }

            if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_EXECUTABLE_KEY) == 0) {
              BuiltinKext->BinaryFileName = AsciiStrCopyToUnicode (XmlNodeContent (InfoPlistValue), 0);
              if (BuiltinKext->BinaryFileName == NULL) {
                FreeBuiltInKext (BuiltinKext);
                XmlDocumentFree (InfoPlistDocument);
                FreePool (InfoPlist);
                FileKext->Close (FileKext);
                File->SetPosition (File, 0);
                FreePool (FileInfo);
                return EFI_OUT_OF_RESOURCES;
              }

            } else if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_IDENTIFIER_KEY) == 0) {
              BuiltinKext->Identifier = AllocateCopyPool (AsciiStrSize (XmlNodeContent (InfoPlistValue)), XmlNodeContent (InfoPlistValue));
              if (BuiltinKext->Identifier == NULL) {
                FreeBuiltInKext (BuiltinKext);
                XmlDocumentFree (InfoPlistDocument);
                FreePool (InfoPlist);
                FileKext->Close (FileKext);
                File->SetPosition (File, 0);
                FreePool (FileInfo);
                return EFI_OUT_OF_RESOURCES;
              }

            } else if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_OS_BUNDLE_REQUIRED_KEY) == 0) {
              //
              // If OSBundleRequired is present and is not Safe Boot, no action is required.
              //
              if (AsciiStrCmp (XmlNodeContent (InfoPlistValue), OS_BUNDLE_REQUIRED_SAFE_BOOT) != 0) {
                BuiltinKext->OSBundleRequiredValue = KEXT_OSBUNDLE_REQUIRED_VALID;
              } else {
                BuiltinKext->OSBundleRequiredValue = KEXT_OSBUNDLE_REQUIRED_INVALID;
              }

            } else if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_LIBRARIES_KEY) == 0) {
              InfoPlistLibraries = PlistNodeCast (InfoPlistValue, PLIST_NODE_TYPE_DICT);
              if (InfoPlistLibraries == NULL) {
                FreeBuiltInKext (BuiltinKext);
                XmlDocumentFree (InfoPlistDocument);
                FreePool (InfoPlist);
                FileKext->Close (FileKext);
                File->SetPosition (File, 0);
                FreePool (FileInfo);
                return EFI_INVALID_PARAMETER;
              }

              Status = AddKextDependencies (&BuiltinKext->Dependencies, InfoPlistLibraries);
              if (EFI_ERROR (Status)) {
                FreeBuiltInKext (BuiltinKext);
                XmlDocumentFree (InfoPlistDocument);
                FreePool (InfoPlist);
                FileKext->Close (FileKext);
                File->SetPosition (File, 0);
                FreePool (FileInfo);
                return Status;
              }
            }
          }

          XmlDocumentFree (InfoPlistDocument);
          FreePool (InfoPlist);

          if (BuiltinKext->Identifier == NULL) {
            FreeBuiltInKext (BuiltinKext);
            FileKext->Close (FileKext);
            File->SetPosition (File, 0);
            FreePool (FileInfo);
            return EFI_INVALID_PARAMETER;
          }

          //
          // Create plist path.
          //
          Status = OcUnicodeSafeSPrint (
            TmpPath,
            sizeof (TmpPath),
            L"%s\\%s\\%s",
            FilePath,
            FileInfo->FileName,
            UseContents ? L"Contents\\Info.plist" : L"Info.plist"
            );
          if (EFI_ERROR (Status)) {
            FreeBuiltInKext (BuiltinKext);
            FileKext->Close (FileKext);
            File->SetPosition (File, 0);
            FreePool (FileInfo);
            return EFI_INVALID_PARAMETER;
          }

          BuiltinKext->PlistPath = AllocateCopyPool (StrSize (TmpPath), TmpPath);
          if (BuiltinKext->PlistPath == NULL) {
            FreeBuiltInKext (BuiltinKext);
            FileKext->Close (FileKext);
            File->SetPosition (File, 0);
            FreePool (FileInfo);
            return EFI_OUT_OF_RESOURCES;
          }

          //
          // Create binary path. If plist is in root of kext, binary is also there.
          //
          if (BuiltinKext->BinaryFileName != NULL) {
            Status = OcUnicodeSafeSPrint (
              TmpPath,
              sizeof (TmpPath),
              L"%s\\%s\\%s%s",
              FilePath,
              FileInfo->FileName,
              UseContents ? L"Contents\\MacOS\\" : L"\\",
              BuiltinKext->BinaryFileName
              );
            if (EFI_ERROR (Status)) {
              FreeBuiltInKext (BuiltinKext);
              FileKext->Close (FileKext);
              File->SetPosition (File, 0);
              FreePool (FileInfo);
              return EFI_INVALID_PARAMETER;
            }

            BuiltinKext->BinaryPath = AllocateCopyPool (StrSize (TmpPath), TmpPath);
            if (BuiltinKext->BinaryPath == NULL) {
              FreeBuiltInKext (BuiltinKext);
              FileKext->Close (FileKext);
              File->SetPosition (File, 0);
              FreePool (FileInfo);
              return EFI_OUT_OF_RESOURCES;
            }
          }

          InsertTailList (&Context->BuiltInKexts, &BuiltinKext->Link);
          DEBUG ((
            DEBUG_VERBOSE,
            "OCAK: Discovered bundle %a %s %s %u\n",
            BuiltinKext->Identifier,
            BuiltinKext->PlistPath,
            BuiltinKext->BinaryPath,
            BuiltinKext->OSBundleRequiredValue
            ));
        }

        //
        // Scan PlugIns directory.
        //
        if (ReadPlugins) {
          Status = FileKext->Open (FileKext, &FilePlugins, UseContents ? L"Contents\\PlugIns" : L"PlugIns", EFI_FILE_MODE_READ, EFI_FILE_DIRECTORY);
          if (Status == EFI_SUCCESS) {
            Status = OcUnicodeSafeSPrint (
              TmpPath,
              sizeof (TmpPath),
              L"%s\\%s\\%s",
              FilePath,
              FileInfo->FileName,
              UseContents ? L"Contents\\PlugIns" : L"PlugIns"
              );

            Status = ScanExtensions (Context, FilePlugins, TmpPath, FALSE);
            FilePlugins->Close (FilePlugins);
            if (EFI_ERROR (Status)) {
              FileKext->Close (FileKext);
              File->SetPosition (File, 0);
              FreePool (FileInfo);
              return Status;
            }
          } else if (Status != EFI_NOT_FOUND) {
            FileKext->Close (FileKext);
            File->SetPosition (File, 0);
            FreePool (FileInfo);
            return Status;
          }
        }

        FileKext->Close (FileKext);
      }
    }
  } while (FileInfoSize > 0);

  File->SetPosition (File, 0);
  FreePool (FileInfo);

  return EFI_SUCCESS;
}

STATIC
PATCHED_KEXT*
LookupPatchedKextForIdentifier (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR8          *Identifier
  )
{
  PATCHED_KEXT  *PatchedKext;
  LIST_ENTRY    *KextLink;

  KextLink = GetFirstNode (&Context->PatchedKexts);
  while (!IsNull (&Context->PatchedKexts, KextLink)) {
    PatchedKext = GET_PATCHED_KEXT_FROM_LINK (KextLink);
 
    if (AsciiStrCmp (Identifier, PatchedKext->Identifier) == 0) {
      return PatchedKext;
    }

    KextLink = GetNextNode (&Context->PatchedKexts, KextLink);
  }

  return NULL;
}

STATIC
BUILTIN_KEXT*
LookupBuiltinKextForIdentifier (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR8          *Identifier
  )
{
  BUILTIN_KEXT  *BuiltinKext;
  LIST_ENTRY    *KextLink;

  KextLink = GetFirstNode (&Context->BuiltInKexts);
  while (!IsNull (&Context->BuiltInKexts, KextLink)) {
    BuiltinKext = GET_BUILTIN_KEXT_FROM_LINK (KextLink);
 
    if (AsciiStrCmp (Identifier, BuiltinKext->Identifier) == 0) {
      return BuiltinKext;
    }

    KextLink = GetNextNode (&Context->BuiltInKexts, KextLink);
  }

  return NULL;
}

STATIC
BUILTIN_KEXT*
LookupBuiltinKextForPlistPath (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR16         *PlistPath
  )
{
  BUILTIN_KEXT  *BuiltinKext;
  LIST_ENTRY    *KextLink;

  KextLink = GetFirstNode (&Context->BuiltInKexts);
  while (!IsNull (&Context->BuiltInKexts, KextLink)) {
    BuiltinKext = GET_BUILTIN_KEXT_FROM_LINK (KextLink);
 
    if (StrCmp (PlistPath, BuiltinKext->PlistPath) == 0) {
      return BuiltinKext;
    }

    KextLink = GetNextNode (&Context->BuiltInKexts, KextLink);
  }

  return NULL;
}

STATIC
BUILTIN_KEXT*
LookupBuiltinKextForBinaryPath (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR16         *BinaryPath
  )
{
  BUILTIN_KEXT  *BuiltinKext;
  LIST_ENTRY    *KextLink;

  KextLink = GetFirstNode (&Context->BuiltInKexts);
  while (!IsNull (&Context->BuiltInKexts, KextLink)) {
    BuiltinKext = GET_BUILTIN_KEXT_FROM_LINK (KextLink);
 
    if (BuiltinKext->BinaryPath != NULL
      && StrCmp (BinaryPath, BuiltinKext->BinaryPath) == 0) {
      return BuiltinKext;
    }

    KextLink = GetNextNode (&Context->BuiltInKexts, KextLink);
  }

  return NULL;
}

STATIC
EFI_STATUS
ScanDependencies (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CHAR8                *Identifier
  )
{
  EFI_STATUS      Status;
  BUILTIN_KEXT    *BuiltinKext;
  DEPEND_KEXT     *DependKext;
  LIST_ENTRY      *KextLink;

  DEBUG ((DEBUG_VERBOSE, "OCAK: Scanning dependencies for %a\n", Identifier));

  BuiltinKext = LookupBuiltinKextForIdentifier (Context, Identifier);
  if (BuiltinKext == NULL || BuiltinKext->OSBundleRequiredValue == KEXT_OSBUNDLE_REQUIRED_VALID) {
    //
    // Injected kexts may have dependencies on other injected kexts, which we do not need to handle.
    // We should be able to safely assume that any kext with the OSBundleRequired set correctly does not need to be handled either.
    //
    return EFI_SUCCESS;
  }

  BuiltinKext->PatchValidOSBundleRequired = TRUE;

  Status = AddKextDependency (&Context->InjectedDependencies, Identifier);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  KextLink = GetFirstNode (&BuiltinKext->Dependencies);
  while (!IsNull (&BuiltinKext->Dependencies, KextLink)) {
    DependKext = GET_DEPEND_KEXT_FROM_LINK (KextLink);

    Status = ScanDependencies (Context, DependKext->Identifier);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    KextLink = GetNextNode (&BuiltinKext->Dependencies, KextLink);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalAddPatchedKext (
  IN OUT CACHELESS_CONTEXT      *Context,
  IN     CONST CHAR8            *Identifier,
     OUT PATCHED_KEXT           **Kext
  )
{
  PATCHED_KEXT *PatchedKext;

  PatchedKext = AllocateZeroPool (sizeof (*PatchedKext));
  if (PatchedKext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  PatchedKext->Signature  = PATCHED_KEXT_SIGNATURE;
  PatchedKext->Identifier = AllocateCopyPool (AsciiStrSize (Identifier), Identifier);
  if (PatchedKext->Identifier == NULL) {
    FreePool (PatchedKext);
    return EFI_OUT_OF_RESOURCES;
  }
  InitializeListHead (&PatchedKext->Patches);

  InsertTailList (&Context->PatchedKexts, &PatchedKext->Link);

  *Kext = PatchedKext;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InternalAddKextPatch (
  IN OUT CACHELESS_CONTEXT      *Context,
  IN     CONST CHAR8            *Identifier,
  IN     PATCHER_GENERIC_PATCH  *Patch OPTIONAL,
  IN     KERNEL_QUIRK_NAME      QuirkName
  )
{
  EFI_STATUS            Status;
  PATCHED_KEXT          *PatchedKext;
  KEXT_PATCH            *KextPatch;
  KERNEL_QUIRK          *KernelQuirk;

  if (Patch == NULL) {
    KernelQuirk = &gKernelQuirks[QuirkName];
    ASSERT (KernelQuirk->Identifier != NULL);

    Identifier = KernelQuirk->Identifier;
  }

  //
  // Check if bundle is already present. If not, add to list.
  //
  PatchedKext = LookupPatchedKextForIdentifier (Context, Identifier);
  if (PatchedKext == NULL) {
    Status = InternalAddPatchedKext (Context, Identifier, &PatchedKext);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Add new patch.
  //
  KextPatch = AllocateZeroPool (sizeof (*KextPatch));
  if (KextPatch == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  KextPatch->Signature = KEXT_PATCH_SIGNATURE;

  if (Patch != NULL) {
    CopyMem (&KextPatch->Patch, Patch, sizeof (KextPatch->Patch));

  } else {
    //
    // Apply quirk if no Patch.
    //
    KextPatch->ApplyQuirk = TRUE;
    KextPatch->QuirkName  = QuirkName;
  }
  
  InsertTailList (&PatchedKext->Patches, &KextPatch->Link);

  return EFI_SUCCESS;
}

EFI_STATUS
CachelessContextInit (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR16         *FileName,
  IN     EFI_FILE_PROTOCOL    *ExtensionsDir,
  IN     UINT32               KernelVersion,
  IN     BOOLEAN              Is32Bit
  )
{
  ASSERT (Context != NULL);
  ASSERT (FileName != NULL);
  ASSERT (ExtensionsDir != NULL);

  ZeroMem (Context, sizeof (*Context));

  Context->ExtensionsDir          = ExtensionsDir;
  Context->ExtensionsDirFileName  = FileName;
  Context->KernelVersion          = KernelVersion;
  Context->Is32Bit                = Is32Bit;
  
  InitializeListHead (&Context->InjectedKexts);
  InitializeListHead (&Context->InjectedDependencies);
  InitializeListHead (&Context->PatchedKexts);
  InitializeListHead (&Context->BuiltInKexts);

  return EFI_SUCCESS;
}

VOID
CachelessContextFree (
  IN OUT CACHELESS_CONTEXT    *Context
  )
{
  CACHELESS_KEXT      *CachelessKext;
  PATCHED_KEXT        *PatchedKext;
  KEXT_PATCH          *KextPatch;
  BUILTIN_KEXT        *BuiltinKext;
  LIST_ENTRY          *KextLink;
  LIST_ENTRY          *PatchLink;

  ASSERT (Context != NULL);

  while (!IsListEmpty (&Context->InjectedKexts)) {
    KextLink = GetFirstNode (&Context->InjectedKexts);
    CachelessKext = GET_CACHELESS_KEXT_FROM_LINK (KextLink);
    RemoveEntryList (KextLink);

    if (CachelessKext->PlistData != NULL) {
      FreePool (CachelessKext->PlistData);
    }
    if (CachelessKext->BinaryData != NULL) {
      FreePool (CachelessKext->BinaryData);
    }
    if (CachelessKext->BinaryFileName != NULL) {
      FreePool (CachelessKext->BinaryFileName);
    }
    FreePool (CachelessKext);
  }

  while (!IsListEmpty (&Context->PatchedKexts)) {
    KextLink = GetFirstNode (&Context->PatchedKexts);
    PatchedKext = GET_PATCHED_KEXT_FROM_LINK (KextLink);
    RemoveEntryList (KextLink);

    while (!IsListEmpty (&PatchedKext->Patches)) {
      PatchLink = GetFirstNode (&PatchedKext->Patches);
      KextPatch = GET_KEXT_PATCH_FROM_LINK (PatchLink);
      RemoveEntryList (PatchLink);

      FreePool (KextPatch);
    }

    FreePool (PatchedKext);
  }

  while (!IsListEmpty (&Context->BuiltInKexts)) {
    KextLink = GetFirstNode (&Context->BuiltInKexts);
    BuiltinKext = GET_BUILTIN_KEXT_FROM_LINK (KextLink);
    RemoveEntryList (KextLink);

    if (BuiltinKext->PlistPath != NULL) {
      FreePool (BuiltinKext->PlistPath);
    }
    if (BuiltinKext->Identifier != NULL) {
      FreePool (BuiltinKext->Identifier);
    }
    if (BuiltinKext->BinaryFileName != NULL) {
      FreePool (BuiltinKext->BinaryFileName);
    }
    FreePool (BuiltinKext);
  }
  
  ZeroMem (Context, sizeof (*Context));
}

EFI_STATUS
CachelessContextAddKext (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR8          *InfoPlist,
  IN     UINT32               InfoPlistSize,
  IN     CONST UINT8          *Executable OPTIONAL,
  IN     UINT32               ExecutableSize OPTIONAL
  )
{
  EFI_STATUS        Status;
  CACHELESS_KEXT    *NewKext;

  XML_DOCUMENT      *InfoPlistDocument;
  XML_NODE          *InfoPlistRoot;
  XML_NODE          *InfoPlistValue;
  XML_NODE          *InfoPlistLibraries;
  CHAR8             *TmpInfoPlist;
  CONST CHAR8       *TmpKeyValue;
  UINT32            FieldCount;
  UINT32            FieldIndex;

  BOOLEAN           Failed;
  BOOLEAN           IsLoadable;
  BOOLEAN           PlistHasChanges;
  CHAR8             *NewPlistData;
  UINT32            NewPlistDataSize;

  ASSERT (Context != NULL);
  ASSERT (InfoPlist != NULL);
  ASSERT (InfoPlistSize > 0);

  IsLoadable      = FALSE;
  PlistHasChanges = FALSE;

  NewKext = AllocateZeroPool (sizeof (*NewKext));
  if (NewKext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewKext->Signature = CACHELESS_KEXT_SIGNATURE;
  NewKext->PlistData = AllocateCopyPool (InfoPlistSize, InfoPlist);
  if (NewKext->PlistData == NULL) {
    FreePool (NewKext);
    return EFI_OUT_OF_RESOURCES;
  }
  NewKext->PlistDataSize = InfoPlistSize;

  //
  // Allocate Info.plist copy for XML_DOCUMENT.
  //
  TmpInfoPlist = AllocateCopyPool (InfoPlistSize, InfoPlist);
  if (TmpInfoPlist == NULL) {
    FreePool (NewKext->PlistData);
    FreePool (NewKext);
    return EFI_OUT_OF_RESOURCES;
  }

  InfoPlistDocument = XmlDocumentParse (TmpInfoPlist, InfoPlistSize, FALSE);
  if (InfoPlistDocument == NULL) {
    FreePool (TmpInfoPlist);
    FreePool (NewKext->PlistData);
    FreePool (NewKext);
    return EFI_INVALID_PARAMETER;
  }

  InfoPlistRoot = PlistNodeCast (PlistDocumentRoot (InfoPlistDocument), PLIST_NODE_TYPE_DICT);
  if (InfoPlistRoot == NULL) {
    XmlDocumentFree (InfoPlistDocument);
    FreePool (TmpInfoPlist);
    FreePool (NewKext->PlistData);
    FreePool (NewKext);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Search for plist properties.
  //
  FieldCount = PlistDictChildren (InfoPlistRoot);
  for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
    TmpKeyValue = PlistKeyValue (PlistDictChild (InfoPlistRoot, FieldIndex, &InfoPlistValue));
    if (TmpKeyValue == NULL) {
      continue;
    }

    if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_EXECUTABLE_KEY) == 0) {
      //
      // We are not supposed to check for this, it is XNU responsibility, which reliably panics.
      // However, to avoid certain users making this kind of mistake, we still provide some
      // code in debug mode to diagnose it.
      //
      DEBUG_CODE_BEGIN ();
      if (Executable == NULL) {
        DEBUG ((DEBUG_ERROR, "OCAK: Plist-only kext has %a key\n", INFO_BUNDLE_EXECUTABLE_KEY));
        ASSERT (FALSE);
        CpuDeadLoop ();
      }
      DEBUG_CODE_END ();

      NewKext->BinaryFileName = AsciiStrCopyToUnicode (XmlNodeContent (InfoPlistValue), 0);
      if (NewKext->BinaryFileName == NULL) {
        XmlDocumentFree (InfoPlistDocument);
        FreePool (TmpInfoPlist);
        FreePool (NewKext->PlistData);
        FreePool (NewKext);
        return EFI_INVALID_PARAMETER;
      }

    } else if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_OS_BUNDLE_REQUIRED_KEY) == 0) {
      //
      // If OSBundleRequired is present and is not Safe Boot, no action is required.
      //
      if (AsciiStrCmp (XmlNodeContent (InfoPlistValue), OS_BUNDLE_REQUIRED_SAFE_BOOT) == 0) {
        XmlNodeChangeContent (InfoPlistValue, OS_BUNDLE_REQUIRED_ROOT);
        PlistHasChanges = TRUE;
      }
      IsLoadable = TRUE;

    } else if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_LIBRARIES_KEY) == 0) {
      InfoPlistLibraries = PlistNodeCast (InfoPlistValue, PLIST_NODE_TYPE_DICT);
      if (InfoPlistLibraries == NULL) {
        XmlDocumentFree (InfoPlistDocument);
        FreePool (TmpInfoPlist);
        FreePool (NewKext->PlistData);
        FreePool (NewKext);
        return EFI_INVALID_PARAMETER;
      }

      Status = AddKextDependencies (&Context->InjectedDependencies, InfoPlistLibraries);
      if (EFI_ERROR (Status)) {
        XmlDocumentFree (InfoPlistDocument);
        FreePool (TmpInfoPlist);
        FreePool (NewKext->PlistData);
        FreePool (NewKext);
        return Status;
      }
    }
  }

  //
  // Add OSBundleRequired if not found.
  //
  if (!IsLoadable) {
    Failed = FALSE;
    Failed |= XmlNodeAppend (InfoPlistRoot, "key", NULL, INFO_BUNDLE_OS_BUNDLE_REQUIRED_KEY) == NULL;
    Failed |= XmlNodeAppend (InfoPlistRoot, "string", NULL, OS_BUNDLE_REQUIRED_ROOT) == NULL;

    if (Failed) {
      XmlDocumentFree (InfoPlistDocument);
      FreePool (TmpInfoPlist);
      if (NewKext->BinaryFileName != NULL) {
        FreePool (NewKext->BinaryFileName);
      }
      FreePool (NewKext->PlistData);
      FreePool (NewKext);
      return EFI_OUT_OF_RESOURCES;
    }

    PlistHasChanges = TRUE;
  }

  if (PlistHasChanges) {
    NewPlistData = XmlDocumentExport (InfoPlistDocument, &NewPlistDataSize, 0, TRUE);
    if (NewPlistData == NULL) {
      XmlDocumentFree (InfoPlistDocument);
      FreePool (TmpInfoPlist);
      if (NewKext->BinaryFileName != NULL) {
        FreePool (NewKext->BinaryFileName);
      }
      FreePool (NewKext->PlistData);
      FreePool (NewKext);
      return EFI_OUT_OF_RESOURCES;
    }
    FreePool (NewKext->PlistData);
    NewKext->PlistData = NewPlistData;
    NewKext->PlistDataSize = NewPlistDataSize;
  }

  XmlDocumentFree (InfoPlistDocument);
  FreePool (TmpInfoPlist);

  if (Executable != NULL) {
    ASSERT (ExecutableSize > 0);

    //
    // Ensure a binary name was found.
    //
    ASSERT (NewKext->BinaryFileName != NULL);

    NewKext->BinaryData = AllocateCopyPool (ExecutableSize, Executable);
    if (NewKext->BinaryData == NULL) {
      if (NewKext->BinaryFileName != NULL) {
        FreePool (NewKext->BinaryFileName);
      }
      FreePool (NewKext->PlistData);
      FreePool (NewKext);
      return EFI_OUT_OF_RESOURCES;
    }
    NewKext->BinaryDataSize = ExecutableSize;
  }

  InsertTailList (&Context->InjectedKexts, &NewKext->Link);
  return EFI_SUCCESS;
}

EFI_STATUS
CachelessContextForceKext (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR8          *Identifier
  )
{
  ASSERT (Context != NULL);
  ASSERT (Identifier != NULL);

  return AddKextDependency (&Context->InjectedDependencies, Identifier);
}

EFI_STATUS
CachelessContextAddPatch (
  IN OUT CACHELESS_CONTEXT      *Context,
  IN     CONST CHAR8            *Identifier,
  IN     PATCHER_GENERIC_PATCH  *Patch
  )
{
  ASSERT (Context != NULL);
  ASSERT (Identifier != NULL);
  ASSERT (Patch != NULL);

  return InternalAddKextPatch (Context, Identifier, Patch, 0);
}

EFI_STATUS
CachelessContextAddQuirk (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     KERNEL_QUIRK_NAME    Quirk
  )
{
  ASSERT (Context != NULL);

  return InternalAddKextPatch (Context, NULL, NULL, Quirk);
}

EFI_STATUS
CachelessContextBlock (
  IN OUT CACHELESS_CONTEXT      *Context,
  IN     CONST CHAR8            *Identifier
  )
{
  EFI_STATUS      Status;
  PATCHED_KEXT    *PatchedKext;

  //
  // Check if bundle is already present. If not, add to list.
  //
  PatchedKext = LookupPatchedKextForIdentifier (Context, Identifier);
  if (PatchedKext == NULL) {
    Status = InternalAddPatchedKext (Context, Identifier, &PatchedKext);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  PatchedKext->Block = TRUE;  

  return EFI_SUCCESS;
}

EFI_STATUS
CachelessContextOverlayExtensionsDir (
  IN OUT CACHELESS_CONTEXT    *Context,
     OUT EFI_FILE_PROTOCOL    **File
  )
{
  EFI_STATUS            Status;
  EFI_FILE_PROTOCOL     *ExtensionsDirOverlay;

  CACHELESS_KEXT        *Kext;
  LIST_ENTRY            *KextLink;
  EFI_FILE_INFO         *DirectoryEntry;
  EFI_FILE_PROTOCOL     *NewFile;
  EFI_TIME              ModificationTime;
  UINT32                FileNameIndex;

  ASSERT (Context != NULL);
  ASSERT (File != NULL);

  //
  // Create directory overlay.
  //
  Status = GetFileModificationTime (Context->ExtensionsDir, &ModificationTime);
  if (EFI_ERROR (Status)) {
    ZeroMem (&ModificationTime, sizeof (ModificationTime));
  }

  Status = VirtualDirCreateOverlayFileNameCopy (Context->ExtensionsDirFileName, &ModificationTime, Context->ExtensionsDir, &ExtensionsDirOverlay);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Inject kexts.
  //
  FileNameIndex = 0;
  KextLink = GetFirstNode (&Context->InjectedKexts);
  while (!IsNull (&Context->InjectedKexts, KextLink)) {
    Kext = GET_CACHELESS_KEXT_FROM_LINK (KextLink);

    //
    // Generate next available filename, testing for an existing file to ensure no conflicts.
    //
    do {
      if (FileNameIndex == MAX_UINT32) {
        return EFI_DEVICE_ERROR;
      }

      UnicodeSPrint (Kext->BundleFileName, KEXT_BUNDLE_NAME_SIZE, L"Oc%8X.kext", FileNameIndex++);

      Status = Context->ExtensionsDir->Open (Context->ExtensionsDir, &NewFile, Kext->BundleFileName, EFI_FILE_MODE_READ, 0);
      if (!EFI_ERROR (Status)) {
        NewFile->Close (NewFile);
      }
    } while (!EFI_ERROR (Status));

    DirectoryEntry = AllocateZeroPool (KEXT_BUNDLE_INFO_SIZE);
    if (DirectoryEntry == NULL) {
      VirtualDirFree (ExtensionsDirOverlay);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Populate file information.
    //
    CopyMem (DirectoryEntry->FileName, Kext->BundleFileName, KEXT_BUNDLE_NAME_SIZE);
    DirectoryEntry->Size = KEXT_BUNDLE_INFO_SIZE;
    DirectoryEntry->Attribute = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;
    DirectoryEntry->FileSize = SIZE_OF_EFI_FILE_INFO + L_STR_SIZE (L"Contents");
    DirectoryEntry->PhysicalSize = DirectoryEntry->FileSize;

    VirtualDirAddEntry (ExtensionsDirOverlay, DirectoryEntry);

    KextLink = GetNextNode (&Context->InjectedKexts, KextLink);
  }

  //
  // Return the new handle for the overlayed directory.
  //
  *File = ExtensionsDirOverlay;
  return EFI_SUCCESS;
}

EFI_STATUS
CachelessContextPerformInject (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR16         *FileName,
     OUT EFI_FILE_PROTOCOL    **File
  )
{
  EFI_STATUS          Status;
  EFI_FILE_PROTOCOL   *VirtualFileHandle;
  CHAR16              *RealFileName;
  UINT8               *Buffer;
  UINTN               BufferSize;

  CACHELESS_KEXT      *Kext;
  LIST_ENTRY          *KextLink;

  BOOLEAN             IsBinaryKext;
  CHAR16              *BundleName;
  CHAR16              *KextExtension;
  CHAR16              *BundlePath;
  CHAR16              *BundleBinaryPath;
  UINTN               BundleBinaryPathSize;
  UINTN               BundleLength;

  EFI_FILE_INFO       *ContentsInfo;
  EFI_FILE_INFO       *ContentsMacOs;
  UINTN               ContentsInfoEntrySize;
  UINTN               ContentsMacOsEntrySize;

  ASSERT (Context != NULL);
  ASSERT (FileName != NULL);
  ASSERT (File != NULL);

  //
  // Only process injected extensions.
  //
  BundleName = StrStr (FileName, L"Oc");
  if (BundleName == NULL) {
    return EFI_NOT_FOUND;
  }
  KextExtension = StrStr (BundleName, L".kext");
  if (KextExtension == NULL) {
    return EFI_NOT_FOUND;
  }
  BundlePath = KextExtension + L_STR_LEN (L".kext");
  BundleLength = BundlePath - BundleName;

  //
  // Find matching kext.
  //
  KextLink = GetFirstNode (&Context->InjectedKexts);
  while (!IsNull (&Context->InjectedKexts, KextLink)) {
    Kext = GET_CACHELESS_KEXT_FROM_LINK (KextLink);
    if (StrnCmp (BundleName, Kext->BundleFileName, BundleLength) == 0) {
      IsBinaryKext = Kext->BinaryFileName != NULL;

      //
      // Contents is being requested.
      //
      if (StrCmp (BundlePath, L"\\Contents") == 0) {
        //
        // Create virtual Contents directory.
        //
        Status = VirtualDirCreateOverlayFileNameCopy (L"Contents", NULL, NULL, &VirtualFileHandle);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        //
        // Create Info.plist directory entry.
        //
        ContentsInfoEntrySize   = SIZE_OF_EFI_FILE_INFO + L_STR_SIZE (L"Info.plist");
        ContentsInfo = AllocateZeroPool (ContentsInfoEntrySize);
        if (ContentsInfo == NULL) {
          VirtualDirFree (VirtualFileHandle);
          return EFI_OUT_OF_RESOURCES;
        }

        ContentsInfo->Size = ContentsInfoEntrySize;
        CopyMem (ContentsInfo->FileName, L"Info.plist", L_STR_SIZE (L"Info.plist"));
        ContentsInfo->Attribute = EFI_FILE_READ_ONLY;
        ContentsInfo->PhysicalSize = ContentsInfo->FileSize = Kext->PlistDataSize;
        VirtualDirAddEntry (VirtualFileHandle, ContentsInfo);

        if (IsBinaryKext) {
          //
          // Create MacOS directory entry.
          //
          ContentsMacOsEntrySize  = SIZE_OF_EFI_FILE_INFO + L_STR_SIZE (L"MacOS");
          ContentsMacOs = AllocateZeroPool (ContentsMacOsEntrySize);
          if (ContentsMacOs == NULL) {
            FreePool (ContentsInfo);
            VirtualDirFree (VirtualFileHandle);
            return EFI_OUT_OF_RESOURCES;
          }

          ContentsMacOs->Size = ContentsMacOsEntrySize;
          CopyMem (ContentsMacOs->FileName, L"MacOS", L_STR_SIZE (L"MacOS"));
          ContentsMacOs->Attribute = EFI_FILE_READ_ONLY | EFI_FILE_DIRECTORY;
          if (OcOverflowAddU64 (SIZE_OF_EFI_FILE_INFO, StrSize (Kext->BinaryFileName), &ContentsMacOs->FileSize)) {
            FreePool (ContentsInfo);
            VirtualDirFree (VirtualFileHandle);
            return EFI_INVALID_PARAMETER;
          }
          ContentsMacOs->PhysicalSize = ContentsMacOs->FileSize;
          VirtualDirAddEntry (VirtualFileHandle, ContentsMacOs);
        }

      } else {
        //
        // Contents/Info.plist is being requested.
        //
        if (StrCmp (BundlePath, L"\\Contents\\Info.plist") == 0) {
          RealFileName = L"Info.plist";
          BufferSize = Kext->PlistDataSize;
          Buffer = AllocateCopyPool (BufferSize, Kext->PlistData);
          if (Buffer == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }

        //
        // Contents/MacOS/BINARY is being requested.
        // It should be safe to assume there will only be one binary ever requested per kext?
        //
        } else if (IsBinaryKext) {
          if (OcOverflowAddUN (L_STR_SIZE (L"\\Contents\\MacOS\\"), StrSize (Kext->BinaryFileName), &BundleBinaryPathSize)) {
            return EFI_OUT_OF_RESOURCES;
          }
          BundleBinaryPath = AllocateZeroPool (BundleBinaryPathSize);
          if (BundleBinaryPath == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }
          StrCatS (BundleBinaryPath, BundleBinaryPathSize / sizeof (CHAR16), L"\\Contents\\MacOS\\");
          StrCatS (BundleBinaryPath, BundleBinaryPathSize / sizeof (CHAR16), Kext->BinaryFileName);

          if (StrCmp (BundlePath, BundleBinaryPath) == 0) {
            FreePool (BundleBinaryPath);

            RealFileName = Kext->BinaryFileName;
            BufferSize = Kext->BinaryDataSize;
            Buffer = AllocateCopyPool (BufferSize, Kext->BinaryData);
            if (Buffer == NULL) {
              return EFI_OUT_OF_RESOURCES;
            }
          } else {
            FreePool (BundleBinaryPath);
            return EFI_NOT_FOUND;
          }
        } else {
          return EFI_NOT_FOUND;
        }

        //
        // Create virtual file.
        //
        Status = CreateVirtualFileFileNameCopy (RealFileName, Buffer, BufferSize, NULL, &VirtualFileHandle);
        if (EFI_ERROR (Status)) {
          FreePool (Buffer);
          return EFI_OUT_OF_RESOURCES;
        }
      }

      //
      // Return our handle.
      //
      *File = VirtualFileHandle;
      return EFI_SUCCESS;
    }

    KextLink = GetNextNode (&Context->InjectedKexts, KextLink);
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
CachelessContextHookBuiltin (
  IN OUT CACHELESS_CONTEXT    *Context,
  IN     CONST CHAR16         *FileName,
  IN     EFI_FILE_PROTOCOL    *File,
     OUT EFI_FILE_PROTOCOL    **VirtualFile
  )
{
  EFI_STATUS          Status;
  EFI_TIME            ModificationTime;
  BUILTIN_KEXT        *BuiltinKext;
  KEXT_PATCH          *KextPatch;
  PATCHED_KEXT        *PatchedKext;
  DEPEND_KEXT         *DependKext;
  LIST_ENTRY          *KextLink;

  PATCHER_CONTEXT     Patcher;

  VOID                *Buffer;
  UINT32              BufferSize;
  XML_DOCUMENT        *InfoPlistDocument;
  XML_NODE            *InfoPlistRoot;
  XML_NODE            *InfoPlistValue;
  CONST CHAR8         *TmpKeyValue;
  UINT32              FieldCount;
  UINT32              FieldIndex;

  BOOLEAN             Failed;
  CHAR8               *NewPlistData;
  UINT32              NewPlistDataSize;

  ASSERT (Context != NULL);
  ASSERT (FileName != NULL);
  ASSERT (File != NULL);
  ASSERT (VirtualFile != NULL);

  //
  // Scan built-in kexts if we have not yet done so.
  //
  if (!Context->BuiltInKextsValid) {
    DEBUG ((DEBUG_INFO, "OCAK: Built-in kext cache is not yet built, building...\n"));

    //
    // Build list of kexts in system Extensions directory.
    //
    Status = ScanExtensions (Context, Context->ExtensionsDir, Context->ExtensionsDirFileName, TRUE);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Ensure all kexts to be patched will be loaded.
    //
    KextLink = GetFirstNode (&Context->PatchedKexts);
    while (!IsNull (&Context->PatchedKexts, KextLink)) {
      PatchedKext = GET_PATCHED_KEXT_FROM_LINK (KextLink);

      BuiltinKext = LookupBuiltinKextForIdentifier (Context, PatchedKext->Identifier);
      if (BuiltinKext == NULL) {
        //
        // Kext is not present, skip.
        //
        DEBUG ((DEBUG_WARN, "OCAK: Attempted to patch non-existent kext %a\n", PatchedKext->Identifier));
        
      } else {
        BuiltinKext->PatchKext = TRUE;
        Status = ScanDependencies (Context, PatchedKext->Identifier);
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }

      KextLink = GetNextNode (&Context->PatchedKexts, KextLink);
    }

    //
    // Scan dependencies, adding any others besides ones being injected.
    //
    KextLink = GetFirstNode (&Context->InjectedDependencies);
    while (!IsNull (&Context->InjectedDependencies, KextLink)) {
      DependKext = GET_DEPEND_KEXT_FROM_LINK (KextLink);

      Status = ScanDependencies (Context, DependKext->Identifier);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      KextLink = GetNextNode (&Context->InjectedDependencies, KextLink);
    }

    Context->BuiltInKextsValid = TRUE;
    DEBUG ((DEBUG_INFO, "OCAK: Built-in kext cache successfully built\n"));
  }

  //
  // Try to get Info.plist.
  //
  if (OcUnicodeEndsWith (FileName, L"Info.plist", FALSE)) {
    BuiltinKext = LookupBuiltinKextForPlistPath (Context, FileName);
    if (BuiltinKext != NULL && BuiltinKext->PatchValidOSBundleRequired) {
      DEBUG ((DEBUG_INFO, "OCAK: Processing plist patches for %s\n", FileName));

      //
      // Open Info.plist
      //
      Status = AllocateCopyFileData (File, (UINT8 **) &Buffer, &BufferSize);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      InfoPlistDocument = XmlDocumentParse (Buffer, BufferSize, FALSE);
      if (InfoPlistDocument == NULL) {
        FreePool (Buffer);
        return EFI_INVALID_PARAMETER;
      }

      InfoPlistRoot = PlistNodeCast (PlistDocumentRoot (InfoPlistDocument), PLIST_NODE_TYPE_DICT);
      if (InfoPlistRoot == NULL) {
        XmlDocumentFree (InfoPlistDocument);
        FreePool (Buffer);
        return EFI_INVALID_PARAMETER;
      }

      //
      // If kext is present but invalid, we need to change it.
      // Otherwise add new property.
      //
      if (BuiltinKext->OSBundleRequiredValue == KEXT_OSBUNDLE_REQUIRED_INVALID) {
        FieldCount = PlistDictChildren (InfoPlistRoot);
        for (FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex) {
          TmpKeyValue = PlistKeyValue (PlistDictChild (InfoPlistRoot, FieldIndex, &InfoPlistValue));
          if (TmpKeyValue == NULL) {
            continue;
          }

          if (AsciiStrCmp (TmpKeyValue, INFO_BUNDLE_OS_BUNDLE_REQUIRED_KEY) == 0) {
            XmlNodeChangeContent (InfoPlistValue, OS_BUNDLE_REQUIRED_ROOT);
          }
        }

      } else if (BuiltinKext->OSBundleRequiredValue == KEXT_OSBUNDLE_REQUIRED_NONE) {
        Failed = FALSE;
        Failed |= XmlNodeAppend (InfoPlistRoot, "key", NULL, INFO_BUNDLE_OS_BUNDLE_REQUIRED_KEY) == NULL;
        Failed |= XmlNodeAppend (InfoPlistRoot, "string", NULL, OS_BUNDLE_REQUIRED_ROOT) == NULL;
        if (Failed) {
          XmlDocumentFree (InfoPlistDocument);
          FreePool (Buffer);
          return EFI_OUT_OF_RESOURCES;
        }
      }

      //
      // Export plist.
      //
      NewPlistData = XmlDocumentExport (InfoPlistDocument, &NewPlistDataSize, 0, TRUE);
      if (NewPlistData == NULL) {
        XmlDocumentFree (InfoPlistDocument);
        FreePool (Buffer);
        return EFI_OUT_OF_RESOURCES;
      }

      XmlDocumentFree (InfoPlistDocument);
      FreePool (Buffer);

      //
      // Virtualize newly created Info.plist.
      //
      Status = GetFileModificationTime (File, &ModificationTime);
      if (EFI_ERROR (Status)) {
        ZeroMem (&ModificationTime, sizeof (ModificationTime));
      }

      Status = CreateVirtualFileFileNameCopy (FileName, NewPlistData, NewPlistDataSize, &ModificationTime, VirtualFile);
      if (EFI_ERROR (Status)) {
        *VirtualFile = NULL;
        FreePool (NewPlistData);
      }
      return Status;
    }
  } else {
    //
    // Try to get binary for built-in kext.
    //
    BuiltinKext = LookupBuiltinKextForBinaryPath (Context, FileName);
    if (BuiltinKext != NULL && BuiltinKext->PatchKext) {
      DEBUG ((DEBUG_INFO, "OCAK: Processing binary patches for %s\n", FileName));

      PatchedKext = LookupPatchedKextForIdentifier (Context, BuiltinKext->Identifier);
      if (PatchedKext == NULL) {
        return EFI_INVALID_PARAMETER;
      }

      Status = AllocateCopyFileData (File, (UINT8 **) &Buffer, &BufferSize);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = PatcherInitContextFromBuffer (&Patcher, Buffer, BufferSize, Context->Is32Bit);
      if (EFI_ERROR (Status)) {
        FreePool (Buffer);
        return Status;
      }

      //
      // Apply patches.
      //
      KextLink = GetFirstNode (&PatchedKext->Patches);
      while (!IsNull (&PatchedKext->Patches, KextLink)) {
        KextPatch = GET_KEXT_PATCH_FROM_LINK (KextLink);

        if (KextPatch->ApplyQuirk) {
          Status = KernelApplyQuirk (KextPatch->QuirkName, &Patcher, Context->KernelVersion);
          DEBUG ((
            EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
            "OCAK: Cacheless kernel quirk result for %a (%u) - %r\n",
            PatchedKext->Identifier,
            KextPatch->QuirkName,
            Status
            ));
        } else {
          Status = PatcherApplyGenericPatch (&Patcher, &KextPatch->Patch);
          DEBUG ((
            EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
            "OCAK: Cacheless patcher result for %a (%a) - %r\n",
            PatchedKext->Identifier,
            KextPatch->Patch.Comment,
            Status
            ));
        }

        KextLink = GetNextNode (&PatchedKext->Patches, KextLink);
      }

      //
      // Block kext if requested.
      //
      if (PatchedKext->Block) {
        Status = PatcherBlockKext (&Patcher);
        DEBUG ((
          EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
          "OCAK: Cacheless blocker result for %a (%a) - %r\n",
          PatchedKext->Identifier,
          KextPatch->Patch.Comment,
          Status
          ));
      }

      //
      // Virtualize patched binary.
      //
      Status = GetFileModificationTime (File, &ModificationTime);
      if (EFI_ERROR (Status)) {
        ZeroMem (&ModificationTime, sizeof (ModificationTime));
      }

      Status = CreateVirtualFileFileNameCopy (FileName, Buffer, BufferSize, &ModificationTime, VirtualFile);
      if (EFI_ERROR (Status)) {
        *VirtualFile = NULL;
        FreePool (Buffer);
      }
      return Status;
    }
  }

  //
  // No file hooked.
  //
  *VirtualFile = NULL;

  return EFI_SUCCESS;
}
