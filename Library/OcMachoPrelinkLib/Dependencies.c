#include <Base.h>

#include <IndustryStandard/AppleMachoImage.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcGuardLib.h>
#include <Library/OcMachoLib.h>
#include <Library/OcStringLib.h>

#include "OcMachoPrelinkInternal.h"

/**
  Fills SymbolTable with the symbols provided in Symbols.  For performance
  reasons, the C++ symbols are continuously added to the top of the buffer.
  Their order is not preserved.  SymbolTable->SymbolTable is expected to be
  a valid buffer that can store at least NumSymbols symbols.

  @param[in]     MachoContext  Context of the Mach-O.
  @param[in]     NumSymbols    The number of symbols to copy.
  @param[in]     Symbols       The source symbol array.
  @param[in,out] SymbolTable   The desination Symbol List.  Must be able to
                               hold at least NumSymbols symbols.

**/
VOID
InternalFillSymbolTable64 (
  IN OUT OC_MACHO_CONTEXT     *MachoContext,
  IN     UINT32               NumSymbols,
  IN     CONST MACH_NLIST_64  *Symbols,
  IN OUT OC_SYMBOL_TABLE_64   *SymbolTable
  )
{
  OC_SYMBOL_64        *WalkerBottom;
  OC_SYMBOL_64        *WalkerTop;
  UINT32              NumCxxSymbols;
  UINT32              Index;
  CONST MACH_NLIST_64 *Symbol;
  CONST CHAR8         *Name;
  BOOLEAN             Result;

  ASSERT (MachoContext != NULL);
  ASSERT (Symbols != NULL);
  ASSERT (SymbolTable != NULL);

  WalkerBottom = &SymbolTable->Symbols[0];
  WalkerTop    = &SymbolTable->Symbols[SymbolTable->NumSymbols - 1];

  NumCxxSymbols = 0;

  for (Index = 0; Index < NumSymbols; ++Index) {
    Symbol = &Symbols[Index];
    Name   = MachoGetSymbolName64 (MachoContext, Symbol);
    Result = MachoSymbolNameIsCxx (Name);

    if (!Result) {
      WalkerBottom->StringIndex = Symbol->UnifiedName.StringIndex;
      WalkerBottom->Value       = Symbol->Value;
      ++WalkerBottom;
    } else {
      WalkerTop->StringIndex = Symbol->UnifiedName.StringIndex;
      WalkerTop->Value       = Symbol->Value;
      --WalkerTop;

      ++NumCxxSymbols;
    }
  }

  SymbolTable->NumSymbols    = NumSymbols;
  SymbolTable->NumCxxSymbols = NumCxxSymbols;
}

STATIC
INTN
EFIAPI
AsciiKextNameCmp (
  IN CONST CHAR8  *FirstString,
  IN CONST CHAR8  *SecondString
  )
{
  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (FirstString));
  ASSERT (AsciiStrSize (SecondString));

  while ((*FirstString != '<') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }

  if ((*FirstString == '<') && (*SecondString == '0')) {
    return 0;
  }

  return (*FirstString - *SecondString);
}

STATIC
CONST CHAR8 *
InternalFindPrelinkedKextPlist (
  IN CONST CHAR8  *PrelinkedPlist,
  IN CONST CHAR8  *BundleId
  )
{
  CONST CHAR8 *Walker;
  CONST CHAR8 *Walker2;
  CONST CHAR8 *BundleIdKey;
  INTN        Result;
  UINTN       DictLevel;
  //
  // Iterate through all OS Bundle IDs.
  //
  BundleIdKey = "<key>" OS_BUNDLE_IDENTIFIER_STR "<";

  for (
    Walker = AsciiStrStr (PrelinkedPlist, BundleIdKey);
    Walker != NULL;
    Walker = AsciiStrStr (Walker, BundleIdKey)
    ) {
    Walker2 = AsciiStrStr (Walker, "<string>");
    ASSERT (Walker2 != NULL);
    Walker2 += L_STR_LEN ("<string>");

    Result = AsciiKextNameCmp (Walker2, BundleId);
    if (Result == 0) {
      //
      // The OS Bundle ID is in the first level of the KEXT PLIST dictionary.
      //
      DictLevel = 1;

      for (Walker -= L_STR_LEN ("<dict>"); TRUE; --Walker) {
        ASSERT (Walker >= PrelinkedPlist);
        //
        // Find the KEXT PLIST dictionary entry.
        //
        Result = AsciiStrnCmp (Walker, STR_N_HELPER ("</dict>"));
        if (Result == 0) {
          ++DictLevel;
          Walker -= (L_STR_LEN ("<dict>") - 1);
          continue;
        }

        Result = AsciiStrnCmp (Walker, STR_N_HELPER ("<dict>"));
        if (Result == 0) {
          --DictLevel;

          if (DictLevel == 0) {
            //
            // Return the content within the KEXT PLIST dictionary.
            //
            return (Walker + L_STR_LEN ("<dict>"));
          }

          Walker -= (L_STR_LEN ("<dict>") - 1);
          continue;
        }
      }
    }

    Walker = Walker2;
  }

  return NULL;
}

