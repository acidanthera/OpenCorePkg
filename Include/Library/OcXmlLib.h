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

//
// Maximum nest level.
// XML_PARSER_NEST_LEVEL is required to fit into INT32.
//
#ifndef XML_PARSER_NEST_LEVEL
#define XML_PARSER_NEST_LEVEL 32ULL
#endif

//
// Maximum child node count
// XML_PARSER_NODE_COUNT*2 is required to fit into INT32.
//
#ifndef XML_PARSER_NODE_COUNT
#define XML_PARSER_NODE_COUNT 32768ULL
#endif

//
// Maximum reference count.
// XML_PARSER_MAX_REFERENCE_COUNT*2 is required to fit into INT32.
//
#ifndef XML_PARSER_MAX_REFERENCE_COUNT
#define XML_PARSER_MAX_REFERENCE_COUNT (32ULL*1024)
#endif

//
// Maximum input data size, currently 32 MB.
// XML_PARSER_MAX_SIZE is required to fit into INT32.
//
#ifndef XML_PARSER_MAX_SIZE
#define XML_PARSER_MAX_SIZE (32ULL*1024*1024)
#endif

//
// Debug controls
//
//#define XML_PARSER_VERBOSE
//#define XML_PRINT_ERRORS

//
// Plist node types.
//
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

//
// Opaque structure holding the parsed xml document.
//
struct XML_DOCUMENT_;
struct XML_NODE_;
typedef struct XML_DOCUMENT_ XML_DOCUMENT;
typedef struct XML_NODE_ XML_NODE;


//
// Tries to parse the XML fragment in buffer
// References in the document to allow deduplicated node reading:
// <integer ID="0" size="64">0x0</integer>
// <integer IDREF="0" size="64"/>
//
// @param Buffer  Chunk to parse
// @param Length  Size of the buffer
// @param WithRef Enable reference lookup support
//
// @warning `Buffer` will be referenced by the document, you may not free it
//     until you free the XML_DOCUMENT
// @warning You have to call XmlDocumentFree after you finished using the
//     document
// @warning `Buffer` contents are permanently modified during parsing
//
// @return The parsed xml fragment iff parsing was successful, 0 otherwise
//
XML_DOCUMENT *
XmlDocumentParse (
  CHAR8    *Buffer,
  UINT32   Length,
  BOOLEAN  WithRefs
  );

//
// Exports parsed document into the buffer.
//
// @param Document XML_DOCUMENT to export
// @param Length   Resulting length of the buffer without trailing \0 (optional)
// @param Skip     N root levels before exporting, normally 0.
//
// @return Exported buffer allocated from pool or NULL.
//
CHAR8 *
XmlDocumentExport (
  XML_DOCUMENT  *Document,
  UINT32        *Length,
  UINT32        Skip
  );

//
// Frees all resources associated with the document. All XML_NODE
// references obtained through the document will be invalidated.
//
// @param Document XML_DOCUMENT to free
//
VOID
XmlDocumentFree (
	XML_DOCUMENT  *Document
  );

//
// @return XML_NODE representing the document root.
//
XML_NODE *
XmlDocumentRoot (
  XML_DOCUMENT  *Document
  );

//
// @return The XML_NODE's tag name.
//
CONST CHAR8 *
XmlNodeName (
  XML_NODE  *Node
  );

//
// @return The XML_NODE's string content (if available, otherwise NULL).
//
CONST CHAR8 *
XmlNodeContent (
  XML_NODE  *Node
  );

//
// @return Number of child nodes.
//
UINT32
XmlNodeChildren (
  XML_NODE  *Node
  );

//
// @return The n-th child, behaviour is undefined if out of range.
//
XML_NODE *
XmlNodeChild (
  XML_NODE  *Node,
  UINT32    Child
  );

//
// @return The node described by the path or NULL if child cannot be found.
// @warning Each element on the way must be unique.
// @warning Last argument must be NULL.
//
XML_NODE *
EFIAPI
XmlEasyChild (
  XML_NODE     *Node,
  CONST CHAR8  *Child,
  ...
  );

