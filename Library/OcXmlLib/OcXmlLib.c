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

#include <Library/OcXmlLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>

//
// Minimal extra allocation size during export.
//
#define XML_EXPORT_MIN_ALLOCATION_SIZE 4096

struct XML_NODE_LIST_;
struct XML_PARSER_;

typedef struct XML_NODE_LIST_ XML_NODE_LIST;
typedef struct XML_PARSER_ XML_PARSER;

//
// An XML_NODE will always contain a tag name and possibly a list of
// children or text content.
//
struct XML_NODE_ {
  CONST CHAR8    *Name;
  CONST CHAR8    *Attributes;
  CONST CHAR8    *Content;
  XML_NODE       *Real;
  XML_NODE_LIST  *Children;
};

struct XML_NODE_LIST_ {
  UINT32    NodeCount;
  UINT32    AllocCount;
  XML_NODE  *NodeList[];
};

typedef struct {
  UINT32        RefCount;
  UINT32        RefAllocCount;
  XML_NODE      **RefList;
} XML_REFLIST;

//
// An XML_DOCUMENT simply contains the root node and the underlying buffer.
//
struct XML_DOCUMENT_ {
  struct {
    CHAR8       *Buffer;
    UINT32      Length;
  } Buffer;

  XML_NODE      *Root;
  XML_REFLIST   References;
};

//
// Parser context.
//
struct XML_PARSER_ {
  CHAR8  *Buffer;
  UINT32 Position;
  UINT32 Length;
  UINT32 Level;
};

//
// Character offsets.
//
typedef enum XML_PARSER_OFFSET_ {
  NO_CHARACTER        = -1,
  CURRENT_CHARACTER   = 0,
  NEXT_CHARACTER      = 1,
} XML_PARSER_OFFSET;

//
// Plist node types.
//
CONST CHAR8 *
PlistNodeTypes[PLIST_NODE_TYPE_MAX] = {
  NULL,
  "array",
  "dict",
  "key",
  "string",
  "data",
  "date",
  "true",
  "false",
  "real",
  "integer"
};


STATIC
BOOLEAN
XmlParseAttributeNumber (
  CONST CHAR8  *Attributes,
  CONST CHAR8  *Argument,
  UINT32       ArgumentLength,
  UINT32       *ArgumentValue
  )
{
  CONST CHAR8  *ArgumentStart;
  CONST CHAR8  *ArgumentEnd;
  UINTN        Number;
  CHAR8        NumberStr[16];

  //
  // FIXME: This may give false positives.
  //

  ArgumentStart = AsciiStrStr (Attributes, Argument);
  if (ArgumentStart == NULL) {
    return FALSE;
  }

  ArgumentStart += ArgumentLength;
  ArgumentEnd   = AsciiStrStr (ArgumentStart, "\"");
  Number        = ArgumentEnd - ArgumentStart;

  if (ArgumentEnd == NULL || Number > sizeof (NumberStr) - 1) {
    return FALSE;
  }

  CopyMem (&NumberStr, ArgumentStart, Number);
  NumberStr[Number] = '\0';
  *ArgumentValue = (UINT32) AsciiStrDecimalToUint64 (NumberStr);

  return TRUE;
}

//
// Allocates the node with contents.
//
STATIC
XML_NODE *
XmlNodeCreate (
  CONST CHAR8    *Name,
  CONST CHAR8    *Attributes,
  CONST CHAR8    *Content,
  XML_NODE       *Real,
  XML_NODE_LIST  *Children
  )
{
  XML_NODE  *Node;

  Node = AllocatePool (sizeof (XML_NODE));

  if (Node != NULL) {
    Node->Name       = Name;
    Node->Attributes = Attributes;
    Node->Content    = Content;
    Node->Real       = Real;
    Node->Children   = Children;
  }

  return Node;
}

//
// Adds child nodes to node.
//
STATIC
BOOLEAN
XmlNodeChildPush (
  XML_NODE  *Node,
  XML_NODE  *Child
  )
{
  UINT32         NodeCount;
  UINT32         AllocCount;
  XML_NODE_LIST  *NewList;

  NodeCount = 0;
  AllocCount = 1;

  //
  // Push new node if there is enough room.
  //
  if (Node->Children != NULL) {
    NodeCount = Node->Children->NodeCount;
    AllocCount = Node->Children->AllocCount;

    if (NodeCount < XML_PARSER_NODE_COUNT && AllocCount > NodeCount) {
      Node->Children->NodeList[NodeCount] = Child;
      Node->Children->NodeCount++;
      return TRUE;
    }
  }

  //
  // Insertion will exceed the limit.
  //
  if (NodeCount >= XML_PARSER_NODE_COUNT - 1) {
    return FALSE;
  }

  //
  // Allocate three times more room.
  // This balances performance and memory usage on large files like prelinked plist.
  //
  AllocCount *= 3;

  NewList = (XML_NODE_LIST *) AllocatePool (
    sizeof (XML_NODE_LIST) + sizeof (NewList->NodeList[0]) * AllocCount
    );

  if (NewList == NULL) {
    return FALSE;
  }

  NewList->NodeCount  = NodeCount + 1;
  NewList->AllocCount = AllocCount;

  if (Node->Children != NULL) {
    CopyMem (
      &NewList->NodeList[0],
      &Node->Children->NodeList[0],
      sizeof (NewList->NodeList[0]) * NodeCount
      );

    FreePool (Node->Children);
  }

  NewList->NodeList[NodeCount] = Child;
  Node->Children = NewList;

  return TRUE;
}

