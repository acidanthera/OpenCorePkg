/*++
 
 Created by HermitCrabs on 26/4/2015.
 Copyright 2010-2014 The HermitCrab Labs. All rights reserved.
 
 Module Name:
 
 AppleGraphicsDriver.h
 
 Abstract:
 
 Revision History
 
 1.0 Initial Version
 
 --*/

#ifndef APPLE_GRAPHICS_DRIVER_H
#define APPLE_GRAPHICS_DRIVER_H

#define APPLE_GRAPHICS_DRIVER_PROTOCOL_GUID               \
  { 0xDD8E06AC, 0x00E2, 0x49A9,                           \
    { 0x88, 0x8F, 0xFA, 0x46, 0xDE, 0xD4, 0x0A, 0x52 } }

typedef struct APPLE_GRAPHICS_DRIVER_PROTOCOL APPLE_GRAPHICS_DRIVER_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_DRIVER_SET_PROPERTY)(
  IN APPLE_GRAPHICS_DRIVER_PROTOCOL  *This,
  IN EFI_GUID                        *Guid,
  IN VOID                            *Buffer,
  IN UINTN                            BufferSize
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_DRIVER_GET_PROPERTY)(
  IN     APPLE_GRAPHICS_DRIVER_PROTOCOL  *This,
  IN     EFI_GUID                        *Guid,
  OUT    VOID                            *Buffer,
  IN OUT UINTN                           *BufferSize
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_DRIVER_SET_PROPERTY_BY_FUNCTION)(
  IN APPLE_GRAPHICS_DRIVER_PROTOCOL  *This,
  IN UINT32                          Function,
  IN VOID                            *Buffer,
  IN UINTN                           BufferSize
  );

typedef
EFI_STATUS
(EFIAPI *APPLE_GRAPHICS_DRIVER_GET_PROPERTY_BY_FUNCTION)(
  IN     APPLE_GRAPHICS_DRIVER_PROTOCOL  *This,
  IN     UINT32                          Function,
  OUT    VOID                            *Buffer,
  IN OUT UINTN                           *BufferSize
  );

struct APPLE_GRAPHICS_DRIVER_PROTOCOL {
	UINTN                                          Revision;
	APPLE_GRAPHICS_DRIVER_SET_PROPERTY		         SetProperty;
	APPLE_GRAPHICS_DRIVER_GET_PROPERTY		         GetProperty;
	APPLE_GRAPHICS_DRIVER_SET_PROPERTY_BY_FUNCTION SetPropertyByFunction;
	APPLE_GRAPHICS_DRIVER_GET_PROPERTY_BY_FUNCTION GetPropertyByFunction;
};

extern EFI_GUID gAppleGraphicsDriverProtocolGuid;

#endif // APPLE_GRAPHICS_DRIVER_H
