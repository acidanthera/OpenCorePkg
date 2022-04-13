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

#ifndef OC_SERIALIZE_LIB_H
#define OC_SERIALIZE_LIB_H

#include <Library/OcXmlLib.h>
#include <Library/OcTemplateLib.h>

typedef struct OC_SCHEMA_ OC_SCHEMA;
typedef union OC_SCHEMA_INFO_ OC_SCHEMA_INFO;

//
// Generic applier interface that knows how to provide Info with data from Node.
//
typedef
VOID
(*OC_APPLY) (
      OUT  VOID            *Serialized,
  IN       XML_NODE        *Node,
  IN       OC_SCHEMA_INFO  *Info,
  IN       CONST CHAR8     *Context     OPTIONAL,
  IN  OUT  UINT32          *ErrorCount  OPTIONAL
  );

//
// OC_SCHEMA_INFO for nested dictionaries
//
typedef struct {
  //
  // Nested schema list.
  //
  OC_SCHEMA         *Schema;
  //
  // Nested schema list size.
  //
  UINT32            SchemaSize;
} OC_SCHEMA_DICT;

//
// OC_SCHEMA_INFO for static values
//

typedef enum OC_SCHEMA_VALUE_TYPE_ {
  OC_SCHEMA_VALUE_BOOLEAN,
  OC_SCHEMA_VALUE_INTEGER,
  OC_SCHEMA_VALUE_DATA,
  OC_SCHEMA_VALUE_STRING,
  OC_SCHEMA_VALUE_MDATA
} OC_SCHEMA_VALUE_TYPE;

typedef struct {
  //
  // Value pointer.
  //
  UINTN                 Field;
  //
  // Value size.
  //
  UINT32                FieldSize;
  //
  // Source type.
  //
  OC_SCHEMA_VALUE_TYPE  Type;
} OC_SCHEMA_VALUE;

//
// OC_SCHEMA_INFO for blob values
//

typedef enum OC_SCHEMA_BLOB_TYPE_ {
  OC_SCHEMA_BLOB_DATA,
  OC_SCHEMA_BLOB_STRING,
  OC_SCHEMA_BLOB_MDATA
} OC_SCHEMA_BLOB_TYPE;

typedef struct {
  //
  // Blob pointer.
  //
  UINTN                 Field;
  //
  // Source type.
  //
  OC_SCHEMA_BLOB_TYPE   Type;
} OC_SCHEMA_BLOB;

//
// OC_SCHEMA_INFO for array/map lists
//
typedef struct {
  //
  // List pointer.
  //
  UINTN                 Field;
  //
  // List entry schema.
  //
  OC_SCHEMA             *Schema;
} OC_SCHEMA_LIST;

//
// All standard variants of schema info
//
union OC_SCHEMA_INFO_ {
  OC_SCHEMA_DICT   Dict;
  OC_SCHEMA_VALUE  Value;
  OC_SCHEMA_BLOB   Blob;
  OC_SCHEMA_LIST   List;
};

//
// Schema context, allowing node recursion.
//
struct OC_SCHEMA_ {
  //
  // Key name to match against in the dictionary.
  //
  CONST CHAR8          *Name;
  //
  // Node type required to match to be able to call Apply.
  // PLIST_NODE_TYPE_ANY could be specified if Apply does the validation.
  //
  PLIST_NODE_TYPE      Type;
  //
  // Whether this node is optional to use.
  //
  BOOLEAN              Optional;
  //
  // Apply handler that will merge Node data into object.
  //
  OC_APPLY             Apply;
  //
  // Information about Node to object bridge.
  //
  OC_SCHEMA_INFO       Info;
};

//
// Find schema in a sorted list
//
OC_SCHEMA *
LookupConfigSchema (
  IN OC_SCHEMA      *SortedList,
  IN UINT32         Size,
  IN CONST CHAR8    *Name
  );

//
// Apply interface to parse serialized dictionaries
//
VOID
ParseSerializedDict (
      OUT  VOID            *Serialized,
  IN       XML_NODE        *Node,
  IN       OC_SCHEMA_INFO  *Info,
  IN       CONST CHAR8     *Context     OPTIONAL,
  IN  OUT  UINT32          *ErrorCount  OPTIONAL
  );