//
// Append new node to current node.
//
// @param  Node        Current node.
// @param  Name        Name of the new node.
// @param  Attributes  Attributes of the new node (optional).
// @param  Content     New node content (optional).
//
// @return Newly created node or NULL.
//
// @warning Name, Attributes, and Content must stay valid till XmlDocumentFree.
//
XML_NODE *
XmlNodeAppend (
  XML_NODE     *Node,
  CONST CHAR8  *Name,
  CONST CHAR8  *Attributes,
  CONST CHAR8  *Content
  );

//
// Prepend new node to current node.
// Otherwise equivalent to XmlNodeAppend.
//
XML_NODE *
XmlNodePrepend (
  XML_NODE     *Node,
  CONST CHAR8  *Name,
  CONST CHAR8  *Attributes,
  CONST CHAR8  *Content
  );

//
// @return XML_NODE representing plist root or NULL.
// @warning Only a subset of plist is supported.
// @warning No validation of plist format is performed.
//
XML_NODE *
PlistDocumentRoot (
  XML_DOCUMENT  *Document
  );

//
// Performs basic type casting (up to PLIST_NODE_TYPE_MAX).
// Guarantees that node represents passed type.
// Guarantees that arrays and dicts have valid amount of children, while others have 0.
// Guarantees that keys have names and integers have values.
//
// @return Node if it is not NULL and represents passed Type or NULL.
// @warning It is not guaranteed that Node has valid data for data or integer types.
//
XML_NODE *
PlistNodeCast (
  XML_NODE         *Node,
  PLIST_NODE_TYPE  Type
  );

//
// @return Number of plist dictionary entries.
//
UINT32
PlistDictChildren (
  XML_NODE     *Node
  );

//
// @return The n-th dictionary key, behaviour is undefined if out of range.
//
XML_NODE *
PlistDictChild (
  XML_NODE     *Node,
  UINT32       Child,
  XML_NODE     **Value OPTIONAL
  );

//
// @return key value for valid type or NULL.
//
CONST CHAR8 *
PlistKeyValue (
  XML_NODE  *Node
  );

//
// @return string value for valid type or empty string ("").
//
BOOLEAN
PlistStringValue (
  XML_NODE  *Node,
  CHAR8     *Value,
  UINT32    *Size
  );

//
// Decodes data content for valid type or sets *Size to 0.
//
// @param Buffer output buffer.
// @param Size size of buffer.
//
BOOLEAN
PlistDataValue (
  XML_NODE  *Node,
  UINT8     *Buffer,
  UINT32    *Size
  );

//
// @return boolean value for valid type or FALSE.
//
BOOLEAN
PlistBooleanValue (
  XML_NODE  *Node,
  BOOLEAN   *Value
  );

//
// @return integral value for valid type or 0.
//
BOOLEAN
PlistIntegerValue (
  XML_NODE  *Node,
  VOID      *Value,
  UINT32    Size,
  BOOLEAN   Hex
  );

//
// Decodes data content for valid type or sets *Size to 0.
// Valid type for MetaData is DATA itself, STRING, INTEGER,
// or BOOLEAN (as 1 byte with 1 or 0 value).
//
// @param Buffer output buffer.
// @param Size size of buffer.
// @warn Integer must fit 32-bit UNSIGNED.
// @warn Buffer must be at least 1 byte long.
//
BOOLEAN
PlistMetaDataValue (
  XML_NODE  *Node,
  VOID      *Buffer,
  UINT32    *Size
  );

//
// Estimates string content size.
//
BOOLEAN
PlistStringSize (
  XML_NODE  *Node,
  UINT32    *Size
  );

//
// Estimates data content size.
//
BOOLEAN
PlistDataSize (
  XML_NODE  *Node,
  UINT32    *Size
  );

//
// Estimates meta data content size.
//
BOOLEAN
PlistMetaDataSize (
  XML_NODE  *Node,
  UINT32    *Size
  );

#endif // OC_XML_LIB_H
