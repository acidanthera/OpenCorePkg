/*
 * Intel ACPI Component Architecture
 * AML/ASL+ Disassembler version 20190215 (64-bit version)
 * Copyright (c) 2000 - 2019 Intel Corporation
 * 
 * Disassembling to symbolic ASL+ operators
 *
 * Disassembly of iASL8AmXq4.aml, Tue Apr 16 22:05:03 2019
 *
 * Original Table Header:
 *     Signature        "SSDT"
 *     Length           0x0000005E (94)
 *     Revision         0x02
 *     Checksum         0x8F
 *     OEM ID           "APPLE "
 *     OEM Table ID     "EHCx_OFF"
 *     OEM Revision     0x00001000 (4096)
 *     Compiler ID      "INTL"
 *     Compiler Version 0x20190215 (538509845)
 */
DefinitionBlock ("", "SSDT", 2, "APPLE ", "EHCx_OFF", 0x00001000)
{
    Scope (\)
    {
        OperationRegion (RCRG, SystemMemory, 0xFED1F418, One)
        Field (RCRG, DWordAcc, Lock, Preserve)
        {
                ,   13, 
            EH2D,   1, 
                ,   1, 
            EH1D,   1
        }

        Method (_INI, 0, NotSerialized)  // _INI: Initialize
        {
            EH1D = One  // Disable EHC1
            EH2D = One  // Disable EHC2
        }
    }
}

// Attention!
// Only for 7,8,9-series chipsets and 10.11 and newer!
//
// To disable EHC1 and EHC2 - set an option "XHCI Mode" to "Enabled" in yours BIOS.
// If the "XHCI Mode" option is not available in yours BIOS or works incorrectly, then use this ACPI table.
// Disabling through BIOS is preferable whenever possible.
//
// Note: for 7-series you need to use either "EH1D = One" or "EH2D = One" but not both!
// This is because only one of the devices (EHC1 or EHC2) is used by macOS. Check the IOReg.