STATIC
CONST CHAR8 *
InternalKextCollectDependencies (
  IN  CONST CHAR8  **BundleLibrariesStr,
  OUT UINTN        *NumberOfDependencies
  )
{
  CONST CHAR8 *Dependencies;
  UINTN       NumDependencies;

  CONST CHAR8 *Walker;
  CHAR8       *DictEnd;

  Dependencies    = NULL;
  NumDependencies = 0;
  DictEnd         = NULL;

  Walker  = *BundleLibrariesStr;
  Walker += L_STR_LEN (OS_BUNDLE_LIBRARIES_STR "</key>");
  //
  // Retrieve the opening of the OSBundleLibraries key.
  //
  Walker = AsciiStrStr (Walker, "<dict");
  ASSERT (Walker != NULL);
  Walker += L_STR_LEN ("<dict");
  //
  // Verify the dict is not an empty single tag.
  //
  while ((*Walker == ' ') || (*Walker == '\t')) {
    ++Walker;
  }
  if (*Walker != '/') {
    ASSERT (*Walker == '>');
    ++Walker;
    //
    // Locate the closure of the OSBundleLibraries dict.  Dicts inside
    // OSBundleLibraries are not valid, so searching for the next match is
    // fine.
    //
    DictEnd = AsciiStrStr (Walker, "</dict>");
    ASSERT (DictEnd != NULL);
    //
    // Temporarily terminate the PLIST string at the beginning of the
    // OSBundleLibraries closing tag so that AsciiStrStr() calls do not exceed
    // its bounds.
    //
    DictEnd[0] = '\0';
    //
    // Cache the result to retrieve the dependencies from the prelinked PLIST
    // later.
    //
    Dependencies = AsciiStrStr (Walker, "<key>");
    ASSERT (Dependencies != NULL);

    for (
      Walker = Dependencies;
      Walker != NULL;
      Walker = AsciiStrStr (Walker, "<key>")
      ) {
      ++NumDependencies;
      //
      // A dependency must have a non-empty name to be valid.
      // A valid string-value for the version identifies is mandatory.
      //
      Walker += L_STR_LEN ("<key>x</key><string>x</string>");
    }
    //
    // Restore the previously replaced opening brace.
    //
    DictEnd[0] = '<';
  }

  if (DictEnd != NULL) {
    *BundleLibrariesStr = (DictEnd + L_STR_LEN ("</dict>"));
  } else {
    *BundleLibrariesStr = Walker;
  }

  *NumberOfDependencies = NumDependencies;

  return Dependencies;
}

STATIC
UINT16
InternalStringToKextVersionWorker (
  IN CONST CHAR8  **VersionString
  )
{
  UINT16      Version;

  CONST CHAR8 *VersionStart;
  CHAR8       *VersionEnd;
  CONST CHAR8 *VersionReturn;

  VersionReturn = NULL;

  VersionStart = *VersionString;
  VersionEnd   = AsciiStrStr (VersionStart, ".");

  if (VersionEnd != NULL) {
    *VersionEnd = '\0';
  }

  Version = (UINT16)AsciiStrDecimalToUintn (VersionStart);

  if (VersionEnd != NULL) {
    *VersionEnd   = '.';
    VersionReturn = (VersionEnd + 1);
    ASSERT (*VersionReturn != '\0');
  }

  *VersionString = VersionReturn;

  return Version;
}

