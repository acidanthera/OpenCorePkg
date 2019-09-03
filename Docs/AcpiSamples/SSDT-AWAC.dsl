/*
 * For 300-series only. If you can't force enable Legacy RTC in BIOS GUI.
 * macOS does yet not support AWAC, so we have to force enable RTC. Do not use RTC ACPI patch.
 * 
 * The Time and Alarm device provides an alternative to the real time clock (RTC), which is defined as a fixed feature hardware device.
 * The wake timers allow the system to transition from the S3 (or optionally S4/S5) state to S0 state after a time period elapses.
 * In comparison with the Real Time Clock (RTC) Alarm, the Time and Alarm device provides a larger scale of flexibility in the operation of the wake timers,
 * and allows the implementation of the time source to be abstracted from the OSPM.
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "AWAC", 0x00000000)
{
    External (STAS, IntObj)

    Scope (_SB)
    {
        Method (_INI, 0, NotSerialized)  // _INI: Initialize
        {
            If (_OSI ("Darwin"))
            {
                STAS = One
            }
        }
    }
}

