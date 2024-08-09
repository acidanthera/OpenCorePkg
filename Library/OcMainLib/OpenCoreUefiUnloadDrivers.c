/** @file
  Unload images by name. Includes image name code from ShellPkg UefiHandleParsingLib.c.

  Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2015-2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2024, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/OcConfigurationLib.h>

#include <PiDxe.h>

#include <Protocol/ComponentName2.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>

typedef
VOID
(*PROCESS_IMAGE)(
  IN VOID          *Context,
  IN CONST CHAR16  *Name,
  IN EFI_HANDLE    Handle
  );

typedef struct {
  CHAR16     *UnloadName;
  BOOLEAN    Unloaded;
} UNLOAD_INFO;

typedef struct {
  UINTN          UnloadNameCount;
  UNLOAD_INFO    *UnloadInfo;
} UNLOAD_IMAGE_CONTEXT;

typedef struct {
  CHAR8    *FileBuffer;
  UINTN    FileBufferSize;
} DRIVER_REPORT_CONTEXT;

/**
  Function to find the file name associated with a LoadedImageProtocol.

  @param[in] LoadedImage     An instance of LoadedImageProtocol.

  @retval                    A string representation of the file name associated
                             with LoadedImage, or NULL if no name can be found.
**/
CHAR16 *
FindLoadedImageFileName (
  IN EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage
  )
{
  EFI_GUID                       *NameGuid;
  EFI_STATUS                     Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv;
  VOID                           *Buffer;
  UINTN                          BufferSize;
  UINT32                         AuthenticationStatus;

  if ((LoadedImage == NULL) || (LoadedImage->FilePath == NULL)) {
    return NULL;
  }

  NameGuid = EfiGetNameGuidFromFwVolDevicePathNode ((MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)LoadedImage->FilePath);

  if (NameGuid == NULL) {
    return NULL;
  }

  //
  // Get the FirmwareVolume2Protocol of the device handle that this image was loaded from.
  //
  Status = gBS->HandleProtocol (LoadedImage->DeviceHandle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **)&Fv);

  //
  // FirmwareVolume2Protocol is PI, and is not required to be available.
  //
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Read the user interface section of the image.
  //
  Buffer = NULL;
  Status = Fv->ReadSection (Fv, NameGuid, EFI_SECTION_USER_INTERFACE, 0, &Buffer, &BufferSize, &AuthenticationStatus);

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // ReadSection returns just the section data, without any section header. For
  // a user interface section, the only data is the file name.
  //
  return Buffer;
}

/**
  Get the best supported language for this driver.

  First base on the user input language  to search, otherwise get the first language
  from the supported language list. The caller needs to free the best language buffer.

  @param[in] SupportedLanguages      The supported languages for this driver.
  @param[in] InputLanguage           The user input language.
  @param[in] Iso639Language          Whether get language for ISO639.

  @return                            The best supported language for this driver.
**/
CHAR8 *
EFIAPI
GetBestLanguageForDriver (
  IN CONST CHAR8  *SupportedLanguages,
  IN CONST CHAR8  *InputLanguage,
  IN BOOLEAN      Iso639Language
  )
{
  CHAR8  *BestLanguage;

  BestLanguage = GetBestLanguage (
                   SupportedLanguages,
                   Iso639Language,
                   (InputLanguage != NULL) ? InputLanguage : "",
                   SupportedLanguages,
                   NULL
                   );

  return BestLanguage;
}

/**
  Function to retrieve the driver name (if possible) from the ComponentName or
  ComponentName2 protocol

  @param[in] TheHandle      The driver handle to get the name of.
  @param[in] Language       The language to use.

  @retval NULL              The name could not be found.
  @return                   A pointer to the string name.  Do not de-allocate the memory.
**/
CONST CHAR16 *
EFIAPI
GetStringNameFromHandle (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST CHAR8       *Language
  )
{
  EFI_COMPONENT_NAME2_PROTOCOL  *CompNameStruct;
  EFI_STATUS                    Status;
  CHAR16                        *RetVal;
  CHAR8                         *BestLang;

  BestLang = NULL;

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **)&CompNameStruct,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    BestLang = GetBestLanguageForDriver (CompNameStruct->SupportedLanguages, Language, FALSE);
    Status   = CompNameStruct->GetDriverName (CompNameStruct, BestLang, &RetVal);
    if (BestLang != NULL) {
      FreePool (BestLang);
      BestLang = NULL;
    }

    if (!EFI_ERROR (Status)) {
      return (RetVal);
    }
  }

  Status = gBS->OpenProtocol (
                  TheHandle,
                  &gEfiComponentNameProtocolGuid,
                  (VOID **)&CompNameStruct,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    BestLang = GetBestLanguageForDriver (CompNameStruct->SupportedLanguages, Language, FALSE);
    Status   = CompNameStruct->GetDriverName (CompNameStruct, BestLang, &RetVal);
    if (BestLang != NULL) {
      FreePool (BestLang);
    }

    if (!EFI_ERROR (Status)) {
      return (RetVal);
    }
  }

  return (NULL);
}

VOID
ReportImageName (
  IN VOID          *Context,
  IN CONST CHAR16  *Name,
  IN EFI_HANDLE    Handle
  )
{
  DRIVER_REPORT_CONTEXT  *ReportContext;

  ReportContext = Context;

  OcAsciiPrintBuffer (
    &ReportContext->FileBuffer,
    &ReportContext->FileBufferSize,
    "%s\n",
    Name
    );
}