STATIC
UINT64
InternalStringToKextVersion (
  IN CONST CHAR8  *VersionString
  )
{
  OC_KEXT_VERSION KextVersion;
  CONST CHAR8     *Walker;

  KextVersion.Value = 0;

  Walker = VersionString;

  KextVersion.Bits.Major = InternalStringToKextVersionWorker (&Walker);
  ASSERT (KextVersion.Bits.Major != 0);
  ASSERT (Walker != NULL);

  KextVersion.Bits.Minor = InternalStringToKextVersionWorker (&Walker);
  ASSERT (KextVersion.Bits.Minor != 0);

  if (Walker == NULL) {
    return KextVersion.Value;
  }

  KextVersion.Bits.Revision = InternalStringToKextVersionWorker (&Walker);
  ASSERT (KextVersion.Bits.Revision != 0);

  if (Walker == NULL) {
    return KextVersion.Value;
  }

  switch (Walker[0]) {
    case 'd':
    {
      KextVersion.Bits.Stage = OcKextVersionStageDevelopment;
      break;
    }

    case 'a':
    {
      KextVersion.Bits.Stage = OcKextVersionStageAlpha;
      break;
    }

    case 'b':
    {
      KextVersion.Bits.Stage = OcKextVersionStageBeta;
      break;
    }

    case 'f':
    {
      KextVersion.Bits.Stage = OcKextVersionStageCandidate;

      if (Walker[1] == 'c') {
        ++Walker;
      }
      ASSERT ((Walker[1] >= '0') && (Walker[1] <= '9'));

      break;
    }

    default:
    {
      return 0;
    }
  }

  ++Walker;

  KextVersion.Bits.StageLevel = (UINT16)AsciiStrDecimalToUintn (Walker);
  ASSERT (KextVersion.Bits.StageLevel != 0);
  
  return KextVersion.Value;
}

VOID
InternalFreeDependencyEntry (
  IN OC_DEPENDENCY_INFO_ENTRY  *Entry
  )
{
  if (Entry->Data.SymbolTable != NULL) {
    FreePool (Entry->Data.SymbolTable);
  }

  if (Entry->Data.Vtables != NULL) {
    FreePool (Entry->Data.Vtables);
  }

  RemoveEntryList (&Entry->Link);

  FreePool (Entry);
}

LIST_ENTRY *
InternalRemoveDependency (
  IN     CONST LIST_ENTRY          *Dependencies,
  IN     UINTN                     NumRequests,
  IN OUT OC_KEXT_REQUEST           *Requests,
  IN     OC_DEPENDENCY_INFO_ENTRY  *DependencyInfo
  )
{
  UINTN                    Index;
  UINTN                    Index2;
  OC_KEXT_REQUEST          *Request;
  OC_DEPENDENCY_INFO_ENTRY *Entry;
  LIST_ENTRY               *DependencyEntry;
  OC_DEPENDENCY_INFO_ENTRY *DepWalker;
  LIST_ENTRY               *PreviousEntry;
  BOOLEAN                  IsDependency;

  IsDependency = FALSE;
  //
  // Invalidate dependencies depending on this KEXT.
  //
  for (
    DependencyEntry = GetFirstNode (Dependencies);
    !IsNull (DependencyEntry, Dependencies);
    DependencyEntry = GetNextNode (Dependencies, DependencyEntry)
    ) {
    DepWalker = OC_DEP_INFO_FROM_LINK (DependencyEntry);
    if (DepWalker == DependencyInfo) {
      IsDependency = TRUE;
      continue;
    }

    for (Index = 0; Index < DepWalker->Info.NumDependencies; ++Index) {
      Entry = DepWalker->Info.Dependencies[Index];
      if (Entry == DependencyInfo) {
        DependencyEntry = InternalRemoveDependency (
                            Dependencies,
                            NumRequests,
                            Requests,
                            DepWalker
                            );
        break;
      }
    }
  }
  //
  // Invalidate KEXT Requests depending on this KEXT.
  //
  for (Index = 0; Index < NumRequests; ++Index) {
    Request = &Requests[Index];
    Entry   = Request->Private.Info;

    if (Entry == DependencyInfo) {
      continue;
    }

    for (Index2 = 0; Index2 < Entry->Info.NumDependencies; ++Index2) {
      if (Entry->Info.Dependencies[Index2] == DependencyInfo) {
        InternalInvalidateKextRequest (
          Dependencies,
          NumRequests,
          Requests,
          Request
          );
        break;
      }
    }
  }
  //
  // Prevent future linkage against this dependency.
  //
  if (IsDependency) {
    //
    // Return the dependency previous to this one, so that loops on the
    // dependency list continue uninterrupted.
    //
    PreviousEntry = GetPreviousNode (Dependencies, &DependencyInfo->Link);
    InternalFreeDependencyEntry (DependencyInfo);
    return PreviousEntry;
  }

  return NULL;
}

