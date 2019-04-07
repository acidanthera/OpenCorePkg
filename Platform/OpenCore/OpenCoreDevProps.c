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
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathPropertyDatabase.h>

VOID
OcLoadDevPropsSupport (
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  EFI_STATUS                                  Status;
  UINT32                                      DeviceIndex;
  UINT32                                      PropertyIndex;
  EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *PropertyDatabase;
  OC_ASSOC                                    *PropertyMap;
  CHAR8                                       *AsciiDevicePath;
  CHAR16                                      *UnicodeDevicePath;
  CHAR8                                       *AsciiProperty;
  CHAR16                                      *UnicodeProperty;
  EFI_DEVICE_PATH_PROTOCOL                    *DevicePath;

  PropertyDatabase = OcDevicePathPropertyInstallProtocol (Config->DeviceProperties.ReinstallProtocol);
  if (PropertyDatabase == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Device property database protocol is missing\n"));
    return;
  }

  for (DeviceIndex = 0; DeviceIndex < Config->DeviceProperties.Add.Count; ++DeviceIndex) {
    PropertyMap       = Config->DeviceProperties.Add.Values[DeviceIndex];
    AsciiDevicePath   = OC_BLOB_GET (Config->DeviceProperties.Add.Keys[DeviceIndex]);
    UnicodeDevicePath = AsciiStrCopyToUnicode (AsciiDevicePath, 0);
    DevicePath        = NULL;

    if (UnicodeDevicePath != NULL) {
      DevicePath = ConvertTextToDevicePath (UnicodeDevicePath);
      FreePool (UnicodeDevicePath);
    }

    if (DevicePath == NULL) {
      DEBUG ((DEBUG_WARN, "OC: Failed to parse %a device path\n", DevicePath));
      continue;
    }

    for (PropertyIndex = 0; PropertyIndex < PropertyMap->Count; ++PropertyIndex) {
      AsciiProperty   = OC_BLOB_GET (PropertyMap->Keys[PropertyIndex]);
      UnicodeProperty = AsciiStrCopyToUnicode (AsciiProperty, 0);

      if (UnicodeProperty == NULL) {
        DEBUG ((DEBUG_WARN, "OC: Failed to convert %a property\n", AsciiProperty));
        continue;
      }

      Status = PropertyDatabase->SetProperty (
        PropertyDatabase,
        DevicePath,
        UnicodeProperty,
        OC_BLOB_GET (PropertyMap->Values[PropertyIndex]),
        PropertyMap->Values[PropertyIndex]->Size
        );

      FreePool (UnicodeProperty);

      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OC: Failed to set %a property - %r\n", AsciiProperty, Status));
      }
    }

    FreePool (DevicePath);
  }
}
