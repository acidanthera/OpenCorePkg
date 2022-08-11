/*
 * Hyper-V CPU plugin-type SSDT to enable VMPlatformPlugin on Big Sur and newer.
 *
 * This SSDT must be loaded after SSDT-HV-DEV.dsl
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "HVPLUG", 0x00000000)
{
    External (\_SB.P001, ProcessorObj)

    Scope (\_SB.P001)
    {
        If (_OSI ("Darwin"))
        {
            Method (_DSM, 4, NotSerialized)  
            {
                If (LEqual (Arg2, Zero))
                {
                    Return (Buffer () { 0x03 })
                }

                Return (Package ()
                {
                    "plugin-type", 
                    0x02
                })
            }
        }
    }
}
