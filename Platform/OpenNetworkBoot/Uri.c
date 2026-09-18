/** @file
  URI utilities for HTTP Boot.

  Copyright (c) 2024, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "NetworkBootInternal.h"

STATIC EFI_DEVICE_PATH_PROTOCOL  mEndDevicePath = {
  END_DEVICE_PATH_TYPE,
  END_ENTIRE_DEVICE_PATH_SUBTYPE,
  {
    END_DEVICE_PATH_LENGTH,
    0
  }
};

BOOLEAN
HasHttpsUri (
  CHAR16  *Uri
  )
{
  ASSERT (Uri != NULL);

  return (OcStrniCmp (L"https://", Uri, L_STR_LEN (L"https://")) == 0);
}

EFI_DEVICE_PATH_PROTOCOL *
GetUriNode (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Previous;
  EFI_DEVICE_PATH_PROTOCOL  *Node;

  for ( Previous = NULL, Node = DevicePath
        ; !IsDevicePathEnd (Node)
        ; Node = NextDevicePathNode (Node))
  {
    Previous = Node;
  }

  if (  (Previous != NULL)
     && (DevicePathType (Previous) == MESSAGING_DEVICE_PATH)
     && (DevicePathSubType (Previous) == MSG_URI_DP)
        )
  {
    return Previous;
  }

  return NULL;
}

//
// See HttpBootCheckImageType and HttpUrlGetPath within it for how to get
// the proper filename from URL: we would need to use HttpParseUrl, then
// HttpUrlGetPath, then HttpUrlFreeParser.
//
// However, here are some examples:
//
//  - https://myserver.com/images/OpenShell.efi
//  - https://myserver.com/download.php?a=1&b=2&filename=OpenShell.efi
//  - https://myserver.com/images/OpenShell.efi?secure=123
//
// Rather than parse the URL properly, we can handle all of the above
// (one of which would not be handled by proper parsing of the filename
// part, since the filename is in a parameter; even though this is a fudge,
// as the parameter must come last to be fixed up) if we check if url ends
// with the file ext, and replace it if so; otherwise check if the part
// ending at '?' ends with ext and replace it if so.
//
STATIC
EFI_STATUS
ExtractOtherUriFromUri (
  IN  CHAR8    *Uri,
  IN  CHAR8    *FromExt,
  IN  CHAR8    *ToExt,
  OUT CHAR8    **OtherUri,
  IN  BOOLEAN  OnlySearchForFromExt
  )
{
  CHAR8  *SearchUri;
  UINTN  UriLen;
  UINTN  OtherUriLen;
  UINTN  FromExtLen;
  UINTN  ToExtLen;
  INTN   SizeChange;
  CHAR8  *ParamsStart;

  ASSERT (FromExt != NULL);

  if (!OnlySearchForFromExt) {
    ASSERT (ToExt != NULL);
    ASSERT (OtherUri != NULL);
    *OtherUri = NULL;
  }

  UriLen = AsciiStrLen (Uri);
  if (UriLen > MAX_INTN - 1) {
    ///< i.e. UriSize > MAX_INTN
    return EFI_INVALID_PARAMETER;
  }

  FromExtLen = AsciiStrLen (FromExt);
  if (FromExtLen > MAX_INTN - 1) {
    ///< i.e. FromExtSize > MAX_INTN
    return EFI_INVALID_PARAMETER;
  }

  if (UriLen < FromExtLen) {
    return EFI_NOT_FOUND;
  }

  if (OnlySearchForFromExt) {
    SearchUri = Uri;
  } else {
    ToExtLen = AsciiStrLen (ToExt);
    if (ToExtLen > MAX_INTN - 1) {
      ///< i.e. ToExtSize > MAX_INTN
      return EFI_INVALID_PARAMETER;
    }

    if (BaseOverflowSubSN ((INTN)ToExtLen, (INTN)FromExtLen, &SizeChange)) {
      return EFI_INVALID_PARAMETER;
    }

    if (BaseOverflowAddSN ((INTN)UriLen, SizeChange, (INTN *)&OtherUriLen)) {
      return EFI_INVALID_PARAMETER;
    }

    if (OtherUriLen > MAX_INTN - 1) {
      ///< i.e. OtherUriSize > MAX_INTN
      return EFI_INVALID_PARAMETER;
    }

    *OtherUri = AllocatePool (MAX (UriLen, OtherUriLen) + 1);
    if (*OtherUri == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (*OtherUri, Uri, UriLen + 1);
    SearchUri = *OtherUri;
  }

  if (AsciiStrnCmp (&SearchUri[UriLen - FromExtLen], FromExt, FromExtLen) == 0) {
    if (!OnlySearchForFromExt) {
      CopyMem (&SearchUri[UriLen - FromExtLen], ToExt, ToExtLen + 1);
      if (SizeChange < -1) {
        ZeroMem (&SearchUri[UriLen + SizeChange + 1], -(SizeChange + 1));
      }
    }

    return EFI_SUCCESS;
  }

  ParamsStart = AsciiStrStr (SearchUri, "?");
  if (  (ParamsStart != NULL)
     && (AsciiStrnCmp (ParamsStart - FromExtLen, FromExt, FromExtLen) == 0))
  {
    if (!OnlySearchForFromExt) {
      CopyMem (ParamsStart + SizeChange, ParamsStart, &SearchUri[UriLen] - ParamsStart + 1);
      CopyMem (ParamsStart - FromExtLen, ToExt, ToExtLen);
      if (SizeChange < -1) {
        ZeroMem (&SearchUri[UriLen + SizeChange + 1], -(SizeChange + 1));
      }
    }

    return EFI_SUCCESS;
  }

  if (!OnlySearchForFromExt) {
    FreePool (*OtherUri);
    *OtherUri = NULL;
  }

  return EFI_NOT_FOUND;
}

//
// We have the filename in the returned device path, so we can try to load the
// matching file (.chunklist or .dmg, whichever one was not fetched), if we
// we find the other.
//
EFI_STATUS
ExtractOtherUriFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CHAR8                     *FromExt,
  IN  CHAR8                     *ToExt,
  OUT CHAR8                     **OtherUri,
  IN  BOOLEAN                   OnlySearchForFromExt
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Previous;
  CHAR8                     *Uri;

  ASSERT (DevicePath != NULL);

  Previous = GetUriNode (DevicePath);
  if (Previous == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Uri = (CHAR8 *)Previous + sizeof (EFI_DEVICE_PATH_PROTOCOL);

  return ExtractOtherUriFromUri (
           Uri,
           FromExt,
           ToExt,
           OtherUri,
           OnlySearchForFromExt
           );
}

//
// Determine whether file at URI has extension. This isn't only checking
// the file part of the URI, instead it first checks the argument to the last
// param, if there is one. This is a convenience to allow the 'download.php'
// example shown at ExtractOtherUriFromUri.
//
BOOLEAN
UriFileHasExtension (
  IN  CHAR8  *Uri,
  IN  CHAR8  *Ext
  )
{
  return (!EFI_ERROR (ExtractOtherUriFromUri (Uri, Ext, NULL, NULL, TRUE)));
}

EFI_STATUS
HttpBootAddUri (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  VOID                      *Uri,
  OC_STRING_FORMAT          StringFormat,
  EFI_DEVICE_PATH_PROTOCOL  **UriDevicePath
  )
{
  UINTN                     Length;
  UINTN                     UriSize;
  VOID                      *OriginalUri;
  EFI_DEVICE_PATH_PROTOCOL  *Previous;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *TmpDevicePath;

  ASSERT (UriDevicePath != NULL);
  *UriDevicePath = NULL;

  TmpDevicePath = DuplicateDevicePath (DevicePath);
  if (TmpDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // If last non-end node is a URI node, replace it with an end node.
  //
  Previous = GetUriNode (TmpDevicePath);
  if (Previous == NULL) {
    FreePool (TmpDevicePath);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (Previous, &mEndDevicePath, sizeof (mEndDevicePath));

  //
  // Create replacement URI node with the input boot file URI.
  //
  if (StringFormat == OcStringFormatUnicode) {
    OriginalUri = Uri;
    UriSize     = StrSize (Uri);
    Uri         = AllocatePool (UriSize * sizeof (CHAR8));
    if (Uri == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    UnicodeStrToAsciiStrS (OriginalUri, Uri, UriSize);
  } else {
    OriginalUri = NULL;
    UriSize     = AsciiStrSize (Uri);
  }

  Length = sizeof (EFI_DEVICE_PATH_PROTOCOL) + UriSize;
  Node   = AllocatePool (Length);
  if (Node == NULL) {
    FreePool (TmpDevicePath);
    return EFI_OUT_OF_RESOURCES;
  }

  Node->Type    = MESSAGING_DEVICE_PATH;
  Node->SubType = MSG_URI_DP;
  SetDevicePathNodeLength (Node, Length);
  CopyMem ((UINT8 *)Node + sizeof (EFI_DEVICE_PATH_PROTOCOL), Uri, UriSize);
  if (OriginalUri != NULL) {
    FreePool (Uri);
  }

  *UriDevicePath = AppendDevicePathNode (TmpDevicePath, Node);
  FreePool (Node);
  FreePool (TmpDevicePath);
  if (*UriDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}