//
// Apply interface to parse serialized OC_SCHEMA_VALUE_TYPE.
//
VOID
ParseSerializedValue (
      OUT  VOID            *Serialized,
  IN       XML_NODE        *Node,
  IN       OC_SCHEMA_INFO  *Info,
  IN       CONST CHAR8     *Context     OPTIONAL,
  IN  OUT  UINT32          *ErrorCount  OPTIONAL
  );

//
// Apply interface to parse serialized OC_SCHEMA_BLOB_TYPE.
//
VOID
ParseSerializedBlob (
      OUT  VOID            *Serialized,
  IN       XML_NODE        *Node,
  IN       OC_SCHEMA_INFO  *Info,
  IN       CONST CHAR8     *Context     OPTIONAL,
  IN  OUT  UINT32          *ErrorCount  OPTIONAL
  );

//
// Apply interface to parse serialized map with arbitrary values,
// and OC_STRING strings.
//
VOID
ParseSerializedMap (
      OUT  VOID            *Serialized,
  IN       XML_NODE        *Node,
  IN       OC_SCHEMA_INFO  *Info,
  IN       CONST CHAR8     *Context     OPTIONAL,
  IN  OUT  UINT32          *ErrorCount  OPTIONAL
  );

//
// Apply interface to parse serialized array
//
VOID
ParseSerializedArray (
      OUT  VOID            *Serialized,
  IN       XML_NODE        *Node,
  IN       OC_SCHEMA_INFO  *Info,
  IN       CONST CHAR8     *Context     OPTIONAL,
  IN  OUT  UINT32          *ErrorCount  OPTIONAL
  );

//
// Main interface for parsing serialized data.
// PlistBuffer will be modified during the execution.
//
BOOLEAN
ParseSerialized (
      OUT  VOID                *Serialized,
  IN       OC_SCHEMA_INFO      *RootSchema,
  IN       VOID                *PlistBuffer,
  IN       UINT32              PlistSize,
  IN  OUT  UINT32              *ErrorCount  OPTIONAL
  );

//
// Retrieve typed field pointer from offset
//
#define OC_SCHEMA_FIELD(Base, Type, Offset)                              \
  ((Type *)(((UINT8 *) (Base)) + (Offset)))

//
// Smart declaration base macros, see usage below.
//
#define OC_SCHEMA_VALUE(Name, Offset, Type, SourceType)                  \
  {(Name), PLIST_NODE_TYPE_ANY, FALSE, ParseSerializedValue,             \
    {.Value = {Offset, sizeof (Type), SourceType}}}

#define OC_SCHEMA_BLOB(Name, Offset, SourceType)                         \
  {(Name), PLIST_NODE_TYPE_ANY, FALSE, ParseSerializedBlob,              \
    {.Blob = {Offset, SourceType}}}

//
// Smart declaration macros for builtin appliers,
// which point straight to types.
//
// Name represents dictionary name if any (otherwise NULL).
// Type represents value or value type for size calculation.
// Schema represents element schema.
//
// M prefix stands for Meta, which means meta data type casting is used.
// F suffix stands for Fixed, which means fixed file size is assumed.
//
#define OC_SCHEMA_DICT(Name, Schema)                                     \
  {(Name), PLIST_NODE_TYPE_DICT, FALSE, ParseSerializedDict,             \
    {.Dict = {(Schema), ARRAY_SIZE (Schema)}}}

#define OC_SCHEMA_DICT_OPT(Name, Schema)                                 \
  {(Name), PLIST_NODE_TYPE_DICT, TRUE, ParseSerializedDict,              \
    {.Dict = {(Schema), ARRAY_SIZE (Schema)}}}

#define OC_SCHEMA_BOOLEAN(Name)                                          \
  OC_SCHEMA_VALUE (Name, 0, BOOLEAN, OC_SCHEMA_VALUE_BOOLEAN)

#define OC_SCHEMA_INTEGER(Name, Type)                                    \
  OC_SCHEMA_VALUE (Name, 0, Type, OC_SCHEMA_VALUE_INTEGER)

#define OC_SCHEMA_STRING(Name)                                           \
  OC_SCHEMA_BLOB (Name, 0, OC_SCHEMA_BLOB_STRING)