STATIC
BOOLEAN
XmlPushReference (
  XML_REFLIST  *References,
  XML_NODE     *Node,
  UINT32       ReferenceNumber
  )
{
  XML_NODE   **NewReferences;
  UINT32     NewRefAllocCount;

  if (ReferenceNumber >= XML_PARSER_MAX_REFERENCE_COUNT) {
    return FALSE;
  }

  if (ReferenceNumber >= References->RefAllocCount) {
    if (OcOverflowAddMulU32 (ReferenceNumber, 1, 2, &NewRefAllocCount)) {
      return FALSE;
    }

    NewReferences = AllocateZeroPool (NewRefAllocCount * sizeof (References->RefList[0]));
    if (NewReferences == NULL) {
      return FALSE;
    }

    if (References->RefList != NULL) {
      CopyMem (
        &NewReferences[0],
        &References->RefList[0],
        References->RefCount * sizeof (References->RefList[0])
        );
      FreePool (References->RefList);
    }

    References->RefList       = NewReferences;
    References->RefAllocCount = NewRefAllocCount;
  }

  References->RefList[ReferenceNumber] = Node;
  if (ReferenceNumber >= References->RefCount) {
    References->RefCount = ReferenceNumber + 1;
  }

  return TRUE;
}

STATIC
XML_NODE *
XmlNodeReal (
  XML_REFLIST  *References,
  CONST CHAR8  *Attributes
  )
{
  BOOLEAN      HasArgument;
  UINT32       Number;

  if (References == NULL || Attributes == NULL) {
    return NULL;
  }

  HasArgument = XmlParseAttributeNumber (
    Attributes,
    "IDREF=\"",
    L_STR_LEN ("IDREF=\""),
    &Number
    );

  if (!HasArgument || Number >= References->RefCount) {
    return NULL;
  }

  return References->RefList[Number];
}

//
// Frees the resources allocated by the node.
//
STATIC
VOID
XmlNodeFree (
  XML_NODE  *Node
  )
{
  UINT32  Index;

  if (Node->Children != NULL) {
    for (Index = 0; Index < Node->Children->NodeCount; ++Index) {
      XmlNodeFree (Node->Children->NodeList[Index]);
    }
    FreePool (Node->Children);
  }

  FreePool (Node);
}

STATIC
VOID
XmlFreeRefs (
  XML_REFLIST  *References
  )
{
  if (References->RefList != NULL) {
    FreePool (References->RefList);
    References->RefList = NULL;
  }
}

//
// Echos the parsers call stack for debugging purposes.
//
#ifdef XML_PARSER_VERBOSE
#define XML_PARSER_INFO(Parser, Message) \
  DEBUG ((DEBUG_VERBOSE, "OCXML: XML_PARSER_INFO %a\n", Message));
#define XML_PARSER_TAG(Parser, Tag) \
  DEBUG ((DEBUG_VERBOSE, "OCXML: XML_PARSER_TAG %a\n", Tag));
#else
#define XML_PARSER_INFO(Parser, Message) do {} while (0)
#define XML_PARSER_TAG(Parser, Tag) do {} while (0)
#endif

//
// Echos an error regarding the parser's source to the console.
//
VOID
XmlParserError (
  XML_PARSER         *Parser,
  XML_PARSER_OFFSET  Offset,
  CONST CHAR8        *Message
  )
{
  UINT32  Character = 0;
  UINT32  Position;
  UINT32  Row = 0;
  UINT32  Column = 0;

  if (Parser->Length > 0 && (Parser->Position > 0 || NO_CHARACTER != Offset)) {
    Character = Parser->Position + Offset;
    if (Character > Parser->Length-1) {
      Character = Parser->Length-1;
    }

    for (Position = 0; Position <= Character; ++Position) {
      Column++;

      if ('\n' == Parser->Buffer[Position]) {
        Row++;
        Column = 0;
      }
    }
  }

  if (NO_CHARACTER != Offset) {
    DEBUG ((DEBUG_INFO, "OCXML: XmlParserError at %u:%u (is %c): %a\n",
      Row + 1, Column, Parser->Buffer[Character], Message
      ));
  } else {
    DEBUG ((DEBUG_INFO, "OCXML: XmlParserError at %u:%u: %a\n",
      Row + 1, Column, Message
      ));
  }
}

//
// Conditionally enable error printing.
//
#ifdef XML_PRINT_ERRORS
#define XML_PARSER_ERROR(Parser, Offset, Message) \
  XmlParserError (Parser, Offset, Message)
#define XML_USAGE_ERROR(Message) \
  DEBUG ((DEBUG_VERBOSE, "OCXML: %a\n", Message));
#else
#define XML_PARSER_ERROR(Parser, Offset, Message) do {} while (0)
#define XML_USAGE_ERROR(X) do {} while (0)
#endif

//
// Returns the n-th not-whitespace byte in parser and 0 if such a byte does not
// exist.
//
STATIC
CHAR8
XmlParserPeek (
  XML_PARSER  *Parser,
  UINT32      N
  )
{
  UINT32  Position;

  if (!OcOverflowAddU32 (Parser->Position, N, &Position)
    && Position < Parser->Length) {
    return Parser->Buffer[Position];
  }

  return 0;
}


