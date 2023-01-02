#ifndef APPLE_DEVICE_PATH_H
#define APPLE_DEVICE_PATH_H

#include <Protocol/DevicePath.h>

#pragma pack(1)

#define APPLE_APFS_VOLUME_DEVICE_PATH_GUID  \
  { 0xBE74FCF7, 0x0B7C, 0x49F3,             \
    { 0x91, 0x47, 0x01, 0xF4, 0x04, 0x2E, 0x68, 0x42 } }

typedef PACKED struct {
  VENDOR_DEVICE_PATH    Header;
  GUID                  Uuid;
} APPLE_APFS_VOLUME_DEVICE_PATH;

extern EFI_GUID  gAppleApfsVolumeDevicePathGuid;

#define APPLE_SD_CARD_VENDOR_DEVICE_PATH_GUID \
  { 0xC063C579, 0x9F78, 0x4BA5,               \
    { 0x9F, 0x42, 0xD0, 0xB0, 0x14, 0x95, 0x97, 0xA6 } }

extern EFI_GUID  gAppleSdCardVendorDevicePathGuid;

#pragma pack()

#endif // APPLE_DEVICE_PATH_H