#define OC_SCHEMA_STRINGF(Name, Type)                                    \
  OC_SCHEMA_VALUE (Name, 0, Type, OC_SCHEMA_VALUE_STRING)

#define OC_SCHEMA_DATA(Name)                                             \
  OC_SCHEMA_BLOB (Name, 0, OC_SCHEMA_BLOB_DATA)

#define OC_SCHEMA_DATAF(Name, Type)                                      \
  OC_SCHEMA_VALUE (Name, 0, Type, OC_SCHEMA_VALUE_DATA)

#define OC_SCHEMA_MDATA(Name)                                            \
  OC_SCHEMA_BLOB (Name, 0, OC_SCHEMA_BLOB_MDATA)

#define OC_SCHEMA_MDATAF(Name, Type)                                     \
  OC_SCHEMA_VALUE (Name, 0, Type, OC_SCHEMA_VALUE_MDATA)

#define OC_SCHEMA_ARRAY(Name, ChildSchema)                               \
  {(Name), PLIST_NODE_TYPE_ARRAY, FALSE, ParseSerializedArray,           \
    {.List = {0, ChildSchema}}}

#define OC_SCHEMA_MAP(Name, ChildSchema)                                 \
  {(Name), PLIST_NODE_TYPE_DICT, FALSE, ParseSerializedMap,              \
    {.List = {0, ChildSchema}}}

//
// Smart declaration macros for builtin appliers,
// which point to structures, containing these types.
//
// Name represents dictionary name if any (otherwise NULL).
// Type represents container structure type for this value.
// Field represents item in the container type.
// Schema represents element schema.
//
#define OC_SCHEMA_BOOLEAN_IN(Name, Type, Field)                          \
  OC_SCHEMA_VALUE (Name, OFFSET_OF (Type, Field), (((Type *)0)->Field),  \
    OC_SCHEMA_VALUE_BOOLEAN)

#define OC_SCHEMA_INTEGER_IN(Name, Type, Field)                          \
  OC_SCHEMA_VALUE (Name, OFFSET_OF (Type, Field), (((Type *)0)->Field),  \
    OC_SCHEMA_VALUE_INTEGER)

#define OC_SCHEMA_STRING_IN(Name, Type, Field)                           \
  OC_SCHEMA_BLOB (Name, OFFSET_OF (Type, Field), OC_SCHEMA_BLOB_STRING)

#define OC_SCHEMA_STRINGF_IN(Name, Type, Field)                          \
  OC_SCHEMA_VALUE (Name, OFFSET_OF (Type, Field), (((Type *)0)->Field),  \
    OC_SCHEMA_VALUE_STRING)

#define OC_SCHEMA_DATA_IN(Name, Type, Field)                             \
  OC_SCHEMA_BLOB (Name, OFFSET_OF (Type, Field), OC_SCHEMA_BLOB_DATA)

#define OC_SCHEMA_DATAF_IN(Name, Type, Field)                            \
  OC_SCHEMA_VALUE (Name, OFFSET_OF (Type, Field), (((Type *)0)->Field),  \
    OC_SCHEMA_VALUE_DATA)

#define OC_SCHEMA_MDATA_IN(Name, Type, Field)                            \
  OC_SCHEMA_BLOB (Name, OFFSET_OF (Type, Field), OC_SCHEMA_BLOB_MDATA)

#define OC_SCHEMA_MDATAF_IN(Name, Type, Field)                           \
  OC_SCHEMA_VALUE (Name, OFFSET_OF (Type, Field), (((Type *)0)->Field),  \
    OC_SCHEMA_VALUE_MDATA)

#define OC_SCHEMA_ARRAY_IN(Name, Type, Field, ChildSchema)               \
  {(Name), PLIST_NODE_TYPE_ARRAY, FALSE, ParseSerializedArray,           \
    {.List = {OFFSET_OF (Type, Field), ChildSchema}}}

#define OC_SCHEMA_MAP_IN(Name, Type, Field, ChildSchema)                 \
  {(Name), PLIST_NODE_TYPE_DICT, FALSE, ParseSerializedMap,              \
    {.List = {OFFSET_OF (Type, Field), ChildSchema}}}

#endif // OC_SERIALIZE_LIB_H