VOID
InternalInvalidateKextRequest (
  IN     CONST LIST_ENTRY  *Dependencies,
  IN     UINTN             NumRequests,
  IN OUT OC_KEXT_REQUEST   *Requests,
  IN     OC_KEXT_REQUEST   *Request
  )
{
  OC_DEPENDENCY_INFO_ENTRY *DependencyInfo;

  DependencyInfo = Request->Private.Info;

  InternalRemoveDependency (
    Dependencies,
    NumRequests,
    Requests,
    DependencyInfo
    );

  FreePool (DependencyInfo);
  Request->Private.Info = NULL;
}

STATIC
BOOLEAN
InternalConstructDependencyArraysWorker (
  IN  UINTN                     NumDependencies,
  IN  OC_DEPENDENCY_INFO_ENTRY  **Dependencies,
  OUT OC_DEPENDENCY_DATA        *DependencyData,
  IN  BOOLEAN                   IsTopLevel
  )
{
  OC_DEPENDENCY_INFO_ENTRY *Dependency;
  OC_DEPENDENCY_INFO       *Info;
  UINTN                    Index;
  BOOLEAN                  Result;

  for (Index = 0; Index < NumDependencies; ++Index) {
    Dependency = Dependencies[Index];
    if (!Dependency->Prelinked) {
      return FALSE;
    }

    Info = &Dependency->Info;

    if (Dependency->Data.SymbolTable->Link.ForwardLink != NULL) {
      //
      // This dependency already has been added to the list and henceforth its
      // dependencies as well.
      //
      continue;
    }

    Result = InternalConstructDependencyArraysWorker (
               Info->NumDependencies,
               Info->Dependencies,
               DependencyData,
               FALSE
               );
    if (!Result) {
      return FALSE;
    }

    InsertHeadList (
      &DependencyData->SymbolTable->Link,
      &Dependency->Data.SymbolTable->Link
      );
    InsertHeadList (
      &DependencyData->Vtables->Link,
      &Dependency->Data.Vtables->Link
      );

    Dependency->Data.SymbolTable->IsIndirect = !IsTopLevel;
  }

  return TRUE;
}

