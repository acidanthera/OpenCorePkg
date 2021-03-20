/** @file
  Copyright (c) 2020-2021, Ubsefor & koralexa. All rights reserved.
  Copyright (c) 2021, vit9696. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <IndustryStandard/Acpi62.h>
#include <Library/OcAcpiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <IndustryStandard/AcpiAml.h>

#include "AcpiParser.h"

/**
  Parses identifier or path (several identifiers). Returns info about
  the identifier / path if necessary.

  @param[in, out] Context       Structure containing the parser context.
  @param[in, out] NamePathStart Pointer to first opcode of identifier / path.
  @param[in, out] PathLength    Quantity of identifiers in path.
  @param[in, out] IsRootPath    1 if parsed path was a root path, 0 otherwise.

  @retval EFI_SUCCESS          Name was parsed successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseNameString (
  IN OUT ACPI_PARSER_CONTEXT *Context,
  IN OUT UINT8               **NamePathStart  OPTIONAL,
  IN OUT UINT8               *PathLength      OPTIONAL,
  IN OUT UINT8               *IsRootPath      OPTIONAL
  )
{
  CONTEXT_ENTER (Context, "NameString");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  if (IsRootPath != NULL) {
    *IsRootPath = 0;
  }

  while (Context->CurrentOpcode[0] == AML_ROOT_CHAR || Context->CurrentOpcode[0] == AML_PARENT_PREFIX_CHAR) {
    if (Context->CurrentOpcode[0] == AML_ROOT_CHAR && IsRootPath != NULL) {
      *IsRootPath = 1;
    }

    CONTEXT_ADVANCE_OPCODE (Context);
  }

  switch (Context->CurrentOpcode[0]) {
    case AML_ZERO_OP:
      if (NamePathStart != NULL) {
        *PathLength = 0;
        *NamePathStart = NULL;
      }

      Context->CurrentOpcode += 1;
      break;

    case AML_DUAL_NAME_PREFIX:
      CONTEXT_ADVANCE_OPCODE (Context);
      CONTEXT_PEEK_BYTES (Context, 2 * IDENT_LEN);

      if (NamePathStart != NULL) {
        *PathLength = 2;
        *NamePathStart = Context->CurrentOpcode;
      }

      Context->CurrentOpcode += 2 * IDENT_LEN;
      break;

    case AML_MULTI_NAME_PREFIX:
      CONTEXT_ADVANCE_OPCODE (Context);
      CONTEXT_PEEK_BYTES (Context, 1 + Context->CurrentOpcode[0] * IDENT_LEN);

      if (NamePathStart != NULL) {
        *PathLength = Context->CurrentOpcode[0];
        *NamePathStart = Context->CurrentOpcode + 1;
      }

      Context->CurrentOpcode += 1 + Context->CurrentOpcode[0] * IDENT_LEN;
      break;

    default:
      CONTEXT_PEEK_BYTES (Context, IDENT_LEN);

      if (NamePathStart != NULL) {
        *PathLength = 1;
        *NamePathStart = Context->CurrentOpcode;
      }

      Context->CurrentOpcode += IDENT_LEN;
  }

  if (NamePathStart != NULL) {
    PRINT_ACPI_NAME ("Read name", *NamePathStart, *PathLength);
  }

  CONTEXT_DECREASE_NESTING (Context);
  return EFI_SUCCESS;
}

/**
  Parses package length (length of current section).

  @param[in, out] Context   Structure containing the parser context.
  @param[out]     PkgLength Total length of section.

  @retval EFI_SUCCESS          Length was parsed successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing length.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParsePkgLength (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT32              *PkgLength
  )
{
  UINT8  LeadByte;
  UINT8  ByteCount;
  UINT32 TotalSize;
  UINT32 Index;

  CONTEXT_ENTER (Context, "PkgLength");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  LeadByte = Context->CurrentOpcode[0];
  TotalSize = 0;
  ByteCount = (LeadByte & 0xC0) >> 6;

  CONTEXT_CONSUME_BYTES (Context, 1);

  if (ByteCount > 0) {
    CONTEXT_PEEK_BYTES (Context, ByteCount);

    for (Index = 0; Index < ByteCount; Index++) {
      TotalSize |= (UINT32) Context->CurrentOpcode[Index] << (Index * 8U + 4);
    }

    TotalSize |= LeadByte & 0x0F;
    *PkgLength = TotalSize;
    CONTEXT_CONSUME_BYTES (Context, ByteCount);
  } else {
    *PkgLength = (UINT32) LeadByte;
  }

  CONTEXT_DECREASE_NESTING (Context);
  return EFI_SUCCESS;
}

/**
  Parses alias section to skip it correctly.

  @param[in, out] Context Structure containing the parser context.

  @retval EFI_NOT_FOUND        Alias was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing alias.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseAlias (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  CONTEXT_ENTER (Context, "Alias");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  if (ParseNameString (
    Context,
    NULL,
    NULL,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  CONTEXT_PEEK_BYTES (Context, 1);

  if (ParseNameString (
    Context,
    NULL,
    NULL,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses scope or device section. In case it's name is suitable parses
  elements inside it, or returns pointer to it's opcode if this scope / device
  was sought. Otherwise skips this section.

  @param[in, out] Context Structure containing the parser context.
  @param[out]     Result  Pointer to sought opcode if required entry was found.

  @retval EFI_SUCCESS          Required entry was found.
  @retval EFI_NOT_FOUND        Scope / device was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing scope / device.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseScopeOrDevice (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT8               **Result
  )
{
  UINT32     PkgLength;
  UINT8      *ScopeStart;
  UINT32     *CurrentPath;
  UINT8      *ScopeEnd;
  UINT8      *ScopeName;
  UINT8      *ScopeNameStart;
  UINT8      ScopeNameLength;
  UINT8      IsRootPath;
  EFI_STATUS Status;
  UINT8      Index;
  UINT8      Index2;
  BOOLEAN    Breakout;

  CONTEXT_ENTER (Context, "Scope / Device");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  ScopeStart = Context->CurrentOpcode;
  CurrentPath = Context->CurrentIdentifier;

  if (ParsePkgLength (
    Context,
    &PkgLength
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if ((UINT32) (Context->TableEnd - ScopeStart) < PkgLength) {
    return EFI_DEVICE_ERROR;
  }

  ScopeEnd = ScopeStart + PkgLength;

  CONTEXT_PEEK_BYTES (Context, 1);

  if (ParseNameString (
    Context,
    &ScopeName,
    &ScopeNameLength,
    &IsRootPath
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  ScopeNameStart = ScopeName;

  if (Context->CurrentOpcode > ScopeEnd) {
    return EFI_DEVICE_ERROR;
  }

  if (IsRootPath) {
    Context->CurrentIdentifier = Context->PathStart;
  }

  //
  // Both exit conditions in these loops are for cases when there can be
  // root-relative scopes within the current scope that does not match ours at all.
  //
  Breakout = FALSE;

  for (Index = 0; Index < ScopeNameLength; ++Index) {
    if (Context->CurrentIdentifier == Context->PathEnd) {
      Context->CurrentIdentifier = Context->PathStart;
      break;
    }

    for (Index2 = 0; Index2 < IDENT_LEN; ++Index2) {
      if (*(ScopeName + Index2) != *((UINT8 *)Context->CurrentIdentifier + (IDENT_LEN - Index2 - 1))) {
        Context->CurrentIdentifier = Context->PathStart;
        Breakout = TRUE;
        break;
      }
    }

    if (Breakout) {
      break;
    }

    Context->CurrentIdentifier += 1;
    ScopeName += 4;
  }

  if (Context->CurrentIdentifier == Context->PathEnd) {
    Context->EntriesFound += 1;
    if (Context->EntriesFound == Context->RequiredEntry) {
      *Result = ScopeStart - 1;
      return EFI_SUCCESS;
    }
    //
    // Same issue with root-relative scopes. Retry search.
    //
    Context->CurrentIdentifier = Context->PathStart;
  }

  PRINT_ACPI_NAME ("Entered scope", ScopeNameStart, ScopeNameLength);

  while (Context->CurrentOpcode < ScopeEnd) {
    Status = InternalAcpiParseTerm (
      Context,
      Result
      );

    if (Status != EFI_NOT_FOUND) {
      return Status;
    }
  }

  PRINT_ACPI_NAME ("Left scope", ScopeNameStart, ScopeNameLength);

  Context->CurrentIdentifier = CurrentPath;
  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses name section to correctly skip it.

  @param[in, out] Context Structure containing the parser context.

  @retval EFI_NOT_FOUND        Name was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing name.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseName (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  UINT32 PkgLength;
  UINT8  *CurrentOpcode;

  CONTEXT_ENTER (Context, "Name");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  if (ParseNameString (
    Context,
    NULL,
    NULL,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  CONTEXT_PEEK_BYTES (Context, 1);

  switch (Context->CurrentOpcode[0]) {
    case AML_BYTE_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 2);
      break;

    case AML_WORD_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 3);
      break;

    case AML_DWORD_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 5);
      break;

    case AML_QWORD_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 9);
      break;

    case AML_STRING_PREFIX:
      do {
        CONTEXT_ADVANCE_OPCODE (Context);
      } while (Context->CurrentOpcode[0] >= 0x01 && Context->CurrentOpcode[0] <= 0x7f);

      if (Context->CurrentOpcode[0] != AML_ZERO_OP) {
        return EFI_DEVICE_ERROR;
      }

      CONTEXT_CONSUME_BYTES (Context, 1);
      break;

    case AML_ZERO_OP:
    case AML_ONE_OP:
    case AML_ONES_OP:
      CONTEXT_CONSUME_BYTES (Context, 1);
      break;

    case AML_EXT_OP:
      CONTEXT_CONSUME_BYTES (Context, 2);
      break;

    case AML_BUFFER_OP:
    case AML_PACKAGE_OP:
    case AML_VAR_PACKAGE_OP:
      CONTEXT_ADVANCE_OPCODE (Context);
      CurrentOpcode = Context->CurrentOpcode;

      if (ParsePkgLength (
        Context,
        &PkgLength
        ) != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }

      Context->CurrentOpcode = CurrentOpcode;
      CONTEXT_CONSUME_BYTES (Context, PkgLength);
      break;

    default:
      return EFI_DEVICE_ERROR;
  }

  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses bank field section. In case it's name is suitable returns
  pointer to it's opcode. Otherwise skips this section.

  @param[in, out] Context Structure containing the parser context.
  @param[out]     Result  Pointer to sought opcode if required entry was found.

  @retval EFI_SUCCESS          Required entry was found.
  @retval EFI_NOT_FOUND        Bank field was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing bank field.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseBankField (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT8               **Result
  )
{
  UINT32 PkgLength;
  UINT8  *BankStart;
  UINT8  *BankEnd;
  UINT8  *Name;
  UINT8  NameLength;
  UINT8  Index;

  CONTEXT_ENTER (Context, "BankField");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  BankStart = Context->CurrentOpcode;

  if (ParsePkgLength (
    Context,
    &PkgLength
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if ((UINT32) (Context->TableEnd - BankStart) < PkgLength) {
    return EFI_DEVICE_ERROR;
  }

  BankEnd = BankStart + PkgLength;

  CONTEXT_PEEK_BYTES (Context, 1);

  if (ParseNameString (
    Context,
    &Name,
    &NameLength,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (Context->CurrentOpcode >= BankEnd) {
    return EFI_DEVICE_ERROR;
  }

  if (NameLength != 1) {
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < IDENT_LEN; ++Index) {
    if (*(Name + Index) != *((UINT8 *)Context->CurrentIdentifier + (IDENT_LEN - Index - 1))) {
      Context->CurrentOpcode = BankEnd;
      CONTEXT_DECREASE_NESTING (Context);
      return EFI_NOT_FOUND;
    }
  }

  Context->CurrentIdentifier += 1;

  CONTEXT_PEEK_BYTES (Context, 1);

  if (Context->CurrentIdentifier == Context->PathEnd) {
    //
    // FIXME: This looks weird. If it is a match, should not it return success?
    //
    Context->CurrentIdentifier -= 1;
    if (ParseNameString (
      Context,
      &Name,
      &NameLength,
      NULL
      ) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }
    Context->CurrentOpcode = BankEnd;
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_NOT_FOUND;
  }

  if (ParseNameString (
    Context,
    &Name,
    &NameLength,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (Context->CurrentOpcode > BankEnd) {
    return EFI_DEVICE_ERROR;
  }

  if (NameLength != 1) {
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < IDENT_LEN; ++Index) {
    if (*(Name + Index) != *((UINT8 *)Context->CurrentIdentifier + (IDENT_LEN - Index - 1))) {
      Context->CurrentOpcode = BankEnd;
      Context->CurrentIdentifier -= 1;
      CONTEXT_DECREASE_NESTING (Context);
      return EFI_NOT_FOUND;
    }
  }

  if (Context->CurrentIdentifier + 1 != Context->PathEnd) {
    Context->CurrentOpcode = BankEnd;
    Context->CurrentIdentifier -= 1;
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_NOT_FOUND;
  }

  Context->EntriesFound += 1;

  if (Context->EntriesFound != Context->RequiredEntry) {
    Context->CurrentOpcode = BankEnd;
    Context->CurrentIdentifier -= 1;
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_NOT_FOUND;
  }

  *Result = BankStart - 1;
  return EFI_SUCCESS;
}

/**
  Parses create field section (any of CreateDwordField, CreateWordField,
  CreateByteField, CreateBitField, CreateQWordField, CreateField sections).
  In case it's name is suitable returns pointer to it's opcode.
  Otherwise skips this section.

  @param[in, out] Context Structure containing the parser context.
  @param[out]     Result  Pointer to sought opcode if required entry was found.

  @retval EFI_SUCCESS          Required entry was found.
  @retval EFI_NOT_FOUND        Field was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing field.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseCreateField (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT8               **Result
  )
{
  UINT8    *FieldStart;
  UINT8    *FieldOpcode;
  UINT8    *Name;
  UINT8    NameLength;
  UINT8    Index;
  BOOLEAN  Matched;

  CONTEXT_ENTER (Context, "CreateField");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  FieldStart = Context->CurrentOpcode;
  FieldOpcode = Context->CurrentOpcode - 1;

  switch (Context->CurrentOpcode[0]) {
    case AML_ARG0:
    case AML_ARG1:
    case AML_ARG2:
    case AML_ARG3:
    case AML_ARG4:
    case AML_ARG5:
    case AML_ARG6:
      CONTEXT_CONSUME_BYTES (Context, 5);

      if (*FieldOpcode == AML_EXT_CREATE_FIELD_OP) {
        CONTEXT_CONSUME_BYTES (Context, 4);
      }

      CONTEXT_PEEK_BYTES (Context, 1);

      if (ParseNameString (
        Context,
        NULL,
        NULL,
        NULL
        ) != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }

      CONTEXT_DECREASE_NESTING (Context);
      return EFI_NOT_FOUND;

    default:

      if (ParseNameString (
        Context,
        &Name,
        &NameLength,
        NULL
        ) != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }

      if (NameLength != 1) {
        return EFI_DEVICE_ERROR;
      }

      Matched = TRUE;
      for (Index = 0; Index < IDENT_LEN; Index++) {
        if (*(Name + Index) != *((UINT8 *)Context->CurrentIdentifier + (IDENT_LEN - Index - 1))) {
          Matched = FALSE;
          break;
        }
      }

      CONTEXT_PEEK_BYTES (Context, 1);

      switch (Context->CurrentOpcode[0]) {
        case AML_ZERO_OP:
        case AML_ONE_OP:
          CONTEXT_CONSUME_BYTES (Context, 1);
          break;

        case AML_BYTE_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 2);
          break;

        case AML_WORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 3);
          break;

        case AML_DWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 5);
          break;

        case AML_QWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 9);
          break;
      }

      if (*FieldOpcode == AML_EXT_CREATE_FIELD_OP) {
        CONTEXT_CONSUME_BYTES (Context, 1);
      }

      CONTEXT_PEEK_BYTES (Context, 1);

      if (ParseNameString (
        Context,
        &Name,
        &NameLength,
        NULL
        ) != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }

      if (NameLength != 1) {
        return EFI_DEVICE_ERROR;
      }

      if (!Matched) {
        CONTEXT_DECREASE_NESTING (Context);
        return EFI_NOT_FOUND;
      }

      Context->CurrentIdentifier += 1;

      for (Index = 0; Index < IDENT_LEN; Index++) {
        if (*(Name + Index) != *((UINT8 *)Context->CurrentIdentifier + (IDENT_LEN - Index - 1))) {
          CONTEXT_DECREASE_NESTING (Context);
          Context->CurrentIdentifier--;
          return EFI_NOT_FOUND;
        }
      }

      Context->CurrentIdentifier += 1;

      if (Context->CurrentIdentifier != Context->PathEnd) {
        CONTEXT_DECREASE_NESTING (Context);
        Context->CurrentIdentifier -= 2;
        return EFI_NOT_FOUND;
      }

      Context->EntriesFound += 1;

      if (Context->EntriesFound != Context->RequiredEntry) {
        CONTEXT_DECREASE_NESTING (Context);
        Context->CurrentIdentifier -= 2;
        return EFI_NOT_FOUND;
      }

      *Result = FieldStart - 1;
      return EFI_SUCCESS;
  }
}

/**
  Parses external section to correctly skip it.

  @param[in, out] Context Structure containing the parser context.

  @retval EFI_NOT_FOUND        External was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing external.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseExternal (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  CONTEXT_ENTER (Context, "External");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  if (ParseNameString (
    Context,
    NULL,
    NULL,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  CONTEXT_CONSUME_BYTES (Context, 2);
  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses operation region to skip it correctly.

  @param[in, out] Context Structure containing the parser context.

  @retval EFI_NOT_FOUND        OpRegion was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing OpRegion.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseOpRegion (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  CONTEXT_ENTER (Context, "OpRegion");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  if (ParseNameString (
    Context,
    NULL,
    NULL,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  CONTEXT_ADVANCE_OPCODE (Context);

  switch (Context->CurrentOpcode[0]) {
    case AML_ZERO_OP:
    case AML_ONE_OP:
      CONTEXT_CONSUME_BYTES (Context, 1);
      break;

    case AML_BYTE_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 2);
      break;

    case AML_WORD_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 3);
      break;

    case AML_DWORD_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 5);
      break;

    case AML_QWORD_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 9);
      break;

    case AML_ADD_OP:
    case AML_SUBTRACT_OP:
      CONTEXT_ADVANCE_OPCODE (Context);

      switch (Context->CurrentOpcode[0]) {
        case AML_ZERO_OP:
        case AML_ONE_OP:
          CONTEXT_CONSUME_BYTES (Context, 1);
          break;

        case AML_BYTE_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 2);
          break;

        case AML_WORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 3);
          break;

        case AML_DWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 5);
          break;

        case AML_QWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 9);
          break;

        default:
          if (ParseNameString (
            Context,
            NULL,
            NULL,
            NULL
            ) != EFI_SUCCESS) {
            return EFI_DEVICE_ERROR;
          }
          break;
      }

      CONTEXT_PEEK_BYTES (Context, 1);

      switch (Context->CurrentOpcode[0]) {
        case AML_ZERO_OP:
        case AML_ONE_OP:
          CONTEXT_CONSUME_BYTES (Context, 1);
          break;

        case AML_BYTE_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 2);
          break;

        case AML_WORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 3);
          break;

        case AML_DWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 5);
          break;

        case AML_QWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 9);
          break;

        default:
          if (ParseNameString (
            Context,
            NULL,
            NULL,
            NULL
            ) != EFI_SUCCESS) {
            return EFI_DEVICE_ERROR;
          }
          break;
      }

      CONTEXT_PEEK_BYTES (Context, 1);

      /* Fallthrough */
    default:
      if (ParseNameString (
        Context,
        NULL,
        NULL,
        NULL
        ) != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }
      break;
  }

  CONTEXT_PEEK_BYTES (Context, 1);

  switch (Context->CurrentOpcode[0]) {
    case AML_ZERO_OP:
    case AML_ONE_OP:
      CONTEXT_CONSUME_BYTES (Context, 1);
      break;

    case AML_BYTE_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 2);
      break;

    case AML_WORD_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 3);
      break;

    case AML_DWORD_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 5);
      break;

    case AML_QWORD_PREFIX:
      CONTEXT_CONSUME_BYTES (Context, 9);
      break;

    case AML_ADD_OP:
    case AML_SUBTRACT_OP:
      CONTEXT_ADVANCE_OPCODE (Context);

      switch (Context->CurrentOpcode[0]) {
        case AML_ZERO_OP:
        case AML_ONE_OP:
          CONTEXT_CONSUME_BYTES (Context, 1);
          break;

        case AML_BYTE_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 2);
          break;

        case AML_WORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 3);
          break;

        case AML_DWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 5);
          break;

        case AML_QWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 9);
          break;

        default:
          if (ParseNameString (
            Context,
            NULL,
            NULL,
            NULL
            ) != EFI_SUCCESS) {
            return EFI_DEVICE_ERROR;
          }
          break;
      }

      CONTEXT_PEEK_BYTES (Context, 1);

      switch (Context->CurrentOpcode[0]) {
        case AML_ZERO_OP:
        case AML_ONE_OP:
          CONTEXT_CONSUME_BYTES (Context, 1);
          break;

        case AML_BYTE_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 2);
          break;

        case AML_WORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 3);
          break;

        case AML_DWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 5);
          break;

        case AML_QWORD_PREFIX:
          CONTEXT_CONSUME_BYTES (Context, 9);
          break;

        default:
          if (ParseNameString (
            Context,
            NULL,
            NULL,
            NULL
            ) != EFI_SUCCESS) {
            return EFI_DEVICE_ERROR;
          }
          break;
      }

      CONTEXT_PEEK_BYTES (Context, 1);
      /* Fallthrough */

    default:
      if (ParseNameString (
        Context,
        NULL,
        NULL,
        NULL
        ) != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }
      break;
  }

  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses PowerRes to skip it correctly.

  @param[in, out] Context Structure containing the parser context.

  @retval EFI_NOT_FOUND        PowerRes was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing PowerRes.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParsePowerRes (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  UINT32 PkgLength;
  UINT8  *CurrentOpcode;

  CONTEXT_ENTER (Context, "PowerRes");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  CurrentOpcode = Context->CurrentOpcode;

  if (ParsePkgLength (
    Context,
    &PkgLength
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  Context->CurrentOpcode = CurrentOpcode;
  CONTEXT_CONSUME_BYTES (Context, PkgLength);
  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses processor section to skip it correctly.

  @param[in, out] Context Structure containing the parser context.

  @retval EFI_NOT_FOUND        Processor was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing processor.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseProcessor (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  UINT8 *CurrentOpcode;
  UINT32 PkgLength;

  CONTEXT_ENTER (Context, "Processor");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  CurrentOpcode = Context->CurrentOpcode;

  if (ParsePkgLength (
    Context,
    &PkgLength
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  Context->CurrentOpcode = CurrentOpcode;
  CONTEXT_CONSUME_BYTES (Context, PkgLength);
  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses thermal zone to skip it correctly.

  @param[in, out] Context Structure containing the parser context.

  @retval EFI_NOT_FOUND        Processor was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing processor.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseThermalZone (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  UINT8 *CurrentOpcode;
  UINT32 PkgLength;

  CONTEXT_ENTER (Context, "ThermalZone");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  CurrentOpcode = Context->CurrentOpcode;

  if (ParsePkgLength (
    Context,
    &PkgLength
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  Context->CurrentOpcode = CurrentOpcode;
  CONTEXT_CONSUME_BYTES (Context, PkgLength);
  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses method section.
  In case it's name is suitable returns pointer to it's opcode.
  Otherwise skips this section.

  @param[in, out] Context Structure containing the parser context.
  @param[out]     Result  Pointer to sought opcode if required entry was found.

  @retval EFI_SUCCESS          Required entry was found.
  @retval EFI_NOT_FOUND        Method was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing method.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseMethod (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT8               **Result
  )
{
  UINT8  *MethodStart;
  UINT32 *CurrentPath;
  UINT32 PkgLength;
  UINT8  *MethodEnd;
  UINT8  *MethodName;
  UINT8  MethodNameLength;
  UINT8  Index;
  UINT8  Index2;

  CONTEXT_ENTER (Context, "Method");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  MethodStart = Context->CurrentOpcode;
  CurrentPath = Context->CurrentIdentifier;

  if (ParsePkgLength (
    Context,
    &PkgLength
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if ((UINT32) (Context->TableEnd - MethodStart) < PkgLength) {
    return EFI_DEVICE_ERROR;
  }

  MethodEnd = MethodStart + PkgLength;

  if (Context->CurrentOpcode >= MethodEnd) {
    return EFI_DEVICE_ERROR;
  }

  if (ParseNameString (
    Context,
    &MethodName,
    &MethodNameLength,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (Context->CurrentOpcode > MethodEnd) {
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < MethodNameLength; ++Index) {
    //
    // If the method is within our lookup path but not at it, this is not a match.
    //
    if (Context->CurrentIdentifier == Context->PathEnd) {
      Context->CurrentOpcode = MethodEnd;
      Context->CurrentIdentifier = CurrentPath;
      CONTEXT_DECREASE_NESTING (Context);
      return EFI_NOT_FOUND;
    }

    for (Index2 = 0; Index2 < IDENT_LEN; Index2++) {
      if (*(MethodName + Index2) != *((UINT8 *)Context->CurrentIdentifier + (IDENT_LEN - Index2 - 1))) {
        Context->CurrentOpcode = MethodEnd;
        Context->CurrentIdentifier = CurrentPath;
        CONTEXT_DECREASE_NESTING (Context);
        return EFI_NOT_FOUND;
      }
    }

    Context->CurrentIdentifier += 1;
    MethodName += 4;
  }

  if (Context->CurrentIdentifier != Context->PathEnd) {
    Context->CurrentOpcode = MethodEnd;
    Context->CurrentIdentifier = CurrentPath;
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_NOT_FOUND;
  }

  Context->EntriesFound += 1;

  if (Context->EntriesFound != Context->RequiredEntry) {
    Context->CurrentOpcode = MethodEnd;
    Context->CurrentIdentifier = CurrentPath;
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_NOT_FOUND;
  }

  *Result = MethodStart - 1;
  return EFI_SUCCESS;
}

/**
  Parses if-else section. Looks for other sections inside it.

  @param[in, out] Context Structure containing the parser context.
  @param[out]     Result  Pointer to sought opcode if required entry was found.

  @retval EFI_SUCCESS          Required entry was found.
  @retval EFI_NOT_FOUND        If-else section was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing if-else section.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseIfElse (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT8               **Result
  )
{
  UINT32     PkgLength;
  UINT8      *IfStart;
  UINT32     *CurrentPath;
  UINT8      *IfEnd;
  EFI_STATUS Status;

  CONTEXT_ENTER (Context, "IfElse");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  IfStart = Context->CurrentOpcode;
  CurrentPath = Context->CurrentIdentifier;

  if (ParsePkgLength (
    Context,
    &PkgLength
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if ((UINT32) (Context->TableEnd - IfStart) < PkgLength) {
    return EFI_DEVICE_ERROR;
  }

  IfEnd = IfStart + PkgLength;

  //
  // FIXME: This is broken like hell.
  //
  Status = EFI_NOT_FOUND;
  while (Status != EFI_SUCCESS && Context->CurrentOpcode < IfEnd) {
    Status = InternalAcpiParseTerm (Context, Result);
    if (Status == EFI_DEVICE_ERROR) {
      Context->CurrentOpcode += 1;
    }
  }

  if (Status == EFI_DEVICE_ERROR) {
    return EFI_DEVICE_ERROR;
  }

  if (Status == EFI_SUCCESS) {
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_SUCCESS;
  }

  if (Context->CurrentOpcode > IfEnd) {
    Context->CurrentOpcode = IfEnd;
  }

  Context->CurrentIdentifier = CurrentPath;

  CONTEXT_PEEK_BYTES (Context, 1);

  if (Context->CurrentOpcode[0] == AML_ELSE_OP) {
    CONTEXT_ADVANCE_OPCODE (Context);
    IfStart = Context->CurrentOpcode;

    if (ParsePkgLength (
      Context,
      &PkgLength
      ) != EFI_SUCCESS) {
      return EFI_DEVICE_ERROR;
    }

    if ((UINT32) (Context->TableEnd - IfStart) < PkgLength) {
      return EFI_DEVICE_ERROR;
    }

    IfEnd = IfStart + PkgLength;

    //
    // FIXME: This is broken like hell.
    //
    Status = EFI_NOT_FOUND;
    while (Status != EFI_SUCCESS && Context->CurrentOpcode < IfEnd) {
      Status = InternalAcpiParseTerm (Context, Result);
      if (Status == EFI_DEVICE_ERROR) {
        Context->CurrentOpcode += 1;
      }
    }

    if (Context->CurrentOpcode > IfEnd || Status == EFI_DEVICE_ERROR) {
      return EFI_DEVICE_ERROR;
    }

    if (Status == EFI_SUCCESS) {
      CONTEXT_DECREASE_NESTING (Context);
      return EFI_SUCCESS;
    }

    Context->CurrentIdentifier = CurrentPath;
  }

  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses event section to skip it correctly.

  @param[in, out] Context Structure containing the parser context.

  @retval EFI_NOT_FOUND        Event was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing event.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseEvent (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  CONTEXT_ENTER (Context, "Event");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  if (ParseNameString (
    Context,
    NULL,
    NULL,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses field section.
  In case it's name is suitable returns pointer to it's opcode.
  Otherwise skips this section.

  @param[in, out] Context Structure containing the parser context.
  @param[out]     Result  Pointer to sought opcode if required entry was found.

  @retval EFI_SUCCESS          Required entry was found.
  @retval EFI_NOT_FOUND        Field was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing field.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseField (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT8               **Result
  )
{
  UINT8  *FieldStart;
  UINT32 PkgLength;
  UINT8  *FieldEnd;
  UINT8  *FieldName;
  UINT8  FieldNameLength;
  UINT32 *CurrentPath;
  UINT8  Index;
  UINT8  Index2;

  CONTEXT_ENTER (Context, "Field");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  FieldStart = Context->CurrentOpcode;

  if (ParsePkgLength (
    Context,
    &PkgLength
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if ((UINT32) (Context->TableEnd - FieldStart) < PkgLength) {
    return EFI_DEVICE_ERROR;
  }

  FieldEnd = FieldStart + PkgLength;

  if (Context->CurrentOpcode >= FieldEnd) {
    return EFI_DEVICE_ERROR;
  }

  if (ParseNameString (
    Context,
    &FieldName,
    &FieldNameLength,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (Context->CurrentOpcode > FieldEnd) {
    return EFI_DEVICE_ERROR;
  }

  CurrentPath = Context->CurrentIdentifier;

  for (Index = 0; Index < FieldNameLength; Index++) {
    if (Context->CurrentIdentifier == Context->PathEnd) {
      Context->CurrentOpcode = FieldEnd;
      Context->CurrentIdentifier = CurrentPath;
      CONTEXT_DECREASE_NESTING (Context);
      return EFI_NOT_FOUND;
    }

    for (Index2 = 0; Index2 < IDENT_LEN; Index2++) {
      if (*(FieldName + Index2) != *((UINT8 *)Context->CurrentIdentifier + (IDENT_LEN - Index2 - 1))) {
        Context->CurrentOpcode = FieldEnd;
        Context->CurrentIdentifier = CurrentPath;
        CONTEXT_DECREASE_NESTING (Context);
        return EFI_NOT_FOUND;
      }
    }

    Context->CurrentIdentifier += 1;
    FieldName += 4;
  }

  if (Context->CurrentIdentifier != Context->PathEnd) {
    Context->CurrentOpcode = FieldEnd;
    Context->CurrentIdentifier = CurrentPath;
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_NOT_FOUND;
  }

  Context->EntriesFound += 1;

  if (Context->EntriesFound != Context->RequiredEntry) {
    Context->CurrentOpcode = FieldEnd;
    Context->CurrentIdentifier = CurrentPath;
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_NOT_FOUND;
  }

  *Result = FieldStart - 1;
  return EFI_SUCCESS;
}

/**
  Parses mutex to correctly skip it.

  @param[in, out] Context Structure containing the parser context.

  @retval EFI_NOT_FOUND        Mutex was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing mutex.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseMutex (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  CONTEXT_ENTER (Context, "Mutex");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  if (ParseNameString (
    Context,
    NULL,
    NULL,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  CONTEXT_CONSUME_BYTES (Context, 1);
  CONTEXT_DECREASE_NESTING (Context);
  return EFI_NOT_FOUND;
}

/**
  Parses index field section.
  In case it's name is suitable returns pointer to it's opcode.
  Otherwise skips this section.

  @param[in, out] Context Structure containing the parser context.
  @param[out]     Result  Pointer to sought opcode if required entry was found.

  @retval EFI_SUCCESS          Required entry was found.
  @retval EFI_NOT_FOUND        Index field was parsed and skipped successfuly.
  @retval EFI_DEVICE_ERROR     Error occured during parsing index field.
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached.
**/
STATIC
EFI_STATUS
ParseIndexField (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT8               **Result
  )
{
  UINT8  *FieldStart;
  UINT32 PkgLength;
  UINT8  *FieldEnd;
  UINT8  *FieldName;
  UINT8  FieldNameLength;
  UINT8  Index;

  CONTEXT_ENTER (Context, "IndexField");
  CONTEXT_HAS_WORK (Context);
  CONTEXT_INCREASE_NESTING (Context);

  FieldStart = Context->CurrentOpcode;

  if (ParsePkgLength (
    Context,
    &PkgLength
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if ((UINT32) (Context->TableEnd - FieldStart) < PkgLength) {
    return EFI_DEVICE_ERROR;
  }

  FieldEnd = FieldStart + PkgLength;

  if (Context->CurrentOpcode >= FieldEnd) {
    return EFI_DEVICE_ERROR;
  }

  if (ParseNameString (
    Context,
    &FieldName,
    &FieldNameLength,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (Context->CurrentOpcode >= FieldEnd) {
    return EFI_DEVICE_ERROR;
  }

  if (FieldNameLength != 1) {
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < IDENT_LEN; ++Index) {
    if (*(FieldName + Index) != *((UINT8 *)Context->CurrentIdentifier + (IDENT_LEN - Index - 1))) {
      if (ParseNameString (
        Context,
        &FieldName,
        &FieldNameLength,
        NULL
        ) != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
      }

      Context->CurrentOpcode = FieldEnd;
      CONTEXT_DECREASE_NESTING (Context);
      return EFI_NOT_FOUND;
    }
  }

  Context->CurrentIdentifier += 1;

  //
  // FIXME: Not sure what to do.
  //
  if (Context->CurrentIdentifier == Context->PathEnd) {
    return EFI_DEVICE_ERROR;
  }

  CONTEXT_PEEK_BYTES (Context, 1);

  if (ParseNameString (
    Context,
    &FieldName,
    &FieldNameLength,
    NULL
    ) != EFI_SUCCESS) {
    return EFI_DEVICE_ERROR;
  }

  if (Context->CurrentOpcode >= FieldEnd) {
    return EFI_DEVICE_ERROR;
  }

  if (FieldNameLength != 1) {
    return EFI_DEVICE_ERROR;
  }

  for (Index = 0; Index < IDENT_LEN; Index++) {
    if (*(FieldName + Index) != *((UINT8 *)Context->CurrentIdentifier + (IDENT_LEN - Index - 1))) {
      Context->CurrentOpcode = FieldEnd;
      Context->CurrentIdentifier -= 1;
      CONTEXT_DECREASE_NESTING (Context);
      return EFI_NOT_FOUND;
    }
  }

  if (Context->CurrentIdentifier + 1 != Context->PathEnd) {
    Context->CurrentOpcode = FieldEnd;
    Context->CurrentIdentifier -= 1;
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_NOT_FOUND;
  }

  Context->EntriesFound += 1;

  if (Context->EntriesFound != Context->RequiredEntry) {
    Context->CurrentOpcode = FieldEnd;
    Context->CurrentIdentifier -= 1;
    CONTEXT_DECREASE_NESTING (Context);
    return EFI_NOT_FOUND;
  }

  *Result = FieldStart - 1;
  return EFI_SUCCESS;
}

EFI_STATUS
InternalAcpiParseTerm (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT8               **Result
  )
{
  EFI_STATUS Status;

  CONTEXT_ENTER (Context, "Term");
  CONTEXT_HAS_WORK (Context);
  ASSERT (Result != NULL);
  CONTEXT_INCREASE_NESTING (Context);

  DEBUG ((DEBUG_VERBOSE, "Opcode: %x\n", Context->CurrentOpcode[0]));

  switch (Context->CurrentOpcode[0]) {
    case AML_ALIAS_OP:   // Alias
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseAlias (Context);
      break;

    case AML_SCOPE_OP:   // Scope
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseScopeOrDevice (Context, Result);
      break;

    case AML_NAME_OP:   // Name
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseName (Context);
      break;

    case AML_ZERO_OP:  // ZeroOp
    case AML_EXT_OP:   // ExtOpPrefix
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = EFI_NOT_FOUND;
      break;

    case AML_EXT_BANK_FIELD_OP:   // BankField
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseBankField (Context, Result);
      break;

    case AML_CREATE_DWORD_FIELD_OP: // CreateDwordField
    case AML_CREATE_WORD_FIELD_OP:  // CreateWordField
    case AML_CREATE_BYTE_FIELD_OP:  // CreateByteField
    case AML_CREATE_BIT_FIELD_OP:   // CreateBitField
    case AML_CREATE_QWORD_FIELD_OP: // CreateQWordField
    case AML_EXT_CREATE_FIELD_OP:   // CreateField
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseCreateField (Context, Result);
      break;

    case AML_EXTERNAL_OP:   // External
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseExternal (Context);
      break;

    case AML_EXT_REGION_OP:   // OpRegion
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseOpRegion (Context);
      break;

    case AML_EXT_POWER_RES_OP:   // PowerRes
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParsePowerRes (Context);
      break;

    case AML_EXT_PROCESSOR_OP:   // Processor
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseProcessor (Context);
      break;

    case AML_EXT_THERMAL_ZONE_OP:   // ThermalZone
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseThermalZone (Context);
      break;

    case AML_METHOD_OP:   // Method
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseMethod (Context, Result);
      break;

    case AML_IF_OP:   // IfElse
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseIfElse (Context, Result);
      break;

    case AML_EXT_DEVICE_OP:   // Device
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseScopeOrDevice (Context, Result);
      break;

    case AML_EXT_EVENT_OP:   // Event
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseEvent (Context);
      break;

    case AML_EXT_FIELD_OP:   // Field
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseField (Context, Result);
      break;

    case AML_EXT_INDEX_FIELD_OP:   // IndexField
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseIndexField (Context, Result);
      break;

    case AML_EXT_MUTEX_OP:   // Mutex
      CONTEXT_ADVANCE_OPCODE (Context);
      Status = ParseMutex (Context);
      break;

    default:
      Status = EFI_DEVICE_ERROR;
      break;
  }

  CONTEXT_DECREASE_NESTING (Context);
  return Status;
}

/**
  Translates one identifier to AML opcodes. Complements it with '_'
  opcodes if identifier length is under 4.

  @param[in]  Name       Pointer to the start of the identifier.
  @param[in]  NameEnd    Pointer to the end of the identifier.
  @param[out] OpcodeName Identifier translated to opcodes.

  @retval EFI_SUCCESS           Identifier was translated successfuly.
  @retval EFI_INVALID_PARAMETER Identifier can't be translated to opcodes.
**/
EFI_STATUS
TranslateNameToOpcodes (
  IN     CONST CHAR8 *Name,
  IN     CONST CHAR8 *NameEnd,
     OUT UINT32      *OpcodeName
  )
{
  UINT32 CurrentOpcode;
  UINT8  NameLength;
  UINT8  Index;

  CurrentOpcode = 0;
  NameLength    = (UINT8) (NameEnd - Name);
  *OpcodeName   = 0;

  if (NameLength > IDENT_LEN) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < IDENT_LEN; ++Index) {
    if (Index < NameLength) {
      if ((Name[Index] >= 'a') && (Name[Index] <= 'z')) {
        CurrentOpcode = AML_NAME_CHAR_A + (Name[Index] - 'a');
      } else if ((Name[Index] >= 'A') && (Name[Index] <= 'Z')) {
        CurrentOpcode = AML_NAME_CHAR_A + (Name[Index] - 'A');
      } else if ((Name[Index] >= '0') && (Name[Index] <= '9')) {
          // AML_EXT_REVISION_OP == AML_NAME_INTEGER_0
        CurrentOpcode = AML_EXT_REVISION_OP + (Name[Index] - '0');
      } else if (Name[Index] == '_') {
        CurrentOpcode = AML_NAME_CHAR__;
      } else if (Name[Index] == '\\') {
        *OpcodeName = 0;
        return EFI_INVALID_PARAMETER;
      } else {
        *OpcodeName = 0;
        return EFI_INVALID_PARAMETER;
      }
    } else {
      CurrentOpcode = AML_NAME_CHAR__;
    }

    *OpcodeName = *OpcodeName << OPCODE_LEN;
    *OpcodeName |= CurrentOpcode;
  }

  return EFI_SUCCESS;
}

/**
  Translates path (one or more identifiers) to AML opcodes.

  @param[in, out] Context    Structure containing the parser context.
  @param[in]      PathString Original path.

  @retval EFI_SUCCESS           Path was translated successfuly.
  @retval EFI_INVALID_PARAMETER Path can't be translated to opcodes.
**/
EFI_STATUS
GetOpcodeArray (
  IN OUT ACPI_PARSER_CONTEXT *Context,
  IN     CONST CHAR8         *PathString
  )
{
  EFI_STATUS  Status;
  UINT32      Index;
  UINT32      PathLength;
  CHAR8       *Walker;
  CHAR8       *IdentifierStart;
  CONST CHAR8 *PathStringEnd;
  UINTN       AsciiLength;

  ASSERT (Context != NULL);
  ASSERT (PathString != NULL);

  if (PathString[0] == '\\') {
    PathString += 1;
  }

  AsciiLength = AsciiStrLen (PathString);
  if (AsciiLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  PathLength = 0;
  PathStringEnd = PathString + AsciiLength;

  for (Index = 0; Index < AsciiLength; Index++) {
    if (PathString[Index] == '.') {
      PathLength += 1;
    }
  }

  PathLength += 1;
  Context->PathStart = (UINT32 *) AllocateZeroPool (sizeof (UINT32) * PathLength);
  if (Context->PathStart == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Context->CurrentIdentifier = Context->PathStart;
  Walker                     = (CHAR8 *) PathString;
  IdentifierStart            = (CHAR8 *) PathString;

  for (Index = 0; Index < PathLength; ++Index) {
    while (*Walker != '.' && Walker < PathStringEnd) {
      Walker++;
    }

    if (Walker - IdentifierStart < 1) {
      return EFI_INVALID_PARAMETER;
    }

    Status = TranslateNameToOpcodes (
      IdentifierStart,
      Walker,
      (UINT32 *) (Context->CurrentIdentifier + Index)
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    ++Walker;
    IdentifierStart = Walker;
  }

  Context->PathEnd = Context->CurrentIdentifier + PathLength;
  return EFI_SUCCESS;
}


/**
  Initializes ACPI Context parser variable which stores ACPI header.

  @param[out]  Context  Address to a context structure, containing the parser context.
**/
VOID
InitContext (
  OUT ACPI_PARSER_CONTEXT *Context
  )
{
  ASSERT (Context != NULL);

  ZeroMem (Context, sizeof (*Context));
}

/**
  Deinitialises ACPI Context parser variable which stores ACPI header.

  @param[in,out]  Context  Address to a context structure, containing the parser context.
**/
VOID
ClearContext (
  IN OUT ACPI_PARSER_CONTEXT *Context
  )
{
  if (Context->PathStart != NULL) {
    FreePool (Context->PathStart);
    Context->PathStart = NULL;
  }

  ZeroMem (Context, sizeof (*Context));
}

EFI_STATUS
AcpiFindEntryInMemory (
  IN     UINT8        *Table,
  IN     CONST CHAR8  *PathString,
  IN     UINT8        Entry,
     OUT UINT32       *Offset,
  IN     UINT32       TableLength OPTIONAL
  )
{
  EFI_STATUS           Status;
  UINT8                *Result;
  ACPI_PARSER_CONTEXT  Context;

  ASSERT (Table != NULL);
  ASSERT (PathString != NULL);
  ASSERT (Offset != NULL);

  if (TableLength > 0) {
    if (TableLength < sizeof (EFI_ACPI_COMMON_HEADER)) {
      DEBUG ((DEBUG_VERBOSE, "OCA: Got bad table format which does not specify its length!\n"));
      return EFI_LOAD_ERROR;
    }

    //
    // We do not check length here, mainly because TableLength > 0 is for fuzzing.
    //
  } else {
    TableLength = ((EFI_ACPI_COMMON_HEADER *) Table)->Length;
  }

  if (TableLength <= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
    DEBUG ((DEBUG_VERBOSE, "OCA: Bad or unsupported table header!\n"));
    return EFI_DEVICE_ERROR;
  }

  InitContext (&Context);

  Context.CurrentOpcode = Table;
  Context.RequiredEntry = Entry;
  Context.TableStart    = Table;
  Context.TableEnd      = Table + TableLength;
  Context.CurrentOpcode += sizeof (EFI_ACPI_DESCRIPTION_HEADER);

  Status = GetOpcodeArray (
    &Context,
    PathString
    );

  if (EFI_ERROR (Status)) {
    ClearContext (&Context);
    return Status;
  }

  while (Context.CurrentOpcode < Context.TableEnd) {
    Status = InternalAcpiParseTerm (&Context, &Result);

    if (!EFI_ERROR (Status)) {
      *Offset = (UINT32) (Result - Table);
      ClearContext (&Context);
      return EFI_SUCCESS;
    }

    if (Status != EFI_NOT_FOUND) {
      ClearContext (&Context);
      return Status;
    }
  }

  ClearContext (&Context);
  return EFI_NOT_FOUND;
}
