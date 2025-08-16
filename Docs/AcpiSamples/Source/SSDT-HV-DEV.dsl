/*
 * Hyper-V device SSDT to disable unsupported devices under macOS on Windows Server 2019 / Windows 10 and newer.
 *
 * Windows 10 and newer define various objects conditionally, which are
 * unsupported in older versions of macOS. All objects are enabled regardless
 * of state in older versions, causing slow boot behavior and crashes during boot caused by AppleACPIPlatform.
 *
 * Windows 10 and newer also use ACPI0007 Device objects for CPUs, which macOS cannot detect.
 * This SSDT redefines the first 64 as Processor objects.
 * This may cause issues with other operating systems.
 * 
 * This SSDT defines _STA methods for all affected objects and disables them entirely, or enables only if
 * the approprate register is set (enabled in Hyper-V).
 *
 * On Windows Server 2012 R2 / Windows 8.1 and older this SSDT is unnecessary and can be disabled along with associated patches.
 *
 * Requires the following ACPI patches:
 * (1) Base:            \_SB.VMOD.TPM2
 *     Comment:         _STA to XSTA rename (Hyper-V TPM)
 *     Count:           1
 *     Find:            5F535441 (_STA)
 *     Replace:         58535441 (XSTA)
 *     TableSignature:  44534454 (DSDT)
 * (2) Base:            \_SB.NVDR
 *     Comment:         _STA to XSTA rename (Hyper-V NVDIMM)
 *     Count:           1
 *     Find:            5F535441 (_STA)
 *     Replace:         58535441 (XSTA)
 *     TableSignature:  44534454 (DSDT)
 * (3) Base:            \_SB.EPC
 *     Comment:         _STA to XSTA rename (Hyper-V EPC)
 *     Count:           1
 *     Find:            5F535441 (_STA)
 *     Replace:         58535441 (XSTA)
 *     TableSignature:  44534454 (DSDT)
 * (4) Base:            \_SB.VMOD.BAT1
 *     Comment:         _STA to XSTA rename (Hyper-V battery)
 *     Count:           1
 *     Find:            5F535441 (_STA)
 *     Replace:         58535441 (XSTA)
 *     TableSignature:  44534454 (DSDT)
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "HVDEV", 0x00000000)
{
    //
    // EC fields in DSDT
    //
    External (SCFG, FieldUnitObj)
    External (TCFG, FieldUnitObj)
    External (NCFG, FieldUnitObj)
    External (SGXE, FieldUnitObj)
    External (BCFG, FieldUnitObj)
    External (PCNT, FieldUnitObj)

    //
    // Device and method objects in DSDT
    //
    External (\_SB.UAR1, DeviceObj)
    External (\_SB.UAR2, DeviceObj)
    External (\_SB.VMOD.TPM2, DeviceObj)
    External (\_SB.VMOD.TPM2.XSTA, MethodObj)
    External (\_SB.NVDR, DeviceObj)
    External (\_SB.NVDR.XSTA, MethodObj)
    External (\_SB.EPC, DeviceObj)
    External (\_SB.EPC.XSTA, MethodObj)
    External (\_SB.VMOD.BAT1, DeviceObj)
    External (\_SB.VMOD.BAT1.XSTA, MethodObj)
    External (\_SB.VMOD.AC1, DeviceObj)

    //
    // Duplicated from Windows 8.1 Hyper-V Generation 2 DSDT.
    // Defines old-style Processor objects that macOS can use.
    //
    // PSTA will exist on 8.1 and older, and should block this SSDT from
    // being loaded if used on older Hyper-V versions.
    //
    Scope (\_SB)
    {
        Method (PSTA, 1, Serialized)
        {
            If (LAnd (_OSI ("Darwin"), LLessEqual (Arg0, PCNT)))
            {
                Return (0x0F)
            }

            Return (Zero)
        }

        Processor (P001, 0x01, 0x00000000, 0x00)
        {
            Name (PNUM, 0x01)
            Method (_STA, 0, Serialized)
            {
                Return (PSTA (PNUM))
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

    //
    // Older versions of macOS before 10.6 do not support conditional definitions
    // and will always expose the objects.
    // Re-evaluate in each object's _STA function as a workaround.
    //
    // Serial port devices.
    //
    If (LGreater (SCFG, Zero))
    {
        Scope (\_SB.UAR1)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (SCFG, Zero))
                {
                    Return (0x0F)
                }

                Return (Zero)
            }
        }

        Scope (\_SB.UAR2)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (SCFG, Zero))
                {
                    Return (0x0F)
                }

                Return (Zero)
            }
        }
    }

    //
    // TPM device.
    //
    If (LGreater (TCFG, Zero))
    {
        Scope (\_SB.VMOD.TPM2)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (TCFG, Zero))
                {
                    Return (\_SB.VMOD.TPM2.XSTA())
                }

                Return (Zero)
            }
        }
    }

    //
    // NVDIMM device.
    //
    If (LGreater (NCFG, Zero))
    {
        Scope (\_SB.NVDR)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (NCFG, Zero))
                {
                    Return (\_SB.NVDR.XSTA())
                }

                Return (Zero)
            }
        }
    }

    //
    // Enclave Page Cache device.
    //
    If (LGreater (SGXE, Zero))
    {
        Scope (\_SB.EPC)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (SGXE, Zero))    
                {
                    Return (\_SB.EPC.XSTA())
                }

                Return (Zero)
            }
        }
    }

    //
    // Battery and AC adapter device.
    //
    If (LGreater (BCFG, Zero))
    {
        Scope (\_SB.VMOD.BAT1)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (BCFG, Zero))
                {
                    Return (\_SB.VMOD.BAT1.XSTA())
                }

                Return (Zero)
            }
        }

        Scope (\_SB.VMOD.AC1)
        {
            Method (_STA, 0, NotSerialized)
            {
                If (LGreater (BCFG, Zero))
                {
                    Return (0x0F)
                }

                Return (Zero)
            }
        }
    }
}
