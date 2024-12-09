/** @file
  IPv4 utilities.

  Methods from:
   - NetworkPkg/Ip4Dxe/Ip4Common.c
   - NetworkPkg/Ip4Dxe/Ip4Config2Nv.c

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "NetworkBootInternal.h"

/**
  Validate that Ip/Netmask pair is OK to be used as station
  address. Only continuous netmasks are supported. and check
  that StationAddress is a unicast address on the network.

  @param[in]  Ip                 The IP address to validate.
  @param[in]  Netmask            The netmask of the IP.

  @retval TRUE                   The Ip/Netmask pair is valid.
  @retval FALSE                  The Ip/Netmask pair is invalid.

**/
BOOLEAN
Ip4StationAddressValid (
  IN IP4_ADDR  Ip,
  IN IP4_ADDR  Netmask
  )
{
  //
  // Only support the station address with 0.0.0.0/0 to enable DHCP client.
  //
  if (Netmask == IP4_ALLZERO_ADDRESS) {
    return (BOOLEAN)(Ip == IP4_ALLZERO_ADDRESS);
  }

  //
  // Only support the continuous net masks
  //
  if (NetGetMaskLength (Netmask) == (IP4_MASK_MAX + 1)) {
    return FALSE;
  }

  //
  // Station address can't be class D or class E address
  //
  if (NetGetIpClass (Ip) > IP4_ADDR_CLASSC) {
    return FALSE;
  }

  return NetIp4IsUnicast (Ip, Netmask);
}

/**
  Calculate the prefix length of the IPv4 subnet mask.

  @param[in]  SubnetMask The IPv4 subnet mask.

  @return The prefix length of the subnet mask.
  @retval 0 Other errors as indicated.

**/
UINT8
GetSubnetMaskPrefixLength (
  IN EFI_IPv4_ADDRESS  *SubnetMask
  )
{
  UINT8   Len;
  UINT32  ReverseMask;

  //
  // The SubnetMask is in network byte order.
  //
  ReverseMask = SwapBytes32 (*(UINT32 *)&SubnetMask[0]);

  //
  // Reverse it.
  //
  ReverseMask = ~ReverseMask;

  if ((ReverseMask & (ReverseMask + 1)) != 0) {
    return 0;
  }

  Len = 0;

  while (ReverseMask != 0) {
    ReverseMask = ReverseMask >> 1;
    Len++;
  }

  return (UINT8)(32 - Len);
}