//
// Moves the parser's position n bytes. If the new position would be out of
// bounds, it will be converted to the bounds itself.
//
STATIC
VOID
XmlParserConsume (
  XML_PARSER  *Parser,
  UINT32      N
  )
{
#ifdef XML_PARSER_VERBOSE
  CHAR8   *Consumed;
  CHAR8   *MessageBuffer;
  UINT32  Left;

  //
  // Debug information.
  //

  Consumed = AllocatePool ((N + 1) * sizeof (CHAR8));
  MessageBuffer = AllocatePool (512 * sizeof (CHAR8));
  if (Consumed != NULL && MessageBuffer != NULL) {
    Left = N;
    if (Left > Parser->Length - Parser->Position) {
      Left = Parser->Length - Parser->Position;
    }

    CopyMem (Consumed, &Parser->Buffer[Parser->Position], Left);
    Consumed[Left] = 0;

    AsciiSPrint (MessageBuffer, 512, "Consuming %u bytes \"%a\"", N, Consumed);
    XML_PARSER_INFO (Parser, MessageBuffer);
  }

  if (Consumed != NULL) {
    FreePool (Consumed);
  }

  if (MessageBuffer != NULL) {
    FreePool (MessageBuffer);
  }
#endif

  //
  // Move the position forward.
  //
  if (OcOverflowAddU32 (Parser->Position, N, &Parser->Position)
    || Parser->Position > Parser->Length) {
    Parser->Position = Parser->Length;
  }
}

//
// Skips to the next non-whitespace character.
//
STATIC
VOID
XmlSkipWhitespace (
  XML_PARSER  *Parser
  )
{
  XML_PARSER_INFO (Parser, "whitespace");

  while (Parser->Position < Parser->Length
    && IsAsciiSpace (Parser->Buffer[Parser->Position])) {
    Parser->Position++;
  }
}

//
// Parses the name out of the an XML tag's ending.
//
// ---( Example )---
// tag_name>
// ---
//
STATIC
CONST CHAR8 *
XmlParseTagEnd (
  XML_PARSER   *Parser,
  BOOLEAN      *SelfClosing,
  CONST CHAR8  **Attributes
  )
{
  CHAR8   Current;
  UINT32  Start;
  UINT32  AttributeStart;
  UINT32  Length = 0;
  UINT32  NameLength = 0;

  XML_PARSER_INFO (Parser, "tag_end");

  Current = XmlParserPeek (Parser, CURRENT_CHARACTER);
  Start = Parser->Position;

  //
  // Parse until `>' or a whitespace is reached.
  //
  while (Start + Length < Parser->Length) {
    if (('/' == Current) || ('>' == Current)) {
      break;
    }

    if (NameLength == 0 && IsAsciiSpace (Current)) {
      NameLength = Length;

      if (NameLength == 0) {
        XML_PARSER_ERROR (Parser, CURRENT_CHARACTER, "XmlParseTagEnd::expected tag name");
        return NULL;
      }
    }

    XmlParserConsume (Parser, 1);
    Length++;

    Current = XmlParserPeek (Parser, CURRENT_CHARACTER);
  }

  //
  // Handle attributes.
  //
  if (NameLength != 0) {
    if (Attributes != NULL && (Current == '/' || Current == '>')) {
      *Attributes = &Parser->Buffer[Start + NameLength];
      AttributeStart = NameLength;
      while (AttributeStart < Length && IsAsciiSpace (**Attributes)) {
        (*Attributes)++;
        AttributeStart++;
      }
      Parser->Buffer[Start + Length] = '\0';
    }
  } else {
    //
    // No attributes besides name.
    //
    NameLength = Length;
  }

  if ('/' == Current) {
    if (SelfClosing == NULL) {
      XML_PARSER_ERROR (Parser, CURRENT_CHARACTER, "XmlParseTagEnd::unexpected self closing tag");
      return NULL;
    }

    *SelfClosing = TRUE;
    XmlParserConsume (Parser, 1);
    Current = XmlParserPeek (Parser, CURRENT_CHARACTER);
  }

  //
  // Consume `>'.
  //
  if ('>' != Current) {
    XML_PARSER_ERROR (Parser, CURRENT_CHARACTER, "XmlParseTagEnd::expected tag end");
    return NULL;
  }
  XmlParserConsume (Parser, 1);

  //
  // Return parsed tag name.
  //
  Parser->Buffer[Start + NameLength] = 0;
  XML_PARSER_TAG (Parser, &Parser->Buffer[Start]);
  return &Parser->Buffer[Start];
}

//
// Parses an opening XML tag without attributes.
//
// ---( Example )---
// <tag_name>
// ---
//
STATIC
CONST CHAR8 *
XmlParseTagOpen (
  XML_PARSER  *Parser,
  BOOLEAN     *SelfClosing,
  CONST CHAR8 **Attributes
  )
{
  CHAR8  Current;

  XML_PARSER_INFO (Parser, "tag_open");

  do {
    XmlSkipWhitespace (Parser);

    //
    // Consume `<'.
    //
    if ('<' != XmlParserPeek (Parser, CURRENT_CHARACTER)) {
      XML_PARSER_ERROR (Parser, CURRENT_CHARACTER, "XmlParseTagOpen::expected opening tag");
      return NULL;
    }
    XmlParserConsume (Parser, 1);

    Current = XmlParserPeek (Parser, CURRENT_CHARACTER);

    //
    // This is closing tag, e.g. `</tag>', return.
    //
    if (Current == '/') {
      return NULL;
    }

    //
    // This is not a control sequence, e.g. `<!DOCTYPE...>', continue parsing tag.
    //
    if (Current != '?' && Current != '!') {
      break;
    }

    //
    // Skip the control sequence.
    //
    do {
      XmlParserConsume (Parser, 1);
    } while (XmlParserPeek (Parser, CURRENT_CHARACTER) != '>' && Parser->Position < Parser->Length);
    XmlParserConsume (Parser, 1);

  } while (Parser->Position < Parser->Length);

  //
  // Consume tag name.
  //
  return XmlParseTagEnd (Parser, SelfClosing, Attributes);
}

