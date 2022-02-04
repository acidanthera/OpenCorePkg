/** @file

OcXmlLib

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Copyright (c) 2012 ooxi/xml.c
//     https://github.com/ooxi/xml.c
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the
// use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software in a
//     product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//

#ifndef OC_XML_LIB_H
#define OC_XML_LIB_H

#include <Library/OcGuardLib.h>

/**
  Maximum nest level.
  XML_PARSER_NEST_LEVEL is required to fit into INT32.
**/
#ifndef XML_PARSER_NEST_LEVEL
#define XML_PARSER_NEST_LEVEL 32ULL
#endif

/**
  Maximum child node count.
  XML_PARSER_NODE_COUNT*2 is required to fit into INT32.
**/
#ifndef XML_PARSER_NODE_COUNT
#define XML_PARSER_NODE_COUNT 32768ULL
#endif

/**
  Maximum reference count.
  XML_PARSER_MAX_REFERENCE_COUNT*2 is required to fit into INT32.
**/
#ifndef XML_PARSER_MAX_REFERENCE_COUNT
#define XML_PARSER_MAX_REFERENCE_COUNT (32ULL*1024)
#endif

/**
  Maximum input data size, currently 32 MB.
  XML_PARSER_MAX_SIZE is required to fit into INT32.
**/
#ifndef XML_PARSER_MAX_SIZE
#define XML_PARSER_MAX_SIZE (32ULL*1024*1024)
#endif

/**
  Debug controls.
**/
//#define XML_PARSER_VERBOSE
//#define XML_PRINT_ERRORS

/**
  Plist node types.
**/
typedef enum PLIST_NODE_TYPE_ {
  PLIST_NODE_TYPE_ANY,
  PLIST_NODE_TYPE_ARRAY,
  PLIST_NODE_TYPE_DICT,
  PLIST_NODE_TYPE_KEY,
  PLIST_NODE_TYPE_STRING,
  PLIST_NODE_TYPE_DATA,
  PLIST_NODE_TYPE_DATE,
  PLIST_NODE_TYPE_TRUE,
  PLIST_NODE_TYPE_FALSE,
  PLIST_NODE_TYPE_REAL,
  PLIST_NODE_TYPE_INTEGER,
  PLIST_NODE_TYPE_MAX
} PLIST_NODE_TYPE;

/**
  Opaque structure holding the parsed xml document.
**/
struct XML_DOCUMENT_;
struct XML_NODE_;
typedef struct XML_DOCUMENT_ XML_DOCUMENT;
typedef struct XML_NODE_ XML_NODE;


/**
  Parse the XML fragment in buffer.
  References in the document to allow deduplicated node reading:
  <integer ID="0" size="64">0x0</integer>
  <integer IDREF="0" size="64"/>

  @param[in,out]  Buffer  Chunk to be parsed.
  @param[in]      Length  Size of the buffer.
  @param[in]      WithRef TRUE to enable reference lookup support.

  @warning `Buffer` will be referenced by the document, it may not be freed
           until XML_DOCUMENT is freed.
  @warning XmlDocumentFree should be called after completion.
  @warning `Buffer` contents are permanently modified during parsing

  @return The parsed xml fragment or NULL.
**/
XML_DOCUMENT *
XmlDocumentParse (
  IN OUT  CHAR8    *Buffer,
  IN      UINT32   Length,
  IN      BOOLEAN  WithRefs
  );

/**
  Export parsed document into the buffer.

  @param[in]  Document          XML_DOCUMENT to export.
  @param[out] Length            Resulting length of the buffer without trailing '\0'. Optional.
  @param[in]  Skip              Number of root levels to be skipped before exporting, normally 0.
  @param[in]  PrependPlistInfo  TRUE to prepend XML plist doc info to exported document.

  @return The exported buffer allocated from pool or NULL.
**/
CHAR8 *
XmlDocumentExport (
  IN   CONST XML_DOCUMENT  *Document,
  OUT  UINT32              *Length  OPTIONAL,
  IN   UINT32              Skip,
  IN   BOOLEAN             PrependPlistInfo
  );

/**
  Free all resources associated with the document. All XML_NODE
  references obtained through the document will be invalidated.

  @param[in,out] Document XML document to be freed.
**/
VOID
XmlDocumentFree (
  IN OUT  XML_DOCUMENT  *Document
  );

/**
  Get the root of the XML document.

  @param[in]  Document  A pointer to the XML document.

  @return Root node of Document.
**/
XML_NODE *
XmlDocumentRoot (
  IN  CONST XML_DOCUMENT  *Document
  );

