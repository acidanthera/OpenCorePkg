/** @file
  This header provides message definitions for select ME client communucation.
  This header is based on various sources:

  - Platform Embedded Security Technology Revealed
    https://link.springer.com/content/pdf/10.1007%2F978-1-4302-6572-6.pdf
  - EPID SDK
    https://github.com/Intel-EPID-SDK/epid-sdk
  - Android source code for x86
    https://github.com/shreekantsingh/uefi/tree/master/drivers/misc/mei

  Copyright (c) 2019, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef HECI_CLIENT_MSG_H
#define HECI_CLIENT_MSG_H

#include <Library/OcGuardLib.h>

//
// Select known clients
//

#pragma pack(1)

//
// Generic EPID SDK structures.
//

#define EPID_STATUS_PROVISIONED     0x00
#define EPID_STATUS_CAN_PROVISION   0x02
#define EPID_STATUS_FAIL_PROVISION  0x03

#define EPID_CERTIFICATE_SIZE          876U
#define EPID_GROUP_PUBLIC_KEY_SIZE     392U

typedef struct {
  UINT8   Data[EPID_CERTIFICATE_SIZE];
} EPID_CERTIFICATE;

typedef struct {
  UINT16  Version;           ///< EpidVersion
  UINT16  FileType;          ///< EpidFileType
  UINT32  GroupId;           ///< Epid11GroupId
  UINT8   Data[EPID_GROUP_PUBLIC_KEY_SIZE - sizeof (UINT16) * 2 - sizeof (UINT32)];
} EPID_GROUP_PUBLIC_KEY;

STATIC_ASSERT (sizeof (EPID_GROUP_PUBLIC_KEY) == EPID_GROUP_PUBLIC_KEY_SIZE, "Invalid GPK size");

//
// FBF6FCF1-96CF-4E2E-A6A6-1BAB8CBE36B1
//
#define ME_PAVP_PROTOCOL_GUID \
  { 0xFBF6FCF1, 0x96CF, 0x4E2E, { 0xA6, 0xA6, 0x1B, 0xAB, 0x8C, 0xBE, 0x36, 0xB1 } }

extern EFI_GUID gMePavpProtocolGuid;

#define ME_PAVP_PROTOCOL_VERSION          0x10005U

#define ME_PAVP_PROVISION_REQUEST_COMMAND 0x00000000
#define ME_PAVP_PROVISION_PERFORM_COMMAND 0x00000001
#define ME_PAVP_INITIALIZE_DMA_COMMAND    0x000A0002
#define ME_PAVP_DEINITIALIZE_DMA_COMMAND  0x000A000B

//
// Common header for all commands
//

typedef struct {
  UINT32  Version;
  UINT32  Command;
  UINT32  Status;
  UINT32  PayloadSize;
} ME_PAVP_COMMAND_HEADER;

//
// ME_PAVP_PROVISION_REQUEST_COMMAND
//

typedef struct {
  ME_PAVP_COMMAND_HEADER  Header;
} ME_PAVP_PROVISION_REQUEST_REQUEST;

typedef struct {
  ME_PAVP_COMMAND_HEADER  Header;
  UINT32                  Status;
  UINT32                  GroupId;
} ME_PAVP_PROVISION_REQUEST_RESPONSE;

typedef union {
  ME_PAVP_PROVISION_REQUEST_REQUEST   Request;
  ME_PAVP_PROVISION_REQUEST_RESPONSE  Response;
} ME_PAVP_PROVISION_REQUEST_BUFFER;

//
// ME_PAVP_PROVISION_PERFORM_COMMAND
//

typedef struct {
  ME_PAVP_COMMAND_HEADER  Header;
  UINT8                   Certificate[EPID_CERTIFICATE_SIZE];
  UINT8                   PublicKey[EPID_GROUP_PUBLIC_KEY_SIZE];
} ME_PAVP_PROVISION_PERFORM_REQUEST;

typedef struct {
  ME_PAVP_COMMAND_HEADER  Header;
} ME_PAVP_PROVISION_PERFORM_RESPONSE;

typedef union {
  ME_PAVP_PROVISION_PERFORM_REQUEST   Request;
  ME_PAVP_PROVISION_PERFORM_RESPONSE  Response;
} ME_PAVP_PROVISION_PERFORM_BUFFER;

#define ME_PAVP_PROVISION_PERFORM_PAYLOAD_SIZE (EPID_CERTIFICATE_SIZE + EPID_GROUP_PUBLIC_KEY_SIZE)

//
// 3893448C-EAB6-4F4C-B23C-57C2C4658DFC
//
#define ME_FPF_PROTOCOL_GUID \
  { 0x3893448C, 0xEAB6, 0x4F4C, { 0xB2, 0x3C, 0x57, 0xC2, 0xC4, 0x65, 0x8D, 0xFC } }

extern EFI_GUID gMeFpfProtocolGuid;

#pragma pack()

#endif // HECI_MSG_H
