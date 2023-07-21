/**
 * \file ProBookFanReset.c
 * Overwrite embedded controller CPU fan register to automatic mode,
 * resetting the fan to factory behavior at system startup.
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

/*
 * RehabMan:
 *
 * The specific code to read and write to the EC was copied and modified from
 * original code written to control the fan on Thinkpads.
 *
 * I modified it to compile/run using the Mac's Xcode Tools.
 *
 * I could not find a specific open-source license provided by that code, so
 * my version here is released under GPLv2.
 *
 * The original source is here: http://sourceforge.net/projects/tp4xfancontrol/
 */

/*
 * Ubihazard:
 *
 * Port to OpenCore bootloader.
 */

//
// Common UEFI includes and library classes.
//
#include <Base.h>
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

#include "ProBookFanReset.h"

// ---

#define DEBUG_ME 0

#if DEBUG_ME
  #define DEBUGLOG(...) do { DEBUG((DEBUG_INFO, __VA_ARGS__)); } while (0)
#else
  #define DEBUGLOG(...) do {} while (0)
#endif

#define RT_INLINE_ASM_GNU_STYLE 1

#define true  1
#define false 0

typedef unsigned char byte;

//-------------------------------------------------------------------------
//  asm inline port io
//-------------------------------------------------------------------------

static inline void ASMOutU8(int Port, byte u8)
{
  __asm__ __volatile__("outb %b1, %w0\n\t"
             :: "Nd" (Port),
             "a" (u8));
}

static inline byte ASMInU8(int Port)
{
  byte u8;
  __asm__ __volatile__("inb %w1, %b0\n\t"
             : "=a" (u8)
             : "Nd" (Port));
  return u8;
}

#define inb(port) ASMInU8((port))
#define outb(port, val) ASMOutU8((port), (val))

//-------------------------------------------------------------------------
//  delay by milliseconds
//-------------------------------------------------------------------------
static void delay(int tick)
{
  // does not work... MicroSecondDelay(1000*tick);
  gBS->Stall(1000*tick);
}

//-------------------------------------------------------------------------
//  read control port and wait for set/clear of a status bit
//-------------------------------------------------------------------------
int waitportstatus(int mask, int wanted)
{
  int timeout = 1000;
  int tick = 10;
  int port = EC_CTRLPORT;
  // wait until input on control port has desired state or times out
  int time;
  for (time = 0; time < timeout; time += tick)
  {
    byte data = (byte)inb(port);
    // check for desired result
    if (wanted == (data & mask))
      return true;
    // try again after a moment
    delay(tick);
  }
  return false;
}

//-------------------------------------------------------------------------
//  write a character to an io port through WinIO device
//-------------------------------------------------------------------------
int writeport(int port, byte data)
{
  // write byte
  outb(port, data);
  return true;
}

//-------------------------------------------------------------------------
//  read a character from an io port through WinIO device
//-------------------------------------------------------------------------
int readport(int port, byte *pdata)
{
  // read byte
  byte data = inb(port);
  *pdata = data;
  return true;
}

//-------------------------------------------------------------------------
//  read a byte from the embedded controller (EC) via port io
//-------------------------------------------------------------------------
int ReadByteFromEC(int offset, byte *pdata)
{
  int ok;

  // wait for IBF and OBF to clear
  ok = waitportstatus(EC_STAT_IBF|EC_STAT_OBF, 0);
  if (!ok) return false;

  // tell 'em we want to "READ"
  ok = writeport(EC_CTRLPORT, EC_CTRLPORT_READ);
  if (!ok) return false;

  // wait for IBF to clear (command byte removed from EC's input queue)
  ok = waitportstatus(EC_STAT_IBF, 0);
  if (!ok) return false;

  // tell 'em where we want to read from
  ok = writeport(EC_DATAPORT, offset);
  if (!ok) return false;

  // wait for IBF to clear (address byte removed from EC's input queue)
  // Note: Techically we should waitportstatus(IBF|OBF,OBF) here. (a byte
  //  being in the EC's output buffer being ready to read). For some reason
  //  this never seems to happen
  ok = waitportstatus(EC_STAT_IBF, 0);
  if (!ok) return false;

  // read result (EC byte at offset)
  byte data;
  ok = readport(EC_DATAPORT, &data);
  if (ok) *pdata= data;

  return ok;
}

