/*
 * Hyper-V VMBUS SSDT to enable ACPI node identification.
 *
 * AppleACPIPlatform requires standard _HID values for proper EFI device
 * path generation, and will not work with the default string values Hyper-V provides.
 *
 * Requires the following ACPI patches:
 * (1) Base:            \_SB.VMOD
 *     Comment:         _HID to XHID rename (Hyper-V VMOD)
 *     Count:           1
 *     Find:            5F484944 (_HID)
 *     Replace:         58484944 (XHID)
 *     TableSignature:  44534454 (DSDT)
 * (2) Base:            \_SB.VMOD.VMBS
 *     Comment:         _HID to XHID rename (Hyper-V VMBus)
 *     Count:           1
 *     Find:            5F484944 (_HID)
 *     Replace:         58484944 (XHID)
 *     TableSignature:  44534454 (DSDT)
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "HVVMBUS", 0x00000000)
{
    External (\_SB.VMOD, DeviceObj)
    External (\_SB.VMOD.XHID, MethodObj)
    External (\_SB.VMOD.VMBS, DeviceObj)
    External (\_SB.VMOD.VMBS.XHID, MethodObj)

    Scope (\_SB.VMOD)
    {
        Method (_HID, 0, NotSerialized)
        {
            If (_OSI ("Darwin"))
            {
                Return (EisaId ("VMD0001"))
            }

            Return (\_SB.VMOD.XHID())
        }
    }

    Scope (\_SB.VMOD.VMBS)
    {
        Method (_HID, 0, NotSerialized)
        {
            If (_OSI ("Darwin"))
            {
                Return (EisaId ("VBS0001"))
            }

            Return (\_SB.VMOD.VMBS.XHID())
        }
    }
}
