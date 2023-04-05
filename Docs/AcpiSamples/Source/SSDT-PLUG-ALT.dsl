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

    Scope (\)
    {
        Method (_OFF, 0, NotSerialized)  // _OFF: Power Off
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
                Return (\_SB.PR00) /* External reference */
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
                    Return (_OFF ())
                }
            }

            Processor (CP01, 0x01, 0x00000510, 0x06)
            {
                Return (\_SB.PR01) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP02, 0x02, 0x00000510, 0x06)
            {
                Return (\_SB.PR02) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP03, 0x03, 0x00000510, 0x06)
            {
                Return (\_SB.PR03) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP04, 0x04, 0x00000510, 0x06)
            {
                Return (\_SB.PR04) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP05, 0x05, 0x00000510, 0x06)
            {
                Return (\_SB.PR05) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP06, 0x06, 0x00000510, 0x06)
            {
                Return (\_SB.PR06) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP07, 0x07, 0x00000510, 0x06)
            {
                Return (\_SB.PR07) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP08, 0x08, 0x00000510, 0x06)
            {
                Return (\_SB.PR08) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP09, 0x09, 0x00000510, 0x06)
            {
                Return (\_SB.PR09) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP10, 0x0A, 0x00000510, 0x06)
            {
                Return (\_SB.PR10) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP11, 0x0B, 0x00000510, 0x06)
            {
                Return (\_SB.PR11) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP12, 0x0C, 0x00000510, 0x06)
            {
                Return (\_SB.PR12) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP13, 0x0D, 0x00000510, 0x06)
            {
                Return (\_SB.PR13) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP14, 0x0E, 0x00000510, 0x06)
            {
                Return (\_SB.PR14) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP15, 0x0F, 0x00000510, 0x06)
            {
                Return (\_SB.PR15) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP16, 0x10, 0x00000510, 0x06)
            {
                Return (\_SB.PR16) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP17, 0x11, 0x00000510, 0x06)
            {
                Return (\_SB.PR17) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP18, 0x12, 0x00000510, 0x06)
            {
                Return (\_SB.PR18) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP19, 0x13, 0x00000510, 0x06)
            {
                Return (\_SB.PR19) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP20, 0x14, 0x00000510, 0x06)
            {
                Return (\_SB.PR20) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP21, 0x15, 0x00000510, 0x06)
            {
                Return (\_SB.PR21) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP22, 0x16, 0x00000510, 0x06)
            {
                Return (\_SB.PR22) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP23, 0x17, 0x00000510, 0x06)
            {
                Return (\_SB.PR23) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP24, 0x18, 0x00000510, 0x06)
            {
                Return (\_SB.PR24) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP25, 0x19, 0x00000510, 0x06)
            {
                Return (\_SB.PR25) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP26, 0x1A, 0x00000510, 0x06)
            {
                Return (\_SB.PR26) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP27, 0x1B, 0x00000510, 0x06)
            {
                Return (\_SB.PR27) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP28, 0x1C, 0x00000510, 0x06)
            {
                Return (\_SB.PR28) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP29, 0x1D, 0x00000510, 0x06)
            {
                Return (\_SB.PR29) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP30, 0x1E, 0x00000510, 0x06)
            {
                Return (\_SB.PR30) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP31, 0x1F, 0x00000510, 0x06)
            {
                Return (\_SB.PR31) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP32, 0x20, 0x00000510, 0x06)
            {
                Return (\_SB.PR32) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP33, 0x21, 0x00000510, 0x06)
            {
                Return (\_SB.PR33) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP34, 0x22, 0x00000510, 0x06)
            {
                Return (\_SB.PR34) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP35, 0x23, 0x00000510, 0x06)
            {
                Return (\_SB.PR35) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP36, 0x24, 0x00000510, 0x06)
            {
                Return (\_SB.PR36) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP37, 0x25, 0x00000510, 0x06)
            {
                Return (\_SB.PR37) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP38, 0x26, 0x00000510, 0x06)
            {
                Return (\_SB.PR38) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP39, 0x27, 0x00000510, 0x06)
            {
                Return (\_SB.PR39) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP40, 0x28, 0x00000510, 0x06)
            {
                Return (\_SB.PR40) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP41, 0x29, 0x00000510, 0x06)
            {
                Return (\_SB.PR41) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP42, 0x2A, 0x00000510, 0x06)
            {
                Return (\_SB.PR42) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP43, 0x2B, 0x00000510, 0x06)
            {
                Return (\_SB.PR43) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP44, 0x2C, 0x00000510, 0x06)
            {
                Return (\_SB.PR44) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP45, 0x2D, 0x00000510, 0x06)
            {
                Return (\_SB.PR45) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP46, 0x2E, 0x00000510, 0x06)
            {
                Return (\_SB.PR46) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP47, 0x2F, 0x00000510, 0x06)
            {
                Return (\_SB.PR47) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP48, 0x30, 0x00000510, 0x06)
            {
                Return (\_SB.PR48) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP49, 0x31, 0x00000510, 0x06)
            {
                Return (\_SB.PR49) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP50, 0x32, 0x00000510, 0x06)
            {
                Return (\_SB.PR50) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP51, 0x33, 0x00000510, 0x06)
            {
                Return (\_SB.PR51) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP52, 0x34, 0x00000510, 0x06)
            {
                Return (\_SB.PR52) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP53, 0x35, 0x00000510, 0x06)
            {
                Return (\_SB.PR53) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP54, 0x36, 0x00000510, 0x06)
            {
                Return (\_SB.PR54) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP55, 0x37, 0x00000510, 0x06)
            {
                Return (\_SB.PR55) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP56, 0x38, 0x00000510, 0x06)
            {
                Return (\_SB.PR56) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP57, 0x39, 0x00000510, 0x06)
            {
                Return (\_SB.PR57) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP58, 0x3A, 0x00000510, 0x06)
            {
                Return (\_SB.PR58) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP59, 0x3B, 0x00000510, 0x06)
            {
                Return (\_SB.PR59) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP60, 0x3C, 0x00000510, 0x06)
            {
                Return (\_SB.PR60) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP61, 0x3D, 0x00000510, 0x06)
            {
                Return (\_SB.PR61) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP62, 0x3E, 0x00000510, 0x06)
            {
                Return (\_SB.PR62) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }

            Processor (CP63, 0x3F, 0x00000510, 0x06)
            {
                Return (\_SB.PR63) /* External reference */
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (_OFF ())
                }
            }
        }
    }
}
