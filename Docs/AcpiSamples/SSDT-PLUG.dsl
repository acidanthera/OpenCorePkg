/*
 * XCPM power management compatibility table with Darwin method.
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
    External (_SB_.CPU0, ProcessorObj)
    External (_PR_.CPU0, ProcessorObj)
    External (_PR_.CP00, ProcessorObj)
    External (_PR_.C000, ProcessorObj)
    External (_PR_.P000, ProcessorObj)
    External (_SB_.PR00, ProcessorObj)
    External (_PR_.PR00, ProcessorObj)
    External (_SB_.SCK0.CP00, ProcessorObj)
    External (_SB_.SCK0.PR00, ProcessorObj)

    Method (PMPM, 4, NotSerialized) {
       If (LEqual (Arg2, Zero)) {
           Return (Buffer (One) { 0x03 })
       }

       Return (Package (0x02)
       {
           "plugin-type", 
           One
       })
    }

    If (CondRefOf (\_SB.CPU0)) {
        If ((ObjectType (\_SB.CPU0) == 0x0C)) {
            Scope (\_SB.CPU0) {
                If (_OSI ("Darwin")) {
                    Method (_DSM, 4, NotSerialized)  
                    {
                        Return (PMPM (Arg0, Arg1, Arg2, Arg3))
                    }
                }
            }
        }
    }
    
    If (CondRefOf (\_PR.CPU0)) {
        If ((ObjectType (\_PR.CPU0) == 0x0C)) {
            Scope (\_PR.CPU0) {
                If (_OSI ("Darwin")) {
                    Method (_DSM, 4, NotSerialized)  
                    {
                        Return (PMPM (Arg0, Arg1, Arg2, Arg3))
                    }
                }
            }
        }
    }

    If (CondRefOf (\_SB.PR00)) {
        If ((ObjectType (\_SB.PR00) == 0x0C)) {
            Scope (\_SB.PR00) {
                If (_OSI ("Darwin")) {
                    Method (_DSM, 4, NotSerialized)  
                    {
                        Return (PMPM (Arg0, Arg1, Arg2, Arg3))
                    }
                }
            }
        }
    }

    If (CondRefOf (\_PR.CP00)) {
        If ((ObjectType (\_PR.CP00) == 0x0C)) {
            Scope (\_PR.CP00) {
                If (_OSI ("Darwin")) {
                    Method (_DSM, 4, NotSerialized)  
                    {
                        Return (PMPM (Arg0, Arg1, Arg2, Arg3))
                    }
                }
            }
        }
    }

    If (CondRefOf (\_PR.C000)) {
        If ((ObjectType (\_PR.C000) == 0x0C)) {
            Scope (\_PR.C000) {
                If (_OSI ("Darwin")) {
                    Method (_DSM, 4, NotSerialized)  
                    {
                        Return (PMPM (Arg0, Arg1, Arg2, Arg3))
                    }
                }
            }
        }
    }
    
    If (CondRefOf (\_PR.P000)) {
        If ((ObjectType (\_PR.P000) == 0x0C)) {
            Scope (\_PR.P000) {
                If (_OSI ("Darwin")) {
                    Method (_DSM, 4, NotSerialized)  
                    {
                        Return (PMPM (Arg0, Arg1, Arg2, Arg3))
                    }
                }
            }
        }
    }
    
    If (CondRefOf (\_PR.PR00)) {
        If ((ObjectType (\_PR.PR00) == 0x0C)) {
            Scope (\_PR.PR00) {
                If (_OSI ("Darwin")) {
                    Method (_DSM, 4, NotSerialized)  
                    {
                        Return (PMPM (Arg0, Arg1, Arg2, Arg3))
                    }
                }
            }
        }
    }

    If (CondRefOf (\_SB.SCK0.CP00)) {
        If ((ObjectType (\_SB.SCK0.CP00) == 0x0C)) {
            Scope (\_SB.SCK0.CP00) {
                If (_OSI ("Darwin")) {
                    Method (_DSM, 4, NotSerialized)  
                    {
                        Return (PMPM (Arg0, Arg1, Arg2, Arg3))
                    }
                }
            }
        }
    }

    If (CondRefOf (\_SB.SCK0.PR00)) {
        If ((ObjectType (\_SB.SCK0.PR00) == 0x0C)) {
            Scope (\_SB.SCK0.PR00) {
                If (_OSI ("Darwin")) {
                    Method (_DSM, 4, NotSerialized)  
                    {
                        Return (PMPM (Arg0, Arg1, Arg2, Arg3))
                    }
                }
            }
        }
    }
}
