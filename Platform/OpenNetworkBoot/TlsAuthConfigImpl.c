/** @file
  The Miscellaneous Routines for TlsAuthConfigDxe driver.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TlsAuthConfigImpl.h"

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
