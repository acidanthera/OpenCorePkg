/*
 * This table provides an example of creating a missing ACPI device
 * to ensure early DeviceProperty application. In this example
 * a GPU device is created for a platform having an extra PCI
 * bridge in the path - PCI0.PEG0.PEGP.BRG0.GFX0:
 * PciRoot(0x0)/Pci(0x1,0x0)/Pci(0x0,0x0)/Pci(0x0,0x0)/Pci(0x0,0x0)
 * Such tables are particularly relevant for macOS 11.0 and newer.
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "BRG0", 0x00000000)
{
    External (_SB_.PCI0.PEG0.PEGP, DeviceObj)

    Scope (\_SB.PCI0.PEG0.PEGP)
    {
        /*
         * This is a PCI bridge device present on PEGP.
         * Normally seen as pci-bridge in I/O Registry.
         */
        Device (BRG0)
        {
            Name (_ADR, Zero)

            /*
             * This is an actual GPU device present on the bridge.
             * Normally seen as display in I/O Registry.
             */
            Device (GFX0)
            {
                Name (_ADR, Zero)  // _ADR: Address
            }
        }
    }
}