//
// Parses an closing XML tag without attributes.
//
// ---( Example )---
// </tag_name>
// ---
//
STATIC
CONST CHAR8 *
XmlParseTagClose (
  XML_PARSER  *Parser,
  BOOLEAN     Unprefixed
  )
{
  XML_PARSER_INFO (Parser, "tag_close");
  XmlSkipWhitespace (Parser);

  if (Unprefixed) {
    //
    // Consume `/'.
    //
    if ('/' != XmlParserPeek (Parser, CURRENT_CHARACTER)) {
      XML_PARSER_ERROR (Parser, CURRENT_CHARACTER, "XmlParseTagClose::expected closing tag `/'");
      return NULL;
    }

    XmlParserConsume (Parser, 1);
  } else {
    //
    // Consume `</'.
    //
    if ('<' != XmlParserPeek (Parser, CURRENT_CHARACTER)) {
      XML_PARSER_ERROR (Parser, CURRENT_CHARACTER, "XmlParseTagClose::expected closing tag `<'");
      return NULL;
    }

    if ('/' != XmlParserPeek (Parser, NEXT_CHARACTER)) {
      XML_PARSER_ERROR (Parser, NEXT_CHARACTER, "XmlParseTagClose::expected closing tag `/'");
      return NULL;
    }

    XmlParserConsume (Parser, 2);
  }

  //
  // Consume tag name.
  //
  return XmlParseTagEnd(Parser, NULL, NULL);
}

//
// Parses a tag's content.
//
// ---( Example )---
//     this is
//   a
//       tag {} content
// ---
//
// @warning CDATA etc. is _not_ and will never be supported
//
STATIC
CONST CHAR8 *
XmlParseContent (
  XML_PARSER  *Parser
  )
{
  UINTN  Start;
  UINTN  Length;
  CHAR8  Current;

  XML_PARSER_INFO(Parser, "content");

  //
  // Whitespace will be ignored.
  //
  XmlSkipWhitespace (Parser);

  Start = Parser->Position;
  Length = 0;

  //
  // Consume until `<' is reached.
  //
  while (Start + Length < Parser->Length) {
    Current = XmlParserPeek (Parser, CURRENT_CHARACTER);

    if ('<' == Current) {
      break;
    } else {
      XmlParserConsume (Parser, 1);
      Length++;
    }
  }

  //
  // Next character must be an `<' or we have reached end of file.
  //
  if ('<' != XmlParserPeek (Parser, CURRENT_CHARACTER)) {
    XML_PARSER_ERROR (Parser, CURRENT_CHARACTER, "XmlParseContent::expected <");
    return NULL;
  }

  //
  // Ignore tailing whitespace.
  //
  while ((Length > 0) && IsAsciiSpace (Parser->Buffer[Start + Length - 1])) {
    Length--;
  }

  //
  // Return text.
  //
  Parser->Buffer[Start + Length] = 0;
  XmlParserConsume (Parser, 1);
  return &Parser->Buffer[Start];
}

//
// Prints to growing buffer always preserving one byte extra.
//
STATIC
VOID
XmlBufferAppend (
  CHAR8        **Buffer,
  UINT32       *AllocSize,
  UINT32       *CurrentSize,
  CONST CHAR8  *Data,
  UINT32       DataLength
  )
{
  CHAR8   *NewBuffer;
  UINT32  NewSize;

  NewSize = *AllocSize;

  if (NewSize - *CurrentSize <= DataLength) {
    if (DataLength + 1 <= XML_EXPORT_MIN_ALLOCATION_SIZE) {
      NewSize += XML_EXPORT_MIN_ALLOCATION_SIZE;
    } else {
      NewSize += DataLength + 1;
    }

    NewBuffer = AllocatePool (NewSize);
    if (NewBuffer == NULL) {
      XML_USAGE_ERROR("XmlBufferAppend::failed to allocate");
      return;
    }

    CopyMem (NewBuffer, *Buffer, *CurrentSize);
    FreePool (*Buffer);
    *Buffer    = NewBuffer;
    *AllocSize = NewSize;
  }

  CopyMem (&(*Buffer)[*CurrentSize], Data, DataLength);
  *CurrentSize += DataLength;
}

