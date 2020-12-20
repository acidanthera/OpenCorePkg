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

#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

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
  CHAR16                    *UnicodeDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *TextualDevicePath;
  CONST CHAR8               *AsciiProperty;
  OC_ASSOC                  *PropertyMap;

  DEBUG ((DEBUG_VERBOSE, "config loaded into DeviceProperties checker!\n"));

  ErrorCount  = 0;
  UserDevProp = &Config->DeviceProperties;

  for (DeviceIndex = 0; DeviceIndex < UserDevProp->Delete.Count; ++DeviceIndex) {
    AsciiDevicePath   = OC_BLOB_GET (UserDevProp->Delete.Keys[DeviceIndex]);
    DevicePath        = NULL;
    TextualDevicePath = NULL;

    //
    // Sanitise strings.
    //
    if (!AsciiDevicePathIsLegal (AsciiDevicePath)) {
      DEBUG ((DEBUG_WARN, "DeviceProperties->Delete[%u] contains illegal character!\n", DeviceIndex));
      ++ErrorCount;
      //
      // If even illegal by an entry check, this one must be borked. Skipping such.
      //
      continue;
    }

    //
    // Convert ASCII device path to Unicode format.
    //
    UnicodeDevicePath = AsciiStrCopyToUnicode (AsciiDevicePath, 0);
    if (UnicodeDevicePath != NULL) {
      //
      // Firstly, convert Unicode device path to binary.
      //
      DevicePath        = ConvertTextToDevicePath (UnicodeDevicePath);
      //
      // Secondly, convert binary back to Unicode device path.
      //
      TextualDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
      FreePool (DevicePath);

      if (TextualDevicePath != NULL) {
        //
        // If the results do not match, then the original device path is borked.
        //
        if (OcStriCmp (UnicodeDevicePath, TextualDevicePath) != 0) {
          DEBUG ((DEBUG_WARN, "DeviceProperties->Delete[%u] is borked!\n", DeviceIndex));
          ++ErrorCount;
        }
      }

      FreePool (UnicodeDevicePath);
      FreePool (TextualDevicePath);
    }

    for (PropertyIndex = 0; PropertyIndex < UserDevProp->Delete.Values[DeviceIndex]->Count; ++PropertyIndex) {
      AsciiProperty = OC_BLOB_GET (UserDevProp->Delete.Values[DeviceIndex]->Values[PropertyIndex]);

      //
      // Sanitise strings.
      //
      if (!AsciiDevicePropertyIsLegal (AsciiProperty)) {
        DEBUG ((
          DEBUG_WARN,
          "DeviceProperties->Delete[%u]->%a contains illegal character!\n",
          DeviceIndex,
          AsciiProperty
          ));
        ++ErrorCount;
      }
    }
  }

  for (DeviceIndex = 0; DeviceIndex < UserDevProp->Add.Count; ++DeviceIndex) {
    PropertyMap       = UserDevProp->Add.Values[DeviceIndex];
    AsciiDevicePath   = OC_BLOB_GET (UserDevProp->Add.Keys[DeviceIndex]);
    DevicePath        = NULL;
    TextualDevicePath = NULL;

    //
    // Sanitise strings.
    //
    if (!AsciiDevicePathIsLegal (AsciiDevicePath)) {
      DEBUG ((DEBUG_WARN, "DeviceProperties->Add[%u] contains illegal character!\n", DeviceIndex));
      ++ErrorCount;
      //
      // If even illegal by an entry check, this one must be borked. Skipping such.
      //
      continue;
    }

    //
    // Convert ASCII device path to Unicode format.
    //
    UnicodeDevicePath = AsciiStrCopyToUnicode (AsciiDevicePath, 0);
    if (UnicodeDevicePath != NULL) {
      //
      // Firstly, convert Unicode device path to binary.
      //
      DevicePath        = ConvertTextToDevicePath (UnicodeDevicePath);
      //
      // Secondly, convert binary back to Unicode device path.
      //
      TextualDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
      FreePool (DevicePath);

      if (TextualDevicePath != NULL) {
        //
        // If the results do not match, then the original device path is borked.
        //
        if (OcStriCmp (UnicodeDevicePath, TextualDevicePath) != 0) {
          DEBUG ((DEBUG_WARN, "DeviceProperties->Add[%u] is borked!\n", DeviceIndex));
          ++ErrorCount;
        }
      }
      
      FreePool (UnicodeDevicePath);
      FreePool (TextualDevicePath);
    }

    for (PropertyIndex = 0; PropertyIndex < PropertyMap->Count; ++PropertyIndex) {
      AsciiProperty = OC_BLOB_GET (PropertyMap->Keys[PropertyIndex]);

      //
      // Sanitise strings.
      //
      if (!AsciiDevicePropertyIsLegal (AsciiProperty)) { ///< MARK
        DEBUG ((
          DEBUG_WARN,
          "DeviceProperties->Add[%u]->%a contains illegal character!\n",
          DeviceIndex,
          AsciiProperty
          ));
        ++ErrorCount;
      }
    }
  }

  return ReportError (__func__, ErrorCount);
}
