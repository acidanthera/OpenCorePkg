/*
 * Intel ACPI Component Architecture
 * AML/ASL+ Disassembler version 20180427 (64-bit version)(RM)
 * Copyright (c) 2000 - 2018 Intel Corporation
 * 
 * Disassembling to symbolic ASL+ operators
 *
 * Disassembly of SSDT-NVMe.aml, Fri Apr 16 23:34:27 2021
 *
 * Original Table Header:
 *     Signature        "SSDT"
 *     Length           0x000000C6 (198)
 *     Revision         0x02
 *     Checksum         0x29
 *     OEM ID           "hack"
 *     OEM Table ID     "nvme"
 *     OEM Revision     0x00000000 (0)
 *     Compiler ID      "INTL"
 *     Compiler Version 0x20161210 (538317328)
 */
DefinitionBlock ("", "SSDT", 2, "hack", "nvme", 0x00000000)
{
    External (_SB_.PCI0.NPE1, DeviceObj)    // (from opcode)
    External (_SB_.PCI0.NPE1.H000._ADR, UnknownObj)    // (from opcode)

    Scope (\_SB.PCI0.NPE1)
    {
        Device (NVME)
        {
            Name (_ADR, Zero)  // _ADR: Address
            Name (_SUN, One)  // _SUN: Slot User Number
            Method (_INI, 0, NotSerialized)  // _INI: Initialize
            {
                \_SB.PCI0.NPE1.H000._ADR = 0x0F
            }

            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                If ((Arg2 == Zero))
                {
                    Return (Buffer (One)
                    {
                         0x03                                           
                    })
                }

                Return (Package (0x02)
                {
                    "built-in", 
                    Buffer (0x0A)
                    {
                        "NVMe SSD"
                    }
                })
            }
        }
    }
}