//
// Prints node to growing buffer always preserving one byte extra.
//
STATIC
VOID
XmlNodeExportRecursive (
  XML_NODE  *Node,
  CHAR8     **Buffer,
  UINT32    *AllocSize,
  UINT32    *CurrentSize,
  UINT32    Skip
  )
{
  UINT32  Index;
  UINT32  NameLength;

  if (Skip != 0) {
    if (Node->Children != NULL) {
      for (Index = 0; Index < Node->Children->NodeCount; ++Index) {
        XmlNodeExportRecursive (Node->Children->NodeList[Index], Buffer, AllocSize, CurrentSize, Skip - 1);
      }
    }

    return;
  }

  NameLength = (UINT32)AsciiStrLen (Node->Name);

  XmlBufferAppend (Buffer, AllocSize, CurrentSize, "<", L_STR_LEN ("<"));
  XmlBufferAppend (Buffer, AllocSize, CurrentSize, Node->Name, NameLength);

  if (Node->Attributes != NULL) {
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, " ", L_STR_LEN (" "));
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, Node->Attributes, (UINT32)AsciiStrLen (Node->Attributes));
  }

  if (Node->Children != NULL || Node->Content != NULL) {
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, ">", L_STR_LEN (">"));

    if (Node->Children != NULL) {
      for (Index = 0; Index < Node->Children->NodeCount; ++Index) {
        XmlNodeExportRecursive (Node->Children->NodeList[Index], Buffer, AllocSize, CurrentSize, 0);
      }
    } else {
      XmlBufferAppend (Buffer, AllocSize, CurrentSize, Node->Content, (UINT32)AsciiStrLen (Node->Content));
    }

    XmlBufferAppend (Buffer, AllocSize, CurrentSize, "</", L_STR_LEN ("</"));
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, Node->Name, NameLength);
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, ">", L_STR_LEN (">"));
  } else {
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, "/>", L_STR_LEN ("/>"));
  }
}

//
// Parses an XML fragment node.
//
// ---( Example without children )---
// <Node>Text</Node>
// ---
//
// ---( Example with children )---
// <Parent>
//     <Child>Text</Child>
//     <Child>Text</Child>
//     <Test>Content</Test>
// </Parent>
// ---
//
STATIC
XML_NODE *
XmlParseNode (
  XML_PARSER  *Parser,
  XML_REFLIST *References
  )
{
  CONST CHAR8  *TagOpen;
  CONST CHAR8  *TagClose;
  CONST CHAR8  *Attributes;
  XML_NODE     *Node;
  XML_NODE     *Child;
  UINT32       ReferenceNumber;
  BOOLEAN      IsReference;
  BOOLEAN      SelfClosing;
  BOOLEAN      Unprefixed;
  BOOLEAN      HasChildren;

  XML_PARSER_INFO (Parser, "node");

  Attributes  = NULL;
  SelfClosing = FALSE;
  Unprefixed  = FALSE;
  IsReference = FALSE;

  //
  // Parse open tag.
  //
  TagOpen = XmlParseTagOpen (Parser, &SelfClosing, &Attributes);
  if (TagOpen == NULL) {
    if ('/' != XmlParserPeek (Parser, CURRENT_CHARACTER)) {
      XML_PARSER_ERROR (Parser, NO_CHARACTER, "XmlParseNode::tag_open");
    }
    return NULL;
  }

  XmlSkipWhitespace (Parser);

  Node = XmlNodeCreate (TagOpen, Attributes, NULL, XmlNodeReal (References, Attributes), NULL);
  if (Node == NULL) {
    XML_PARSER_ERROR (Parser, NO_CHARACTER, "XmlParseNode::node alloc fail");
    return NULL;
  }

  //
  // If tag ends with `/' it's self closing, skip content lookup.
  //
  if (SelfClosing) {
    return Node;
  }

  //
  // If the content does not start with '<', a text content is assumed.
  //
  if ('<' != XmlParserPeek (Parser, CURRENT_CHARACTER)) {
    Node->Content = XmlParseContent (Parser);

    if (Node->Content == NULL) {
      XML_PARSER_ERROR (Parser, 0, "XmlParseNode::content");
      XmlNodeFree (Node);
      return NULL;
    }

    //
    // All references must be defined sequentially.
    //
    if (References != NULL && Node->Attributes != NULL) {
      IsReference = XmlParseAttributeNumber (
        Node->Attributes,
        "ID=\"",
        L_STR_LEN ("ID=\""),
        &ReferenceNumber
        );
    }

    Unprefixed = TRUE;

  //
  // Otherwise children are to be expected.
  //
  } else {
    Parser->Level++;

    if (Parser->Level > XML_PARSER_NEST_LEVEL) {
      XML_PARSER_ERROR (Parser, NO_CHARACTER, "XmlParseNode::level overflow");
      XmlNodeFree (Node);
      return NULL;
    }

    HasChildren = FALSE;

    while ('/' != XmlParserPeek (Parser, NEXT_CHARACTER)) {

      //
      // Parse child node.
      //
      Child = XmlParseNode (Parser, References);
      if (Child == NULL) {
        if ('/' == XmlParserPeek (Parser, CURRENT_CHARACTER)) {
          XML_PARSER_INFO (Parser, "child_end");
          Unprefixed = TRUE;
          break;
        }

        XML_PARSER_ERROR (Parser, NEXT_CHARACTER, "XmlParseNode::child");
        XmlNodeFree (Node);
        return NULL;
      }

      if (!XmlNodeChildPush (Node, Child)) {
        XML_PARSER_ERROR (Parser, NO_CHARACTER, "XmlParseNode::node push fail");
        XmlNodeFree (Node);
        XmlNodeFree (Child);
        return NULL;
      }

      HasChildren = TRUE;
    }

    Parser->Level--;

    if (!HasChildren && References != NULL && Attributes != NULL) {
      IsReference = XmlParseAttributeNumber (
        Node->Attributes,
        "ID=\"",
        L_STR_LEN ("ID=\""),
        &ReferenceNumber
        );
    }
  }

  //
  // Parse close tag.
  //
  TagClose = XmlParseTagClose (Parser, Unprefixed);
  if (TagClose == NULL) {
    XML_PARSER_ERROR (Parser, NO_CHARACTER, "XmlParseNode::tag close");
    XmlNodeFree (Node);
    return NULL;
  }

  //
  // Close tag has to match open tag.
  //
  if (AsciiStrCmp (TagOpen, TagClose) != 0) {
    XML_PARSER_ERROR (Parser, NO_CHARACTER, "XmlParseNode::tag missmatch");
    XmlNodeFree (Node);
    return NULL;
  }

  if (IsReference && !XmlPushReference (References, Node, ReferenceNumber)) {
    XML_PARSER_ERROR (Parser, 0, "XmlParseNode::reference");
    XmlNodeFree (Node);
    return NULL;
  }

  return Node;
}