STATIC
OC_DEPENDENCY_INFO_ENTRY *
InternalGetDependencyInfoEntry (
  IN     LIST_ENTRY       *Dependencies,
  IN     UINTN            NumRequests, OPTIONAL
  IN OUT OC_KEXT_REQUEST  *Requests, OPTIONAL
  IN     CONST CHAR8      *PrelinkedPlist,
  IN     CONST CHAR8      *DependencyString,
  IN     UINT64           KextsVirtual,
  IN     UINTN            KextsPhysical
  )
{
  UINTN                    Index;
  LIST_ENTRY               *DependencyEntry;
  OC_DEPENDENCY_INFO_ENTRY *DepWalker;
  CONST CHAR8              *Name;
  CONST CHAR8              *Version;
  CHAR8                    *VersionEnd;
  UINT64                   KextVersion;
  CONST CHAR8              *KextPlist;
  BOOLEAN                  Result;

  ASSERT (Dependencies != NULL);
  ASSERT (PrelinkedPlist != NULL);
  ASSERT (DependencyString != NULL);
  ASSERT (KextsVirtual != 0);
  ASSERT (KextsPhysical != 0);

  if (NumRequests > 0) {
    ASSERT (Requests != NULL);
  }
  //
  // Extract the information.
  //
  Name = (DependencyString + L_STR_LEN ("<key>"));

  Version = AsciiStrStr ((Name + L_STR_LEN ("x</key>")), "<integer");
  Version = (AsciiStrStr (Version, ">") + L_STR_LEN (">"));

  VersionEnd  = AsciiStrStr (Version, "<");
  *VersionEnd = '\0';

  KextVersion = InternalStringToKextVersion (Version);

  *VersionEnd = '<';
  //
  // Find information among the already processed prelinked KEXTs.
  //
  for (
    DependencyEntry = GetFirstNode (Dependencies);
    !IsNull (DependencyEntry, Dependencies);
    DependencyEntry = GetNextNode (Dependencies, DependencyEntry)
    ) {
    DepWalker = OC_DEP_INFO_FROM_LINK (DependencyEntry);
    if (AsciiKextNameCmp (Name, DepWalker->Info.Name) == 0) {
      if ((KextVersion < DepWalker->Info.CompatibleVersion.Value)
       || (KextVersion > DepWalker->Info.Version.Value)) {
        return NULL;
      }

      return DepWalker;
    }
  }
  //
  // Find information among the KEXTs to prelink.
  //
  for (Index = 0; Index < NumRequests; ++Index) {
    DepWalker = Requests[Index].Private.Info;
    if ((DepWalker != NULL)
     && (AsciiKextNameCmp (Name, DepWalker->Info.Name) == 0)) {
      if ((KextVersion < DepWalker->Info.CompatibleVersion.Value)
       || (KextVersion > DepWalker->Info.Version.Value)) {
        return NULL;
      }

      Requests[Index].Private.IsDependedOn = TRUE;
      return DepWalker;
    }
  }
  //
  // Find information among the remaining prelinked KEXTs.
  //
  KextPlist = InternalFindPrelinkedKextPlist (PrelinkedPlist, Name);
  if (KextPlist == NULL) {
    return NULL;
  }

  DepWalker = InternalKextCollectInformation (
                KextPlist,
                NULL,
                KextsVirtual,
                KextsPhysical,
                KextVersion
                );
  if (DepWalker != NULL) {
    Result = InternalResolveDependencies (
               Dependencies,
               0,
               NULL,
               PrelinkedPlist,
               DepWalker,
               KextsVirtual,
               KextsPhysical
               );
    if (!Result) {
      FreePool (DepWalker);
      return NULL;
    }

    DepWalker->Prelinked = TRUE;
    InsertTailList (Dependencies, &DepWalker->Link);
    return DepWalker;
  }

  return NULL;
}

STATIC
VOID
InternalDestructDependencyArraysWorker (
  OUT LIST_ENTRY  *ListEntry
  )
{
  do {
    //
    // Only clear ForwardLink as BackLink is not explicitly checked.
    //
    ListEntry->ForwardLink = NULL;
    ListEntry = ListEntry->BackLink;
  } while (ListEntry->ForwardLink != NULL);
}

