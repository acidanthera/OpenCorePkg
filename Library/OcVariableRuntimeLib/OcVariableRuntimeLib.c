/** @file
  Save, load and delete emulated NVRAM from file storage.

  Copyright (c) 2019-2022, vit9696, mikebeaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcVariableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/OcVariableRuntime.h>

#define BASE64_CHUNK_SIZE     (52)
#define NVRAM_PLIST_MAX_SIZE  (BASE_1MB)

typedef struct {
  UINT8                     *DataBuffer;
  UINTN                     DataBufferSize;
  CHAR8                     *Base64Buffer;
  UINTN                     Base64BufferSize;
  OC_ASCII_STRING_BUFFER    *StringBuffer;
  GUID                      SectionGuid;
  OC_NVRAM_LEGACY_ENTRY     *SchemaEntry;
  EFI_STATUS                Status;
} NVRAM_SAVE_CONTEXT;

/**
  Version check for NVRAM file. Not the same as protocol revision.
**/
#define OC_NVRAM_STORAGE_VERSION  1

/**
  Structure declaration for NVRAM file.
**/
#define OC_NVRAM_STORAGE_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_ASSOC, _, __)
OC_DECLARE (OC_NVRAM_STORAGE_MAP)

#define OC_NVRAM_STORAGE_FIELDS(_, __) \
  _(UINT32                      , Version  ,     , 0                                       , () ) \
  _(OC_NVRAM_STORAGE_MAP        , Add      ,     , OC_CONSTR (OC_NVRAM_STORAGE_MAP, _, __) , OC_DESTR (OC_NVRAM_STORAGE_MAP))
OC_DECLARE (OC_NVRAM_STORAGE)

OC_MAP_STRUCTORS (OC_NVRAM_STORAGE_MAP)
OC_STRUCTORS (OC_NVRAM_STORAGE, ())

/**
  Schema definition for NVRAM file.
**/
STATIC
OC_SCHEMA
mNvramStorageEntrySchema = OC_SCHEMA_MDATA (NULL);

STATIC
OC_SCHEMA
  mNvramStorageAddSchema = OC_SCHEMA_MAP (NULL, &mNvramStorageEntrySchema);

STATIC
OC_SCHEMA
  mNvramStorageNodesSchema[] = {
  OC_SCHEMA_MAP_IN ("Add",         OC_NVRAM_STORAGE, Add,      &mNvramStorageAddSchema),
  OC_SCHEMA_INTEGER_IN ("Version", OC_NVRAM_STORAGE, Version),
};

STATIC
OC_SCHEMA_INFO
  mNvramStorageRootSchema = {
  .Dict = { mNvramStorageNodesSchema, ARRAY_SIZE (mNvramStorageNodesSchema) }
};

STATIC
OC_STORAGE_CONTEXT
*mStorageContext = NULL;

STATIC
OC_NVRAM_LEGACY_MAP
*mLegacyMap = NULL;

