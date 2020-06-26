/*
 * AppleUsbPower compatibility table for legacy hardware.
 *
 * Be warned that power supply values can be different
 * for different systems. Depending on the configuration
 * the values must be present in injected IOKitPersonalities
 * for com.apple.driver.AppleUSBMergeNub. iPad remains being
 * the most reliable device for testing USB port charging.
 *
 * Try NOT to rename EC0, H_EC, etc. to EC.
 * These devices are incompatible with macOS and may break
 * at any time. AppleACPIEC kext must NOT load.
 * See the disable code below.
 *
 * Reference USB: https://applelife.ru/posts/550233
 * Reference EC: https://applelife.ru/posts/807985
 */
DefinitionBlock ("", "SSDT", 2, "ACDT", "SsdtEC", 0x00001000)
{
    External (_SB_.PCI0.LPCB, DeviceObj)
    External (_SB_.PCI0.LPCB.EC0_, DeviceObj)
    External (_SB_.PCI0.SBRG, DeviceObj)
    External (_SB_.PCI0.SBRG.EC0_, DeviceObj)

    /*
     * While renaming EC0 to EC might potentially work initially,
     * it connects an incompatible driver (AppleACPIEC) to your hardware.
     * This can make your system unbootable at any time or hide bugs that
     * could trigger randomly.
     */

    If (CondRefOf (\_SB.PCI0.LPCB))
    {
        If (CondRefOf (\_SB.PCI0.LPCB.EC0))
        {
            Scope (\_SB.PCI0.LPCB.EC0)
            {
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    If (_OSI ("Darwin"))
                    {
                        Return (Zero)
                    }
                    Else
                    {
                        Return (0x0F)
                    }
                }
            }
        }

        Scope (\_SB.PCI0.LPCB)
        {
            Device (EC)
            {
                Name (_HID, "ACID0001")  // _HID: Hardware ID
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
            }
        }
    }
    ElseIf (CondRefOf (\_SB.PCI0.SBRG))
    {
        If (CondRefOf (\_SB.PCI0.SBRG.EC0))
        {
            Scope (\_SB.PCI0.SBRG.EC0)
            {
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    If (_OSI ("Darwin"))
                    {
                        Return (Zero)
                    }
                    Else
                    {
                        Return (0x0F)
                    }
                }
            }
        }

        Scope (\_SB.PCI0.SBRG)
        {
            Device (EC)
            {
                Name (_HID, "ACID0001")  // _HID: Hardware ID
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
            }
        }
    }


}