/**
  Convert the decimal dotted IPv4 address into the binary IPv4 address.

  @param[in]   Str             The UNICODE string.
  @param[out]  Ip              The storage to return the IPv4 address.

  @retval EFI_SUCCESS           The binary IP address is returned in Ip.
  @retval EFI_INVALID_PARAMETER The IP string is malformatted.

**/
EFI_STATUS
Ip4Config2StrToIp (
  IN  CHAR16            *Str,
  OUT EFI_IPv4_ADDRESS  *Ip
  )
{
  UINTN  Index;
  UINTN  Number;

  Index = 0;

  while (*Str != L'\0') {
    if (Index > 3) {
      return EFI_INVALID_PARAMETER;
    }

    Number = 0;
    while ((*Str >= L'0') && (*Str <= L'9')) {
      Number = Number * 10 + (*Str - L'0');
      Str++;
    }

    if (Number > 0xFF) {
      return EFI_INVALID_PARAMETER;
    }

    Ip->Addr[Index] = (UINT8)Number;

    if ((*Str != L'\0') && (*Str != L'.')) {
      //
      // The current character should be either the NULL terminator or
      // the dot delimiter.
      //
      return EFI_INVALID_PARAMETER;
    }

    if (*Str == L'.') {
      //
      // Skip the delimiter.
      //
      Str++;
    }

    Index++;
  }

  if (Index != 4) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Convert the decimal dotted IPv4 addresses separated by space into the binary IPv4 address list.

  @param[in]   Str             The UNICODE string contains IPv4 addresses.
  @param[out]  PtrIpList       The storage to return the IPv4 address list.
  @param[out]  IpCount         The size of the IPv4 address list.

  @retval EFI_SUCCESS           The binary IP address list is returned in PtrIpList.
  @retval EFI_OUT_OF_RESOURCES  Error occurs in allocating memory.
  @retval EFI_INVALID_PARAMETER The IP string is malformatted.

**/
EFI_STATUS
Ip4Config2StrToIpList (
  IN  CHAR16            *Str,
  OUT EFI_IPv4_ADDRESS  **PtrIpList,
  OUT UINTN             *IpCount
  )
{
  UINTN    BeginIndex;
  UINTN    EndIndex;
  UINTN    Index;
  UINTN    IpIndex;
  CHAR16   *StrTemp;
  BOOLEAN  SpaceTag;

  BeginIndex = 0;
  EndIndex   = BeginIndex;
  Index      = 0;
  IpIndex    = 0;
  StrTemp    = NULL;
  SpaceTag   = TRUE;

  *PtrIpList = NULL;
  *IpCount   = 0;

  if (Str == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Get the number of Ip.
  //
  while (*(Str + Index) != L'\0') {
    if (*(Str + Index) == L' ') {
      SpaceTag = TRUE;
    } else {
      if (SpaceTag) {
        (*IpCount)++;
        SpaceTag = FALSE;
      }
    }

    Index++;
  }

  if (*IpCount == 0) {
    return EFI_SUCCESS;
  }

  //
  // Allocate buffer for IpList.
  //
  *PtrIpList = AllocateZeroPool (*IpCount * sizeof (EFI_IPv4_ADDRESS));
  if (*PtrIpList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get IpList from Str.
  //
  Index = 0;
  while (*(Str + Index) != L'\0') {
    if (*(Str + Index) == L' ') {
      if (!SpaceTag) {
        StrTemp = AllocateZeroPool ((EndIndex - BeginIndex + 1) * sizeof (CHAR16));
        if (StrTemp == NULL) {
          FreePool (*PtrIpList);
          *PtrIpList = NULL;
          *IpCount   = 0;
          return EFI_OUT_OF_RESOURCES;
        }

        CopyMem (StrTemp, Str + BeginIndex, (EndIndex - BeginIndex) * sizeof (CHAR16));
        *(StrTemp + (EndIndex - BeginIndex)) = L'\0';

        if (Ip4Config2StrToIp (StrTemp, &((*PtrIpList)[IpIndex])) != EFI_SUCCESS) {
          FreePool (StrTemp);
          FreePool (*PtrIpList);
          *PtrIpList = NULL;
          *IpCount   = 0;
          return EFI_INVALID_PARAMETER;
        }

        BeginIndex = EndIndex;
        IpIndex++;

        FreePool (StrTemp);
      }

      BeginIndex++;
      EndIndex++;
      SpaceTag = TRUE;
    } else {
      EndIndex++;
      SpaceTag = FALSE;
    }

    Index++;

    if (*(Str + Index) == L'\0') {
      if (!SpaceTag) {
        StrTemp = AllocateZeroPool ((EndIndex - BeginIndex + 1) * sizeof (CHAR16));
        if (StrTemp == NULL) {
          FreePool (*PtrIpList);
          *PtrIpList = NULL;
          *IpCount   = 0;
          return EFI_OUT_OF_RESOURCES;
        }

        CopyMem (StrTemp, Str + BeginIndex, (EndIndex - BeginIndex) * sizeof (CHAR16));
        *(StrTemp + (EndIndex - BeginIndex)) = L'\0';

        if (Ip4Config2StrToIp (StrTemp, &((*PtrIpList)[IpIndex])) != EFI_SUCCESS) {
          FreePool (StrTemp);
          FreePool (*PtrIpList);
          *PtrIpList = NULL;
          *IpCount   = 0;
          return EFI_INVALID_PARAMETER;
        }

        FreePool (StrTemp);
      }
    }
  }

  return EFI_SUCCESS;
}
