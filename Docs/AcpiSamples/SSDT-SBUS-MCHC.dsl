/*
 * SMBus compatibility table.
 */
DefinitionBlock ("", "SSDT", 2, "ACDT", "MCHCSBUS", 0x00000000)
{
    External (_SB_.PCI0, DeviceObj)
    External (_SB_.PCI0.SBUS.BUS0, DeviceObj)

    Scope (_SB.PCI0)
    {
        Device (MCHC)
        {
            Name (_ADR, Zero)  // _ADR: Address
        }
    }

    Device (_SB.PCI0.SBUS.BUS0)
    {
        Name (_CID, "smbus")  // _CID: Compatible ID
        Name (_ADR, Zero)  // _ADR: Address
        Device (DVL0)
        {
            Name (_ADR, 0x57)  // _ADR: Address
            Name (_CID, "diagsvault")  // _CID: Compatible ID
            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                If (!Arg2)
                {
                    Return (Buffer (One)
                    {
                         0x57                                             // W
                    })
                }

                Return (Package (0x02)
                {
                    "address", 
                    0x57
                })
            }
        }
    }

    Method (DTGP, 5, NotSerialized)
    {
        If ((Arg0 == ToUUID ("a0b5b7c6-1318-441c-b0c9-fe695eaf949b")))
        {
            If ((Arg1 == One))
            {
                If ((Arg2 == Zero))
                {
                    Arg4 = Buffer (One)
                        {
                             0x03                                             // .
                        }
                    Return (One)
                }

                If ((Arg2 == One))
                {
                    Return (One)
                }
            }
        }

        Arg4 = Buffer (One)
            {
                 0x00                                             // .
            }
        Return (Zero)
    }
}

