/** @file
  IPv4 NVRAM utilities.

  Code derived from:
   - NetworkPkg/Ip4Dxe/Ip4Config2Impl.c
   - NetworkPkg/Ip4Dxe/Ip4Config2Nv.c

  Copyright (c) 2024, Mike Beaton. All rights reserved.<BR>
  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "NetworkBootInternal.h"

STATIC
VOID
AddDataRecord (
  IN OUT IP4_CONFIG2_VARIABLE   *Variable,
  IN EFI_IP4_CONFIG2_DATA_TYPE  DataType,
  IN VOID                       *Data,
  IN UINTN                      DataSize,
  IN OUT CHAR8                  **Heap
  )
{
  *Heap -= DataSize;
  CopyMem (*Heap, Data, DataSize);

  Variable->DataRecord[Variable->DataRecordCount].Offset   = (UINT16)(*Heap - (CHAR8 *)Variable);
  Variable->DataRecord[Variable->DataRecordCount].DataSize = (UINT32)DataSize;
  Variable->DataRecord[Variable->DataRecordCount].DataType = DataType;

  ++Variable->DataRecordCount;
}

//
// Performs the same job as
// https://github.com/tianocore/edk2/blob/e99d532fd7224e68026543834ed9c0fe3cfaf88c/NetworkPkg/Ip4Dxe/Ip4Config2Impl.c#L303
// but in order to avoid initialising unnecessary data structures we work directly
// from the config data which is in fact acccepted there.
//
STATIC
EFI_STATUS
Ip4Config2WriteConfigData (
  IN CHAR16                       *VarName,
  EFI_IP4_CONFIG2_POLICY          Policy,
  EFI_IP4_CONFIG2_MANUAL_ADDRESS  *ManualAddress,
  EFI_IP_ADDRESS                  *Gateway,
  EFI_IPv4_ADDRESS                *DnsAddress,
  UINTN                           DnsCount
  )
{
  EFI_STATUS  Status;

  IP4_CONFIG2_VARIABLE  *Variable;
  UINTN                 VarSize;
  UINT16                DataRecordCount;
  CHAR8                 *Heap;

  DataRecordCount = 0;
  VarSize         = sizeof (*Variable);
  {
    ++DataRecordCount;
    VarSize += sizeof (Policy);
  }
  if (ManualAddress != NULL) {
    ++DataRecordCount;
    VarSize += sizeof (*ManualAddress);
  }

  if (Gateway != NULL) {
    ++DataRecordCount;
    VarSize += sizeof (Gateway->v4);
  }

  if (DnsAddress != NULL) {
    ASSERT (DnsCount != 0);
    ++DataRecordCount;
    VarSize += DnsCount * sizeof (*DnsAddress);
  }

  VarSize += sizeof (Variable->DataRecord[0]) * DataRecordCount;

  Variable = AllocatePool (VarSize);

  if (Variable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Heap = (CHAR8 *)Variable + VarSize;

  Variable->DataRecordCount = 0;

  AddDataRecord (Variable, Ip4Config2DataTypePolicy, &Policy, sizeof (Policy), &Heap);
  if (ManualAddress != NULL) {
    AddDataRecord (Variable, Ip4Config2DataTypeManualAddress, ManualAddress, sizeof (*ManualAddress), &Heap);
  }

  if (Gateway != NULL) {
    AddDataRecord (Variable, Ip4Config2DataTypeGateway, &Gateway->v4, sizeof (Gateway->v4), &Heap);
  }

  if (DnsAddress != NULL) {
    AddDataRecord (Variable, Ip4Config2DataTypeDnsServer, DnsAddress, sizeof (*DnsAddress) * DnsCount, &Heap);
  }

  ASSERT (Variable->DataRecordCount == DataRecordCount);
  ASSERT (Heap == (CHAR8 *)&Variable->DataRecord[DataRecordCount]);

  Variable->Checksum = 0;
  Variable->Checksum = (UINT16) ~NetblockChecksum ((UINT8 *)Variable, (UINT32)VarSize);

  Status = gRT->SetVariable (
                  VarName,
                  &gEfiIp4Config2ProtocolGuid,
                  IP4_CONFIG2_VARIABLE_ATTRIBUTE,
                  VarSize,
                  Variable
                  );

  FreePool (Variable);

  return Status;
}

EFI_STATUS
Ip4Config2ConvertOcConfigDataToNvData (
  IN CHAR16                      *VarName,
  IN IP4_CONFIG2_OC_CONFIG_DATA  *ConfigData
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  EFI_IP4_CONFIG2_POLICY          Policy;
  EFI_IP4_CONFIG2_MANUAL_ADDRESS  ManualAddress;
  EFI_IP_ADDRESS                  StationAddress;
  EFI_IP_ADDRESS                  SubnetMask;
  EFI_IP_ADDRESS                  Gateway;
  IP4_ADDR                        Ip;
  EFI_IPv4_ADDRESS                *DnsAddress;
  UINTN                           DnsCount;

  //
  // cf Ip4Config2ConvertIfrNvDataToConfigNvData
  // REF: https://github.com/tianocore/edk2/blob/e99d532fd7224e68026543834ed9c0fe3cfaf88c/NetworkPkg/Ip4Dxe/Ip4Config2Nv.c#L551
  //
  Policy = Ip4Config2PolicyStatic;

  Status = Ip4Config2StrToIp (ConfigData->SubnetMask, &SubnetMask.v4);
  if (EFI_ERROR (Status) || ((SubnetMask.Addr[0] != 0) && (GetSubnetMaskPrefixLength (&SubnetMask.v4) == 0))) {
    DEBUG ((DEBUG_WARN, "NETB: Invalid subnet mask %s\n", ConfigData->SubnetMask));
    return EFI_INVALID_PARAMETER;
  }

  Status = Ip4Config2StrToIp (ConfigData->StationAddress, &StationAddress.v4);
  if (EFI_ERROR (Status) ||
      ((SubnetMask.Addr[0] != 0) && !NetIp4IsUnicast (NTOHL (StationAddress.Addr[0]), NTOHL (SubnetMask.Addr[0]))) ||
      !Ip4StationAddressValid (NTOHL (StationAddress.Addr[0]), NTOHL (SubnetMask.Addr[0])))
  {
    DEBUG ((DEBUG_WARN, "NETB: Invalid IP address %s\n", ConfigData->StationAddress));
    return EFI_INVALID_PARAMETER;
  }

  Status = Ip4Config2StrToIp (ConfigData->GatewayAddress, &Gateway.v4);
  if (EFI_ERROR (Status) ||
      ((Gateway.Addr[0] != 0) && (SubnetMask.Addr[0] != 0) && !NetIp4IsUnicast (NTOHL (Gateway.Addr[0]), NTOHL (SubnetMask.Addr[0]))))
  {
    DEBUG ((DEBUG_WARN, "NETB: Invalid gateway %s\n", ConfigData->GatewayAddress));
    return EFI_INVALID_PARAMETER;
  }

  Status = Ip4Config2StrToIpList (ConfigData->DnsAddress, &DnsAddress, &DnsCount);
  if (!EFI_ERROR (Status) && (DnsCount > 0)) {
    for (Index = 0; Index < DnsCount; Index++) {
      CopyMem (&Ip, &DnsAddress[Index], sizeof (IP4_ADDR));
      if (IP4_IS_UNSPECIFIED (NTOHL (Ip)) || IP4_IS_LOCAL_BROADCAST (NTOHL (Ip))) {
        DEBUG ((
          DEBUG_WARN,
          "NETB: Invalid DNS address %d.%d.%d.%d\n",
          DnsAddress[Index].Addr[0],
          DnsAddress[Index].Addr[1],
          DnsAddress[Index].Addr[2],
          DnsAddress[Index].Addr[3]
          ));
        FreePool (DnsAddress);
        return EFI_INVALID_PARAMETER;
      }
    }
  } else {
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "NETB: Invalid DNS server list %s\n", ConfigData->DnsAddress));
      //
      // Unlike Ip4Config2ConvertIfrNvDataToConfigNvData we abort if unparseable DNS list.
      //
      return EFI_INVALID_PARAMETER;
    }
  }

  CopyMem (&ManualAddress.Address, &StationAddress.v4, sizeof (EFI_IPv4_ADDRESS));
  CopyMem (&ManualAddress.SubnetMask, &SubnetMask.v4, sizeof (EFI_IPv4_ADDRESS));

  Status = Ip4Config2WriteConfigData (VarName, Policy, &ManualAddress, &Gateway, DnsAddress, DnsCount);

  DEBUG ((
    DEBUG_INFO,
    "NETB: Adding %a %s - %r\n",
    "IPv4 NVRAM config for MAC address",
    VarName,
    Status
    ));

  if (DnsAddress != NULL) {
    FreePool (DnsAddress);
  }

  return Status;
}

//
// cf Ip4Config2ReadConfigData
// REF: https://github.com/tianocore/edk2/blob/e99d532fd7224e68026543834ed9c0fe3cfaf88c/NetworkPkg/Ip4Dxe/Ip4Config2Impl.c#L188
//
EFI_STATUS
Ip4Config2DeleteStaticIpNvData (
  IN     CHAR16  *VarName
  )
{
  EFI_STATUS               Status;
  UINTN                    VarSize;
  IP4_CONFIG2_VARIABLE     *Variable;
  UINTN                    Index;
  IP4_CONFIG2_DATA_RECORD  *DataRecord;

  EFI_IP4_CONFIG2_POLICY  Policy;

  BOOLEAN  FoundStaticPolicy;
  BOOLEAN  FoundManualAddress;

  VarSize = 0;
  Status  = gRT->GetVariable (
                   VarName,
                   &gEfiIp4Config2ProtocolGuid,
                   NULL,
                   &VarSize,
                   NULL
                   );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  Variable = AllocatePool (VarSize);
  if (Variable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gRT->GetVariable (
                  VarName,
                  &gEfiIp4Config2ProtocolGuid,
                  NULL,
                  &VarSize,
                  Variable
                  );
  if (EFI_ERROR (Status) || ((UINT16)(~NetblockChecksum ((UINT8 *)Variable, (UINT32)VarSize)) != 0)) {
    FreePool (Variable);

    //
    // Unlike Ip4Config2ReadConfigData it is not our job to resolve existing problematic variable,
    // just warn and return EFI_NOT_FOUND.
    //
    DEBUG ((
      DEBUG_INFO,
      "NETB: %a, %a %a %s - %r\n",
      "Invalid variable",
      "not removing",
      "IPv4 NVRAM config for MAC address",
      VarName,
      Status
      ));

    return EFI_NOT_FOUND;
  }

  //
  // In order to avoid fighting with PXE and HTTP boot over the variable value,
  // only remove valid static IP settings.
  //
  FoundStaticPolicy  = FALSE;
  FoundManualAddress = FALSE;

  for (Index = 0; Index < Variable->DataRecordCount; Index++) {
    DataRecord = &Variable->DataRecord[Index];
    if (  (DataRecord->DataType == Ip4Config2DataTypePolicy)
       && (DataRecord->DataSize == sizeof (Policy)))
    {
      CopyMem (&Policy, (CHAR8 *)Variable + DataRecord->Offset, DataRecord->DataSize);
      if (Policy == Ip4Config2PolicyStatic) {
        FoundStaticPolicy = TRUE;
      }
    } else if (  (DataRecord->DataType == Ip4Config2DataTypeManualAddress)
              && (DataRecord->DataSize == sizeof (EFI_IP4_CONFIG2_MANUAL_ADDRESS)))
    {
      FoundManualAddress = TRUE;
    }
  }

  if (FoundStaticPolicy && FoundManualAddress) {
    //
    // We may be called after Ip4Dxe has run (e.g. OpenNetworkBoot loaded by OpenCore, with native network stack).
    // Since this variable is boot services access only we should not just delete it, as this may leave one boot
    // where the OS could set it. Instead reinitialise the variable to the value to which Ip4Dxe initialises it.
    // For initial values, see:
    // https://github.com/tianocore/edk2/blob/e99d532fd7224e68026543834ed9c0fe3cfaf88c/NetworkPkg/Ip4Dxe/Ip4Config2Impl.c#L1985-L2015
    //
    Policy = Ip4Config2PolicyStatic;
    Status = Ip4Config2WriteConfigData (VarName, Policy, NULL, NULL, NULL, 0);

    DEBUG ((
      DEBUG_INFO,
      "NETB: Removing %a %s - %r\n",
      "IPv4 NVRAM config for MAC address",
      VarName,
      Status
      ));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "NETB: %a, %a %a %s\n",
      "No static IP",
      "not removing",
      "IPv4 NVRAM config for MAC address",
      VarName
      ));
    Status = EFI_NOT_FOUND;
  }

  FreePool (Variable);
  return Status;
}
