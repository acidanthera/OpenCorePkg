/**
 * \file ProBookWifiWhlistOff.c
 * Implement PCI memory space hack based on ProBook DSDT reading.
 *
 * HP ProBook 4330s, 4530s, and 4730s models only:
 * running this code on any other laptop might brick the device.
 */

/*-
 * Copyright (c) 2012 RehabMan
 *               2023 Ubihazard
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * This PCI memory hack was also (independently?) discovered by Milksnot:
 * https://web.archive.org/web/20190319081929/http://milksnot.com/content/project-dirty-laundry-how-defeat-whitelisting-without-bios-modding
 */

//
// Common UEFI includes and library classes.
//
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

#include <IndustryStandard/HdaVerbs.h>

#include "ProBookWifiWhlistOff.h"

// ---

#define true  1
#define false 0

typedef unsigned char byte;

// ---

EFI_STATUS
EFIAPI
ProBookWifiWhlistOffEntry (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
)
{
  // BIOS whitelist hack below is based on this bit
  // of info in ProBook DSDT:
  //
  //   OperationRegion (RCRB, SystemMemory, 0xFED1C000, 0x4000)
  //   Field (RCRB, DWordAcc, Lock, Preserve)
  //   {
  //     Offset (0x1A8),
  //     APMC,   2,
  //     Offset (0x1000),
  //     Offset (0x3000),
  //     Offset (0x3404),
  //     HPAS,   2,
  //         ,   5,
  //     HPAE,   1,
  //     Offset (0x3418),
  //         ,   1,
  //         ,   1,
  //     SATD,   1,
  //     SMBD,   1,
  //     HDAD,   1,
  //     Offset (0x341A),
  //     RP1D,   1,
  //     RP2D,   1,
  //     RP3D,   1,
  //     RP4D,   1, //! target bit (set to 1 to disable PCIe card in that slot)
  //     RP5D,   1,
  //     RP6D,   1,
  //     RP7D,   1,
  //     RP8D,   1
  //   }
  //
  // The code below writes zero to RP4D:
  // bit 3 at address 0xFED1C000 + 0x341A.

  *((byte*)0xFED1F41A) &= (byte)((1 << 3) ^ 0xFF);

  return EFI_SUCCESS;
}