VOID
UnloadImageByName (
  IN VOID          *Context,
  IN CONST CHAR16  *Name,
  IN EFI_HANDLE    Handle
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  UNLOAD_IMAGE_CONTEXT  *UnloadContext;

  UnloadContext = Context;

  for (Index = 0; Index < UnloadContext->UnloadNameCount; Index++) {
    if (StrCmp (UnloadContext->UnloadInfo[Index].UnloadName, Name) == 0) {
      Status = gBS->UnloadImage (Handle);
      DEBUG ((
        EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
        "OC: UnloadImage %s - %r\n",
        UnloadContext->UnloadInfo[Index].UnloadName,
        Status
        ));
      UnloadContext->UnloadInfo[Index].Unloaded = TRUE; ///< Do not report as 'not found' even if we failed to unload it.
      break;
    }
  }
}

//
// Perform action for all driver binding protocols which are
// present on the same handle as loaded image protocol and
// where the handle has a usable name.
//
VOID
ProcessAllDrivers (
  IN VOID        *Context,
  PROCESS_IMAGE  ProcessImage
  )
{
  EFI_STATUS                 Status;
  UINTN                      HandleCount;
  EFI_HANDLE                 *HandleBuffer;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  UINTN                      Index;
  CHAR16                     *FileName;
  CONST CHAR16               *Name;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDriverBindingProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    if (!EFI_ERROR (Status)) {
      FileName = NULL;
      Name     = GetStringNameFromHandle (HandleBuffer[Index], NULL); ///< Do not free this one.
      if (Name == NULL) {
        FileName = FindLoadedImageFileName (LoadedImage);
        Name     = FileName;
      }

      if (Name != NULL) {
        ProcessImage (Context, Name, HandleBuffer[Index]);
      }

      if (FileName != NULL) {
        FreePool (FileName);
      }
    }
  }

  return;
}

VOID
OcUnloadDrivers (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINTN                 Index;
  UINTN                 Index2;
  UINTN                 UnloadInfoSize;
  UNLOAD_IMAGE_CONTEXT  UnloadContext;

  if (Config->Uefi.Unload.Count == 0) {
    return;
  }

  if (!BaseOverflowMulUN (
         Config->Uefi.Unload.Count,
         sizeof (*UnloadContext.UnloadInfo),
         &UnloadInfoSize
         ))
  {
    UnloadContext.UnloadInfo = AllocateZeroPool (UnloadInfoSize);
  } else {
    UnloadContext.UnloadInfo = NULL;
  }

  if (UnloadContext.UnloadInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Failed to allocate unload names!\n"));
    return;
  }

  for (Index = 0; Index < Config->Uefi.Unload.Count; ++Index) {
    UnloadContext.UnloadInfo[Index].UnloadName = AsciiStrCopyToUnicode (
                                                   OC_BLOB_GET (
                                                     Config->Uefi.Unload.Values[Index]
                                                     ),
                                                   0
                                                   );
    if (UnloadContext.UnloadInfo[Index].UnloadName == NULL) {
      for (Index2 = 0; Index2 < Index; ++Index2) {
        FreePool (UnloadContext.UnloadInfo[Index2].UnloadName);
      }

      FreePool (UnloadContext.UnloadInfo);
      DEBUG ((DEBUG_ERROR, "OC: Failed to allocate unload names!\n"));
      return;
    }
  }

  UnloadContext.UnloadNameCount = Config->Uefi.Unload.Count;

  ProcessAllDrivers (&UnloadContext, UnloadImageByName);

  for (Index = 0; Index < Config->Uefi.Unload.Count; ++Index) {
    if (!UnloadContext.UnloadInfo[Index].Unloaded) {
      DEBUG ((DEBUG_INFO, "OC: Unload %s - %r\n", UnloadContext.UnloadInfo[Index].UnloadName, EFI_NOT_FOUND));
    }

    FreePool (UnloadContext.UnloadInfo[Index].UnloadName);
  }

  FreePool (UnloadContext.UnloadInfo);

  return;
}

EFI_STATUS
OcDriverInfoDump (
  IN EFI_FILE_PROTOCOL  *Root
  )
{
  EFI_STATUS             Status;
  DRIVER_REPORT_CONTEXT  Context;
  CHAR16                 TmpFileName[32];

  ASSERT (Root != NULL);

  Context.FileBufferSize = SIZE_1KB;
  Context.FileBuffer     = AllocateZeroPool (Context.FileBufferSize);
  if (Context.FileBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ProcessAllDrivers (&Context, ReportImageName);

  //
  // Save dumped driver info to file.
  //
  if (Context.FileBuffer != NULL) {
    UnicodeSPrint (TmpFileName, sizeof (TmpFileName), L"DriverImageNames.txt");
    Status = OcSetFileData (Root, TmpFileName, Context.FileBuffer, (UINT32)AsciiStrLen (Context.FileBuffer));
    DEBUG ((DEBUG_INFO, "OC: Dumped driver info - %r\n", Status));

    FreePool (Context.FileBuffer);
  }

  return EFI_SUCCESS;
}
