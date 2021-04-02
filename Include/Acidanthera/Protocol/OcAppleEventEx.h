#ifndef OC_APPLE_EVENT_EX_PROTOCOL_H_
#define OC_APPLE_EVENT_EX_PROTOCOL_H_

#include <Protocol/AppleEvent.h>

#define OC_APPLE_EVENT_EX_PROTOCOL_REVISION  0x00000001

#define OC_APPLE_EVENT_EX_PROTOCOL_GUID  \
  { 0xA35F3047, 0x4F1C, 0x4937,  \
    { 0xB9, 0x52, 0xBB, 0x58, 0x3A, 0x2E, 0x4E, 0x30 }}

typedef struct OC_APPLE_EVENT_EX_PROTOCOL_ OC_APPLE_EVENT_EX_PROTOCOL;

typedef
UINT32
(EFIAPI *OC_APPLE_EVENT_EX_SET_POINTER_SCALE)(
  IN OUT OC_APPLE_EVENT_EX_PROTOCOL  *This,
  IN     UINT32                      Scale
  );

struct OC_APPLE_EVENT_EX_PROTOCOL_ {
  UINTN                               Revision;
  OC_APPLE_EVENT_EX_SET_POINTER_SCALE SetPointerScale;
  APPLE_EVENT_PROTOCOL                AppleEvent;
};

extern EFI_GUID gOcAppleEventExProtocolGuid;

#endif // OC_APPLE_EVENT_EX_PROTOCOL_H_
