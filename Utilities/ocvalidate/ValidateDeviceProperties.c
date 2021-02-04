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

/**
  Callback function to verify whether one entry is duplicated in DeviceProperties->Add.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
DevPropsAddHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  CONST OC_STRING                      *DevPropsAddPrimaryEntry;
  CONST OC_STRING                      *DevPropsAddSecondaryEntry;
  CONST CHAR8                          *DevPropsAddPrimaryDevicePathString;
  CONST CHAR8                          *DevPropsAddSecondaryDevicePathString;

  DevPropsAddPrimaryEntry              = *(CONST OC_STRING **) PrimaryEntry;
  DevPropsAddSecondaryEntry            = *(CONST OC_STRING **) SecondaryEntry;
  DevPropsAddPrimaryDevicePathString   = OC_BLOB_GET (DevPropsAddPrimaryEntry);
  DevPropsAddSecondaryDevicePathString = OC_BLOB_GET (DevPropsAddSecondaryEntry);

  return StringIsDuplicated ("DeviceProperties->Add", DevPropsAddPrimaryDevicePathString, DevPropsAddSecondaryDevicePathString);
}

/**
  Callback function to verify whether one entry is duplicated in DeviceProperties->Delete.

  @param[in]  PrimaryEntry    Primary entry to be checked.
  @param[in]  SecondaryEntry  Secondary entry to be checked.

  @retval     TRUE            If PrimaryEntry and SecondaryEntry are duplicated.
**/
STATIC
BOOLEAN
DevPropsDeleteHasDuplication (
  IN  CONST VOID  *PrimaryEntry,
  IN  CONST VOID  *SecondaryEntry
  )
{
  CONST OC_STRING                         *DevPropsDeletePrimaryEntry;
  CONST OC_STRING                         *DevPropsDeleteSecondaryEntry;
  CONST CHAR8                             *DevPropsDeletePrimaryDevicePathString;
  CONST CHAR8                             *DevPropsDeleteSecondaryDevicePathString;

  DevPropsDeletePrimaryEntry              = *(CONST OC_STRING **) PrimaryEntry;
  DevPropsDeleteSecondaryEntry            = *(CONST OC_STRING **) SecondaryEntry;
  DevPropsDeletePrimaryDevicePathString   = OC_BLOB_GET (DevPropsDeletePrimaryEntry);
  DevPropsDeleteSecondaryDevicePathString = OC_BLOB_GET (DevPropsDeleteSecondaryEntry);

  return StringIsDuplicated ("DeviceProperties->Delete", DevPropsDeletePrimaryDevicePathString, DevPropsDeleteSecondaryDevicePathString);
}

STATIC
UINT32
CheckDevicePropertiesAdd (
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

  ErrorCount = 0;
  UserDevProp = &Config->DeviceProperties;

  for (DeviceIndex = 0; DeviceIndex < UserDevProp->Add.Count; ++DeviceIndex) {
    AsciiDevicePath = OC_BLOB_GET (UserDevProp->Add.Keys[DeviceIndex]);
    
    if (!AsciiDevicePathIsLegal (AsciiDevicePath)) {
      DEBUG ((DEBUG_WARN, "DeviceProperties->Add[%u]->DevicePath is borked! Please check the information above!\n", DeviceIndex));
      ++ErrorCount;
    }

    PropertyMap = UserDevProp->Add.Values[DeviceIndex];
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

    //
    // Check duplicated properties in DeviceProperties->Add[N].
    //
    ErrorCount += FindArrayDuplication (
      PropertyMap->Keys,
      PropertyMap->Count,
      sizeof (PropertyMap->Keys[0]),
      DevPropsAddHasDuplication
      );
  }

  //
  // Check duplicated entries in DeviceProperties->Add.
  //
  ErrorCount += FindArrayDuplication (
    UserDevProp->Add.Keys,
    UserDevProp->Add.Count,
    sizeof (UserDevProp->Add.Keys[0]),
    DevPropsAddHasDuplication
    );

  return ErrorCount;
}

STATIC
UINT32
CheckDevicePropertiesDelete (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32                    ErrorCount;
  UINT32                    DeviceIndex;
  UINT32                    PropertyIndex;
  OC_DEV_PROP_CONFIG        *UserDevProp;
  CONST CHAR8               *AsciiDevicePath;
  CONST CHAR8               *AsciiProperty;

  ErrorCount = 0;
  UserDevProp = &Config->DeviceProperties;

  for (DeviceIndex = 0; DeviceIndex < UserDevProp->Delete.Count; ++DeviceIndex) {
    AsciiDevicePath = OC_BLOB_GET (UserDevProp->Delete.Keys[DeviceIndex]);
    
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

    //
    // Check duplicated properties in DeviceProperties->Delete[N].
    //
    ErrorCount += FindArrayDuplication (
      UserDevProp->Delete.Values[DeviceIndex]->Values,
      UserDevProp->Delete.Values[DeviceIndex]->Count,
      sizeof (UserDevProp->Delete.Values[DeviceIndex]->Values[0]),
      DevPropsDeleteHasDuplication
      );
  }

  //
  // Check duplicated entries in DeviceProperties->Delete.
  //
  ErrorCount += FindArrayDuplication (
    UserDevProp->Delete.Keys,
    UserDevProp->Delete.Count,
    sizeof (UserDevProp->Delete.Keys[0]),
    DevPropsDeleteHasDuplication
    );

  return ErrorCount;
}

UINT32
CheckDeviceProperties (
  IN  OC_GLOBAL_CONFIG  *Config
  )
{
  UINT32               ErrorCount;
  UINTN                Index;
  STATIC CONFIG_CHECK  DevicePropertiesCheckers[] = {
    &CheckDevicePropertiesAdd,
    &CheckDevicePropertiesDelete
  };

  DEBUG ((DEBUG_VERBOSE, "config loaded into %a!\n", __func__));

  ErrorCount  = 0;

  for (Index = 0; Index < ARRAY_SIZE (DevicePropertiesCheckers); ++Index) {
    ErrorCount += DevicePropertiesCheckers[Index] (Config);
  }

  return ReportError (__func__, ErrorCount);
}
