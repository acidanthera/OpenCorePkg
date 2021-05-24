/*
 * Hyper-V CPU SSDT to enable proper CPU detection under macOS.
 *
 * Windows 10 and newer use ACPI0007 Device objects for CPUs, which macOS cannot detect.
 * This SSDT redefines the first 64 as Processor objects.
 * This may cause issues with other operating systems.
 *
 * On Windows 8.1, the objects defined here will conflict with the existing
 * objects preventing two sets of Processor objects from being loaded.
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "HVCPU", 0x00000000)
{
    // Processor count
    External (PCNT, FieldUnitObj)

    // Duplicated from Windows 8.1 Hyper-V Generation 2 DSDT
    Scope (\_SB)
    {
        Method (PSTA, 1, Serialized)
        {
            If (LLessEqual (Arg0, PCNT))
            {
                Return (0x0F)
            }

            Return (Zero)
        }

        Processor (P001, 0x01, 0x00000000, 0x00)
        {
            Method (_STA, 0, Serialized)
            {
                Return (0x0F)
            }
        }

        Processor (P002, 0x02, 0x00000000, 0x00)
        {
            Name (PNUM, 0x02)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P003, 0x03, 0x00000000, 0x00)
        {
            Name (PNUM, 0x03)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P004, 0x04, 0x00000000, 0x00)
        {
            Name (PNUM, 0x04)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P005, 0x05, 0x00000000, 0x00)
        {
            Name (PNUM, 0x05)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P006, 0x06, 0x00000000, 0x00)
        {
            Name (PNUM, 0x06)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P007, 0x07, 0x00000000, 0x00)
        {
            Name (PNUM, 0x07)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P008, 0x08, 0x00000000, 0x00)
        {
            Name (PNUM, 0x08)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P009, 0x09, 0x00000000, 0x00)
        {
            Name (PNUM, 0x09)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P010, 0x0A, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0A)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P011, 0x0B, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0B)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P012, 0x0C, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0C)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P013, 0x0D, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0D)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P014, 0x0E, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0E)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P015, 0x0F, 0x00000000, 0x00)
        {
            Name (PNUM, 0x0F)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P016, 0x10, 0x00000000, 0x00)
        {
            Name (PNUM, 0x10)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P017, 0x11, 0x00000000, 0x00)
        {
            Name (PNUM, 0x11)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P018, 0x12, 0x00000000, 0x00)
        {
            Name (PNUM, 0x12)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P019, 0x13, 0x00000000, 0x00)
        {
            Name (PNUM, 0x13)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P020, 0x14, 0x00000000, 0x00)
        {
            Name (PNUM, 0x14)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P021, 0x15, 0x00000000, 0x00)
        {
            Name (PNUM, 0x15)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P022, 0x16, 0x00000000, 0x00)
        {
            Name (PNUM, 0x16)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P023, 0x17, 0x00000000, 0x00)
        {
            Name (PNUM, 0x17)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P024, 0x18, 0x00000000, 0x00)
        {
            Name (PNUM, 0x18)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P025, 0x19, 0x00000000, 0x00)
        {
            Name (PNUM, 0x19)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P026, 0x1A, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1A)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P027, 0x1B, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1B)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P028, 0x1C, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1C)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P029, 0x1D, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1D)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P030, 0x1E, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1E)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P031, 0x1F, 0x00000000, 0x00)
        {
            Name (PNUM, 0x1F)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P032, 0x20, 0x00000000, 0x00)
        {
            Name (PNUM, 0x20)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P033, 0x21, 0x00000000, 0x00)
        {
            Name (PNUM, 0x21)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P034, 0x22, 0x00000000, 0x00)
        {
            Name (PNUM, 0x22)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P035, 0x23, 0x00000000, 0x00)
        {
            Name (PNUM, 0x23)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P036, 0x24, 0x00000000, 0x00)
        {
            Name (PNUM, 0x24)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P037, 0x25, 0x00000000, 0x00)
        {
            Name (PNUM, 0x25)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P038, 0x26, 0x00000000, 0x00)
        {
            Name (PNUM, 0x26)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P039, 0x27, 0x00000000, 0x00)
        {
            Name (PNUM, 0x27)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P040, 0x28, 0x00000000, 0x00)
        {
            Name (PNUM, 0x28)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P041, 0x29, 0x00000000, 0x00)
        {
            Name (PNUM, 0x29)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P042, 0x2A, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2A)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P043, 0x2B, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2B)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P044, 0x2C, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2C)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P045, 0x2D, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2D)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P046, 0x2E, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2E)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P047, 0x2F, 0x00000000, 0x00)
        {
            Name (PNUM, 0x2F)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P048, 0x30, 0x00000000, 0x00)
        {
            Name (PNUM, 0x30)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P049, 0x31, 0x00000000, 0x00)
        {
            Name (PNUM, 0x31)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P050, 0x32, 0x00000000, 0x00)
        {
            Name (PNUM, 0x32)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P051, 0x33, 0x00000000, 0x00)
        {
            Name (PNUM, 0x33)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P052, 0x34, 0x00000000, 0x00)
        {
            Name (PNUM, 0x34)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P053, 0x35, 0x00000000, 0x00)
        {
            Name (PNUM, 0x35)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P054, 0x36, 0x00000000, 0x00)
        {
            Name (PNUM, 0x36)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P055, 0x37, 0x00000000, 0x00)
        {
            Name (PNUM, 0x37)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P056, 0x38, 0x00000000, 0x00)
        {
            Name (PNUM, 0x38)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P057, 0x39, 0x00000000, 0x00)
        {
            Name (PNUM, 0x39)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P058, 0x3A, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3A)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P059, 0x3B, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3B)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P060, 0x3C, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3C)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P061, 0x3D, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3D)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P062, 0x3E, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3E)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P063, 0x3F, 0x00000000, 0x00)
        {
            Name (PNUM, 0x3F)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }

        Processor (P064, 0x40, 0x00000000, 0x00)
        {
            Name (PNUM, 0x40)
            Method (_STA, 0, NotSerialized)
            {
                Return (PSTA (PNUM))
            }
        }
    }
}