OC_DEPENDENCY_INFO_ENTRY *
InternalKextCollectInformation (
  IN     CONST CHAR8       *Plist,
  IN OUT OC_MACHO_CONTEXT  *MachoContext, OPTIONAL
  IN     UINT64            KextsVirtual, OPTIONAL
  IN     UINTN             KextsPhysical, OPTIONAL
  IN     UINT64            RequestedVersion  OPTIONAL
  )
{
  OC_DEPENDENCY_INFO       DependencyInfoHdr;
  CONST CHAR8              *Dependencies;

  MACH_HEADER_64           *MachHeader;
  UINT32                   MachoSize;

  OC_DEPENDENCY_INFO_ENTRY *DependencyEntry;
  OC_DEPENDENCY_INFO       *DependencyInfo;

  CONST CHAR8              *Walker;
  INTN                     Result;

  UINT64                   Value;

  ASSERT (Plist != NULL);
  ASSERT (MachoContext != NULL);

  ZeroMem (&DependencyInfoHdr, sizeof (DependencyInfoHdr));
  Dependencies = NULL;

  if (MachoContext != NULL) {
    MachHeader = MachoGetMachHeader64 (MachoContext);
    ASSERT (MachHeader != NULL);

    MachoSize = MachoGetFileSize (MachoContext);
    ASSERT (MachoSize != 0);

    DependencyInfoHdr.MachHeader = MachHeader;
    DependencyInfoHdr.MachoSize  = MachoSize;
  }
  //
  // Skip to past the enclosing <dict>.
  //
  Plist = (AsciiStrStr (Plist, "<dict>") + L_STR_LEN ("<dict>"));

  for (
    Walker = InternalPlistStrStrSameLevelDown (Plist, STR_N_HELPER ("<key>"));
    Walker != NULL;
    Walker = InternalPlistStrStrSameLevelDown (Walker, STR_N_HELPER ("<key>"))
    ) {
    if ((DependencyInfoHdr.MachHeader != NULL)
     && (DependencyInfoHdr.MachoSize != 0)
     && (DependencyInfoHdr.Name != NULL)
     && (DependencyInfoHdr.Version.Value != 0)
     && (DependencyInfoHdr.CompatibleVersion.Value != 0)
     && (Dependencies != NULL)
      ) {
      DependencyEntry = AllocatePool (
                          sizeof (*DependencyEntry)
                            + (DependencyInfoHdr.NumDependencies
                                * sizeof (*DependencyInfo->Dependencies))
                          );
      if (DependencyEntry != NULL) {
        DependencyEntry->Signature = OC_DEPENDENCY_INFO_ENTRY_SIGNATURE;
        DependencyEntry->Link.ForwardLink = NULL;
        CopyMem (
          &DependencyEntry->Info,
          &DependencyInfoHdr,
          sizeof (DependencyEntry->Info)
          );
        DependencyEntry->Prelinked        = FALSE;
        DependencyEntry->Data.SymbolTable = NULL;
        DependencyEntry->Data.Vtables     = NULL;
        DependencyInfo = &DependencyEntry->Info;
        //
        // Temporarily store the Dependency strings there until resolution.
        //
        DependencyInfo->Dependencies[0] = (OC_DEPENDENCY_INFO_ENTRY *)(
                                            Dependencies
                                            );
        //
        // Zero the other dependency values so they do not happen to equal to
        // actual dependency info addresses.
        //
        ZeroMem (
          &DependencyInfo->Dependencies[1],
          ((DependencyInfoHdr.NumDependencies - 1)
            * sizeof (*DependencyInfo->Dependencies))
          );
      }

      return DependencyEntry;
    }

    Walker += L_STR_LEN ("<key>");
    //
    // Retrieve the KEXT's name.
    //
    if (DependencyInfoHdr.Name == NULL) {
      Result = AsciiStrnCmp (
                 Walker,
                 STR_N_HELPER (OS_BUNDLE_IDENTIFIER_STR "<")
                 );
      if (Result == 0) {
        Walker += L_STR_LEN (OS_BUNDLE_IDENTIFIER_STR "</key>");
        Walker  = AsciiStrStr (Walker, "<string>");
        ASSERT (Walker != NULL);
        Walker += L_STR_LEN ("<string>");

        DependencyInfoHdr.Name = Walker;

        Walker += L_STR_LEN ("x</string>");
        continue;
      }
    }
    //
    // Retrieve the KEXT's version.
    //
    if (DependencyInfoHdr.Version.Value == 0) {
      Result = AsciiStrnCmp (
                 Walker,
                 STR_N_HELPER (OS_BUNDLE_COMPATIBLE_VERSION_STR "<")
                 );
      if (Result == 0) {
        Walker += L_STR_LEN (OS_BUNDLE_COMPATIBLE_VERSION_STR "</key>");
        Walker  = AsciiStrStr (Walker, "<string>");
        ASSERT (Walker != NULL);
        Walker += L_STR_LEN ("<string>");

        DependencyInfoHdr.Version.Value = InternalStringToKextVersion (Walker);
        if ((DependencyInfoHdr.Version.Value == 0)
         || ((RequestedVersion != 0)
           && (RequestedVersion > DependencyInfoHdr.Version.Value))
          ) {
          return NULL;
        }

        Walker += L_STR_LEN ("x</string>");
        continue;
      }
    }
    //
    // Retrieve the KEXT's compatible version.
    //
    if (DependencyInfoHdr.CompatibleVersion.Value == 0) {
      Result = AsciiStrnCmp (
                 Walker,
                 STR_N_HELPER (OS_BUNDLE_COMPATIBLE_VERSION_STR "<")
                 );
      if (Result == 0) {
        Walker += L_STR_LEN (OS_BUNDLE_COMPATIBLE_VERSION_STR "</key>");
        Walker  = AsciiStrStr (Walker, "<string>");
        ASSERT (Walker != NULL);
        Walker += L_STR_LEN ("<string>");

        DependencyInfoHdr.CompatibleVersion.Value = InternalStringToKextVersion (
                                                      Walker
                                                      );
        if ((DependencyInfoHdr.Version.Value == 0)
         || ((RequestedVersion != 0)
          && (RequestedVersion < DependencyInfoHdr.CompatibleVersion.Value))
          ) {
          return NULL;
        }

        Walker += L_STR_LEN ("x</string>");
        continue;
      }
    }
    //
    // Retrieve the KEXT's dependency information.
    //
    if (Dependencies == NULL) {
      Result = AsciiStrnCmp (
                 Walker,
                 STR_N_HELPER (OS_BUNDLE_LIBRARIES_STR "<")
                 );
      if (Result == 0) {
        Dependencies = InternalKextCollectDependencies (
                         &Walker,
                         &DependencyInfoHdr.NumDependencies
                         );
      }
    }

    if (DependencyInfoHdr.MachHeader == NULL) {
      Result = AsciiStrnCmp (
                 Walker,
                 STR_N_HELPER ("_PrelinkExecutableSourceAddr<")
                 );
      if (Result == 0) {
        Walker += L_STR_LEN ("_PrelinkExecutableSourceAddr</key>");
        Walker  = AsciiStrStr (Walker, "<integer");
        ASSERT (Walker != NULL);
        Value = InternalPlistGetIntValue (&Walker);
        if (Value == 0) {
          return NULL;
        }

        ASSERT (KextsVirtual != 0);
        ASSERT (KextsPhysical != 0);
        ASSERT ((UINTN)Value > KextsVirtual);
        DependencyInfoHdr.MachHeader = (MACH_HEADER_64 *)(
                                         (UINTN)(Value - KextsVirtual)
                                           + KextsPhysical
                                         );
      }
    }

    if (DependencyInfoHdr.MachoSize == 0) {
      Result = AsciiStrnCmp (
                 Walker,
                 STR_N_HELPER ("_PrelinkExecutableSize<")
                 );
      if (Result == 0) {
        Walker += L_STR_LEN ("_PrelinkExecutableSize</key>");
        Walker  = AsciiStrStr (Walker, "<integer");
        ASSERT (Walker != NULL);
        Value = InternalPlistGetIntValue (&Walker);
        if (Value == 0) {
          return NULL;
        }

        DependencyInfoHdr.MachoSize = Value;
      }
    }
  }

  return NULL;
}

