/*
 * Hyper-V VMBUS SSDT to enable ACPI node identification.
 * Requires the following ACPI patches:
 * (1) Base:     \_SB.VMOD
 *     Find:     _HID
 *     Replace:  XHID
 *     Count:    1
 * (2) Base:     \_SB.VMOD.VMBS
 *     Find:     _HID
 *     Replace:  XHID
 *     Count:    1
 * Note: This SSDT may conflict with Windows.
 */

DefinitionBlock ("", "SSDT", 2, "ACDT", "VMBUS", 0x00000000)
{
    External (\_SB.VMOD, DeviceObj)
    External (\_SB.VMOD.VMBS, DeviceObj)

    Scope (\_SB.VMOD)
    {
        Name (_HID, EisaId ("VMD0001"))
    }

    Scope (\_SB.VMOD.VMBS)
    {
        Name (_HID, EisaId ("VBS0001"))
    }
}
