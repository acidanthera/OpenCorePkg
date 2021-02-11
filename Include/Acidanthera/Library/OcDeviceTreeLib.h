/** @file
  Copyright (C) 2016, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef OC_DEVICE_TREE_LIB_H
#define OC_DEVICE_TREE_LIB_H

//
// Struct at the beginning of every loaded kext.
// Pointers to every loaded kext (to this struct) are
// properties Driver-<hex addr of DriverInfo> in DevTree /chosen/memory-map.
//
typedef struct DTBooterKextFileInfo_ {
  UINT32  InfoDictPhysAddr;
  UINT32  InfoDictLength;
  UINT32  ExecutablePhysAddr;
  UINT32  ExecutableLength;
  UINT32  BundlePathPhysAddr;
  UINT32  BundlePathLength;
} DTBooterKextFileInfo;

//
// DeviceTree MemoryMap entry structure.
//
typedef struct DTMemMapEntry_ {
  UINT32  Address;
  UINT32  Length;
} DTMemMapEntry;

//
// Foundation Types.
//

#define DT_PATH_NAME_SEPERATOR      '/'    ///< 0x2F

#define DT_MAX_PROPERTY_NAME_LENGTH 31     ///< Max length of Property Name (terminator not included)
#define DT_MAX_ENTRY_NAME_LENGTH    31     ///< Max length of a C-String Entry Name (terminator not included)
#define DT_PROPERTY_NAME_LENGTH     32

typedef CHAR8 DTEntryNameBuf[DT_PROPERTY_NAME_LENGTH];        ///< Length of DTEntryNameBuf = DT_MAX_ENTRY_NAME_LENGTH + 1

typedef struct OpaqueDTEntry_             DeviceTreeNode;
typedef DeviceTreeNode                    *DTEntry;           ///< Entry

typedef struct OpaqueDTProperty_          DTProperty;

typedef struct DTSavedScope_              DTSavedScope;
typedef DTSavedScope                      *DTSavedScopePtr;

typedef struct OpaqueDTEntryIterator_     OpaqueDTEntryIterator;
typedef OpaqueDTEntryIterator             *DTEntryIterator;

typedef struct OpaqueDTPropertyIterator_  OpaqueDTPropertyIterator;
typedef OpaqueDTPropertyIterator          *DTPropertyIterator;

//
// Structures for a Flattened Device Tree.
//

struct OpaqueDTProperty_ {
  CHAR8   Name[DT_PROPERTY_NAME_LENGTH];  ///< NUL terminated property name
  UINT32  Length;                         ///< Length (bytes) of folloing prop value
};

struct OpaqueDTEntry_ {
  UINT32  NumProperties;          // Number of props[] elements (0 => end)
  UINT32  NumChildren;            // Number of children[] elements
};

//
// Entry.
//

struct DTSavedScope_ {
  DTSavedScopePtr  NextScope;
  DTEntry          Scope;
  DTEntry          Entry;
  UINT32           Index;
};

//
// Entry Iterator.
//
struct OpaqueDTEntryIterator_ {
  DTEntry           OuterScope;
  DTEntry           CurrentScope;
  DTEntry           CurrentEntry;
  DTSavedScopePtr   SavedScope;
  UINT32            CurrentIndex;
};

//
// Property Iterator.
//
struct OpaqueDTPropertyIterator_ {
  DTEntry                 Entry;
  DTProperty              *CurrentProperty;
  UINT32                  CurrentIndex;
};


/**
  Lookup Entry
  Locates an entry given a specified subroot (searchPoint) and path name. If the
  searchPoint pointer is NULL, the path name is assumed to be an absolute path
  name rooted to the root of the device tree.
**/
EFI_STATUS
DTLookupEntry (
  IN CONST DTEntry      SearchPoint,
  IN CONST CHAR8        *PathName,
  IN DTEntry            *FoundEntry
  );

