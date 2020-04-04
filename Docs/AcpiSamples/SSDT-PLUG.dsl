/*
 * XCPM power management compatibility table.
 *
 * Please note that this table is only a sample and may need to be
 * adapted to fit your board's ACPI stack. For instance, both scope
 * and device name may vary (e.g. _SB_.PR00 instead of _PR_.CPU0).
 *
 * While the table contains several examples of CPU paths, you should
 * remove all the ones irrelevant for your board.
 */
DefinitionBlock ("", "SSDT", 2, "ACDT", "CpuPlug", 0x00003000)
{
    External (_SB_.CPU0, DeviceObj)
    External (_PR_.CPU0, DeviceObj)
    External (_PR_.C000, DeviceObj)
    External (_PR_.P000, DeviceObj)
    External (_SB_.PR00, DeviceObj)
    External (_PR_.PR00, DeviceObj)
    External (_SB_.SCK0.CP00, DeviceObj)
    External (_SB_.SCK0.PR00, DeviceObj)

    If (CondRefOf (\_SB.CPU0)) {
        Scope (\_SB.CPU0) {
            Method (_DSM, 4, NotSerialized) {
                If (LEqual (Arg2, Zero)) {
                    Return (Buffer (One) { 0x03 })
                }

                Return (Package (0x02) {
                    "plugin-type",
                    One
                })
            }
        }
    }

    If (CondRefOf (\_PR.CPU0)) {
        Scope (\_PR.CPU0) {
            Method (_DSM, 4, NotSerialized) {
                If (LEqual (Arg2, Zero)) {
                    Return (Buffer (One) { 0x03 })
                }

                Return (Package (0x02) {
                    "plugin-type",
                    One
                })
            }
        }
    }

    If (CondRefOf (\_SB.PR00)) {
        Scope (\_SB.PR00) {
            Method (_DSM, 4, NotSerialized) {
                If (LEqual (Arg2, Zero)) {
                    Return (Buffer (One) { 0x03 })
                }

                Return (Package (0x02) {
                    "plugin-type",
                    One
                })
            }
        }
    }
    
    If (CondRefOf (\_PR.C000)) {
        Scope (\_PR.C000) {
            Method (_DSM, 4, NotSerialized) {
                If (LEqual (Arg2, Zero)) {
                    Return (Buffer (One) { 0x03 })
                }

                Return (Package (0x02) {
                    "plugin-type",
                    One
                })
            }
        }
    }
    
    If (CondRefOf (\_PR.P000)) {
        Scope (\_PR.P000) {
            Method (_DSM, 4, NotSerialized) {
                If (LEqual (Arg2, Zero)) {
                    Return (Buffer (One) { 0x03 })
                }

                Return (Package (0x02) {
                    "plugin-type",
                    One
                })
            }
        }
    }

    If (CondRefOf (\_PR.PR00)) {
        Scope (\_PR.PR00) {
            Method (_DSM, 4, NotSerialized) {
                If (LEqual (Arg2, Zero)) {
                    Return (Buffer (One) { 0x03 })
                }

                Return (Package (0x02) {
                    "plugin-type",
                    One
                })
            }
        }
    }

    If (CondRefOf (\_SB.SCK0.CP00)) {
        Scope (\_SB.SCK0.CP00) {
            Method (_DSM, 4, NotSerialized) {
                If (LEqual (Arg2, Zero)) {
                    Return (Buffer (One) { 0x03 })
                }

                Return (Package (0x02) {
                    "plugin-type",
                    One
                })
            }
        }
    }

    If (CondRefOf (\_SB.SCK0.PR00)) {
        Scope (\_SB.SCK0.PR00) {
            Method (_DSM, 4, NotSerialized) {
                If (LEqual (Arg2, Zero)) {
                    Return (Buffer (One) { 0x03 })
                }

                Return (Package (0x02) {
                    "plugin-type",
                    One
                })
            }
        }
    }
}
