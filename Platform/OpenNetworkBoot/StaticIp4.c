/** @file
  Static IPv4 handling for HTTP Boot.
  Set and remove NVRAM vars in which Ip4Dxe stores per MAC(+VLAN) IP settings.

  Copyright (c) 2024, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "NetworkBootInternal.h"

typedef enum {
  MAC_STRING_MAC_ADDRESS,
  MAC_STRING_VLAN
} PARSE_MAC_ADDRESS_STATE;

#define STATIC4  L"static4:"

#define NORMAL_MAC_ADDRESS_DIGIT_COUNT  (NET_ETHER_ADDR_LEN * 2)
#define MAX_MAC_ADDRESS_DIGIT_COUNT     (sizeof (EFI_MAC_ADDRESS) * 2)
#define VLAN_DIGIT_COUNT                4

//
// Variable length hex digit MAC string with no separators, follow by optional backslash then 4 digit hex VLAN id string.
// REF: https://github.com/tianocore/edk2/blob/e99d532fd7224e68026543834ed9c0fe3cfaf88c/NetworkPkg/Library/DxeNetLib/DxeNetLib.c#L2363
//
STATIC
EFI_STATUS
ValidateMacString (
  CHAR16  *MacString
  )
{
  PARSE_MAC_ADDRESS_STATE  State;
  CHAR16                   *Walker;
  CHAR16                   Ch;
  UINTN                    Count;

  State  = MAC_STRING_MAC_ADDRESS;
  Count  = 0;
  Walker = MacString;

  for ( ; ;) {
    Ch = *Walker;
    switch (State) {
      case MAC_STRING_MAC_ADDRESS:
        if ((Ch == CHAR_NULL) || (Ch == L'\\')) {
          if (Count != NORMAL_MAC_ADDRESS_DIGIT_COUNT) {
            DEBUG ((
              DEBUG_INFO,
              "NETB: Non-standard MAC address length %d!=%d (invalid unless iSCSI)!\n",
              Count,
              NORMAL_MAC_ADDRESS_DIGIT_COUNT
              ));
          }

          if (Ch == CHAR_NULL) {
            return EFI_SUCCESS;
          }

          State = MAC_STRING_VLAN;
          Count = 0;
        }

        break;

      case MAC_STRING_VLAN:
        if (Ch == CHAR_NULL) {
          if (Count != VLAN_DIGIT_COUNT) {
            return EFI_INVALID_PARAMETER;
          }

          return EFI_SUCCESS;
        }

        break;
    }

    if (!(((Ch >= L'0') && (Ch <= L'9')) || ((Ch >= L'A') && (Ch <= L'F')) || ((Ch >= L'a') && (Ch <= L'f')))) {
      if ((Ch == ':') || (Ch == '-')) {
        DEBUG ((DEBUG_WARN, "NETB: Do not use : or - separators in %s{MAC}\n", STATIC4));
      }

      return EFI_INVALID_PARAMETER;
    }

    ++Walker;
    ++Count;

    if (Count > ((State == MAC_STRING_MAC_ADDRESS) ? MAX_MAC_ADDRESS_DIGIT_COUNT : VLAN_DIGIT_COUNT)) {
      return EFI_INVALID_PARAMETER;
    }
  }
}

//
// Normalise in separate pass to avoid users complaining 'that's not my MAC string' and to avoid
// showing half-converted MAC string in error messages.
//
STATIC
VOID
NormaliseMacString (
  CHAR16  *MacString
  )
{
  CHAR16  *Walker;
  CHAR16  Ch;

  for (Walker = MacString; (Ch = *Walker) != CHAR_NULL; ++Walker) {
    if ((Ch >= L'a') && (Ch <= L'f')) {
      //
      // All digits are upper cased.
      // REF: https://github.com/tianocore/edk2/blob/e99d532fd7224e68026543834ed9c0fe3cfaf88c/NetworkPkg/Library/DxeNetLib/DxeNetLib.c#L2363
      //
      *Walker -= (L'a' - L'A');
    }
  }
}

STATIC
CHAR16 *
GetNextIp4 (
  CHAR16  *CurrentIP,
  CHAR8   *RequiredName
  )
{
  CHAR16  *NextIP;

  NextIP = StrStr (CurrentIP, L",");
  if (NextIP == NULL) {
    if (RequiredName != NULL) {
      DEBUG ((DEBUG_WARN, "NETB: Missing required %a\n", RequiredName));
    }

    return NULL;
  }

  *NextIP = CHAR_NULL;

  return ++NextIP;
}

EFI_STATUS
AddRemoveStaticIPs (
  OC_FLEX_ARRAY  *ParsedLoadOptions
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  OC_PARSED_VAR               *Option;
  CHAR16                      *MacString;
  IP4_CONFIG2_OC_CONFIG_DATA  ConfigData;

  //
  // Find and apply static IP settings in driver arguments.
  //
  for (Index = 0; Index < ParsedLoadOptions->Count; ++Index) {
    Option = OcFlexArrayItemAt (ParsedLoadOptions, Index);

    if (OcUnicodeStartsWith (Option->Unicode.Name, STATIC4, FALSE)) {
      MacString = &Option->Unicode.Name[L_STR_LEN (STATIC4)];
      Status    = ValidateMacString (MacString);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "NETB: Invalid MAC %s - %r\n", MacString, Status));
        continue;
      }

      NormaliseMacString (MacString);
      if (Option->Unicode.Value == NULL) {
        Ip4Config2DeleteStaticIpNvData (MacString);
      } else {
        ConfigData.StationAddress = Option->Unicode.Value;
        ConfigData.SubnetMask     = GetNextIp4 (ConfigData.StationAddress, "subnet mask");
        if (ConfigData.SubnetMask == NULL) {
          return EFI_INVALID_PARAMETER;
        }

        ConfigData.GatewayAddress = GetNextIp4 (ConfigData.SubnetMask, "gateway");
        if (ConfigData.GatewayAddress == NULL) {
          return EFI_INVALID_PARAMETER;
        }

        ConfigData.DnsAddress = GetNextIp4 (ConfigData.GatewayAddress, NULL);
        Ip4Config2ConvertOcConfigDataToNvData (MacString, &ConfigData);
      }
    }
  }

  return EFI_SUCCESS;
}
