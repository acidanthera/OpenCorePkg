/*
 * File: HttpDemo.c
 *
 * Copyright (c) 2018 John Davis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "HttpDemo.h"
#include <Library/ShellLib.h>

#include <Protocol/Http.h>
#include <Protocol/Tcp4.h>
#include <Protocol/ServiceBinding.h>

// #include <Protocol/DmgRamDiskInterface.h>

// #include <Library/DmgImageLib.h>

#include <Library/OcXmlLib.h>

#define OSRECOVERY_URL L"http://17.57.21.52/"
#define OSRECOVERY_RECOVERYIMAGE_URL L"http://17.57.21.52/InstallationPayload/RecoveryImage"

#define HTTP_HOST_NAME          "Host"
#define HTTP_HOST_VALUE         "osrecovery.apple.com"
#define HTTP_CONNECTION_NAME    "Connection"
#define HTTP_CONNECTION_VALUE   "close"
#define HTTP_USER_AGENT_NAME    "User-Agent"
#define HTTP_USER_AGENT_VALUE   "InternetRecovery/1.0"
#define HTTP_SET_COOKIE_NAME    "Set-Cookie"

#define HTTP_COOKIE_NAME        "Cookie"
#define HTTP_CONTENT_TYPE_NAME          "Content-Type"
#define HTTP_CONTENT_TYPE_VALUE_TEXT    "text/plain"
#define HTTP_CONTENT_LENGTH_NAME        "Content-Length"

VOID
EFIAPI
HttpCallback(
    IN EFI_EVENT Event,
    IN VOID *Context) {
    *((BOOLEAN*)Context) = TRUE;
}

EFI_STATUS
EFIAPI
GetSessionCookie(
    IN EFI_HTTP_PROTOCOL *HttpIo,
    OUT CHAR8 **SessionCookieValue) {
    DEBUG((DEBUG_INFO, "GetSessionCookie(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_EVENT CallbackEvent;
    BOOLEAN CallbackReached;
    CHAR8 *SessionStr;
    CHAR8 *SessionStrEnd;
    UINTN SessionLength;
    CHAR8 *SessionValue = NULL;

    // GET request to service.
    EFI_HTTP_REQUEST_DATA GetRequestData;
    EFI_HTTP_MESSAGE GetRequestMessage;
    EFI_HTTP_TOKEN GetRequestToken;

    // GET response from service.
    EFI_HTTP_RESPONSE_DATA GetResponseData;
    EFI_HTTP_MESSAGE GetResponseMessage;
    EFI_HTTP_TOKEN GetResponseToken;

    // Create callback event.
    Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, HttpCallback,
        &CallbackReached, &CallbackEvent);
    if (EFI_ERROR(Status))
        goto DONE;

    //
    // Request.
    //
    // Initialize request data.
    ZeroMem(&GetRequestData, sizeof(EFI_HTTP_REQUEST_DATA));
    GetRequestData.Method = HttpMethodGet;
    GetRequestData.Url = OSRECOVERY_URL;

    // Initialize request message.
    ZeroMem(&GetRequestMessage, sizeof(EFI_HTTP_MESSAGE));
    GetRequestMessage.Data.Request = &GetRequestData;
    GetRequestMessage.HeaderCount = 3;
    GetRequestMessage.Headers = AllocateZeroPool(GetRequestMessage.HeaderCount * sizeof(EFI_HTTP_HEADER));
    if (!GetRequestMessage.Headers) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE;
    }

    // Set request headers.
    GetRequestMessage.Headers[0].FieldName = HTTP_HOST_NAME;
    GetRequestMessage.Headers[0].FieldValue = HTTP_HOST_VALUE;
    GetRequestMessage.Headers[1].FieldName = HTTP_CONNECTION_NAME;
    GetRequestMessage.Headers[1].FieldValue = HTTP_CONNECTION_VALUE;
    GetRequestMessage.Headers[2].FieldName = HTTP_USER_AGENT_NAME;
    GetRequestMessage.Headers[2].FieldValue = HTTP_USER_AGENT_VALUE;

    // Initialize request token.
    ZeroMem(&GetRequestToken, sizeof(EFI_HTTP_TOKEN));
    GetRequestToken.Event = CallbackEvent;
    GetRequestToken.Message = &GetRequestMessage;

    //
    // Response.
    //
    // Initialize response data.
    ZeroMem(&GetResponseData, sizeof(EFI_HTTP_RESPONSE_DATA));

    // Initialize response message.
    ZeroMem(&GetResponseMessage, sizeof(EFI_HTTP_MESSAGE));
    GetResponseMessage.Data.Response = &GetResponseData;

    // Initialize response token.
    ZeroMem(&GetResponseToken, sizeof(EFI_HTTP_TOKEN));
    GetResponseToken.Event = CallbackEvent;
    GetResponseToken.Message = &GetResponseMessage;

    // Send request.
    CallbackReached = FALSE;
    Status = HttpIo->Request(HttpIo, &GetRequestToken);
    if (EFI_ERROR(Status))
        goto DONE;

    // Wait for request to send.
    while (!CallbackReached);

    // Get response.
    CallbackReached = FALSE;
    Status = HttpIo->Response(HttpIo, &GetResponseToken);
    if (EFI_ERROR(Status))
        goto DONE;

    // Wait for response to come in.
    while (!CallbackReached);

    // Get cookie session value.
    for (UINTN i = 0; i < GetResponseMessage.HeaderCount; i++) {
        if (!AsciiStrCmp(GetResponseMessage.Headers[i].FieldName, HTTP_SET_COOKIE_NAME)) {
            // Find start and end.
            SessionStr = AsciiStrStr(GetResponseMessage.Headers[i].FieldValue, "session=");
            SessionStrEnd = AsciiStrStr(SessionStr, ";");
            if (!SessionStrEnd)
                SessionStrEnd = AsciiStrStr(SessionStr, "\r\n");
            if (!SessionStrEnd) {
                Status = EFI_INVALID_PARAMETER;
                goto DONE;
            }

            // Determine length of session cookie.
            SessionLength = SessionStrEnd - SessionStr;

            // Allocate session cookie.
            SessionValue = AllocatePool(SessionLength + 1);
            if (!SessionValue) {
                Status = EFI_OUT_OF_RESOURCES;
                goto DONE;
            }

            // Copy session cookie.
            CopyMem(SessionValue, SessionStr, SessionLength);
            SessionValue[SessionLength] = '\0';
            *SessionCookieValue = SessionValue;
            
            // Success.
            Status = EFI_SUCCESS;
            goto DONE;
        }
    }

    // If we get here, we couldn't find the session cookie.
    Status = EFI_NOT_FOUND;

DONE:
    // Free headers.
    if (GetRequestMessage.Headers)
        FreePool(GetRequestMessage.Headers);
    if (GetResponseMessage.Headers) {
        for (UINTN i = 0; i < GetResponseMessage.HeaderCount; i++) {
            if (GetResponseMessage.Headers[i].FieldName)
                FreePool(GetResponseMessage.Headers[i].FieldName);
            if (GetResponseMessage.Headers[i].FieldValue)
                FreePool(GetResponseMessage.Headers[i].FieldValue);
        }
        FreePool(GetResponseMessage.Headers);
    }

    // Free event.
    gBS->CloseEvent(CallbackEvent);
    return Status;
}

EFI_STATUS
EFIAPI
PostMachineData(
    IN EFI_HTTP_PROTOCOL *HttpIo,
    IN CHAR8 *MachineData,
    IN CHAR8 *SessionCookieValue,
    OUT CHAR8 **RecoveryImageData) {
    DEBUG((DEBUG_INFO, "PostMachineDetails(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_EVENT CallbackEvent;
    BOOLEAN CallbackReached;
    UINTN MachineDataLength;
    CHAR16 *MachineDataLengthUniStr;
    CHAR8 *MachineDataLengthStr;
    CHAR8 *RecoveryImageStr = NULL;

    // POST request to service.
    EFI_HTTP_REQUEST_DATA PostRequestData;
    EFI_HTTP_MESSAGE PostRequestMessage;
    EFI_HTTP_TOKEN PostRequestToken;

    // POST response from service.
    EFI_HTTP_RESPONSE_DATA PostResponseData;
    EFI_HTTP_MESSAGE PostResponseMessage;
    EFI_HTTP_TOKEN PostResponseToken;

    // Create callback event.
    Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, HttpCallback,
        &CallbackReached, &CallbackEvent);
    if (EFI_ERROR(Status))
        goto DONE;

    //
    // Request.
    //
    // Initialize request data.
    ZeroMem(&PostRequestData, sizeof(EFI_HTTP_REQUEST_DATA));
    PostRequestData.Method = HttpMethodPost;
    PostRequestData.Url = OSRECOVERY_RECOVERYIMAGE_URL;

    // Initialize request message.
    ZeroMem(&PostRequestMessage, sizeof(EFI_HTTP_MESSAGE));
    PostRequestMessage.Data.Request = &PostRequestData;
    PostRequestMessage.HeaderCount = 6;
    PostRequestMessage.Headers = AllocateZeroPool(PostRequestMessage.HeaderCount * sizeof(EFI_HTTP_HEADER));
    if (!PostRequestMessage.Headers) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE;
    }

    // Get length of machine data.
    MachineDataLength = AsciiStrLen(MachineData);
    MachineDataLengthUniStr = CatSPrint(NULL, L"%u", MachineDataLength);
    if (!MachineDataLengthUniStr) {
        Status = EFI_INVALID_PARAMETER;
        goto DONE;
    }

    // Get 8-bit string.
    MachineDataLengthStr = AllocateZeroPool((StrLen(MachineDataLengthUniStr) + 1) * sizeof(CHAR8));
    for (UINTN i = 0; i < StrLen(MachineDataLengthUniStr); i++)
        MachineDataLengthStr[i] = (CHAR8)(MachineDataLengthUniStr[i]);

    // Set request headers.
    PostRequestMessage.Headers[0].FieldName = HTTP_HOST_NAME;
    PostRequestMessage.Headers[0].FieldValue = HTTP_HOST_VALUE;
    PostRequestMessage.Headers[1].FieldName = HTTP_CONNECTION_NAME;
    PostRequestMessage.Headers[1].FieldValue = HTTP_CONNECTION_VALUE;
    PostRequestMessage.Headers[2].FieldName = HTTP_USER_AGENT_NAME;
    PostRequestMessage.Headers[2].FieldValue = HTTP_USER_AGENT_VALUE;
    PostRequestMessage.Headers[3].FieldName = HTTP_COOKIE_NAME;
    PostRequestMessage.Headers[3].FieldValue = SessionCookieValue;
    PostRequestMessage.Headers[4].FieldName = HTTP_CONTENT_TYPE_NAME;
    PostRequestMessage.Headers[4].FieldValue = HTTP_CONTENT_TYPE_VALUE_TEXT;
    PostRequestMessage.Headers[5].FieldName = HTTP_CONTENT_LENGTH_NAME;
    PostRequestMessage.Headers[5].FieldValue = MachineDataLengthStr;

    // Set data.
    PostRequestMessage.Body = MachineData;
    PostRequestMessage.BodyLength = MachineDataLength;

    // Initialize request token.
    ZeroMem(&PostRequestToken, sizeof(EFI_HTTP_TOKEN));
    PostRequestToken.Event = CallbackEvent;
    PostRequestToken.Message = &PostRequestMessage;

    //
    // Response.
    //
    // Initialize response data.
    ZeroMem(&PostResponseData, sizeof(EFI_HTTP_RESPONSE_DATA));

    // Initialize response message.
    ZeroMem(&PostResponseMessage, sizeof(EFI_HTTP_MESSAGE));
    PostResponseMessage.Data.Response = &PostResponseData;

    // Allocate 4KB worth of buffer.
    PostResponseMessage.BodyLength = SIZE_4KB;
    PostResponseMessage.Body = AllocateZeroPool(PostResponseMessage.BodyLength);

    // Initialize response token.
    ZeroMem(&PostResponseToken, sizeof(EFI_HTTP_TOKEN));
    PostResponseToken.Event = CallbackEvent;
    PostResponseToken.Message = &PostResponseMessage;

    // Send request.
    CallbackReached = FALSE;
    Status = HttpIo->Request(HttpIo, &PostRequestToken);
    if (EFI_ERROR(Status))
        goto DONE;

    // Wait for request to send.
    while (!CallbackReached);
    Print(L"status %r\n", PostRequestToken.Status);

    // Get response.
    CallbackReached = FALSE;
    Status = HttpIo->Response(HttpIo, &PostResponseToken);
    if (EFI_ERROR(Status))
        goto DONE;

    // Wait for response to come in.
    while (!CallbackReached);

    // Get recovery image data from response.
    for (UINTN i = 0; i < PostResponseMessage.HeaderCount; i++) {
        // Ensure that Content-Type is text/plain.
        if (!AsciiStrCmp(PostResponseMessage.Headers[i].FieldName, HTTP_CONTENT_TYPE_NAME) &&
            AsciiStrStr(PostResponseMessage.Headers[i].FieldValue, HTTP_CONTENT_TYPE_VALUE_TEXT)) {
            // Allocate recovery image data.
            RecoveryImageStr = AllocateZeroPool(PostResponseMessage.BodyLength + 1);
            if (!RecoveryImageData) {
                Status = EFI_OUT_OF_RESOURCES;
                goto DONE;
            }

            // Copy recovery image data.
            CopyMem(RecoveryImageStr, PostResponseMessage.Body, PostResponseMessage.BodyLength);
            RecoveryImageStr[PostResponseMessage.BodyLength] ='\0';
            *RecoveryImageData = RecoveryImageStr;

            // Success.
            Status = EFI_SUCCESS;
            goto DONE;
        }
    }

    // If we get here, we couldn't get the recovery image data.
    Status = EFI_NOT_FOUND;

DONE:
    // Free headers.
    if (PostRequestMessage.Headers)
        FreePool(PostRequestMessage.Headers);
    if (PostResponseMessage.Headers) {
        for (UINTN i = 0; i < PostResponseMessage.HeaderCount; i++) {
            if (PostResponseMessage.Headers[i].FieldName)
                FreePool(PostResponseMessage.Headers[i].FieldName);
            if (PostResponseMessage.Headers[i].FieldValue)
                FreePool(PostResponseMessage.Headers[i].FieldValue);
        }
        FreePool(PostResponseMessage.Headers);
    }

    // Free strings.
    if (MachineDataLengthUniStr)
        FreePool(MachineDataLengthUniStr);
    if (MachineDataLengthStr)
        FreePool(MachineDataLengthStr);

    // Free event.
    gBS->CloseEvent(CallbackEvent);
    return Status;
}

EFI_STATUS
EFIAPI
HttpDemoMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"HttpDemo start\n");

    EFI_STATUS Status;
    EFI_HANDLE *handles;
    UINTN handleCount;
    
   Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiTcp4ServiceBindingProtocolGuid, NULL, &handleCount, &handles);
   ASSERT_EFI_ERROR(Status);
    

   Print(L"Handles: %u\n", handleCount);

    handleCount = 0;
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiHttpServiceBindingProtocolGuid, NULL, &handleCount, &handles);
    ASSERT_EFI_ERROR(Status);
    Print(L"Handles: %u\n", handleCount);

    // Get http service.
    EFI_SERVICE_BINDING_PROTOCOL *ServiceBinding;
    Status = gBS->HandleProtocol(handles[0], &gEfiHttpServiceBindingProtocolGuid, (VOID**)&ServiceBinding);
    ASSERT_EFI_ERROR(Status);

    // create child.
    EFI_HTTP_PROTOCOL *HttpIo;
    Status = ServiceBinding->CreateChild(ServiceBinding, &ImageHandle);
    ASSERT_EFI_ERROR(Status);

    Status = gBS->HandleProtocol(ImageHandle, &gEfiHttpProtocolGuid, (VOID**)&HttpIo);
    ASSERT_EFI_ERROR(Status);
    //Print(L"Handles: %u\n", handleCount);

    EFI_HTTPv4_ACCESS_POINT ipv4Access;
    ipv4Access.UseDefaultAddress = TRUE;
    ipv4Access.LocalPort = 10000;

    EFI_HTTP_CONFIG_DATA configData;
    configData.HttpVersion = HttpVersion11;
    configData.TimeOutMillisec = 2000;
    configData.LocalAddressIsIPv6 = FALSE;
    configData.AccessPoint.IPv4Node = &ipv4Access;


    Status = HttpIo->Configure(HttpIo, NULL);
    ASSERT_EFI_ERROR(Status);
    Status = HttpIo->Configure(HttpIo, &configData);
    ASSERT_EFI_ERROR(Status);

    CHAR8 *SessionCookieValue = "Test";

    CHAR8 *MachineData = "cid=0000000000000000\nsn=C072164044NDVF9UE\nbid=Mac-F65AE981FFA204ED\nk=0000000000000000000000000000000000000000000000000000000000000000\nos=default\nfg=0000000000000000000000000000000000000000000000000000000000000000";
    CHAR8 *RecoveryData;

    Status = GetSessionCookie(HttpIo, &SessionCookieValue);
    ASSERT_EFI_ERROR(Status);
    Status = PostMachineData(HttpIo, MachineData, SessionCookieValue, &RecoveryData);
    ASSERT_EFI_ERROR(Status);

    

    Print(L"Machine data:\n");
    AsciiPrint(RecoveryData);



    // Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &handleCount, &handles);
    // ASSERT_EFI_ERROR(Status);

    // EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = NULL;
    // DEBUG((DEBUG_INFO, "Handles %u\n", handleCount));
    // EFI_FILE_PROTOCOL* root = NULL;

    // opewn file.
    // EFI_FILE_PROTOCOL* token = NULL;
    // for (UINTN handle = 0; handle < handleCount; handle++) {
    //     Status = gBS->HandleProtocol(handles[handle], &gEfiSimpleFileSystemProtocolGuid, (void**)&fs);
    //     ASSERT_EFI_ERROR(Status);

    //     Status = fs->OpenVolume(fs, &root);
    //     ASSERT_EFI_ERROR(Status);

    //     Status = root->Open(root, &token, L"BaseSystem.dmg", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    //     if (!(EFI_ERROR(Status)))
    //         break;
    // }

    // Get size.
    // EFI_FILE_INFO *FileInfo;

    // UINTN FileInfoSize = 512;
    // VOID *fileinfobuf = AllocateZeroPool(FileInfoSize);
    // Status = token->GetInfo(token, &gEfiFileInfoGuid, &FileInfoSize, fileinfobuf);
    // ASSERT_EFI_ERROR(Status);

    // FileInfo = (EFI_FILE_INFO*)fileinfobuf;

    // Print(L"File size: %u bytes\n", FileInfo->FileSize);



    // Get DMG.
    /*DMG_FILE *File;
    Status = DmgImageRead(bytes, bytesLength, &File);
    ASSERT_EFI_ERROR(Status);
    
    Print(L"DMG version 0x%X\n", File->Trailer.Version);
    Print(L"Data fork offset: 0x%X, length: 0x%X\n", File->Trailer.DataForkOffset, File->Trailer.DataForkLength);
    Print(L"XML offset: 0x%X, length: 0x%X\n", File->Trailer.XmlOffset, File->Trailer.XmlLength);
    Print(L"Sector count: %u\n", File->Trailer.SectorCount);
    Print(L"Block count: %u\n", File->BlockCount);

    for (UINT32 b = 0; b < 3; b++) {
        Print(L"Block %u: ", b);
        AsciiPrint(File->Blocks[b].Name);
        Print(L"\n  Attributes: 0x%X\n", File->Blocks[b].Attributes);
        Print(L"  Start sector: %u, count: %u\n", File->Blocks[b].BlockData->SectorNumber, File->Blocks[b].BlockData->SectorCount);
        Print(L"  Data offset: 0x%lx\n", File->Blocks[b].BlockData->DataOffset);
        Print(L"  Chunk count: %u\n", File->Blocks[b].BlockData->NumberOfBlockChunks);
        for (UINT32 c = 0; c < File->Blocks[b].BlockData->NumberOfBlockChunks; c++) {
            Print(L"  Chunk %u (type 0x%X): start %u count %u\n", c, File->Blocks[b].BlockData->BlockChunks[c].Type,
                File->Blocks[b].BlockData->BlockChunks[c].SectorNumber, File->Blocks[b].BlockData->BlockChunks[c].SectorCount);
            Print(L"    Data: 0x%X length 0x%X\n", File->Blocks[b].BlockData->BlockChunks[c].CompressedOffset, File->Blocks[b].BlockData->BlockChunks[c].CompressedLength);
        }
    }*/


    // Get handle.
    /*EFI_DMG_RAM_DISK_INTERFACE_PROTOCOL *DmgRamDiskInterface;
    Status = gBS->LocateProtocol(&gEfiDmgRamDiskInterfaceProtocolGuid, NULL, (VOID**)&DmgRamDiskInterface);
    ASSERT_EFI_ERROR(Status);

    // Allocate DMG.
    UINTN bytesToRead = FileInfo->FileSize;
    VOID *bytes;
    UINTN bytesLength;
    Status = DmgRamDiskInterface->AllocateDmg(DmgRamDiskInterface, bytesToRead, &bytes, &bytesLength);
    ASSERT_EFI_ERROR(Status);

    // Read DMG into buffer.
    Status = token->Read(token, &bytesToRead, bytes);
    ASSERT_EFI_ERROR(Status);

    // Install DMG.
    Status = DmgRamDiskInterface->InstallDmg(DmgRamDiskInterface, bytes, bytesLength);
    ASSERT_EFI_ERROR(Status);*/

    return EFI_SUCCESS;
}
