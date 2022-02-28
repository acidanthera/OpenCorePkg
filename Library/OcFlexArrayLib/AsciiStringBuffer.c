/** @file
  String buffer.

  Copyright (c) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcFlexArrayLib.h>
#include <Library/OcGuardLib.h>
#include <Library/PrintLib.h>

OC_STRING_BUFFER *
OcAsciiStringBufferInit (
  VOID
  )
{
  OC_STRING_BUFFER  *Buffer;

  Buffer = AllocateZeroPool (sizeof (OC_STRING_BUFFER));

  return Buffer;
}

EFI_STATUS
OcAsciiStringBufferAppend (
  IN OUT  OC_STRING_BUFFER    *Buffer,
  IN      CONST CHAR8         *AppendString    OPTIONAL
  )
{
  return OcAsciiStringBufferAppendN (Buffer, AppendString, MAX_UINTN);
}

/**
  Extend string buffer by a specified length.

  @param[in,out]  Buffer                String buffer to be extended.
  @param[in]      AppendLength          Length to be appended.
  @param[out]     TargetLength          Target length of the extended buffer.

  @retval         EFI_SUCCESS           The buffer was extended successfully.
  @retval         EFI_OUT_OF_RESOURCES  The extension failed due to the lack of resources.
  @retval         EFI_UNSUPPORTED       The buffer has no size.
**/
STATIC
EFI_STATUS
InternalAsciiStringBufferExtendBy (
  IN OUT  OC_STRING_BUFFER    *Buffer,
  IN      CONST UINTN         AppendLength,
     OUT  UINTN               *TargetLength
  )
{
  UINTN       NewSize;

  ASSERT (AppendLength != 0);

  if (Buffer->String == NULL) {
    ASSERT (Buffer->BufferSize == 0);

    Buffer->String = AllocatePool (AppendLength + 1);
    if (Buffer->String == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Buffer->BufferSize  = AppendLength + 1;
    *TargetLength       = AppendLength;
  } else {
    if (Buffer->BufferSize == 0) {
      ASSERT (FALSE);
      return EFI_UNSUPPORTED;
    }

    NewSize = Buffer->BufferSize;
    if (OcOverflowAddUN (Buffer->StringLength, AppendLength, TargetLength)) {
        return EFI_OUT_OF_RESOURCES;
    }
    
    while (NewSize <= *TargetLength) {
      if (OcOverflowMulUN (NewSize, 2, &NewSize)) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    if (NewSize > Buffer->BufferSize) {
      Buffer->String = ReallocatePool (Buffer->BufferSize, NewSize, Buffer->String);
      if (Buffer->String == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      Buffer->BufferSize = NewSize;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
OcAsciiStringBufferAppendN (
  IN OUT  OC_STRING_BUFFER    *Buffer,
  IN      CONST CHAR8         *AppendString,   OPTIONAL
  IN      CONST UINTN         Length
  )
{
  EFI_STATUS  Status;
  UINTN       AppendLength;
  UINTN       TargetLength;

  if (AppendString == NULL) {
    return EFI_SUCCESS;
  }

  AppendLength = AsciiStrnLenS (AppendString, Length);
  
  //
  // Buffer stays NULL if zero appended.
  //
  if (AppendLength == 0) {
    return EFI_SUCCESS;
  }

  Status = InternalAsciiStringBufferExtendBy (Buffer, AppendLength, &TargetLength);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AsciiStrnCpyS (&Buffer->String[Buffer->StringLength], AppendLength + 1, AppendString, AppendLength);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buffer->StringLength  = TargetLength;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OcAsciiStringBufferSPrint (
  IN OUT  OC_STRING_BUFFER    *Buffer,
  IN      CONST CHAR8         *FormatString,
  ...
  )
{
  EFI_STATUS  Status;
  VA_LIST     Marker;
  VA_LIST     Marker2;
  UINTN       NumberOfPrinted;
  UINTN       TargetLength;

  ASSERT (FormatString != NULL);

  VA_START (Marker, FormatString);

  VA_COPY (Marker2, Marker);
  NumberOfPrinted = SPrintLengthAsciiFormat (FormatString, Marker2);
  VA_END (Marker2);

  //
  // Buffer stays NULL if zero appended.
  //
  if (NumberOfPrinted == 0) {
    Status = EFI_SUCCESS;
  } else {
    Status = InternalAsciiStringBufferExtendBy (Buffer, NumberOfPrinted, &TargetLength);
    if (!EFI_ERROR (Status)) {
      AsciiVSPrint (&Buffer->String[Buffer->StringLength], NumberOfPrinted + 1, FormatString, Marker);
      Buffer->StringLength = TargetLength;
    }
  }  

  VA_END (Marker);
  return Status;
}

CHAR8 *
OcAsciiStringBufferFreeContainer (
  IN OUT  OC_STRING_BUFFER    **StringBuffer
  )
{
  CHAR8   *String;

  if (StringBuffer == NULL || *StringBuffer == NULL) {
    ASSERT (FALSE);
    return NULL;
  }

  String = (*StringBuffer)->String;
  FreePool (*StringBuffer);
  *StringBuffer = NULL;

  return String;
}

VOID
OcAsciiStringBufferFree (
  IN OUT  OC_STRING_BUFFER    **StringBuffer
  )
{
  CHAR8 *Result;

  Result = OcAsciiStringBufferFreeContainer (StringBuffer);
  if (Result != NULL) {
    FreePool (Result);
  }
}
