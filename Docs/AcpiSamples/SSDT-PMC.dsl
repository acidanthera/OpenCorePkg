/*
 * Intel 300-series PMC support for macOS
 *
 * Starting from Z390 chipsets PMC (D31:F2) is only available through MMIO.
 * Since there is no standard device for PMC in ACPI, Apple introduced its
 * own naming "APP9876" to access this device from AppleIntelPCHPMC driver.
 * To avoid confusion we disable this device for all other operating systems,
 * as they normally use another non-standard device with "PNP0C02" HID and
 * "PCHRESV" UID.
 *
 * On certain implementations, including APTIO V, PMC initialisation is
 * required for NVRAM access. Otherwise it will freeze in SMM mode.
 * The reason for this is rather unclear. Note, that PMC and SPI are
 * located in separate memory regions and PCHRESV maps both, yet only
 * PMC region is used by AppleIntelPCHPMC:
 * 0xFE000000~0xFE00FFFF - PMC MBAR
 * 0xFE010000~0xFE010FFF - SPI BAR0
 * 0xFE020000~0xFE035FFF - SerialIo BAR in ACPI mode
 *
 * PMC device has nothing to do to LPC bus, but is added to its scope for
 * faster initialisation. If we add it to PCI0, where it normally exists,
 * it will start in the end of PCI configuration, which is too late for
 * NVRAM support.
 */
DefinitionBlock ("", "SSDT", 2, "ACDT", "PMCR", 0x00001000)
{
    External (_SB_.PCI0.LPCB, DeviceObj)

    Scope (_SB.PCI0.LPCB)
    {
        Device (PMCR)
        {
            Name (_HID, EisaId ("APP9876"))  // _HID: Hardware ID
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (_OSI ("Darwin"))
                {
                    Return (0x0B)
                }
                Else
                {
                    Return (Zero)
                }
            }
            Name (_CRS, ResourceTemplate ()  // _CRS: Current Resource Settings
            {
                Memory32Fixed (ReadWrite,
                    0xFE000000,         // Address Base
                    0x00010000,         // Address Length
                    )
            })
        }
    }
}
