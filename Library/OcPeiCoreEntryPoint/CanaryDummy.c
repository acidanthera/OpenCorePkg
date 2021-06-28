/** @file
  Copyright (C) 2021, ISP RAS. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <Library/PeiCoreEntryPoint.h>

/**
  The entry point of PE/COFF Image for the PEI Core.

  This function is the entry point for the PEI Foundation, which allows the SEC phase
  to pass information about the stack, temporary RAM and the Boot Firmware Volume.
  In addition, it also allows the SEC phase to pass services and data forward for use
  during the PEI phase in the form of one or more PPIs.
  There is no limit to the number of additional PPIs that can be passed from SEC into
  the PEI Foundation. As part of its initialization phase, the PEI Foundation will add
  these SEC-hosted PPIs to its PPI database such that both the PEI Foundation and any
  modules can leverage the associated service calls and/or code in these early PPIs.
  This function is required to call ProcessModuleEntryPointList() with the Context
  parameter set to NULL.  ProcessModuleEntryPoint() is never expected to return.
  The PEI Core is responsible for calling ProcessLibraryConstructorList() as soon as
  the PEI Services Table and the file handle for the PEI Core itself have been established.
  If ProcessModuleEntryPointList() returns, then ASSERT() and halt the system.

  @param SecCoreData  Points to a data structure containing information about the
                      PEI core's operating environment, such as the size and
                      location of temporary RAM, the stack location and the BFV
                      location.

  @param PpiList      Points to a list of one or more PPI descriptors to be
                      installed initially by the PEI core. An empty PPI list
                      consists of a single descriptor with the end-tag
                      EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST.
                      As part of its initialization phase, the PEI Foundation will
                      add these SEC-hosted PPIs to its PPI database, such that both
                      the PEI Foundation and any modules can leverage the associated
                      service calls and/or code in these early PPIs.

**/
VOID
EFIAPI
_ModuleEntryPointReal (
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList
  );

VOID
EFIAPI
_ModuleEntryPoint (
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList
  )
{
  _ModuleEntryPointReal (SecCoreData, PpiList);
}