//-------------------------------------------------------------------------
//  write a byte to the embedded controller (EC) via port io
//-------------------------------------------------------------------------
int WriteByteToEC(int offset, byte data)
{
  int ok;

  // wait for IBF and OBF to clear
  ok = waitportstatus(EC_STAT_IBF| EC_STAT_OBF, 0);
  if (!ok)
  {
    DEBUGLOG("HPFanReset:WriteByteToEC(1): waitportstatus IBF|OBF didn't clear\n");
    return false;
  }

  // tell 'em we want to "WRITE"
  ok = writeport(EC_CTRLPORT, EC_CTRLPORT_WRITE);
  if (!ok)
  {
    DEBUGLOG("HPFanReset:WriteByteToEC(2): writeport EC_CTRLPORT failed\n");
    return false;
  }

  // wait for IBF to clear (command byte removed from EC's input queue)
  ok = waitportstatus(EC_STAT_IBF, 0);
  if (!ok)
  {
    DEBUGLOG("HPFanReset:WriteByteToEC(3): waitportstatus IBF didn't clear\n");
    return false;
  }

  // tell 'em where we want to write to
  ok = writeport(EC_DATAPORT, offset);
  if (!ok)
  {
    DEBUGLOG("HPFanReset:WriteByteToEC(4): writeport EC_DATAPORT offset failed\n");
    return false;
  }

  // wait for IBF to clear (address byte removed from EC's input queue)
  ok = waitportstatus(EC_STAT_IBF, 0);
  if (!ok)
  {
    DEBUGLOG("HPFanReset:WriteByteToEC(5): waitportstatus IBF didn't clear\n");
    return false;
  }

  // tell 'em what we want to write there
  ok = writeport(EC_DATAPORT, data);
  if (!ok)
  {
    DEBUGLOG("HPFanReset:WriteByteToEC(6): writeport EC_DATAPORT data\n");
    return false;
  }

  // wait for IBF to clear (data byte removed from EC's input queue)
  ok = waitportstatus(EC_STAT_IBF, 0);
  if (!ok)
  {
    DEBUGLOG("HPFanReset:WriteByteToEC(7): waitportstatus IBF didn't clear\n");
    return false;
  }
  return ok;
}

//-------------------------------------------------------------------------
//  reset FAN control EC registers for HP ProBook 4x30s
//-------------------------------------------------------------------------
int HPFanReset()
{
  // first get rid of fake temp setting
  int result = WriteByteToEC(EC_ZONE_SEL, 1); // select CPU zone
  DEBUGLOG("HPFanReset:Fanreset_start: EC_ZONE_SEL result = %d\n", result);
  if (!result)
    return false;

  result = WriteByteToEC(EC_ZONE_TEMP, 0); // zero causes fake temp to reset
  DEBUGLOG("HPFanReset:Fanreset_start: EC_ZONE_TEMP result = %d\n", result);
  if (!result)
    return false;

  // next set fan to automatic
  result = WriteByteToEC(EC_FAN_CONTROL, 0xFF); // 0xFF is "automatic mode"
  DEBUGLOG("HPFanReset:Fanreset_start: EC_FAN_CONTROL result = %d\n", result);
  if (!result)
    return false;

  return true;
}

EFI_STATUS
EFIAPI
ProBookFanResetEntry (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
)
{
  DEBUGLOG("HPFanReset: entry point called\n");

  if (HPFanReset())
    DEBUGLOG("HPFanReset: successfully set fan control to BIOS mode.\n");
  else
    DEBUGLOG("HPFanReset: Error! Unable to set fan control to BIOS mode.\n");

  return EFI_SUCCESS;
}
