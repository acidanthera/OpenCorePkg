/*
 * Only necessary when B0D4 device is present in the (INTEL) DSDT.
 * For all intel generations, This Configuration Adjust B0D4 device Return (Fix DSDT Warnings and error).
 */
DefinitionBlock("", "SSDT", 2, "ACDT", "B0D4", 0x00000000)
{
    External (_SB_.PCI0.B0D4, DeviceObj)
    
    Scope (_SB.PCI0.B0D4)
    {    
        If (_OSI ("Darwin")) 
        {    
            Return (Zero)
        }
    }
}
