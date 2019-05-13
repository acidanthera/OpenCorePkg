/*
 * Intel ACPI Component Architecture
 * AML/ASL+ Disassembler version 20190215 (64-bit version)
 * Copyright (c) 2000 - 2019 Intel Corporation
 * 
 * Disassembling to symbolic ASL+ operators
 *
 * Disassembly of iASLTtGlr6.aml, Thu May  9 02:09:20 2019
 *
 * Original Table Header:
 *     Signature        "SSDT"
 *     Length           0x000000F4 (244)
 *     Revision         0x02
 *     Checksum         0x2F
 *     OEM ID           "APPLE "
 *     OEM Table ID     "SsdtEC"
 *     OEM Revision     0x00001000 (4096)
 *     Compiler ID      "INTL"
 *     Compiler Version 0x20190215 (538509845)
 */
DefinitionBlock ("", "SSDT", 2, "APPLE ", "SsdtEC", 0x00001000)
{
    External (_SB_.PCI0.LPCB, DeviceObj)

    Scope (\_SB)
    {
        Device (USBX)
        {
            Name (_ADR, Zero)  // _ADR: Address
            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                If ((Arg2 == Zero))
                {
                    Return (Buffer (One)
                    {
                         0x03                                             // .
                    })
                }

                Return (Package (0x08)
                {
                    "kUSBSleepPowerSupply", 
                    0x0640, 
                    "kUSBSleepPortCurrentLimit", 
                    0x0A8C, 
                    "kUSBWakePowerSupply", 
                    0x0640, 
                    "kUSBWakePortCurrentLimit", 
                    0x0A8C
                })
            }
        }

        Scope (\_SB.PCI0.LPCB)
        {
            Device (EC)
            {
                Name (_HID, EisaId ("PNP0C09") /* Embedded Controller Device */)  // _HID: Hardware ID
            }
        }
    }
}

