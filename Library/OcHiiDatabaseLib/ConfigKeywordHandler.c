/** @file
Implementation of interfaces function for EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "HiiDatabase.h"

extern HII_DATABASE_PRIVATE_DATA mPrivate;

/**
  Convert the hex UNICODE %02x encoding of a UEFI device path to binary
  from <PathHdr> of <MultiKeywordRequest>.

  This is a internal function.

  @param  String                 MultiKeywordRequest string.
  @param  DevicePathData         Binary of a UEFI device path.
  @param  NextString             string follow the possible PathHdr string.

  @retval EFI_INVALID_PARAMETER  The device path is not valid or the incoming parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Lake of resources to store necessary structures.
  @retval EFI_SUCCESS            The device path is retrieved and translated to binary format.
                                 The Input string not include PathHdr section.

**/
EFI_STATUS
ExtractDevicePath (
  IN  EFI_STRING                   String,
  OUT UINT8                        **DevicePathData,
  OUT EFI_STRING                   *NextString
  )
{
  UINTN                    Length;
  EFI_STRING               PathHdr;
  UINT8                    *DevicePathBuffer;
  CHAR16                   TemStr[2];
  UINTN                    Index;
  UINT8                    DigitUint8;
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;

  ASSERT (NextString != NULL && DevicePathData != NULL);

  //
  // KeywordRequest == NULL case.
  //
  if (String == NULL) {
    *DevicePathData = NULL;
    *NextString = NULL;
    return EFI_SUCCESS;
  }

  //
  // Skip '&' if exist.
  //
  if (*String == L'&') {
    String ++;
  }

  //
  // Find the 'PATH=' of <PathHdr>.
  //
  if (StrnCmp (String, L"PATH=", StrLen (L"PATH=")) != 0) {
    if (StrnCmp (String, L"KEYWORD=", StrLen (L"KEYWORD=")) != 0) {
      return EFI_INVALID_PARAMETER;
    } else {
      //
      // Not include PathHdr, return success and DevicePath = NULL.
      //
      *DevicePathData = NULL;
      *NextString = String;
      return EFI_SUCCESS;
    }
  }

  //
  // Check whether path data does exist.
  //
  String += StrLen (L"PATH=");
  if (*String == 0) {
    return EFI_INVALID_PARAMETER;
  }
  PathHdr = String;

  //
  // The content between 'PATH=' of <ConfigHdr> and '&' of next element
  // or '\0' (end of configuration string) is the UNICODE %02x bytes encoding
  // of UEFI device path.
  //
  for (Length = 0; *String != 0 && *String != L'&'; String++, Length++);

  //
  // Save the return next keyword string value.
  //
  *NextString = String;

  //
  // Check DevicePath Length
  //
  if (((Length + 1) / 2) < sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The data in <PathHdr> is encoded as hex UNICODE %02x bytes in the same order
  // as the device path resides in RAM memory.
  // Translate the data into binary.
  //
  DevicePathBuffer = (UINT8 *) AllocateZeroPool ((Length + 1) / 2);
  if (DevicePathBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Convert DevicePath
  //
  ZeroMem (TemStr, sizeof (TemStr));
  for (Index = 0; Index < Length; Index ++) {
    TemStr[0] = PathHdr[Index];
    DigitUint8 = (UINT8) StrHexToUint64 (TemStr);
    if ((Index & 1) == 0) {
      DevicePathBuffer [Index/2] = DigitUint8;
    } else {
      DevicePathBuffer [Index/2] = (UINT8) ((DevicePathBuffer [Index/2] << 4) + DigitUint8);
    }
  }

  //
  // Validate DevicePath
  //
  DevicePath  = (EFI_DEVICE_PATH_PROTOCOL *) DevicePathBuffer;
  while (!IsDevicePathEnd (DevicePath)) {
    if ((DevicePath->Type == 0) || (DevicePath->SubType == 0) || (DevicePathNodeLength (DevicePath) < sizeof (EFI_DEVICE_PATH_PROTOCOL))) {
      //
      // Invalid device path
      //
      FreePool (DevicePathBuffer);
      return EFI_INVALID_PARAMETER;
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }

  //
  // return the device path
  //
  *DevicePathData = DevicePathBuffer;

  return EFI_SUCCESS;
}

/**
  Get NameSpace from the input NameSpaceId string.

  This is a internal function.

  @param  String                 <NameSpaceId> format string.
  @param  NameSpace              Return the name space string.
  @param  NextString             Return the next string follow namespace.

  @retval   EFI_SUCCESS             Get the namespace string success.
  @retval   EFI_INVALID_PARAMETER   The NameSpaceId string not follow spec definition.

**/
EFI_STATUS
ExtractNameSpace (
  IN  EFI_STRING                   String,
  OUT CHAR8                        **NameSpace,
  OUT EFI_STRING                   *NextString
  )
{
  CHAR16    *TmpPtr;
  UINTN     NameSpaceSize;

  ASSERT (NameSpace != NULL);

  TmpPtr = NULL;

  //
  // Input NameSpaceId == NULL
  //
  if (String == NULL) {
    *NameSpace = NULL;
    if (NextString != NULL) {
      *NextString = NULL;
    }
    return EFI_SUCCESS;
  }

  //
  // Skip '&' if exist.
  //
  if (*String == L'&') {
    String++;
  }

  if (StrnCmp (String, L"NAMESPACE=", StrLen (L"NAMESPACE=")) != 0) {
    return EFI_INVALID_PARAMETER;
  }
  String += StrLen (L"NAMESPACE=");

  TmpPtr = StrStr (String, L"&");
  if (TmpPtr != NULL) {
    *TmpPtr = 0;
  }
  if (NextString != NULL) {
    *NextString = String + StrLen (String);
  }

  //
  // Input NameSpace is unicode string. The language in String package is ascii string.
  // Here will convert the unicode string to ascii and save it.
  //
  NameSpaceSize = StrLen (String) + 1;
  *NameSpace = AllocatePool (NameSpaceSize);
  if (*NameSpace == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  UnicodeStrToAsciiStrS (String, *NameSpace, NameSpaceSize);

  if (TmpPtr != NULL) {
    *TmpPtr = L'&';
  }

  return EFI_SUCCESS;
}

/**
  Get Keyword from the input KeywordRequest string.

  This is a internal function.

  @param  String                 KeywordRequestformat string.
  @param  Keyword                return the extract keyword string.
  @param  NextString             return the next string follow this keyword section.

  @retval EFI_SUCCESS            Success to get the keyword string.
  @retval EFI_INVALID_PARAMETER  Parse the input string return error.

**/
EFI_STATUS
ExtractKeyword (
  IN  EFI_STRING                   String,
  OUT EFI_STRING                   *Keyword,
  OUT EFI_STRING                   *NextString
  )
{
  EFI_STRING  TmpPtr;

  ASSERT ((Keyword != NULL) && (NextString != NULL));

  TmpPtr = NULL;

  //
  // KeywordRequest == NULL case.
  //
  if (String == NULL) {
    *Keyword = NULL;
    *NextString = NULL;
    return EFI_SUCCESS;
  }

  //
  // Skip '&' if exist.
  //
  if (*String == L'&') {
    String++;
  }

  if (StrnCmp (String, L"KEYWORD=", StrLen (L"KEYWORD=")) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  String += StrLen (L"KEYWORD=");

  TmpPtr = StrStr (String, L"&");
  if (TmpPtr != NULL) {
    *TmpPtr = 0;
  }
  *NextString = String + StrLen (String);

  *Keyword = AllocateCopyPool (StrSize (String), String);
  if (*Keyword == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (TmpPtr != NULL) {
    *TmpPtr = L'&';
  }

  return EFI_SUCCESS;
}

/**
  Get value from the input KeywordRequest string.

  This is a internal function.

  @param  String                 KeywordRequestformat string.
  @param  Value                  return the extract value string.
  @param  NextString             return the next string follow this keyword section.

  @retval EFI_SUCCESS            Success to get the keyword string.
  @retval EFI_INVALID_PARAMETER  Parse the input string return error.

**/
EFI_STATUS
ExtractValue (
  IN  EFI_STRING                   String,
  OUT EFI_STRING                   *Value,
  OUT EFI_STRING                   *NextString
  )
{
  EFI_STRING  TmpPtr;

  ASSERT ((Value != NULL) && (NextString != NULL) && (String != NULL));

  //
  // Skip '&' if exist.
  //
  if (*String == L'&') {
    String++;
  }

  if (StrnCmp (String, L"VALUE=", StrLen (L"VALUE=")) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  String += StrLen (L"VALUE=");

  TmpPtr = StrStr (String, L"&");
  if (TmpPtr != NULL) {
    *TmpPtr = 0;
  }
  *NextString = String + StrLen (String);

  *Value = AllocateCopyPool (StrSize (String), String);
  if (*Value == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (TmpPtr != NULL) {
    *TmpPtr = L'&';
  }

  return EFI_SUCCESS;
}

/**
  Get filter from the input KeywordRequest string.

  This is a internal function.

  @param  String                 KeywordRequestformat string.
  @param  FilterFlags            return the filter condition.
  @param  NextString             return the next string follow this keyword section.

  @retval EFI_SUCCESS            Success to get the keyword string.
  @retval EFI_INVALID_PARAMETER  Parse the input string return error.

**/
BOOLEAN
ExtractFilter (
  IN  EFI_STRING                   String,
  OUT UINT8                        *FilterFlags,
  OUT EFI_STRING                   *NextString
  )
{
  CHAR16      *PathPtr;
  CHAR16      *KeywordPtr;
  BOOLEAN     RetVal;

  ASSERT ((FilterFlags != NULL) && (NextString != NULL));

  //
  // String end, no filter section.
  //
  if (String == NULL) {
    *NextString = NULL;
    return FALSE;
  }

  *FilterFlags = 0;
  RetVal = TRUE;

  //
  // Skip '&' if exist.
  //
  if (*String == L'&') {
    String++;
  }

  if (StrnCmp (String, L"ReadOnly", StrLen (L"ReadOnly")) == 0) {
    //
    // Find ReadOnly filter.
    //
    *FilterFlags |= EFI_KEYWORD_FILTER_READONY;
    String += StrLen (L"ReadOnly");
  } else if (StrnCmp (String, L"ReadWrite", StrLen (L"ReadWrite")) == 0) {
    //
    // Find ReadWrite filter.
    //
    *FilterFlags |= EFI_KEYWORD_FILTER_REAWRITE;
    String += StrLen (L"ReadWrite");
  } else if (StrnCmp (String, L"Buffer", StrLen (L"Buffer")) == 0) {
    //
    // Find Buffer Filter.
    //
    *FilterFlags |= EFI_KEYWORD_FILTER_BUFFER;
    String += StrLen (L"Buffer");
  } else if (StrnCmp (String, L"Numeric", StrLen (L"Numeric")) == 0) {
    //
    // Find Numeric Filter
    //
    String += StrLen (L"Numeric");
    if (*String != L':') {
      *FilterFlags |= EFI_KEYWORD_FILTER_NUMERIC;
    } else {
      String++;
      switch (*String) {
      case L'1':
        *FilterFlags |= EFI_KEYWORD_FILTER_NUMERIC_1;
        break;
      case L'2':
        *FilterFlags |= EFI_KEYWORD_FILTER_NUMERIC_2;
        break;
      case L'4':
        *FilterFlags |= EFI_KEYWORD_FILTER_NUMERIC_4;
        break;
      case L'8':
        *FilterFlags |= EFI_KEYWORD_FILTER_NUMERIC_8;
        break;
      default:
        ASSERT (FALSE);
        break;
      }
      String++;
    }
  } else {
    //
    // Check whether other filter item defined by Platform.
    //
    if ((StrnCmp (String, L"&PATH", StrLen (L"&PATH")) == 0) ||
        (StrnCmp (String, L"&KEYWORD", StrLen (L"&KEYWORD")) == 0)) {
      //
      // New KeywordRequest start, no platform defined filter.
      //
    } else {
      //
      // Platform defined filter rule.
      // Just skip platform defined filter rule, return success.
      //
      PathPtr = StrStr(String, L"&PATH");
      KeywordPtr = StrStr(String, L"&KEYWORD");
      if (PathPtr != NULL && KeywordPtr != NULL) {
        //
        // If both sections exist, return the first follow string.
        //
        String = KeywordPtr > PathPtr ? PathPtr : KeywordPtr;
      } else if (PathPtr != NULL) {
        //
        // Should not exist PathPtr != NULL && KeywordPtr == NULL case.
        //
        ASSERT (FALSE);
      } else if (KeywordPtr != NULL) {
        //
        // Just to the next keyword section.
        //
        String = KeywordPtr;
      } else {
        //
        // Only has platform defined filter section, just skip it.
        //
        String += StrLen (String);
      }
    }
    RetVal = FALSE;
  }

  *NextString = String;

  return RetVal;
}

/**
  Extract Readonly flag from opcode.

  This is a internal function.

  @param  OpCodeData             Input opcode for this question.

  @retval TRUE                   This question is readonly.
  @retval FALSE                  This question is not readonly.

**/
BOOLEAN
ExtractReadOnlyFromOpCode (
  IN  UINT8         *OpCodeData
  )
{
  EFI_IFR_QUESTION_HEADER   *QuestionHdr;

  ASSERT (OpCodeData != NULL);

  QuestionHdr = (EFI_IFR_QUESTION_HEADER *) (OpCodeData + sizeof (EFI_IFR_OP_HEADER));

  return (QuestionHdr->Flags & EFI_IFR_FLAG_READ_ONLY) != 0;
}

/**
  Create a circuit to check the filter section.

  This is a internal function.

  @param  OpCodeData             The question binary ifr data.
  @param  KeywordRequest         KeywordRequestformat string.
  @param  NextString             return the next string follow this keyword section.
  @param  ReadOnly               Return whether this question is read only.

  @retval KEYWORD_HANDLER_NO_ERROR                     Success validate.
  @retval KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED  Validate fail.

**/
UINT32
ValidateFilter (
  IN  UINT8         *OpCodeData,
  IN  CHAR16        *KeywordRequest,
  OUT CHAR16        **NextString,
  OUT BOOLEAN       *ReadOnly
  )
{
  CHAR16                    *NextFilter;
  CHAR16                    *StringPtr;
  UINT8                     FilterFlags;
  EFI_IFR_QUESTION_HEADER   *QuestionHdr;
  EFI_IFR_OP_HEADER         *OpCodeHdr;
  UINT8                     Flags;
  UINT32                    RetVal;

  RetVal = KEYWORD_HANDLER_NO_ERROR;
  StringPtr = KeywordRequest;

  OpCodeHdr = (EFI_IFR_OP_HEADER *) OpCodeData;
  QuestionHdr = (EFI_IFR_QUESTION_HEADER *) (OpCodeData + sizeof (EFI_IFR_OP_HEADER));
  if (OpCodeHdr->OpCode == EFI_IFR_ONE_OF_OP || OpCodeHdr->OpCode == EFI_IFR_NUMERIC_OP) {
    Flags = *(OpCodeData + sizeof (EFI_IFR_OP_HEADER) + sizeof (EFI_IFR_QUESTION_HEADER));
  } else {
    Flags = 0;
  }

  //
  // Get ReadOnly flag from Question.
  //
  *ReadOnly = ExtractReadOnlyFromOpCode(OpCodeData);

  while (ExtractFilter (StringPtr, &FilterFlags, &NextFilter)) {
    switch (FilterFlags) {
    case EFI_KEYWORD_FILTER_READONY:
      if ((QuestionHdr->Flags & EFI_IFR_FLAG_READ_ONLY) == 0) {
        RetVal = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
        goto Done;
      }
      break;

    case EFI_KEYWORD_FILTER_REAWRITE:
      if ((QuestionHdr->Flags & EFI_IFR_FLAG_READ_ONLY) != 0) {
        RetVal = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
        goto Done;
      }
      break;

    case EFI_KEYWORD_FILTER_BUFFER:
      //
      // Only these three opcode use numeric value type.
      //
      if (OpCodeHdr->OpCode == EFI_IFR_ONE_OF_OP || OpCodeHdr->OpCode == EFI_IFR_NUMERIC_OP || OpCodeHdr->OpCode == EFI_IFR_CHECKBOX_OP) {
        RetVal = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
        goto Done;
      }
      break;

    case EFI_KEYWORD_FILTER_NUMERIC:
      if (OpCodeHdr->OpCode != EFI_IFR_ONE_OF_OP && OpCodeHdr->OpCode != EFI_IFR_NUMERIC_OP && OpCodeHdr->OpCode != EFI_IFR_CHECKBOX_OP) {
        RetVal = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
        goto Done;
      }
      break;

    case EFI_KEYWORD_FILTER_NUMERIC_1:
    case EFI_KEYWORD_FILTER_NUMERIC_2:
    case EFI_KEYWORD_FILTER_NUMERIC_4:
    case EFI_KEYWORD_FILTER_NUMERIC_8:
      if (OpCodeHdr->OpCode != EFI_IFR_ONE_OF_OP && OpCodeHdr->OpCode != EFI_IFR_NUMERIC_OP && OpCodeHdr->OpCode != EFI_IFR_CHECKBOX_OP) {
        RetVal = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
        goto Done;
      }

      //
      // For numeric and oneof, it has flags field to specify the detail numeric type.
      //
      if (OpCodeHdr->OpCode == EFI_IFR_ONE_OF_OP || OpCodeHdr->OpCode == EFI_IFR_NUMERIC_OP) {
        switch (Flags & EFI_IFR_NUMERIC_SIZE) {
        case EFI_IFR_NUMERIC_SIZE_1:
          if (FilterFlags != EFI_KEYWORD_FILTER_NUMERIC_1) {
            RetVal = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
            goto Done;
          }
          break;

        case EFI_IFR_NUMERIC_SIZE_2:
          if (FilterFlags != EFI_KEYWORD_FILTER_NUMERIC_2) {
            RetVal = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
            goto Done;
          }
          break;

        case EFI_IFR_NUMERIC_SIZE_4:
          if (FilterFlags != EFI_KEYWORD_FILTER_NUMERIC_4) {
            RetVal = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
            goto Done;
          }
          break;

        case EFI_IFR_NUMERIC_SIZE_8:
          if (FilterFlags != EFI_KEYWORD_FILTER_NUMERIC_8) {
            RetVal = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
            goto Done;
          }
          break;

        default:
          ASSERT (FALSE);
          break;
        }
      }
      break;

    default:
      ASSERT (FALSE);
      break;
    }

    //
    // Jump to the next filter.
    //
    StringPtr = NextFilter;
  }

Done:
  //
  // The current filter which is processing.
  //
  *NextString = StringPtr;

  return RetVal;
}

/**
  Get HII_DATABASE_RECORD from the input device path info.

  This is a internal function.

  @param  DevicePath             UEFI device path protocol.

  @retval Internal data base record.

**/
HII_DATABASE_RECORD *
GetRecordFromDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  LIST_ENTRY             *Link;
  UINT8                  *DevicePathPkg;
  UINT8                  *CurrentDevicePath;
  UINTN                  DevicePathSize;
  HII_DATABASE_RECORD    *TempDatabase;

  ASSERT (DevicePath != NULL);

  for (Link = mPrivate.DatabaseList.ForwardLink; Link != &mPrivate.DatabaseList; Link = Link->ForwardLink) {
    TempDatabase = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
    DevicePathPkg = TempDatabase->PackageList->DevicePathPkg;
    if (DevicePathPkg != NULL) {
      CurrentDevicePath = DevicePathPkg + sizeof (EFI_HII_PACKAGE_HEADER);
      DevicePathSize = GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) CurrentDevicePath);
      if ((CompareMem (DevicePath, CurrentDevicePath, DevicePathSize) == 0)) {
        return TempDatabase;
      }
    }
  }

  return NULL;
}

/**
  Calculate the size of StringSrc and output it. Also copy string text from src
  to dest.

  This is a internal function.

  @param  StringSrc              Points to current null-terminated string.
  @param  BufferSize             Length of the buffer.
  @param  StringDest             Buffer to store the string text.

  @retval EFI_SUCCESS            The string text was outputted successfully.
  @retval EFI_OUT_OF_RESOURCES   Out of resource.

**/
EFI_STATUS
GetUnicodeStringTextAndSize (
  IN  UINT8            *StringSrc,
  OUT UINTN            *BufferSize,
  OUT EFI_STRING       *StringDest
  )
{
  UINTN  StringSize;
  UINT8  *StringPtr;

  ASSERT (StringSrc != NULL && BufferSize != NULL && StringDest != NULL);

  StringSize = sizeof (CHAR16);
  StringPtr  = StringSrc;
  while (ReadUnaligned16 ((UINT16 *) StringPtr) != 0) {
    StringSize += sizeof (CHAR16);
    StringPtr += sizeof (CHAR16);
  }

  *StringDest = AllocatePool (StringSize);
  if (*StringDest == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (*StringDest, StringSrc, StringSize);

  *BufferSize = StringSize;
  return EFI_SUCCESS;
}

/**
  Find the string id for the input keyword.

  @param  StringPackage           Hii string package instance.
  @param  KeywordValue            Input keyword value.
  @param  StringId                The string's id, which is unique within PackageList.


  @retval EFI_SUCCESS             The string text and font is retrieved
                                  successfully.
  @retval EFI_NOT_FOUND           The specified text or font info can not be found
                                  out.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.
**/
EFI_STATUS
GetStringIdFromString (
  IN HII_STRING_PACKAGE_INSTANCE      *StringPackage,
  IN CHAR16                           *KeywordValue,
  OUT EFI_STRING_ID                   *StringId
  )
{
  UINT8                                *BlockHdr;
  EFI_STRING_ID                        CurrentStringId;
  UINTN                                BlockSize;
  UINTN                                Index;
  UINT8                                *StringTextPtr;
  UINTN                                Offset;
  UINT16                               StringCount;
  UINT16                               SkipCount;
  UINT8                                Length8;
  EFI_HII_SIBT_EXT2_BLOCK              Ext2;
  UINT32                               Length32;
  UINTN                                StringSize;
  CHAR16                               *String;
  CHAR8                                *AsciiKeywordValue;
  UINTN                                KeywordValueSize;
  EFI_STATUS                           Status;

  ASSERT (StringPackage != NULL && KeywordValue != NULL && StringId != NULL);
  ASSERT (StringPackage->Signature == HII_STRING_PACKAGE_SIGNATURE);

  CurrentStringId = 1;
  Status = EFI_SUCCESS;
  String = NULL;
  BlockHdr = StringPackage->StringBlock;
  BlockSize = 0;
  Offset = 0;

  //
  // Make a ascii keyword value for later use.
  //
  KeywordValueSize = StrLen (KeywordValue) + 1;
  AsciiKeywordValue = AllocatePool (KeywordValueSize);
  if (AsciiKeywordValue == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  UnicodeStrToAsciiStrS (KeywordValue, AsciiKeywordValue, KeywordValueSize);

  while (*BlockHdr != EFI_HII_SIBT_END) {
    switch (*BlockHdr) {
    case EFI_HII_SIBT_STRING_SCSU:
      Offset = sizeof (EFI_HII_STRING_BLOCK);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset + AsciiStrSize ((CHAR8 *) StringTextPtr);
      if (AsciiStrCmp(AsciiKeywordValue, (CHAR8 *) StringTextPtr) == 0) {
        *StringId = CurrentStringId;
        goto Done;
      }
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRING_SCSU_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK) - sizeof (UINT8);
      StringTextPtr = BlockHdr + Offset;
      if (AsciiStrCmp(AsciiKeywordValue, (CHAR8 *) StringTextPtr) == 0) {
        *StringId = CurrentStringId;
        goto Done;
      }
      BlockSize += Offset + AsciiStrSize ((CHAR8 *) StringTextPtr);
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRINGS_SCSU:
      CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      StringTextPtr = (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_BLOCK) - sizeof (UINT8));
      BlockSize += StringTextPtr - BlockHdr;

      for (Index = 0; Index < StringCount; Index++) {
        BlockSize += AsciiStrSize ((CHAR8 *) StringTextPtr);
        if (AsciiStrCmp(AsciiKeywordValue, (CHAR8 *) StringTextPtr) == 0) {
          *StringId = CurrentStringId;
          goto Done;
        }
        StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *) StringTextPtr);
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRINGS_SCSU_FONT:
      CopyMem (
        &StringCount,
        (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
        sizeof (UINT16)
        );
      StringTextPtr = (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK) - sizeof (UINT8));
      BlockSize += StringTextPtr - BlockHdr;

      for (Index = 0; Index < StringCount; Index++) {
        BlockSize += AsciiStrSize ((CHAR8 *) StringTextPtr);
        if (AsciiStrCmp(AsciiKeywordValue, (CHAR8 *) StringTextPtr) == 0) {
          *StringId = CurrentStringId;
          goto Done;
        }
        StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *) StringTextPtr);
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRING_UCS2:
      Offset        = sizeof (EFI_HII_STRING_BLOCK);
      StringTextPtr = BlockHdr + Offset;
      //
      // Use StringSize to store the size of the specified string, including the NULL
      // terminator.
      //
      Status = GetUnicodeStringTextAndSize (StringTextPtr, &StringSize, &String);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      ASSERT (String != NULL);
      if (StrCmp(KeywordValue, String) == 0) {
        *StringId = CurrentStringId;
        goto Done;
      }
      BlockSize += Offset + StringSize;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRING_UCS2_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK)  - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      //
      // Use StringSize to store the size of the specified string, including the NULL
      // terminator.
      //
      Status = GetUnicodeStringTextAndSize (StringTextPtr, &StringSize, &String);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      ASSERT (String != NULL);
      if (StrCmp(KeywordValue, String) == 0) {
        *StringId = CurrentStringId;
        goto Done;
      }
      BlockSize += Offset + StringSize;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRINGS_UCS2:
      Offset = sizeof (EFI_HII_SIBT_STRINGS_UCS2_BLOCK) - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset;
      CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      for (Index = 0; Index < StringCount; Index++) {
        Status = GetUnicodeStringTextAndSize (StringTextPtr, &StringSize, &String);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        ASSERT (String != NULL);
        BlockSize += StringSize;
        if (StrCmp(KeywordValue, String) == 0) {
          *StringId = CurrentStringId;
          goto Done;
        }
        StringTextPtr = StringTextPtr + StringSize;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRINGS_UCS2_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK) - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset;
      CopyMem (
        &StringCount,
        (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
        sizeof (UINT16)
        );
      for (Index = 0; Index < StringCount; Index++) {
        Status = GetUnicodeStringTextAndSize (StringTextPtr, &StringSize, &String);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        ASSERT (String != NULL);
        BlockSize += StringSize;
        if (StrCmp(KeywordValue, String) == 0) {
          *StringId = CurrentStringId;
          goto Done;
        }
        StringTextPtr = StringTextPtr + StringSize;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_DUPLICATE:
      BlockSize       += sizeof (EFI_HII_SIBT_DUPLICATE_BLOCK);
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_SKIP1:
      SkipCount = (UINT16) (*(UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK)));
      CurrentStringId = (UINT16) (CurrentStringId + SkipCount);
      BlockSize       +=  sizeof (EFI_HII_SIBT_SKIP1_BLOCK);
      break;

    case EFI_HII_SIBT_SKIP2:
      CopyMem (&SkipCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      CurrentStringId = (UINT16) (CurrentStringId + SkipCount);
      BlockSize       +=  sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
      break;

    case EFI_HII_SIBT_EXT1:
      CopyMem (
        &Length8,
        (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
        sizeof (UINT8)
        );
      BlockSize += Length8;
      break;

    case EFI_HII_SIBT_EXT2:
      CopyMem (&Ext2, BlockHdr, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
      BlockSize += Ext2.Length;
      break;

    case EFI_HII_SIBT_EXT4:
      CopyMem (
        &Length32,
        (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
        sizeof (UINT32)
        );

      BlockSize += Length32;
      break;

    default:
      break;
    }

    if (String != NULL) {
      FreePool (String);
      String = NULL;
    }

    BlockHdr  = StringPackage->StringBlock + BlockSize;
  }

  Status = EFI_NOT_FOUND;

Done:
  if (AsciiKeywordValue != NULL) {
    FreePool (AsciiKeywordValue);
  }
  if (String != NULL) {
    FreePool (String);
  }
  return Status;
}

/**
  Find the next valid string id for the input string id.

  @param  StringPackage           Hii string package instance.
  @param  StringId                The current string id which is already got.
                                  1 means just begin to get the string id.
  @param  KeywordValue            Return the string for the next string id.


  @retval EFI_STRING_ID           Not 0 means a valid stringid found.
                                  0 means not found a valid string id.
**/
EFI_STRING_ID
GetNextStringId (
  IN  HII_STRING_PACKAGE_INSTANCE      *StringPackage,
  IN  EFI_STRING_ID                    StringId,
  OUT EFI_STRING                       *KeywordValue
  )
{
  UINT8                                *BlockHdr;
  EFI_STRING_ID                        CurrentStringId;
  UINTN                                BlockSize;
  UINTN                                Index;
  UINT8                                *StringTextPtr;
  UINTN                                Offset;
  UINT16                               StringCount;
  UINT16                               SkipCount;
  UINT8                                Length8;
  EFI_HII_SIBT_EXT2_BLOCK              Ext2;
  UINT32                               Length32;
  BOOLEAN                              FindString;
  UINTN                                StringSize;
  CHAR16                               *String;

  ASSERT (StringPackage != NULL);
  ASSERT (StringPackage->Signature == HII_STRING_PACKAGE_SIGNATURE);

  CurrentStringId = 1;
  FindString = FALSE;
  String = NULL;

  //
  // Parse the string blocks to get the string text and font.
  //
  BlockHdr  = StringPackage->StringBlock;
  BlockSize = 0;
  Offset    = 0;
  while (*BlockHdr != EFI_HII_SIBT_END) {
    switch (*BlockHdr) {
    case EFI_HII_SIBT_STRING_SCSU:
      Offset = sizeof (EFI_HII_STRING_BLOCK);
      StringTextPtr = BlockHdr + Offset;

      if (FindString) {
        StringSize = AsciiStrSize ((CHAR8 *) StringTextPtr);
        *KeywordValue = AllocatePool (StringSize * sizeof (CHAR16));
        if (*KeywordValue == NULL) {
          return 0;
        }
        AsciiStrToUnicodeStrS ((CHAR8 *) StringTextPtr, *KeywordValue, StringSize);
        return CurrentStringId;
      } else if (CurrentStringId == StringId) {
        FindString = TRUE;
      }

      BlockSize += Offset + AsciiStrSize ((CHAR8 *) StringTextPtr);
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRING_SCSU_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK) - sizeof (UINT8);
      StringTextPtr = BlockHdr + Offset;

      if (FindString) {
        StringSize = AsciiStrSize ((CHAR8 *) StringTextPtr);
        *KeywordValue = AllocatePool (StringSize * sizeof (CHAR16));
        if (*KeywordValue == NULL) {
          return 0;
        }
        AsciiStrToUnicodeStrS ((CHAR8 *) StringTextPtr, *KeywordValue, StringSize);
        return CurrentStringId;
      } else if (CurrentStringId == StringId) {
        FindString = TRUE;
      }

      BlockSize += Offset + AsciiStrSize ((CHAR8 *) StringTextPtr);
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRINGS_SCSU:
      CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      StringTextPtr = (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_BLOCK) - sizeof (UINT8));
      BlockSize += StringTextPtr - BlockHdr;

      for (Index = 0; Index < StringCount; Index++) {
        if (FindString) {
          StringSize = AsciiStrSize ((CHAR8 *) StringTextPtr);
          *KeywordValue = AllocatePool (StringSize * sizeof (CHAR16));
          if (*KeywordValue == NULL) {
            return 0;
          }
          AsciiStrToUnicodeStrS ((CHAR8 *) StringTextPtr, *KeywordValue, StringSize);
          return CurrentStringId;
        } else if (CurrentStringId == StringId) {
          FindString = TRUE;
        }

        BlockSize += AsciiStrSize ((CHAR8 *) StringTextPtr);
        StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *) StringTextPtr);
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRINGS_SCSU_FONT:
      CopyMem (
        &StringCount,
        (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
        sizeof (UINT16)
        );
      StringTextPtr = (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK) - sizeof (UINT8));
      BlockSize += StringTextPtr - BlockHdr;

      for (Index = 0; Index < StringCount; Index++) {
        if (FindString) {
          StringSize = AsciiStrSize ((CHAR8 *) StringTextPtr);
          *KeywordValue = AllocatePool (StringSize * sizeof (CHAR16));
          if (*KeywordValue == NULL) {
            return 0;
          }
          AsciiStrToUnicodeStrS ((CHAR8 *) StringTextPtr, *KeywordValue, StringSize);
          return CurrentStringId;
        } else if (CurrentStringId == StringId) {
          FindString = TRUE;
        }

        BlockSize += AsciiStrSize ((CHAR8 *) StringTextPtr);
        StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *) StringTextPtr);
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRING_UCS2:
      Offset        = sizeof (EFI_HII_STRING_BLOCK);
      StringTextPtr = BlockHdr + Offset;
      //
      // Use StringSize to store the size of the specified string, including the NULL
      // terminator.
      //
      GetUnicodeStringTextAndSize (StringTextPtr, &StringSize, &String);
      if (FindString && (String != NULL) && (*String != L'\0')) {
        //
        // String protocol use this type for the string id which has value for other package.
        // It will allocate an empty string block for this string id. so here we also check
        // *String != L'\0' to prohibit this case.
        //
        *KeywordValue = String;
        return CurrentStringId;
      } else if (CurrentStringId == StringId) {
        FindString = TRUE;
      }

      BlockSize += Offset + StringSize;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRING_UCS2_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK)  - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      //
      // Use StringSize to store the size of the specified string, including the NULL
      // terminator.
      //
      GetUnicodeStringTextAndSize (StringTextPtr, &StringSize, &String);
      if (FindString) {
        *KeywordValue = String;
        return CurrentStringId;
      } else if (CurrentStringId == StringId) {
        FindString = TRUE;
      }

      BlockSize += Offset + StringSize;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRINGS_UCS2:
      Offset = sizeof (EFI_HII_SIBT_STRINGS_UCS2_BLOCK) - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset;
      CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      for (Index = 0; Index < StringCount; Index++) {
        GetUnicodeStringTextAndSize (StringTextPtr, &StringSize, &String);

        if (FindString) {
          *KeywordValue = String;
          return CurrentStringId;
        } else if (CurrentStringId == StringId) {
          FindString = TRUE;
        }

        BlockSize += StringSize;
        StringTextPtr = StringTextPtr + StringSize;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRINGS_UCS2_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK) - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset;
      CopyMem (
        &StringCount,
        (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
        sizeof (UINT16)
        );
      for (Index = 0; Index < StringCount; Index++) {
        GetUnicodeStringTextAndSize (StringTextPtr, &StringSize, &String);
        if (FindString) {
          *KeywordValue = String;
          return CurrentStringId;
        } else if (CurrentStringId == StringId) {
          FindString = TRUE;
        }

        BlockSize += StringSize;
        StringTextPtr = StringTextPtr + StringSize;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_DUPLICATE:
      BlockSize       += sizeof (EFI_HII_SIBT_DUPLICATE_BLOCK);
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_SKIP1:
      SkipCount = (UINT16) (*(UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK)));
      CurrentStringId = (UINT16) (CurrentStringId + SkipCount);
      BlockSize       +=  sizeof (EFI_HII_SIBT_SKIP1_BLOCK);
      break;

    case EFI_HII_SIBT_SKIP2:
      CopyMem (&SkipCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      CurrentStringId = (UINT16) (CurrentStringId + SkipCount);
      BlockSize       +=  sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
      break;

    case EFI_HII_SIBT_EXT1:
      CopyMem (
        &Length8,
        (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
        sizeof (UINT8)
        );
      BlockSize += Length8;
      break;

    case EFI_HII_SIBT_EXT2:
      CopyMem (&Ext2, BlockHdr, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
      BlockSize += Ext2.Length;
      break;

    case EFI_HII_SIBT_EXT4:
      CopyMem (
        &Length32,
        (UINT8*)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
        sizeof (UINT32)
        );

      BlockSize += Length32;
      break;

    default:
      break;
    }

    if (String != NULL) {
      FreePool (String);
      String = NULL;
    }

    BlockHdr  = StringPackage->StringBlock + BlockSize;
  }

  return 0;
}

/**
  Get string package from the input NameSpace string.

  This is a internal function.

  @param  DatabaseRecord                 HII_DATABASE_RECORD format string.
  @param  NameSpace                      NameSpace format string.
  @param  KeywordValue                   Keyword value.
  @param  StringId                       String Id for this keyword.

  @retval KEYWORD_HANDLER_NO_ERROR                     Get String id successfully.
  @retval KEYWORD_HANDLER_KEYWORD_NOT_FOUND            Not found the string id in the string package.
  @retval KEYWORD_HANDLER_NAMESPACE_ID_NOT_FOUND       Not found the string package for this namespace.
  @retval KEYWORD_HANDLER_UNDEFINED_PROCESSING_ERROR   Out of resource error.

**/
UINT32
GetStringIdFromRecord (
  IN HII_DATABASE_RECORD   *DatabaseRecord,
  IN CHAR8                 **NameSpace,
  IN CHAR16                *KeywordValue,
  OUT EFI_STRING_ID        *StringId
  )
{
  LIST_ENTRY                          *Link;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_STRING_PACKAGE_INSTANCE         *StringPackage;
  EFI_STATUS                          Status;
  CHAR8                               *Name;
  UINT32                              RetVal;

  ASSERT (DatabaseRecord != NULL && NameSpace != NULL && KeywordValue != NULL);

  PackageListNode = DatabaseRecord->PackageList;
  RetVal = KEYWORD_HANDLER_NAMESPACE_ID_NOT_FOUND;

  if (*NameSpace != NULL) {
    Name = *NameSpace;
  } else {
    Name = UEFI_CONFIG_LANG;
  }

  for (Link = PackageListNode->StringPkgHdr.ForwardLink; Link != &PackageListNode->StringPkgHdr; Link = Link->ForwardLink) {
    StringPackage = CR (Link, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);

    if (AsciiStrnCmp(Name, StringPackage->StringPkgHdr->Language, AsciiStrLen (Name)) == 0) {
      Status = GetStringIdFromString (StringPackage, KeywordValue, StringId);
      if (EFI_ERROR (Status)) {
        return KEYWORD_HANDLER_KEYWORD_NOT_FOUND;
      } else {
        if (*NameSpace == NULL) {
          *NameSpace = AllocateCopyPool (AsciiStrSize (StringPackage->StringPkgHdr->Language), StringPackage->StringPkgHdr->Language);
          if (*NameSpace == NULL) {
            return KEYWORD_HANDLER_UNDEFINED_PROCESSING_ERROR;
          }
        }
        return KEYWORD_HANDLER_NO_ERROR;
      }
    }
  }

  return RetVal;
}

/**
  Tell whether this Operand is an Statement OpCode.

  @param  Operand                Operand of an IFR OpCode.

  @retval TRUE                   This is an Statement OpCode.
  @retval FALSE                  Not an Statement OpCode.

**/
BOOLEAN
IsStatementOpCode (
  IN UINT8              Operand
  )
{
  if ((Operand == EFI_IFR_SUBTITLE_OP) ||
      (Operand == EFI_IFR_TEXT_OP) ||
      (Operand == EFI_IFR_RESET_BUTTON_OP) ||
      (Operand == EFI_IFR_REF_OP) ||
      (Operand == EFI_IFR_ACTION_OP) ||
      (Operand == EFI_IFR_NUMERIC_OP) ||
      (Operand == EFI_IFR_ORDERED_LIST_OP) ||
      (Operand == EFI_IFR_CHECKBOX_OP) ||
      (Operand == EFI_IFR_STRING_OP) ||
      (Operand == EFI_IFR_PASSWORD_OP) ||
      (Operand == EFI_IFR_DATE_OP) ||
      (Operand == EFI_IFR_TIME_OP) ||
      (Operand == EFI_IFR_GUID_OP) ||
      (Operand == EFI_IFR_ONE_OF_OP)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Tell whether this Operand is an Statement OpCode.

  @param  Operand                Operand of an IFR OpCode.

  @retval TRUE                   This is an Statement OpCode.
  @retval FALSE                  Not an Statement OpCode.

**/
BOOLEAN
IsStorageOpCode (
  IN UINT8              Operand
  )
{
  if ((Operand == EFI_IFR_VARSTORE_OP) ||
      (Operand == EFI_IFR_VARSTORE_NAME_VALUE_OP) ||
      (Operand == EFI_IFR_VARSTORE_EFI_OP)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Base on the prompt string id to find the question.

  @param  FormPackage            The input form package.
  @param  KeywordStrId           The input prompt string id for one question.

  @retval  the opcode for the question.

**/
UINT8 *
FindQuestionFromStringId (
  IN HII_IFR_PACKAGE_INSTANCE      *FormPackage,
  IN EFI_STRING_ID                 KeywordStrId
  )
{
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  EFI_IFR_STATEMENT_HEADER     *StatementHeader;
  EFI_IFR_OP_HEADER            *OpCodeHeader;
  UINT32                       FormDataLen;

  ASSERT (FormPackage != NULL);

  FormDataLen = FormPackage->FormPkgHdr.Length - sizeof (EFI_HII_PACKAGE_HEADER);
  Offset = 0;
  while (Offset < FormDataLen) {
    OpCodeData = FormPackage->IfrData + Offset;
    OpCodeHeader = (EFI_IFR_OP_HEADER *) OpCodeData;

    if (IsStatementOpCode(OpCodeHeader->OpCode)) {
      StatementHeader = (EFI_IFR_STATEMENT_HEADER *) (OpCodeData + sizeof (EFI_IFR_OP_HEADER));
      if (StatementHeader->Prompt == KeywordStrId) {
        return OpCodeData;
      }
    }

    Offset += OpCodeHeader->Length;
  }

  return NULL;
}

/**
  Base on the varstore id to find the storage info.

  @param  FormPackage            The input form package.
  @param  VarStoreId             The input storage id.

  @retval  the opcode for the storage.

**/
UINT8 *
FindStorageFromVarId (
  IN HII_IFR_PACKAGE_INSTANCE      *FormPackage,
  IN EFI_VARSTORE_ID               VarStoreId
  )
{
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  EFI_IFR_OP_HEADER            *OpCodeHeader;
  UINT32                       FormDataLen;

  ASSERT (FormPackage != NULL);

  FormDataLen = FormPackage->FormPkgHdr.Length - sizeof (EFI_HII_PACKAGE_HEADER);
  Offset = 0;
  while (Offset < FormDataLen) {
    OpCodeData = FormPackage->IfrData + Offset;
    OpCodeHeader = (EFI_IFR_OP_HEADER *) OpCodeData;

    if (IsStorageOpCode(OpCodeHeader->OpCode)) {
      switch (OpCodeHeader->OpCode) {
      case EFI_IFR_VARSTORE_OP:
        if (VarStoreId == ((EFI_IFR_VARSTORE *) OpCodeData)->VarStoreId) {
          return OpCodeData;
        }
        break;

      case EFI_IFR_VARSTORE_NAME_VALUE_OP:
        if (VarStoreId == ((EFI_IFR_VARSTORE_NAME_VALUE *) OpCodeData)->VarStoreId) {
          return OpCodeData;
        }
        break;

      case EFI_IFR_VARSTORE_EFI_OP:
        if (VarStoreId == ((EFI_IFR_VARSTORE_EFI *) OpCodeData)->VarStoreId) {
          return OpCodeData;
        }
        break;

      default:
        break;
      }
    }

    Offset += OpCodeHeader->Length;
  }

  return NULL;
}

/**
  Get width info for one question.

  @param  OpCodeData            The input opcode for one question.

  @retval  the width info for one question.

**/
UINT16
GetWidth (
  IN UINT8        *OpCodeData
  )
{
  UINT8      *NextOpCodeData;

  ASSERT (OpCodeData != NULL);

  switch (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode) {
  case EFI_IFR_REF_OP:
    return (UINT16) sizeof (EFI_HII_REF);

  case EFI_IFR_ONE_OF_OP:
  case EFI_IFR_NUMERIC_OP:
    switch (((EFI_IFR_ONE_OF *) OpCodeData)->Flags & EFI_IFR_NUMERIC_SIZE) {
    case EFI_IFR_NUMERIC_SIZE_1:
      return (UINT16) sizeof (UINT8);

    case EFI_IFR_NUMERIC_SIZE_2:
      return  (UINT16) sizeof (UINT16);

    case EFI_IFR_NUMERIC_SIZE_4:
      return (UINT16) sizeof (UINT32);

    case EFI_IFR_NUMERIC_SIZE_8:
      return (UINT16) sizeof (UINT64);

    default:
      ASSERT (FALSE);
      return 0;
    }

  case EFI_IFR_ORDERED_LIST_OP:
    NextOpCodeData = OpCodeData + ((EFI_IFR_ORDERED_LIST *) OpCodeData)->Header.Length;
    //
    // OneOfOption must follow the orderedlist opcode.
    //
    ASSERT (((EFI_IFR_OP_HEADER *) NextOpCodeData)->OpCode == EFI_IFR_ONE_OF_OPTION_OP);
    switch (((EFI_IFR_ONE_OF_OPTION *) NextOpCodeData)->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      return (UINT16) sizeof (UINT8) * ((EFI_IFR_ORDERED_LIST *) OpCodeData)->MaxContainers;

    case EFI_IFR_TYPE_NUM_SIZE_16:
      return (UINT16) sizeof (UINT16) * ((EFI_IFR_ORDERED_LIST *) OpCodeData)->MaxContainers ;

    case EFI_IFR_TYPE_NUM_SIZE_32:
      return (UINT16) sizeof (UINT32) * ((EFI_IFR_ORDERED_LIST *) OpCodeData)->MaxContainers;

    case EFI_IFR_TYPE_NUM_SIZE_64:
      return (UINT16) sizeof (UINT64) * ((EFI_IFR_ORDERED_LIST *) OpCodeData)->MaxContainers;

    default:
      ASSERT (FALSE);
      return 0;
    }

  case EFI_IFR_CHECKBOX_OP:
    return (UINT16) sizeof (BOOLEAN);

  case EFI_IFR_PASSWORD_OP:
    return (UINT16)((UINTN) ((EFI_IFR_PASSWORD *) OpCodeData)->MaxSize * sizeof (CHAR16));

  case EFI_IFR_STRING_OP:
    return (UINT16)((UINTN) ((EFI_IFR_STRING *) OpCodeData)->MaxSize * sizeof (CHAR16));

  case EFI_IFR_DATE_OP:
    return (UINT16) sizeof (EFI_HII_DATE);

  case EFI_IFR_TIME_OP:
    return (UINT16) sizeof (EFI_HII_TIME);

  default:
    ASSERT (FALSE);
    return 0;
  }
}

/**
  Converts all hex string characters in range ['A'..'F'] to ['a'..'f'] for
  hex digits that appear between a '=' and a '&' in a config string.

  If ConfigString is NULL, then ASSERT().

  @param[in] ConfigString  Pointer to a Null-terminated Unicode string.

  @return  Pointer to the Null-terminated Unicode result string.

**/
EFI_STRING
EFIAPI
InternalLowerConfigString (
  IN EFI_STRING  ConfigString
  )
{
  EFI_STRING  String;
  BOOLEAN     Lower;

  ASSERT (ConfigString != NULL);

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  for (String = ConfigString, Lower = FALSE; *String != L'\0'; String++) {
    if (*String == L'=') {
      Lower = TRUE;
    } else if (*String == L'&') {
      Lower = FALSE;
    } else if (Lower && *String >= L'A' && *String <= L'F') {
      *String = (CHAR16) (*String - L'A' + L'a');
    }
  }

  return ConfigString;
}

/**
  Allocates and returns a Null-terminated Unicode <ConfigHdr> string.

  The format of a <ConfigHdr> is as follows:

    GUID=<HexCh>32&NAME=<Char>NameLength&PATH=<HexChar>DevicePathSize<Null>

  @param[in]  OpCodeData    The opcode for the storage.
  @param[in]  DriverHandle  The driver handle which supports a Device Path Protocol
                            that is the routing information PATH.  Each byte of
                            the Device Path associated with DriverHandle is converted
                            to a 2 Unicode character hexadecimal string.

  @retval NULL   DriverHandle does not support the Device Path Protocol.
  @retval Other  A pointer to the Null-terminate Unicode <ConfigHdr> string

**/
EFI_STRING
ConstructConfigHdr (
  IN UINT8           *OpCodeData,
  IN EFI_HANDLE      DriverHandle
  )
{
  UINTN                     NameLength;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     DevicePathSize;
  CHAR16                    *String;
  CHAR16                    *ReturnString;
  UINTN                     Index;
  UINT8                     *Buffer;
  CHAR16                    *Name;
  CHAR8                     *AsciiName;
  UINTN                     NameSize;
  EFI_GUID                  *Guid;
  UINTN                     MaxLen;

  ASSERT (OpCodeData != NULL);

  switch (((EFI_IFR_OP_HEADER *)OpCodeData)->OpCode) {
  case EFI_IFR_VARSTORE_OP:
    Guid      = (EFI_GUID *)(UINTN *)&((EFI_IFR_VARSTORE *) OpCodeData)->Guid;
    AsciiName = (CHAR8 *) ((EFI_IFR_VARSTORE *) OpCodeData)->Name;
    break;

  case EFI_IFR_VARSTORE_NAME_VALUE_OP:
    Guid      = (EFI_GUID *)(UINTN *)&((EFI_IFR_VARSTORE_NAME_VALUE *) OpCodeData)->Guid;
    AsciiName = NULL;
    break;

  case EFI_IFR_VARSTORE_EFI_OP:
    Guid      = (EFI_GUID *)(UINTN *)&((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Guid;
    AsciiName = (CHAR8 *) ((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Name;
    break;

  default:
    ASSERT (FALSE);
    Guid      = NULL;
    AsciiName = NULL;
    break;
  }

  if (AsciiName != NULL) {
    NameSize = AsciiStrSize (AsciiName);
    Name = AllocateZeroPool (NameSize * sizeof (CHAR16));
    ASSERT (Name != NULL);
    AsciiStrToUnicodeStrS (AsciiName, Name, NameSize);
  } else {
    Name = NULL;
  }

  //
  // Compute the length of Name in Unicode characters.
  // If Name is NULL, then the length is 0.
  //
  NameLength = 0;
  if (Name != NULL) {
    NameLength = StrLen (Name);
  }

  DevicePath = NULL;
  DevicePathSize = 0;
  //
  // Retrieve DevicePath Protocol associated with DriverHandle
  //
  if (DriverHandle != NULL) {
    DevicePath = DevicePathFromHandle (DriverHandle);
    if (DevicePath == NULL) {
      return NULL;
    }
    //
    // Compute the size of the device path in bytes
    //
    DevicePathSize = GetDevicePathSize (DevicePath);
  }

  //
  // GUID=<HexCh>32&NAME=<Char>NameLength&PATH=<HexChar>DevicePathSize <Null>
  // | 5 | sizeof (EFI_GUID) * 2 | 6 | NameStrLen*4 | 6 | DevicePathSize * 2 | 1 |
  //
  MaxLen = 5 + sizeof (EFI_GUID) * 2 + 6 + NameLength * 4 + 6 + DevicePathSize * 2 + 1;
  String = AllocateZeroPool (MaxLen * sizeof (CHAR16));
  if (String == NULL) {
    return NULL;
  }

  //
  // Start with L"GUID="
  //
  StrCpyS (String, MaxLen, L"GUID=");
  ReturnString = String;
  String += StrLen (String);

  if (Guid != NULL) {
    //
    // Append Guid converted to <HexCh>32
    //
    for (Index = 0, Buffer = (UINT8 *)Guid; Index < sizeof (EFI_GUID); Index++) {
      UnicodeValueToStringS (
        String,
        MaxLen * sizeof (CHAR16) - ((UINTN)String - (UINTN)ReturnString),
        PREFIX_ZERO | RADIX_HEX,
        *(Buffer++),
        2
        );
      String += StrnLenS (String, MaxLen - ((UINTN)String - (UINTN)ReturnString) / sizeof (CHAR16));
    }
  }

  //
  // Append L"&NAME="
  //
  StrCatS (ReturnString, MaxLen, L"&NAME=");
  String += StrLen (String);

  if (Name != NULL) {
    //
    // Append Name converted to <Char>NameLength
    //
    for (; *Name != L'\0'; Name++) {
      UnicodeValueToStringS (
        String,
        MaxLen * sizeof (CHAR16) - ((UINTN)String - (UINTN)ReturnString),
        PREFIX_ZERO | RADIX_HEX,
        *Name,
        4
        );
      String += StrnLenS (String, MaxLen - ((UINTN)String - (UINTN)ReturnString) / sizeof (CHAR16));
    }
  }

  //
  // Append L"&PATH="
  //
  StrCatS (ReturnString, MaxLen, L"&PATH=");
  String += StrLen (String);

  //
  // Append the device path associated with DriverHandle converted to <HexChar>DevicePathSize
  //
  for (Index = 0, Buffer = (UINT8 *)DevicePath; Index < DevicePathSize; Index++) {
    UnicodeValueToStringS (
      String,
      MaxLen * sizeof (CHAR16) - ((UINTN)String - (UINTN)ReturnString),
      PREFIX_ZERO | RADIX_HEX,
      *(Buffer++),
      2
      );
    String += StrnLenS (String, MaxLen - ((UINTN)String - (UINTN)ReturnString) / sizeof (CHAR16));
  }

  //
  // Null terminate the Unicode string
  //
  *String = L'\0';

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  return InternalLowerConfigString (ReturnString);
}

/**
  Generate the Config request element for one question.

  @param   Name    The name info for one question.
  @param   Offset  The offset info for one question.
  @param   Width   The width info for one question.

  @return  Pointer to the Null-terminated Unicode request element string.

**/
EFI_STRING
ConstructRequestElement (
  IN CHAR16      *Name,
  IN UINT16      Offset,
  IN UINT16      Width
  )
{
  CHAR16    *StringPtr;
  UINTN     Length;

  if (Name != NULL) {
    //
    // Add <BlockName> length for each Name
    //
    // <BlockName> ::= Name + \0
    //                 StrLen(Name) | 1
    //
    Length = StrLen (Name) + 1;
  } else {
    //
    // Add <BlockName> length for each Offset/Width pair
    //
    // <BlockName> ::= OFFSET=1234&WIDTH=1234 + \0
    //                 |  7   | 4 |   7  | 4 |  1
    //
    Length = (7 + 4 + 7 + 4 + 1);
  }

  //
  // Allocate buffer for the entire <ConfigRequest>
  //
  StringPtr = AllocateZeroPool (Length * sizeof (CHAR16));
  ASSERT (StringPtr != NULL);

  if (Name != NULL) {
    //
    // Append Name\0
    //
    UnicodeSPrint (
      StringPtr,
      (StrLen (Name) + 1) * sizeof (CHAR16),
      L"%s",
      Name
    );
  } else {
    //
    // Append OFFSET=XXXX&WIDTH=YYYY\0
    //
    UnicodeSPrint (
      StringPtr,
      (7 + 4 + 7 + 4 + 1) * sizeof (CHAR16),
      L"OFFSET=%04X&WIDTH=%04X",
      Offset,
      Width
    );
  }

  return StringPtr;
}

/**
  Get string value for question's name field.

  @param  DatabaseRecord                 HII_DATABASE_RECORD format string.
  @param  NameId                         The string id for the name field.

  @retval Name string.

**/
CHAR16 *
GetNameFromId (
  IN HII_DATABASE_RECORD   *DatabaseRecord,
  IN EFI_STRING_ID         NameId
  )
{
  CHAR16      *Name;
  CHAR8       *PlatformLanguage;
  CHAR8       *SupportedLanguages;
  CHAR8       *BestLanguage;
  UINTN       StringSize;
  CHAR16      TempString;
  EFI_STATUS  Status;

  Name = NULL;
  BestLanguage = NULL;
  PlatformLanguage = NULL;
  SupportedLanguages = NULL;

  GetEfiGlobalVariable2 (L"PlatformLang", (VOID**)&PlatformLanguage, NULL);
  SupportedLanguages = GetSupportedLanguages(DatabaseRecord->Handle);

  //
  // Get the best matching language from SupportedLanguages
  //
  BestLanguage = GetBestLanguage (
                   SupportedLanguages,
                   FALSE,                                             // RFC 4646 mode
                   PlatformLanguage != NULL ? PlatformLanguage : "",  // Highest priority
                   SupportedLanguages,                                // Lowest priority
                   NULL
                   );
  if (BestLanguage == NULL) {
    BestLanguage = AllocateCopyPool (AsciiStrLen ("en-US"), "en-US");
    ASSERT (BestLanguage != NULL);
  }

  StringSize = 0;
  Status = mPrivate.HiiString.GetString (
                                 &mPrivate.HiiString,
                                 BestLanguage,
                                 DatabaseRecord->Handle,
                                 NameId,
                                 &TempString,
                                 &StringSize,
                                 NULL
                                 );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    goto Done;
  }

  Name = AllocateZeroPool (StringSize);
  if (Name == NULL) {
    goto Done;
  }

  Status = mPrivate.HiiString.GetString (
                          &mPrivate.HiiString,
                          BestLanguage,
                          DatabaseRecord->Handle,
                          NameId,
                          Name,
                          &StringSize,
                          NULL
                          );

  if (EFI_ERROR (Status)) {
    FreePool (Name);
    Name = NULL;
    goto Done;
  }

Done:
  if (SupportedLanguages != NULL) {
    FreePool(SupportedLanguages);
  }
  if (BestLanguage != NULL) {
    FreePool (BestLanguage);
  }
  if (PlatformLanguage != NULL) {
    FreePool (PlatformLanguage);
  }

  return Name;
}

/**
  Base on the input parameter to generate the ConfigRequest string.

  This is a internal function.

  @param  DatabaseRecord                 HII_DATABASE_RECORD format string.
  @param  KeywordStrId                   Keyword string id.
  @param  OpCodeData                     The IFR data for this question.
  @param  ConfigRequest                  Return the generate ConfigRequest string.

  @retval EFI_SUCCESS               Generate ConfigResp string success.
  @retval EFI_OUT_OF_RESOURCES      System out of memory resource error.
  @retval EFI_NOT_FOUND             Not found the question which use this string id
                                    as the prompt string id.
**/
EFI_STATUS
ExtractConfigRequest (
  IN  HII_DATABASE_RECORD   *DatabaseRecord,
  IN  EFI_STRING_ID         KeywordStrId,
  OUT UINT8                 **OpCodeData,
  OUT EFI_STRING            *ConfigRequest
  )
{
  LIST_ENTRY                          *Link;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_IFR_PACKAGE_INSTANCE            *FormPackage;
  EFI_IFR_QUESTION_HEADER             *Header;
  UINT8                               *Storage;
  UINT8                               *OpCode;
  CHAR16                              *Name;
  UINT16                              Offset;
  UINT16                              Width;
  CHAR16                              *ConfigHdr;
  CHAR16                              *RequestElement;
  UINTN                               MaxLen;
  CHAR16                              *StringPtr;

  ASSERT (DatabaseRecord != NULL && OpCodeData != NULL && ConfigRequest != NULL);

  OpCode = NULL;
  Name   = NULL;
  Width  = 0;
  Offset = 0;

  PackageListNode = DatabaseRecord->PackageList;

  //
  // Search the languages in the specified packagelist.
  //
  for (Link = PackageListNode->FormPkgHdr.ForwardLink; Link != &PackageListNode->FormPkgHdr; Link = Link->ForwardLink) {
    FormPackage = CR (Link, HII_IFR_PACKAGE_INSTANCE, IfrEntry, HII_IFR_PACKAGE_SIGNATURE);

    OpCode = FindQuestionFromStringId (FormPackage, KeywordStrId);
    if (OpCode != NULL) {
      *OpCodeData = OpCode;
      Header = (EFI_IFR_QUESTION_HEADER *) (OpCode + sizeof (EFI_IFR_OP_HEADER));
      //
      // Header->VarStoreId == 0 means no storage for this question.
      //
      ASSERT (Header->VarStoreId != 0);
      DEBUG ((EFI_D_INFO, "Varstore Id: 0x%x\n", Header->VarStoreId));

      Storage = FindStorageFromVarId (FormPackage, Header->VarStoreId);
      ASSERT (Storage != NULL);

      if (((EFI_IFR_OP_HEADER *) Storage)->OpCode == EFI_IFR_VARSTORE_NAME_VALUE_OP) {
        Name = GetNameFromId (DatabaseRecord, Header->VarStoreInfo.VarName);
      } else {
        Offset = Header->VarStoreInfo.VarOffset;
        Width = GetWidth (OpCode);
      }
      RequestElement = ConstructRequestElement(Name, Offset, Width);
      ConfigHdr = ConstructConfigHdr(Storage, DatabaseRecord->DriverHandle);
      ASSERT (ConfigHdr != NULL);

      MaxLen = StrLen (ConfigHdr) + 1 + StrLen(RequestElement) + 1;
      *ConfigRequest = AllocatePool (MaxLen * sizeof (CHAR16));
      if (*ConfigRequest == NULL) {
        FreePool (ConfigHdr);
        FreePool (RequestElement);
        return EFI_OUT_OF_RESOURCES;
      }
      StringPtr = *ConfigRequest;

      StrCpyS (StringPtr, MaxLen, ConfigHdr);

      StrCatS (StringPtr, MaxLen, L"&");

      StrCatS (StringPtr, MaxLen, RequestElement);

      FreePool (ConfigHdr);
      FreePool (RequestElement);

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Base on the input parameter to generate the ConfigResp string.

  This is a internal function.

  @param  DatabaseRecord                 HII_DATABASE_RECORD format string.
  @param  KeywordStrId                   Keyword string id.
  @param  ValueElement                   The value for the question which use keyword string id
                                         as the prompt string id.
  @param  OpCodeData                     The IFR data for this question.
  @param  ConfigResp                     Return the generate ConfigResp string.

  @retval EFI_SUCCESS               Generate ConfigResp string success.
  @retval EFI_OUT_OF_RESOURCES      System out of memory resource error.
  @retval EFI_NOT_FOUND             Not found the question which use this string id
                                    as the prompt string id.
**/
EFI_STATUS
ExtractConfigResp (
  IN  HII_DATABASE_RECORD   *DatabaseRecord,
  IN  EFI_STRING_ID         KeywordStrId,
  IN  EFI_STRING            ValueElement,
  OUT UINT8                 **OpCodeData,
  OUT EFI_STRING            *ConfigResp
  )
{
  LIST_ENTRY                          *Link;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_IFR_PACKAGE_INSTANCE            *FormPackage;
  EFI_IFR_QUESTION_HEADER             *Header;
  UINT8                               *Storage;
  UINT8                               *OpCode;
  CHAR16                              *Name;
  UINT16                              Offset;
  UINT16                              Width;
  CHAR16                              *ConfigHdr;
  CHAR16                              *RequestElement;
  UINTN                               MaxLen;
  CHAR16                              *StringPtr;

  ASSERT ((DatabaseRecord != NULL) && (OpCodeData != NULL) && (ConfigResp != NULL) && (ValueElement != NULL));

  OpCode = NULL;
  Name   = NULL;
  Width  = 0;
  Offset = 0;

  PackageListNode = DatabaseRecord->PackageList;

  //
  // Search the languages in the specified packagelist.
  //
  for (Link = PackageListNode->FormPkgHdr.ForwardLink; Link != &PackageListNode->FormPkgHdr; Link = Link->ForwardLink) {
    FormPackage = CR (Link, HII_IFR_PACKAGE_INSTANCE, IfrEntry, HII_IFR_PACKAGE_SIGNATURE);

    OpCode = FindQuestionFromStringId (FormPackage, KeywordStrId);
    if (OpCode != NULL) {
      *OpCodeData = OpCode;
      Header = (EFI_IFR_QUESTION_HEADER *) (OpCode + sizeof (EFI_IFR_OP_HEADER));
      //
      // Header->VarStoreId == 0 means no storage for this question.
      //
      ASSERT (Header->VarStoreId != 0);
      DEBUG ((EFI_D_INFO, "Varstore Id: 0x%x\n", Header->VarStoreId));

      Storage = FindStorageFromVarId (FormPackage, Header->VarStoreId);
      ASSERT (Storage != NULL);

      if (((EFI_IFR_OP_HEADER *) Storage)->OpCode == EFI_IFR_VARSTORE_NAME_VALUE_OP) {
        Name = GetNameFromId (DatabaseRecord, Header->VarStoreInfo.VarName);
      } else {
        Offset = Header->VarStoreInfo.VarOffset;
        Width  = GetWidth (OpCode);
      }
      RequestElement = ConstructRequestElement(Name, Offset, Width);

      ConfigHdr = ConstructConfigHdr(Storage, DatabaseRecord->DriverHandle);
      ASSERT (ConfigHdr != NULL);

      MaxLen = StrLen (ConfigHdr) + 1 + StrLen(RequestElement) + 1 + StrLen (L"VALUE=") + StrLen(ValueElement) + 1;
      *ConfigResp = AllocatePool (MaxLen * sizeof (CHAR16));
      if (*ConfigResp == NULL) {
        FreePool (ConfigHdr);
        FreePool (RequestElement);
        return EFI_OUT_OF_RESOURCES;
      }
      StringPtr = *ConfigResp;

      StrCpyS (StringPtr, MaxLen, ConfigHdr);

      StrCatS (StringPtr, MaxLen, L"&");


      StrCatS (StringPtr, MaxLen, RequestElement);

      StrCatS (StringPtr, MaxLen, L"&");

      StrCatS (StringPtr, MaxLen, L"VALUE=");

      StrCatS (StringPtr, MaxLen, ValueElement);

      FreePool (ConfigHdr);
      FreePool (RequestElement);

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get the Value section from the Hii driver.

  This is a internal function.

  @param  ConfigRequest                  The input ConfigRequest string.
  @param  ValueElement                   The respond Value section from the hii driver.

  @retval Misc value                     The error status return from ExtractConfig function.
  @retval EFI_OUT_OF_RESOURCES           The memory can't be allocated
  @retval EFI_SUCCESS                    Get the value section success.

**/
EFI_STATUS
ExtractValueFromDriver (
  IN  CHAR16           *ConfigRequest,
  OUT CHAR16           **ValueElement
  )
{
  EFI_STATUS   Status;
  EFI_STRING   Result;
  EFI_STRING   Progress;
  CHAR16       *StringPtr;
  CHAR16       *StringEnd;

  ASSERT ((ConfigRequest != NULL) && (ValueElement != NULL));

  Status = mPrivate.ConfigRouting.ExtractConfig (
                      &mPrivate.ConfigRouting,
                      (EFI_STRING) ConfigRequest,
                      &Progress,
                      &Result
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find Value Section and return it.
  //
  StringPtr = StrStr (Result, L"&VALUE=");
  ASSERT (StringPtr != NULL);
  StringEnd = StrStr (StringPtr + 1, L"&");
  if (StringEnd != NULL) {
    *StringEnd = L'\0';
  }

  *ValueElement = AllocateCopyPool (StrSize (StringPtr), StringPtr);
  if (*ValueElement == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (StringEnd != NULL) {
    *StringEnd = L'&';
  }
  FreePool (Result);

  return EFI_SUCCESS;
}

/**
  Get EFI_STRING_ID info from the input device path, namespace and keyword.

  This is a internal function.

  @param  DevicePath                     Input device path info.
  @param  NameSpace                      NameSpace format string.
  @param  KeywordData                    Keyword used to get string id.
  @param  ProgressErr                    Return extra error type.
  @param  KeywordStringId                Return EFI_STRING_ID.
  @param  DataBaseRecord                 DataBase record data for this driver.

  @retval EFI_INVALID_PARAMETER          Can't find the database record base on the input device path or namespace.
  @retval EFI_NOT_FOUND                  Can't find the EFI_STRING_ID for the keyword.
  @retval EFI_SUCCESS                    Find the EFI_STRING_ID.

**/
EFI_STATUS
GetStringIdFromDatabase (
  IN  EFI_DEVICE_PATH_PROTOCOL            **DevicePath,
  IN  CHAR8                               **NameSpace,
  IN  CHAR16                              *KeywordData,
  OUT UINT32                              *ProgressErr,
  OUT EFI_STRING_ID                       *KeywordStringId,
  OUT HII_DATABASE_RECORD                 **DataBaseRecord
 )
{
  HII_DATABASE_RECORD                 *Record;
  LIST_ENTRY                          *Link;
  BOOLEAN                             FindNameSpace;
  EFI_DEVICE_PATH_PROTOCOL            *DestDevicePath;
  UINT8                               *DevicePathPkg;
  UINTN                               DevicePathSize;

  ASSERT ((NameSpace != NULL) && (KeywordData != NULL) && (ProgressErr != NULL) && (KeywordStringId != NULL) && (DataBaseRecord != NULL));

  FindNameSpace = FALSE;

  if (*DevicePath != NULL) {
    //
    // Get DataBaseRecord from device path protocol.
    //
    Record = GetRecordFromDevicePath(*DevicePath);
    if (Record == NULL) {
      //
      // Can't find the DatabaseRecord base on the input device path info.
      // NEED TO CONFIRM the return ProgressErr.
      //
      *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
      return EFI_INVALID_PARAMETER;
    }

    //
    // Get string id from the record.
    //
    *ProgressErr = GetStringIdFromRecord (Record, NameSpace, KeywordData, KeywordStringId);
    switch (*ProgressErr) {
    case KEYWORD_HANDLER_NO_ERROR:
      *DataBaseRecord = Record;
      return EFI_SUCCESS;

    case KEYWORD_HANDLER_NAMESPACE_ID_NOT_FOUND:
      return EFI_INVALID_PARAMETER;

    default:
      ASSERT (*ProgressErr == KEYWORD_HANDLER_KEYWORD_NOT_FOUND);
      return EFI_NOT_FOUND;
    }
  } else {
    //
    // Find driver which matches the routing data.
    //
    for (Link = mPrivate.DatabaseList.ForwardLink; Link != &mPrivate.DatabaseList; Link = Link->ForwardLink) {
      Record = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);

      *ProgressErr = GetStringIdFromRecord (Record, NameSpace, KeywordData, KeywordStringId);
      if (*ProgressErr == KEYWORD_HANDLER_NO_ERROR) {
        *DataBaseRecord = Record;

        if ((DevicePathPkg = Record->PackageList->DevicePathPkg) != NULL) {
          DestDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) (DevicePathPkg + sizeof (EFI_HII_PACKAGE_HEADER));
          DevicePathSize = GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) DestDevicePath);
          *DevicePath = AllocateCopyPool (DevicePathSize, DestDevicePath);
          if (*DevicePath == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }
        } else {
          //
          // Need to verify this ASSERT.
          //
          ASSERT (FALSE);
        }

        return EFI_SUCCESS;
      } else if (*ProgressErr == KEYWORD_HANDLER_UNDEFINED_PROCESSING_ERROR) {
        return EFI_OUT_OF_RESOURCES;
      } else if (*ProgressErr == KEYWORD_HANDLER_KEYWORD_NOT_FOUND) {
        FindNameSpace = TRUE;
      }
    }

    //
    // When PathHdr not input, if ever find the namespace, will return KEYWORD_HANDLER_KEYWORD_NOT_FOUND.
    // This is a bit more progress than KEYWORD_HANDLER_NAMESPACE_ID_NOT_FOUND.
    //
    if (FindNameSpace) {
      return EFI_NOT_FOUND;
    } else {
      return EFI_INVALID_PARAMETER;
    }
  }
}

/**
  Generate the KeywordResp String.

  <KeywordResp> ::= <NameSpaceId><PathHdr>'&'<Keyword>'&VALUE='<Number>['&READONLY']

  @param  NameSpace                      NameSpace format string.
  @param  DevicePath                     Input device path info.
  @param  KeywordData                    Keyword used to get string id.
  @param  ValueStr                       The value section for the keyword.
  @param  ReadOnly                       Whether this value is readonly.
  @param  KeywordResp                    Return the point to the KeywordResp string.

  @retval EFI_OUT_OF_RESOURCES           The memory can't be allocated.
  @retval EFI_SUCCESS                    Generate the KeywordResp string.

**/
EFI_STATUS
GenerateKeywordResp (
  IN  CHAR8                          *NameSpace,
  IN  EFI_DEVICE_PATH_PROTOCOL       *DevicePath,
  IN  EFI_STRING                     KeywordData,
  IN  EFI_STRING                     ValueStr,
  IN  BOOLEAN                        ReadOnly,
  OUT EFI_STRING                     *KeywordResp
  )
{
  UINTN     RespStrLen;
  CHAR16    *RespStr;
  CHAR16    *PathHdr;
  CHAR16    *UnicodeNameSpace;
  UINTN     NameSpaceLength;

  ASSERT ((NameSpace != NULL) && (DevicePath != NULL) && (KeywordData != NULL) && (ValueStr != NULL) && (KeywordResp != NULL));

  //
  // 1. Calculate the string length.
  //
  //
  // 1.1 NameSpaceId size.
  // 'NAMESPACE='<String>
  //
  NameSpaceLength = AsciiStrLen (NameSpace);
  RespStrLen = 10 + NameSpaceLength;
  UnicodeNameSpace = AllocatePool ((NameSpaceLength + 1) * sizeof (CHAR16));
  if (UnicodeNameSpace == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  AsciiStrToUnicodeStrS (NameSpace, UnicodeNameSpace, NameSpaceLength + 1);

  //
  // 1.2 PathHdr size.
  // PATH=<UEFI binary Device Path represented as hex number>'&'
  // Attention: The output include the '&' at the end.
  //
  GenerateSubStr (
    L"&PATH=",
    GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) DevicePath),
    (VOID *) DevicePath,
    1,
    &PathHdr
    );
  RespStrLen += StrLen (PathHdr);

  //
  // 1.3 Keyword section.
  // 'KEYWORD='<String>[':'<DecCh>(1/4)]
  //
  RespStrLen += 8 + StrLen (KeywordData);

  //
  // 1.4 Value section.
  // ValueStr = '&VALUE='<Number>
  //
  RespStrLen += StrLen (ValueStr);

  //
  // 1.5 ReadOnly Section.
  // '&READONLY'
  //
  if (ReadOnly) {
    RespStrLen += 9;
  }

  //
  // 2. Allocate the buffer and create the KeywordResp string include '\0'.
  //
  RespStrLen += 1;
  *KeywordResp = AllocatePool (RespStrLen * sizeof (CHAR16));
  if (*KeywordResp == NULL) {
    if (UnicodeNameSpace != NULL) {
      FreePool (UnicodeNameSpace);
    }

    return EFI_OUT_OF_RESOURCES;
  }
  RespStr = *KeywordResp;

  //
  // 2.1 Copy NameSpaceId section.
  //
  StrCpyS (RespStr, RespStrLen, L"NAMESPACE=");

  StrCatS (RespStr, RespStrLen, UnicodeNameSpace);

  //
  // 2.2 Copy PathHdr section.
  //
  StrCatS (RespStr, RespStrLen, PathHdr);

  //
  // 2.3 Copy Keyword section.
  //
  StrCatS (RespStr, RespStrLen, L"KEYWORD=");

  StrCatS (RespStr, RespStrLen, KeywordData);

  //
  // 2.4 Copy the Value section.
  //
  StrCatS (RespStr, RespStrLen, ValueStr);

  //
  // 2.5 Copy ReadOnly section if exist.
  //
  if (ReadOnly) {
    StrCatS (RespStr, RespStrLen, L"&READONLY");
  }

  if (UnicodeNameSpace != NULL) {
    FreePool (UnicodeNameSpace);
  }
  if (PathHdr != NULL) {
    FreePool (PathHdr);
  }

  return EFI_SUCCESS;
}

/**
  Merge the KeywordResp String to MultiKeywordResp string.

  This is a internal function.

  @param  MultiKeywordResp               The existed multikeywordresp string.
  @param  KeywordResp                    The input keywordResp string.

  @retval EFI_OUT_OF_RESOURCES           The memory can't be allocated.
  @retval EFI_SUCCESS                    Generate the MultiKeywordResp string.

**/
EFI_STATUS
MergeToMultiKeywordResp (
  IN OUT EFI_STRING         *MultiKeywordResp,
  IN     EFI_STRING         *KeywordResp
  )
{
  UINTN       MultiKeywordRespLen;
  EFI_STRING  StringPtr;

  if (*MultiKeywordResp == NULL) {
    *MultiKeywordResp = *KeywordResp;
    *KeywordResp = NULL;
    return EFI_SUCCESS;
  }

  MultiKeywordRespLen = (StrLen (*MultiKeywordResp) + 1 + StrLen (*KeywordResp) + 1) * sizeof (CHAR16);

  StringPtr = ReallocatePool (
                StrSize (*MultiKeywordResp),
                MultiKeywordRespLen,
                *MultiKeywordResp
                );
  if (StringPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *MultiKeywordResp = StringPtr;

  StrCatS (StringPtr, MultiKeywordRespLen / sizeof (CHAR16), L"&");

  StrCatS (StringPtr, MultiKeywordRespLen / sizeof (CHAR16), *KeywordResp);

  return EFI_SUCCESS;
}

/**
  Enumerate all keyword in the system.

  If error occur when parse one keyword, just skip it and parse the next one.

  This is a internal function.

  @param  NameSpace                      The namespace used to search the string.
  @param  MultiResp                      Return the MultiKeywordResp string for the system.
  @param  ProgressErr                    Return the error status.

  @retval EFI_OUT_OF_RESOURCES           The memory can't be allocated.
  @retval EFI_SUCCESS                    Generate the MultiKeywordResp string.
  @retval EFI_NOT_FOUND                  No keyword found.

**/
EFI_STATUS
EnumerateAllKeywords (
  IN  CHAR8             *NameSpace,
  OUT EFI_STRING        *MultiResp,
  OUT UINT32            *ProgressErr
  )
{
  LIST_ENTRY                          *Link;
  LIST_ENTRY                          *StringLink;
  UINT8                               *DevicePathPkg;
  UINT8                               *DevicePath;
  HII_DATABASE_RECORD                 *DataBaseRecord;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_STRING_PACKAGE_INSTANCE         *StringPackage;
  CHAR8                               *LocalNameSpace;
  EFI_STRING_ID                       NextStringId;
  EFI_STATUS                          Status;
  UINT8                               *OpCode;
  CHAR16                              *ConfigRequest;
  CHAR16                              *ValueElement;
  CHAR16                              *KeywordResp;
  CHAR16                              *MultiKeywordResp;
  CHAR16                              *KeywordData;
  BOOLEAN                             ReadOnly;
  BOOLEAN                             FindKeywordPackages;

  DataBaseRecord   = NULL;
  Status           = EFI_SUCCESS;
  MultiKeywordResp = NULL;
  DevicePath       = NULL;
  LocalNameSpace   = NULL;
  ConfigRequest    = NULL;
  ValueElement     = NULL;
  KeywordResp      = NULL;
  FindKeywordPackages = FALSE;

  if (NameSpace == NULL) {
    NameSpace = UEFI_CONFIG_LANG;
  }

  //
  // Find driver which matches the routing data.
  //
  for (Link = mPrivate.DatabaseList.ForwardLink; Link != &mPrivate.DatabaseList; Link = Link->ForwardLink) {
    DataBaseRecord = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
    if ((DevicePathPkg = DataBaseRecord->PackageList->DevicePathPkg) != NULL) {
      DevicePath = DevicePathPkg + sizeof (EFI_HII_PACKAGE_HEADER);
    }
    PackageListNode = DataBaseRecord->PackageList;

    for (StringLink = PackageListNode->StringPkgHdr.ForwardLink; StringLink != &PackageListNode->StringPkgHdr; StringLink = StringLink->ForwardLink) {
      StringPackage = CR (StringLink, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);

      //
      // Check whether has keyword string package.
      //
      if (AsciiStrnCmp(NameSpace, StringPackage->StringPkgHdr->Language, AsciiStrLen (NameSpace)) == 0) {
        FindKeywordPackages = TRUE;
        //
        // Keep the NameSpace string.
        //
        LocalNameSpace = AllocateCopyPool (AsciiStrSize (StringPackage->StringPkgHdr->Language), StringPackage->StringPkgHdr->Language);
        if (LocalNameSpace == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        //
        // 1 means just begin the enumerate the valid string ids.
        // StringId == 1 is always used to save the language for this string package.
        // Any valid string start from 2. so here initial it to 1.
        //
        NextStringId = 1;

        //
        // Enumerate all valid stringid in the package.
        //
        while ((NextStringId = GetNextStringId (StringPackage, NextStringId, &KeywordData)) != 0) {
          //
          // 3.3 Construct the ConfigRequest string.
          //
          Status = ExtractConfigRequest (DataBaseRecord, NextStringId, &OpCode, &ConfigRequest);
          if (EFI_ERROR (Status)) {
            //
            // If can't generate ConfigRequest for this question, skip it and start the next.
            //
            goto Error;
          }

          //
          // 3.4 Extract Value for the input keyword.
          //
          Status = ExtractValueFromDriver(ConfigRequest, &ValueElement);
          if (EFI_ERROR (Status)) {
            if (Status != EFI_OUT_OF_RESOURCES) {
              //
              // If can't generate ConfigRequest for this question, skip it and start the next.
              //
              goto Error;
            }
            //
            // If EFI_OUT_OF_RESOURCES error occur, no need to continue.
            //
            goto Done;
          }

          //
          // Extract readonly flag from opcode.
          //
          ReadOnly = ExtractReadOnlyFromOpCode(OpCode);

          //
          // 5. Generate KeywordResp string.
          //
          ASSERT (DevicePath != NULL);
          Status = GenerateKeywordResp(LocalNameSpace, (EFI_DEVICE_PATH_PROTOCOL *)DevicePath, KeywordData, ValueElement, ReadOnly, &KeywordResp);
          if (Status != EFI_SUCCESS) {
            //
            // If EFI_OUT_OF_RESOURCES error occur, no need to continue.
            //
            goto Done;
          }

          //
          // 6. Merge to the MultiKeywordResp string.
          //
          Status = MergeToMultiKeywordResp(&MultiKeywordResp, &KeywordResp);
          if (EFI_ERROR (Status)) {
            goto Done;
          }
Error:
          //
          // Clean the temp buffer to later use again.
          //
          if (ConfigRequest != NULL) {
            FreePool (ConfigRequest);
            ConfigRequest = NULL;
          }
          if (ValueElement != NULL) {
            FreePool (ValueElement);
            ValueElement = NULL;
          }
          if (KeywordResp != NULL) {
            FreePool (KeywordResp);
            KeywordResp = NULL;
          }
        }

        if (LocalNameSpace != NULL) {
          FreePool (LocalNameSpace);
          LocalNameSpace = NULL;
        }
      }
    }
  }

  //
  // return the already get MultiKeywordString even error occurred.
  //
  if (MultiKeywordResp == NULL) {
    Status = EFI_NOT_FOUND;
    if (!FindKeywordPackages) {
      *ProgressErr = KEYWORD_HANDLER_NAMESPACE_ID_NOT_FOUND;
    } else {
      *ProgressErr = KEYWORD_HANDLER_KEYWORD_NOT_FOUND;
    }
  } else {
    Status = EFI_SUCCESS;
  }
  *MultiResp = MultiKeywordResp;

Done:
  if (LocalNameSpace != NULL) {
    FreePool (LocalNameSpace);
  }
  if (ConfigRequest != NULL) {
    FreePool (ConfigRequest);
  }
  if (ValueElement != NULL) {
    FreePool (ValueElement);
  }

  return Status;
}

/**

  This function accepts a <MultiKeywordResp> formatted string, finds the associated
  keyword owners, creates a <MultiConfigResp> string from it and forwards it to the
  EFI_HII_ROUTING_PROTOCOL.RouteConfig function.

  If there is an issue in resolving the contents of the KeywordString, then the
  function returns an error and also sets the Progress and ProgressErr with the
  appropriate information about where the issue occurred and additional data about
  the nature of the issue.

  In the case when KeywordString containing multiple keywords, when an EFI_NOT_FOUND
  error is generated during processing the second or later keyword element, the system
  storage associated with earlier keywords is not modified. All elements of the
  KeywordString must successfully pass all tests for format and access prior to making
  any modifications to storage.

  In the case when EFI_DEVICE_ERROR is returned from the processing of a KeywordString
  containing multiple keywords, the state of storage associated with earlier keywords
  is undefined.


  @param This             Pointer to the EFI_KEYWORD_HANDLER _PROTOCOL instance.

  @param KeywordString    A null-terminated string in <MultiKeywordResp> format.

  @param Progress         On return, points to a character in the KeywordString.
                          Points to the string's NULL terminator if the request
                          was successful. Points to the most recent '&' before
                          the first failing name / value pair (or the beginning
                          of the string if the failure is in the first name / value
                          pair) if the request was not successful.

  @param ProgressErr      If during the processing of the KeywordString there was
                          a failure, this parameter gives additional information
                          about the possible source of the problem. The various
                          errors are defined in "Related Definitions" below.


  @retval EFI_SUCCESS             The specified action was completed successfully.

  @retval EFI_INVALID_PARAMETER   One or more of the following are TRUE:
                                  1. KeywordString is NULL.
                                  2. Parsing of the KeywordString resulted in an
                                     error. See Progress and ProgressErr for more data.

  @retval EFI_NOT_FOUND           An element of the KeywordString was not found.
                                  See ProgressErr for more data.

  @retval EFI_OUT_OF_RESOURCES    Required system resources could not be allocated.
                                  See ProgressErr for more data.

  @retval EFI_ACCESS_DENIED       The action violated system policy. See ProgressErr
                                  for more data.

  @retval EFI_DEVICE_ERROR        An unexpected system error occurred. See ProgressErr
                                  for more data.

**/
EFI_STATUS
EFIAPI
EfiConfigKeywordHandlerSetData (
  IN EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL *This,
  IN CONST EFI_STRING                    KeywordString,
  OUT EFI_STRING                         *Progress,
  OUT UINT32                             *ProgressErr
  )
{
  CHAR8                               *NameSpace;
  EFI_STATUS                          Status;
  CHAR16                              *StringPtr;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  CHAR16                              *NextStringPtr;
  CHAR16                              *KeywordData;
  EFI_STRING_ID                       KeywordStringId;
  UINT32                              RetVal;
  HII_DATABASE_RECORD                 *DataBaseRecord;
  UINT8                               *OpCode;
  CHAR16                              *ConfigResp;
  CHAR16                              *MultiConfigResp;
  CHAR16                              *ValueElement;
  BOOLEAN                             ReadOnly;
  EFI_STRING                          InternalProgress;
  CHAR16                              *TempString;
  CHAR16                              *KeywordStartPos;

  if (This == NULL || Progress == NULL || ProgressErr == NULL || KeywordString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress    = KeywordString;
  *ProgressErr = KEYWORD_HANDLER_UNDEFINED_PROCESSING_ERROR;
  Status       = EFI_SUCCESS;
  MultiConfigResp = NULL;
  NameSpace       = NULL;
  DevicePath      = NULL;
  KeywordData     = NULL;
  ValueElement    = NULL;
  ConfigResp      = NULL;
  KeywordStartPos = NULL;
  KeywordStringId = 0;

  //
  // Use temp string to avoid changing input string buffer.
  //
  TempString = AllocateCopyPool (StrSize (KeywordString), KeywordString);
  ASSERT (TempString != NULL);
  StringPtr = TempString;

  while ((StringPtr != NULL) && (*StringPtr != L'\0')) {
    //
    // 1. Get NameSpace from NameSpaceId keyword.
    //
    Status = ExtractNameSpace (StringPtr, &NameSpace, &NextStringPtr);
    if (EFI_ERROR (Status)) {
      *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
      goto Done;
    }
    ASSERT (NameSpace != NULL);
    //
    // 1.1 Check whether the input namespace is valid.
    //
    if (AsciiStrnCmp(NameSpace, UEFI_CONFIG_LANG, AsciiStrLen (UEFI_CONFIG_LANG)) != 0) {
      *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    StringPtr = NextStringPtr;

    //
    // 2. Get possible Device Path info from KeywordString.
    //
    Status = ExtractDevicePath (StringPtr, (UINT8 **)&DevicePath, &NextStringPtr);
    if (EFI_ERROR (Status)) {
      *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
      goto Done;
    }
    StringPtr = NextStringPtr;

    //
    // 3. Extract keyword from the KeywordRequest string.
    //
    KeywordStartPos = StringPtr;
    Status = ExtractKeyword(StringPtr, &KeywordData, &NextStringPtr);
    if (EFI_ERROR (Status)) {
      //
      // Can't find Keyword base on the input device path info.
      //
      *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
    StringPtr = NextStringPtr;

    //
    // 4. Extract Value from the KeywordRequest string.
    //
    Status = ExtractValue (StringPtr, &ValueElement, &NextStringPtr);
    if (EFI_ERROR (Status)) {
      //
      // Can't find Value base on the input device path info.
      //
      *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
    StringPtr = NextStringPtr;

    //
    // 5. Find READONLY tag.
    //
    if ((StringPtr != NULL) && StrnCmp (StringPtr, L"&READONLY", StrLen (L"&READONLY")) == 0) {
      ReadOnly = TRUE;
      StringPtr += StrLen (L"&READONLY");
    } else {
      ReadOnly = FALSE;
    }

    //
    // 6. Get EFI_STRING_ID for the input keyword.
    //
    Status = GetStringIdFromDatabase (&DevicePath, &NameSpace, KeywordData, &RetVal, &KeywordStringId, &DataBaseRecord);
    if (EFI_ERROR (Status)) {
      *ProgressErr = RetVal;
      goto Done;
    }

    //
    // 7. Construct the ConfigRequest string.
    //
    Status = ExtractConfigResp (DataBaseRecord, KeywordStringId, ValueElement, &OpCode, &ConfigResp);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // 8. Check the readonly flag.
    //
    if (ExtractReadOnlyFromOpCode (OpCode) != ReadOnly) {
      //
      // Extracting readonly flag form opcode and extracting "READONLY" tag form KeywordString should have the same results.
      // If not, the input KeywordString must be incorrect, return the error status to caller.
      //
      *ProgressErr = KEYWORD_HANDLER_INCOMPATIBLE_VALUE_DETECTED;
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
    if (ReadOnly) {
      *ProgressErr = KEYWORD_HANDLER_ACCESS_NOT_PERMITTED;
      Status = EFI_ACCESS_DENIED;
      goto Done;
    }

    //
    // 9. Merge to the MultiKeywordResp string.
    //
    Status = MergeToMultiKeywordResp(&MultiConfigResp, &ConfigResp);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // 10. Clean the temp buffer point.
    //
    FreePool (NameSpace);
    FreePool (DevicePath);
    FreePool (KeywordData);
    FreePool (ValueElement);
    NameSpace = NULL;
    DevicePath = NULL;
    KeywordData = NULL;
    ValueElement = NULL;
    if (ConfigResp != NULL) {
      FreePool (ConfigResp);
      ConfigResp = NULL;
    }
    KeywordStartPos = NULL;
  }

  //
  // 11. Set value to driver.
  //
  Status = mPrivate.ConfigRouting.RouteConfig(
                    &mPrivate.ConfigRouting,
                    (EFI_STRING) MultiConfigResp,
                    &InternalProgress
                    );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  *ProgressErr = KEYWORD_HANDLER_NO_ERROR;

Done:
  if (KeywordStartPos != NULL) {
    *Progress = KeywordString + (KeywordStartPos - TempString);
  } else {
    *Progress = KeywordString + (StringPtr - TempString);
  }

  ASSERT (TempString != NULL);
  FreePool (TempString);
  if (NameSpace != NULL) {
    FreePool (NameSpace);
  }
  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }
  if (KeywordData != NULL) {
    FreePool (KeywordData);
  }
  if (ValueElement != NULL) {
    FreePool (ValueElement);
  }
  if (ConfigResp != NULL) {
    FreePool (ConfigResp);
  }
  if (MultiConfigResp != NULL && MultiConfigResp != ConfigResp) {
    FreePool (MultiConfigResp);
  }

  return Status;
}

/**

  This function accepts a <MultiKeywordRequest> formatted string, finds the underlying
  keyword owners, creates a <MultiConfigRequest> string from it and forwards it to the
  EFI_HII_ROUTING_PROTOCOL.ExtractConfig function.

  If there is an issue in resolving the contents of the KeywordString, then the function
  returns an EFI_INVALID_PARAMETER and also set the Progress and ProgressErr with the
  appropriate information about where the issue occurred and additional data about the
  nature of the issue.

  In the case when KeywordString is NULL, or contains multiple keywords, or when
  EFI_NOT_FOUND is generated while processing the keyword elements, the Results string
  contains values returned for all keywords processed prior to the keyword generating the
  error but no values for the keyword with error or any following keywords.


  @param This           Pointer to the EFI_KEYWORD_HANDLER _PROTOCOL instance.

  @param NameSpaceId    A null-terminated string containing the platform configuration
                        language to search through in the system. If a NULL is passed
                        in, then it is assumed that any platform configuration language
                        with the prefix of "x-UEFI-" are searched.

  @param KeywordString  A null-terminated string in <MultiKeywordRequest> format. If a
                        NULL is passed in the KeywordString field, all of the known
                        keywords in the system for the NameSpaceId specified are
                        returned in the Results field.

  @param Progress       On return, points to a character in the KeywordString. Points
                        to the string's NULL terminator if the request was successful.
                        Points to the most recent '&' before the first failing name / value
                        pair (or the beginning of the string if the failure is in the first
                        name / value pair) if the request was not successful.

  @param ProgressErr    If during the processing of the KeywordString there was a
                        failure, this parameter gives additional information about the
                        possible source of the problem. See the definitions in SetData()
                        for valid value definitions.

  @param Results        A null-terminated string in <MultiKeywordResp> format is returned
                        which has all the values filled in for the keywords in the
                        KeywordString. This is a callee-allocated field, and must be freed
                        by the caller after being used.

  @retval EFI_SUCCESS             The specified action was completed successfully.

  @retval EFI_INVALID_PARAMETER   One or more of the following are TRUE:
                                  1.Progress, ProgressErr, or Results is NULL.
                                  2.Parsing of the KeywordString resulted in an error. See
                                    Progress and ProgressErr for more data.


  @retval EFI_NOT_FOUND           An element of the KeywordString was not found. See
                                  ProgressErr for more data.

  @retval EFI_NOT_FOUND           The NamespaceId specified was not found.  See ProgressErr
                                  for more data.

  @retval EFI_OUT_OF_RESOURCES    Required system resources could not be allocated.  See
                                  ProgressErr for more data.

  @retval EFI_ACCESS_DENIED       The action violated system policy.  See ProgressErr for
                                  more data.

  @retval EFI_DEVICE_ERROR        An unexpected system error occurred.  See ProgressErr
                                  for more data.

**/
EFI_STATUS
EFIAPI
EfiConfigKeywordHandlerGetData (
  IN EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL  *This,
  IN CONST EFI_STRING                     NameSpaceId, OPTIONAL
  IN CONST EFI_STRING                     KeywordString, OPTIONAL
  OUT EFI_STRING                          *Progress,
  OUT UINT32                              *ProgressErr,
  OUT EFI_STRING                          *Results
  )
{
  CHAR8                               *NameSpace;
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  HII_DATABASE_RECORD                 *DataBaseRecord;
  CHAR16                              *StringPtr;
  CHAR16                              *NextStringPtr;
  CHAR16                              *KeywordData;
  EFI_STRING_ID                       KeywordStringId;
  UINT8                               *OpCode;
  CHAR16                              *ConfigRequest;
  CHAR16                              *ValueElement;
  UINT32                              RetVal;
  BOOLEAN                             ReadOnly;
  CHAR16                              *KeywordResp;
  CHAR16                              *MultiKeywordResp;
  CHAR16                              *TempString;

  if (This == NULL || Progress == NULL || ProgressErr == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ProgressErr = KEYWORD_HANDLER_UNDEFINED_PROCESSING_ERROR;
  Status       = EFI_SUCCESS;
  DevicePath   = NULL;
  NameSpace    = NULL;
  KeywordData  = NULL;
  ConfigRequest= NULL;
  StringPtr    = KeywordString;
  ReadOnly     = FALSE;
  MultiKeywordResp = NULL;
  KeywordStringId  = 0;
  TempString   = NULL;

  //
  // Use temp string to avoid changing input string buffer.
  //
  if (NameSpaceId != NULL) {
    TempString = AllocateCopyPool (StrSize (NameSpaceId), NameSpaceId);
    ASSERT (TempString != NULL);
  }
  //
  // 1. Get NameSpace from NameSpaceId keyword.
  //
  Status = ExtractNameSpace (TempString, &NameSpace, NULL);
  if (TempString != NULL) {
    FreePool (TempString);
    TempString = NULL;
  }
  if (EFI_ERROR (Status)) {
    *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
    return Status;
  }
  //
  // 1.1 Check whether the input namespace is valid.
  //
  if (NameSpace != NULL){
    if (AsciiStrnCmp(NameSpace, UEFI_CONFIG_LANG, AsciiStrLen (UEFI_CONFIG_LANG)) != 0) {
      *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
      return EFI_INVALID_PARAMETER;
    }
  }

  if (KeywordString != NULL) {
    //
    // Use temp string to avoid changing input string buffer.
    //
    TempString = AllocateCopyPool (StrSize (KeywordString), KeywordString);
    ASSERT (TempString != NULL);
    StringPtr = TempString;

    while (*StringPtr != L'\0') {
      //
      // 2. Get possible Device Path info from KeywordString.
      //
      Status = ExtractDevicePath (StringPtr, (UINT8 **)&DevicePath, &NextStringPtr);
      if (EFI_ERROR (Status)) {
        *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
        goto Done;
      }
      StringPtr = NextStringPtr;


      //
      // 3. Process Keyword section from the input keywordRequest string.
      //
      // 3.1 Extract keyword from the KeywordRequest string.
      //
      Status = ExtractKeyword(StringPtr, &KeywordData, &NextStringPtr);
      if (EFI_ERROR (Status)) {
        //
        // Can't find Keyword base on the input device path info.
        //
        *ProgressErr = KEYWORD_HANDLER_MALFORMED_STRING;
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }

      //
      // 3.2 Get EFI_STRING_ID for the input keyword.
      //
      Status = GetStringIdFromDatabase (&DevicePath, &NameSpace, KeywordData, &RetVal, &KeywordStringId, &DataBaseRecord);
      if (EFI_ERROR (Status)) {
        *ProgressErr = RetVal;
        goto Done;
      }

      //
      // 3.3 Construct the ConfigRequest string.
      //
      Status = ExtractConfigRequest (DataBaseRecord, KeywordStringId, &OpCode, &ConfigRequest);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // 3.4 Extract Value for the input keyword.
      //
      Status = ExtractValueFromDriver(ConfigRequest, &ValueElement);
      if (EFI_ERROR (Status)) {
        if (Status != EFI_OUT_OF_RESOURCES) {
          Status = EFI_DEVICE_ERROR;
        }
        goto Done;
      }
      StringPtr = NextStringPtr;

      //
      // 4. Process the possible filter section.
      //
      RetVal = ValidateFilter (OpCode, StringPtr, &NextStringPtr, &ReadOnly);
      if (RetVal != KEYWORD_HANDLER_NO_ERROR) {
        *ProgressErr = RetVal;
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      StringPtr = NextStringPtr;


      //
      // 5. Generate KeywordResp string.
      //
      Status = GenerateKeywordResp(NameSpace, DevicePath, KeywordData, ValueElement, ReadOnly, &KeywordResp);
      if (Status != EFI_SUCCESS) {
        goto Done;
      }

      //
      // 6. Merge to the MultiKeywordResp string.
      //
      Status = MergeToMultiKeywordResp(&MultiKeywordResp, &KeywordResp);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // 7. Update return value.
      //
      *Results = MultiKeywordResp;

      //
      // 8. Clean the temp buffer.
      //
      FreePool (DevicePath);
      FreePool (KeywordData);
      FreePool (ValueElement);
      FreePool (ConfigRequest);
      DevicePath = NULL;
      KeywordData = NULL;
      ValueElement = NULL;
      ConfigRequest = NULL;
      if (KeywordResp != NULL) {
        FreePool (KeywordResp);
        KeywordResp = NULL;
      }
    }
  } else {
    //
    // Enumerate all keyword in the system.
    //
    Status = EnumerateAllKeywords(NameSpace, &MultiKeywordResp, ProgressErr);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    *Results = MultiKeywordResp;
  }

  *ProgressErr = KEYWORD_HANDLER_NO_ERROR;

Done:
  *Progress = KeywordString + (StringPtr - TempString);

  if (TempString != NULL) {
    FreePool (TempString);
  }
  if (NameSpace != NULL) {
    FreePool (NameSpace);
  }
  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }
  if (KeywordData != NULL) {
    FreePool (KeywordData);
  }

  return Status;
}
