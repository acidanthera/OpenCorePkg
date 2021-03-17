/** @file
  Copyright (C) 2021, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/OcCpuLib.h>
#include <Library/DebugLib.h>

#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct FREQUENCY_TEST_ {
  UINT64  FrequencyHz;
  UINT16  FrequencyMHz;
  CONST CHAR8 *Model;
} FREQUENCY_TEST;

int main() {
  STATIC FREQUENCY_TEST mTests[] = {
    { 3506084610 /* Hz */, 3500 /* MHz, Raw 3506 MHz */, "i7 4770K" },
    { 3324999575 /* Hz */, 3330 /* MHz, Raw 3325 MHz */, "Xeon X5680" },
    { 3324999977 /* Hz */, 3330 /* MHz, Raw 3325 MHz */, "Xeon X5680" },
    { 3457999888 /* Hz */, 3460 /* MHz, Raw 3458 MHz */, "Xeon X5690" },
    { 2666362112 /* Hz */, 2660 /* MHz, Raw 2666 MHz */, "C2Q Q9450" },
    { 3058999712 /* Hz */, 3060 /* MHz, Raw 3059 MHz */, "Xeon X5675" },
    { 1992617296 /* Hz */, 2000 /* MHz, Raw 1993 MHz */, "i7 2630QM" },
    { 3010680273 /* Hz */, 3000 /* MHz, Raw 3011 MHz */, "P4 530" },
    { 2389242546 /* Hz */, 2400 /* MHz, Raw 2389 MHz */, "C2D P8600" },
  };

  int code = 0;

  for (size_t i = 0; i < sizeof(mTests)/sizeof(mTests[0]); ++i) {
    UINT16 Read = OcCpuFrequencyToDisplayFrequency (mTests[i].FrequencyHz);
    if (Read != mTests[i].FrequencyMHz) {
      DEBUG ((
        DEBUG_WARN,
        "Frequency %04u instead if %04u for %a\n",
        Read,
        mTests[i].FrequencyMHz,
        mTests[i].Model
        ));
      code = 1;
    }
  }

  return code;
}