XML_DOCUMENT *
XmlDocumentParse (
  CHAR8    *Buffer,
  UINT32   Length,
  BOOLEAN  WithRefs
  )
{
  XML_NODE      *Root;
  XML_DOCUMENT  *Document;
  XML_REFLIST   References;

  //
  // Initialize parser.
  //
  XML_PARSER Parser;
  ZeroMem (&Parser, sizeof (Parser));
  Parser.Buffer = Buffer;
  Parser.Length = Length;
  ZeroMem (&References, sizeof (References));

  //
  // An empty buffer can never contain a valid document.
  //
  if (Length == 0 || Length > XML_PARSER_MAX_SIZE) {
    XML_PARSER_ERROR (&Parser, NO_CHARACTER, "XmlDocumentParse::length is too small or too large");
    return NULL;
  }

  //
  // Parse the root node.
  //
  Root = XmlParseNode (&Parser, WithRefs ? &References : NULL);
  if (Root == NULL) {
    XML_PARSER_ERROR (&Parser, NO_CHARACTER, "XmlDocumentParse::parsing document failed");
    return NULL;
  }

  //
  // Return parsed document.
  //
  Document = AllocatePool (sizeof(XML_DOCUMENT));

  if (Document == NULL) {
    XML_PARSER_ERROR (&Parser, NO_CHARACTER, "XmlDocumentParse::document allocation failed");
    XmlNodeFree (Root);
    XmlFreeRefs (&References);
    return NULL;
  }

  Document->Buffer.Buffer = Buffer;
  Document->Buffer.Length = Length;
  Document->Root = Root;
  CopyMem (&Document->References, &References, sizeof (References));

  return Document;
}

CHAR8 *
XmlDocumentExport (
  XML_DOCUMENT  *Document,
  UINT32        *Length,
  UINT32        Skip
  )
{
  CHAR8   *Buffer;
  UINT32  AllocSize;
  UINT32  CurrentSize;

  AllocSize = Document->Buffer.Length + 1;
  Buffer    = AllocatePool (AllocSize);
  if (Buffer == NULL) {
    XML_USAGE_ERROR ("XmlDocumentExport::failed to allocate");
    return NULL;
  }

  CurrentSize = 0;
  XmlNodeExportRecursive (Document->Root, &Buffer, &AllocSize, &CurrentSize, Skip);

  if (Length != NULL) {
    *Length = CurrentSize;
  }

  //
  // XmlBufferAppend guarantees one more byte.
  //
  Buffer[CurrentSize] = '\0';

  return Buffer;
}

VOID
XmlDocumentFree (
  XML_DOCUMENT  *Document
  )
{
  XmlNodeFree (Document->Root);
  XmlFreeRefs (&Document->References);
  FreePool (Document);
}

XML_NODE *
XmlDocumentRoot (
  XML_DOCUMENT  *Document
  )
{
  return Document->Root;
}

CONST CHAR8 *
XmlNodeName (
  XML_NODE  *Node
  )
{
  return Node->Name;
}

CONST CHAR8 *
XmlNodeContent (
  XML_NODE  *Node
  )
{
  return Node->Real != NULL ? Node->Real->Content : Node->Content;
}

UINT32
XmlNodeChildren (
  XML_NODE  *Node
  )
{
  return Node->Children ? Node->Children->NodeCount : 0;
}

XML_NODE  *
XmlNodeChild (
  XML_NODE  *Node,
  UINT32    Child
  )
{
  return Node->Children->NodeList[Child];
}

XML_NODE *
EFIAPI
XmlEasyChild (
  XML_NODE     *Node,
  CONST CHAR8  *ChildName,
  ...)
{
  VA_LIST   Arguments;
  XML_NODE  *Next;
  XML_NODE  *Child;
  UINT32    Index;

  VA_START (Arguments, ChildName);

  //
  // Descent to current child.
  //
  while (ChildName != NULL) {
    //
    // Interate through all children.
    //
    Next = NULL;

    for (Index = 0; Index < XmlNodeChildren (Node); ++Index) {
      Child = XmlNodeChild (Node, Index);

      if (AsciiStrCmp (XmlNodeName (Child), ChildName) != 0) {
        if (Next == NULL) {
          Next = Child;
        } else {
          //
          // Two children with the same name.
          //
          VA_END (Arguments);
          return NULL;
        }
      }
    }

    //
    // No child with that name found.
    //
    if (Next == NULL) {
      VA_END (Arguments);
      return NULL;
    }

    Node = Next;

    //
    // Find name of next child.
    //
    ChildName = VA_ARG (Arguments, CONST CHAR8*);
  }
  VA_END (Arguments);

  //
  // Return current element.
  //
  return Node;
}