STATIC
EFI_STATUS
LocateNvramDir (
  OUT EFI_FILE_PROTOCOL  **NvramDir
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *Root;

  if ((mStorageContext == NULL) || (mLegacyMap == NULL)) {
    return EFI_NOT_READY;
  }

  if (mStorageContext->FileSystem == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = mStorageContext->FileSystem->OpenVolume (mStorageContext->FileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status = OcSafeFileOpen (
             Root,
             NvramDir,
             OPEN_CORE_NVRAM_ROOT_PATH,
             EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
             EFI_FILE_DIRECTORY
             );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
LoadNvram (
  IN OC_STORAGE_CONTEXT   *StorageContext,
  IN OC_NVRAM_LEGACY_MAP  *LegacyMap,
  IN BOOLEAN              LegacyOverwrite
  )
{
  EFI_STATUS             Status;
  EFI_FILE_PROTOCOL      *NvramDir;
  UINT8                  *FileBuffer;
  UINT32                 FileSize;
  BOOLEAN                IsValid;
  OC_NVRAM_STORAGE       NvramStorage;
  UINT32                 GuidIndex;
  UINT32                 VariableIndex;
  GUID                   VariableGuid;
  OC_ASSOC               *VariableMap;
  OC_NVRAM_LEGACY_ENTRY  *SchemaEntry;

  if ((mStorageContext != NULL) || (mLegacyMap != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  if ((StorageContext == NULL) || (LegacyMap == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  mStorageContext = StorageContext;
  mLegacyMap      = LegacyMap;

  Status = LocateNvramDir (&NvramDir);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FileBuffer = OcReadFileFromDirectory (NvramDir, OPEN_CORE_NVRAM_FILENAME, &FileSize, NVRAM_PLIST_MAX_SIZE);
  if (FileBuffer == NULL) {
    FileBuffer = OcReadFileFromDirectory (NvramDir, OPEN_CORE_NVRAM_FALLBACK_FILENAME, &FileSize, NVRAM_PLIST_MAX_SIZE);
  }

  NvramDir->Close (NvramDir);
  if (FileBuffer == NULL) {
    return EFI_NOT_FOUND;
  }

  OC_NVRAM_STORAGE_CONSTRUCT (&NvramStorage, sizeof (NvramStorage));
  IsValid = ParseSerialized (&NvramStorage, &mNvramStorageRootSchema, FileBuffer, FileSize, NULL);
  FreePool (FileBuffer);

  if (!IsValid) {
    OC_NVRAM_STORAGE_DESTRUCT (&NvramStorage, sizeof (NvramStorage));
    return EFI_UNSUPPORTED;
  }

  if (NvramStorage.Version != OC_NVRAM_STORAGE_VERSION) {
    OC_NVRAM_STORAGE_DESTRUCT (&NvramStorage, sizeof (NvramStorage));
    return EFI_UNSUPPORTED;
  }

  for (GuidIndex = 0; GuidIndex < NvramStorage.Add.Count; ++GuidIndex) {
    Status = OcProcessVariableGuid (
               OC_BLOB_GET (NvramStorage.Add.Keys[GuidIndex]),
               &VariableGuid,
               mLegacyMap,
               &SchemaEntry
               );

    if (EFI_ERROR (Status)) {
      continue;
    }

    VariableMap = NvramStorage.Add.Values[GuidIndex];

    //
    // Note 1: LegacyOverwrite remains useful here, even though we know we are writing to
    // emulated NVRAM which 'starts off empty'; both for any variables set by the emulated
    // NVRAM driver itself, and for those set by any part of OpenDuet when that is in use.
    //
    // Note 2: If we obey WriteFlash here, then when it is TRUE the SaveNvram method fails
    // to save anything to nvram.plist, since everything is marked volatile. As we are in a
    // context where emulated NVRAM must be present, we always write non-volatile here.
    // (This issue was only not relevant prior to implementation of the emulated NVRAM
    // protocol because the previous and current scripts for saving NVRAM variables from
    // within macOS do not check whether the variables they are saving are non-volatile.)
    //
    for (VariableIndex = 0; VariableIndex < VariableMap->Count; ++VariableIndex) {
      OcSetNvramVariable (
        OC_BLOB_GET (VariableMap->Keys[VariableIndex]),
        &VariableGuid,
        OPEN_CORE_NVRAM_NV_ATTR, ///< Was NvramConfig->WriteFlash ? OPEN_CORE_NVRAM_NV_ATTR : OPEN_CORE_NVRAM_ATTR
        VariableMap->Values[VariableIndex]->Size,
        OC_BLOB_GET (VariableMap->Values[VariableIndex]),
        SchemaEntry,
        LegacyOverwrite
        );
    }
  }

  OC_NVRAM_STORAGE_DESTRUCT (&NvramStorage, sizeof (NvramStorage));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
DeleteFile (
  IN EFI_FILE_PROTOCOL  *Directory,
  IN CONST CHAR16       *FileName
  )
{
  EFI_STATUS  Status;

  Status = OcDeleteFile (Directory, FileName);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

//
// Serialize one section at a time, NVRAM scan per section.
//
STATIC
OC_PROCESS_VARIABLE_RESULT
EFIAPI
SerializeSectionVariables (
  IN EFI_GUID  *Guid,
  IN CHAR16    *Name,
  IN VOID      *Context
  )
{
  EFI_STATUS          Status;
  NVRAM_SAVE_CONTEXT  *SaveContext;
  UINT32              Attributes;
  UINTN               DataSize;
  UINTN               Base64Size;
  UINTN               Base64Pos;

  ASSERT (Context != NULL);
  SaveContext = Context;

  if (!CompareGuid (Guid, &SaveContext->SectionGuid)) {
    return OcProcessVariableContinue;
  }

  if (!OcVariableIsAllowedBySchemaEntry (SaveContext->SchemaEntry, Guid, Name, OcStringFormatUnicode)) {
    return OcProcessVariableContinue;
  }

  do {
    DataSize = SaveContext->DataBufferSize;
    Status   = gRT->GetVariable (
                      Name,
                      Guid,
                      &Attributes,
                      &DataSize,
                      SaveContext->DataBuffer
                      );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      while (DataSize > SaveContext->DataBufferSize) {
        if (BaseOverflowMulUN (SaveContext->DataBufferSize, 2, &SaveContext->DataBufferSize)) {
          SaveContext->Status = EFI_OUT_OF_RESOURCES;
          return OcProcessVariableAbort;
        }
      }

      FreePool (SaveContext->DataBuffer);
      SaveContext->DataBuffer = AllocatePool (SaveContext->DataBufferSize);
      if (SaveContext->DataBuffer == NULL) {
        SaveContext->Status = EFI_OUT_OF_RESOURCES;
        return OcProcessVariableAbort;
      }
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  if (EFI_ERROR (Status)) {
    SaveContext->Status = Status;
    return OcProcessVariableAbort;
  }

  //
  // Only save non-volatile variables; also, match launchd script and only save
  // variables which it can save, i.e. runtime accessible.
  //
  if (  ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)
     || ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0))
  {
    DEBUG ((DEBUG_VERBOSE, "NVRAM %g:%s skipped w/ attributes 0x%X\n", Guid, Name, Attributes));
    return OcProcessVariableContinue;
  }

  Base64Size = 0;
  Base64Encode (SaveContext->DataBuffer, DataSize, NULL, &Base64Size);
  if (Base64Size > SaveContext->Base64BufferSize) {
    while (Base64Size > SaveContext->Base64BufferSize) {
      if (BaseOverflowMulUN (SaveContext->Base64BufferSize, 2, &SaveContext->Base64BufferSize)) {
        SaveContext->Status = EFI_OUT_OF_RESOURCES;
        return OcProcessVariableAbort;
      }
    }

    FreePool (SaveContext->Base64Buffer);
    SaveContext->Base64Buffer = AllocatePool (SaveContext->Base64BufferSize);
    if (SaveContext->Base64Buffer == NULL) {
      SaveContext->Status = EFI_OUT_OF_RESOURCES;
      return OcProcessVariableAbort;
    }
  }

  Base64Encode (SaveContext->DataBuffer, DataSize, SaveContext->Base64Buffer, &Base64Size);

  //
  // %c works around BasePrintLibSPrintMarker converting \n to \r\n.
  //
  Status = OcAsciiStringBufferSPrint (
             SaveContext->StringBuffer,
             "\t\t\t<key>%s</key>%c"
             "\t\t\t<data>%c",
             Name,
             '\n',
             '\n'
             );
  if (EFI_ERROR (Status)) {
    SaveContext->Status = Status;
    return OcProcessVariableAbort;
  }

  for (Base64Pos = 0; Base64Pos < (Base64Size - 1); Base64Pos += BASE64_CHUNK_SIZE) {
    Status = OcAsciiStringBufferAppend (
               SaveContext->StringBuffer,
               "\t\t\t"
               );
    if (EFI_ERROR (Status)) {
      SaveContext->Status = Status;
      return OcProcessVariableAbort;
    }

    Status = OcAsciiStringBufferAppendN (
               SaveContext->StringBuffer,
               &SaveContext->Base64Buffer[Base64Pos],
               BASE64_CHUNK_SIZE
               );
    if (EFI_ERROR (Status)) {
      SaveContext->Status = Status;
      return OcProcessVariableAbort;
    }

    Status = OcAsciiStringBufferAppend (
               SaveContext->StringBuffer,
               "\n"
               );
    if (EFI_ERROR (Status)) {
      SaveContext->Status = Status;
      return OcProcessVariableAbort;
    }
  }

  Status = OcAsciiStringBufferAppend (
             SaveContext->StringBuffer,
             "\t\t\t</data>\n"
             );
  if (EFI_ERROR (Status)) {
    SaveContext->Status = Status;
    return OcProcessVariableAbort;
  }

  return OcProcessVariableContinue;
}

STATIC
EFI_STATUS
EFIAPI
SaveNvram (
  VOID
  )
{
  EFI_STATUS          Status;
  EFI_FILE_PROTOCOL   *NvramDir;
  UINT32              GuidIndex;
  NVRAM_SAVE_CONTEXT  Context;

  Status = LocateNvramDir (&NvramDir);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Context.Status = EFI_SUCCESS;

  Context.DataBufferSize = BASE_1KB;
  Context.DataBuffer     = AllocatePool (Context.DataBufferSize);
  if (Context.DataBuffer == NULL) {
    NvramDir->Close (NvramDir);
    return EFI_OUT_OF_RESOURCES;
  }

  Context.Base64BufferSize = BASE_1KB;
  Context.Base64Buffer     = AllocatePool (Context.Base64BufferSize);
  if (Context.Base64Buffer == NULL) {
    NvramDir->Close (NvramDir);
    FreePool (Context.DataBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Context.StringBuffer = OcAsciiStringBufferInit ();
  if (Context.StringBuffer == NULL) {
    NvramDir->Close (NvramDir);
    FreePool (Context.DataBuffer);
    FreePool (Context.Base64Buffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = OcAsciiStringBufferAppend (
             Context.StringBuffer,
             "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
             "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
             "<plist version=\"1.0\">\n"
             "<dict>\n"
             "\t<key>Add</key>\n"
             "\t<dict>\n"
             );

  if (EFI_ERROR (Status)) {
    NvramDir->Close (NvramDir);
    FreePool (Context.DataBuffer);
    FreePool (Context.Base64Buffer);
    OcAsciiStringBufferFree (&Context.StringBuffer);
    return Status;
  }

  for (GuidIndex = 0; GuidIndex < mLegacyMap->Count; ++GuidIndex) {
    Status = OcProcessVariableGuid (
               OC_BLOB_GET (mLegacyMap->Keys[GuidIndex]),
               &Context.SectionGuid,
               mLegacyMap,
               &Context.SchemaEntry
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_SUCCESS;
      continue;
    }

    Status = OcAsciiStringBufferSPrint (
               Context.StringBuffer,
               "\t\t<key>%g</key>%c"
               "\t\t<dict>%c",
               &Context.SectionGuid,
               '\n',
               '\n'
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    OcScanVariables (SerializeSectionVariables, &Context);
    Status = Context.Status;
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = OcAsciiStringBufferAppend (
               Context.StringBuffer,
               "\t\t</dict>\n"
               );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  FreePool (Context.DataBuffer);
  FreePool (Context.Base64Buffer);

  if (!EFI_ERROR (Status)) {
    Status = OcAsciiStringBufferSPrint (
               Context.StringBuffer,
               "\t</dict>%c"
               "\t<key>Version</key>%c"
               "\t<integer>%u</integer>%c"
               "</dict>%c"
               "</plist>%c",
               '\n',
               '\n',
               OC_NVRAM_STORAGE_VERSION,
               '\n',
               '\n',
               '\n'
               );
  }

  if (EFI_ERROR (Status)) {
    NvramDir->Close (NvramDir);
    OcAsciiStringBufferFree (&Context.StringBuffer);
    return Status;
  }

  DeleteFile (NvramDir, OPEN_CORE_NVRAM_FILENAME);
  STATIC_ASSERT (NVRAM_PLIST_MAX_SIZE <= MAX_UINT32, "NVRAM_PLIST_MAX_SIZE must be less than or equal to UINT32_MAX");
  if (Context.StringBuffer->StringLength > NVRAM_PLIST_MAX_SIZE) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    Status = OcSetFileData (
               NvramDir,
               OPEN_CORE_NVRAM_FILENAME,
               Context.StringBuffer->String,
               (UINT32)Context.StringBuffer->StringLength
               );
  }

  OcAsciiStringBufferFree (&Context.StringBuffer);
  NvramDir->Close (NvramDir);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
ResetNvram (
  VOID
  )
{
  EFI_STATUS         Status;
  EFI_STATUS         AltStatus;
  EFI_FILE_PROTOCOL  *NvramDir;

  Status = LocateNvramDir (&NvramDir);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status    = DeleteFile (NvramDir, OPEN_CORE_NVRAM_FILENAME);
  AltStatus = DeleteFile (NvramDir, OPEN_CORE_NVRAM_FALLBACK_FILENAME);

  NvramDir->Close (NvramDir);

  return EFI_ERROR (Status) ? Status : AltStatus;
}

//
// If Luanchd.command is installed this should correctly handle reboots during full or partial OTA updates.
// When installing from USB we will likely go to the wrong OS after first reboot: the one in nvram.fallback,
// if present (which might or might not be the drive we installed to); otherwise the empty NVRAM default OS
// (likewise). Apart from needing to manually select that, at some point, the install should otherwise be correct.
//
STATIC
EFI_STATUS
EFIAPI
SwitchToFallback (
  VOID
  )
{
  EFI_STATUS         Status;
  EFI_FILE_PROTOCOL  *NvramDir;
  UINT8              *FileBuffer;
  UINT32             FileSize;

  Status = LocateNvramDir (&NvramDir);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Given that this approach is designed to avoid continually displaying 'macOS Installer' option,
  // we want to switch back to empty NVRAM defaults even if nvram.fallback does not exist, so we
  // do not check that.
  //
  FileBuffer = OcReadFileFromDirectory (NvramDir, OPEN_CORE_NVRAM_FILENAME, &FileSize, NVRAM_PLIST_MAX_SIZE);
  if (FileBuffer == NULL) {
    NvramDir->Close (NvramDir);
    return EFI_ALREADY_STARTED;
  }

  DeleteFile (NvramDir, OPEN_CORE_NVRAM_USED_FILENAME);
  DeleteFile (NvramDir, OPEN_CORE_NVRAM_FILENAME);
  Status = OcSetFileData (
             NvramDir,
             OPEN_CORE_NVRAM_USED_FILENAME,
             FileBuffer,
             FileSize
             );

  NvramDir->Close (NvramDir);
  FreePool (FileBuffer);

  return Status;
}

STATIC
OC_VARIABLE_RUNTIME_PROTOCOL
  mOcVariableRuntimeProtocol = {
  OC_VARIABLE_RUNTIME_PROTOCOL_REVISION,
  LoadNvram,
  SaveNvram,
  ResetNvram,
  SwitchToFallback
};

EFI_STATUS
EFIAPI
OcVariableRuntimeLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gOcVariableRuntimeProtocolGuid,
                &mOcVariableRuntimeProtocol,
                NULL
                );
}
