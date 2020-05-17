/*++

Copyright (c) 2005 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PciEnumerator.c
  
Abstract:

  PCI Bus Driver

Revision History

--*/

#include "PciBus.h"

EFI_STATUS
PciEnumerator (
  IN EFI_HANDLE                    Controller
  )
/*++

Routine Description:

  This routine is used to enumerate entire pci bus system 
  in a given platform

Arguments:

Returns:

  None

--*/
{
  //
  // This PCI bus driver depends on the legacy BIOS
  // to do the resource allocation
  //
  gFullEnumeration = FALSE;

  return PciEnumeratorLight (Controller) ;
  
}