BOOLEAN
InternalResolveDependencies (
  IN     LIST_ENTRY                *Dependencies,
  IN     UINTN                     NumRequests, OPTIONAL
  IN OUT OC_KEXT_REQUEST           *Requests, OPTIONAL
  IN     CONST CHAR8               *PrelinkedPlist,
  IN     OC_DEPENDENCY_INFO_ENTRY  *KextInfo,
  IN     UINT64                    KextsVirtual,
  IN     UINTN                     KextsPhysical
  )
{
  CONST CHAR8              *DepWalker;
  UINTN                    Index;
  OC_DEPENDENCY_INFO_ENTRY *DepEntry;

  ASSERT (KextsVirtual != 0);
  ASSERT (KextsPhysical != 0);

  for (
    Index = 0, DepWalker = (CHAR8 *)KextInfo->Info.Dependencies[0];
    Index < KextInfo->Info.NumDependencies;
    ++Index, DepWalker = AsciiStrStr (DepWalker, "<key>")
    ) {
    DepEntry = InternalGetDependencyInfoEntry (
                 Dependencies,
                 NumRequests,
                 Requests,
                 PrelinkedPlist,
                 DepWalker,
                 KextsVirtual,
                 KextsPhysical
                 );
    if (DepEntry == NULL) {
      //
      // Cleaning up cannot be done here as other KEXT Requests might still
      // have the temporary strings as their dependency array and this function
      // has no mean of verifying which do or do't.
      //
      return FALSE;
    }

    KextInfo->Info.Dependencies[Index] = DepEntry;
  }

  return TRUE;
}

