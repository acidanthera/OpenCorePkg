/** @file
  Copyright (c) 2020-2021, Ubsefor & koralexa. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef ACPI_PARSER_H
#define ACPI_PARSER_H

typedef struct {
  ///
  /// Currently processed opcode in ACPI table.
  ///
  UINT8  *CurrentOpcode;
  ///
  /// Pointer to the end of ACPI table.
  ///
  UINT8  *TableStart;
  ///
  /// Pointer to the end of ACPI table.
  ///
  UINT8  *TableEnd;
  ///
  /// Decoded lookup path allocated from pool.
  /// Contains a sequence of parsed identifiers.
  ///
  UINT32 *PathStart;
  ///
  /// Identifier we need to match next.
  /// Once it reaches PathEnd, matching is successful.
  /// Requested number of matches is required to finish lookup.
  ///
  UINT32 *CurrentIdentifier;
  ///
  /// Pointer to the end of lookup path.
  ///
  UINT32 *PathEnd;
  ///
  /// Nesting level. Once it reaches MAX_NESTING the table is discarded.
  ///
  UINT32 Nesting;
  ///
  /// Number of entries to find. Generally 1 for first match success.
  ///
  UINT32 RequiredEntry;
  ///
  /// Number of entries already found.
  ///
  UINT32 EntriesFound;
} ACPI_PARSER_CONTEXT;


#define IDENT_LEN   4
#define OPCODE_LEN  8
#define MAX_NESTING 1024

/**
  Print new entry name.
**/
#define CONTEXT_ENTER(Context, Name) \
  DEBUG (( \
    DEBUG_VERBOSE, \
    "%a 0x%x (looking for %c%c%c%c)\n", \
    (Name), \
    (UINT32) ((Context)->CurrentOpcode - (Context)->TableStart), \
    ((CHAR8 *) (Context)->CurrentIdentifier)[3], \
    ((CHAR8 *) (Context)->CurrentIdentifier)[2], \
    ((CHAR8 *) (Context)->CurrentIdentifier)[1], \
    ((CHAR8 *) (Context)->CurrentIdentifier)[0] \
    ));

#define PRINT_ACPI_NAME(Str, Name, Length) \
  DEBUG (( \
    DEBUG_VERBOSE, \
    "%a %u (%c%c%c%c)\n", \
    (Str), \
    (Length), \
    (Length) > 0 ? (Name)[((Length) - 1) * 4 + 0] : 'Z', \
    (Length) > 0 ? (Name)[((Length) - 1) * 4 + 1] : 'Z', \
    (Length) > 0 ? (Name)[((Length) - 1) * 4 + 2] : 'Z', \
    (Length) > 0 ? (Name)[((Length) - 1) * 4 + 3] : 'Z' \
    ));

/**
  Check that context is valid and has work to do.
**/
#define CONTEXT_HAS_WORK(Context) do { \
    ASSERT ((Context) != NULL); \
    ASSERT ((Context)->CurrentOpcode != NULL); \
    ASSERT ((Context)->TableEnd != NULL); \
    ASSERT ((Context)->CurrentOpcode < (Context)->TableEnd); /* Must always have work to do. */ \
    ASSERT ((Context)->PathStart != NULL); \
    ASSERT ((Context)->PathEnd != NULL); \
    ASSERT ((Context)->CurrentIdentifier != NULL); \
    ASSERT ((Context)->CurrentIdentifier < (Context)->PathEnd); /* Must always have IDs to parse. */ \
    ASSERT ((Context)->RequiredEntry > 0); \
    ASSERT ((Context)->EntriesFound < (Context)->RequiredEntry); /* Must have entries to find. */ \
  } while (0)

/**
  Enter new nesting level.
**/
#define CONTEXT_INCREASE_NESTING(Context) do { \
    ++(Context)->Nesting; \
    if ((Context)->Nesting > MAX_NESTING) { \
      return EFI_OUT_OF_RESOURCES; \
    } \
  } while (0)

/**
  Exit nesting level.
  Does not need to be called on error-exit.
**/
#define CONTEXT_DECREASE_NESTING(Context) do { \
    ASSERT ((Context)->Nesting > 0); \
    --(Context)->Nesting; \
  } while (0)

/**
  Check the specified amount of bytes exists.
**/
#define CONTEXT_PEEK_BYTES(Context, Bytes) do { \
    if ((UINT32) ((Context)->TableEnd - (Context)->CurrentOpcode) < (UINT32) (Bytes)) { \
      return EFI_DEVICE_ERROR; \
    } \
  } while (0)

/**
  Consume the specified amount of bytes.
**/
#define CONTEXT_CONSUME_BYTES(Context, Bytes) do { \
    if ((UINT32) ((Context)->TableEnd - (Context)->CurrentOpcode) < (UINT32) (Bytes)) { \
      return EFI_DEVICE_ERROR; \
    } \
    (Context)->CurrentOpcode += (Bytes); \
  } while (0)

/**
  Advance opcode to call the new parsing code.
  This one will error as long as there is no new opcode,
  i.e. one byte after the current one.
**/
#define CONTEXT_ADVANCE_OPCODE(Context) do { \
    if ((UINT32) ((Context)->TableEnd - (Context)->CurrentOpcode) <= 1U) { \
      return EFI_DEVICE_ERROR; \
    } \
    ++(Context)->CurrentOpcode; \
  } while (0)

/**
  Determines which object to parse (depending on the opcode)
  and calls the corresponding parser function.

  @param[in, out] Context Structure containing the parser context.
  @param[out]     Result  Pointer to sought opcode if required entry was found.

  @retval EFI_SUCCESS          Required entry was found (search complete).
  @retval EFI_NOT_FOUND        Required entry was not found (more terms need to be parsed).
  @retval EFI_DEVICE_ERROR     Error occured during parsing (must abort).
  @retval EFI_OUT_OF_RESOURCES Nesting limit has been reached (must abort).
 **/
EFI_STATUS
InternalAcpiParseTerm (
  IN OUT ACPI_PARSER_CONTEXT *Context,
     OUT UINT8               **Result
  );

#endif // ACPI_PARSER_H
