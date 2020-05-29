/** @file

OcSerializeLib

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/OcSerializeLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

STATIC
CONST CHAR8 *
mSchemaTypeNames[] = {
  [OC_SCHEMA_VALUE_BOOLEAN] = "boolean",
  [OC_SCHEMA_VALUE_INTEGER] = "integer",
  [OC_SCHEMA_VALUE_DATA] = "data",
  [OC_SCHEMA_VALUE_STRING] = "string",
  [OC_SCHEMA_VALUE_MDATA] = "mdata"
};

STATIC
CONST CHAR8 *
GetSchemaTypeName (
  IN UINT32 Type
  )
{
  if (Type < ARRAY_SIZE (mSchemaTypeNames)) {
    return mSchemaTypeNames[Type];
  }
  return "custom";
}

OC_SCHEMA *
LookupConfigSchema (
  IN OC_SCHEMA      *SortedList,
  IN UINT32         Size,
  IN CONST CHAR8    *Name
  )
{
  UINT32  Start;
  UINT32  End;
  UINT32  Curr;
  INTN    Cmp;

  if (Size == 0) {
    return NULL;
  }

  //
  // Classic binary search in a sorted string list.
  //
  Start = 0;
  End   = Size - 1;

  while (Start <= End) {
    Curr = (Start + End) / 2;
    Cmp = AsciiStrCmp (SortedList[Curr].Name, Name);

    if (Cmp == 0) {
      return &SortedList[Curr];
    } else if (Cmp < 0) {
      Start = Curr + 1;
    } else if (Curr > 0) {
      End = Curr - 1;
    } else {
      //
      // Even the first element does not match, required due to unsigned End.
      //
      return NULL;
    }
  }

  return NULL;
}

VOID
ParseSerializedDict (
  OUT VOID            *Serialized,
  IN  XML_NODE        *Node,
  IN  OC_SCHEMA_INFO  *Info,
  IN  CONST CHAR8     *Context  OPTIONAL
  )
{
  UINT32         DictSize;
  UINT32         Index;
  CONST CHAR8    *CurrentKey;
  XML_NODE       *CurrentValue;
  XML_NODE       *OldValue;
  OC_SCHEMA      *NewSchema;

  DictSize = PlistDictChildren (Node);

  for (Index = 0; Index < DictSize; Index++) {
    CurrentKey = PlistKeyValue (PlistDictChild (Node, Index, &CurrentValue));

    if (CurrentKey == NULL) {
      DEBUG ((DEBUG_WARN, "OCS: No serialized key at %u index, context <%a>!\n", Index, Context));
      continue;
    }

    //
    // Skip comments.
    //
    if (CurrentKey[0] == '#') {
      continue;
    }

    DEBUG ((DEBUG_VERBOSE, "OCS: Parsing serialized at %a at %u index!\n", CurrentKey, Index));

    //
    // We do not protect from duplicating serialized entries.
    //
    NewSchema = LookupConfigSchema (Info->Dict.Schema, Info->Dict.SchemaSize, CurrentKey);

    if (NewSchema == NULL) {
      DEBUG ((DEBUG_WARN, "OCS: No schema for %a at %u index, context <%a>!\n", CurrentKey, Index, Context));
      continue;
    }

    OldValue = CurrentValue;
    CurrentValue = PlistNodeCast (CurrentValue, NewSchema->Type);
    if (CurrentValue == NULL) {
      DEBUG ((
        DEBUG_WARN,
        "OCS: No type match for %a at %u index, expected type %a got %a, context <%a>!\n",
        CurrentKey,
        Index,
        GetSchemaTypeName (NewSchema->Type),
        XmlNodeName (OldValue),
        Context
        ));
      continue;
    }

    NewSchema->Apply (Serialized, CurrentValue, &NewSchema->Info, CurrentKey);
  }
}

VOID
ParseSerializedValue (
  OUT VOID            *Serialized,
  IN  XML_NODE        *Node,
  IN  OC_SCHEMA_INFO  *Info,
  IN  CONST CHAR8     *Context  OPTIONAL
  )
{
  BOOLEAN  Result;
  VOID     *Field;
  UINT32   Size;

  Result = FALSE;
  Field  = OC_SCHEMA_FIELD (Serialized, VOID, Info->Value.Field);
  Size   = Info->Value.FieldSize;

  switch (Info->Value.Type) {
    case OC_SCHEMA_VALUE_BOOLEAN:
      Result = PlistBooleanValue (Node, (BOOLEAN *) Field);
      break;
    case OC_SCHEMA_VALUE_INTEGER:
      Result = PlistIntegerValue (Node, Field, Size, FALSE);
      break;
    case OC_SCHEMA_VALUE_DATA:
      Result = PlistDataValue (Node, Field, &Size);
      break;
    case OC_SCHEMA_VALUE_STRING:
      Result = PlistStringValue (Node, Field, &Size);
      break;
    case OC_SCHEMA_VALUE_MDATA:
      Result = PlistMetaDataValue (Node, Field, &Size);
      break;
  }

  if (Result == FALSE) {
    DEBUG ((
      DEBUG_WARN,
      "OCS: Failed to parse %a field as value with type %a and <%a> contents, context <%a>!\n",
      XmlNodeName (Node),
      GetSchemaTypeName (Info->Value.Type),
      XmlNodeContent (Node) != NULL ? XmlNodeContent (Node) : "empty",
      Context
      ));
  }
}

VOID
ParseSerializedBlob (
  OUT VOID            *Serialized,
  IN  XML_NODE        *Node,
  IN  OC_SCHEMA_INFO  *Info,
  IN  CONST CHAR8     *Context  OPTIONAL
  )
{
  BOOLEAN  Result;
  VOID     *Field;
  UINT32   Size;
  VOID     *BlobMemory;
  UINT32   *BlobSize;

  Result = FALSE;

  switch (Info->Blob.Type) {
    case OC_SCHEMA_BLOB_DATA:
      Result = PlistDataSize (Node, &Size);
      break;
    case OC_SCHEMA_BLOB_STRING:
      Result = PlistStringSize (Node, &Size);
      break;
    case OC_SCHEMA_BLOB_MDATA:
      Result = PlistMetaDataSize (Node, &Size);
      break;
  }

  if (Result == FALSE) {
    DEBUG ((
      DEBUG_WARN,
      "OCS: Failed to calculate size of %a field containing <%a> as type %a, context <%a>!\n",
      XmlNodeName (Node),
      XmlNodeContent (Node) != NULL ? XmlNodeContent (Node) : "empty",
      GetSchemaTypeName (Info->Blob.Type),
      Context
      ));
    return;
  }

  Field  = OC_SCHEMA_FIELD (Serialized, VOID, Info->Blob.Field);
  BlobMemory = OcBlobAllocate (Field, Size, &BlobSize);

  if (BlobMemory == NULL) {
    DEBUG ((
      DEBUG_INFO,
      "OCS: Failed to allocate %u bytes %a field of type %a, context <%a>!\n",
      Size,
      XmlNodeName (Node),
      GetSchemaTypeName (Info->Value.Type),
      Context
      ));
    return;
  }

  Result = FALSE;

  switch (Info->Blob.Type) {
    case OC_SCHEMA_BLOB_DATA:
      Result = PlistDataValue (Node, (UINT8 *) BlobMemory, BlobSize);
      break;
    case OC_SCHEMA_BLOB_STRING:
      Result = PlistStringValue (Node, (CHAR8 *) BlobMemory, BlobSize);
      break;
    case OC_SCHEMA_BLOB_MDATA:
      Result = PlistMetaDataValue (Node, (UINT8 *) BlobMemory, BlobSize);
      break;
  }

  if (Result == FALSE) {
    DEBUG ((
      DEBUG_WARN,
      "OCS: Failed to parse %a field as blob with type %a and <%a> contents, context <%a>!\n",
      XmlNodeName (Node),
      GetSchemaTypeName (Info->Value.Type),
      XmlNodeContent (Node) != NULL ? XmlNodeContent (Node) : "empty",
      Context
      ));
  }
}

VOID
ParseSerializedMap (
  OUT VOID            *Serialized,
  IN  XML_NODE        *Node,
  IN  OC_SCHEMA_INFO  *Info,
  IN  CONST CHAR8     *Context  OPTIONAL
  )
{
  UINT32       DictSize;
  UINT32       Index;
  CONST CHAR8  *CurrentKey;
  UINT32       CurrentKeyLen;
  XML_NODE     *ChildNode;
  VOID         *NewValue;
  VOID         *NewKey;
  VOID         *NewKeyValue;
  BOOLEAN      Success;

  DictSize = PlistDictChildren (Node);

  for (Index = 0; Index < DictSize; Index++) {
    CurrentKey = PlistKeyValue (PlistDictChild (Node, Index, &ChildNode));
    CurrentKeyLen = CurrentKey != NULL ? (UINT32) (AsciiStrLen (CurrentKey) + 1) : 0;

    if (CurrentKeyLen == 0) {
      DEBUG ((DEBUG_INFO, "OCS: No get serialized key at %u index!\n", Index));
      continue;
    }

    //
    // Skip comments.
    //
    if (CurrentKey[0] == '#') {
      continue;
    }

    if (PlistNodeCast (ChildNode, Info->List.Schema->Type) == NULL) {
      DEBUG ((DEBUG_INFO, "OCS: No valid serialized value at %u index!\n", Index));
      continue;
    }

    Success = OcListEntryAllocate (
      OC_SCHEMA_FIELD (Serialized, VOID, Info->List.Field),
      &NewValue,
      &NewKey
      );
    if (Success == FALSE) {
      DEBUG ((DEBUG_INFO, "OCS: Couldn't insert dict serialized at %u index!\n", Index));
      continue;
    }

    NewKeyValue = OcBlobAllocate (NewKey, CurrentKeyLen, NULL);
    if (NewKeyValue != NULL) {
      AsciiStrnCpyS ((CHAR8 *) NewKeyValue, CurrentKeyLen, CurrentKey, CurrentKeyLen - 1);
    } else {
      DEBUG ((DEBUG_INFO, "OCS: Couldn't allocate key name at %u index!\n", Index));
    }

    Info->List.Schema->Apply (NewValue, ChildNode, &Info->List.Schema->Info, CurrentKey);
  }
}

VOID
ParseSerializedArray (
  OUT VOID            *Serialized,
  IN  XML_NODE        *Node,
  IN  OC_SCHEMA_INFO  *Info,
  IN  CONST CHAR8     *Context  OPTIONAL
  )
{
  UINT32    ArraySize;
  UINT32    Index;
  XML_NODE  *ChildNode;
  VOID      *NewValue;
  BOOLEAN   Success;

  ArraySize = XmlNodeChildren (Node);

  for (Index = 0; Index < ArraySize; Index++) {
    ChildNode = PlistNodeCast (XmlNodeChild (Node, Index), Info->List.Schema->Type);

    DEBUG ((DEBUG_VERBOSE, "OCS: Processing array %u/%u element\n", Index + 1, ArraySize));

    if (ChildNode == NULL) {
      DEBUG ((DEBUG_INFO, "OCS: Couldn't get array serialized at %u index!\n", Index));
      continue;
    }

    Success = OcListEntryAllocate (
      OC_SCHEMA_FIELD (Serialized, VOID, Info->List.Field),
      &NewValue,
      NULL
      );
    if (Success == FALSE) {
      DEBUG ((DEBUG_INFO, "OCS: Couldn't insert array serialized at %u index!\n", Index));
      continue;
    }

    Info->List.Schema->Apply (NewValue, ChildNode, &Info->List.Schema->Info, Context);
  }
}

BOOLEAN
ParseSerialized (
  OUT VOID            *Serialized,
  IN  OC_SCHEMA_INFO  *RootSchema,
  IN  VOID            *PlistBuffer,
  IN  UINT32          PlistSize
  )
{
  XML_DOCUMENT        *Document;
  XML_NODE            *RootDict;

  Document = XmlDocumentParse (PlistBuffer, PlistSize, FALSE);

  if (Document == NULL) {
    DEBUG ((DEBUG_INFO, "OCS: Couldn't parse serialized file!\n"));
    return FALSE;
  }

  RootDict = PlistNodeCast (PlistDocumentRoot (Document), PLIST_NODE_TYPE_DICT);

  if (RootDict == NULL) {
    DEBUG ((DEBUG_INFO, "OCS: Couldn't get serialized root!\n"));
    XmlDocumentFree (Document);
    return FALSE;
  }

  ParseSerializedDict (
    Serialized,
    RootDict,
    RootSchema,
    "root"
    );

  XmlDocumentFree (Document);
  return TRUE;
}
