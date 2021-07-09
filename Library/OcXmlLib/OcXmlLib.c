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

/**
  Minimal extra allocation size during export.
**/
#define XML_EXPORT_MIN_ALLOCATION_SIZE 4096

#define XML_PLIST_HEADER  "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"

struct XML_NODE_LIST_;
struct XML_PARSER_;

typedef struct XML_NODE_LIST_ XML_NODE_LIST;
typedef struct XML_PARSER_ XML_PARSER;

/**
  An XML_NODE will always contain a tag name and possibly a list of
  children or text content.
**/
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

/**
  An XML_DOCUMENT simply contains the root node and the underlying buffer.
**/
struct XML_DOCUMENT_ {
  struct {
    CHAR8       *Buffer;
    UINT32      Length;
  } Buffer;

  XML_NODE      *Root;
  XML_REFLIST   References;
};

/**
  Parser context.
**/
struct XML_PARSER_ {
  CHAR8  *Buffer;
  UINT32 Position;
  UINT32 Length;
  UINT32 Level;
};

/**
  Character offsets.
**/
typedef enum XML_PARSER_OFFSET_ {
  NO_CHARACTER        = -1,
  CURRENT_CHARACTER   = 0,
  NEXT_CHARACTER      = 1,
} XML_PARSER_OFFSET;

/**
  Plist node types.
**/
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

