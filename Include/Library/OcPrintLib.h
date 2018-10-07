/** @file
  Copyright (C) 2016 - 2018, The HermitCrabs Lab. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _OC_PRINT_LIB_H
#define _OC_PRINT_LIB_H

#define LOG(arg)		OcLog arg

/**
  *
**/

#ifndef __FUNCTION_NAME__
	#ifdef _MSC_VER
		#define __FUNCTION_NAME__   __FUNCTION__
	#else
		#define __FUNCTION_NAME__   __func__
	#endif
#endif

/**
  * Print Primitives
**/

#define LEFT_JUSTIFY              0x01
#define PREFIX_SIGN               0x02
#define PREFIX_BLANK              0x04
#define COMMA_TYPE                0x08
#define LONG_TYPE                 0x10
#define PREFIX_ZERO               0x20
#define RADIX_HEX                 0x80
#define UPPER_CASE_HEX            0x100
#define PAD_TO_WIDTH              0x200
#define PRECISION                 0x800

/**
  * Format Identifiers
**/

#define OUTPUT_ASCII              0x00
#define OUTPUT_UNICODE            0x40
#define OUTPUT_CONSOLE            0x80
#define FORMAT_ASCII              0x000
#define FORMAT_UNICODE            0x100
#define ARGUMENT_ASCII            0x000
#define ARGUMENT_UNICODE          0x400
#define ARGUMENT_REVERSED         0x1000

/**
  * The maximum number of values OcVSPrint will process - 128 because of bin decodes
**/

#define MAXIMUM_VALUE_CHARACTERS  128

/**
  * The number of warning status strings in mOcStatusString
**/

#define WARNING_STATUS_NUMBER     5

/**
  * The number of error status strings in mOcStatusString
**/

#define ERROR_STATUS_NUMBER       33

// OcLog
/// @brief
///
/// @param[in] ErrorLevel			The firmware allocated handle for the EFI image.
/// @param[in] Format				A pointer to the string format list which describes the VA_ARGS list
/// @param[in] ......				VA_ARGS list
///
/// @retval EFI_SUCCESS				The platform detection executed successfully.

VOID
EFIAPI
OcLog (
	IN  UINTN ErrorLevel,
	IN  const CHAR8 *Format,
	...
	)
;

// OcSPrint
/// @brief
///
/// @param[out] StartOfBuffer		A pointer to the character buffer to print the results of the parsing of Format into.
/// @param[in] BufferSize			Maximum number of characters to put into buffer. Zero means no limit.
/// @param[in] Flags				Intial flags value. Can only have FORMAT_UNICODE and OUTPUT_UNICODE set.
/// @param[in] FormatString			A pointer a Null-terminated format string which describes the VA_ARGS list.
/// @param[in] ......				VA_ARGS list
///
/// @retval EFI_SUCCESS				The platform detection executed successfully.

UINTN
EFIAPI
OcSPrint (
	OUT VOID        *StartOfBuffer,
	IN  UINTN        BufferSize,
	IN  UINTN        Flags,
	IN  VOID        *FormatString,
	...
	)
;

// OcVSPrint
/// @brief Worker function that produces a Null-terminated string in an output buffer based on a Null-terminated format string and variable argument list.
///
/// @param[in] Buffer        		A pointer to the character buffer to print the results of the parsing of Format into.
/// @param[in] BufferSize			Maximum number of characters to put into buffer. Zero means no limit.
/// @param[in] Flags				Intial flags value. Can only have FORMAT_UNICODE and OUTPUT_UNICODE set.
/// @param[in] FormatString			A pointer a Null-terminated format string which describes the VA_ARGS list
/// @param[in] ......				VA_ARGS list
///
/// @retval							Number of characters printed not including the Null-terminator.

UINTN
EFIAPI
OcVSPrint (
	OUT CHAR8        *Buffer,
	IN  UINTN        BufferSize,
	IN  UINTN        Flags,
	IN  CONST CHAR8  *Format,
	IN  VA_LIST      Marker
	)
;


// OcGuidToString
/// @brief
///
/// @param[in] Guid        			A pointer to the guid to convert to string.
///
/// @retval							A pointer to the buffer containg the string representation of the guid.

CHAR8 *
EFIAPI
OcGuidToString (
	IN EFI_GUID			*Guid
	)
;

// mOcPrintLibHexStr
/// @brief 
///
extern CONST CHAR8 mOcPrintLibHexStr[];

// mOcStatusString
/// @brief 
///
extern CONST CHAR8 *CONST mOcStatusString[];

#endif /* _OC_PRINT_LIB_H */
