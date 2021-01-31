/** @file
  Implements APIs to retrieve general information about PE/COFF Images.

  Copyright (c) 2020, Marvin Häuser. All rights reserved.<BR>
  Copyright (c) 2020, Vitaly Cheptsov. All rights reserved.<BR>
  Copyright (c) 2020, ISP RAS. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

/** @file
  Provides the services to get the entry point to a PE/COFF image that has either been
  loaded into memory or is executing at it's linked address.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeCoffInternal.h"
#include "PeCoffInfo.h"

UINT32
PeCoffGetEntryPoint (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);
  ASSERT (Context->ImageBuffer != NULL);

  return Context->AddressOfEntryPoint;
}

UINT16
PeCoffGetMachineType (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->Machine;
}

UINT16
PeCoffGetSubsystem (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->Subsystem;
}

UINT32
PeCoffGetSectionAlignment (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->SectionAlignment;
}

UINT32
PeCoffGetSizeOfImage (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->SizeOfImage;
}

UINT64
PeCoffGetImageBase (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->ImageBase;
}

UINT32
PeCoffGetSizeOfHeaders (
  IN OUT PE_COFF_IMAGE_CONTEXT  *Context
  )
{
  ASSERT (Context != NULL);

  return Context->SizeOfHeaders;
}
