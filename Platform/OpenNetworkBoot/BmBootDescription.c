/** @file
  Library functions which relate to boot option description.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
Copyright (C) 2024, Mike Beaton. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NetworkBootInternal.h"

/**
  Return the description for network boot device.

  @param Handle                Controller handle.

  @return  The description string.
**/
CHAR16 *
BmGetNetworkDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  MAC_ADDR_DEVICE_PATH      *Mac;
  VLAN_DEVICE_PATH          *Vlan;
  EFI_DEVICE_PATH_PROTOCOL  *Ip;
  EFI_DEVICE_PATH_PROTOCOL  *Uri;
  CHAR16                    *Description;
  UINTN                     DescriptionSize;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiLoadFileProtocolGuid,
                  NULL,
                  gImageHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  gImageHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status) || (DevicePath == NULL)) {
    return NULL;
  }

  //
  // The PXE device path is like:
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv4(...)
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv6(...)
  //
  // The HTTP device path is like:
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv4(...)[/Dns(...)]/Uri(...)
  //   ....../Mac(...)[/Vlan(...)][/Wi-Fi(...)]/IPv6(...)[/Dns(...)]/Uri(...)
  //
  while (!IsDevicePathEnd (DevicePath) &&
         ((DevicePathType (DevicePath) != MESSAGING_DEVICE_PATH) ||
          (DevicePathSubType (DevicePath) != MSG_MAC_ADDR_DP))
         )
  {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  if (IsDevicePathEnd (DevicePath)) {
    return NULL;
  }

  Mac        = (MAC_ADDR_DEVICE_PATH *)DevicePath;
  DevicePath = NextDevicePathNode (DevicePath);

  //
  // Locate the optional Vlan node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_VLAN_DP)
      )
  {
    Vlan       = (VLAN_DEVICE_PATH *)DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    Vlan = NULL;
  }

  //
  // Skip the optional Wi-Fi node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_WIFI_DP)
      )
  {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  //
  // Locate the IP node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      ((DevicePathSubType (DevicePath) == MSG_IPv4_DP) ||
       (DevicePathSubType (DevicePath) == MSG_IPv6_DP))
      )
  {
    Ip         = DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    Ip = NULL;
  }

  //
  // Skip the optional DNS node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_DNS_DP)
      )
  {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  //
  // Locate the URI node
  //
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      (DevicePathSubType (DevicePath) == MSG_URI_DP)
      )
  {
    Uri        = DevicePath;
    DevicePath = NextDevicePathNode (DevicePath);
  } else {
    Uri = NULL;
  }

  //
  // Build description like below:
  //   "PXE Boot IPv6 (MAC:11-22-33-44-55-66 VLAN1)"
  //   "HTTP Boot IPv4 (MAC:11-22-33-44-55-66)"
  //
  DescriptionSize = sizeof (L"HTTP Boot IPv6 (MAC:11-22-33-44-55-66 VLAN65535)");
  Description     = AllocatePool (DescriptionSize);
  ASSERT (Description != NULL);
  UnicodeSPrint (
    Description,
    DescriptionSize,
    (Vlan == NULL) ?
    L"%s Boot IPv%d (MAC:%02x-%02x-%02x-%02x-%02x-%02x)" :
    L"%s Boot IPv%d (MAC:%02x-%02x-%02x-%02x-%02x-%02x VLAN%d)",
    (Uri == NULL) ? L"PXE" : L"HTTP",
    ((Ip == NULL) || (DevicePathSubType (Ip) == MSG_IPv4_DP)) ? 4 : 6,
    Mac->MacAddress.Addr[0],
    Mac->MacAddress.Addr[1],
    Mac->MacAddress.Addr[2],
    Mac->MacAddress.Addr[3],
    Mac->MacAddress.Addr[4],
    Mac->MacAddress.Addr[5],
    (Vlan == NULL) ? 0 : Vlan->VlanId
    );
  return Description;
}