/**
  Get the name of an XML node.

  @param[in]  Node  A pointer to the XML node.

  @return The name of the XML node.
**/
CONST CHAR8 *
XmlNodeName (
  IN  CONST XML_NODE  *Node
  );

/**
  Get the content of an XML node.

  @param[in]  Node  A pointer to the XML node.

  @return The string content of the XML node or NULL.
**/
CONST CHAR8 *
XmlNodeContent (
  IN  CONST XML_NODE  *Node
  );

/**
  Change the string content of an XML node.

  @param[in,out]  Node     A pointer to the XML node.
  @param[in]      Content  New node content.

  @warning Content must stay valid until freed by XmlDocumentFree.
**/
VOID
XmlNodeChangeContent (
  IN OUT  XML_NODE     *Node,
  IN      CONST CHAR8  *Content
  );

/**
  Get the number of child nodes under an XML node.

  @param[in]  Node  A pointer to the XML node.

  @return Number of child nodes.
**/
UINT32
XmlNodeChildren (
  IN  CONST XML_NODE  *Node
  );

/**
  Get the specific child node.

  @param[in]  Node   A pointer to the XML node.
  @param[in]  Child  Index of children of Node.

  @return The n-th child, behaviour is undefined if out of range.
**/
XML_NODE *
XmlNodeChild (
  IN  CONST XML_NODE  *Node,
  IN  UINT32          Child
  );

/**
  Get the child node specified by the list of names.

  @param[in,out]  Node       A pointer to the XML node.
  @param[in]      ChildName  List of child names.

  @return The node described by the path or NULL if child cannot be found.

  @warning Each element on the way must be unique.
  @warning Last argument must be NULL.
**/
XML_NODE *
EFIAPI
XmlEasyChild (
  IN OUT  XML_NODE     *Node,
  IN      CONST CHAR8  *ChildName,
  ...
  );

/**
  Append new node to current node.

  @param[in,out]  Node        Current node.
  @param[in]      Name        Name of the new node.
  @param[in]      Attributes  Attributes of the new node. Optional.
  @param[in]      Content     New node content. Optional.

  @return Newly created node or NULL.

  @warning Name, Attributes, and Content must stay valid till XmlDocumentFree.
**/
XML_NODE *
XmlNodeAppend (
  IN OUT  XML_NODE     *Node,
  IN      CONST CHAR8  *Name,
  IN      CONST CHAR8  *Attributes  OPTIONAL,
  IN      CONST CHAR8  *Content     OPTIONAL
  );

/**
  Prepend new node to current node.

  @param[in,out]  Node        Current node.
  @param[in]      Name        Name of the new node.
  @param[in]      Attributes  Attributes of the new node. Optional.
  @param[in]      Content     New node content. Optional.

  @return Newly created node or NULL.

  @warning Name, Attributes, and Content must stay valid till XmlDocumentFree.
**/
XML_NODE *
XmlNodePrepend (
  IN OUT  XML_NODE     *Node,
  IN      CONST CHAR8  *Name,
  IN      CONST CHAR8  *Attributes,
  IN      CONST CHAR8  *Content
  );

/**
  Get the root node of the plist document.

  @param[in]  Document  A pointer to the plist document.

  @return Root node of the plist document or NULL.

  @warning Only a subset of plist is supported.
  @warning No validation of plist format is performed.
**/
XML_NODE *
PlistDocumentRoot (
  IN  CONST XML_DOCUMENT  *Document
  );

/**
  Basic type casting (up to PLIST_NODE_TYPE_MAX).

  Guarantees that node represents passed type.
  Guarantees that arrays and dicts have valid amount of children, while others have 0.
  Guarantees that keys have names and integers have values.

  @param[in]  Node  A pointer to the XML node. Optional.
  @param[in]  Type  Plist node type to be casted to.

  @return Node if it is not NULL and represents passed Type or NULL.

  @warning It is not guaranteed that Node has valid data, for data or integer types.
**/
XML_NODE *
PlistNodeCast (
  IN  XML_NODE         *Node  OPTIONAL,
  IN  PLIST_NODE_TYPE  Type
  );

/**
  Get the number of plist dictionary entries.

  @param[in]  Node  A pointer to the XML node.

  @return Number of plist dictionary entries.
**/
UINT32
PlistDictChildren (
  IN  CONST XML_NODE  *Node
  );

/**
  Get the specific child node under a plist dictionary.

  @param[in]   Node   A pointer to the XML node.
  @param[in]   Child  Index of children of Node.
  @param[out]  Value  Value of the returned Node. Optional.

  @return The n-th dictionary key, behaviour is undefined if out of range.
**/
XML_NODE *
PlistDictChild (
  IN   CONST XML_NODE  *Node,
  IN   UINT32          Child,
  OUT  XML_NODE        **Value OPTIONAL
  );

