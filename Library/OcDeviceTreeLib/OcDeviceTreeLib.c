/** @file
  Copyright (C) 2016 - 2017, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/OcDeviceTreeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

//
// Obtain next property by address.
//
#define DEVICE_TREE_GET_NEXT_PROPERTY(Prop)  \
  (DTProperty *)(((UINT8 *)(UINTN)(Prop))    \
    + sizeof (*Prop) + ALIGN_VALUE (Prop->Length, sizeof (UINT32)))

//
// Location of the Device Tree.
//
STATIC DTEntry mDTRootNode;

//
// Pointer to location that contains the length of the Device Tree.
//
STATIC UINT32  *mDTLength;

STATIC UINT32  mDTNodeDepth;

STATIC OpaqueDTPropertyIterator mOpaquePropIter;

//
// Support Routines.
//

STATIC
DTEntry
DTSkipProperties (
  IN DTEntry        Entry
  )
{
  DTProperty  *Prop;
  UINT32      Count;

  if (Entry == NULL || Entry->NumProperties == 0) {
    return NULL;
  } else {
    Prop = (DTProperty *) (Entry + 1);
    for (Count = 0; Count < Entry->NumProperties; ++Count) {
      Prop = DEVICE_TREE_GET_NEXT_PROPERTY (Prop);
    }
  }

  return (DTEntry) Prop;
}

STATIC
DTEntry
DTSkipTree (
  IN DTEntry        Root
  )
{
  DTEntry   Entry;
  UINT32    Count;

  Entry = DTSkipProperties (Root);

  if (Entry == NULL) {
    return NULL;
  }

  for (Count = 0; Count < Root->NumChildren; ++Count) {
    Entry = DTSkipTree (Entry);
  }
  return Entry;
}

STATIC
DTEntry
GetFirstChild (
  IN DTEntry        Parent
  )
{
  return DTSkipProperties (Parent);
}

STATIC
DTEntry
GetNextChild (
  IN DTEntry        Sibling
  )
{
  return DTSkipTree (Sibling);
}

STATIC
CONST CHAR8 *
GetNextComponent (
  IN CONST CHAR8        *Cp,
  IN CHAR8              *Bp
  )
{
  while (*Cp != 0) {
    if (*Cp == DT_PATH_NAME_SEPERATOR) {
      Cp++;
      break;
    }
    *Bp++ = *Cp++;
  }

  *Bp = 0;
  return Cp;
}

STATIC
DTEntry
FindChild (
  IN DTEntry        Cur,
  IN CHAR8          *Buf
  )
{
  DTEntry     Child;
  UINTN       Index;
  CHAR8       *Str;
  UINT32      Dummy;

  if (Cur->NumChildren == 0) {
    return NULL;
  }

  Index = 1;
  Child = GetFirstChild (Cur);
  while (1) {
    if (EFI_ERROR (DTGetProperty (Child, "name", (VOID **)&Str, &Dummy))) {
      break;
    }

    if (AsciiStrCmp (Str, Buf) == 0) {
      return Child;
    }

    if (Index >= Cur->NumChildren) {
      break;
    }

    Child = GetNextChild (Child);
    ++Index;
  }

  return NULL;
}

//
// External Routines.
//

EFI_STATUS
DTLookupEntry (
  IN CONST DTEntry       SearchPoint,
  IN CONST CHAR8         *PathName,
  IN DTEntry             *FoundEntry
  )
{
  DTEntryNameBuf  Buf;
  DTEntry         Cur;
  CONST CHAR8     *Cp;

  if (mDTRootNode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (SearchPoint == NULL) {
    Cur = mDTRootNode;
  } else {
    Cur = SearchPoint;
  }

  Cp = PathName;
  if (*Cp == DT_PATH_NAME_SEPERATOR) {
    Cp++;
    if (*Cp == '\0') {
      *FoundEntry = Cur;
      return EFI_SUCCESS;
    }
  }

  do {
    Cp = GetNextComponent (Cp, Buf);

    //
    // Check for done.
    //
    if (*Buf == '\0') {
      if (*Cp == '\0') {
        *FoundEntry = Cur;
        return EFI_SUCCESS;
      }
      break;
    }

    Cur = FindChild (Cur, Buf);
  } while (Cur != NULL);

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
DTCreateEntryIterator (
  IN CONST DTEntry       StartEntry,
  IN DTEntryIterator     *Iterator
  )
{
  DTEntryIterator Iter;

  if (mDTRootNode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Iter = AllocatePool (sizeof (OpaqueDTEntryIterator));

  if (Iter == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (StartEntry != NULL) {
    Iter->OuterScope   = (DTEntry) StartEntry;
    Iter->CurrentScope = (DTEntry) StartEntry;
  } else {
    Iter->OuterScope   = mDTRootNode;
    Iter->CurrentScope = mDTRootNode;
  }

  Iter->CurrentEntry = NULL;
  Iter->SavedScope   = NULL;
  Iter->CurrentIndex = 0;

  *Iterator = Iter;
  return EFI_SUCCESS;
}

EFI_STATUS
DTDisposeEntryIterator (
  IN DTEntryIterator     Iterator
  )
{
  DTSavedScopePtr     Scope;

  while ((Scope = Iterator->SavedScope) != NULL) {
    Iterator->SavedScope = Scope->NextScope;
    FreePool (Scope);
  }

  FreePool (Iterator);
  return EFI_SUCCESS;
}

EFI_STATUS
DTEnterEntry (
  IN DTEntryIterator     Iterator,
  IN DTEntry             ChildEntry
  )
{
  DTSavedScopePtr  NewScope;

  if (ChildEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NewScope = AllocatePool (sizeof (DTSavedScope));

  if (NewScope == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewScope->NextScope = Iterator->SavedScope;
  NewScope->Scope     = Iterator->CurrentScope;
  NewScope->Entry     = Iterator->CurrentEntry;
  NewScope->Index     = Iterator->CurrentIndex;

  Iterator->CurrentScope = ChildEntry;
  Iterator->CurrentEntry = NULL;
  Iterator->SavedScope   = NewScope;
  Iterator->CurrentIndex = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
DTExitEntry (
  IN DTEntryIterator     Iterator,
  IN DTEntry             *CurrentPosition
  )
{
  DTSavedScopePtr     NewScope;

  NewScope = Iterator->SavedScope;

  if (NewScope == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Iterator->SavedScope   = NewScope->NextScope;
  Iterator->CurrentScope = NewScope->Scope;
  Iterator->CurrentEntry = NewScope->Entry;
  Iterator->CurrentIndex = NewScope->Index;

  *CurrentPosition = Iterator->CurrentEntry;

  FreePool (NewScope);

  return EFI_SUCCESS;
}

EFI_STATUS
DTIterateEntries (
  IN DTEntryIterator     Iterator,
  IN DTEntry             *NextEntry
  )
{
  if (Iterator->CurrentIndex >= Iterator->CurrentScope->NumChildren) {
    *NextEntry = NULL;
    return EFI_END_OF_MEDIA;
  }

  ++Iterator->CurrentIndex;
  if (Iterator->CurrentIndex == 1) {
    Iterator->CurrentEntry = GetFirstChild (Iterator->CurrentScope);
  } else {
    Iterator->CurrentEntry = GetNextChild (Iterator->CurrentEntry);
  }
  *NextEntry = Iterator->CurrentEntry;
  return EFI_SUCCESS;
}

EFI_STATUS
DTRestartEntryIteration (
  IN DTEntryIterator     Iterator
  )
{
  Iterator->CurrentEntry = NULL;
  Iterator->CurrentIndex = 0;
  return EFI_SUCCESS;
}

EFI_STATUS
DTGetProperty (
  IN CONST DTEntry       Entry,
  IN CHAR8               *PropertyName,
  IN VOID                **PropertyValue,
  IN UINT32              *PropertySize
  )
{
  DTProperty  *Prop;
  UINT32      Count;

  if (Entry == NULL || Entry->NumProperties == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Prop = (DTProperty *) (Entry + 1);
  for (Count = 0; Count < Entry->NumProperties; Count++) {
    if (AsciiStrCmp (Prop->Name, PropertyName) == 0) {
      *PropertyValue = (VOID *) (((UINT8 *)Prop) + sizeof (DTProperty));
      *PropertySize = Prop->Length;
      return EFI_SUCCESS;
    }
    Prop = DEVICE_TREE_GET_NEXT_PROPERTY (Prop);
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
DTCreatePropertyIterator (
  IN CONST DTEntry       Entry,
  IN DTPropertyIterator  Iterator
  )
{
  Iterator->Entry           = Entry;
  Iterator->CurrentProperty = NULL;
  Iterator->CurrentIndex    = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
DTIterateProperties (
  IN DTPropertyIterator  Iterator,
  IN CHAR8               **FoundProperty
  )
{
  if (Iterator->CurrentIndex >= Iterator->Entry->NumProperties) {
    *FoundProperty = NULL;
    return EFI_END_OF_MEDIA;
  }

  Iterator->CurrentIndex++;
  if (Iterator->CurrentIndex == 1) {
    Iterator->CurrentProperty = (DTProperty *) (Iterator->Entry + 1);
  } else {
    Iterator->CurrentProperty = DEVICE_TREE_GET_NEXT_PROPERTY (Iterator->CurrentProperty);
  }
  *FoundProperty = Iterator->CurrentProperty->Name;

  return EFI_SUCCESS;
}

EFI_STATUS
DTRestartPropertyIteration (
  IN DTPropertyIterator Iterator
  )
{
  Iterator->CurrentProperty = NULL;
  Iterator->CurrentIndex    = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
DumpDeviceTreeNodeRecusively (
  IN DTEntry            Entry
  )
{
  EFI_STATUS  Status;
  DTEntry     Root;
  UINTN       Spacer;

  DTEntryIterator      EntryIterator;
  DTPropertyIterator   PropIter;
  DTMemMapEntry        *MemMap;
  DTBooterKextFileInfo *Kext;
  UINT8                *Address;

  CHAR8 *PropertyParent = NULL;
  CHAR8 *PropertyName   = NULL;
  CHAR8 *PropertyValue  = NULL;

  UINT32 PropertySize = 0;

  PropIter = &mOpaquePropIter;

  if (Entry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Root   = Entry;
  Spacer = 1;

  Status = DTCreatePropertyIterator (Entry, PropIter);

  if (!EFI_ERROR (Status)) {
    PropertyParent = "/";

    while ((Status = DTIterateProperties (PropIter, &PropertyName)) == EFI_SUCCESS) {
      if ((Status = DTGetProperty (Entry, (CHAR8 *)PropertyName, (void *)&PropertyValue, &PropertySize)) != EFI_SUCCESS) {
        DEBUG ((DEBUG_WARN, "DeviceTree is probably invalid - %r\n", Status));
        break;
      }

      if (AsciiStrnCmp (PropertyName, "name", 4) == 0) {
        DEBUG ((DEBUG_INFO, "+%*ao %a\n", mDTNodeDepth * Spacer, "-", PropertyValue, mDTNodeDepth));
        DEBUG ((DEBUG_INFO, "|%*a\n", mDTNodeDepth * Spacer, " {" , mDTNodeDepth));
        PropertyParent = PropertyValue;
        mDTNodeDepth++;
      } else if (AsciiStrnCmp (PropertyName, "guid", 4) == 0) {
        //
        // Show Guid.
        //
        DEBUG ((DEBUG_INFO, "|%*a \"%a\" = < %g >\n", mDTNodeDepth * Spacer, " ", PropertyName, PropertyValue, PropertySize));
      } else if (AsciiStrnCmp (PropertyParent, "memory-map", 10) == 0) {
        MemMap = (DTMemMapEntry *)PropertyValue;

        if (AsciiStrnCmp (PropertyName, "Driver-", 7) == 0) {
          Kext = (DTBooterKextFileInfo *)(UINTN)MemMap->Address;

          if (Kext != NULL && Kext->ExecutablePhysAddr != 0) {
            DEBUG ((
              DEBUG_INFO,
              "|%*a \"%a\" = < Dict 0x%0X Binary 0x%0X \"%a\" >\n", mDTNodeDepth * Spacer, " ",
              PropertyName,
              Kext->InfoDictPhysAddr,
              Kext->ExecutablePhysAddr,
              Kext->BundlePathPhysAddr
              ));
          }
        } else {
          Address = (UINT8 *)(UINTN)MemMap->Address;
          if (Address != NULL) {
            DEBUG ((DEBUG_INFO, "|%*a \"%a\" = < 0x%0X %02X %02X %02X %02X Length %X >\n", mDTNodeDepth * Spacer, " ",
              PropertyName,
              MemMap->Address,
              Address[0], Address[1], Address[2], Address[3],
              MemMap->Length
              ));
          }
        }

      } else {
        //
        // TODO: Print data here.
        //
        DEBUG ((DEBUG_INFO, "|%*a \"%a\" = < ... > (%d)\n", mDTNodeDepth * Spacer, " ",
            PropertyName,
            PropertySize));
      }
    }

    DEBUG ((DEBUG_INFO, "|%*a\n", (mDTNodeDepth - 1 ) * Spacer, " }" , mDTNodeDepth));
  }

  Status = DTCreateEntryIterator (Root, &EntryIterator);

  if (!EFI_ERROR (Status)) {
    while (!EFI_ERROR(DTIterateEntries (EntryIterator, &Root))) {
      DumpDeviceTreeNodeRecusively (Root);
      mDTNodeDepth--;
    }
  }

  return !EFI_ERROR (Status) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

VOID
DumpDeviceTree (
  VOID
  )
{
  DTEntry DTRoot = NULL;

  if (!EFI_ERROR (DTLookupEntry (NULL, "/", &DTRoot))) {
    DumpDeviceTreeNodeRecusively (DTRoot);
  }
}

// DTInit
///
///
/// @param[in] Base      Pointer to the Device Tree
/// @param[in] Length    Pointer to location containg the Device Tree length
///

VOID
DTInit (
  IN VOID               *Base,
  IN UINT32             *Length
  )
{
  if (Base != NULL && Length != NULL) {
    mDTRootNode    = (DTEntry) Base;
    mDTLength      = Length;
  }
}

// DTDeleteProperty
///
///
/// @param[in] NodeName
/// @param[in] DeletePropertyName
///

UINT32
DTDeleteProperty (
  IN CHAR8              *NodeName,
  IN CHAR8              *DeletePropertyName
  )
{
  DTEntry             Node;
  DTPropertyIterator  PropIter;
  DTProperty          *Property;
  CHAR8               *DeletePosition;
  CHAR8               *DeviceTreeEnd;
  UINT32              DeleteLength;

  ASSERT (mDTLength != NULL);

  PropIter       = &mOpaquePropIter;
  DeletePosition = NULL;
  DeviceTreeEnd  = (CHAR8 *) mDTRootNode + *mDTLength;
  DeleteLength   = 0;

  if (!EFI_ERROR (DTLookupEntry (NULL, NodeName, &Node))) {
    if (!EFI_ERROR (DTCreatePropertyIterator (Node, PropIter))) {
      while (!EFI_ERROR (DTIterateProperties (PropIter, &DeletePosition))) {
        if (AsciiStrStr (DeletePosition, DeletePropertyName) != NULL) {
          Property     = (DTProperty *)DeletePosition;
          DeleteLength = sizeof (DTProperty) + ALIGN_VALUE (Property->Length, sizeof (UINT32));

          //
          // Adjust Device Tree Length.
          //
          *mDTLength -= DeleteLength;

          //
          // Delete Property.
          //
          CopyMem (DeletePosition, DeletePosition + DeleteLength, DeviceTreeEnd - DeletePosition);
          ZeroMem (DeviceTreeEnd - DeleteLength, DeleteLength);

          //
          // Decrement Nodes Properties Count.
          //
          Node->NumProperties--;

          break;
        }
      }
    }
  }

  return DeleteLength;
}

// DTInsertProperty
///
///
/// @param[in] NodeName
/// @param[in] InsertPropertyName
/// @param[in] AddPropertyName
/// @param[in] AddPropertyValue
/// @param[in] ValueLength
/// @param[in] InsertAfter
///

VOID
DTInsertProperty (
  IN CHAR8              *NodeName,
  IN CHAR8              *InsertPropertyName,
  IN CHAR8              *AddPropertyName,
  IN VOID               *AddPropertyValue,
  IN UINT32             ValueLength,
  IN BOOLEAN            InsertAfter
  )
{
  DTEntry             Node;
  DTPropertyIterator  PropIter;
  DTProperty          *Property;
  UINT32              EntryLength;
  CHAR8               *DeviceTree;
  CHAR8               *DeviceTreeEnd;
  CHAR8               *InsertPosition;

  ASSERT (mDTLength != NULL);

  PropIter       = &mOpaquePropIter;
  EntryLength    = ALIGN_VALUE (ValueLength, sizeof (UINT32));
  DeviceTree     = NULL;
  DeviceTreeEnd  = (CHAR8 *)mDTRootNode + *mDTLength;
  InsertPosition = NULL;

  if (!EFI_ERROR (DTLookupEntry (NULL, NodeName, &Node))) {
    if (!EFI_ERROR (DTCreatePropertyIterator (Node, PropIter))) {
      while (!EFI_ERROR (DTIterateProperties (PropIter, &DeviceTree))) {
        InsertPosition = DeviceTree;
        if (AsciiStrStr (InsertPosition, InsertPropertyName) != NULL) {
          break;
        }
      }

      if (InsertAfter) {
        Property        = (DTProperty *) InsertPosition;
        InsertPosition += sizeof (DTProperty) + ALIGN_VALUE (Property->Length, sizeof (UINT32));
      }

      Property = (DTProperty *)InsertPosition;

      //
      // Make space.
      //
      CopyMem (InsertPosition + sizeof (DTProperty) + EntryLength, InsertPosition, DeviceTreeEnd - InsertPosition);
      ZeroMem (InsertPosition, sizeof (DTProperty) + EntryLength);

      //
      // Insert Property Name.
      //
      CopyMem (Property->Name, AddPropertyName, AsciiStrLen (AddPropertyName));

      //
      // Insert Property Value Length.
      //
      Property->Length = ValueLength;

      //
      // Insert Property Value.
      //
      CopyMem (InsertPosition + sizeof (DTProperty), AddPropertyValue, ValueLength);

      //
      // Increment Nodes Properties Count.
      //
      Node->NumProperties++;

      //
      // Adjust Length.
      //
      *mDTLength += sizeof (DTProperty) + EntryLength;
    }
  }
}
