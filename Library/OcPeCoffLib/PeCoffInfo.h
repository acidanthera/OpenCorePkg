/** @file
  Provides APIs to retrieve general information about PE/COFF Images.

  Copyright (c) 2020, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef PE_COFF_INFO_H
#define PE_COFF_INFO_H

//
// TODO: Provide annotations for optional features.
//

UINT32
PeCoffGetEntryPoint (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  );

UINT16
PeCoffGetMachineType (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  );

UINT16
PeCoffGetSubsystem (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  );

UINT32
PeCoffGetSectionAlignment (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  );

UINT32
PeCoffGetSizeOfImage (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  );

UINT64
PeCoffGetImageBase (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  );

UINT32
PeCoffGetSizeOfHeaders (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  );

#endif // PE_COFF_INFO_H
