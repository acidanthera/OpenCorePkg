/** @file
  The Miscellaneous Routines for TlsAuthConfigDxe driver.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TlsAuthConfigImpl.h"

VOID                *mStartOpCodeHandle = NULL;
VOID                *mEndOpCodeHandle   = NULL;
EFI_IFR_GUID_LABEL  *mStartLabel        = NULL;
EFI_IFR_GUID_LABEL  *mEndLabel          = NULL;

CHAR16  mTlsAuthConfigStorageName[] = L"TLS_AUTH_CONFIG_IFR_NVDATA";

TLS_AUTH_CONFIG_PRIVATE_DATA  *mTlsAuthPrivateData = NULL;

HII_VENDOR_DEVICE_PATH  mTlsAuthConfigHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    TLS_AUTH_CONFIG_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

//
// Possible DER-encoded certificate file suffixes, end with NULL pointer.
//
CHAR16  *mDerPemEncodedSuffix[] = {
  L".cer",
  L".der",
  L".crt",
  L".pem",
  NULL
};

/**
  This code checks if the FileSuffix is one of the possible DER/PEM-encoded certificate suffix.

  @param[in] FileSuffix            The suffix of the input certificate file

  @retval    TRUE           It's a DER/PEM-encoded certificate.
  @retval    FALSE          It's NOT a DER/PEM-encoded certificate.

**/
BOOLEAN
IsDerPemEncodeCertificate (
  IN CONST CHAR16  *FileSuffix
  )
{
  UINTN  Index;

  for (Index = 0; mDerPemEncodedSuffix[Index] != NULL; Index++) {
    if (StrCmp (FileSuffix, mDerPemEncodedSuffix[Index]) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Worker function that prints an EFI_GUID into specified Buffer.

  @param[in]     Guid          Pointer to GUID to print.
  @param[in]     Buffer        Buffer to print Guid into.
  @param[in]     BufferSize    Size of Buffer.

  @retval    Number of characters printed.

**/
UINTN
GuidToString (
  IN  EFI_GUID  *Guid,
  IN  CHAR16    *Buffer,
  IN  UINTN     BufferSize
  )
{
  return UnicodeSPrint (
           Buffer,
           BufferSize,
           L"%g",
           Guid
           );
}

/**
  List all cert in specified database by GUID in the page
  for user to select and delete as needed.

  @param[in]    PrivateData         Module's private data.
  @param[in]    VariableName        The variable name of the vendor's signature database.
  @param[in]    VendorGuid          A unique identifier for the vendor.
  @param[in]    LabelNumber         Label number to insert opcodes.
  @param[in]    FormId              Form ID of current page.
  @param[in]    QuestionIdBase      Base question id of the signature list.

  @retval   EFI_SUCCESS             Success to update the signature list page
  @retval   EFI_OUT_OF_RESOURCES    Unable to allocate required resources.

**/
EFI_STATUS
UpdateDeletePage (
  IN TLS_AUTH_CONFIG_PRIVATE_DATA  *Private,
  IN CHAR16                        *VariableName,
  IN EFI_GUID                      *VendorGuid,
  IN UINT16                        LabelNumber,
  IN EFI_FORM_ID                   FormId,
  IN EFI_QUESTION_ID               QuestionIdBase
  )
{
  EFI_STATUS          Status;
  UINT32              Index;
  UINTN               CertCount;
  UINTN               GuidIndex;
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  EFI_IFR_GUID_LABEL  *EndLabel;
  UINTN               DataSize;
  UINT8               *Data;
  EFI_SIGNATURE_LIST  *CertList;
  EFI_SIGNATURE_DATA  *Cert;
  UINT32              ItemDataSize;
  CHAR16              *GuidStr;
  EFI_STRING_ID       GuidID;
  EFI_STRING_ID       Help;

  Data              = NULL;
  CertList          = NULL;
  Cert              = NULL;
  GuidStr           = NULL;
  StartOpCodeHandle = NULL;
  EndOpCodeHandle   = NULL;

  //
  // Initialize the container for dynamic opcodes.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (StartOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (EndOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Create Hii Extend Label OpCode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                       StartOpCodeHandle,
                                       &gEfiIfrTianoGuid,
                                       NULL,
                                       sizeof (EFI_IFR_GUID_LABEL)
                                       );
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = LabelNumber;

  EndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                     EndOpCodeHandle,
                                     &gEfiIfrTianoGuid,
                                     NULL,
                                     sizeof (EFI_IFR_GUID_LABEL)
                                     );
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = LABEL_END;

  //
  // Read Variable.
  //
  DataSize = 0;
  Status   = gRT->GetVariable (VariableName, VendorGuid, NULL, &DataSize, Data);
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    goto ON_EXIT;
  }

  Data = (UINT8 *)AllocateZeroPool (DataSize);
  if (Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = gRT->GetVariable (VariableName, VendorGuid, NULL, &DataSize, Data);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  GuidStr = AllocateZeroPool (100);
  if (GuidStr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Enumerate all data.
  //
  ItemDataSize = (UINT32)DataSize;
  CertList     = (EFI_SIGNATURE_LIST *)Data;
  GuidIndex    = 0;

  while ((ItemDataSize > 0) && (ItemDataSize >= CertList->SignatureListSize)) {
    if (CompareGuid (&CertList->SignatureType, &gEfiCertX509Guid)) {
      Help = STRING_TOKEN (STR_CERT_TYPE_PCKS_GUID);
    } else {
      //
      // The signature type is not supported in current implementation.
      //
      ItemDataSize -= CertList->SignatureListSize;
      CertList      = (EFI_SIGNATURE_LIST *)((UINT8 *)CertList + CertList->SignatureListSize);
      continue;
    }

    CertCount = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
    for (Index = 0; Index < CertCount; Index++) {
      Cert = (EFI_SIGNATURE_DATA *)((UINT8 *)CertList
                                    + sizeof (EFI_SIGNATURE_LIST)
                                    + CertList->SignatureHeaderSize
                                    + Index * CertList->SignatureSize);
      //
      // Display GUID and help
      //
      GuidToString (&Cert->SignatureOwner, GuidStr, 100);
      GuidID = HiiSetString (Private->RegisteredHandle, 0, GuidStr, NULL);
      HiiCreateCheckBoxOpCode (
        StartOpCodeHandle,
        (EFI_QUESTION_ID)(QuestionIdBase + GuidIndex++),
        0,
        0,
        GuidID,
        Help,
        EFI_IFR_FLAG_CALLBACK,
        0,
        NULL
        );
    }

    ItemDataSize -= CertList->SignatureListSize;
    CertList      = (EFI_SIGNATURE_LIST *)((UINT8 *)CertList + CertList->SignatureListSize);
  }

ON_EXIT:
  HiiUpdateForm (
    Private->RegisteredHandle,
    &gTlsAuthConfigGuid,
    FormId,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  if (StartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (StartOpCodeHandle);
  }

  if (EndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (EndOpCodeHandle);
  }

  if (Data != NULL) {
    FreePool (Data);
  }

  if (GuidStr != NULL) {
    FreePool (GuidStr);
  }

  return EFI_SUCCESS;
}

/**
  Delete one entry from cert database.

  @param[in]    Private             Module's private data.
  @param[in]    VariableName        The variable name of the database.
  @param[in]    VendorGuid          A unique identifier for the vendor.
  @param[in]    LabelNumber         Label number to insert opcodes.
  @param[in]    FormId              Form ID of current page.
  @param[in]    QuestionIdBase      Base question id of the cert list.
  @param[in]    DeleteIndex         Cert index to delete.

  @retval   EFI_SUCCESS             Delete signature successfully.
  @retval   EFI_NOT_FOUND           Can't find the signature item,
  @retval   EFI_OUT_OF_RESOURCES    Could not allocate needed resources.
**/
EFI_STATUS
DeleteCert (
  IN TLS_AUTH_CONFIG_PRIVATE_DATA  *Private,
  IN CHAR16                        *VariableName,
  IN EFI_GUID                      *VendorGuid,
  IN UINT16                        LabelNumber,
  IN EFI_FORM_ID                   FormId,
  IN EFI_QUESTION_ID               QuestionIdBase,
  IN UINTN                         DeleteIndex
  )
{
  EFI_STATUS          Status;
  UINTN               DataSize;
  UINT8               *Data;
  UINT8               *OldData;
  UINT32              Attr;
  UINT32              Index;
  EFI_SIGNATURE_LIST  *CertList;
  EFI_SIGNATURE_LIST  *NewCertList;
  EFI_SIGNATURE_DATA  *Cert;
  UINTN               CertCount;
  UINT32              Offset;
  BOOLEAN             IsItemFound;
  UINT32              ItemDataSize;
  UINTN               GuidIndex;

  Data     = NULL;
  OldData  = NULL;
  CertList = NULL;
  Cert     = NULL;
  Attr     = 0;

  //
  // Get original signature list data.
  //
  DataSize = 0;
  Status   = gRT->GetVariable (VariableName, VendorGuid, NULL, &DataSize, NULL);
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    goto ON_EXIT;
  }

  OldData = (UINT8 *)AllocateZeroPool (DataSize);
  if (OldData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = gRT->GetVariable (VariableName, VendorGuid, &Attr, &DataSize, OldData);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Allocate space for new variable.
  //
  Data = (UINT8 *)AllocateZeroPool (DataSize);
  if (Data == NULL) {
    Status =  EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Enumerate all data and erasing the target item.
  //
  IsItemFound  = FALSE;
  ItemDataSize = (UINT32)DataSize;
  CertList     = (EFI_SIGNATURE_LIST *)OldData;
  Offset       = 0;
  GuidIndex    = 0;
  while ((ItemDataSize > 0) && (ItemDataSize >= CertList->SignatureListSize)) {
    if (CompareGuid (&CertList->SignatureType, &gEfiCertX509Guid)) {
      //
      // Copy EFI_SIGNATURE_LIST header then calculate the signature count in this list.
      //
      CopyMem (Data + Offset, CertList, (sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize));
      NewCertList = (EFI_SIGNATURE_LIST *)(Data + Offset);
      Offset     += (sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
      Cert        = (EFI_SIGNATURE_DATA *)((UINT8 *)CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
      CertCount   = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
      for (Index = 0; Index < CertCount; Index++) {
        if (GuidIndex == DeleteIndex) {
          //
          // Find it! Skip it!
          //
          NewCertList->SignatureListSize -= CertList->SignatureSize;
          IsItemFound                     = TRUE;
        } else {
          //
          // This item doesn't match. Copy it to the Data buffer.
          //
          CopyMem (Data + Offset, (UINT8 *)(Cert), CertList->SignatureSize);
          Offset += CertList->SignatureSize;
        }

        GuidIndex++;
        Cert = (EFI_SIGNATURE_DATA *)((UINT8 *)Cert + CertList->SignatureSize);
      }
    } else {
      //
      // This List doesn't match. Just copy it to the Data buffer.
      //
      CopyMem (Data + Offset, (UINT8 *)(CertList), CertList->SignatureListSize);
      Offset += CertList->SignatureListSize;
    }

    ItemDataSize -= CertList->SignatureListSize;
    CertList      = (EFI_SIGNATURE_LIST *)((UINT8 *)CertList + CertList->SignatureListSize);
  }

  if (!IsItemFound) {
    //
    // Doesn't find the signature Item!
    //
    Status = EFI_NOT_FOUND;
    goto ON_EXIT;
  }

  //
  // Delete the EFI_SIGNATURE_LIST header if there is no signature in the list.
  //
  ItemDataSize = Offset;
  CertList     = (EFI_SIGNATURE_LIST *)Data;
  Offset       = 0;
  ZeroMem (OldData, ItemDataSize);
  while ((ItemDataSize > 0) && (ItemDataSize >= CertList->SignatureListSize)) {
    CertCount = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
    DEBUG ((DEBUG_INFO, "       CertCount = %x\n", CertCount));
    if (CertCount != 0) {
      CopyMem (OldData + Offset, (UINT8 *)(CertList), CertList->SignatureListSize);
      Offset += CertList->SignatureListSize;
    }

    ItemDataSize -= CertList->SignatureListSize;
    CertList      = (EFI_SIGNATURE_LIST *)((UINT8 *)CertList + CertList->SignatureListSize);
  }

  DataSize = Offset;

  Status = gRT->SetVariable (
                  VariableName,
                  VendorGuid,
                  Attr,
                  DataSize,
                  OldData
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to set variable, Status = %r\n", Status));
    goto ON_EXIT;
  }

ON_EXIT:
  if (Data != NULL) {
    FreePool (Data);
  }

  if (OldData != NULL) {
    FreePool (OldData);
  }

  return UpdateDeletePage (
           Private,
           VariableName,
           VendorGuid,
           LabelNumber,
           FormId,
           QuestionIdBase
           );
}

/**
  Clean the file related resource.

  @param[in]    Private             Module's private data.

**/
VOID
CleanFileContext (
  IN TLS_AUTH_CONFIG_PRIVATE_DATA  *Private
  )
{
  if (Private->FileContext->FHandle != NULL) {
    Private->FileContext->FHandle->Close (Private->FileContext->FHandle);
    Private->FileContext->FHandle = NULL;
    if (Private->FileContext->FileName != NULL) {
      FreePool (Private->FileContext->FileName);
      Private->FileContext->FileName = NULL;
    }
  }
}

/**
  Read file content into BufferPtr, the size of the allocate buffer
  is *FileSize plus AddtionAllocateSize.

  @param[in]       FileHandle            The file to be read.
  @param[in, out]  BufferPtr             Pointers to the pointer of allocated buffer.
  @param[out]      FileSize              Size of input file
  @param[in]       AddtionAllocateSize   Addtion size the buffer need to be allocated.
                                         In case the buffer need to contain others besides the file content.

  @retval   EFI_SUCCESS                  The file was read into the buffer.
  @retval   EFI_INVALID_PARAMETER        A parameter was invalid.
  @retval   EFI_OUT_OF_RESOURCES         A memory allocation failed.
  @retval   others                       Unexpected error.

**/
EFI_STATUS
ReadFileContent (
  IN      EFI_FILE_HANDLE  FileHandle,
  IN OUT  VOID             **BufferPtr,
  OUT  UINTN               *FileSize,
  IN      UINTN            AddtionAllocateSize
  )

{
  UINTN       BufferSize;
  UINT64      SourceFileSize;
  VOID        *Buffer;
  EFI_STATUS  Status;

  if ((FileHandle == NULL) || (FileSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Buffer = NULL;

  //
  // Get the file size
  //
  Status = FileHandle->SetPosition (FileHandle, (UINT64)-1);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = FileHandle->GetPosition (FileHandle, &SourceFileSize);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = FileHandle->SetPosition (FileHandle, 0);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  BufferSize = (UINTN)SourceFileSize + AddtionAllocateSize;
  Buffer     =  AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BufferSize = (UINTN)SourceFileSize;
  *FileSize  = BufferSize;

  Status = FileHandle->Read (FileHandle, &BufferSize, Buffer);
  if (EFI_ERROR (Status) || (BufferSize != *FileSize)) {
    FreePool (Buffer);
    Buffer = NULL;
    Status = EFI_BAD_BUFFER_SIZE;
    goto ON_EXIT;
  }

ON_EXIT:

  *BufferPtr = Buffer;
  return Status;
}

/**
  This function converts an input device structure to a Unicode string.

  @param[in] DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
EFIAPI
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
{
  return ConvertDevicePathToText (
           DevPath,
           FALSE,
           TRUE
           );
}

/**
  Extract filename from device path. The returned buffer is allocated using AllocateCopyPool.
  The caller is responsible for freeing the allocated buffer using FreePool(). If return NULL
  means not enough memory resource.

  @param DevicePath       Device path.

  @retval NULL            Not enough memory resource for AllocateCopyPool.
  @retval Other           A new allocated string that represents the file name.

**/
CHAR16 *
ExtractFileNameFromDevicePath (
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CHAR16  *String;
  CHAR16  *MatchString;
  CHAR16  *LastMatch;
  CHAR16  *FileName;
  UINTN   Length;

  ASSERT (DevicePath != NULL);

  String      = DevicePathToStr (DevicePath);
  MatchString = String;
  LastMatch   = String;
  FileName    = NULL;

  while (MatchString != NULL) {
    LastMatch   = MatchString + 1;
    MatchString = StrStr (LastMatch, L"\\");
  }

  Length   = StrLen (LastMatch);
  FileName = AllocateCopyPool ((Length + 1) * sizeof (CHAR16), LastMatch);
  if (FileName != NULL) {
    *(FileName + Length) = 0;
  }

  FreePool (String);

  return FileName;
}

/**
  Enroll a new X509 certificate into Variable.

  @param[in] PrivateData     The module's private data.
  @param[in] VariableName    Variable name of CA database.

  @retval   EFI_SUCCESS            New X509 is enrolled successfully.
  @retval   EFI_OUT_OF_RESOURCES   Could not allocate needed resources.

**/
EFI_STATUS
EnrollX509toVariable (
  IN TLS_AUTH_CONFIG_PRIVATE_DATA  *Private,
  IN CHAR16                        *VariableName
  )
{
  EFI_STATUS          Status;
  UINTN               X509DataSize;
  VOID                *X509Data;
  EFI_SIGNATURE_LIST  *CACert;
  EFI_SIGNATURE_DATA  *CACertData;
  VOID                *Data;
  UINTN               DataSize;
  UINTN               SigDataSize;
  UINT32              Attr;

  X509DataSize = 0;
  SigDataSize  = 0;
  DataSize     = 0;
  X509Data     = NULL;
  CACert       = NULL;
  CACertData   = NULL;
  Data         = NULL;
  Attr         = 0;

  Status = ReadFileContent (
             Private->FileContext->FHandle,
             &X509Data,
             &X509DataSize,
             0
             );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  ASSERT (X509Data != NULL);

  SigDataSize = sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + X509DataSize;

  Data = AllocateZeroPool (SigDataSize);
  if (Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Fill Certificate Database parameters.
  //
  CACert                      = (EFI_SIGNATURE_LIST *)Data;
  CACert->SignatureListSize   = (UINT32)SigDataSize;
  CACert->SignatureHeaderSize = 0;
  CACert->SignatureSize       = (UINT32)(sizeof (EFI_SIGNATURE_DATA) - 1 + X509DataSize);
  CopyGuid (&CACert->SignatureType, &gEfiCertX509Guid);

  CACertData = (EFI_SIGNATURE_DATA *)((UINT8 *)CACert + sizeof (EFI_SIGNATURE_LIST));
  CopyGuid (&CACertData->SignatureOwner, Private->CertGuid);
  CopyMem ((UINT8 *)(CACertData->SignatureData), X509Data, X509DataSize);

  //
  // Check if the signature database entry already exists. If it does, use the
  // EFI_VARIABLE_APPEND_WRITE attribute to append the new signature data to
  // the original variable, plus preserve the original variable attributes.
  //
  Status = gRT->GetVariable (
                  VariableName,
                  &gEfiTlsCaCertificateGuid,
                  &Attr,
                  &DataSize,
                  NULL
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Attr |= EFI_VARIABLE_APPEND_WRITE;
  } else if (Status == EFI_NOT_FOUND) {
    Attr = TLS_AUTH_CONFIG_VAR_BASE_ATTR;
  } else {
    goto ON_EXIT;
  }

  Status = gRT->SetVariable (
                  VariableName,
                  &gEfiTlsCaCertificateGuid,
                  Attr,
                  SigDataSize,
                  Data
                  );
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

ON_EXIT:
  CleanFileContext (Private);

  if (Private->CertGuid != NULL) {
    FreePool (Private->CertGuid);
    Private->CertGuid = NULL;
  }

  if (Data != NULL) {
    FreePool (Data);
  }

  if (X509Data != NULL) {
    FreePool (X509Data);
  }

  return Status;
}

/**
  Enroll Cert into TlsCaCertificate. The GUID will be Private->CertGuid.

  @param[in] PrivateData     The module's private data.
  @param[in] VariableName    Variable name of signature database.

  @retval   EFI_SUCCESS            New Cert enrolled successfully.
  @retval   EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval   EFI_UNSUPPORTED        The Cert file is unsupported type.
  @retval   others                 Fail to enroll Cert data.

**/
EFI_STATUS
EnrollCertDatabase (
  IN TLS_AUTH_CONFIG_PRIVATE_DATA  *Private,
  IN CHAR16                        *VariableName
  )
{
  UINT16  *FilePostFix;
  UINTN   NameLength;

  if ((Private->FileContext->FileName == NULL) || (Private->FileContext->FHandle == NULL) || (Private->CertGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Parse the file's postfix.
  //
  NameLength = StrLen (Private->FileContext->FileName);
  if (NameLength <= 4) {
    return EFI_INVALID_PARAMETER;
  }

  FilePostFix = Private->FileContext->FileName + NameLength - 4;

  if (IsDerPemEncodeCertificate (FilePostFix)) {
    //
    // Supports DER-encoded X509 certificate.
    //
    return EnrollX509toVariable (Private, VariableName);
  }

  return EFI_UNSUPPORTED;
}

/**
  Refresh the global UpdateData structure.

**/
VOID
RefreshUpdateData (
  VOID
  )
{
  //
  // Free current updated date
  //
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  //
  // Create new OpCode Handle
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                        mStartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
}

/**
  Clean up the dynamic opcode at label and form specified by both LabelId.

  @param[in] LabelId         It is both the Form ID and Label ID for opcode deletion.
  @param[in] PrivateData     Module private data.

**/
VOID
CleanUpPage (
  IN UINT16                        LabelId,
  IN TLS_AUTH_CONFIG_PRIVATE_DATA  *PrivateData
  )
{
  RefreshUpdateData ();

  //
  // Remove all op-codes from dynamic page
  //
  mStartLabel->Number = LabelId;
  HiiUpdateForm (
    PrivateData->RegisteredHandle,
    &gTlsAuthConfigGuid,
    LabelId,
    mStartOpCodeHandle, // Label LabelId
    mEndOpCodeHandle    // LABEL_END
    );
}

/**
  Update the form base on the selected file.

  @param FilePath   Point to the file path.
  @param FormId     The form need to display.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
UpdatePage (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_FORM_ID               FormId
  )
{
  CHAR16         *FileName;
  EFI_STRING_ID  StringToken;

  FileName = NULL;

  if (FilePath != NULL) {
    FileName = ExtractFileNameFromDevicePath (FilePath);
  }

  if (FileName == NULL) {
    //
    // FileName = NULL has two case:
    // 1. FilePath == NULL, not select file.
    // 2. FilePath != NULL, but ExtractFileNameFromDevicePath return NULL not enough memory resource.
    // In these two case, no need to update the form, and exit the caller function.
    //
    return TRUE;
  }

  StringToken =  HiiSetString (mTlsAuthPrivateData->RegisteredHandle, 0, FileName, NULL);

  mTlsAuthPrivateData->FileContext->FileName = FileName;

  EfiOpenFileByDevicePath (
    &FilePath,
    &mTlsAuthPrivateData->FileContext->FHandle,
    EFI_FILE_MODE_READ,
    0
    );
  //
  // Create Subtitle op-code for the display string of the option.
  //
  RefreshUpdateData ();
  mStartLabel->Number = FormId;

  HiiCreateSubTitleOpCode (
    mStartOpCodeHandle,
    StringToken,
    0,
    0,
    0
    );

  HiiUpdateForm (
    mTlsAuthPrivateData->RegisteredHandle,
    &gTlsAuthConfigGuid,
    FormId,
    mStartOpCodeHandle, /// Label FormId
    mEndOpCodeHandle    /// LABEL_END
    );

  return TRUE;
}

/**
  Update the form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateCAFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return UpdatePage (FilePath, TLS_AUTH_CONFIG_FORMID4_FORM);
}

/**
  Unload the configuration form, this includes: delete all the configuration
  entries, uninstall the form callback protocol, and free the resources used.

  @param[in]  Private             Pointer to the driver private data.

  @retval EFI_SUCCESS             The configuration form is unloaded.
  @retval Others                  Failed to unload the form.

**/
EFI_STATUS
TlsAuthConfigFormUnload (
  IN TLS_AUTH_CONFIG_PRIVATE_DATA  *Private
  )
{
  if (Private->DriverHandle != NULL) {
    //
    // Uninstall EFI_HII_CONFIG_ACCESS_PROTOCOL
    //
    gBS->UninstallMultipleProtocolInterfaces (
           Private->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mTlsAuthConfigHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &Private->ConfigAccess,
           NULL
           );
    Private->DriverHandle = NULL;
  }

  if (Private->RegisteredHandle != NULL) {
    //
    // Remove HII package list
    //
    HiiRemovePackages (Private->RegisteredHandle);
    Private->RegisteredHandle = NULL;
  }

  if (Private->CertGuid != NULL) {
    FreePool (Private->CertGuid);
  }

  if (Private->FileContext != NULL) {
    FreePool (Private->FileContext);
  }

  FreePool (Private);

  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  if (mEndOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mEndOpCodeHandle);
  }

  return EFI_SUCCESS;
}

/**
  Initialize the configuration form.

  @param[in]  Private             Pointer to the driver private data.

  @retval EFI_SUCCESS             The configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.

**/
EFI_STATUS
TlsAuthConfigFormInit (
  IN TLS_AUTH_CONFIG_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;

  Private->Signature = TLS_AUTH_CONFIG_PRIVATE_DATA_SIGNATURE;

  Private->ConfigAccess.ExtractConfig = TlsAuthConfigAccessExtractConfig;
  Private->ConfigAccess.RouteConfig   = TlsAuthConfigAccessRouteConfig;
  Private->ConfigAccess.Callback      = TlsAuthConfigAccessCallback;

  //
  // Install Device Path Protocol and Config Access protocol to driver handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mTlsAuthConfigHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &Private->ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Publish our HII data.
  //
  Private->RegisteredHandle = HiiAddPackages (
                                &gTlsAuthConfigGuid,
                                Private->DriverHandle,
                                TlsAuthConfigDxeStrings,
                                TlsAuthConfigVfrBin,
                                NULL
                                );
  if (Private->RegisteredHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Private->FileContext = AllocateZeroPool (sizeof (TLS_AUTH_CONFIG_FILE_CONTEXT));
  if (Private->FileContext == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  //
  // Init OpCode Handle and Allocate space for creation of Buffer
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (mStartOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  mEndOpCodeHandle = HiiAllocateOpCodeHandle ();
  if (mEndOpCodeHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                        mStartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  mEndLabel = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (
                                      mEndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  mEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  mEndLabel->Number       = LABEL_END;

  return EFI_SUCCESS;

Error:
  TlsAuthConfigFormUnload (Private);
  return Status;
}

/**

  This function allows the caller to request the current
  configuration for one or more named elements. The resulting
  string is in <ConfigAltResp> format. Any and all alternative
  configuration strings shall also be appended to the end of the
  current configuration string. If they are, they must appear
  after the current configuration. They must contain the same
  routing (GUID, NAME, PATH) as the current configuration string.
  They must have an additional description indicating the type of
  alternative configuration the string represents,
  "ALTCFG=<StringToken>". That <StringToken> (when
  converted from Hex UNICODE to binary) is a reference to a
  string in the associated string pack.

  @param This       Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param Request    A null-terminated Unicode string in
                    <ConfigRequest> format. Note that this
                    includes the routing information as well as
                    the configurable name / value pairs. It is
                    invalid for this string to be in
                    <MultiConfigRequest> format.
                    If a NULL is passed in for the Request field,
                    all of the settings being abstracted by this function
                    will be returned in the Results field.  In addition,
                    if a ConfigHdr is passed in with no request elements,
                    all of the settings being abstracted for that particular
                    ConfigHdr reference will be returned in the Results Field.

  @param Progress   On return, points to a character in the
                    Request string. Points to the string's null
                    terminator if request was successful. Points
                    to the most recent "&" before the first
                    failing name / value pair (or the beginning
                    of the string if the failure is in the first
                    name / value pair) if the request was not
                    successful.

  @param Results    A null-terminated Unicode string in
                    <MultiConfigAltResp> format which has all values
                    filled in for the names in the Request string.
                    String to be allocated by the called function.

  @retval EFI_SUCCESS             The Results string is filled with the
                                  values corresponding to all requested
                                  names.

  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.

  @retval EFI_NOT_FOUND           Routing data doesn't match any
                                  known driver. Progress set to the
                                  first character in the routing header.
                                  Note: There is no requirement that the
                                  driver validate the routing data. It
                                  must skip the <ConfigHdr> in order to
                                  process the names.

  @retval EFI_INVALID_PARAMETER   Illegal syntax. Progress set
                                  to most recent "&" before the
                                  error or the beginning of the
                                  string.

  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question.

**/
EFI_STATUS
EFIAPI
TlsAuthConfigAccessExtractConfig (
  IN CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST  EFI_STRING                      Request,
  OUT       EFI_STRING                      *Progress,
  OUT       EFI_STRING                      *Results
  )
{
  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  UINTN                         Size;
  EFI_STRING                    ConfigRequest;
  EFI_STRING                    ConfigRequestHdr;
  TLS_AUTH_CONFIG_PRIVATE_DATA  *Private;
  BOOLEAN                       AllocatedRequest;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  AllocatedRequest = FALSE;
  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  Size             = 0;

  Private = TLS_AUTH_CONFIG_PRIVATE_FROM_THIS (This);

  BufferSize = sizeof (TLS_AUTH_CONFIG_IFR_NVDATA);
  ZeroMem (&Private->TlsAuthConfigNvData, BufferSize);

  *Progress = Request;

  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &gTlsAuthConfigGuid, mTlsAuthConfigStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request is set to NULL or OFFSET is NULL, construct full request string.
    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gTlsAuthConfigGuid, mTlsAuthConfigStorageName, Private->DriverHandle);
    Size             = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest    = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
    ConfigRequestHdr = NULL;
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *)&Private->TlsAuthConfigNvData,
                                BufferSize,
                                Results,
                                Progress
                                );

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
  }

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**

  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job.

  @param This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param Configuration  A null-terminated Unicode string in
                        <ConfigString> format.

  @param Progress       A pointer to a string filled in with the
                        offset of the most recent '&' before the
                        first failing name / value pair (or the
                        beginning of the string if the failure
                        is in the first name / value pair) or
                        the terminating NULL if all was
                        successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.

  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.

  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.

  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found

**/
EFI_STATUS
EFIAPI
TlsAuthConfigAccessRouteConfig (
  IN CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST  EFI_STRING                      Configuration,
  OUT       EFI_STRING                      *Progress
  )
{
  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  TLS_AUTH_CONFIG_PRIVATE_DATA  *Private;

  if (Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: there is no name for Name/Value storage, only GUID will be checked
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gTlsAuthConfigGuid, mTlsAuthConfigStorageName)) {
    return EFI_NOT_FOUND;
  }

  Private = TLS_AUTH_CONFIG_PRIVATE_FROM_THIS (This);

  BufferSize = sizeof (TLS_AUTH_CONFIG_IFR_NVDATA);
  ZeroMem (&Private->TlsAuthConfigNvData, BufferSize);

  Status = gHiiConfigRouting->ConfigToBlock (
                                gHiiConfigRouting,
                                Configuration,
                                (UINT8 *)&Private->TlsAuthConfigNvData,
                                &BufferSize,
                                Progress
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}

/**

  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect. The format of the data tends to
                                 vary based on the opcode that generated the callback.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.
**/
EFI_STATUS
EFIAPI
TlsAuthConfigAccessCallback (
  IN     CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN     EFI_BROWSER_ACTION                    Action,
  IN     EFI_QUESTION_ID                       QuestionId,
  IN     UINT8                                 Type,
  IN OUT EFI_IFR_TYPE_VALUE                    *Value,
  OUT    EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  EFI_STATUS                    Status;
  RETURN_STATUS                 RStatus;
  TLS_AUTH_CONFIG_PRIVATE_DATA  *Private;
  UINTN                         BufferSize;
  TLS_AUTH_CONFIG_IFR_NVDATA    *IfrNvData;
  UINT16                        LabelId;
  EFI_DEVICE_PATH_PROTOCOL      *File;
  EFI_HII_POPUP_PROTOCOL        *HiiPopUp;
  EFI_HII_POPUP_SELECTION       PopUpSelect;

  Status = EFI_SUCCESS;
  File   = NULL;

  if ((This == NULL) || (Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = TLS_AUTH_CONFIG_PRIVATE_FROM_THIS (This);

  mTlsAuthPrivateData = Private;
  Status              = gBS->LocateProtocol (&gEfiHiiPopupProtocolGuid, NULL, (VOID **)&HiiPopUp);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Can't find Form PopUp protocol. Exit (%r)\n", Status));
    return Status;
  }

  //
  // Retrieve uncommitted data from Browser
  //
  BufferSize = sizeof (TLS_AUTH_CONFIG_IFR_NVDATA);
  IfrNvData  = AllocateZeroPool (BufferSize);
  if (IfrNvData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HiiGetBrowserData (&gTlsAuthConfigGuid, mTlsAuthConfigStorageName, BufferSize, (UINT8 *)IfrNvData);

  if ((Action != EFI_BROWSER_ACTION_CHANGED) &&
      (Action != EFI_BROWSER_ACTION_CHANGING) &&
      (Action != EFI_BROWSER_ACTION_FORM_CLOSE))
  {
    Status = EFI_UNSUPPORTED;
    goto EXIT;
  }

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (QuestionId) {
      case KEY_TLS_AUTH_CONFIG_CLIENT_CERT:
      case KEY_TLS_AUTH_CONFIG_SERVER_CA:
        //
        // Clear Cert GUID.
        //
        ZeroMem (IfrNvData->CertGuid, sizeof (IfrNvData->CertGuid));
        if (Private->CertGuid == NULL) {
          Private->CertGuid = (EFI_GUID *)AllocateZeroPool (sizeof (EFI_GUID));
          if (Private->CertGuid == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }
        }

        if (QuestionId == KEY_TLS_AUTH_CONFIG_CLIENT_CERT) {
          LabelId = TLS_AUTH_CONFIG_FORMID3_FORM;
        } else {
          LabelId = TLS_AUTH_CONFIG_FORMID4_FORM;
        }

        //
        // Refresh selected file.
        //
        CleanUpPage (LabelId, Private);
        break;
      case KEY_TLS_AUTH_CONFIG_ENROLL_CERT_FROM_FILE:
        //
        // If the file is already opened, clean the file related resource first.
        //
        CleanFileContext (Private);

        ChooseFile (NULL, NULL, UpdateCAFromFile, &File);
        break;

      case KEY_TLS_AUTH_CONFIG_VALUE_SAVE_AND_EXIT:
        Status = EnrollCertDatabase (Private, EFI_TLS_CA_CERTIFICATE_VARIABLE);
        if (EFI_ERROR (Status)) {
          CleanFileContext (Private);

          HiiPopUp->CreatePopup (
                      HiiPopUp,
                      EfiHiiPopupStyleError,
                      EfiHiiPopupTypeOk,
                      Private->RegisteredHandle,
                      STRING_TOKEN (STR_TLS_AUTH_ENROLL_CERT_FAILURE),
                      &PopUpSelect
                      );
        }

        break;

      case KEY_TLS_AUTH_CONFIG_VALUE_NO_SAVE_AND_EXIT:
        CleanFileContext (Private);

        if (Private->CertGuid != NULL) {
          FreePool (Private->CertGuid);
          Private->CertGuid = NULL;
        }

        break;

      case KEY_TLS_AUTH_CONFIG_DELETE_CERT:
        UpdateDeletePage (
          Private,
          EFI_TLS_CA_CERTIFICATE_VARIABLE,
          &gEfiTlsCaCertificateGuid,
          LABEL_CA_DELETE,
          TLS_AUTH_CONFIG_FORMID5_FORM,
          OPTION_DEL_CA_ESTION_ID
          );
        break;

      default:
        if ((QuestionId >= OPTION_DEL_CA_ESTION_ID) &&
            (QuestionId < (OPTION_DEL_CA_ESTION_ID + OPTION_CONFIG_RANGE)))
        {
          DeleteCert (
            Private,
            EFI_TLS_CA_CERTIFICATE_VARIABLE,
            &gEfiTlsCaCertificateGuid,
            LABEL_CA_DELETE,
            TLS_AUTH_CONFIG_FORMID5_FORM,
            OPTION_DEL_CA_ESTION_ID,
            QuestionId - OPTION_DEL_CA_ESTION_ID
            );
        }

        break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    switch (QuestionId) {
      case KEY_TLS_AUTH_CONFIG_CERT_GUID:
        ASSERT (Private->CertGuid != NULL);
        RStatus = StrToGuid (
                    IfrNvData->CertGuid,
                    Private->CertGuid
                    );
        if (RETURN_ERROR (RStatus) || (IfrNvData->CertGuid[GUID_STRING_LENGTH] != L'\0')) {
          Status = EFI_INVALID_PARAMETER;
          break;
        }

        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
        break;
      default:
        break;
    }
  } else if (Action == EFI_BROWSER_ACTION_FORM_CLOSE) {
    CleanFileContext (Private);
  }

EXIT:

  if (!EFI_ERROR (Status)) {
    BufferSize = sizeof (TLS_AUTH_CONFIG_IFR_NVDATA);
    HiiSetBrowserData (&gTlsAuthConfigGuid, mTlsAuthConfigStorageName, BufferSize, (UINT8 *)IfrNvData, NULL);
  }

  FreePool (IfrNvData);

  if (File != NULL) {
    FreePool (File);
    File = NULL;
  }

  return EFI_SUCCESS;
}
