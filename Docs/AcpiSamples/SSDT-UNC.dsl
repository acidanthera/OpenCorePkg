/*
 * Discovered on X99-series.
 * These platforms have uncore PCI bridges for 4 CPU sockets
 * present in ACPI despite having none physically.
 *
 * Under normal conditions these are disabled depending on
 * CPU presence in the socket via Processor Bit Mask (PRBM),
 * but on X99 this code is unused or broken as such bridges
 * simply do not exist. We fix that by writing 0 to PRBM.
 *
 * Doing so is important as starting with macOS 11 IOPCIFamily
 * will crash as soon as it sees non-existent PCI bridges.
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "UNC", 0x00000000)
{
    External (_SB.UNC0, DeviceObj)
    External (PRBM, IntObj)

    Scope (_SB.UNC0)
    {
        Method (_INI, 0, NotSerialized)
        {
            // In most cases this patch does benefit all operating systems,
            // yet on select pre-Windows 10 it may cause issues.
            // Remove If (_OSI ("Darwin")) in case you have none.
            If (_OSI ("Darwin")) {
                PRBM = 0
            }
        }
    }
}
