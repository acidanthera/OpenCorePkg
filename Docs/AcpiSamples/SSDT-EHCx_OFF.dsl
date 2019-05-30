/*
 * USB compatibility table.
 *
 * Attention!
 * Only for 7,8,9-series chipsets and 10.11 and newer!
 *
 * To disable EHC1 and EHC2 - set an option "XHCI Mode" to "Enabled" in yours BIOS.
 * If the "XHCI Mode" option is not available in yours BIOS or works incorrectly, then use this ACPI table.
 * Disabling through BIOS is preferable whenever possible.
 *
 * Note: for 7-series you need to use either "EH1D = One" or "EH2D = One" but not both!
 * This is because only one of the devices (EHC1 or EHC2) is used by macOS. Check the IOReg.
 */
DefinitionBlock ("", "SSDT", 2, "APPLE ", "EHCx_OFF", 0x00001000)
{
    Scope (\)
    {
        OperationRegion (RCRG, SystemMemory, 0xFED1F418, One)
        Field (RCRG, DWordAcc, Lock, Preserve)
        {
                ,   13, 
            EH2D,   1, 
                ,   1, 
            EH1D,   1
        }

        Method (_INI, 0, NotSerialized)  // _INI: Initialize
        {
            EH1D = One  // Disable EHC1
            EH2D = One  // Disable EHC2
        }
    }
}
