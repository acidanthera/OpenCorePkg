// Adding PNLF device for WhateverGreen.kext and others.
// This one is specific to CFL+
// Rename GFX0 to anything else if your IGPU name is different.

DefinitionBlock ("", "SSDT", 2, "ACDT", "PNLFCFL", 0x00000000)
{
    External (_SB_.PCI0.GFX0, DeviceObj)

    Device (_SB.PCI0.GFX0.PNLF)
    {
     // Name (_ADR, Zero)  // _ADR: Address
        Name (_HID, EisaId ("APP0002"))  // _HID: Hardware ID
        Name (_CID, "backlight")  // _CID: Compatible ID
        Name (_UID, 0x13)  // _UID: Unique ID
        Method (_STA, 0, NotSerialized)  // _STA: Status
        {
            If (_OSI ("Darwin"))
            {
                Return (0x0B)
            }
            Else
            {
                Return (Zero)
            }
        }
    }
}