/**
  An Entry Iterator maintains three variables that are of interest to clients.
  First is an "OutermostScope" which defines the outer boundry of the iteration.
  This is defined by the starting entry and includes that entry plus all of its
  embedded entries. Second is a "currentScope" which is the entry the iterator is
  currently in. And third is a "currentPosition" which is the last entry returned
  during an iteration.

  Create Entry Iterator
  Create the iterator structure. The outermostScope and currentScope of the iterator
  are set to "startEntry". If "startEntry" = NULL, the outermostScope and
  currentScope are set to the root entry. The currentPosition for the iterator is
  set to "nil".
**/
EFI_STATUS
DTCreateEntryIterator (
  IN CONST DTEntry      StartEntry,
  IN DTEntryIterator    *Iterator
  );

/**
  Dispose Entry Iterator
**/
EFI_STATUS
DTDisposeEntryIterator (
  IN DTEntryIterator    Iterator
  );

/**
  Enter Child Entry
  Move an Entry Iterator into the scope of a specified child entry. The
  currentScope of the iterator is set to the entry specified in "childEntry". If
  "childEntry" is nil, the currentScope is set to the entry specified by the
  currentPosition of the iterator.
**/
EFI_STATUS
DTEnterEntry (
  IN DTEntryIterator    Iterator,
  IN DTEntry            ChildEntry
  );

/**
  Exit to Parent Entry
  Move an Entry Iterator out of the current entry back into the scope of its parent
  entry. The currentPosition of the iterator is reset to the current entry (the
  previous currentScope), so the next iteration call will continue where it left off.
  This position is returned in parameter "currentPosition".
**/
EFI_STATUS
DTExitEntry (
  IN DTEntryIterator    Iterator,
  IN DTEntry            *CurrentPosition
  );

/**
  Iterate Entries
  Iterate and return entries contained within the entry defined by the current
  scope of the iterator. Entries are returned one at a time. When
  int == kIterationDone, all entries have been exhausted, and the
  value of nextEntry will be Nil.
**/
EFI_STATUS
DTIterateEntries (
  IN DTEntryIterator    Iterator,
  IN DTEntry            *NextEntry
  );

/**
  Restart Entry Iteration
  Restart an iteration within the current scope. The iterator is reset such that
  iteration of the contents of the currentScope entry can be restarted. The
  outermostScope and currentScope of the iterator are unchanged. The currentPosition
  for the iterator is set to "nil".
**/
EFI_STATUS
DTRestartEntryIteration (
  IN DTEntryIterator    Iterator
  );

/**
  Get the value of the specified property for the specified entry.
**/
EFI_STATUS
DTGetProperty (
  IN CONST DTEntry      Entry,
  IN CHAR8              *PropertyName,
  IN VOID               **PropertyValue,
  IN UINT32             *PropertySize
  );

/**
  Create Property Iterator
  Create the property iterator structure. The target entry is defined by entry.
**/
EFI_STATUS
DTCreatePropertyIterator (
  IN CONST DTEntry      Entry,
  IN DTPropertyIterator Iterator
  );

/**
  Iterate Properites
  Iterate and return properties for given entry.
  EFI_END_OF_MEDIA, all properties have been exhausted.
**/

EFI_STATUS
DTIterateProperties (
  IN DTPropertyIterator Iterator,
  IN CHAR8              **FoundProperty
  );

/**
  Restart Property Iteration
  Used to re-iterate over a list of properties. The Property Iterator is
  reset to the beginning of the list of properties for an entry.
**/
EFI_STATUS
DTRestartPropertyIteration (
  IN DTPropertyIterator Iterator
  );

//
// Exported Functions
//

/**
  Used to initalize the device tree functions.
  Base is the base address of the flatened device tree.
**/
VOID
DTInit (
  IN VOID               *Base,
  IN UINT32             *Length
  );

VOID
DumpDeviceTree (
  VOID
  );

UINT32
DTDeleteProperty (
  IN CHAR8              *NodeName,
  IN CHAR8              *DeletePropertyName
  );

VOID
DTInsertProperty (
  IN CHAR8              *NodeName,
  IN CHAR8              *InsertPropertyName,
  IN CHAR8              *AddPropertyName,
  IN VOID               *AddPropertyValue,
  IN UINT32             ValueLength,
  IN BOOLEAN            InsertAfter OPTIONAL
  );

#endif // OC_DEVICE_TREE_LIB_H
