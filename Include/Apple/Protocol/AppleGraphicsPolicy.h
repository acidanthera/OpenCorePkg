/*++
 
 Created by HermitCrabs on 26/4/2015.
 Copyright 2010-2014 The HermitCrab Labs. All rights reserved.
 
 Module Name:
 
 AppleGraphicsPolicy.h
 
 Abstract:
 
 Revision History
 
 1.0 Initial Version
 
 --*/

#ifndef APPLE_GRAPHICS_POLICY_H
#define APPLE_GRAPHICS_POLICY_H

#define APPLE_GRAPHICS_POLICY_PROTCOL_GUID                \
  { 0xA4BB4654, 0x9F72, 0x4BC8,                           \
    { 0x93, 0xEB, 0x65, 0x9F, 0xD8, 0x70, 0x8B, 0x10 } }

typedef struct APPLE_GRAPHICS_POLICY_PROTOCOL APPLE_GRAPHICS_POLICY_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_MEMBER_01)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
     VOID                            *param2
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_MEMBER_02)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
  IN UINT16                          VendorId,
  IN UINT32                          DeviceId,
  IN EFI_HANDLE                      ControllerHandle
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_MEMBER_03)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_MEMBER_04)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
  IN EFI_GUID                        *Guid,
  IN VOID                            *Buffer,
  IN UINT64                          *BufferSize
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_MEMBER_05)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL        *DevicePath,
  IN CHAR16                          *ValueBuffer,
  IN CHAR16                          *Variable,
  IN VOID                            *OutputBuffer,
  IN UINTN                           OutputBufferLength
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_MEMBER_06)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_POLICY_MEMBER_07)(
  IN APPLE_GRAPHICS_POLICY_PROTOCOL  *This,
  IN UINT32                          Function,
  IN VOID                            *Buffer,
  IN UINT64                          *BufferSize
  );

struct APPLE_GRAPHICS_POLICY_PROTOCOL {
  UINTN                            Revision;
  APPLE_GRAPHICS_POLICY_MEMBER_01	 Unknown_01;
  APPLE_GRAPHICS_POLICY_MEMBER_02	 Unknown_02;
  APPLE_GRAPHICS_POLICY_MEMBER_03	 Unknown_03;
  APPLE_GRAPHICS_POLICY_MEMBER_04	 Unknown_04;
  APPLE_GRAPHICS_POLICY_MEMBER_05	 Unknown_05;
  APPLE_GRAPHICS_POLICY_MEMBER_06  Unknown_06;
  APPLE_GRAPHICS_POLICY_MEMBER_07  Unknown_07;
};

extern EFI_GUID gAppleGraphicsPolicyProtocolGuid;

#endif // APPLE_GRAPHICS_POLICY_H
