/** @file
  Copyright (C) 2018, vit9696. All rights reserved.
  Copyright (C) 2020, PMheart. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "ocvalidate.h"
#include "OcValidateLib.h"

UINT32
CheckDeviceProperties (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32                    ErrorCount;
  UINT32                    DeviceIndex;
  UINT32                    PropertyIndex;
  OC_DEV_PROP_CONFIG        *UserDevProp;
  CONST CHAR8               *AsciiDevicePath;
  CONST CHAR8               *AsciiProperty;
  OC_ASSOC                  *PropertyMap;

  DEBUG ((DEBUG_VERBOSE, "config loaded into DeviceProperties checker!\n"));

  ErrorCount  = 0;
  UserDevProp = &Config->DeviceProperties;

  for (DeviceIndex = 0; DeviceIndex < UserDevProp->Delete.Count; ++DeviceIndex) {
    AsciiDevicePath   = OC_BLOB_GET (UserDevProp->Delete.Keys[DeviceIndex]);
    
    if (!AsciiDevicePathIsLegal (AsciiDevicePath)) {
      DEBUG ((DEBUG_WARN, "DeviceProperties->Delete[%u]->DevicePath is borked! Please check the information above!\n", DeviceIndex));
      ++ErrorCount;
    }

    for (PropertyIndex = 0; PropertyIndex < UserDevProp->Delete.Values[DeviceIndex]->Count; ++PropertyIndex) {
      AsciiProperty = OC_BLOB_GET (UserDevProp->Delete.Values[DeviceIndex]->Values[PropertyIndex]);

      //
      // Sanitise strings.
      //
      if (!AsciiPropertyIsLegal (AsciiProperty)) {
        DEBUG ((
          DEBUG_WARN,
          "DeviceProperties->Delete[%u]->Property[%u] contains illegal character!\n",
          DeviceIndex,
          PropertyIndex
          ));
        ++ErrorCount;
      }
    }
  }

  for (DeviceIndex = 0; DeviceIndex < UserDevProp->Add.Count; ++DeviceIndex) {
    PropertyMap       = UserDevProp->Add.Values[DeviceIndex];
    AsciiDevicePath   = OC_BLOB_GET (UserDevProp->Add.Keys[DeviceIndex]);
    
    if (!AsciiDevicePathIsLegal (AsciiDevicePath)) {
      DEBUG ((DEBUG_WARN, "DeviceProperties->Add[%u]->DevicePath is borked! Please check the information above!\n", DeviceIndex));
      ++ErrorCount;
    }

    for (PropertyIndex = 0; PropertyIndex < PropertyMap->Count; ++PropertyIndex) {
      AsciiProperty = OC_BLOB_GET (PropertyMap->Keys[PropertyIndex]);

      //
      // Sanitise strings.
      //
      if (!AsciiPropertyIsLegal (AsciiProperty)) {
        DEBUG ((
          DEBUG_WARN,
          "DeviceProperties->Add[%u]->Property[%u] contains illegal character!\n",
          DeviceIndex,
          PropertyIndex
          ));
        ++ErrorCount;
      }
    }
  }

  return ReportError (__func__, ErrorCount);
}
