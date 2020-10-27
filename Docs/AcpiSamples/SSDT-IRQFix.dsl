/*
 * Intel ACPI Component Architecture
 * AML/ASL+ Disassembler version 20180427 (64-bit version)(RM)
 * Copyright (c) 2000 - 2018 Intel Corporation
 * 
 * Disassembling to non-symbolic legacy ASL operators
 *
 * Disassembly of iASLpzP6ix.aml, Tue Oct 27 14:27:40 2020
 *
 * Original Table Header:
 *     Signature        "SSDT"
 *     Length           0x0000028D (653)
 *     Revision         0x02
 *     Checksum         0xFD
 *     OEM ID           "ACDT "
 *     OEM Table ID     "IRQFix"
 *     OEM Revision     0x00001000 (4096)
 *     Compiler ID      "INTL"
 *     Compiler Version 0x20180427 (538444839)
 */
DefinitionBlock ("", "SSDT", 2, "ACDT ", "IRQFix", 0x00001000)
{
    External (_SB_.PCI0.LPCB.HPET._CRS, UnknownObj)    // (from opcode)
    External (_SB_.PCI0.LPCB.HPET._STA, UnknownObj)    // (from opcode)
    External (_SB_.PCI0.LPCB.IPIC._CRS, UnknownObj)    // (from opcode)
    External (_SB_.PCI0.LPCB.RTC_._CRS, UnknownObj)    // (from opcode)
    External (_SB_.PCI0.LPCB.TIMR._CRS, UnknownObj)    // (from opcode)

    If (_OSI ("Darwin"))
    {
        Device (DEAX)
        {
            Name (_HID, "DEA00000")  // _HID: Hardware ID
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (_OSI ("Darwin"))
                {
                    Return (0x0F)
                }
                Else
                {
                    Return (Zero)
                }
            }

            Method (_INI, 0, NotSerialized)  // _INI: Initialize
            {
                /* 
                    * You can choose "Store (Zero, \_SB.PCI0.LPCB.HPET._STA) or Store (0x0F, \_SB.PCI0.LPCB.HPET._STA)"
                    * to Disable or Enable HPET, "Store (0x0F, \_SB.PCI0.LPCB.HPET._STA)' is used as default (enabled HPET)
                */
                /* 
                Store (Zero, \_SB.PCI0.LPCB.HPET._STA)
                */
                Store (0x0F, \_SB.PCI0.LPCB.HPET._STA)
                Store (DEA1, \_SB.PCI0.LPCB.HPET._CRS)
                Store (DEA2, \_SB.PCI0.LPCB.RTC._CRS)
                Store (DEA3, \_SB.PCI0.LPCB.TIMR._CRS)
                Store (DEA4, \_SB.PCI0.LPCB.IPIC._CRS)
            }

            Name (DEA1, ResourceTemplate ()
            {
                IRQNoFlags ()
                    {0,8, 11, 15}
                Memory32Fixed (ReadWrite,
                    0xFED00000,         // Address Base
                    0x00000400,         // Address Length
                    )
            })
            Name (DEA2, ResourceTemplate ()
            {
                IO (Decode16,
                    0x0070,             // Range Minimum
                    0x0070,             // Range Maximum
                    0x01,               // Alignment
                    0x08,               // Length
                    )
            })
            Name (DEA3, ResourceTemplate ()
            {
                IO (Decode16,
                    0x0040,             // Range Minimum
                    0x0040,             // Range Maximum
                    0x01,               // Alignment
                    0x04,               // Length
                    )
                IO (Decode16,
                    0x0050,             // Range Minimum
                    0x0050,             // Range Maximum
                    0x10,               // Alignment
                    0x04,               // Length
                    )
            })
            Name (DEA4, ResourceTemplate ()
            {
                IO (Decode16,
                    0x0020,             // Range Minimum
                    0x0020,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x0024,             // Range Minimum
                    0x0024,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x0028,             // Range Minimum
                    0x0028,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x002C,             // Range Minimum
                    0x002C,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x0030,             // Range Minimum
                    0x0030,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x0034,             // Range Minimum
                    0x0034,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x0038,             // Range Minimum
                    0x0038,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x003C,             // Range Minimum
                    0x003C,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x00A0,             // Range Minimum
                    0x00A0,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x00A4,             // Range Minimum
                    0x00A4,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x00A8,             // Range Minimum
                    0x00A8,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x00AC,             // Range Minimum
                    0x00AC,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x00B0,             // Range Minimum
                    0x00B0,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x00B4,             // Range Minimum
                    0x00B4,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x00B8,             // Range Minimum
                    0x00B8,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x00BC,             // Range Minimum
                    0x00BC,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
                IO (Decode16,
                    0x04D0,             // Range Minimum
                    0x04D0,             // Range Maximum
                    0x01,               // Alignment
                    0x02,               // Length
                    )
            })
        }
    }
}