/**
  Get the value of a plist key.

  @param[in]   Node   A pointer to the XML node. Optional.

  @return Key value for valid type or NULL.
**/
CONST CHAR8 *
PlistKeyValue (
  IN  XML_NODE  *Node  OPTIONAL
  );

/**
  Get the value of a plist string.

  @param[in]      Node   A pointer to the XML node. Optional.
  @param[out]     Value  Value of plist string.
  @param[in,out]  Size   Size of Value, including the '\0' terminator.

  @return TRUE if Node can be casted to PLIST_NODE_TYPE_STRING.
**/
BOOLEAN
PlistStringValue (
  IN      XML_NODE  *Node   OPTIONAL,
     OUT  CHAR8     *Value,
  IN OUT  UINT32    *Size
  );

/**
  Decode data content for valid type or set *Size to 0.

  @param[in]      Node    A pointer to the XML node. Optional.
  @param[out]     Buffer  Buffer of plist data.
  @param[in,out]  Size    Size of Buffer.

  @return TRUE if Node can be casted to PLIST_NODE_TYPE_DATA.
**/
BOOLEAN
PlistDataValue (
  IN      XML_NODE  *Node    OPTIONAL,
  OUT     UINT8     *Buffer,
  IN OUT  UINT32    *Size
  );

/**
  Get the value of a plist boolean.

  @param[in]      Node   A pointer to the XML node. Optional.
  @param[out]     Value  Value of plist boolean.

  @return TRUE if Node can be casted to PLIST_NODE_TYPE_TRUE or PLIST_NODE_TYPE_FALSE.
**/
BOOLEAN
PlistBooleanValue (
  IN   XML_NODE  *Node   OPTIONAL,
  OUT  BOOLEAN   *Value
  );

/**
  Get the value of a plist integer.

  @param[in]   Node   A pointer to the XML node. Optional.
  @param[out]  Value  Value of plist integer.
  @param[in]   Size   Size of Value to be casted to (UINT8, UINT16, UINT32, or UINT64).
  @param[in]   Hex    TRUE to interpret the value as hexadecimal values, decimal otherwise.   

  @return TRUE if Node can be casted to PLIST_NODE_TYPE_TRUE or PLIST_NODE_TYPE_FALSE.
**/
BOOLEAN
PlistIntegerValue (
  IN   XML_NODE  *Node   OPTIONAL,
  OUT  VOID      *Value,
  IN   UINT32    Size,
  IN   BOOLEAN   Hex
  );

/**
  Get the values of multiple types of data that are valid.

  Valid type for MultiData is DATA itself, STRING, INTEGER,
  or BOOLEAN (as 1 byte with 1 or 0 value).

  @param[in]      Node    A pointer to the XML node. Optional.
  @param[out]     Buffer  Buffer of plist MultiData.
  @param[in,out]  Size    Size of Buffer.

  @warning Integer must fit 32-bit UNSIGNED.
  @warning Buffer must be at least 1 byte long.

  @return TRUE if Node can be casted to any of the aforementioned types.
**/
BOOLEAN
PlistMultiDataValue (
  IN      XML_NODE  *Node    OPTIONAL,
     OUT  VOID      *Buffer,
  IN OUT  UINT32    *Size
  );

/**
  Get size of a plist string, including the '\0' terminator.

  @param[in]      Node   A pointer to the XML node. Optional.
  @param[out]     Size   Size of string.

  @return TRUE if Node can be casted to PLIST_NODE_TYPE_STRING.
**/
BOOLEAN
PlistStringSize (
  IN   XML_NODE  *Node  OPTIONAL,
  OUT  UINT32    *Size
  );

/**
  Get size of a plist data.

  @param[in]      Node   A pointer to the XML node. Optional.
  @param[out]     Size   Size of data.

  @return TRUE if Node can be casted to PLIST_NODE_TYPE_DATA.
**/
BOOLEAN
PlistDataSize (
  IN   XML_NODE  *Node  OPTIONAL,
  OUT  UINT32    *Size
  );

/**
  Get size of multiple types of data that are valid.

  Valid type for MultiData is DATA itself, STRING, INTEGER,
  or BOOLEAN (as 1 byte with 1 or 0 value).

  @param[in]      Node   A pointer to the XML node. Optional.
  @param[out]     Size   Size of MultiData.

  @return TRUE if Node can be casted to any of the aforementioned types.
**/
BOOLEAN
PlistMultiDataSize (
  IN   XML_NODE  *Node  OPTIONAL,
  OUT  UINT32    *Size
  );

#endif // OC_XML_LIB_H
