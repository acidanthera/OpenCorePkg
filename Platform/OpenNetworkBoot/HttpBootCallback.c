/** @file
  Callback handler for HTTP Boot.

  Copyright (c) 2024, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "NetworkBootInternal.h"

#include <IndustryStandard/Http11.h>
#include <Library/HttpLib.h>
#include <Protocol/Http.h>

#define HTTP_CONTENT_TYPE_APP_EFI  "application/efi"

OC_DMG_LOADING_SUPPORT  gDmgLoading;

STATIC EFI_HTTP_BOOT_CALLBACK  mOriginalHttpBootCallback;
STATIC BOOLEAN                 mUriValidated;

//
// Abort if we are loading a .dmg and these are banned, or if underlying drivers have
// allowed http:// in URL but user setting for OpenNetworkBoot does not allow it.
// If PcdAllowHttpConnections was not set (via NETWORK_ALLOW_HTTP_CONNECTIONS compilation
// flag) then both HttpDxe and HttpBootDxe will enforce https:// before we get to here.
//
EFI_STATUS
ValidateDmgAndHttps (
  CHAR16   *Uri,
  BOOLEAN  ShowLog,
  BOOLEAN  *HasDmgExtension
  )
{
  CHAR8  *Match;
  CHAR8  *Uri8;
  UINTN  UriSize;

  if (gRequireHttpsUri && !HasHttpsUri (Uri)) {
    //
    // Do not return ACCESS_DENIED as this will attempt to add authentication to the request.
    //
    if (ShowLog) {
      DEBUG ((DEBUG_INFO, "NTBT: Invalid URI https:// is required\n"));
    }

    return EFI_UNSUPPORTED;
  }

  UriSize = StrSize (Uri);
  Uri8    = AllocatePool (UriSize);
  if (Uri8 == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UnicodeStrToAsciiStrS (Uri, Uri8, UriSize);

  Match            = ".dmg";
  *HasDmgExtension = UriFileHasExtension (Uri8, Match);
  if (!*HasDmgExtension) {
    Match            = ".chunklist";
    *HasDmgExtension = UriFileHasExtension (Uri8, Match);
  }

  FreePool (Uri8);

  if (gDmgLoading == OcDmgLoadingDisabled) {
    if (*HasDmgExtension) {
      if (ShowLog) {
        DEBUG ((DEBUG_INFO, "NTBT: %a file is requested while DMG loading is disabled\n", Match));
      }

      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
OpenNetworkBootHttpBootCallback (
  IN EFI_HTTP_BOOT_CALLBACK_PROTOCOL   *This,
  IN EFI_HTTP_BOOT_CALLBACK_DATA_TYPE  DataType,
  IN BOOLEAN                           Received,
  IN UINT32                            DataLength,
  IN VOID                              *Data   OPTIONAL
  )
{
  EFI_STATUS        Status;
  EFI_HTTP_MESSAGE  *HttpMessage;
  EFI_HTTP_HEADER   *Header;
  STATIC BOOLEAN    HasDmgExtension = FALSE;

  switch (DataType) {
    //
    // Abort if http:// is specified but only https:// is allowed.
    // Since the URI can come from DHCP boot options this is important for security.
    // This will fail to be enforced if this callback doesn't get registered, or
    // isn't used by a given implementation of HTTP Boot (it is used by ours, ofc).
    // Therefore if a file is returned from HTTP Boot, we check that this
    // was really hit, and won't load it otherwise. (Which is less ideal than
    // not even fetching the file, as will happen when this callback is hit.)
    //
    // Also abort early if .dmg or .chunklist is found when DmgLoading is disabled.
    // This is a convenience, we could allow these to load and they would be
    // rejected eventually anyway.
    //
    case HttpBootHttpRequest:
      if (Data != NULL) {
        HttpMessage = (EFI_HTTP_MESSAGE *)Data;
        if (HttpMessage->Data.Request->Url != NULL) {
          //
          // Print log messages once on initial access with HTTP HEAD, don't
          // log on subsequent GET which is an attempt to get file size by
          // pre-loading entire file for case of chunked encoding (where file
          // size is not known until it has been transferred).
          //
          Status = ValidateDmgAndHttps (
                     HttpMessage->Data.Request->Url,
                     HttpMessage->Data.Request->Method == HttpMethodHead,
                     &HasDmgExtension
                     );
          if (EFI_ERROR (Status)) {
            return Status;
          }

          mUriValidated = TRUE;
        }
      }

      break;

    //
    // Provide fake MIME type of 'application/efi' for .dmg and .chunklist.
    // This is also a convenience of sorts, in that as long as the user
    // sets 'application/efi' MIME type for these files on their web server,
    // they would work anyway.
    //
    case HttpBootHttpResponse:
      if ((Data != NULL) && HasDmgExtension) {
        //
        // We do not need to keep modifying Content-Type for subsequent packets.
        //
        HasDmgExtension = FALSE;

        HttpMessage = (EFI_HTTP_MESSAGE *)Data;
        Header      = HttpFindHeader (HttpMessage->HeaderCount, HttpMessage->Headers, HTTP_HEADER_CONTENT_TYPE);

        if (Header == NULL) {
          Header = HttpMessage->Headers;
          ++HttpMessage->HeaderCount;
          HttpMessage->Headers = AllocatePool (HttpMessage->HeaderCount * sizeof (HttpMessage->Headers[0]));
          if (HttpMessage->Headers == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }

          CopyMem (HttpMessage->Headers, Header, HttpMessage->HeaderCount * sizeof (HttpMessage->Headers[0]));
          Header             = &HttpMessage->Headers[HttpMessage->HeaderCount - 1];
          Header->FieldValue = NULL;
          Header->FieldName  = AllocateCopyPool (L_STR_SIZE (HTTP_HEADER_CONTENT_TYPE), HTTP_HEADER_CONTENT_TYPE);
          if (Header->FieldName == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }
        } else {
          ASSERT (Header->FieldValue != NULL);
          if (AsciiStrCmp (Header->FieldValue, HTTP_CONTENT_TYPE_APP_EFI) != 0) {
            FreePool (Header->FieldValue);
            Header->FieldValue = NULL;
          }
        }

        if (Header->FieldValue == NULL) {
          Header->FieldValue = AllocateCopyPool (L_STR_SIZE (HTTP_CONTENT_TYPE_APP_EFI), HTTP_CONTENT_TYPE_APP_EFI);
          if (Header->FieldValue == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }
        }
      }

      break;

    default:
      break;
  }

  return mOriginalHttpBootCallback (
           This,
           DataType,
           Received,
           DataLength,
           Data
           );
}

STATIC
VOID
EFIAPI
NotifyInstallHttpBootCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                       Status;
  EFI_HANDLE                       LoadFileHandle;
  EFI_HTTP_BOOT_CALLBACK_PROTOCOL  *HttpBootCallback;

  LoadFileHandle = Context;

  Status = gBS->HandleProtocol (
                  LoadFileHandle,
                  &gEfiHttpBootCallbackProtocolGuid,
                  (VOID **)&HttpBootCallback
                  );

  if (!EFI_ERROR (Status)) {
    //
    // Due to a bug in EDK 2 HttpBootUninstallCallback, they do not uninstall
    // this protocol when they try to. This is okay as long as there is only
    // one consumer of the protocol, because our hooked version stays installed
    // and gets reused (found as the already installed protocol) on second
    // and subsequent tries in HttpBootInstallCallback.
    // REF: https://edk2.groups.io/g/devel/message/117469
    // TODO: Add edk2 bugzilla issue.
    // TODO: To safely allow for more than one consumer while allowing for
    // their bug, we would need to store multiple original callbacks, one
    // per http boot callback protocol address. (Otherwise using consumer
    // A then consumer B, so that we are already installed on both handles,
    // then consumer A again, will use the original callback for B.)
    //
    mOriginalHttpBootCallback  = HttpBootCallback->Callback;
    HttpBootCallback->Callback = OpenNetworkBootHttpBootCallback;
  }
}

BOOLEAN
UriWasValidated (
  VOID
  )
{
  return mUriValidated;
}

EFI_EVENT
MonitorHttpBootCallback (
  EFI_HANDLE  LoadFileHandle
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  VOID        *Registration;

  //
  // Everything in our callback except https validation is convenient but optional.
  // So we can make our driver fully usable with some (hypothetical?) http boot
  // implementation which never hits our callback, as long as we treat the URI as
  // already validated (even if the callback is never hit) when https validation
  // is not turned on.
  //
  mUriValidated = !gRequireHttpsUri;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  NotifyInstallHttpBootCallback,
                  LoadFileHandle,
                  &Event
                  );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiHttpBootCallbackProtocolGuid,
                  Event,
                  &Registration
                  );

  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (Event);
    return NULL;
  }

  return Event;
}