/**
  Parse the attribute number.

  @param[in]   Attributes      XML attributes.
  @param[in]   Argument        Name of the XML argument.
  @param[in]   ArgumentLength  Length of the XML argument.
  @param[out]  ArgumentValue   The parsed XML argument value.

  @retval  TRUE on successful parsing.
**/
STATIC
BOOLEAN
XmlParseAttributeNumber (
  IN  CONST CHAR8   *Attributes,
  IN  CONST CHAR8   *Argument,
  IN  UINT32        ArgumentLength,
  OUT UINT32        *ArgumentValue
  )
{
  CONST CHAR8  *ArgumentStart;
  CONST CHAR8  *ArgumentEnd;
  UINTN        Number;
  CHAR8        NumberStr[16];

  ASSERT (Attributes    != NULL);
  ASSERT (Argument      != NULL);
  ASSERT (ArgumentValue != NULL);

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

/**
  Create a new XML node.

  @param[in]  Name        Name of the new node.
  @param[in]  Attributes  Attributes of the new node. Optional.
  @param[in]  Content     Content of the new node. Optional.
  @param[in]  Real        Pointer to the acual content when a reference exists. Optional.
  @param[in]  Children    Pointer to the children of the node. Optional.

  @return  The created XML node.
**/
STATIC
XML_NODE *
XmlNodeCreate (
  IN  CONST CHAR8          *Name,
  IN  CONST CHAR8          *Attributes  OPTIONAL,
  IN  CONST CHAR8          *Content     OPTIONAL,
  IN  XML_NODE             *Real        OPTIONAL,
  IN  XML_NODE_LIST        *Children    OPTIONAL
  )
{
  XML_NODE  *Node;

  ASSERT (Name != NULL);

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

/**
  Add a child node to the node given.

  @param[in,out]  Node   Pointer to the XML node to which the child will be added.
  @param[in]      Child  Pointer to the child XML node.

  @retval  TRUE on successful adding.
**/
STATIC
BOOLEAN
XmlNodeChildPush (
  IN OUT  XML_NODE  *Node,
  IN      XML_NODE  *Child
  )
{
  UINT32         NodeCount;
  UINT32         AllocCount;
  XML_NODE_LIST  *NewList;

  ASSERT (Node  != NULL);
  ASSERT (Child != NULL);

  NodeCount  = 0;
  AllocCount = 1;

  //
  // Push new node if there is enough room.
  //
  if (Node->Children != NULL) {
    NodeCount = Node->Children->NodeCount;
    AllocCount = Node->Children->AllocCount;

    if (NodeCount < XML_PARSER_NODE_COUNT && AllocCount > NodeCount) {
      Node->Children->NodeList[NodeCount] = Child;
      ++Node->Children->NodeCount;
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

/**
  Store XML reference.

  @param[in,out]  References       A pointer to the list of XML references.
  @param[in]      Node             A pointer to the XML node.
  @param[in]      ReferenceNumber  Number of reference.

  @retval  TRUE if the XML reference was successfully pushed.
**/
STATIC
BOOLEAN
XmlPushReference (
  IN OUT  XML_REFLIST  *References,
  IN      XML_NODE     *Node,
  IN      UINT32       ReferenceNumber
  )
{
  XML_NODE   **NewReferences;
  UINT32     NewRefAllocCount;

  ASSERT (References != NULL);
  ASSERT (Node       != NULL);

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

/**
  Get real value from a node referencing another.

  @param[in]  References  A pointer to the XML references. Optional.
  @param[in]  Attributes  XML attributes. Optional.

  @return  The real XML node from the one referencing it.
**/
STATIC
XML_NODE *
XmlNodeReal (
  IN  CONST XML_REFLIST  *References  OPTIONAL,
  IN  CONST CHAR8        *Attributes  OPTIONAL
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

/**
  Free the resources allocated by the node.

  @param[in,out]  Node  A pointer to the XML node to be freed.
**/
STATIC
VOID
XmlNodeFree (
  IN OUT  XML_NODE  *Node
  )
{
  UINT32  Index;

  ASSERT (Node != NULL);

  if (Node->Children != NULL) {
    for (Index = 0; Index < Node->Children->NodeCount; ++Index) {
      XmlNodeFree (Node->Children->NodeList[Index]);
    }
    FreePool (Node->Children);
  }

  FreePool (Node);
}

/**
  Free the XML references.

  @param[in,out]  References  A pointer to the XML references to be freed.
**/
STATIC
VOID
XmlFreeRefs (
  IN OUT  XML_REFLIST  *References
  )
{
  ASSERT (References != NULL);

  if (References->RefList != NULL) {
    FreePool (References->RefList);
    References->RefList = NULL;
  }
}

/**
  Echo the parsers call stack for debugging purposes.
**/
#ifdef XML_PARSER_VERBOSE
#define XML_PARSER_INFO(Parser, Message) \
  DEBUG ((DEBUG_VERBOSE, "OCXML: XML_PARSER_INFO %a\n", Message));
#define XML_PARSER_TAG(Parser, Tag) \
  DEBUG ((DEBUG_VERBOSE, "OCXML: XML_PARSER_TAG %a\n", Tag));
#else
#define XML_PARSER_INFO(Parser, Message) do {} while (0)
#define XML_PARSER_TAG(Parser, Tag) do {} while (0)
#endif

/**
  Echo an error regarding the parser's source to the console.

  @param[in]  Parser   A pointer to the XML parser.
  @param[in]  Offset   Offset of the XML parser.
  @param[in]  Message  Message to be displayed
**/
VOID
XmlParserError (
  IN  CONST XML_PARSER   *Parser,
  IN  XML_PARSER_OFFSET  Offset,
  IN  CONST CHAR8        *Message
  )
{
  UINT32  Character;
  UINT32  Position;
  UINT32  Row;
  UINT32  Column;

  ASSERT (Parser  != NULL);
  ASSERT (Message != NULL);

  Character = 0;
  Row       = 0;
  Column    = 0;

  if (Parser->Length > 0 && (Parser->Position > 0 || NO_CHARACTER != Offset)) {
    Character = Parser->Position + Offset;
    if (Character > Parser->Length-1) {
      Character = Parser->Length-1;
    }

    for (Position = 0; Position <= Character; ++Position) {
      ++Column;

      if ('\n' == Parser->Buffer[Position]) {
        ++Row;
        Column = 0;
      }
    }
  }

  if (NO_CHARACTER != Offset) {
    DEBUG ((
      DEBUG_INFO, "OCXML: XmlParserError at %u:%u (is %c): %a\n",
      Row + 1,
      Column,
      Parser->Buffer[Character],
      Message
      ));
  } else {
    DEBUG ((
      DEBUG_INFO, "OCXML: XmlParserError at %u:%u: %a\n",
      Row + 1,
      Column,
      Message
      ));
  }
}

/**
  Conditionally enable error printing.
**/
#ifdef XML_PRINT_ERRORS
#define XML_PARSER_ERROR(Parser, Offset, Message) \
  XmlParserError (Parser, Offset, Message)
#define XML_USAGE_ERROR(Message) \
  DEBUG ((DEBUG_VERBOSE, "OCXML: %a\n", Message));
#else
#define XML_PARSER_ERROR(Parser, Offset, Message) do {} while (0)
#define XML_USAGE_ERROR(X) do {} while (0)
#endif

/**
  Return the N-th not-whitespace byte in parser or 0 if such a byte does not exist.

  @param[in]  Parser  A pointer to the XML parser.
  @param[in]  N       The N-th index.

  @return The N-th not-whitespace byte or 0.
**/
STATIC
CHAR8
XmlParserPeek (
  IN  CONST XML_PARSER  *Parser,
  IN  UINT32            N
  )
{
  UINT32  Position;

  ASSERT (Parser != NULL);

  if (!OcOverflowAddU32 (Parser->Position, N, &Position)
    && Position < Parser->Length) {
    return Parser->Buffer[Position];
  }

  return 0;
}


/**
  Move the parser's position n bytes. If the new position would be out of
  bounds, it will be converted to the bounds itself.

  @param[in,out]  Parser  A pointer to the XML parser.
  @param[in]      N       The N-th index.
**/
STATIC
VOID
XmlParserConsume (
  IN OUT  XML_PARSER  *Parser,
  IN      UINT32      N
  )
{
  ASSERT (Parser != NULL);

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

/**
  Skip to the next non-whitespace character.

  @param[in,out]  Parser  A pointer to the XML parser.
**/
STATIC
VOID
XmlSkipWhitespace (
  IN OUT  XML_PARSER  *Parser
  )
{
  ASSERT (Parser != NULL);

  XML_PARSER_INFO (Parser, "whitespace");

  while (Parser->Position < Parser->Length
    && IsAsciiSpace (Parser->Buffer[Parser->Position])) {
    ++Parser->Position;
  }
}

/**
  Parse the name out of the an XML tag's ending.

  ---( Example )---
  tag_name>
  ---

  @param[in,out]  Parser       A pointer to the XML parser.
  @param[out]     SelfClosing  TRUE to indicate self-closing. Optional.
  @param[out]     Attributes   Exported XML attributes. Optional.

  @return The XML tag in the end.
**/
STATIC
CONST CHAR8 *
XmlParseTagEnd (
  IN OUT  XML_PARSER   *Parser,
     OUT  BOOLEAN      *SelfClosing  OPTIONAL,
     OUT  CONST CHAR8  **Attributes  OPTIONAL
  )
{
  CHAR8   Current;
  UINT32  Start;
  UINT32  AttributeStart;
  UINT32  Length = 0;
  UINT32  NameLength = 0;

  ASSERT (Parser != NULL);

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
    ++Length;

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
        ++(*Attributes);
        ++AttributeStart;
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

/**
  Parse an opening XML tag without attributes.

  ---( Example )---
  <tag_name>
  ---

  @param[in,out]  Parser       A pointer to the XML parser.
  @param[out]     SelfClosing  TRUE to indicate self-closing. Optional.
  @param[out]     Attributes   Exported XML attributes. Optional.

  @return The parsed opening XML tag.
**/
STATIC
CONST CHAR8 *
XmlParseTagOpen (
  IN OUT  XML_PARSER   *Parser,
     OUT  BOOLEAN      *SelfClosing  OPTIONAL,
     OUT  CONST CHAR8  **Attributes
  )
{
  CHAR8    Current;
  CHAR8    Next;
  BOOLEAN  IsComment;

  ASSERT (Parser     != NULL);
  ASSERT (Attributes != NULL);

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
    // A crazy XML comment may look like this:
    // <!-- som>>><<<<ething -->
    //
    // '<' has already been consumed a bit earlier, now continue to check '!',
    // and then the two '-'.
    //
    IsComment = FALSE;
    if (Current == '!') {
      //
      // Consume one more byte to check the two `-'.
      // Now `<!--' is guaranteed.
      //
      XmlParserConsume (Parser, 1);
      Current   = XmlParserPeek (Parser, CURRENT_CHARACTER);
      Next      = XmlParserPeek (Parser, NEXT_CHARACTER);
      if (Current == '-' && Next == '-') {
        //
        // Now consume Current and Next which take up 2 bytes.
        //
        XmlParserConsume (Parser, 2);
        IsComment = TRUE;
      }
    }

    //
    // Skip the control sequence.
    // NOTE: This inner loop is created mainly for code simplification.
    //
    while (Parser->Position < Parser->Length) {
      if (IsComment) {
        //
        // Scan `-->' for comments and break if matched.
        //
        if (XmlParserPeek (Parser, CURRENT_CHARACTER) == '-'
            && XmlParserPeek (Parser, NEXT_CHARACTER) == '-'
            && XmlParserPeek (Parser, 2) == '>') {
          //
          // `-->' should all be consumed, which takes 3 bytes.
          //
          XmlParserConsume (Parser, 3);
          break;
        }
      } else {
        //
        // For non-comments, simply match '>'.
        //
        if (XmlParserPeek (Parser, CURRENT_CHARACTER) == '>') {
          XmlParserConsume (Parser, 1);
          break;
        }
      }

      //
      // Consume each byte normally.
      //
      XmlParserConsume (Parser, 1);
    }
  } while (Parser->Position < Parser->Length);

  //
  // Consume tag name.
  //
  return XmlParseTagEnd (Parser, SelfClosing, Attributes);
}

/**
  Parse an closing XML tag without attributes.

  ---( Example )---
  </tag_name>
  ---

  @param[in,out]  Parser      A pointer to the XML parser.
  @param[in]      Unprefixed  TRUE to parse without the starting `<'.

  @return The parsed closing XML tag.
**/
STATIC
CONST CHAR8 *
XmlParseTagClose (
  IN OUT  XML_PARSER  *Parser,
  IN      BOOLEAN     Unprefixed
  )
{
  ASSERT (Parser != NULL);

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
  return XmlParseTagEnd (Parser, NULL, NULL);
}

/**
  Parse a tag's content.

  ---( Example )---
      this is
    a
        tag {} content
  ---

  @warning CDATA etc. is _not_ and will never be supported

  @param[in,out]  Parser  A pointer to the XML parser.

  @return The parsed content of the tag.
**/
STATIC
CONST CHAR8 *
XmlParseContent (
  IN OUT  XML_PARSER  *Parser
  )
{
  UINTN  Start;
  UINTN  Length;
  CHAR8  Current;

  ASSERT (Parser != NULL);

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
      ++Length;
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
    --Length;
  }

  //
  // Return text.
  //
  Parser->Buffer[Start + Length] = 0;
  XmlParserConsume (Parser, 1);
  return &Parser->Buffer[Start];
}

/**
  Print to growing buffer always preserving one byte extra.

  @param[in,out]  Buffer       A pointer to the buffer holding contents.
  @param[in,out]  AllocSize    Size of Buffer to be allocated.
  @param[in,out]  CurrentSize  Current size of Buffer before appending.
  @param[in]      Data         Data to be appended.
  @param[in]      DataLength   Length of Data.
**/
STATIC
VOID
XmlBufferAppend (
  IN OUT  CHAR8        **Buffer,
  IN OUT  UINT32       *AllocSize,
  IN OUT  UINT32       *CurrentSize,
  IN      CONST CHAR8  *Data,
  IN      UINT32       DataLength
  )
{
  CHAR8   *NewBuffer;
  UINT32  NewSize;

  ASSERT (Buffer      != NULL);
  ASSERT (AllocSize   != NULL);
  ASSERT (CurrentSize != NULL);
  ASSERT (Data        != NULL);

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

/**
  Print node to growing buffer always preserving one byte extra.

  @param[in]      Node         A pointer to the XML node.
  @param[in,out]  Buffer       A pointer to the buffer holding contents exported.
  @param[in,out]  AllocSize    Size of Buffer to be allocated.
  @param[in,out]  CurrentSize  Current size of Buffer.
  @param[in]      Skip         Levels of XML contents to be skipped.
**/
STATIC
VOID
XmlNodeExportRecursive (
  IN      CONST XML_NODE  *Node,
  IN OUT  CHAR8           **Buffer,
  IN OUT  UINT32          *AllocSize,
  IN OUT  UINT32          *CurrentSize,
  IN      UINT32          Skip
  )
{
  UINT32  Index;
  UINT32  NameLength;

  ASSERT (Node        != NULL);
  ASSERT (Buffer      != NULL);
  ASSERT (AllocSize   != NULL);
  ASSERT (CurrentSize != NULL);

  if (Skip != 0) {
    if (Node->Children != NULL) {
      for (Index = 0; Index < Node->Children->NodeCount; ++Index) {
        XmlNodeExportRecursive (Node->Children->NodeList[Index], Buffer, AllocSize, CurrentSize, Skip - 1);
      }
    }

    return;
  }

  NameLength = (UINT32) AsciiStrLen (Node->Name);

  XmlBufferAppend (Buffer, AllocSize, CurrentSize, "<", L_STR_LEN ("<"));
  XmlBufferAppend (Buffer, AllocSize, CurrentSize, Node->Name, NameLength);

  if (Node->Attributes != NULL) {
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, " ", L_STR_LEN (" "));
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, Node->Attributes, (UINT32) AsciiStrLen (Node->Attributes));
  }

  if (Node->Children != NULL || Node->Content != NULL) {
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, ">", L_STR_LEN (">"));

    if (Node->Children != NULL) {
      for (Index = 0; Index < Node->Children->NodeCount; ++Index) {
        XmlNodeExportRecursive (Node->Children->NodeList[Index], Buffer, AllocSize, CurrentSize, 0);
      }
    } else {
      XmlBufferAppend (Buffer, AllocSize, CurrentSize, Node->Content, (UINT32) AsciiStrLen (Node->Content));
    }

    XmlBufferAppend (Buffer, AllocSize, CurrentSize, "</", L_STR_LEN ("</"));
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, Node->Name, NameLength);
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, ">", L_STR_LEN (">"));
  } else {
    XmlBufferAppend (Buffer, AllocSize, CurrentSize, "/>", L_STR_LEN ("/>"));
  }
}

/**
  Parse an XML fragment node.

  ---( Example without children )---
  <Node>Text</Node>
  ---

  ---( Example with children )---
  <Parent>
      <Child>Text</Child>
      <Child>Text</Child>
      <Test>Content</Test>
  </Parent>
  ---

  @param[in,out]  Parser      A pointer to the XML parser.
  @param[in,out]  References  A pointer to the XML references. Optional.

  @return  The parsed XML node.
**/
STATIC
XML_NODE *
XmlParseNode (
  IN OUT  XML_PARSER   *Parser,
  IN OUT  XML_REFLIST  *References  OPTIONAL
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

  ASSERT (Parser != NULL);

  XML_PARSER_INFO (Parser, "node");

  Attributes      = NULL;
  SelfClosing     = FALSE;
  Unprefixed      = FALSE;
  IsReference     = FALSE;
  ReferenceNumber = 0;

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
    ++Parser->Level;

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

    --Parser->Level;

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
  IN OUT  CHAR8    *Buffer,
  IN      UINT32   Length,
  IN      BOOLEAN  WithRefs
  )
{
  XML_NODE      *Root;
  XML_DOCUMENT  *Document;
  XML_REFLIST   References;
  XML_PARSER    Parser;

  ASSERT (Buffer != NULL);

  //
  // Initialize parser.
  //
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
  IN   CONST XML_DOCUMENT  *Document,
  OUT  UINT32              *Length  OPTIONAL,
  IN   UINT32              Skip,
  IN   BOOLEAN             PrependPlistInfo
  )
{
  CHAR8   *Buffer;
  CHAR8   *NewBuffer;
  UINT32  AllocSize;
  UINT32  CurrentSize;
  UINT32  NewSize;

  ASSERT (Document != NULL);

  AllocSize = Document->Buffer.Length + 1;
  Buffer    = AllocatePool (AllocSize);
  if (Buffer == NULL) {
    XML_USAGE_ERROR ("XmlDocumentExport::failed to allocate");
    return NULL;
  }

  CurrentSize = 0;
  XmlNodeExportRecursive (Document->Root, &Buffer, &AllocSize, &CurrentSize, Skip);

  if (PrependPlistInfo) {
    //
    // XmlNodeExportRecursive returns a size that does not include the null terminator,
    // but the allocated buffer does. During this reallocation, we count the null terminator
    // of the plist header instead to ensure allocated buffer is the proper size.
    //
    if (OcOverflowAddU32 (CurrentSize, L_STR_SIZE (XML_PLIST_HEADER), &NewSize)) {
      FreePool (Buffer);
      return NULL;
    }

    NewBuffer = AllocatePool (NewSize);
    if (NewBuffer == NULL) {
      FreePool (Buffer);
      XML_USAGE_ERROR ("XmlDocumentExport::failed to allocate");
      return NULL;
    }
    CopyMem (NewBuffer, XML_PLIST_HEADER, L_STR_SIZE_NT (XML_PLIST_HEADER));
    CopyMem (&NewBuffer[L_STR_LEN (XML_PLIST_HEADER)], Buffer, CurrentSize);
    FreePool (Buffer);

    //
    // Null terminator is not included in size returned by XmlBufferAppend.
    //
    CurrentSize = NewSize - 1;
    Buffer      = NewBuffer;
  }

  if (Length != NULL) {
    *Length = CurrentSize;
  }

  //
  // Null terminator is not included in size returned by XmlBufferAppend,
  // but the buffer is allocated to include it.
  //
  Buffer[CurrentSize] = '\0';

  return Buffer;
}

VOID
XmlDocumentFree (
  IN OUT  XML_DOCUMENT  *Document
  )
{
  ASSERT (Document != NULL);

  XmlNodeFree (Document->Root);
  XmlFreeRefs (&Document->References);
  FreePool (Document);
}

XML_NODE *
XmlDocumentRoot (
  IN  CONST XML_DOCUMENT  *Document
  )
{
  ASSERT (Document != NULL);

  return Document->Root;
}

CONST CHAR8 *
XmlNodeName (
  IN  CONST XML_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  return Node->Name;
}

CONST CHAR8 *
XmlNodeContent (
  IN  CONST XML_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  return Node->Real != NULL ? Node->Real->Content : Node->Content;
}

VOID
XmlNodeChangeContent (
  IN OUT  XML_NODE     *Node,
  IN      CONST CHAR8  *Content
  )
{
  ASSERT (Node    != NULL);
  ASSERT (Content != NULL);

  if (Node->Real != NULL) {
    Node->Real->Content = Content;
  }
  Node->Content = Content;
}

UINT32
XmlNodeChildren (
  IN  CONST XML_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  return Node->Children ? Node->Children->NodeCount : 0;
}

XML_NODE *
XmlNodeChild (
  IN  CONST XML_NODE  *Node,
  IN  UINT32          Child
  )
{
  ASSERT (Node != NULL);

  return Node->Children->NodeList[Child];
}

XML_NODE *
EFIAPI
XmlEasyChild (
  IN OUT  XML_NODE     *Node,
  IN      CONST CHAR8  *ChildName,
  ...
  )
{
  VA_LIST   Arguments;
  XML_NODE  *Next;
  XML_NODE  *Child;
  UINT32    Index;

  ASSERT (Node      != NULL);
  ASSERT (ChildName != NULL);

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
    ChildName = VA_ARG (Arguments, CONST CHAR8 *);
  }

  VA_END (Arguments);

  //
  // Return current element.
  //
  return Node;
}

XML_NODE *
XmlNodeAppend (
  IN OUT  XML_NODE     *Node,
  IN      CONST CHAR8  *Name,
  IN      CONST CHAR8  *Attributes  OPTIONAL,
  IN      CONST CHAR8  *Content     OPTIONAL
  )
{
  XML_NODE  *NewNode;

  ASSERT (Node != NULL);
  ASSERT (Name != NULL);

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
  IN OUT  XML_NODE     *Node,
  IN      CONST CHAR8  *Name,
  IN      CONST CHAR8  *Attributes,
  IN      CONST CHAR8  *Content
  )
{
  XML_NODE  *NewNode;

  ASSERT (Node       != NULL);
  ASSERT (Name       != NULL);
  ASSERT (Attributes != NULL);
  ASSERT (Content    != NULL);

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
  IN  CONST XML_DOCUMENT  *Document
  )
{
  XML_NODE  *Node;

  ASSERT (Document != NULL);

  Node = Document->Root;

  if (AsciiStrCmp (XmlNodeName (Node), "plist") != 0) {
    XML_USAGE_ERROR ("PlistDocumentRoot::not plist root");
    return NULL;
  }

  if (XmlNodeChildren (Node) != 1) {
    XML_USAGE_ERROR ("PlistDocumentRoot::no single first node");
    return NULL;
  }

  return XmlNodeChild (Node, 0);
}

XML_NODE *
PlistNodeCast (
  IN  XML_NODE         *Node  OPTIONAL,
  IN  PLIST_NODE_TYPE  Type
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

  return (XML_NODE *) Node;
}

UINT32
PlistDictChildren (
  IN  CONST XML_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  return XmlNodeChildren (Node) / 2;
}

XML_NODE *
PlistDictChild (
  IN   CONST XML_NODE  *Node,
  IN   UINT32          Child,
  OUT  XML_NODE        **Value OPTIONAL
  )
{
  ASSERT (Node != NULL);

  Child *= 2;

  if (Value != NULL) {
    *Value = XmlNodeChild (Node, Child + 1);
  }

  return XmlNodeChild (Node, Child);
}

CONST CHAR8 *
PlistKeyValue (
  IN  XML_NODE  *Node  OPTIONAL
  )
{
 if (PlistNodeCast (Node, PLIST_NODE_TYPE_KEY) == NULL) {
    return NULL;
  }

  return XmlNodeContent (Node);
}

BOOLEAN
PlistStringValue (
  IN      XML_NODE  *Node   OPTIONAL,
     OUT  CHAR8     *Value,
  IN OUT  UINT32    *Size
  )
{
  CONST CHAR8  *Content;
  UINTN        Length;

  ASSERT (Value != NULL);
  ASSERT (Size  != NULL);

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
  IN      XML_NODE  *Node    OPTIONAL,
  OUT     UINT8     *Buffer,
  IN OUT  UINT32    *Size
  )
{
  CONST CHAR8    *Content;
  UINTN          Length;
  EFI_STATUS     Status;

  ASSERT (Buffer != NULL);
  ASSERT (Size   != NULL);

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_DATA) == NULL) {
    return FALSE;
  }

  Content = XmlNodeContent (Node);
  if (Content == NULL) {
    *Size = 0;
    return TRUE;
  }

  Length = *Size;
  Status = Base64Decode (Content, AsciiStrLen (Content), Buffer, &Length);

  if (!EFI_ERROR (Status) && (UINT32) Length == Length) {
    *Size = (UINT32) Length;
    return TRUE;
  }

  *Size = 0;
  return FALSE;
}

