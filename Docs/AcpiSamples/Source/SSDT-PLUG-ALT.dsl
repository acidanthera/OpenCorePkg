/*
 * XCPM power management compatibility table with Darwin method
 * for Alder Lake CPUs and possibly others with CPU objects
 * declared as Device instead of Processor.
 *
 * Note 1: Just like Rocket Lake CPUs, Alder Lake CPUs require
 * custom CPU profile via CPUFriend.
 * REF: https://github.com/dortania/bugtracker/issues/190
 *
 * Note 2: PBlockAddress (0x00000510 here) can be corrected
 * to match MADT and may vary across the boards and vendors.
 * This field is ignored by macOS and read from MADT instead,
 * so it is purely cosmetic.
 */
DefinitionBlock ("", "SSDT", 2, "ACDT", "CpuPlugA", 0x00003000)
{
    External (_SB_, DeviceObj)

    Scope (\_SB)
    {
        Processor (CP00, 0x00, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
            Name (_UID, Zero)
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

            Method (_DSM, 4, NotSerialized)
            {
                If (LEqual (Arg2, Zero)) {
                    Return (Buffer (One) { 0x03 })
                }

                Return (Package (0x02)
                {
                    "plugin-type",
                    One
                })
            }
        }

        Processor (CP01, 0x01, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 1)
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

        Processor (CP02, 0x02, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 2)
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

        Processor (CP03, 0x03, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 3)
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

        Processor (CP04, 0x04, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 4)
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

        Processor (CP05, 0x05, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 5)
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

        Processor (CP06, 0x06, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 6)
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

        Processor (CP07, 0x07, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 7)
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

        Processor (CP08, 0x08, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 8)
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

        Processor (CP09, 0x09, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 9)
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

        Processor (CP10, 0x0A, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 10)
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

        Processor (CP11, 0x0B, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 11)
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

        Processor (CP12, 0x0C, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 12)
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

        Processor (CP13, 0x0D, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 13)
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

        Processor (CP14, 0x0E, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 14)
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

        Processor (CP15, 0x0F, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 15)
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

        Processor (CP16, 0x10, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 16)
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

        Processor (CP17, 0x11, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 17)
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

        Processor (CP18, 0x12, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 18)
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

        Processor (CP19, 0x13, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 19)
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

        Processor (CP20, 0x14, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 20)
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

        Processor (CP21, 0x15, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 21)
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

        Processor (CP22, 0x16, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 22)
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

        Processor (CP23, 0x17, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 23)
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

        Processor (CP24, 0x18, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 24)
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

        Processor (CP25, 0x19, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 25)
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

        Processor (CP26, 0x1A, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 26)
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

        Processor (CP27, 0x1B, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 27)
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

        Processor (CP28, 0x1C, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 28)
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

        Processor (CP29, 0x1D, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 29)
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

        Processor (CP30, 0x1E, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 30)
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

        Processor (CP31, 0x1F, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 31)
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

        Processor (CP32, 0x20, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 32)
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

        Processor (CP33, 0x21, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 33)
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

        Processor (CP34, 0x22, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 34)
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

        Processor (CP35, 0x23, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 35)
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

        Processor (CP36, 0x24, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 36)
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

        Processor (CP37, 0x25, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 37)
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

        Processor (CP38, 0x26, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 38)
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

        Processor (CP39, 0x27, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 39)
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

        Processor (CP40, 0x28, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 40)
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

        Processor (CP41, 0x29, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 41)
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

        Processor (CP42, 0x2A, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 42)
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

        Processor (CP43, 0x2B, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 43)
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

        Processor (CP44, 0x2C, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 44)
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

        Processor (CP45, 0x2D, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 45)
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

        Processor (CP46, 0x2E, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 46)
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

        Processor (CP47, 0x2F, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 47)
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

        Processor (CP48, 0x30, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 48)
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

        Processor (CP49, 0x31, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 49)
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

        Processor (CP50, 0x32, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 50)
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

        Processor (CP51, 0x33, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 51)
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

        Processor (CP52, 0x34, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 52)
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

        Processor (CP53, 0x35, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 53)
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

        Processor (CP54, 0x36, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 54)
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

        Processor (CP55, 0x37, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 55)
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

        Processor (CP56, 0x38, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 56)
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

        Processor (CP57, 0x39, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 57)
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

        Processor (CP58, 0x3A, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 58)
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

        Processor (CP59, 0x3B, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 59)
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

        Processor (CP60, 0x3C, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 60)
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

        Processor (CP61, 0x3D, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 61)
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

        Processor (CP62, 0x3E, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 62)
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

        Processor (CP63, 0x3F, 0x00000510, 0x06)
        {
            Name (_HID, "ACPI0007" /* Processor Device */)
            Name (_UID, 63)
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
