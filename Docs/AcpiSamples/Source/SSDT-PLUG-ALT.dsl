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
    External (_SB_.PR00, DeviceObj)
    External (_SB_.PR01, DeviceObj)
    External (_SB_.PR02, DeviceObj)
    External (_SB_.PR03, DeviceObj)
    External (_SB_.PR04, DeviceObj)
    External (_SB_.PR05, DeviceObj)
    External (_SB_.PR06, DeviceObj)
    External (_SB_.PR07, DeviceObj)
    External (_SB_.PR08, DeviceObj)
    External (_SB_.PR09, DeviceObj)
    External (_SB_.PR10, DeviceObj)
    External (_SB_.PR11, DeviceObj)
    External (_SB_.PR12, DeviceObj)
    External (_SB_.PR13, DeviceObj)
    External (_SB_.PR14, DeviceObj)
    External (_SB_.PR15, DeviceObj)
    External (_SB_.PR16, DeviceObj)
    External (_SB_.PR17, DeviceObj)
    External (_SB_.PR18, DeviceObj)
    External (_SB_.PR19, DeviceObj)
    External (_SB_.PR20, DeviceObj)
    External (_SB_.PR21, DeviceObj)
    External (_SB_.PR22, DeviceObj)
    External (_SB_.PR23, DeviceObj)
    External (_SB_.PR24, DeviceObj)
    External (_SB_.PR25, DeviceObj)
    External (_SB_.PR26, DeviceObj)
    External (_SB_.PR27, DeviceObj)
    External (_SB_.PR28, DeviceObj)
    External (_SB_.PR29, DeviceObj)
    External (_SB_.PR30, DeviceObj)
    External (_SB_.PR31, DeviceObj)
    External (_SB_.PR32, DeviceObj)
    External (_SB_.PR33, DeviceObj)
    External (_SB_.PR34, DeviceObj)
    External (_SB_.PR35, DeviceObj)
    External (_SB_.PR36, DeviceObj)
    External (_SB_.PR37, DeviceObj)
    External (_SB_.PR38, DeviceObj)
    External (_SB_.PR39, DeviceObj)
    External (_SB_.PR40, DeviceObj)
    External (_SB_.PR41, DeviceObj)
    External (_SB_.PR42, DeviceObj)
    External (_SB_.PR43, DeviceObj)
    External (_SB_.PR44, DeviceObj)
    External (_SB_.PR45, DeviceObj)
    External (_SB_.PR46, DeviceObj)
    External (_SB_.PR47, DeviceObj)
    External (_SB_.PR48, DeviceObj)
    External (_SB_.PR49, DeviceObj)
    External (_SB_.PR50, DeviceObj)
    External (_SB_.PR51, DeviceObj)
    External (_SB_.PR52, DeviceObj)
    External (_SB_.PR53, DeviceObj)
    External (_SB_.PR54, DeviceObj)
    External (_SB_.PR55, DeviceObj)
    External (_SB_.PR56, DeviceObj)
    External (_SB_.PR57, DeviceObj)
    External (_SB_.PR58, DeviceObj)
    External (_SB_.PR59, DeviceObj)
    External (_SB_.PR60, DeviceObj)
    External (_SB_.PR61, DeviceObj)
    External (_SB_.PR62, DeviceObj)
    External (_SB_.PR63, DeviceObj)
    External (_SB_.SCK0.C000, DeviceObj)
    External (_SB_.SCK0.C001, DeviceObj)
    External (_SB_.SCK0.C002, DeviceObj)
    External (_SB_.SCK0.C003, DeviceObj)
    External (_SB_.SCK0.C004, DeviceObj)
    External (_SB_.SCK0.C005, DeviceObj)
    External (_SB_.SCK0.C006, DeviceObj)
    External (_SB_.SCK0.C007, DeviceObj)
    External (_SB_.SCK0.C008, DeviceObj)
    External (_SB_.SCK0.C009, DeviceObj)
    External (_SB_.SCK0.C00A, DeviceObj)
    External (_SB_.SCK0.C00B, DeviceObj)
    External (_SB_.SCK0.C00C, DeviceObj)
    External (_SB_.SCK0.C00D, DeviceObj)
    External (_SB_.SCK0.C00E, DeviceObj)
    External (_SB_.SCK0.C00F, DeviceObj)
    External (_SB_.SCK0.C010, DeviceObj)
    External (_SB_.SCK0.C011, DeviceObj)
    External (_SB_.SCK0.C012, DeviceObj)
    External (_SB_.SCK0.C013, DeviceObj)
    External (_SB_.SCK0.C014, DeviceObj)
    External (_SB_.SCK0.C015, DeviceObj)
    External (_SB_.SCK0.C016, DeviceObj)
    External (_SB_.SCK0.C017, DeviceObj)
    External (_SB_.SCK0.C018, DeviceObj)
    External (_SB_.SCK0.C019, DeviceObj)
    External (_SB_.SCK0.C01A, DeviceObj)
    External (_SB_.SCK0.C01B, DeviceObj)
    External (_SB_.SCK0.C01C, DeviceObj)
    External (_SB_.SCK0.C01D, DeviceObj)
    External (_SB_.SCK0.C01E, DeviceObj)
    External (_SB_.SCK0.C01F, DeviceObj)
    External (_SB_.SCK0.C020, DeviceObj)
    External (_SB_.SCK0.C021, DeviceObj)
    External (_SB_.SCK0.C022, DeviceObj)
    External (_SB_.SCK0.C023, DeviceObj)
    External (_SB_.SCK0.C024, DeviceObj)
    External (_SB_.SCK0.C025, DeviceObj)
    External (_SB_.SCK0.C026, DeviceObj)
    External (_SB_.SCK0.C027, DeviceObj)
    External (_SB_.SCK0.C028, DeviceObj)
    External (_SB_.SCK0.C029, DeviceObj)
    External (_SB_.SCK0.C02A, DeviceObj)
    External (_SB_.SCK0.C02B, DeviceObj)
    External (_SB_.SCK0.C02C, DeviceObj)
    External (_SB_.SCK0.C02D, DeviceObj)
    External (_SB_.SCK0.C02E, DeviceObj)
    External (_SB_.SCK0.C02F, DeviceObj)
    External (_SB_.SCK0.C030, DeviceObj)
    External (_SB_.SCK0.C031, DeviceObj)
    External (_SB_.SCK0.C032, DeviceObj)
    External (_SB_.SCK0.C033, DeviceObj)
    External (_SB_.SCK0.C034, DeviceObj)
    External (_SB_.SCK0.C035, DeviceObj)
    External (_SB_.SCK0.C036, DeviceObj)
    External (_SB_.SCK0.C037, DeviceObj)
    External (_SB_.SCK0.C038, DeviceObj)
    External (_SB_.SCK0.C039, DeviceObj)
    External (_SB_.SCK0.C03A, DeviceObj)
    External (_SB_.SCK0.C03B, DeviceObj)
    External (_SB_.SCK0.C03C, DeviceObj)
    External (_SB_.SCK0.C03D, DeviceObj)
    External (_SB_.SCK0.C03E, DeviceObj)
    External (_SB_.SCK0.C03F, DeviceObj)

    Scope (\)
    {
        Method (MO86, 0, NotSerialized)
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

        Scope (_SB)
        {
            Processor (CP00, 0x00, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR00))
                {
                    Return (\_SB.PR00) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C000))
                {
                    Return (\_SB.SCK0.C000) /* External reference */
                }

                Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
                {
                    If ((Arg2 == Zero))
                    {
                        Return (Buffer (One)
                        {
                             0x03                                             // .
                        })
                    }

                    Return (Package (0x02)
                    {
                        "plugin-type", 
                        One
                    })
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP01, 0x01, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR01))
                {
                    Return (\_SB.PR01) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C001))
                {
                    Return (\_SB.SCK0.C001) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP02, 0x02, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR02))
                {
                    Return (\_SB.PR02) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C002))
                {
                    Return (\_SB.SCK0.C002) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP03, 0x03, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR03))
                {
                    Return (\_SB.PR03) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C003))
                {
                    Return (\_SB.SCK0.C003) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP04, 0x04, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR04))
                {
                    Return (\_SB.PR04) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C004))
                {
                    Return (\_SB.SCK0.C004) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP05, 0x05, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR05))
                {
                    Return (\_SB.PR05) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C005))
                {
                    Return (\_SB.SCK0.C005) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP06, 0x06, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR06))
                {
                    Return (\_SB.PR06) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C006))
                {
                    Return (\_SB.SCK0.C006) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP07, 0x07, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR07))
                {
                    Return (\_SB.PR07) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C007))
                {
                    Return (\_SB.SCK0.C007) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP08, 0x08, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR08))
                {
                    Return (\_SB.PR08) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C008))
                {
                    Return (\_SB.SCK0.C008) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP09, 0x09, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR09))
                {
                    Return (\_SB.PR09) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C009))
                {
                    Return (\_SB.SCK0.C009) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP10, 0x0A, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR10))
                {
                    Return (\_SB.PR10) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C00A))
                {
                    Return (\_SB.SCK0.C00A) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP11, 0x0B, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR11))
                {
                    Return (\_SB.PR11) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C00B))
                {
                    Return (\_SB.SCK0.C00B) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP12, 0x0C, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR12))
                {
                    Return (\_SB.PR12) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C00C))
                {
                    Return (\_SB.SCK0.C00C) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP13, 0x0D, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR13))
                {
                    Return (\_SB.PR13) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C00D))
                {
                    Return (\_SB.SCK0.C00D) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP14, 0x0E, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR14))
                {
                    Return (\_SB.PR14) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C00E))
                {
                    Return (\_SB.SCK0.C00E) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP15, 0x0F, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR15))
                {
                    Return (\_SB.PR15) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C00F))
                {
                    Return (\_SB.SCK0.C00F) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP16, 0x10, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR16))
                {
                    Return (\_SB.PR16) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C010))
                {
                    Return (\_SB.SCK0.C010) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP17, 0x11, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR17))
                {
                    Return (\_SB.PR17) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C011))
                {
                    Return (\_SB.SCK0.C011) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP18, 0x12, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR18))
                {
                    Return (\_SB.PR18) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C012))
                {
                    Return (\_SB.SCK0.C012) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP19, 0x13, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR19))
                {
                    Return (\_SB.PR19) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C013))
                {
                    Return (\_SB.SCK0.C013) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP20, 0x14, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR20))
                {
                    Return (\_SB.PR20) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C014))
                {
                    Return (\_SB.SCK0.C014) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP21, 0x15, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR21))
                {
                    Return (\_SB.PR21) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C015))
                {
                    Return (\_SB.SCK0.C015) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP22, 0x16, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR22))
                {
                    Return (\_SB.PR22) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C016))
                {
                    Return (\_SB.SCK0.C016) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP23, 0x17, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR23))
                {
                    Return (\_SB.PR23) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C017))
                {
                    Return (\_SB.SCK0.C017) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP24, 0x18, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR24))
                {
                    Return (\_SB.PR24) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C018))
                {
                    Return (\_SB.SCK0.C018) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP25, 0x19, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR25))
                {
                    Return (\_SB.PR25) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C019))
                {
                    Return (\_SB.SCK0.C019) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP26, 0x1A, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR26))
                {
                    Return (\_SB.PR26) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C01A))
                {
                    Return (\_SB.SCK0.C01A) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP27, 0x1B, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR27))
                {
                    Return (\_SB.PR27) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C01B))
                {
                    Return (\_SB.SCK0.C01B) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP28, 0x1C, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR28))
                {
                    Return (\_SB.PR28) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C01C))
                {
                    Return (\_SB.SCK0.C01C) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP29, 0x1D, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR29))
                {
                    Return (\_SB.PR29) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C01D))
                {
                    Return (\_SB.SCK0.C01D) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP30, 0x1E, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR30))
                {
                    Return (\_SB.PR30) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C01E))
                {
                    Return (\_SB.SCK0.C01E) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP31, 0x1F, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR31))
                {
                    Return (\_SB.PR31) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C01F))
                {
                    Return (\_SB.SCK0.C01F) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP32, 0x20, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR32))
                {
                    Return (\_SB.PR32) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C020))
                {
                    Return (\_SB.SCK0.C020) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP33, 0x21, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR33))
                {
                    Return (\_SB.PR33) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C021))
                {
                    Return (\_SB.SCK0.C021) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP34, 0x22, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR34))
                {
                    Return (\_SB.PR34) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C022))
                {
                    Return (\_SB.SCK0.C022) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP35, 0x23, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR35))
                {
                    Return (\_SB.PR35) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C023))
                {
                    Return (\_SB.SCK0.C023) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP36, 0x24, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR36))
                {
                    Return (\_SB.PR36) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C024))
                {
                    Return (\_SB.SCK0.C024) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP37, 0x25, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR37))
                {
                    Return (\_SB.PR37) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C025))
                {
                    Return (\_SB.SCK0.C025) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP38, 0x26, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR38))
                {
                    Return (\_SB.PR38) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C026))
                {
                    Return (\_SB.SCK0.C026) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP39, 0x27, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR39))
                {
                    Return (\_SB.PR39) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C027))
                {
                    Return (\_SB.SCK0.C027) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP40, 0x28, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR40))
                {
                    Return (\_SB.PR40) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C028))
                {
                    Return (\_SB.SCK0.C028) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP41, 0x29, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR41))
                {
                    Return (\_SB.PR41) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C029))
                {
                    Return (\_SB.SCK0.C029) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP42, 0x2A, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR42))
                {
                    Return (\_SB.PR42) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C02A))
                {
                    Return (\_SB.SCK0.C02A) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP43, 0x2B, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR43))
                {
                    Return (\_SB.PR43) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C02B))
                {
                    Return (\_SB.SCK0.C02B) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP44, 0x2C, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR44))
                {
                    Return (\_SB.PR44) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C02C))
                {
                    Return (\_SB.SCK0.C02C) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP45, 0x2D, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR45))
                {
                    Return (\_SB.PR45) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C02D))
                {
                    Return (\_SB.SCK0.C02D) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP46, 0x2E, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR46))
                {
                    Return (\_SB.PR46) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C02E))
                {
                    Return (\_SB.SCK0.C02E) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP47, 0x2F, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR47))
                {
                    Return (\_SB.PR47) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C02F))
                {
                    Return (\_SB.SCK0.C02F) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP48, 0x30, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR48))
                {
                    Return (\_SB.PR48) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C030))
                {
                    Return (\_SB.SCK0.C030) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP49, 0x31, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR49))
                {
                    Return (\_SB.PR49) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C031))
                {
                    Return (\_SB.SCK0.C031) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP50, 0x32, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR50))
                {
                    Return (\_SB.PR50) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C032))
                {
                    Return (\_SB.SCK0.C032) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP51, 0x33, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR51))
                {
                    Return (\_SB.PR51) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C033))
                {
                    Return (\_SB.SCK0.C033) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP52, 0x34, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR52))
                {
                    Return (\_SB.PR52) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C034))
                {
                    Return (\_SB.SCK0.C034) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP53, 0x35, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR53))
                {
                    Return (\_SB.PR53) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C035))
                {
                    Return (\_SB.SCK0.C035) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP54, 0x36, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR54))
                {
                    Return (\_SB.PR54) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C036))
                {
                    Return (\_SB.SCK0.C036) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP55, 0x37, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR55))
                {
                    Return (\_SB.PR55) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C037))
                {
                    Return (\_SB.SCK0.C037) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP56, 0x38, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR56))
                {
                    Return (\_SB.PR56) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C038))
                {
                    Return (\_SB.SCK0.C038) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP57, 0x39, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR57))
                {
                    Return (\_SB.PR57) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C039))
                {
                    Return (\_SB.SCK0.C039) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP58, 0x3A, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR58))
                {
                    Return (\_SB.PR58) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C03A))
                {
                    Return (\_SB.SCK0.C03A) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP59, 0x3B, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR59))
                {
                    Return (\_SB.PR59) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C03B))
                {
                    Return (\_SB.SCK0.C03B) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP60, 0x3C, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR60))
                {
                    Return (\_SB.PR60) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C03C))
                {
                    Return (\_SB.SCK0.C03C) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP61, 0x3D, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR61))
                {
                    Return (\_SB.PR61) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C03D))
                {
                    Return (\_SB.SCK0.C03D) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP62, 0x3E, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR62))
                {
                    Return (\_SB.PR62) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C03E))
                {
                    Return (\_SB.SCK0.C03E) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }

            Processor (CP63, 0x3F, 0x00000510, 0x06)
            {
                If (CondRefOf (\_SB.PR63))
                {
                    Return (\_SB.PR63) /* External reference */
                }

                If (CondRefOf (\_SB.SCK0.C03F))
                {
                    Return (\_SB.SCK0.C03F) /* External reference */
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (MO86 ())
                }
            }
        }
    }
}