XML_NODE *
XmlNodeAppend (
  XML_NODE     *Node,
  CONST CHAR8  *Name,
  CONST CHAR8  *Attributes,
  CONST CHAR8  *Content
  )
{
  XML_NODE  *NewNode;

  NewNode = XmlNodeCreate (Name, Attributes, Content, NULL, NULL);
  if (NewNode == NULL) {
    return NULL;
  }

  if (!XmlNodeChildPush (Node, NewNode)) {
    XmlNodeFree (NewNode);
    return NULL;
  }

  return NewNode;
}

XML_NODE *
XmlNodePrepend (
  XML_NODE     *Node,
  CONST CHAR8  *Name,
  CONST CHAR8  *Attributes,
  CONST CHAR8  *Content
  )
{
  XML_NODE  *NewNode;

  NewNode = XmlNodeAppend (Node, Name, Attributes, Content);
  if (NewNode == NULL) {
    return NULL;
  }

  CopyMem (&Node->Children->NodeList[1], &Node->Children->NodeList[0], (Node->Children->NodeCount - 1) * sizeof (Node->Children->NodeList[0]));
  Node->Children->NodeList[0] = NewNode;

  return NewNode;
}

XML_NODE *
PlistDocumentRoot (
  XML_DOCUMENT  *Document
  )
{
  XML_NODE  *Node;

  Node = Document->Root;

  if (AsciiStrCmp (XmlNodeName (Node), "plist") != 0) {
    XML_USAGE_ERROR ("PlistDocumentRoot::not plist root");
    return NULL;
  }

  if (XmlNodeChildren (Node) != 1) {
    XML_USAGE_ERROR ("PlistDocumentRoot::no single first node");
    return NULL;
  }

  return XmlNodeChild(Node, 0);
}

XML_NODE *
PlistNodeCast (
  XML_NODE         *Node,
  PLIST_NODE_TYPE  Type
  )
{
  UINT32  ChildrenNum;

  if (Node == NULL || Type == PLIST_NODE_TYPE_ANY) {
    return Node;
  }

  if (AsciiStrCmp (XmlNodeName (Node), PlistNodeTypes[Type]) != 0) {
    // XML_USAGE_ERROR ("PlistNodeType::wrong type");
    return NULL;
  }

  ChildrenNum = XmlNodeChildren (Node);

  switch (Type) {
    case PLIST_NODE_TYPE_DICT:
      if (ChildrenNum % 2 != 0) {
        XML_USAGE_ERROR ("PlistNodeType::dict has odd children");
        return NULL;
      }
      break;
    case PLIST_NODE_TYPE_ARRAY:
      break;
    case PLIST_NODE_TYPE_KEY:
    case PLIST_NODE_TYPE_INTEGER:
    case PLIST_NODE_TYPE_REAL:
      if (XmlNodeContent (Node) == NULL) {
        XML_USAGE_ERROR ("PlistNodeType::key or int have no content");
        return NULL;
      }
      // Fallthrough
    default:
      //
      // Only dictionaries and arrays are allowed to have child nodes.
      //
      if (ChildrenNum > 0) {
        XML_USAGE_ERROR ("PlistNodeType::non dict array has children");
        return NULL;
      }
      break;
  }

  return Node;
}

UINT32
PlistDictChildren (
  XML_NODE     *Node
  )
{
  return XmlNodeChildren (Node) / 2;
}

XML_NODE *
PlistDictChild (
  XML_NODE     *Node,
  UINT32       Child,
  XML_NODE     **Value OPTIONAL
  )
{
  Child *= 2;

  if (Value != NULL) {
    *Value = XmlNodeChild (Node, Child + 1);
  }

  return XmlNodeChild (Node, Child);
}

CONST CHAR8 *
PlistKeyValue (
  XML_NODE  *Node
  )
{
 if (PlistNodeCast (Node, PLIST_NODE_TYPE_KEY) == NULL) {
    return NULL;
  }

  return XmlNodeContent (Node);
}

BOOLEAN
PlistStringValue (
  XML_NODE  *Node,
  CHAR8     *Value,
  UINT32    *Size
  )
{
  CONST CHAR8  *Content;
  UINTN        Length;

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_STRING) == NULL) {
    return FALSE;
  }

  Content = XmlNodeContent (Node);
  if (Content == NULL) {
    Value[0] = '\0';
    *Size = 1;
    return TRUE;
  }

  Length = AsciiStrLen (Content);
  if (Length < *Size) {
    *Size = (UINT32) (Length + 1);
  }

  AsciiStrnCpyS (Value, *Size, Content, Length);
  return TRUE;
}

BOOLEAN
PlistDataValue (
  XML_NODE  *Node,
  UINT8     *Buffer,
  UINT32    *Size
  )
{
  CONST CHAR8    *Content;
  UINTN          Length;
  EFI_STATUS     Result;

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_DATA) == NULL) {
    return FALSE;
  }

  Content = XmlNodeContent (Node);
  if (Content == NULL) {
    *Size = 0;
    return TRUE;
  }

  Length = *Size;
  Result = Base64Decode (Content, AsciiStrLen (Content), Buffer, &Length);

  if (!EFI_ERROR (Result) && (UINT32) Length == Length) {
    *Size = (UINT32) Length;
    return TRUE;
  }

  *Size = 0;
  return FALSE;
}