BOOLEAN
PlistBooleanValue (
  IN   XML_NODE  *Node   OPTIONAL,
  OUT  BOOLEAN   *Value
  )
{
  ASSERT (Value != NULL);

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
  IN   XML_NODE  *Node   OPTIONAL,
  OUT  VOID      *Value,
  IN   UINT32    Size,
  IN   BOOLEAN   Hex
  )
{
  UINT64       Temp;
  CONST CHAR8  *TempStr;
  BOOLEAN      Negate;

  ASSERT (Value != NULL);

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
PlistMultiDataValue (
  IN      XML_NODE  *Node    OPTIONAL,
     OUT  VOID      *Buffer,
  IN OUT  UINT32    *Size
  )
{
  CONST CHAR8    *Content;
  UINTN          Length;
  EFI_STATUS     Status;

  ASSERT (Buffer != NULL);
  ASSERT (Size   != NULL);

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_DATA) != NULL) {
    Content = XmlNodeContent (Node);
    if (Content != NULL) {

      Length = *Size;
      Status = Base64Decode (Content, AsciiStrLen (Content), Buffer, &Length);

      if (!EFI_ERROR (Status) && (UINT32) Length == Length) {
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
  IN   XML_NODE  *Node  OPTIONAL,
  OUT  UINT32    *Size
  )
{
  CONST CHAR8  *Content;

  ASSERT (Size != NULL);

  if (PlistNodeCast (Node, PLIST_NODE_TYPE_STRING) == NULL) {
    return FALSE;
  }

  Content = XmlNodeContent (Node);
  if (Content != NULL) {
    *Size = (UINT32) AsciiStrSize (Content);
    return TRUE;
  }

  *Size = 0;
  return TRUE;
}

BOOLEAN
PlistDataSize (
  IN   XML_NODE  *Node  OPTIONAL,
  OUT  UINT32    *Size
  )
{
  CONST CHAR8  *Content;

  ASSERT (Size != NULL);

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
PlistMultiDataSize (
  IN   XML_NODE  *Node  OPTIONAL,
  OUT  UINT32    *Size
  )
{
  CONST CHAR8  *Content;

  ASSERT (Size != NULL);

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