UINT64
InternalGetNewPrelinkedKextLoadAddress (
  IN OUT OC_MACHO_CONTEXT               *KernelContext,
  IN     CONST MACH_SEGMENT_COMMAND_64  *PrelinkedKextsSegment,
  IN     CONST CHAR8                    *PrelinkedPlist
  )
{
  UINT64               Address;

  CONST MACH_HEADER_64 *KernelHeader;
  UINTN                KextSize;
  CONST CHAR8          *Key;
  CONST CHAR8          *HighestKey;
  CONST CHAR8          *NextTag;
  CONST CHAR8          *Walker;
  UINT64               TempAddress;
  BOOLEAN              Result;
  OC_MACHO_CONTEXT     KextContext;
  MACH_HEADER_64       *KextHeader;

  Address    = 0;
  NextTag    = NULL;
  HighestKey = NULL;

  for (
    Key = AsciiStrStr (PrelinkedPlist, "<key>_PrelinkExecutableLoadAddr<");
    Key != NULL;
    Key = AsciiStrStr (Walker, "<key>_PrelinkExecutableLoadAddr<")
    ) {
    Walker  = Key;
    Walker += L_STR_LEN ("<key>_PrelinkExecutableLoadAddr</key>");
    Walker  = AsciiStrStr (Walker, "<integer");
    ASSERT (Walker != NULL);

    TempAddress = InternalPlistGetIntValue (&Walker);
    ASSERT (TempAddress != 0);
    ASSERT (TempAddress != MAX_UINT64);

    if (TempAddress > Address) {
      Address    = TempAddress;
      HighestKey = Key;
      NextTag    = Walker;
    }
  }

  if ((Address == 0)
   || (Address < PrelinkedKextsSegment->VirtualAddress)) {
    return FALSE;
  }

  Walker = InternalPlistStrStrSameLevel (
             HighestKey,
             STR_N_HELPER ("<key>_PrelinkExecutableSize<"),
             (NextTag - HighestKey)
             );
  if (Walker == 0) {
    return 0;
  }

  Walker += L_STR_LEN ("<key>_PrelinkExecutableSize</key>");

  KextSize = InternalPlistGetIntValue (&Walker);
  ASSERT (KextSize != 0);
  ASSERT (KextSize != MAX_UINT64);

  KernelHeader = MachoGetMachHeader64 (KernelContext);
  ASSERT (KernelHeader != NULL);

  Result = OcOverflowAddUN (
             (Address - PrelinkedKextsSegment->VirtualAddress),
             ((UINTN)KernelHeader + PrelinkedKextsSegment->FileOffset),
             &Address
             );
  if (!Result) {
    return FALSE;
  }

  KextHeader = (MACH_HEADER_64 *)Address;
  if (!OC_ALIGNED (KextHeader)) {
    return FALSE;
  }
  
  Result = MachoInitializeContext (&KextContext, KextHeader, KextSize);
  if (!Result) {
    return 0;
  }

  return ALIGN_VALUE (MachoGetLastAddress64 (&KextContext), BASE_4KB);
}

VOID
InternalDestructDependencyArrays (
  OUT CONST OC_DEPENDENCY_DATA *DependencyData
  )
{
  InternalDestructDependencyArraysWorker (
    &DependencyData->SymbolTable->Link
    );
  InternalDestructDependencyArraysWorker (
    &DependencyData->Vtables->Link
    );
}

BOOLEAN
InternalConstructDependencyArrays (
  IN  UINTN                     NumDependencies,
  IN  OC_DEPENDENCY_INFO_ENTRY  **Dependencies,
  OUT OC_DEPENDENCY_DATA        *DependencyData
  )
{
  OC_DEPENDENCY_INFO *Info;
  BOOLEAN            Result;

  if (!Dependencies[0]->Prelinked) {
    return FALSE;
  }

  Info = &Dependencies[0]->Info;

  DependencyData->SymbolTable = Dependencies[0]->Data.SymbolTable;
  DependencyData->Vtables     = Dependencies[0]->Data.Vtables;

  InitializeListHead (&DependencyData->SymbolTable->Link);
  InitializeListHead (&DependencyData->Vtables->Link);
  DependencyData->SymbolTable->IsIndirect = FALSE;
  //
  // Process the first dependency's dependencies as it itself has already been
  // processed implicitly above.
  //
  Result = InternalConstructDependencyArraysWorker (
             Info->NumDependencies,
             Info->Dependencies,
             DependencyData,
             FALSE
             );
  if (Result) {
   Result = InternalConstructDependencyArraysWorker (
             (NumDependencies - 1),
             &Dependencies[1],
             DependencyData,
             TRUE
             );
  }

  if (!Result) {
    InternalDestructDependencyArrays (DependencyData);
  }

  return Result;
}