BOOLEAN
PlistBooleanValue (
  XML_NODE  *Node,
  BOOLEAN   *Value
  )
{
  if (PlistNodeCast (Node, PLIST_NODE_TYPE_TRUE) != NULL) {
    *Value = TRUE;
    return TRUE;
  }

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_FALSE) != NULL) {
    *Value = FALSE;
    return TRUE;
  }

  return FALSE;
}

BOOLEAN
PlistIntegerValue (
  XML_NODE  *Node,
  VOID      *Value,
  UINT32    Size,
  BOOLEAN   Hex
  )
{
  UINT64       Temp;
  CONST CHAR8  *TempStr;
  BOOLEAN      Negate;

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_INTEGER) == NULL) {
    return FALSE;
  }

  TempStr = XmlNodeContent (Node);

  while (*TempStr == ' ' || *TempStr == '\t') {
    ++TempStr;
  }

  Negate = *TempStr == '-';

  if (Negate) {
    ++TempStr;
  }

  if (Hex && TempStr[0] != '0' && TempStr[1] != 'x') {
    Hex = FALSE;
  }

  if (Hex) {
    Temp = AsciiStrHexToUint64 (TempStr);
  } else {
    Temp = AsciiStrDecimalToUint64 (TempStr);
  }

  //
  // May produce unexpected results when the value is too large, but just do not care.
  //
  if (Negate) {
    Temp = 0ULL - Temp;
  }

  switch (Size) {
    case sizeof (UINT64):
      *(UINT64 *) Value = Temp;
      return TRUE;
    case sizeof (UINT32):
      *(UINT32 *) Value = (UINT32) Temp;
      return TRUE;
    case sizeof (UINT16):
      *(UINT16 *) Value = (UINT16) Temp;
      return TRUE;
    case sizeof (UINT8):
      *(UINT8 *) Value = (UINT8) Temp;
      return TRUE;
    default:
      return FALSE;
  }
}

BOOLEAN
PlistMetaDataValue (
  XML_NODE  *Node,
  VOID      *Buffer,
  UINT32    *Size
  )
{
  CONST CHAR8    *Content;
  UINTN          Length;
  EFI_STATUS     Result;

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_DATA) != NULL) {
    Content = XmlNodeContent (Node);
    if (Content != NULL) {

      Length = *Size;
      Result = Base64Decode (Content, AsciiStrLen (Content), Buffer, &Length);

      if (!EFI_ERROR (Result) && (UINT32) Length == Length) {
        *Size = (UINT32) Length;
      } else {
        return FALSE;
      }
    } else {
      *Size = 0;
    }
    return TRUE;
  }

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_STRING) != NULL) {
    Content = XmlNodeContent (Node);
    if (Content != NULL) {
      Length = AsciiStrLen (Content);
      if (Length < *Size) {
        *Size = (UINT32) (Length + 1);
      }

      AsciiStrnCpyS (Buffer, *Size, Content, Length);
    } else {
      *(CHAR8 *) Buffer = '\0';
      *Size = 1;
    }
    return TRUE;
  }

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_INTEGER) != NULL) {
    *(UINT32 *) Buffer = (UINT32) AsciiStrDecimalToUint64 (XmlNodeContent (Node));
    *Size = sizeof (UINT32);
    return TRUE;
  }

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_TRUE) != NULL) {
    *(UINT8 *) Buffer = 1;
    *Size = sizeof (UINT8);
    return TRUE;
  }

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_FALSE) != NULL) {
    *(UINT8 *) Buffer = 0;
    *Size = sizeof (UINT8);
    return TRUE;
  }

  return FALSE;
}

BOOLEAN
PlistStringSize (
  XML_NODE  *Node,
  UINT32    *Size
  )
{
  CONST CHAR8  *Content;

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_STRING) == NULL) {
    return FALSE;
  }

  Content = XmlNodeContent (Node);
  if (Content != NULL) {
    *Size = (UINT32) AsciiStrLen (Content) + 1;
    return TRUE;
  }

  *Size = 0;
  return TRUE;
}

BOOLEAN
PlistDataSize (
  XML_NODE  *Node,
  UINT32    *Size
  )
{
  CONST CHAR8  *Content;

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_DATA) == NULL) {
    return FALSE;
  }

  Content = XmlNodeContent (Node);
  if (Content != NULL) {
    *Size = (UINT32) AsciiStrLen (Content);
  } else {
    *Size = 0;
  }

  return TRUE;
}

BOOLEAN
PlistMetaDataSize (
  XML_NODE  *Node,
  UINT32    *Size
  )
{
  CONST CHAR8  *Content;

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_DATA) != NULL) {
    Content = XmlNodeContent (Node);
    if (Content != NULL) {
      *Size = (UINT32) AsciiStrLen (Content);
    } else {
      *Size = 0;
    }
    return TRUE;
  }

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_STRING) != NULL) {
    Content = XmlNodeContent (Node);
    if (Content != NULL) {
      *Size = (UINT32) (AsciiStrLen (Content) + 1);
    } else {
      *Size = 0;
    }
    return TRUE;
  }

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_INTEGER) != NULL) {
    *Size = sizeof (UINT32);
    return TRUE;
  }

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_TRUE) != NULL
    || PlistNodeCast (Node, PLIST_NODE_TYPE_FALSE) != NULL) {
    *Size = sizeof (UINT8);
    return TRUE;
  }

  return FALSE;
}
