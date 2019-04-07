/** @file
  OpenCore driver.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenCore.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/OcCpuLib.h>
#include <Library/OcDataHubLib.h>
#include <Library/OcSmbiosLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
VOID
OcPlatformUpdateDataHub (
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo
  )
{
  OC_DATA_HUB_DATA  Data;
  EFI_GUID          Uuid;

  ZeroMem (&Data, sizeof (Data));

  if (OC_BLOB_GET (&Config->PlatformInfo.DataHub.PlatformName)[0] != '\0') {
    Data.PlatformName = OC_BLOB_GET (&Config->PlatformInfo.DataHub.PlatformName);
  }

  if (OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemProductName)[0] != '\0') {
    Data.SystemProductName = OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemProductName);
  }

  if (OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemSerialNumber)[0] != '\0') {
    Data.SystemSerialNumber = OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemSerialNumber);
  }

  if (Config->PlatformInfo.DataHub.SystemUuid.Size == GUID_STRING_LENGTH
    && !EFI_ERROR (AsciiStrToGuid (OC_BLOB_GET (&Config->PlatformInfo.DataHub.SystemUuid), &Uuid))) {
    Data.SystemUUID         = &Uuid;
  }

  if (OC_BLOB_GET (&Config->PlatformInfo.DataHub.BoardProduct)[0] != '\0') {
    Data.BoardProduct = OC_BLOB_GET (&Config->PlatformInfo.DataHub.BoardProduct);
  }

  Data.BoardRevision      = &Config->PlatformInfo.DataHub.BoardRevision[0];
  Data.StartupPowerEvents = &Config->PlatformInfo.DataHub.StartupPowerEvents;
  Data.InitialTSC         = &Config->PlatformInfo.DataHub.InitialTSC;

  if (Config->PlatformInfo.DataHub.FSBFrequency != 0) {
    Data.FSBFrequency     = &Config->PlatformInfo.DataHub.FSBFrequency;
  }

  if (Config->PlatformInfo.DataHub.ARTFrequency != 0) {
    Data.ARTFrequency     = &Config->PlatformInfo.DataHub.ARTFrequency;
  }

  if (Config->PlatformInfo.DataHub.DevicePathsSupported[0] != 0) {
    Data.DevicePathsSupported = &Config->PlatformInfo.DataHub.DevicePathsSupported[0];
  }

  if (Config->PlatformInfo.DataHub.SmcRevision[0] != 0
    || Config->PlatformInfo.DataHub.SmcRevision[1] != 0
    || Config->PlatformInfo.DataHub.SmcRevision[2] != 0
    || Config->PlatformInfo.DataHub.SmcRevision[3] != 0
    || Config->PlatformInfo.DataHub.SmcRevision[4] != 0
    || Config->PlatformInfo.DataHub.SmcRevision[5] != 0) {
    Data.SmcRevision      = &Config->PlatformInfo.DataHub.SmcRevision[0];
  }

  if (Config->PlatformInfo.DataHub.SmcBranch[0] != 0
    || Config->PlatformInfo.DataHub.SmcBranch[1] != 0
    || Config->PlatformInfo.DataHub.SmcBranch[2] != 0
    || Config->PlatformInfo.DataHub.SmcBranch[3] != 0
    || Config->PlatformInfo.DataHub.SmcBranch[4] != 0
    || Config->PlatformInfo.DataHub.SmcBranch[5] != 0
    || Config->PlatformInfo.DataHub.SmcBranch[6] != 0
    || Config->PlatformInfo.DataHub.SmcBranch[7] != 0) {
    Data.SmcBranch        = &Config->PlatformInfo.DataHub.SmcBranch[0];
  }

  if (Config->PlatformInfo.DataHub.SmcPlatform[0] != 0
    || Config->PlatformInfo.DataHub.SmcPlatform[1] != 0
    || Config->PlatformInfo.DataHub.SmcPlatform[2] != 0
    || Config->PlatformInfo.DataHub.SmcPlatform[3] != 0
    || Config->PlatformInfo.DataHub.SmcPlatform[4] != 0
    || Config->PlatformInfo.DataHub.SmcPlatform[5] != 0
    || Config->PlatformInfo.DataHub.SmcPlatform[6] != 0
    || Config->PlatformInfo.DataHub.SmcPlatform[7] != 0) {
    Data.SmcPlatform      = &Config->PlatformInfo.DataHub.SmcPlatform[0];
  }

  UpdateDataHub (&Data, CpuInfo);
}

VOID
OcLoadPlatformSupport (
  IN OC_GLOBAL_CONFIG    *Config,
  IN OC_CPU_INFO         *CpuInfo
  )
{
  //
  // TODO: implement
  //
  if (Config->PlatformInfo.Automatic) {
    DEBUG ((DEBUG_ERROR, "OC: Automatic platform information is unsupported\n"));
    return;
  }

  if (Config->PlatformInfo.UpdateDataHub) {
    OcPlatformUpdateDataHub (Config, CpuInfo);
  }
}
