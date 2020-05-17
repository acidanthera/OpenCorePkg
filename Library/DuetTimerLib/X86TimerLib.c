/** @file
  Timer Library functions built upon ACPI on IA32/x64.
  
  ACPI power management timer is a 24-bit or 32-bit fixed rate free running count-up
  timer that runs off a 3.579545 MHz clock. 
  When startup, Duet will check the FADT to determine whether the PM timer is a 
  32-bit or 24-bit timer.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Guid/AcpiDescription.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>

EFI_ACPI_DESCRIPTION *gAcpiDesc          = NULL;

/**
  Internal function to get Acpi information from HOB.
  
  @return Pointer to ACPI description structure.
**/
EFI_ACPI_DESCRIPTION*
InternalGetApciDescrptionTable (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS  GuidHob;
  
  if (gAcpiDesc != NULL) {
    return gAcpiDesc;
  }
  
  GuidHob.Raw = GetFirstGuidHob (&gEfiAcpiDescriptionGuid);
  if (GuidHob.Raw != NULL) {
    gAcpiDesc = GET_GUID_HOB_DATA (GuidHob.Guid);
    DEBUG ((EFI_D_INFO, "ACPI Timer: PM_TMR_BLK.RegisterBitWidth = 0x%X\n", gAcpiDesc->PM_TMR_BLK.RegisterBitWidth));
    DEBUG ((EFI_D_INFO, "ACPI Timer: PM_TMR_BLK.Address = 0x%X\n", gAcpiDesc->PM_TMR_BLK.Address));
    return gAcpiDesc;
  } else {
    DEBUG ((EFI_D_ERROR, "Fail to get Acpi description table from hob\n"));
    return NULL;
  }
}

/**
  Internal function to read the current tick counter of ACPI.

  @return The tick counter read.

**/
STATIC
UINT32
InternalAcpiGetTimerTick (
  VOID
  )
{
  return IoRead32 ((UINTN)gAcpiDesc->PM_TMR_BLK.Address);
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
STATIC
VOID
InternalAcpiDelay (
  IN      UINT32                    Delay
  )
{
  UINT32                            Ticks;
  UINT32                            Times;

  Times    = Delay >> (gAcpiDesc->PM_TMR_BLK.RegisterBitWidth - 2);
  Delay   &= (1 << (gAcpiDesc->PM_TMR_BLK.RegisterBitWidth - 2)) - 1;
  do {
    //
    // The target timer count is calculated here
    //
    Ticks    = InternalAcpiGetTimerTick () + Delay;
    Delay    = 1 << (gAcpiDesc->PM_TMR_BLK.RegisterBitWidth - 2);
    //
    // Wait until time out
    // Delay >= 2^23 (if ACPI provide 24-bit timer) or Delay >= 2^31 (if ACPI
    // provide 32-bit timer) could not be handled by this function
    // Timer wrap-arounds are handled correctly by this function
    //
    while (((Ticks - InternalAcpiGetTimerTick ()) & (1 << (gAcpiDesc->PM_TMR_BLK.RegisterBitWidth - 1))) == 0) {
      CpuPause ();
    }
  } while (Times-- > 0);
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{

  if (InternalGetApciDescrptionTable() == NULL) {
    return MicroSeconds;
  }
 
  InternalAcpiDelay (
    (UINT32)DivU64x32 (
              MultU64x32 (
                MicroSeconds,
                3579545
                ),
              1000000u
              )
    );
  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return NanoSeconds

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN                     NanoSeconds
  )
{
  if (InternalGetApciDescrptionTable() == NULL) {
    return NanoSeconds;
  }
  
  InternalAcpiDelay (
    (UINT32)DivU64x32 (
              MultU64x32 (
                NanoSeconds,
                3579545
                ),
              1000000000u
              )
    );
  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  Retrieves the current value of a 64-bit free running performance counter. The
  counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  if (InternalGetApciDescrptionTable() == NULL) {
    return 0;
  }
  
  return (UINT64)InternalAcpiGetTimerTick ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the performance counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the performance counter in Hz is always returned. If StartValue
  is less than EndValue, then the performance counter counts up. If StartValue
  is greater than EndValue, then the performance counter counts down. For
  example, a 64-bit free running counter that counts up would have a StartValue
  of 0 and an EndValue of 0xFFFFFFFFFFFFFFFF. A 24-bit free running counter
  that counts down would have a StartValue of 0xFFFFFF and an EndValue of 0.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (InternalGetApciDescrptionTable() == NULL) {
    return 0;
  }
  
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = (1 << gAcpiDesc->PM_TMR_BLK.RegisterBitWidth) - 1;
  }

  return 3579545;
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN      UINT64                     Ticks
  )
{
  UINT64  NanoSeconds;
  UINT32  Remainder;

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x32Remainder (Ticks, 3579545, &Remainder), 1000000000u);

  //
  // Frequency < 0x100000000, so Remainder < 0x100000000, then (Remainder * 1,000,000,000)
  // will not overflow 64-bit.
  //
  NanoSeconds += DivU64x32 (MultU64x32 ((UINT64) Remainder, 1000000000u), 3579545);

  return NanoSeconds;
}
